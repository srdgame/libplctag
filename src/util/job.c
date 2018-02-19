/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License  as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License  for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <platform.h>
#include <lib/libplctag.h>
#include <util/debug.h>
#include <util/job.h>
#include <util/mem.h>



//#ifdef _WIN32
//static DWORD __stdcall job_executor(LPVOID not_used);
//#else
//static void* job_executor(void* not_used);
//#endif


/* static vars for jobs. */

static mutex_p job_mutex = NULL;
static thread_p job_executor_thread = NULL;
static volatile int library_terminating = 0;
static job_p jobs = NULL;

struct job_t {
    job_p next;
    char *name;
    callback_func_t job_func;
    int status;
    job_result_t result;

    /* first arg is always the job itself. */
    void *arg2;
    void *arg3;
};

static int job_destroy(void *job_arg, void *arg2, void *arg3);
static int job_remove(job_p job);
static job_p next_job(job_p job);
static THREAD_FUNC(job_executor);



/*******************************************************************************
 ****************************** API functions **********************************
 ******************************************************************************/


job_p job_create(const char *name, callback_func_t job_func, callback_func_t job_cleanup_fun, void *arg2, void *arg3)
{
	job_p job = NULL;
    int rc = PLCTAG_STATUS_OK;
	
	if(!name || str_length(name) == 0) {
		pdebug(DEBUG_WARN,"Job must have a name.");
		return NULL;
	}
	
	if(!job_func) {
		pdebug(DEBUG_WARN,"Job must have a job function.");
		return NULL;
	}
	
	/* allocate it all at once */
	job = (job_p)rc_alloc(sizeof(struct job_t) + str_length(name) + 1, job_destroy);
	if(!job) {
		pdebug(DEBUG_WARN,"Unable to allocate space for job!");
		return NULL;
	}
    
    /* add the clean up function */
    rc = rc_add_cleanup(job, job_cleanup_fun, arg2, arg3);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Unable to add job clean up function!");
        rc_dec(job);
        return NULL;
    }
	
	/* copy the fields we have. */
    job->name = (char *)(job)+sizeof(struct job_t);
	str_copy(job->name, str_length(name), name);
	
	job->job_func = job_func;
    job->status = PLCTAG_STATUS_PENDING;
    
    /* arg1 is the job itself. */
    job->arg2 = arg2;
    job->arg3 = arg3;
		
	/* put the job on the list for execution */
	critical_block(job_mutex) {
		job->next = jobs;
		jobs = job;
	}

	return job;
}




int job_get_status(job_p job)
{
    pdebug(DEBUG_DETAIL,"Starting.");
    
	if(!job) {
		pdebug(DEBUG_WARN,"Job pointer was null!");
		return PLCTAG_ERR_NULL_PTR;
	}
    
    pdebug(DEBUG_DETAIL, "Done");

    return job->status;
}



int job_set_status(job_p job, int status)
{
    pdebug(DEBUG_DETAIL,"Starting.");
    
	if(!job) {
		pdebug(DEBUG_WARN,"Job pointer was null!");
		return PLCTAG_ERR_NULL_PTR;
	}
    
    job->status = status;
    
    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
}



job_result_t job_get_result(job_p job)
{
    pdebug(DEBUG_DETAIL,"Starting.");
    
	if(!job) {
        job_result_t result;
        
        /* zero the whole thing out */
        mem_set(&result, 0, sizeof(result));
        
		pdebug(DEBUG_WARN,"Job pointer was null!");
        
		return result;
	}
    
    pdebug(DEBUG_DETAIL, "Done");

    return job->result;
}


int job_set_result(job_p job, job_result_t result)
{
    pdebug(DEBUG_DETAIL,"Starting.");
    
	if(!job) {
		pdebug(DEBUG_WARN,"Job pointer was null!");
		return PLCTAG_ERR_NULL_PTR;
	}
    
    job->result = result;
    
    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
}



/*******************************************************************************
 ************************** Internal functions *********************************
 ******************************************************************************/





/*  
 * Callback when a job is destroyed.   Note that we do not acquire a reference within
 * the job queue.  When a job's external references are all released, the job will be
 * destroyed and that removes it from the queue.
 * 
 * In order for a job to remove itself, it must release all external references.   While
 * the job is running, the executor thread holds one final reference.   Once the job function
 * returns, the executor thread will release the reference thus calling job_destroy() and 
 * removing the job from the queue.
 * 
 * Memory is freed by the refcount system.
 */
int job_destroy(void *job_arg, void *arg2, void *arg3)
{
    job_p job = job_arg;
    
    (void)arg2;
    (void)arg3;

    if (!job) {
        pdebug(DEBUG_WARN, "Job pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_INFO, "Starting to destroy job %s.", job->name);

	/* make sure the job is removed from the queue. */
    job_remove(job);
     	 	 
    pdebug(DEBUG_INFO, "Done.");

    return PLCTAG_STATUS_OK;
}


int job_remove(job_p job)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting.");
    
	if(!job) {
		pdebug(DEBUG_WARN,"Job pointer was null!");
		return PLCTAG_ERR_NULL_PTR;
	}

    critical_block(job_mutex) {
        job_p *walker = &jobs;
        
        while(*walker && *walker != job) {
            walker = &((*walker)->next);
        }
        
        /* if we found the job, unlink it and release it. */
        if(*walker == job) {
            (*walker) = job->next;
            rc_dec(job);
        } else {
            pdebug(DEBUG_WARN,"Job not found!");
            rc = PLCTAG_ERR_NOT_FOUND;
        }
    }

    pdebug(DEBUG_INFO, "Done.");
    
    return rc;
}



/* called by the library init function */
int job_init()
{
	int rc = PLCTAG_STATUS_OK;
	
	pdebug(DEBUG_INFO,"Beginning job utility initialization.");
	
    /* this is a mutex used to synchronize most activities in this protocol */
    rc = mutex_create((mutex_p*)&job_mutex);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create job mutex!");
        return rc;
    }

    /* create the background IO handler thread */
    rc = thread_create((thread_p*)&job_executor_thread,job_executor, 32*1024, NULL);

    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_INFO,"Unable to create job executor thread!");
        return rc;
    }

    pdebug(DEBUG_INFO,"Finished initializing job utility.");

    return rc;
}


void job_teardown(void)
{
    pdebug(DEBUG_INFO,"Releasing global job utility resources.");

    pdebug(DEBUG_INFO,"Terminating job executor thread.");
    /* kill the IO thread first. */
    library_terminating = 1;

    /* wait for the thread to die */
    if(job_executor_thread) {
		thread_join(job_executor_thread);
		thread_destroy((thread_p*)&job_executor_thread);
	}

    pdebug(DEBUG_INFO,"Freeing global job mutex.");
    
    /* clean up the mutex */
    mutex_destroy((mutex_p*)&job_mutex);

    pdebug(DEBUG_INFO,"Done.");
}



job_p next_job(job_p job) 
{
    job_p result = NULL;
    
    critical_block(job_mutex) {
        if(!job) {
            result = jobs;
        } else {
            result = job->next;
        }
        
        if(result) {
            /* 
             * use assignment here so that if the pointer is already 
             * released by accident we do not keep passing it. 
             */
            result = rc_inc(result);
        }
    }
    
    return result;
}


THREAD_FUNC(job_executor)
{
	pdebug(DEBUG_INFO,"Job executor thread starting.");
    
    (void)arg;
	
	while(!library_terminating) {
		/*
		 * This thread is the only one that removes jobs from the queue.  This
		 * is what makes our brief holding of the lock below safe.
		 */
		job_p job = next_job(NULL);
		
        while(job) {
            job_p next = next_job(job);
            
            /* we ignore the return code. The job itself is the first argument. */
            job->job_func(job, job->arg2, job->arg3); 

            /* release the reference to the job.  This might clean it up. */
            rc_dec(job);
            
            job = next;
        }
        
        /* give up the CPU */
        sleep_ms(1);
	}
	
	pdebug(DEBUG_INFO,"Job executor thread stopping.");

    thread_stop();

    THREAD_RETURN(0);
}


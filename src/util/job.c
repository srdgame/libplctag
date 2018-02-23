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
#include <util/atomic.h>
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


MAKE_ATOMIC_TYPE(atomic_int, int)


struct job_t {
    job_p next;
    char *name;
    callback_func_t job_func;
    void *arg1;
    void *arg2;
    void *arg3;
    atomic_int running;
};

static void job_destroy(void *job_arg);
static int job_remove(job_p job);
static job_p next_job(job_p job);
static THREAD_FUNC(job_executor);



/*******************************************************************************
 ****************************** API functions **********************************
 ******************************************************************************/


job_p job_create(const char *name, callback_func_t job_func, void *arg1, void *arg2, void *arg3)
{
	job_p job = NULL;
	
	if(!name || str_length(name) == 0) {
		pdebug(DEBUG_WARN,"Job must have a name.");
		return NULL;
	}
	
	if(!job_func) {
		pdebug(DEBUG_WARN,"Job must have a job function.");
		return NULL;
	}
	
	/* allocate it all at once */
	job = (job_p)mem_alloc(sizeof(struct job_t) + str_length(name) + 1);
	if(!job) {
		pdebug(DEBUG_WARN,"Unable to allocate space for job!");
		return NULL;
	}
    
    job->name = (char*)(job+1);
    str_copy(job->name, str_length(name), name);
    
    job->arg1 = arg1;
    job->arg2 = arg2;
    job->arg3 = arg3;
    
    atomic_int_init(&job->running, 1);

	/* put the job on the list for execution */
	critical_block(job_mutex) {
		job->next = jobs;
		jobs = job;
	}

	return job;
}


int job_join(job_p job)
{
    if(!job) {
        pdebug(DEBUG_WARN,"Called with null job pointer!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    while(atomic_int_get(&job->running)) {
        sleep_ms(1);
    }
    
    mem_free(job);
    
    return PLCTAG_STATUS_OK;
}


/*******************************************************************************
 ************************** Internal functions *********************************
 ******************************************************************************/



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



THREAD_FUNC(job_executor)
{
    pdebug(DEBUG_INFO,"Job executor thread starting.");
    
    (void)arg;
	
	while(!library_terminating) {
        job_p job = NULL;
        int rc = JOB_RUN;
        int need_next = 1;
        
        do {
            if(need_next) {
                critical_block(job_mutex) {
                    if(!job) {
                        job = jobs;
                    } else {
                        job = job->next;
                    }
                }
            }

            if(job) {
                pdebug(DEBUG_SPEW,"Running job %s", job->name);
                
                rc = job->job_func(job->arg1, job->arg2, job->arg3); 
                
                if(rc == JOB_STOP) {
                    critical_block(job_mutex) {
                        job_p *walker = &jobs;
                        
                        while(*walker && *walker != job) {
                            walker = &((*walker)->next);
                        }
                        
                        /* if we found the job, unlink it. */
                        if(*walker == job) {
                            job_p next = job->next;
                            (*walker) = next;
                            need_next = 0;
                            atomic_int_set(&job->running, 0);
                            job = next;
                        } else {
                            pdebug(DEBUG_WARN,"Job not found!");
                        }
                    }
                }
            }
        } while(job);
        
        /* give up the CPU */
        sleep_ms(1);
	}
	
	pdebug(DEBUG_INFO,"Job executor thread stopping.");

    thread_stop();

    THREAD_RETURN(0);
}


/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
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



#ifdef _WIN32
static DWORD __stdcall job_executor(LPVOID not_used);
#else
static void* job_executor(void* not_used);
#endif

static void job_destroy(void *job_arg);

static mutex_p job_mutex = NULL;
static thread_p job_executor_thread = NULL;
volatile int library_terminating = 0;
static job_p jobs = NULL;



/* API functions */
job_p job_create(const char *name, void *context, int (*job_func)(job_p job, void *context), void (*job_destructor)(void *context))
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
	
	/* copy the fields we have. */
	str_copy((char *)(job)+sizeof(struct job_t), name, str_length(name));
	
	job->context = context;
	job->job_func = job_func;
	job->job_destructor = job_destructor;
	
	/* init the refcount so that this gets cleaned up. */
	job->rc = refcount_init(1, job, job_destroy);
	
	/* put the job on the list for execution */
	critical_block(job_mutex) {
		job->next = jobs;
		jobs = job;
	}

	return job;
}



/* must have the session mutex held here */
void job_destroy(void *job_arg)
{
    job_p job = job_arg;

    pdebug(DEBUG_INFO, "Starting.");

    if (!job) {
        pdebug(DEBUG_WARN, "Job pointer is null!");
        return;
    }

	/*
	 * There is a potential deadlock condition if we just remove the job ourselves.  This could be called from
	 * within a critical_block where the job mutex is held.   In that case we could get into deadlock, so we 
	 * mark the job as dead and let the job executor thread do the work.
	 * 
	 * The job executor calls the job destructor function and then cleans up the job structure.  The job destructor
	 * must do all the necessary job-specific clean up.
	 */
	 
	job->terminate = 1;
	 
    pdebug(DEBUG_INFO, "Done.");

    return;
}


/* 
 * WARNING - Only use this for the current job!
 */
 
void job_set_context(job_p job, void *context)
{
	if(job) {
		job->context = context;
	}
}

int job_acquire(job_p job)
{
    if(!job) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    pdebug(DEBUG_INFO,"Acquire job.");

    return refcount_acquire(&job->rc);
}


int job_release(job_p job)
{
    if(!job) {
        return PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_INFO,"Release job.");

    return refcount_release(&job->rc);
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




#ifdef _WIN32
DWORD __stdcall job_executor(LPVOID not_used)
#else
void* job_executor(void* not_used)
#endif
{
	pdebug(DEBUG_INFO,"Job executor thread starting with arg %p",not_used);
	
	while(!library_terminating || jobs) {
		/*
		 * This thread is the only one that removes jobs from the queue.  This
		 * is what makes our brief holding of the lock below safe.
		 */
		job_p job;
		
		critical_block(job_mutex) {
			job = jobs;
		}
		
		while(job) {
			job_p next = job->next;
			
			/* if this job is dying call the destructor, or if the library is being torn down. */
			if(job->terminate || library_terminating) {  /* FIXME - should library termination be a factor here? */
				if(job->job_destructor) {
					job->job_destructor(job->context);
				}
				
				/* now remove the job from the list */
				critical_block(job_mutex) {
					job_p *walker = &jobs;
					
					while(*walker && *walker != job) {
						walker = &((*walker)->next);
					}
					
					if(*walker) {
						*walker = job->next;
					} /* else not found?? */
				}
				
				/* all done */
				mem_free(job);
			} else {
				/* this is a normal job.  Make sure we should run it. */
				if(!job->is_zombie) {
					int rc = job->job_func(job,job->context);
					
					if(rc == JOB_STOP) {
						job->is_zombie = 1;
					}
				}
			}

			job = next;
		}
		
		/* give up the CPU */
		sleep_ms(1);
	}
	
	pdebug(DEBUG_INFO,"Job executor thread stopping.");

    thread_stop();

    /* FIXME -- this should be factored out as a platform dependency.*/
#ifdef _WIN32
    return (DWORD)0;
#else
    return NULL;
#endif
}


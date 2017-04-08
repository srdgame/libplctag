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

static mutex_p job_mutex = NULL;
static thread_p job_executor_thread = NULL;
volatile int library_terminating = 0;


/* called by the library init function */
int job_init()
{
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


void job_teardown()
{
    pdebug(DEBUG_INFO,"Releasing global job utility resources.");

    pdebug(DEBUG_INFO,"Terminating job executor thread.");
    /* kill the IO thread first. */
    library_terminating = 1;

    /* wait for the thread to die */
    if(job_executor_thread) {
		thread_join(job_executor_thread);
		thread_destroy((thread_p*)&io_handler_thread);
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


    thread_stop();

    /* FIXME -- this should be factored out as a platform dependency.*/
#ifdef _WIN32
    return (DWORD)0;
#else
    return NULL;
#endif
}


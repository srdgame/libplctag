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

#include <stdlib.h>
#include <platform.h>
#include <lib/init.h>
#include <lib/lib.h>
#include <lib/teardown.h>
#include <util/attr.h>
#include <util/debug.h>
#include <util/job.h>
#include <ab/ab.h>
#include <system/system.h>



/*
 * initialize_modules() is called the first time any kind of tag is
 * created.  It will be called before the tag creation routines are
 * run.
 */


int initialize_modules(void)
{
    int rc = PLCTAG_STATUS_OK;

    /* loop until we get the lock flag */
    while (!lock_acquire((lock_t*)&library_initialization_lock)) {
        sleep_ms(1);
    }

    if(!library_initialized) {
        pdebug(DEBUG_INFO,"Initializinglibrary modules.");

		/* first see if the mutex is there. */
		if (!global_library_mutex) {
			rc = mutex_create((mutex_p*)&global_library_mutex);

			if (rc != PLCTAG_STATUS_OK) {
				pdebug(DEBUG_ERROR, "Unable to create global tag mutex!");
			}
		}

		/*
		 * Other initialization code goes below.
		 */

		/* set up the thread for the job utility. */
        if(rc == PLCTAG_STATUS_OK) {
            rc = job_init();
        }

		/* initialize the Allen-Bradley protocol stack. */
        if(rc == PLCTAG_STATUS_OK) {
            rc = ab_init();
        }

        library_initialized = 1;

        /* hook the destructor */
        atexit(destroy_modules);

        pdebug(DEBUG_INFO,"Done initializing library modules.");
    }

    /* we hold the lock, so clear it.*/
    lock_release((lock_t*)&library_initialization_lock);

    return rc;
}


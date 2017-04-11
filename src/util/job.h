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

#ifndef __PLCTAG_UTIL_JOB_H__
#define __PLCTAG_UTIL_JOB_H__

#include <platform.h>
#include <util/refcount.h>


/* called by the library init function */
extern int job_init();
extern void job_teardown();

/*
 * job_create
 * 
 * Create a new job.  It will be scheduled and continue to run until it returns
 * JOB_STOP.  The job is responsible for cleaning up the context.  The job function
 * must return JOB_CONTINUE in order to keep being scheduled.
 * 
 * Jobs MUST NOT BLOCK!   They are not at all preemptive.
 */

typedef struct job_t *job_p;
 
struct job_t {
	struct job_t *next;
	const char *name;
	void *context;
	refcount rc;
	int terminate;
	int is_zombie;
	int (*job_func)(job_p job, void *context);
	void (*job_destructor)(void *context);
}; 
 

extern job_p job_create(const char *name, void *context, int (*job_func)(job_p job, void *context), void (*job_destructor)(void *context));
extern void job_set_context(job_p job, void *context);
extern int job_acquire(job_p job);
extern int job_release(job_p job);

#define JOB_STOP 		(0)
#define JOB_CONTINUE 	(1)



#endif

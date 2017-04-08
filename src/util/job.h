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

#ifndef __PLCTAG_UTIL_JOB_H__
#define __PLCTAG_UTIL_JOB_H__

/* called by the library init function */
extern int job_init();
extern int job_teardown();

/*
 * job_create
 * 
 * Create a new job.  It will be scheduled and continue to run until it returns
 * JOB_STOP.  The job is responsible for cleaning up the context.  The job function
 * must return JOB_CONTINUE in order to keep being scheduled.
 * 
 * Jobs MUST NOT BLOCK!   They are not at all preemptive.
 */
 
extern int job_create(const char *name, void *context, int (*job_func)(void *context, int terminate));

#define JOB_STOP (0)
#define JOB_CONTINUE (1)

#endif

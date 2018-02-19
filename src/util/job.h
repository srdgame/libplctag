/***************************************************************************
 *   Copyright (C) 2018 by Kyle Hayes                                      *
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

#pragma once

#include <platform.h>
#include <util/refcount.h>


typedef struct job_t *job_p;

/*
 * Create and schedule a job.   Jobs are removed
 * when they are rc_dec'ed.  There will be at least
 * two references to a job when it is running because
 * the runner thread will acquire a reference when it
 * runs a job.
 * 
 * Jobs are ref counted objects and can have additional clean up functions
 * added to them.
 */

typedef union {
    void *ptr_res;
    int int_res;
} job_result_t;

extern job_p job_create(const char *name, callback_func_t job_func, callback_func_t job_cleanup_fun, void *arg2, void *arg3);
extern int job_get_status(job_p job);
extern int job_set_status(job_p job, int status);
extern job_result_t job_get_result(job_p job);
extern int job_set_result(job_p job, job_result_t result);


/* called by the library init function */
extern int job_init();
extern void job_teardown();


/***************************************************************************
 *   Copyright (C) 2016 by Kyle Hayes                                      *
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


#ifndef __UTIL__REFCOUNT_H__
#define __UTIL__REFCOUNT_H__ 1

#include <platform.h>
//#include <util/callback.h>

typedef void (*rc_cleanup_func)(void *arg);

//extern void *rc_alloc2(int size, callback_func_t cleanup_func, void *arg1, void *arg2);
//extern void *rc_alloc1(int size, callback_func_t cleanup_func, void *arg1);
extern void *rc_alloc(int size, rc_cleanup_func cleanup_func);
extern void *rc_inc(void *data);
extern void *rc_dec(void *data);
extern void *rc_weak_inc(void *data);
extern void *rc_weak_dec(void *data);
//extern int rc_add_cleanup(void *data, callback_func_t cleanup_func, void *arg1, void *arg2);
//extern void rc_free(const void *data);
//extern int rc_count(const void *data);


#endif

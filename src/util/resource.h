/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License as        *
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

#ifndef __UTIL_RESOURCE_H__
#define __UTIL_RESOURCE_H__ 1

#include <util/macros.h>
#include <util/refcount.h>

extern void *resource_get(const char *name);
extern int resource_put(const char *name, void *resource);
extern int resource_remove(const char *name);

#define resource_make_name(...) resource_make_name_impl(COUNT_NARG(__VA_ARGS__), __VA_ARGS__)
extern char *resource_make_name_impl(int num_args, ...);


extern int resource_service_init(void);
extern void resource_service_teardown(void);



#endif

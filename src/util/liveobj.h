/***************************************************************************
 *   Copyright (C) 2018 by Kyle Hayes                                      *
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


#pragma once

#include <platform.h>
#include <util/refcount.h>

#define LIVEOBJ_TYPE_FREE (0)

typedef struct liveobj_t *liveobj_p;

typedef void (*liveobj_func)(liveobj_p obj);
typedef int (*liveobj_find_func)(liveobj_p obj, int type, void *arg);

struct liveobj_t {
    int id;
    int type;
    liveobj_func obj_func;
};



extern int liveobj_add(liveobj_p obj, int type, liveobj_func obj_func);
extern liveobj_p liveobj_get(int id);
extern int liveobj_remove(int id);
extern liveobj_p liveobj_find(liveobj_find_func finder, void *arg);

extern int liveobj_setup();
extern void liveobj_teardown();

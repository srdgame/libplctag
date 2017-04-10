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

#ifndef __PLCTAG_LIB_TAG_H__
#define __PLCTAG_LIB_TAG_H__

#include <platform.h>
#include <lib/libplctag.h>
#include <util/attr.h>

typedef struct plc_tag_t *plc_tag_p;

/* define tag operation functions */
typedef int (*tag_vtable_func)(plc_tag_p tag);

/* we'll need to set these per protocol type. */
struct tag_vtable_t {
    tag_vtable_func abort;
    tag_vtable_func destroy;
    tag_vtable_func read;
    tag_vtable_func write;
};

typedef struct tag_vtable_t *tag_vtable_p;


#define TAG_INTERNALS                                                  \
    int tag_id;                                                        \
    mutex_p mut;                                                       \
    int64_t read_cache_expire;                                         \
    int64_t read_cache_ms;                                             \
    int status;                                                        \
	tag_vtable_p vtable;                                               \
    int size;                                                          \
    uint8_t *data;                                                     \
    uint8_t int16_byte_order[2];                                       \
    uint8_t uint16_byte_order[2];                                      \
    uint8_t int32_byte_order[4];                                       \
    uint8_t uint32_byte_order[4];                                      \
    uint8_t int64_byte_order[8];                                       \
    uint8_t uint64_byte_order[8];                                      \
    uint8_t float_byte_order[4];                                       \
    uint8_t double_byte_order[8];                                      
                                                                      

/* This is needed for the generic functions */
struct plc_tag_t {
	TAG_INTERNALS
};


typedef plc_tag_p (*tag_create_function)(attr attributes);


/* useful functions */
extern int plc_tag_destroy_mapped(plc_tag_p tag);
extern int plc_tag_abort_mapped(plc_tag_p tag);



#endif

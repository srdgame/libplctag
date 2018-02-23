/***************************************************************************
 *   Copyright (C) 2015 by OmanTek                                         *
 *   Author Kyle Hayes  kylehayes@omantek.com                              *
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

/**************************************************************************
 * CHANGE LOG                                                             *
 *                                                                        *
 * 2012-02-23  KRH - Created file.                                        *
 *                                                                        *
 **************************************************************************/


#ifndef __LIBPLCTAG_TAG_H__
#define __LIBPLCTAG_TAG_H__




#include <lib/libplctag.h>
#include <platform.h>
#include <util/attr.h>

//#define PLCTAG_CANARY (0xACA7CAFE)
//#define PLCTAG_DATA_LITTLE_ENDIAN   (0)
//#define PLCTAG_DATA_BIG_ENDIAN      (1)

extern const char *VERSION;
extern const int VERSION_ARRAY[3];

extern mutex_p global_library_mutex;

typedef struct plc_tag_t *plc_tag_p;


/* define tag operation functions */
//typedef int (*tag_abort_func)(plc_tag_p tag);
//typedef int (*tag_destroy_func)(plc_tag_p tag);
//typedef int (*tag_read_func)(plc_tag_p tag);
//typedef int (*tag_status_func)(plc_tag_p tag);
//typedef int (*tag_write_func)(plc_tag_p tag);

typedef int (*tag_vtable_func)(plc_tag_p tag);
typedef int (*tag_get_int_func)(plc_tag_p tag, int offset, int size, int64_t *val);
typedef int (*tag_set_int_func)(plc_tag_p tag, int offset, int size, int64_t val);
typedef int (*tag_get_float_func)(plc_tag_p tag, int offset, int size, double *val);
typedef int (*tag_set_float_func)(plc_tag_p tag, int offset, int size, double val);



/* 
 * Each protocol must implement the following functions.
 * 
 * abort and get_status are required.
 */
struct tag_vtable_t {
    tag_vtable_func abort;
    tag_vtable_func read;
    tag_vtable_func write;
    tag_get_int_func get_int;
    tag_set_int_func set_int;
    tag_get_float_func get_float;
    tag_set_float_func set_float;
    tag_vtable_func get_status;
    tag_vtable_func get_size;
};

typedef struct tag_vtable_t *tag_vtable_p;


/*
 * The base definition of the tag structure.  This is used
 * by the protocol-specific implementations.
 *
 * The base type only has a vtable for operations.
 */

#define TAG_BASE_STRUCT tag_vtable_p vtable; \
                        mutex_p api_mutex; \
                        mutex_p external_mutex; \
                        int id; \
                        int64_t read_cache_expire_ms; \
                        int64_t read_cache_duration_ms

//struct plc_tag_dummy {
//    int tag_id;
//};

struct plc_tag_t {
    TAG_BASE_STRUCT;
};

#define PLC_TAG_P_NULL ((plc_tag_p)0)


/* the following may need to be used where the tag is already mapped or is not yet mapped */
//extern int lib_init(void);
//extern void lib_teardown(void);
//extern int plc_tag_abort_mapped(plc_tag_p tag);
//extern int plc_tag_destroy_mapped(plc_tag_p tag);
//extern int plc_tag_status_mapped(plc_tag_p tag);

/*
 * Set up the generic parts of a tag.
 */
extern int plc_tag_init(plc_tag_p tag, attr attribs, tag_vtable_p vtable);
extern int plc_tag_deinit(plc_tag_p tag);


#endif

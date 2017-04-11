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


#include <lib/libplctag.h>
#include <platform.h>
#include <lib/tag.h>
#include <util/attr.h>
#include <util/debug.h>
#include <ab/tag.h>
#include <ab/session.h>


static int tag_abort(plc_tag_p tag);
static int tag_destroy(plc_tag_p tag);
static int tag_read(plc_tag_p tag);
static int tag_write(plc_tag_p tag);


plc_tag_p ab_logix_unconn_create(attr attribs)
{
	int elem_count = 1;
	const char *name = NULL;
	session_p session = NULL;
	ab_tag_p tag = NULL;
	
	pdebug(DEBUG_INFO,"Starting.");

	/* check all the inputs we can. */
	elem_count = attr_get_int(attribs,"elem_count",1);
	name = attr_get_str(attribs,"name",NULL);
	
	if(!name || str_length(name) == 0) {
		pdebug(DEBUG_WARN,"Tag name missing or zero length!");
		return NULL;
	}
	
	/* allocate memory for the tag. */
	tag = (ab_tag_p)mem_alloc(sizeof(struct ab_tag_t));
	if(!tag) {
		pdebug(DEBUG_WARN,"Error trying to allocate memory for new tag!");
		return NULL;
	}
	
	/* set up the vtable */
	tag->vtable = &vtable;
	
	
	pdebug(DEBUG_INFO,"Done.");
	return tag;
}




static int tag_abort(plc_tag_p tag);
static int tag_destroy(plc_tag_p tag);
static int debug_tag_read(plc_tag_p tag);
static int debug_tag_write(plc_tag_p tag);
static int version_tag_read(plc_tag_p tag);
static int version_tag_write(plc_tag_p tag);

struct tag_vtable_t debug_vtable = { tag_abort, tag_destroy, debug_tag_read, debug_tag_write};
struct tag_vtable_t version_vtable = { tag_abort, tag_destroy, version_tag_read, version_tag_write};


plc_tag_p system_tag_create(attr attribs)
{
    system_tag_p tag = NULL;
    const char *name = attr_get_str(attribs, "name", NULL);
    size_t tag_size = sizeof(struct system_tag_t);
    int i;

    pdebug(DEBUG_INFO,"Starting.");

    /* check the name, if none given, punt. */
    if(!name || str_length(name) < 1) {
        pdebug(DEBUG_ERROR, "System tag name is empty or missing!");
        return NULL;
    }

    pdebug(DEBUG_DETAIL,"Creating special tag %s", name);

    /*
     * allocate memory for the new tag.  Do this first so that
     * we have a vehicle for returning status.
     */

    tag = (system_tag_p)mem_alloc(tag_size);

    if(!tag) {
        pdebug(DEBUG_ERROR,"Unable to allocate memory for system tag!");
        return NULL;
    }
    
    /* point data at the backing store. */
    tag->data = &tag->backing_data[0];
    tag->size = (int)sizeof(tag->backing_data);

	/* set up the byte order arrays.  little endian for convenience. */
	for(i=0; i < (int)sizeof(tag->int64_byte_order); i++) {
		if(i < (int)sizeof(tag->int16_byte_order)) {
			tag->int16_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->int32_byte_order)) {
			tag->int32_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->int64_byte_order)) {
			tag->int64_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->float_byte_order)) {
			tag->float_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->double_byte_order)) {
			tag->double_byte_order[i] = i;
		}
	}

    /* get the name and copy it */
    str_copy(tag->name, name, (int)(sizeof(tag->name)));
    
    /* set the v-table based on the tag name */
    if(str_cmp_i(tag->name,"debug") == 0) {
		tag->vtable = &debug_vtable;
	} 
	
    if(str_cmp_i(tag->name,"version") == 0) {
		tag->vtable = &version_vtable;
	} else {
		pdebug(DEBUG_WARN,"Usupported system tag %s",tag->name);
		mem_free(tag);
		return NULL;
	}
	

    pdebug(DEBUG_INFO,"Done");

    return (plc_tag_p)tag;
}


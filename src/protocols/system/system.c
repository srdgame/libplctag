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

#include <platform.h>
#include <util/debug.h>
#include <util/attr.h>
#include <lib/tag.h>
#include <lib/libplctag.h>
#include <lib/lib.h>
#include <system/tag.h>
#include <lib/init.h>


/* we'll need to set these per protocol type.
struct tag_vtable_t {
    tag_abort_func          abort;
    tag_destroy_func        destroy;
    tag_read_func           read;
    tag_write_func          write;
};
*/

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



/* Generic Tag Functions */

static int tag_abort(plc_tag_p tag)
{
    /* there are no outstanding operations, so everything is OK. */
    tag->status = PLCTAG_STATUS_OK;
    return PLCTAG_STATUS_OK;
}



static int tag_destroy(plc_tag_p ptag)
{
    system_tag_p tag = (system_tag_p)ptag;

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    mem_free(tag);

    return PLCTAG_STATUS_OK;
}



/* DEBUG tag */

int debug_tag_read(plc_tag_p ptag)
{
    system_tag_p tag = (system_tag_p)ptag;
	int debug_level;
	
    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

	debug_level = get_debug_level();

	int32_encode(&tag->data[0], MAX_SYSTEM_TAG_SIZE, 0, tag->int32_byte_order, debug_level);

	pdebug(DEBUG_INFO,"Done.");
	
	return PLCTAG_STATUS_OK;
}


int debug_tag_write(plc_tag_p ptag)
{
    system_tag_p tag = (system_tag_p)ptag;
	int rc = 0;
	int debug_level = 0;
	
    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    rc = int32_decode(&tag->data[0], MAX_SYSTEM_TAG_SIZE, 0, tag->int32_byte_order, &debug_level);

    if(rc < 0) {
		return PLCTAG_ERR_WRITE;
	} else {
        set_debug_level(debug_level);
	}
	
	pdebug(DEBUG_INFO,"Done.");
	
	return PLCTAG_STATUS_OK;
}




int version_tag_read(plc_tag_p ptag)
{
    system_tag_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

	pdebug(DEBUG_DETAIL,"Version is %s",VERSION);
	str_copy((char *)(&tag->data[0]), VERSION, str_length(VERSION));
	tag->data[str_length(VERSION)] = 0;

	pdebug(DEBUG_INFO,"Done.");
	
	return PLCTAG_STATUS_OK;
}



int version_tag_write(plc_tag_p ptag)
{
    system_tag_p tag = (system_tag_p)ptag;

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

	pdebug(DEBUG_INFO,"Done.");
	
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


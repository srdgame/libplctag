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
//#include <system/tag.h>



#define MAX_SYSTEM_TAG_NAME (20)
#define MAX_SYSTEM_TAG_SIZE (30)


struct system_tag_debug_t {
    TAG_BASE_STRUCT;

    int debug_level;
};

typedef struct system_tag_debug_t *system_tag_debug_p;



struct system_tag_version_t {
    TAG_BASE_STRUCT;
    
    int version_data[3]; /* MAGIC */
};

typedef struct system_tag_version_t *system_tag_version_p;


static int tag_destroy(void *tag_arg, void *arg2, void *arg3);


static int system_tag_abort(plc_tag_p tag);
static int system_tag_status(plc_tag_p tag);

static int system_tag_read_debug(plc_tag_p tag);
static int system_tag_write_debug(plc_tag_p tag);
static int system_get_int_debug(plc_tag_p tag, int offset, int size, int64_t *val);
static int system_set_int_debug(plc_tag_p tag, int offset, int size, int64_t val);
static int system_tag_size_debug(plc_tag_p tag);

struct tag_vtable_t system_tag_debug_vtable = { system_tag_abort, system_tag_read_debug, system_tag_write_debug, system_get_int_debug, system_set_int_debug, NULL, NULL, system_tag_status, system_tag_size_debug};



static int system_tag_read_version(plc_tag_p tag);
static int system_get_int_version(plc_tag_p tag, int offset, int size, int64_t *val);
static int system_tag_size_version(plc_tag_p tag);

struct tag_vtable_t system_tag_version_vtable = { system_tag_abort, system_tag_read_version, NULL, system_get_int_version, NULL, NULL, NULL, system_tag_status, system_tag_size_version};



int system_tag_create(attr attribs, plc_tag_p *result)
{
    int rc = PLCTAG_STATUS_OK;
    const char *name = attr_get_str(attribs, "name", NULL);

    pdebug(DEBUG_INFO,"Starting.");

    /* check the name, if none given, punt. */
    if(!name || str_length(name) < 1) {
        pdebug(DEBUG_ERROR, "System tag name is empty or missing!");
        *result = NULL;
        return PLCTAG_ERR_BAD_PARAM;
    }

    pdebug(DEBUG_DETAIL,"Creating special tag %s", name);

    if(str_cmp_i(name,"debug") == 0) {
        system_tag_debug_p tag = (system_tag_debug_p)rc_alloc(sizeof(*tag), tag_destroy);
        if(!tag) {
            pdebug(DEBUG_ERROR,"Unable to create new debug system tag!");
            *result = NULL;
            return PLCTAG_ERR_NO_MEM;
        }
        
        rc = plc_tag_init((plc_tag_p)tag, attribs, &system_tag_debug_vtable);
        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Unable to initialize common tag fields!");
            rc_dec(tag);
            *result = NULL;
            return rc;
        }
        
        *result = (plc_tag_p)tag;
    } else if(str_cmp_i(name,"version") == 0) {
        system_tag_version_p tag = (system_tag_version_p)rc_alloc(sizeof(*tag), tag_destroy);
        if(!tag) {
            pdebug(DEBUG_ERROR,"Unable to create new version system tag!");
            *result = NULL;
            return PLCTAG_ERR_NO_MEM;
        }
        
        rc = plc_tag_init((plc_tag_p)tag, attribs, &system_tag_version_vtable);
        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Unable to initialize common tag fields!");
            rc_dec(tag);
            *result = NULL;
            return rc;
        }
        
        *result = (plc_tag_p)tag;
    } else {
        pdebug(DEBUG_WARN,"Unsupported system tag %s!", name);
        *result = NULL;
        rc = PLCTAG_ERR_NOT_IMPLEMENTED;
    }
    
    pdebug(DEBUG_INFO,"Done");
    
    return rc;
}



/*******************************************************************************
 *************************** Implementation Functions **************************
 *******************************************************************************/




int tag_destroy(void *tag_arg, void *arg2, void *arg3)
{
    system_tag_p tag = (system_tag_p)tag_arg;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO,"Starting")

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    rc = plc_tag_deinit((plc_tag_p)tag);
    
    pdebug(DEBUG_INFO,"Done.");
    
    return rc;
}



/* generic tag routines */


int system_tag_abort(plc_tag_p tag)
{
    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    return PLCTAG_STATUS_OK;
}



static int system_tag_status(plc_tag_p tag)
{
    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    return PLCTAG_STATUS_OK;
}




/* debug tag routines */


int system_tag_read_debug(plc_tag_p ptag)
{
    system_tag_debug_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    tag->debug_level = get_debug_level();
    
    pdebug(DEBUG_INFO,"Done.");

    return PLCTAG_STATUS_OK;
}



int system_tag_write_debug(plc_tag_p ptag)
{
    system_tag_debug_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    set_debug_level(tag->debug_level);

    pdebug(DEBUG_INFO,"Done.");

    return PLCTAG_STATUS_OK;
}




static int system_get_int_debug(plc_tag_p tag, int offset, int size, int64_t *val)
{
    system_tag_debug_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag || !val) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset != 0) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    if(size != 4) {
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    *val = (int64_t)(tag->debug_level);

    pdebug(DEBUG_INFO,"Done.");

    return PLCTAG_STATUS_OK;
}




static int system_set_int_debug(plc_tag_p tag, int offset, int size, int64_t val)
{
    system_tag_debug_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset != 0) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    if(size != 4) {
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }
    
    if(val < 0 || val > DEBUG_SPEW) {
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    tag->debug_level = (int)val;

    pdebug(DEBUG_INFO,"Done.");

    return PLCTAG_STATUS_OK;
}



static int system_tag_size_debug(plc_tag_p tag)
{
    system_tag_debug_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_INFO,"Done.");

    return 4;
}



/* version tag routines. */

static int system_tag_read_version(plc_tag_p tag)
{
    system_tag_version_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    tag->version_data[0] = VERSION_ARRAY[0];
    tag->version_data[1] = VERSION_ARRAY[1];
    tag->version_data[2] = VERSION_ARRAY[2];

    pdebug(DEBUG_INFO,"Done.");

    return PLCTAG_STATUS_OK;
}



static int system_get_int_version(plc_tag_p tag, int offset, int size, int64_t *val)
{
    int rc = PLCTAG_STATUS_OK;
    system_tag_version_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag || !val) {
        return PLCTAG_ERR_NULL_PTR;
    }
        
    if(size != 4) {
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    switch(offset) {
        case 0:
            *val = (int64_t)(tag->version_data[0]);
            break;
            
        case 4:
            *val = (int64_t)(tag->version_data[1]);
            break;
        
        case 8:
            *val = (int64_t)(tag->version_data[2]);
            break;
        
        default:
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            *val = INT64_MIN;
            break;
    }

    pdebug(DEBUG_INFO,"Done.");

    return rc;
}



static int system_tag_size_version(plc_tag_p tag)
{
    system_tag_version_p tag = (system_tag_p)ptag;

    pdebug(DEBUG_INFO,"Starting.");

    if(!tag) {
        return PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_INFO,"Done.");

    return 12;
}



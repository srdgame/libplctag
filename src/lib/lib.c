/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
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

#include <lib/libplctag.h>
#include <platform.h>
#include <lib/lib.h>
#include <ab/ab.h>
#include <system/system.h>


/*
 * Library-wide globals.
 */
 
const char *VERSION="2.0.0";

lock_t library_initialization_lock = LOCK_INIT;
volatile int library_initialized = 0;
mutex_p global_library_mutex = NULL;




#define MAX_FIELD_MATCH (5)

struct tag_type_map_t {
	struct {
		const char *name;
		const char *value;
	} fields[MAX_FIELD_MATCH];
	const tag_create_function tag_constructor;
};




/*
 * Add new tag constructors to the table below.
 */



struct tag_type_map_t tag_type_map[] = {
	/* Allen-Bradley tag types */
    //{ {{"make","allen-bradley"}, {"family","controllogix"}, {"use_connection","1"}, {"read_group","*"}, {NULL,NULL}}, ab_logix_conn_group_create},
    //{ {{"make","allen-bradley"}, {"family","controllogix"}, {"use_connection","1"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_conn_create},
    //{ {{"make","allen-bradley"}, {"family","controllogix"}, {"read_group","*"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_group_create},
    { {{"make","allen-bradley"}, {"family","controllogix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_create},
    //{ {{"make","allen-bradley"}, {"family","compactlogix"}, {"use_connection","1"}, {"read_group","*"}, {NULL,NULL}}, ab_logix_conn_group_create},
    //{ {{"make","allen-bradley"}, {"family","compactlogix"}, {"use_connection","1"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_conn_create},
    //{ {{"make","allen-bradley"}, {"family","compactlogix"}, {"read_group","*"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_group_create},
    //{ {{"make","allen-bradley"}, {"family","compactlogix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_create},
    //{ {{"make","allen-bradley"}, {"family","micro800"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_micro800_create},
    //{ {{"make","allen-bradley"}, {"family","micrologix"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    //{ {{"make","allen-bradley"}, {"family","micrologix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    //{ {{"make","allen-bradley"}, {"family","slc"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    //{ {{"make","allen-bradley"}, {"family","slc"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    //{ {{"make","allen-bradley"}, {"family","plc5"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    //{ {{"make","allen-bradley"}, {"family","plc5"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    
    /* special system tags. */
    { {{"make","system"}, {"family","library"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, system_tag_create},
};






/*
 * find_tag_create_func()
 *
 * Find an appropriate tag creation function.  This scans through the array
 * above to find a matching tag creation type.  The first match is returned.
 * A passed set of options will match when all non-null entries in the list
 * match.  This means that matches must be ordered from least to most general.
 */

tag_create_function find_tag_create_func(attr attributes)
{
    int entry = 0;
    int field = 0;
    int failed = 0;
	int num_entries = sizeof(tag_type_map)/sizeof(tag_type_map[0]);
	
	pdebug(DEBUG_INFO,"Starting.");
	
	/* match the attributes with the required attributes. */
	for(entry = 0; entry < num_entries; entry++) {
		failed = 0;
		
		pdebug(DEBUG_DETAIL,"Checking entry %d",entry);
		
		/* check each field for a match.  If one does not match, then immediately fail. */
		for(field = 0; !failed && field < MAX_FIELD_MATCH; field++) {
			const char *name = tag_type_map[entry].fields[field].name;

			/* only check valid fields */
			if(name) {
				const char *match_val = tag_type_map[entry].fields[field].value;
				const char *tag_val = attr_get_str(attributes,name,NULL);
				
				pdebug(DEBUG_DETAIL,"Checking %s for value %s given value %s",name, match_val, tag_val);
			
				/* if we are matching a wildcard, then check that the attributes have a value */
				if(str_cmp_i(match_val,"*") == 0) {
					if(!tag_val || str_length(tag_val) == 0) {
						failed = 1;
					}
				} else {
					/* it is not a wildcard, we must match exactly */
					if(str_cmp_i(match_val, tag_val)) {
						failed = 1;
					}
				}
			} else {
				/* we ran out of things to check. */
				break;
			}
		}
		
		if(!failed) {
			return tag_type_map[entry].tag_constructor;
		}
	}
	
	return NULL;
}


/*****************************************************************************************************
 *****************************  Support routines for extra indirection *******************************
 ****************************************************************************************************/


#define TAG_ID_MASK (0xFFFFFFF)
#define TAG_INDEX_MASK (0x3FFF)
#define MAX_TAG_ENTRIES (TAG_INDEX_MASK + 1)
#define TAG_ID_ERROR (-10000)


static int next_tag_id = MAX_TAG_ENTRIES;
static plc_tag_p tag_map[MAX_TAG_ENTRIES + 1] = {0,};


static inline int tag_id_inc(int id)
{
    if(id <= 0 || id == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR, "Incoming ID is not valid! Got %d",id);
        return TAG_ID_ERROR;
    }

    id = (id + 1) & TAG_ID_MASK;

    if(id == 0) {
        id = 1; /* skip zero intentionally! Can't return an ID of zero because it looks like a NULL pointer */
    }

    return id;
}

static inline int to_tag_index(int id)
{
    if(id <= 0 || id == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR, "Incoming ID is not valid! Got %d",id);
        return TAG_ID_ERROR;
    }
    return (id & TAG_INDEX_MASK);
}


int allocate_new_tag_to_id_mapping(plc_tag_p tag)
{
    int new_id = 0;
    int index = 0;
    int found = 0;

    critical_block(global_library_mutex) {
		new_id = next_tag_id;
		
        for(int count=1; count < MAX_TAG_ENTRIES && new_id != TAG_ID_ERROR; count++) {
            new_id = tag_id_inc(new_id);

            /* everything OK? */
            if(new_id == TAG_ID_ERROR) break;

            index = to_tag_index(new_id);

            /* is the slot empty? */
            if(index != TAG_ID_ERROR && !tag_map[index]) {
                next_tag_id = new_id;
                tag->tag_id = new_id;
                tag_map[index] = tag;
                found = 1;
                break;
            }

            if(index == TAG_ID_ERROR) break;
        }
    }

    if(found) {
        return new_id;
    }

    if(index != TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR, "Unable to find empty mapping slot!");
        return PLCTAG_ERR_NO_MEM; /* not really the right error, but close */
    }

    /* this did not work */
    return PLCTAG_ERR_NOT_ALLOWED;
}




plc_tag_p map_id_to_tag(int tag_id)
{
    plc_tag_p result = NULL;
    int index = to_tag_index(tag_id);
    int result_tag_id;

    pdebug(DEBUG_DETAIL, "Starting");

    if(index == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR,"Bad tag ID passed! %d", tag_id);
        return (plc_tag_p)0;
    }

    critical_block(global_library_mutex) {
        result = tag_map[index];
        if(result) {
            result_tag_id = result->tag_id;
        } else {
            result_tag_id = -1;
        }
    }

    if(result && result_tag_id == tag_id) {
        pdebug(DEBUG_DETAIL, "Correct mapping for id %d found with tag %p", tag_id, result);
        return result;
    }

    pdebug(DEBUG_WARN, "Not found, tag id %d maps to tag %p with id %d", tag_id, result, result_tag_id);

    /* either nothing was there or it is the wrong tag. */
    return NULL;
}


int release_tag_to_id_mapping(plc_tag_p tag)
{
    int map_index = 0;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(!tag || tag->tag_id == 0) {
        pdebug(DEBUG_ERROR, "Tag null or tag ID is zero.");
        return PLCTAG_ERR_NOT_FOUND;
    }

    map_index = to_tag_index(tag->tag_id);

    if(map_index == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR,"Bad tag ID %d!", tag->tag_id);
        return PLCTAG_ERR_BAD_DATA;
    }

    critical_block(global_library_mutex) {
        /* find the actual slot and check if it is the right tag */
        if(!tag_map[map_index] || tag_map[map_index] != tag) {
            pdebug(DEBUG_WARN, "Tag not found or entry is already clear.");
            rc = PLCTAG_ERR_NOT_FOUND;
        } else {
            pdebug(DEBUG_DETAIL,"Releasing tag %p(%d) at location %d",tag, tag->tag_id, map_index);
            tag_map[map_index] = (plc_tag_p)(intptr_t)0;
        }
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}



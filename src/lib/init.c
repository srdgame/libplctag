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

#include <stdlib.h>
#include <lib/libplctag.h>
#include <lib/libplctag_tag.h>
#include <platform.h>
#include <util/attr.h>
#include <util/debug.h>
#include <ab/ab.h>
#include <system/system.h>
#include <lib/init.h>


/*
 * The version string.
 */

const char *VERSION="2.0.0";


static lock_t library_initialization_lock = LOCK_INIT;
static volatile int library_initialized = 0;


/*
 * The following maps attributes to the tag creation functions.
 */

/* match up to 5 fields. */
#define MAX_FIELD_MATCH (5)


struct {
	const char *name;
	const char *value;
} tag_type_map_kv;

struct {
	tag_type_map_kv fields[MAX_FIELD_MATCH];
	const tag_create_function tag_constructor;
} tag_type_map[] = {
	/* Allen-Bradley tag types */
    { {{"make","allen-bradley"}, {"family","controllogix"}, {"use_connection","1"}, {"read_group","*"}, {NULL,NULL}}, ab_logix_conn_group_create},
    { {{"make","allen-bradley"}, {"family","controllogix"}, {"use_connection","1"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_conn_create},
    { {{"make","allen-bradley"}, {"family","controllogix"}, {"read_group","*"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_group_create},
    { {{"make","allen-bradley"}, {"family","controllogix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_create},
    { {{"make","allen-bradley"}, {"family","compactlogix"}, {"use_connection","1"}, {"read_group","*"}, {NULL,NULL}}, ab_logix_conn_group_create},
    { {{"make","allen-bradley"}, {"family","compactlogix"}, {"use_connection","1"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_conn_create},
    { {{"make","allen-bradley"}, {"family","compactlogix"}, {"read_group","*"}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_group_create},
    { {{"make","allen-bradley"}, {"family","compactlogix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_logix_unconn_create},
    { {{"make","allen-bradley"}, {"family","micro800"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_micro800_create},
    { {{"make","allen-bradley"}, {"family","micrologix"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    { {{"make","allen-bradley"}, {"family","micrologix"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    { {{"make","allen-bradley"}, {"family","slc"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    { {{"make","allen-bradley"}, {"family","slc"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    { {{"make","allen-bradley"}, {"family","plc5"}, {"dhp_bridge","1"}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_dhp_bridge_create},
    { {{"make","allen-bradley"}, {"family","plc5"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, ab_eip_pccc_create},
    
    /* special system tags. */
    { {{"make","system"}, {"family","library"}, {NULL,NULL}, {NULL,NULL}, {NULL,NULL}}, system_tag_create}
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
    int num_entries = (sizeof(tag_type_map)/sizeof(tag_type_map[0]));
    int entry = 0;
    int field = 0;
    int failed = 0;

	/* match the attributes with the required attributes. */
	for(entry = 0; entry < num_entries; entry++) {
		failed = 0;
		
		/* check each field for a match.  If one does not match, then immediately fail. */
		for(field = 0; !failed && field < MAX_FIELD_MATCH; field++) {
			const char *name = tag_type_map[entry].fields[field].name;
			const char *match_val = tag_type_map[entry].fields[field].name;
			const char *tag_val = attr_get_str(attributes,name,NULL);
			
			pdebug(DEBUG_DETAIL,"Checking %s for value %s given value %s",name, match_val, tag_val);
			
			/* only check valid fields */
			if(name) {
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





/*
 * destroy_modules() is called when the main process exits.
 *
 * Modify this for any PLC/protocol that needs to have something
 * torn down at the end.
 */

void destroy_modules(void)
{
    ab_teardown();
    
    job_teardown();

    lib_teardown();
}



/*
 * initialize_modules() is called the first time any kind of tag is
 * created.  It will be called before the tag creation routines are
 * run.
 */


int initialize_modules(void)
{
    int rc = PLCTAG_STATUS_OK;

    /* loop until we get the lock flag */
    while (!lock_acquire((lock_t*)&library_initialization_lock)) {
        sleep_ms(1);
    }

    if(!library_initialized) {
        pdebug(DEBUG_INFO,"Initialized library modules.");
        rc = lib_init();

        if(rc == PLCTAG_STATUS_OK) {
            rc = job_init();
        }

        if(rc == PLCTAG_STATUS_OK) {
            rc = ab_init();
        }

        library_initialized = 1;

        /* hook the destructor */
        atexit(destroy_modules);

        pdebug(DEBUG_INFO,"Done initializing library modules.");
    }

    /* we hold the lock, so clear it.*/
    lock_release((lock_t*)&library_initialization_lock);

    return rc;
}


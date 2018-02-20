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

#include <lib/libplctag.h>
#include <platform.h>
#include <util/debug.h>
#include <util/hashtable.h>
#include <util/mem.h>
#include <util/refcount.h>



static volatile hashtable_p resource_by_name = NULL;
static volatile mutex_p resource_mutex = NULL;


//static int resource_data_cleanup(void *resource_arg, void *name_arg, void *arg3);



/* FIXME - this does allocation each time a resource is retrieved. */
void *resource_get(const char *name)
{
    void * resource = NULL;
    int name_len = 0;

    if(!name) {
        pdebug(DEBUG_WARN,"Called with null name!");
        return NULL;
    }

    pdebug(DEBUG_DETAIL,"Starting with name %s", name);

    name_len = str_length(name);

    critical_block(resource_mutex) {
        resource = hashtable_get(resource_by_name, (void *)name, name_len);
        if(resource) {
            /* get a strong reference if we can. */
            resource = rc_inc(resource);

            if(!resource) {
                /* clean out the entry */
                hashtable_remove(resource_by_name, (void *)name, name_len);
            }
        }
    }

    pdebug(DEBUG_DETAIL,"Resource%s found!",(resource ? "": " not"));

    return resource;
}



int resource_put(const char *name, void *resource)
{
    int name_len = 0;
    int rc = PLCTAG_STATUS_OK;

    if(!name) {
        pdebug(DEBUG_WARN,"Called with null name!");
        return PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_DETAIL,"Starting with name %s", name);

    name_len = str_length(name);

    critical_block(resource_mutex) {
        rc = hashtable_put(resource_by_name, (void*)name, name_len, resource);
        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Error inserting resource, %s",plc_tag_decode_error(rc));
            break;
        }
    }

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}




char *resource_make_name_impl(int num_args, ...)
{
    va_list arg_list;
    int total_length = 0;
    char *result = NULL;
    char *tmp = NULL;

    /* first loop to find the length */
    va_start(arg_list, num_args);
    for(int i=0; i < num_args; i++) {
        tmp = va_arg(arg_list, char *);
        if(tmp) {
            total_length += str_length(tmp);
        }
    }
    va_end(arg_list);

    /* make a buffer big enough */
    total_length += 1;

    result = mem_alloc(total_length);
    if(!result) {
        pdebug(DEBUG_ERROR,"Unable to allocate new string buffer!");
        return NULL;
    }

    /* loop to copy the strings */
    result[0] = 0;
    va_start(arg_list, num_args);
    for(int i=0; i < num_args; i++) {
        tmp = va_arg(arg_list, char *);
        if(tmp) {
            int len = str_length(result);
            str_copy(&result[len], total_length - len, tmp);
        }
    }
    va_end(arg_list);

    return result;
}



//int resource_data_cleanup(void *resource_arg, void *name_arg, void *arg3)
//{
//    void *resource = resource_arg;
//    char *name = name_arg;
//    
//    (void)arg3;
//
//    if(!resource) {
//        pdebug(DEBUG_WARN, "Null resource argument!");
//        
//        if(name) {
//            mem_free(name);
//        }
//        
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    if(!name) {
//        pdebug(DEBUG_WARN,"Resource name pointer is null!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    critical_block(resource_mutex) {
//        hashtable_remove(resource_by_name, (void*)name, str_length(name)+1);
//
//        mem_free(name);
//    }
//}
//



int resource_service_init(void)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO,"Initializing Resource utility.");

    /* this is a mutex used to synchronize most activities in this protocol */
    rc = mutex_create((mutex_p*)&resource_mutex);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create resource mutex!");
        return rc;
    }

    /* create the hashtable in which we will have the resources stored. */
    resource_by_name = hashtable_create(200); /* MAGIC */
    if(!resource_by_name) {
        pdebug(DEBUG_ERROR,"Unable to allocate a hashtable!");
        mutex_destroy((mutex_p*)&resource_mutex);

        return PLCTAG_ERR_CREATE;
    }

    pdebug(DEBUG_INFO,"Finished initializing Resource utility.");

    return rc;
}


void resource_service_teardown(void)
{
    pdebug(DEBUG_INFO,"Tearing down Resource utility.");

    pdebug(DEBUG_INFO,"Tearing down resource hashtable.");

    hashtable_destroy(resource_by_name);

    pdebug(DEBUG_INFO,"Tearing down resource mutex.");

    mutex_destroy((mutex_p*)&resource_mutex);

    pdebug(DEBUG_INFO,"Done.");
}


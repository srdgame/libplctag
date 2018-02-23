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


#define LIBPLCTAGDLL_EXPORTS 1

#include <limits.h>
#include <float.h>
#include <inttypes.h>
#include <stdlib.h>
#include <lib/libplctag.h>
#include <lib/tag.h>
#include <platform.h>
#include <util/atomic.h>
#include <util/attr.h>
#include <util/debug.h>
#include <util/hashtable.h>
#include <util/job.h>
#include <util/refcount.h>
#include <util/resource.h>
#include <ab/create.h>
#include <system/create.h>


/* library visible data */
const char *VERSION="2.0.6";
const int VERSION_ARRAY[3] = {2,0,6};
mutex_p global_library_mutex = NULL;

/* local data. */
static lock_t library_initialization_lock = LOCK_INIT;
static volatile int library_initialized = 0;
static volatile hashtable_p tags = NULL;
static volatile int global_tag_id = 1;

#define MAX_TAG_ID (1000000000)

static int initialize_modules();
static void destroy_modules();

static int tag_add_to_lookup(plc_tag_p tag);
static plc_tag_p tag_lookup(tag_id id);




/**************************************************************************
 ***************************  API Functions  ******************************
 **************************************************************************/


/*
 * plc_tag_decode_error()
 *
 * This takes an integer error value and turns it into a printable string.
 *
 * FIXME - this should produce better errors than this!
 */



LIB_EXPORT const char* plc_tag_decode_error(int rc)
{
    switch(rc) {
        case PLCTAG_STATUS_PENDING: return "PLCTAG_STATUS_PENDING"; break;
        case PLCTAG_STATUS_OK: return "PLCTAG_STATUS_OK"; break;
        case PLCTAG_ERR_NULL_PTR: return "PLCTAG_ERR_NULL_PTR"; break;
        case PLCTAG_ERR_OUT_OF_BOUNDS: return "PLCTAG_ERR_OUT_OF_BOUNDS"; break;
        case PLCTAG_ERR_NO_MEM: return "PLCTAG_ERR_NO_MEM"; break;
        case PLCTAG_ERR_LL_ADD: return "PLCTAG_ERR_LL_ADD"; break;
        case PLCTAG_ERR_BAD_PARAM: return "PLCTAG_ERR_BAD_PARAM"; break;
        case PLCTAG_ERR_CREATE: return "PLCTAG_ERR_CREATE"; break;
        case PLCTAG_ERR_NOT_EMPTY: return "PLCTAG_ERR_NOT_EMPTY"; break;
        case PLCTAG_ERR_OPEN: return "PLCTAG_ERR_OPEN"; break;
        case PLCTAG_ERR_SET: return "PLCTAG_ERR_SET"; break;
        case PLCTAG_ERR_WRITE: return "PLCTAG_ERR_WRITE"; break;
        case PLCTAG_ERR_TIMEOUT: return "PLCTAG_ERR_TIMEOUT"; break;
        case PLCTAG_ERR_TIMEOUT_ACK: return "PLCTAG_ERR_TIMEOUT_ACK"; break;
        case PLCTAG_ERR_RETRIES: return "PLCTAG_ERR_RETRIES"; break;
        case PLCTAG_ERR_READ: return "PLCTAG_ERR_READ"; break;
        case PLCTAG_ERR_BAD_DATA: return "PLCTAG_ERR_BAD_DATA"; break;
        case PLCTAG_ERR_ENCODE: return "PLCTAG_ERR_ENCODE"; break;
        case PLCTAG_ERR_DECODE: return "PLCTAG_ERR_DECODE"; break;
        case PLCTAG_ERR_UNSUPPORTED: return "PLCTAG_ERR_UNSUPPORTED"; break;
        case PLCTAG_ERR_TOO_LONG: return "PLCTAG_ERR_TOO_LONG"; break;
        case PLCTAG_ERR_CLOSE: return "PLCTAG_ERR_CLOSE"; break;
        case PLCTAG_ERR_NOT_ALLOWED: return "PLCTAG_ERR_NOT_ALLOWED"; break;
        case PLCTAG_ERR_THREAD: return "PLCTAG_ERR_THREAD"; break;
        case PLCTAG_ERR_NO_DATA: return "PLCTAG_ERR_NO_DATA"; break;
        case PLCTAG_ERR_THREAD_JOIN: return "PLCTAG_ERR_THREAD_JOIN"; break;
        case PLCTAG_ERR_THREAD_CREATE: return "PLCTAG_ERR_THREAD_CREATE"; break;
        case PLCTAG_ERR_MUTEX_DESTROY: return "PLCTAG_ERR_MUTEX_DESTROY"; break;
        case PLCTAG_ERR_MUTEX_UNLOCK: return "PLCTAG_ERR_MUTEX_UNLOCK"; break;
        case PLCTAG_ERR_MUTEX_INIT: return "PLCTAG_ERR_MUTEX_INIT"; break;
        case PLCTAG_ERR_MUTEX_LOCK: return "PLCTAG_ERR_MUTEX_LOCK"; break;
        case PLCTAG_ERR_NOT_IMPLEMENTED: return "PLCTAG_ERR_NOT_IMPLEMENTED"; break;
        case PLCTAG_ERR_BAD_DEVICE: return "PLCTAG_ERR_BAD_DEVICE"; break;
        case PLCTAG_ERR_BAD_GATEWAY: return "PLCTAG_ERR_BAD_GATEWAY"; break;
        case PLCTAG_ERR_REMOTE_ERR: return "PLCTAG_ERR_REMOTE_ERR"; break;
        case PLCTAG_ERR_NOT_FOUND: return "PLCTAG_ERR_NOT_FOUND"; break;
        case PLCTAG_ERR_ABORT: return "PLCTAG_ERR_ABORT"; break;
        case PLCTAG_ERR_WINSOCK: return "PLCTAG_ERR_WINSOCK"; break;
        case PLCTAG_ERR_DUPLICATE: return "PLCTAG_ERR_DUPLICATE"; break;
        case PLCTAG_ERR_BUSY: return "PLCTAG_ERR_BUSY"; break;
        case PLCTAG_ERR_NO_RESOURCES: return "PLCTAG_ERR_NO_RESOURCES"; break;
        case PLCTAG_ERR_TOO_SHORT:  return "PLCTAG_ERR_TOO_SHORT"; break;

        default: return "Unknown error."; break;
    }

    return "Unknown error.";
}


/*
 * plc_tag_create()
 *
 * This is where the dispatch occurs to the protocol specific implementation.
 */

LIB_EXPORT tag_id plc_tag_create(const char *attrib_str, int timeout)
{
    plc_tag_p tag = PLC_TAG_P_NULL;
    attr attribs = NULL;
    int rc = PLCTAG_STATUS_OK;
    const char *protocol = NULL;

    pdebug(DEBUG_INFO,"Starting");

    rc = initialize_modules();
    if(rc != PLCTAG_STATUS_OK) {
        return rc;
    }

    if(!attrib_str || str_length(attrib_str) == 0) {
        return PLCTAG_ERR_BAD_PARAM;
    }

    attribs = attr_create_from_str(attrib_str);
    if(!attribs) {
        return PLCTAG_ERR_NO_MEM;
    }

    /* set debug level */
    set_debug_level(attr_get_int(attribs, "debug", DEBUG_NONE));

    /* determine which protocol to use. */
    protocol = attr_get_str(attribs, "protocol", "NONE");
    if(str_cmp_i(protocol,"ab_eip") == 0 || str_cmp_i(protocol,"ab-eip") == 0) {
        /* some AB PLC. */
        rc = ab_tag_create(attribs, &tag);
    } else if(str_cmp_i(protocol,"system") == 0) {
        rc = system_tag_create(attribs, &tag);
    } else {
        pdebug(DEBUG_WARN, "Unsupported protocol %s!", protocol);
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }
    
    if(!tag) {
        pdebug(DEBUG_WARN, "Error creating tag!");
        return rc;
    }
    
    /* this selects a tag ID and adds the tag to the lookup table. */
    rc = tag_add_to_lookup(tag);
    if(rc != PLCTAG_STATUS_OK) {
        rc_dec(tag);
        pdebug(DEBUG_WARN, "Error attempting to add the new tag to the lookup table!");
        return rc;
    }
    
    if(timeout > 0) {
        int64_t max_time = time_ms() + timeout;
        
        rc = tag->vtable->get_status(tag);
        
        while(max_time > time_ms() && rc == PLCTAG_STATUS_PENDING) {
            sleep_ms(1);
            rc = tag->vtable->get_status(tag);
        }
    }

    pdebug(DEBUG_INFO, "Done.");
    
    return tag->id;
}



LIB_EXPORT int plc_tag_lock(tag_id id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_DETAIL, "Starting.");

    if(!tag) { 
        pdebug(DEBUG_WARN,"Tag not found.");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    rc = mutex_lock(tag->external_mutex);
    
    pdebug(DEBUG_DETAIL, "Done.");
    
    return rc;
}




LIB_EXPORT int plc_tag_unlock(tag_id id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_DETAIL, "Starting.");

    if(!tag) { 
        pdebug(DEBUG_WARN,"Tag not found.");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    rc = mutex_unlock(tag->external_mutex);
    
    pdebug(DEBUG_DETAIL, "Done.");
    
    return rc;
}


/*
 * plc_tag_abort()
 *
 * This function calls through the vtable in the passed tag to call
 * the protocol-specific implementation.
 *
 * The implementation must do whatever is necessary to abort any
 * ongoing IO.
 *
 * The status of the operation is returned.
 */


LIB_EXPORT int plc_tag_abort(tag_id id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_INFO, "Starting.");

    if(tag) { 
        critical_block(tag->api_mutex) {
            rc = tag->vtable->abort(tag);
        }
        
        /* we are done. */
        rc_dec(tag);
    } else {
        pdebug(DEBUG_WARN,"Tag not found.");
        rc = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}




/*
 * plc_tag_destroy()
 *
 * Remove all implementation specific details about a tag and clear its
 * memory.
 *
 * FIXME - this can block while connections close etc.   This needs to
 * be modified to mark the tag as destroyed and let the back-end deal
 * with the delays.  As the tag is being destroyed, there is not really
 * any point in having a timeout here.   Instead, this routine should
 * never block.
 */


LIB_EXPORT int plc_tag_destroy(tag_id id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    /*
     * We do this in stages.  
     * 
     * First, we remove the tag from the hashtable.   This prevents
     * any other thread after this from finding the tag.   However, there
     * could be other threads using the tag right now.   Later we need to
     * get the API mutex.
     * 
     * We do not acquire a reference to the tag because we are using the
     * reference from the hashtable.  
     */
    critical_block(global_library_mutex) {
        tag = hashtable_remove(tags, &id, sizeof(id));
    }

    /* abort any operations in flight. We need to call the implementation directly. */
    if(tag) {
        /* Now trigger the API mutex to ensure that no one else is using the tag. */
        critical_block(tag->api_mutex) {
            rc = tag->vtable->abort(tag);
        }
    }

    /* 
     * We are safe to decrement the ref count.  This will kick off any destructors
     * once the ref count hits zero.
     */
    rc_dec(tag);

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}



/*
 * plc_tag_read()
 *
 * This function calls through the vtable in the passed tag to call
 * the protocol-specific implementation.  That starts the read operation.
 * If there is a timeout passed, then this routine waits for either
 * a timeout or an error.
 *
 * The status of the operation is returned.
 */


LIB_EXPORT int plc_tag_read(tag_id id, int timeout)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_SPEW, "Starting.");

    if(tag) { 
        critical_block(tag->api_mutex) {
            /* check for null parts */
            if(!tag->vtable || !tag->vtable->read) {
                pdebug(DEBUG_WARN, "Tag does not have a read function!");
                rc = PLCTAG_ERR_NOT_IMPLEMENTED;
                break;
            }

            /* check read cache, if not expired, return existing data. */
            if(tag->read_cache_expire_ms > time_ms()) {
                pdebug(DEBUG_INFO, "Returning cached data.");
                rc = PLCTAG_STATUS_OK;
                break;
            }

            /* the protocol implementation does not do the timeout. */
            rc = tag->vtable->read(tag);

            /* if error, return now */
            if(rc != PLCTAG_STATUS_PENDING && rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Error from read command!");
                break;
            }

            /* set up the cache time */
            if(tag->read_cache_duration_ms) {
                tag->read_cache_expire_ms = time_ms() + tag->read_cache_duration_ms;
            }

            /*
             * if there is a timeout, then loop until we get
             * an error or we timeout.
             */
            if(timeout) {
                int64_t timeout_time = timeout + time_ms();
                int64_t start_time = time_ms();

                while(rc == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
                    rc = tag->vtable->get_status(tag);

                    /*
                     * terminate early and do not wait again if the
                     * IO is done.
                     */
                    if(rc != PLCTAG_STATUS_PENDING) {
                        break;
                    }

                    sleep_ms(1); /* MAGIC */
                }

                /*
                 * if we dropped out of the while loop but the status is
                 * still pending, then we timed out.
                 *
                 * Abort the operation and set the status to show the timeout.
                 */
                if(rc == PLCTAG_STATUS_PENDING) {
                    tag->vtable->abort(tag);
                    
                    pdebug(DEBUG_WARN, "Write operation timed out!");
                    
                    rc = PLCTAG_ERR_TIMEOUT;
                }

                pdebug(DEBUG_INFO,"elapsed time %" PRId64 "ms",(time_ms()-start_time));
            }
        }
        
        /* we are done. */
        rc_dec(tag);
    } else {
        pdebug(DEBUG_WARN,"Tag not found.");
        rc = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}




/*
 * plc_tag_status
 *
 * Return the current status of the tag.  This will be PLCTAG_STATUS_PENDING if there is
 * an uncompleted IO operation.  It will be PLCTAG_STATUS_OK if everything is fine.  Other
 * errors will be returned as appropriate.
 *
 * This is a function provided by the underlying protocol implementation.
 */




LIB_EXPORT int plc_tag_status(tag_id id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_SPEW, "Starting.");

    if(tag) { 
        critical_block(tag->api_mutex) {
            rc = tag->vtable->get_status(tag);
        }
        
        /* we are done. */
        rc_dec(tag);
    } else {
        pdebug(DEBUG_WARN,"Tag not found.");
        rc = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_SPEW, "Done.");

    return rc;
}





/*
 * plc_tag_write()
 *
 * This function calls through the vtable in the passed tag to call
 * the protocol-specific implementation.  That starts the write operation.
 * If there is a timeout passed, then this routine waits for either
 * a timeout or an error.
 *
 * The status of the operation is returned.
 */


LIB_EXPORT int plc_tag_write(tag_id id, int timeout)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_SPEW, "Starting.");

    if(tag) { 
        critical_block(tag->api_mutex) {
            /* check for null parts */
            if(!tag->vtable || !tag->vtable->write) {
                pdebug(DEBUG_WARN, "Tag does not have a write function!");
                rc = PLCTAG_ERR_NOT_IMPLEMENTED;
                break;
            }

            /* the protocol implementation does not do the timeout. */
            rc = tag->vtable->write(tag);

            /* if error, return now */
            if(rc != PLCTAG_STATUS_PENDING && rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Error from write command!");
                break;
            }

            /*
             * if there is a timeout, then loop until we get
             * an error or we timeout.
             */
            if(timeout) {
                int64_t timeout_time = timeout + time_ms();
                int64_t start_time = time_ms();

                while(rc == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
                    rc = tag->vtable->get_status(tag);

                    /*
                     * terminate early and do not wait again if the
                     * IO is done.
                     */
                    if(rc != PLCTAG_STATUS_PENDING) {
                        break;
                    }

                    sleep_ms(1); /* MAGIC */
                }

                /*
                 * if we dropped out of the while loop but the status is
                 * still pending, then we timed out.
                 *
                 * Abort the operation and set the status to show the timeout.
                 */
                if(rc == PLCTAG_STATUS_PENDING) {
                    tag->vtable->abort(tag);
                    
                    pdebug(DEBUG_WARN, "Write operation timed out!");
                    
                    rc = PLCTAG_ERR_TIMEOUT;
                }

                pdebug(DEBUG_INFO,"elapsed time %" PRId64 "ms",(time_ms()-start_time));
            }
        }
        
        /* we are done. */
        rc_dec(tag);
    } else {
        pdebug(DEBUG_WARN,"Tag not found.");
        rc = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}





/*
 * Tag data accessors.
 */



LIB_EXPORT int plc_tag_get_size(tag_id id)
{
    int result = PLCTAG_STATUS_OK;
    plc_tag_p tag = tag_lookup(id);

    pdebug(DEBUG_SPEW, "Starting.");

    if(tag) { 
        critical_block(tag->api_mutex) {
            /* check for null parts */
            if(!tag->vtable || !tag->vtable->get_size) {
                pdebug(DEBUG_WARN, "Tag does not have a size function!");
                result = PLCTAG_ERR_NOT_IMPLEMENTED;
                break;
            }
            
            result = tag->vtable->get_size(tag);
        }
                
        /* we are done. */
        rc_dec(tag);
    } else {
        pdebug(DEBUG_WARN,"Tag not found.");
        result = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_INFO, "Done.");

    return result;
}









LIB_EXPORT uint64_t plc_tag_get_uint64(tag_id id, int offset)
{
    uint64_t res = UINT64_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(uint64_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (uint64_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}

LIB_EXPORT int plc_tag_set_uint64(tag_id id, int offset, uint64_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(uint64_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}







LIB_EXPORT int64_t plc_tag_get_int64(tag_id id, int offset)
{
    int64_t res = INT64_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(int64_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (int64_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_int64(tag_id id, int offset, int64_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        rc = tag->vtable->set_int(tag, offset, sizeof(int64_t), val);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}








LIB_EXPORT uint32_t plc_tag_get_uint32(tag_id id, int offset)
{
    uint32_t res = UINT32_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(uint32_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (uint32_t)(uint64_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}


LIB_EXPORT int plc_tag_set_uint32(tag_id id, int offset, uint32_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(uint32_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}








LIB_EXPORT int32_t plc_tag_get_int32(tag_id id, int offset)
{
    int32_t res = INT32_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(int32_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (int32_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_int32(tag_id id, int offset, int32_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(int32_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}








LIB_EXPORT uint16_t plc_tag_get_uint16(tag_id id, int offset)
{
    uint16_t res = UINT16_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(uint16_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (uint16_t)(uint64_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_uint16(tag_id id, int offset, uint16_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(uint16_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}







LIB_EXPORT int16_t plc_tag_get_int16(tag_id id, int offset)
{
    int16_t res = INT16_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(int16_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (int16_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_int16(tag_id id, int offset, int16_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(int16_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}










LIB_EXPORT uint8_t plc_tag_get_uint8(tag_id id, int offset)
{
    uint8_t res = UINT8_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(uint8_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (uint8_t)(uint64_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_uint8(tag_id id, int offset, uint8_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(uint8_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}







LIB_EXPORT int8_t plc_tag_get_int8(tag_id id, int offset)
{
    int8_t res = INT8_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Integer getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = 0;
        
        rc = tag->vtable->get_int(tag, offset, sizeof(int8_t), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (int8_t)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_int8(tag_id id, int offset, int8_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        int64_t tmp = (int64_t)val;
        
        rc = tag->vtable->set_int(tag, offset, sizeof(int8_t), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}








LIB_EXPORT double plc_tag_get_float64(tag_id id, int offset)
{
    double res = DBL_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Floating point getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        double tmp = 0;
        
        rc = tag->vtable->get_float(tag, offset, sizeof(double), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (double)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_float64(tag_id id, int offset, double val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        rc = tag->vtable->set_float(tag, offset, sizeof(double), val);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}








LIB_EXPORT float plc_tag_get_float32(tag_id id, int offset)
{
    float res = FLT_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }

    if(!tag->vtable->get_int) {
        pdebug(DEBUG_WARN, "Floating point getting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        double tmp = 0;
        
        rc = tag->vtable->get_float(tag, offset, sizeof(float), &tmp);
        
        if(rc == PLCTAG_STATUS_OK) {
            res = (float)tmp;
        }
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return res;
}



LIB_EXPORT int plc_tag_set_float32(tag_id id, int offset, float val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    tag = tag_lookup(id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag %d not found.", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    if(!tag->vtable->set_int) {
        pdebug(DEBUG_WARN, "Integer setting function not implemented!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    critical_block(tag->api_mutex) {
        double tmp = (double)val;
        
        rc = tag->vtable->set_float(tag, offset, sizeof(float), tmp);
    }
    
    rc_dec(tag);

    pdebug(DEBUG_DETAIL,"Done.");

    return rc;
}







/*******************************************************************************
 *************************** External Helper Functions *************************
 *******************************************************************************/
 
int plc_tag_init(plc_tag_p tag, attr attribs, tag_vtable_p vtable)
{
    int rc = PLCTAG_STATUS_OK;
    int64_t read_cache_ms = 0;
    
    tag->vtable = vtable;
    
    /* set up the mutexes */
    rc = mutex_create(&tag->api_mutex);
    if(rc != PLCTAG_STATUS_OK) {
        rc_dec(tag);
        pdebug(DEBUG_ERROR,"Unable to create API mutex!");
        return rc;
    }

    rc = mutex_create(&tag->external_mutex);
    if(rc != PLCTAG_STATUS_OK) {
        rc_dec(tag);
        pdebug(DEBUG_ERROR,"Unable to create external mutex!");
        return rc;
    }

    /* set up the read cache config. */
    read_cache_ms = attr_get_int(attribs,"read_cache_ms",0);

    if(read_cache_ms < 0) {
        pdebug(DEBUG_WARN, "read_cache_ms value must be positive, using zero.");
        read_cache_ms = 0;
    }

    tag->read_cache_expire_ms = (uint64_t)0;
    tag->read_cache_duration_ms = (uint64_t)read_cache_ms;
    
    return rc;
}


int plc_tag_deinit(plc_tag_p tag)
{
    int rc = PLCTAG_STATUS_OK;
    
    if(tag->external_mutex) {
        rc = mutex_destroy(&tag->external_mutex);
    }
    
    if(tag->api_mutex) {
        rc = mutex_destroy(&tag->api_mutex);
    }
    
    return rc;
}











/*******************************************************************************
 ***********************  Implementation Functions *****************************
 *******************************************************************************/





int tag_add_to_lookup(plc_tag_p tag)
{
    tag_id id;

    critical_block(global_library_mutex) {
        int found = 0;

        id = global_tag_id;

        do {
            plc_tag_p tmp;

            /* get a new ID */
            id++;

            if(id <= 0 || id > MAX_TAG_ID) {
                id = 1;
            }

            tmp = hashtable_get(tags, &id, sizeof(id));
            if(!tmp) {
                found = 1;

                global_tag_id = id;
                tag->id = id;

                /* FIXME - check return code! */
                hashtable_put(tags, &id, sizeof(id), tag);
            }
        } while(!found);
    }

    return PLCTAG_STATUS_OK;
}







plc_tag_p tag_lookup(tag_id id)
{
    plc_tag_p result = NULL;

    critical_block(global_library_mutex) {
        result = hashtable_get(tags, &id, sizeof(id));
        result = rc_inc(result);
    }

    return result;
}












int initialize_modules(void)
{
    int rc = PLCTAG_STATUS_OK;

    /* loop until we get the lock flag */
    while (!lock_acquire((lock_t*)&library_initialization_lock)) {
        sleep_ms(1);
    }

    if(!library_initialized) {
        pdebug(DEBUG_INFO,"Initializing modules.");

        pdebug(DEBUG_INFO,"Initializing library global mutex.");

        /* first see if the mutex is there. */
        if (!global_library_mutex) {
            rc = mutex_create((mutex_p*)&global_library_mutex);

            if (rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_ERROR, "Unable to create global tag mutex!");
            }
        }

        pdebug(DEBUG_INFO,"Creating internal tag hashtable.");
        if(rc == PLCTAG_STATUS_OK) {
            tags = hashtable_create(100); /* FIXME - MAGIC */
            if(!tags) {
                pdebug(DEBUG_ERROR,"Unable to create internal tag hashtable!");
                rc = PLCTAG_ERR_CREATE;
            }
        }

        pdebug(DEBUG_INFO,"Setting up job service.");
        if(rc == PLCTAG_STATUS_OK) {
            rc = job_init();

            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_ERROR,"Job service failed to initialize correctly!");
            }
        }

        pdebug(DEBUG_INFO,"Setting up resource service.");
        if(rc == PLCTAG_STATUS_OK) {
            rc = resource_service_init();

            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_ERROR,"Resource service failed to initialize correctly!");
            }
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




/*
 * teardown_modules() is called when the main process exits.
 *
 * Modify this for any PLC/protocol that needs to have something
 * torn down at the end.
 */

void destroy_modules(void)
{

    pdebug(DEBUG_INFO,"Tearing down library.");

    resource_service_teardown();

    job_teardown();
    

    pdebug(DEBUG_INFO,"Releasing internal tag hashtable.");
    hashtable_destroy(tags);

    pdebug(DEBUG_INFO,"Destroying global library mutex.");
    if(global_library_mutex) {
        mutex_destroy((mutex_p*)&global_library_mutex);
    }

    pdebug(DEBUG_INFO,"Done.");
}


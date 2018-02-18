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
#include <lib/libplctag.h>
#include <lib/tag.h>
#include <platform.h>
#include <util/attr.h>
#include <util/debug.h>
#include <util/hashtable.h>
#include <ab/create.h>


/* library visible data */
const char *VERSION="2.0.5";
const int VERSION_ARRAY[3] = {2,0,5};
mutex_p global_library_mutex = NULL;

/* local data. */
static lock_t library_initialization_lock = LOCK_INIT;
static volatile int library_initialized = 0;
static volatile hashtable_p tags = NULL;
static volatile int global_tag_id = 1;

#define MAX_TAG_ID (1000000000)

static int initialize_modules();
static void destroy_modules();
static int lib_init();
static void lib_teardown();


static plc_tag_p tag_id_to_tag_ptr(plc_tag tag_id);
static int allocate_new_tag_to_id_mapping(plc_tag_p tag);
static int release_tag_to_id_mapping(plc_tag_p tag);
static int api_lock(int index);
static int api_unlock(int index);
static int tag_id_to_tag_index(plc_tag tag_id_ptr);


#define TAG_ID_MASK (0xFFFFFFF)
#define TAG_INDEX_MASK (0x3FFF)
#define MAX_TAG_ENTRIES (TAG_INDEX_MASK + 1)
#define TAG_ID_ERROR INT_MIN

/* these are only internal to the file */

static volatile int next_tag_id = MAX_TAG_ENTRIES;
static volatile plc_tag_p tag_map[MAX_TAG_ENTRIES + 1] = {0,};
static volatile mutex_p tag_api_mutex[MAX_TAG_ENTRIES + 1] = {0,};



#define api_block(tag_id)                                              \
for(int __sync_flag_api_block_foo_##__LINE__ = 1; __sync_flag_api_block_foo_##__LINE__ ; __sync_flag_api_block_foo_##__LINE__ = 0, api_unlock(tag_id_to_tag_index(tag_id))) for(int __sync_rc_api_block_foo_##__LINE__ = api_lock(tag_id_to_tag_index(tag_id)); __sync_rc_api_block_foo_##__LINE__ == PLCTAG_STATUS_OK && __sync_flag_api_block_foo_##__LINE__ ; __sync_flag_api_block_foo_##__LINE__ = 0)


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
        case PLCTAG_ERR_BUSY: return "PLCTAG_ERR_BUSY"; break;

        default: return "Unknown error."; break;
    }

    return "Unknown error.";
}


/*
 * plc_tag_create()
 *
 * This is where the dispatch occurs to the protocol specific implementation.
 */

LIB_EXPORT plc_tag plc_tag_create(const char *attrib_str, int timeout)
{
    plc_tag_p tag = PLC_TAG_P_NULL;
    plc_tag tag_id = PLCTAG_ERR_OUT_OF_BOUNDS;
    attr attribs = NULL;
    int rc = PLCTAG_STATUS_OK;
    int read_cache_ms = 0;
    const char *protocol = NULL;

    pdebug(DEBUG_INFO,"Starting");

    if(initialize_modules() != PLCTAG_STATUS_OK) {
        return PLC_TAG_NULL;
    }

    if(!attrib_str || str_length(attrib_str) == 0) {
        return PLC_TAG_NULL;
    }

    attribs = attr_create_from_str(attrib_str);

    if(!attribs) {
        return PLC_TAG_NULL;
    }

    /* set debug level */
    set_debug_level(attr_get_int(attribs, "debug", DEBUG_NONE));

    /* determine which protocol to use. */
    protocol = attr_get_str(attribs, "protocol", "NONE");
    if(str_cmp_i(protocol,"ab_eip") == 0 || str_cmp_i(protocol,"ab-eip") == 0) {
        /* some AB PLC. */
        rc = ab_create_tag(attribs, &tag);
    } else if(str_cmp_i(protocol,"system") == 0) {
        rc = system_create_tag(attribs, &tag);
    } else {
        pdebug(DEBUG_WARN, "Unsupported protocol %s!", protocol);
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }
    
    if(!tag) {
        pdebug(DEBUG_WARN, "Error creating tag!");
        return rc;
    }
    
    /* this selects a tag ID and adds the tag to the lookup table. */
    rc = add_tag_to_lookup(tag);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Error attempting to add the new tag to the lookup table!");
        return rc;
    }
    
    if(timeout > 0) {
        int64_t max_time = time_ms() + timeout;
        
        while(max_time < time_ms() && tag->vtable->status(tag) == PLCTAG_STATUS_PENDING) {
            sleep_ms(1);
        }
    }

    pdebug(DEBUG_INFO, "Done.");
    
    return tag->tag_id;
}



/*
 * plc_tag_lock
 *
 * Lock the tag against use by other threads.  Because operations on a tag are
 * very much asynchronous, actions like getting and extracting the data from
 * a tag take more than one API call.  If more than one thread is using the same tag,
 * then the internal state of the tag will get broken and you will probably experience
 * a crash.
 *
 * This should be used to initially lock a tag when starting operations with it
 * followed by a call to plc_tag_unlock when you have everything you need from the tag.
 */

LIB_EXPORT int plc_tag_lock(plc_tag tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
        } else {
            rc = mutex_lock(tag->mut);
        }
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}




/*
 * plc_tag_unlock
 *
 * The opposite action of plc_tag_unlock.  This allows other threads to access the
 * tag.
 */

LIB_EXPORT int plc_tag_unlock(plc_tag tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
        } else {
            rc = mutex_unlock(tag->mut);
        }
    }

    pdebug(DEBUG_INFO, "Done.");

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

int plc_tag_abort_mapped(plc_tag_p tag)
{
    int rc = PLCTAG_STATUS_OK;
    pdebug(DEBUG_INFO, "Starting.");

    /* who knows what state the tag data is in.  */
    tag->read_cache_expire = (uint64_t)0;

    if(!tag->vtable || !tag->vtable->abort) {
        pdebug(DEBUG_WARN,"Tag does not have a abort function!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    /* this may be synchronous. */
    rc = tag->vtable->abort(tag);

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}


LIB_EXPORT int plc_tag_abort(plc_tag tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
        } else {
            rc = plc_tag_abort_mapped(tag);
        }
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

int plc_tag_destroy_mapped(plc_tag_p tag)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag || !tag->vtable) {
        pdebug(DEBUG_WARN,"Tag vtable is missing!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /*
     * first, unmap the tag.
     *
     * This might be called from something other than plc_tag_destroy, so
     * do not make assumptions that this was already done.  However, it is
     * required that if this is called directly, then it must always be
     * the case that the tag has not been handed to the library client!
     *
     * If that happens, then it is possible that two threads could try to
     * delete the same tag at the same time.
     */

    pdebug(DEBUG_DETAIL, "Releasing tag mapping.");

    release_tag_to_id_mapping(tag);

    /* destroy the tag's mutex */
    mutex_destroy(&tag->mut);

    /* abort anything in flight */
    rc = plc_tag_abort_mapped(tag);

    /* call the destructor */
    if(!tag->vtable || !tag->vtable->destroy) {
        pdebug(DEBUG_ERROR, "tag destructor not defined!");
        rc = PLCTAG_ERR_NOT_IMPLEMENTED;
    } else {
        /*
         * It is the responsibility of the destroy
         * function to free all memory associated with
         * the tag.
         */
        rc = tag->vtable->destroy(tag);
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}

LIB_EXPORT int plc_tag_destroy(plc_tag tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
        } else {
            /* the tag was still mapped, so destroy it. */
            rc = plc_tag_destroy_mapped(tag);
        }
    }

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

LIB_EXPORT int plc_tag_read(plc_tag tag_id, int timeout)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
            break;
        }

        /* check for null parts */
        if(!tag->vtable || !tag->vtable->read) {
            pdebug(DEBUG_WARN, "Tag does not have a read function!");
            rc = PLCTAG_ERR_NOT_IMPLEMENTED;
            break;
        }

        /* check read cache, if not expired, return existing data. */
        if(tag->read_cache_expire > time_ms()) {
            pdebug(DEBUG_INFO, "Returning cached data.");
            rc = PLCTAG_STATUS_OK;
            break;
        }

        /* the protocol implementation does not do the timeout. */
        rc = tag->vtable->read(tag);

        /* if error, return now */
        if(rc != PLCTAG_STATUS_PENDING && rc != PLCTAG_STATUS_OK) {
            break;
        }

        /* set up the cache time */
        if(tag->read_cache_ms) {
            tag->read_cache_expire = time_ms() + tag->read_cache_ms;
        }

        /*
         * if there is a timeout, then loop until we get
         * an error or we timeout.
         */
        if(timeout) {
            int64_t timeout_time = timeout + time_ms();
            int64_t start_time = time_ms();

            while(rc == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
                rc = plc_tag_status_mapped(tag);

                /*
                 * terminate early and do not wait again if the
                 * IO is done.
                 */
                if(rc != PLCTAG_STATUS_PENDING) {
                    break;
                }

                sleep_ms(5); /* MAGIC */
            }

            /*
             * if we dropped out of the while loop but the status is
             * still pending, then we timed out.
             *
             * Abort the operation and set the status to show the timeout.
             */
            if(rc == PLCTAG_STATUS_PENDING) {
                plc_tag_abort_mapped(tag);
                rc = PLCTAG_ERR_TIMEOUT;
            }

            pdebug(DEBUG_INFO,"elapsed time %ldms",(time_ms()-start_time));
        }
    } /* end of api block */

    pdebug(DEBUG_INFO, "Done");

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


int plc_tag_status_mapped(plc_tag_p tag)
{
    /* pdebug(DEBUG_DETAIL, "Starting."); */
    if(!tag) {
        pdebug(DEBUG_ERROR,"Null tag passed!");
        return PLCTAG_ERR_NULL_PTR;
    }

    if(!tag->vtable || !tag->vtable->status) {
        pdebug(DEBUG_ERROR, "tag status accessor not defined!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    return tag->vtable->status(tag);
}



LIB_EXPORT int plc_tag_status(plc_tag tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_SPEW, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
            break;
        }

        rc = plc_tag_status_mapped(tag);
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

LIB_EXPORT int plc_tag_write(plc_tag tag_id, int timeout)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
            break;
        }

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
            pdebug(DEBUG_WARN,"Response from write command is not OK!");
            break;
        }

        /*
         * if there is a timeout, then loop until we get
         * an error or we timeout.
         */
        if(timeout) {
            int64_t timeout_time = timeout + time_ms();

            while(rc == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
                rc = plc_tag_status_mapped(tag);

                /*
                 * terminate early and do not wait again if the
                 * IO is done.
                 */
                if(rc != PLCTAG_STATUS_PENDING) {
                    break;
                }

                sleep_ms(5); /* MAGIC */
            }

            /*
             * if we dropped out of the while loop but the status is
             * still pending, then we timed out.
             *
             * Abort the operation and set the status to show the timeout.
             */
            if(rc == PLCTAG_STATUS_PENDING) {
                pdebug(DEBUG_WARN, "Write operation timed out.");
                plc_tag_abort_mapped(tag);
                rc = PLCTAG_ERR_TIMEOUT;
            }
        }
    } /* end of api block */

    pdebug(DEBUG_INFO, "Done");

    return rc;
}





/*
 * Tag data accessors.
 */



LIB_EXPORT int plc_tag_get_size(plc_tag tag_id)
{
    int result = 0;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            result = PLCTAG_ERR_NOT_FOUND;
            break;
        }

        result = tag->size;
    }

    return result;
}




LIB_EXPORT uint32_t plc_tag_get_uint32(plc_tag tag_id, int offset)
{
    uint32_t res = UINT32_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint32_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = ((uint32_t)(tag->data[offset])) +
              ((uint32_t)(tag->data[offset+1]) << 8) +
              ((uint32_t)(tag->data[offset+2]) << 16) +
              ((uint32_t)(tag->data[offset+3]) << 24);
    }

    return res;
}



LIB_EXPORT int plc_tag_set_uint32(plc_tag tag_id, int offset, uint32_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint32_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        /* write the data. */
        tag->data[offset]   = (uint8_t)(val & 0xFF);
        tag->data[offset+1] = (uint8_t)((val >> 8) & 0xFF);
        tag->data[offset+2] = (uint8_t)((val >> 16) & 0xFF);
        tag->data[offset+3] = (uint8_t)((val >> 24) & 0xFF);
    }

    return rc;
}




LIB_EXPORT int32_t  plc_tag_get_int32(plc_tag tag_id, int offset)
{
    int32_t res = INT32_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int32_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = (int32_t)(((uint32_t)(tag->data[offset])) +
                        ((uint32_t)(tag->data[offset+1]) << 8) +
                        ((uint32_t)(tag->data[offset+2]) << 16) +
                        ((uint32_t)(tag->data[offset+3]) << 24));
    }

    return res;
}



LIB_EXPORT int plc_tag_set_int32(plc_tag tag_id, int offset, int32_t ival)
{
    uint32_t val = (uint32_t)(ival);
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int32_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset]   = (uint8_t)(val & 0xFF);
        tag->data[offset+1] = (uint8_t)((val >> 8) & 0xFF);
        tag->data[offset+2] = (uint8_t)((val >> 16) & 0xFF);
        tag->data[offset+3] = (uint8_t)((val >> 24) & 0xFF);
    }

    return rc;
}









LIB_EXPORT uint16_t plc_tag_get_uint16(plc_tag tag_id, int offset)
{
    uint16_t res = UINT16_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint16_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = ((uint16_t)(tag->data[offset])) +
              ((uint16_t)(tag->data[offset+1]) << 8);
    }

    return res;
}




LIB_EXPORT int plc_tag_set_uint16(plc_tag tag_id, int offset, uint16_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint16_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset]   = (uint8_t)(val & 0xFF);
        tag->data[offset+1] = (uint8_t)((val >> 8) & 0xFF);
    }

    return rc;
}









LIB_EXPORT int16_t  plc_tag_get_int16(plc_tag tag_id, int offset)
{
    int16_t res = INT16_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int16_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = (int16_t)(((uint16_t)(tag->data[offset])) +
                        ((uint16_t)(tag->data[offset+1]) << 8));
    }

    return res;
}




LIB_EXPORT int plc_tag_set_int16(plc_tag tag_id, int offset, int16_t ival)
{
    uint16_t val = (uint16_t)(ival);
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int16_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset]   = (uint8_t)(val & 0xFF);
        tag->data[offset+1] = (uint8_t)((val >> 8) & 0xFF);
    }

    return rc;
}








LIB_EXPORT uint8_t plc_tag_get_uint8(plc_tag tag_id, int offset)
{
    uint8_t res = UINT8_MAX;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint8_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = tag->data[offset];
    }

    return res;
}




LIB_EXPORT int plc_tag_set_uint8(plc_tag tag_id, int offset, uint8_t val)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(uint8_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset] = val;
    }

    return rc;
}





LIB_EXPORT int8_t plc_tag_get_int8(plc_tag tag_id, int offset)
{
    int8_t res = INT8_MIN;
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
       if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int8_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        res = (int8_t)(tag->data[offset]);
    }

    return res;
}




LIB_EXPORT int plc_tag_set_int8(plc_tag tag_id, int offset, int8_t ival)
{
    uint8_t val = (uint8_t)(ival);
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = NULL;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(int8_t)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset] = (uint8_t)val;
    }

    return rc;
}










LIB_EXPORT float plc_tag_get_float32(plc_tag tag_id, int offset)
{
    uint32_t ures;
    float res = FLT_MAX;
    plc_tag_p tag = NULL;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.");

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(ures)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            break;
        }

        ures = ((uint32_t)(tag->data[offset])) +
               ((uint32_t)(tag->data[offset+1]) << 8) +
               ((uint32_t)(tag->data[offset+2]) << 16) +
               ((uint32_t)(tag->data[offset+3]) << 24);
    }

    /* copy the data */
    mem_copy(&res,&ures,sizeof(res));

    return res;
}




LIB_EXPORT int plc_tag_set_float32(plc_tag tag_id, int offset, float fval)
{
    int rc = PLCTAG_STATUS_OK;
    uint32_t val = 0;
    plc_tag_p tag = NULL;

    /* copy the data */
    mem_copy(&val, &fval, sizeof(val));

    api_block(tag_id) {
        tag = tag_id_to_tag_ptr(tag_id);
        if(!tag) {
            pdebug(DEBUG_WARN,"Tag not found.");
            rc = PLCTAG_ERR_NOT_FOUND;
            break;
        }

        /* is the tag ready for this operation? */
        rc = plc_tag_status_mapped(tag);
        if(rc != PLCTAG_STATUS_OK && rc != PLCTAG_ERR_OUT_OF_BOUNDS) {
            pdebug(DEBUG_WARN,"Tag not in good state!");
            break;
        }

        /* is there data? */
        if(!tag->data) {
            pdebug(DEBUG_WARN,"Tag has no data!");
            rc = PLCTAG_ERR_NO_DATA;
            break;
        }

        /* is there enough data */
        if((offset < 0) || (offset + ((int)sizeof(val)) > tag->size)) {
            pdebug(DEBUG_WARN,"Data offset out of bounds.");
            rc = PLCTAG_ERR_OUT_OF_BOUNDS;
            break;
        }

        tag->data[offset]   = (uint8_t)(val & 0xFF);
        tag->data[offset+1] = (uint8_t)((val >> 8) & 0xFF);
        tag->data[offset+2] = (uint8_t)((val >> 16) & 0xFF);
        tag->data[offset+3] = (uint8_t)((val >> 24) & 0xFF);
    }

    return rc;
}





/*****************************************************************************************************
 ****************************  Helper functions for generic internal use *****************************
 ****************************************************************************************************/




/*
 * plc_tag_init()
 *
 * Set up the generic parts of the tag.
 */

int plc_tag_init(plc_tag_p tag, attr attribs, tag_vtable_p vtable)
{
    plc_tag tag_id = PLCTAG_ERR_OUT_OF_BOUNDS;
    int rc = PLCTAG_STATUS_OK;
    int read_cache_ms = 0;

    pdebug(DEBUG_INFO,"Starting");
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Called with null tag pointer!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    tag->vtable = vtable;

    /* set up the read cache config. */
    read_cache_ms = attr_get_int(attribs,"read_cache_ms",0);
    if(read_cache_ms < 0) {
        pdebug(DEBUG_WARN, "read_cache_ms value must be positive, using zero.");
        read_cache_ms = 0;
    }

    tag->read_cache_expire = (uint64_t)0;
    tag->read_cache_ms = (uint64_t)read_cache_ms;

    /* create tag mutex */
    rc = mutex_create(&tag->api_mutex);

    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create tag mutex!");
        
        return rc;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}







/*
 * plc_tag_deinit()
 *
 * Tear down the generic parts of the tag.
 * 
 * This should be called from the protocol's tag destructor.
 */

int plc_tag_deinit(plc_tag_p tag)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting");

    /* destroy tag mutex */
    rc = mutex_destroy(&tag->api_mutex);

    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to destroy tag mutex!");
        
        return rc;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}




/*****************************************************************************************************
 *********************************  Library set up/tear down routines ********************************
 ****************************************************************************************************/


int initialize_modules(void)
{
    int rc = PLCTAG_STATUS_OK;

    /* loop until we get the lock flag */
    while (!lock_acquire((lock_t*)&library_initialization_lock)) {
        sleep_ms(1);
    }

    if(!library_initialized) {
        pdebug(DEBUG_INFO,"Initializing library modules.");
        rc = lib_init();

        if(rc == PLCTAG_STATUS_OK) {
            rc = job_init();
        } else {
            pdebug(DEBUG_ERROR,"Job utility failed to initialize correctly!");
            return rc;
        }

        if(rc == PLCTAG_STATUS_OK) {
            rc = ab_init();
        } else {
            pdebug(DEBUG_ERROR,"AB protocol failed to initialize correctly!");
            return rc;
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




void destroy_modules(void)
{
    ab_teardown();

    pt_teardown();

    lib_teardown();
}




/*
 * Initialize the library.  This is called in a threadsafe manner and
 * only called once.
 */

int lib_init(void)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO,"Setting up global library data.");

    pdebug(DEBUG_INFO,"Initializing library global mutex.");

    /* first see if the mutex is there. */
    if (!global_library_mutex) {
        rc = mutex_create((mutex_p*)&global_library_mutex);

        if (rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_ERROR, "Unable to create global tag mutex!");
        }
    }

    /* initialize the mutex for API protection */
    for(int i=0; i < (MAX_TAG_ENTRIES + 1); i++) {
        rc = mutex_create((mutex_p*)&tag_api_mutex[i]);
    }

    pdebug(DEBUG_INFO,"Done.");

    return rc;
}

void lib_teardown(void)
{
    pdebug(DEBUG_INFO,"Tearing down library.");

    /* destroy the mutex for API protection */
    for(int i=0; i < (MAX_TAG_ENTRIES + 1); i++) {
        mutex_destroy((mutex_p*)&tag_api_mutex[i]);
        if(tag_map[i]) {
            pdebug(DEBUG_WARN,"Tag %p at index %d was not destroyed!",tag_map[i],i);
        }
    }

    pdebug(DEBUG_INFO,"Destroying global library mutex.");
    if(global_library_mutex) {
        mutex_destroy((mutex_p*)&global_library_mutex);
    }


    pdebug(DEBUG_INFO,"Done.");
}






/*****************************************************************************************************
 *****************************  Support routines for extra indirection *******************************
 ****************************************************************************************************/


static inline int tag_id_inc(int id)
{
    if(id <= 0 || id == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR, "Incoming ID is not valid! Got %d",id);
        return TAG_ID_ERROR;
    }

    id = (id + 1) & TAG_ID_MASK;

    if(id <= 0) {
        id = 1; /* skip zero intentionally! Can't return an ID of zero because it looks like a bad ID */
    }

    return id;
}

static inline int tag_id_to_tag_index(int id)
{
    if(id <= 0 || id == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR, "Incoming ID is not valid! Got %d",id);
        return TAG_ID_ERROR;
    }
    return (id & TAG_INDEX_MASK);
}



/*
 * Must be called with a translated index!
 */

static int api_lock(int index)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL,"Starting");

    if(index < 0 || index > MAX_TAG_ENTRIES) {
        pdebug(DEBUG_WARN,"Illegal tag index %d",index);
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    rc = mutex_lock(tag_api_mutex[index]);

    pdebug(DEBUG_DETAIL,"Done with status %d", rc);

    return rc;
}



/*
 * Must be called with a translated index!
 */
static int api_unlock(int index)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL,"Starting");

    if(index < 0 || index > MAX_TAG_ENTRIES) {
        pdebug(DEBUG_WARN,"Illegal tag index %d",index);
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    rc = mutex_unlock(tag_api_mutex[index]);

    pdebug(DEBUG_DETAIL,"Done with status %d", rc);

    return rc;
}




static int allocate_new_tag_to_id_mapping(plc_tag_p tag)
{
    int new_id = next_tag_id;
    int index = 0;
    int found = 0;

    for(int count=0; !found && count < MAX_TAG_ENTRIES && new_id != TAG_ID_ERROR; count++) {
        new_id = tag_id_inc(new_id);

        /* everything OK? */
        if(new_id == TAG_ID_ERROR) break;

        index = tag_id_to_tag_index(new_id);

        /* is the slot empty? */
        if(index != TAG_ID_ERROR && !tag_map[index]) {
            /* Must lock the api mutex so that we can change the mapping. */
            api_lock(index);

            /* recheck if the slot is empty. It could have changed while we locked the mutex. */
            if(!tag_map[index]) {
                next_tag_id = new_id;
                tag->tag_id = new_id;

                tag_map[index] = tag;

                found = 1;
            }

            api_unlock(index);
        }

        if(index == TAG_ID_ERROR) break;
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



/*
 * This MUST be called while the API mutex for this tag is held!
 */

static plc_tag_p tag_id_to_tag_ptr(plc_tag tag_id)
{
    plc_tag_p result = NULL;
    int index = tag_id_to_tag_index(tag_id);

    pdebug(DEBUG_DETAIL, "Starting");

    if(index == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR,"Bad tag ID passed! %d", tag_id);
        return (plc_tag_p)0;
    }

    result = tag_map[index];
    if(result && result->tag_id == tag_id) {
        pdebug(DEBUG_DETAIL, "Correct mapping at index %d for id %d found with tag %p", index, tag_id, result);
    } else {
        pdebug(DEBUG_WARN, "Not found, tag id %d maps to a different tag", tag_id);
        result = NULL;
    }

    pdebug(DEBUG_DETAIL,"Done with tag %p", result);

    return result;
}




/*
 * It is REQUIRED that the tag API mutex be held when this is called!
 */

static int release_tag_to_id_mapping(plc_tag_p tag)
{
    int map_index = 0;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(!tag || tag->tag_id == 0) {
        pdebug(DEBUG_ERROR, "Tag null or tag ID is zero.");
        return PLCTAG_ERR_NOT_FOUND;
    }

    map_index = tag_id_to_tag_index(tag->tag_id);

    if(map_index == TAG_ID_ERROR) {
        pdebug(DEBUG_ERROR,"Bad tag ID %d!", tag->tag_id);
        return PLCTAG_ERR_BAD_DATA;
    }

    /* find the actual slot and check if it is the right tag */
    if(!tag_map[map_index] || tag_map[map_index] != tag) {
        pdebug(DEBUG_WARN, "Tag not found or entry is already clear.");
        rc = PLCTAG_ERR_NOT_FOUND;
    } else {
        pdebug(DEBUG_DETAIL,"Releasing tag %p(%d) at location %d",tag, tag->tag_id, map_index);
        tag_map[map_index] = (plc_tag_p)(intptr_t)0;
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}






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

#include <limits.h>
#include <float.h>
#include <lib/libplctag.h>
#include <platform.h>
#include <lib/init.h>
#include <lib/lib.h>
#include <lib/tag.h>
#include <lib/teardown.h>
#include <util/attr.h>
#include <util/debug.h>
#include <util/encdec.h>




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

        default: return "Unknown error."; break;
    }

    return "Unknown error.";
}



/*
 * plc_tag_create()
 *
 * This is where the dispatch occurs to the protocol specific implementation.
 */

LIB_EXPORT int plc_tag_create(const char *attrib_str, int timeout)
{
    plc_tag_p tag = NULL;
    int tag_id = PLCTAG_ERR_OUT_OF_BOUNDS;
    attr attribs = NULL;
    int rc = PLCTAG_STATUS_OK;
    int read_cache_ms = 0;
    tag_create_function tag_constructor;

    if((rc = initialize_modules()) != PLCTAG_STATUS_OK) {
        return rc;
    }

    if(!attrib_str || str_length(attrib_str) == 0) {
		pdebug(DEBUG_WARN,"Tag attribute string is null or zero length.");
        return PLCTAG_ERR_BAD_PARAM;
    }

    attribs = attr_create_from_str(attrib_str);

    if(!attribs) {
		pdebug(DEBUG_WARN,"Unable to create attribute structure from tag attribute string.");
        return PLCTAG_ERR_CREATE;
    }

    /* set debug level */
    set_debug_level(attr_get_int(attribs, "debug", DEBUG_NONE));


	/* get the constructor for this kind of tag. */
    tag_constructor = find_tag_create_func(attribs);
    if(!tag_constructor) {
        pdebug(DEBUG_WARN,"Tag creation failed, no tag constructor found for tag type!");
        attr_destroy(attribs);
        return PLCTAG_ERR_CREATE;
    }

    /*
     * create the tag, this is protocol specific.
     *
     * If this routine wants to keep the attributes around, it needs
     * to clone them.
     */
    tag = tag_constructor(attribs);

    /*
     * FIXME - this really should be here???  Maybe not?  But, this is
     * the only place it can be without making every protocol type do this automatically.
     */
    if(!tag) {
        pdebug(DEBUG_WARN, "Tag creation failed, skipping mutex creation and other generic setup.");
        attr_destroy(attribs);
        return PLCTAG_ERR_CREATE;
    }

    /* set up the read cache config. */
    read_cache_ms = attr_get_int(attribs,"read_cache_ms",0);

    if(read_cache_ms < 0) {
        pdebug(DEBUG_WARN, "read_cache_ms value must be positive, using zero.");
        read_cache_ms = 0;
    }

    tag->read_cache_expire = (uint64_t)0;
    tag->read_cache_ms = (uint64_t)read_cache_ms;

    /* create tag mutex */
    rc = mutex_create(&tag->mut);

    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create tag mutex!");

        /* this is fatal! */
        attr_destroy(attribs);
        plc_tag_destroy_mapped(tag);
		return PLCTAG_ERR_CREATE;
    }

	/* done with attributes now */
    attr_destroy(attribs);

    /* map the tag to a tag ID */
    tag_id = allocate_new_tag_to_id_mapping(tag);

    /* if the mapping failed, then punt */
    if(tag_id == PLCTAG_ERR_OUT_OF_BOUNDS) {
        pdebug(DEBUG_ERROR, "Unable to map tag %p to lookup table entry, rc=%d", tag_id);

        /* need to destroy the tag because we allocated memory etc. */
        plc_tag_destroy_mapped(tag);

        return tag_id;
    }

    /*
     * if there is a timeout, then loop until we get
     * an error or we timeout.
     */
    if(timeout>0) {
        int64_t timeout_time = timeout + time_ms();
        rc = tag->status;

        while(rc == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
            rc = tag->status;

            /*
             * terminate early and do not wait again if the
             * async operations are done.
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
         * The create failed, so now we need to punt.
         */
        if(rc == PLCTAG_STATUS_PENDING) {
            pdebug(DEBUG_WARN, "Create operation timed out.");
            rc = PLCTAG_ERR_TIMEOUT;
        }
    } else {
        /* not waiting and no errors yet, so carry on. */
        rc = PLCTAG_STATUS_OK;
    }

    /* if everything did not go OK, then close the tag. */
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Tag creation failed.");
        plc_tag_destroy_mapped(tag);
        return rc;
    }

    pdebug(DEBUG_INFO, "Returning mapped tag %d", tag_id);

    return tag_id;
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

LIB_EXPORT int plc_tag_lock(int tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = map_id_to_tag(tag_id);

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag || !tag->mut) {
        pdebug(DEBUG_WARN,"Tag is missing or mutex is already cleaned up!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* lock the mutex */
    rc = mutex_lock(tag->mut);

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}




/*
 * plc_tag_unlock
 *
 * The opposite action of plc_tag_unlock.  This allows other threads to access the
 * tag.
 */

LIB_EXPORT int plc_tag_unlock(int tag_id)
{
    int rc = PLCTAG_STATUS_OK;
    plc_tag_p tag = map_id_to_tag(tag_id);

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag || !tag->mut) {
        pdebug(DEBUG_WARN,"Tag is missing or mutex is already cleaned up!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* unlock the mutex */
    rc = mutex_unlock(tag->mut);

    pdebug(DEBUG_INFO,"Done.");

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

LIB_EXPORT int plc_tag_abort(int tag_id)
{
    plc_tag_p tag = map_id_to_tag(tag_id);
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag is not mapped or null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    rc = plc_tag_abort_mapped(tag);

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
    //mutex_p tmp_mutex;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag || !tag->vtable) {
        pdebug(DEBUG_WARN,"Tag vtable is missing!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /*
     * FIXME - should the mutex be destroyed first or the tag
     * removed from the mapping table?  It seems like the mapping
     * should be removed first.
     */

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

    /* destroy the mutex, not needed now */
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

LIB_EXPORT int plc_tag_destroy(int tag_id)
{
    plc_tag_p tag = map_id_to_tag(tag_id);
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    /* is the tag still valid? */
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag is null or not mapped!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /*
     * We get the tag mapping, then we remove it.  If it is
     * already removed (due to simultaneous calls by threads
     * to the library), then we skip to the end.
     */

    rc = release_tag_to_id_mapping(tag);
    if(rc != PLCTAG_STATUS_OK) {
        return rc;
    }

    /* the tag was still mapped, so destroy it. */
    rc = plc_tag_destroy_mapped(tag);

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

LIB_EXPORT int plc_tag_read(int tag_id, int timeout)
{
    pdebug(DEBUG_INFO, "Starting.");

    pdebug(DEBUG_DETAIL,"Reading tag id ptr %p.", tag_id);
    pdebug(DEBUG_DETAIL,"Reading tag id %d", (int)(intptr_t)tag_id);

    plc_tag_p tag = map_id_to_tag(tag_id);
    int rc = PLCTAG_STATUS_OK;

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag is NULL!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* check for null parts */
    if(!tag->vtable || !tag->vtable->read) {
        pdebug(DEBUG_WARN, "Tag does not have a read function!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    /* check read cache, if not expired, return existing data. */
    if(tag->read_cache_expire > time_ms()) {
        pdebug(DEBUG_INFO, "Returning cached data.");
        return PLCTAG_STATUS_OK;
    }

    /* the protocol implementation does not do the timeout. */
    rc = tag->vtable->read(tag);

    /* if error, return now */
    if(rc != PLCTAG_STATUS_PENDING && rc != PLCTAG_STATUS_OK) {
        return rc;
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
            rc = tag->status;

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

    pdebug(DEBUG_INFO, "Done");

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

LIB_EXPORT int plc_tag_write(int tag_id, int timeout)
{
    plc_tag_p tag = map_id_to_tag(tag_id);
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag is NULL!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* we are writing so the tag existing data is stale. */
    tag->read_cache_expire = (uint64_t)0;

    /* check the vtable */
    if(!tag->vtable || !tag->vtable->write) {
        pdebug(DEBUG_WARN, "Tag does not have a write function!");
        return PLCTAG_ERR_NOT_IMPLEMENTED;
    }

    /* the protocol implementation does not do the timeout. */
    rc = tag->vtable->write(tag);

    /* if error, return now */
    if(rc != PLCTAG_STATUS_PENDING && rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Response from write command is not OK!");
        return rc;
    }

    /*
     * if there is a timeout, then loop until we get
     * an error or we timeout.
     */
    if(timeout) {
        int64_t timeout_time = timeout + time_ms();

        while(tag->status == PLCTAG_STATUS_PENDING && timeout_time > time_ms()) {
            /*
             * terminate early and do not wait again if the
             * IO is done.
             */
            if(tag->status != PLCTAG_STATUS_PENDING) {
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
        if(tag->status == PLCTAG_STATUS_PENDING) {
            pdebug(DEBUG_WARN, "Write operation timed out.");
            plc_tag_abort_mapped(tag);
            rc = PLCTAG_ERR_TIMEOUT;
        }
    }

    pdebug(DEBUG_INFO, "Done");

    return rc;
}





/*
 * Tag data accessors.
 */



LIB_EXPORT int plc_tag_get_size(int tag_id)
{
    plc_tag_p tag = map_id_to_tag(tag_id);

    if(!tag) {
        pdebug(DEBUG_WARN,"Tag pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    return tag->size;
}





LIB_EXPORT int plc_tag_status(int tag_id)
{
    plc_tag_p tag = map_id_to_tag(tag_id);
    int rc = PLCTAG_STATUS_OK;

    /* commented out due to too much output. */
    /*pdebug(DEBUG_INFO,"Starting.");*/

    if(!tag) {
        pdebug(DEBUG_WARN, "Tag is NULL!");
        return PLCTAG_ERR_NULL_PTR;
    }

    rc = tag->status;

    /* pdebug(DEBUG_DETAIL, "Done."); */

    return rc;
}








LIB_EXPORT int8_t plc_tag_get_int8(int tag_id, int offset)
{
    plc_tag_p t = map_id_to_tag(tag_id);
    int8_t res = INT8_MIN;

    /* is there a tag? */
    if(!t) {
        pdebug(DEBUG_WARN,"Tag pointer is null!");
        return res;
    }

    /* is there data? */
    if(!t->data) {
        pdebug(DEBUG_WARN,"Tag data pointer is null!");
        return res;
    }

    /* is there enough data */
    if((offset < 0) || (offset > t->size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return res;
    }

    res = (int8_t)(t->data[offset]);

    return res;
}




LIB_EXPORT int plc_tag_set_int8(int tag_id, int offset, int8_t val)
{
    plc_tag_p t = map_id_to_tag(tag_id);

    /* is there a tag? */
    if(!t) {
        pdebug(DEBUG_WARN,"Tag pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* is there data? */
    if(!t->data) {
        pdebug(DEBUG_WARN,"Tag data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* is there enough data space to write the value? */
    if((offset < 0) || (offset >= t->size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    t->data[offset] = (uint8_t)val;

    return PLCTAG_STATUS_OK;
}



LIB_EXPORT uint8_t plc_tag_get_uint8(int tag_id, int offset)
{
    plc_tag_p t = map_id_to_tag(tag_id);
    uint8_t res = UINT8_MAX;

    /* is there a tag? */
    if(!t) {
        pdebug(DEBUG_WARN,"Tag pointer is null!");
        return res;
    }

    /* is there data? */
    if(!t->data) {
        pdebug(DEBUG_WARN,"Tag data pointer is null!");
        return res;
    }

    /* is there enough data */
    if((offset < 0) || (offset > t->size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return res;
    }

    res = t->data[offset];

    return res;
}




LIB_EXPORT int plc_tag_set_uint8(int tag_id, int offset, uint8_t val)
{
    plc_tag_p t = map_id_to_tag(tag_id);

    /* is there a tag? */
    if(!t) {
        pdebug(DEBUG_WARN,"Tag pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* is there data? */
    if(!t->data) {
        pdebug(DEBUG_WARN,"Tag data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* is there enough data space to write the value? */
    if((offset < 0) || (offset > t->size)) {
        pdebug(DEBUG_WARN,"Offset it out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    t->data[offset] = val;

    return PLCTAG_STATUS_OK;
}






#define INT_GETTER_SETTER(I_TYPE, I_ERR)                               \
LIB_EXPORT I_TYPE ## _t  plc_tag_get_ ## I_TYPE(int tag_id, int offset)\
{                                                                      \
    plc_tag_p t = map_id_to_tag(tag_id);                               \
    I_TYPE ##_t res = I_ERR;                                           \
    int rc;                                                            \
                                                                       \
    /* is there a tag? */                                              \
    if(!t) {                                                           \
        pdebug(DEBUG_WARN,"Tag pointer is null!");                     \
        return res;                                                    \
    }                                                                  \
                                                                       \
    /* is there data? */                                               \
    if(!t->data) {                                                     \
        pdebug(DEBUG_WARN,"Tag data pointer is null!");                \
        return res;                                                    \
    }                                                                  \
                                                                       \
    /* is there enough data */                                         \
    if((offset < 0) || (offset + ((int)sizeof(t->I_TYPE ## _byte_order)) > t->size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return res;                                                    \
    }                                                                  \
                                                                       \
	rc = I_TYPE ## _decode(&t->data[0], t->size, offset, t->I_TYPE ## _byte_order, &res);\
	if(rc < 0) {                                                       \
		return I_ERR;                                                  \
	}                                                                  \
                                                                       \
    return res;                                                        \
}                                                                      \
                                                                       \
                                                                       \
LIB_EXPORT int plc_tag_set_ ## I_TYPE(int tag_id, int offset, I_TYPE ## _t val)\
{                                                                      \
    plc_tag_p t = map_id_to_tag(tag_id);                               \
	int rc;                                                            \
                                                                       \
    /* is there a tag? */                                              \
    if(!t) {                                                           \
        pdebug(DEBUG_WARN,"Tag pointer is null!");                     \
        return PLCTAG_ERR_NULL_PTR;                                    \
    }                                                                  \
                                                                       \
    /* is there data? */                                               \
    if(!t->data) {                                                     \
        pdebug(DEBUG_WARN,"Tag data pointer is null!");                \
        return PLCTAG_ERR_NULL_PTR;                                    \
    }                                                                  \
                                                                       \
    /* is there enough data space to write the value? */               \
    if((offset < 0) || (offset + ((int)sizeof(t->I_TYPE ## _byte_order)) > t->size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return PLCTAG_ERR_OUT_OF_BOUNDS;                               \
    }                                                                  \
                                                                       \
	rc = I_TYPE ## _encode(&t->data[0], t->size, offset, t->I_TYPE ## _byte_order, val);\
	if(rc < 0) {                                                       \
		return rc;                                                     \
	}                                                                  \
                                                                       \
    return PLCTAG_STATUS_OK;                                           \
}                                                                      


INT_GETTER_SETTER(int16, INT16_MAX)

INT_GETTER_SETTER(uint16, UINT16_MAX)



INT_GETTER_SETTER(int32, INT32_MAX)

INT_GETTER_SETTER(uint32, UINT32_MAX)



INT_GETTER_SETTER(int64, INT64_MAX)

INT_GETTER_SETTER(uint64, UINT64_MAX)



#define FLOAT_GETTER_SETTER(F_TYPE,F_ERR)                              \
LIB_EXPORT F_TYPE plc_tag_get_ ## F_TYPE(int tag_id, int offset)       \
{                                                                      \
    plc_tag_p t = map_id_to_tag(tag_id);                               \
    F_TYPE res = F_ERR;                                                \
    int rc;                                                            \
                                                                       \
    /* is there a tag? */                                              \
    if(!t) {                                                           \
        pdebug(DEBUG_WARN,"Tag pointer is null!");                     \
        return res;                                                    \
    }                                                                  \
                                                                       \
    /* is there data? */                                               \
    if(!t->data) {                                                     \
        pdebug(DEBUG_WARN,"Tag data pointer is null!");                \
        return res;                                                    \
    }                                                                  \
                                                                       \
    /* is there enough data */                                         \
    if((offset < 0) || (offset + ((int)sizeof(t->F_TYPE ## _byte_order)) > t->size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return res;                                                    \
    }                                                                  \
                                                                       \
	rc = F_TYPE ## _decode(&t->data[0], t->size, offset, t->F_TYPE ##_byte_order, &res);\
	if(rc < 0) {                                                       \
		return F_ERR;                                                  \
	}                                                                  \
                                                                       \
    return res;                                                        \
}                                                                      \
                                                                       \
LIB_EXPORT int plc_tag_set_ ## F_TYPE(int tag_id, int offset, F_TYPE fval)\
{                                                                      \
    plc_tag_p t = map_id_to_tag(tag_id);                               \
    int rc;                                                            \
                                                                       \
    /* is there a tag? */                                              \
    if(!t) {                                                           \
        pdebug(DEBUG_WARN,"Tag pointer is null!");                     \
        return PLCTAG_ERR_NULL_PTR;                                    \
    }                                                                  \
                                                                       \
    /* is there data? */                                               \
    if(!t->data) {                                                     \
        pdebug(DEBUG_WARN,"Tag data pointer is null!");                \
        return PLCTAG_ERR_NULL_PTR;                                    \
    }                                                                  \
                                                                       \
    /* is there enough data space to write the value? */               \
    if((offset < 0) || (offset + ((int)sizeof(t->F_TYPE ## _byte_order)) > t->size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return PLCTAG_ERR_OUT_OF_BOUNDS;                               \
    }                                                                  \
                                                                       \
	rc = F_TYPE ## _encode(&t->data[0], t->size, offset, t->F_TYPE ## _byte_order, fval);\
	if(rc < 0) {                                                       \
		return rc;                                                     \
	}                                                                  \
	                                                                   \
    return PLCTAG_STATUS_OK;                                           \
}                                                                      \


FLOAT_GETTER_SETTER(float, FLT_MAX)

FLOAT_GETTER_SETTER(double, DBL_MAX)




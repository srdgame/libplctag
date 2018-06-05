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
 * 2012-06-24  KRH - Updated plc_err() calls for new API.                 *
 *                                                                        *
 * 2013-12-24  KRH - Various munging to make this compile under VS2012    *
 *                                                                        *
 **************************************************************************/


#define LIBPLCTAGDLL_EXPORTS 1

//#include <limits.h>
//#include <float.h>
//#include <lib/libplctag.h>
//#include <lib/libplctag_tag.h>
//#include <lib/init.h>
//#include <platform.h>
//#include <util/attr.h>
//#include <util/debug.h>
//#include <ab/ab.h>

#include <string.h>
#include <lib/libplctag.h>
#include <platform.h>
#include <lib/Tag.h>
#include <system/SystemTag.h>
#include <util/attr.h>
#include <util/debug.h>


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
        case PLCTAG_ERR_NO_RESOURCES: return "PLCTAG_ERR_NO_RESOURCES"; break;
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
 
LIB_EXPORT tag_id plc_tag_create(const char *attrib_str, int timeout)
{
    tag_id id = PLCTAG_ERR_CREATE;
    attr attribs = nullptr;
    
    pdebug(DEBUG_INFO,"Starting");

    if(!attrib_str || strlen(attrib_str) == 0) {
         pdebug(DEBUG_WARN,"The tag string must not be zero length or null!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    attribs = attr_create_from_str(attrib_str);

    if(!attribs) {
        pdebug(DEBUG_WARN,"Error creating attributes!");
        return PLCTAG_ERR_CREATE;
    }

    /* set debug level */
    set_debug_level(attr_get_int(attribs, "debug", DEBUG_NONE));

    do {
        id = SystemTag::Create(attribs);
        if(id > 0 || id != PLCTAG_ERR_UNSUPPORTED) {
            break;
        }
        
//        id = ABTag::Create(attribs);
//        if(id > 0 || id != PLCTAG_ERR_UNSUPPORTED) {
//            break;
//        }
    } while(0);
    
    attr_destroy(attribs);
    
    // was there an error?
    if(id < 0) {
        pdebug(DEBUG_WARN,"Unable to create tag, error %s (%d)!", plc_tag_decode_error(id), id);
        return id;
    }
    
    pdebug(DEBUG_INFO,"Done.");
    
    return id;
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

LIB_EXPORT int plc_tag_lock(tag_id id)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}



/*
 * plc_tag_unlock
 *
 * The opposite action of plc_tag_unlock.  This allows other threads to access the
 * tag.
 */

LIB_EXPORT int plc_tag_unlock(tag_id id)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}





/*
 * plc_tag_abort
 *
 * Abort any outstanding IO to the PLC.  If there is something in flight, then
 * it is marked invalid.  Note that this does not abort anything that might
 * be still processing in the report PLC.
 *
 * The status will be PLCTAG_STATUS_OK unless there is an error such as
 * a null pointer.
 *
 * This is a function provided by the underlying protocol implementation.
 */
LIB_EXPORT int plc_tag_abort(tag_id id)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}




/*
 * plc_tag_destroy
 *
 * This frees all resources associated with the tag.  Internally, it may result in closed
 * connections etc.   This calls through to a protocol-specific function.
 *
 * This is a function provided by the underlying protocol implementation.
 */
LIB_EXPORT int plc_tag_destroy(tag_id id)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}






/*
 * plc_tag_read
 *
 * Start a read.  If the timeout value is zero, then wait until the read
 * returns or the timeout occurs, whichever is first.  Return the status.
 * If the timeout value is zero, then plc_tag_read will normally return
 * PLCTAG_STATUS_PENDING.
 *
 * This is a function provided by the underlying protocol implementation.
 */
LIB_EXPORT int plc_tag_read(tag_id id, int timeout)
{
    ref<Tag> tag = Tag::getTag(id);
    int64_t timeoutEnd = time_ms() + timeout;
    int rc;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    // trigger the read.
    rc = tag->read();
    
    while(rc == PLCTAG_STATUS_PENDING && timeoutEnd > time_ms()) {
        sleep_ms(1);
        rc = tag->status();
    }
    
    if(rc == PLCTAG_STATUS_PENDING) {
        rc = PLCTAG_ERR_TIMEOUT;
    }
    
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
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}







/*
 * plc_tag_write
 *
 * Start a write.  If the timeout value is zero, then wait until the write
 * returns or the timeout occurs, whichever is first.  Return the status.
 * If the timeout value is zero, then plc_tag_write will usually return
 * PLCTAG_STATUS_PENDING.  The write is considered done
 * when it has been written to the socket.
 *
 * This is a function provided by the underlying protocol implementation.
 */
LIB_EXPORT int plc_tag_write(tag_id id, int timeout)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}





/*
 * Tag data accessors.
 */

LIB_EXPORT int plc_tag_get_size(tag_id id)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT uint32_t plc_tag_get_uint32(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_uint32(tag_id id, int offset, uint32_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT int32_t plc_tag_get_int32(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_int32(tag_id id, int offset, int32_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT uint16_t plc_tag_get_uint16(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_uint16(tag_id id, int offset, uint16_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT int16_t plc_tag_get_int16(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_int16(tag_id id, int offset, int16_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT uint8_t plc_tag_get_uint8(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_uint8(tag_id id, int offset, uint8_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


LIB_EXPORT int8_t plc_tag_get_int8(tag_id id, int offset)
{
    return 0;
}

LIB_EXPORT int plc_tag_set_int8(tag_id, int offset, int8_t val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}



LIB_EXPORT float plc_tag_get_float32(tag_id id, int offset)
{
    return 0.0;
}


LIB_EXPORT int plc_tag_set_float32(tag_id id, int offset, float val)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


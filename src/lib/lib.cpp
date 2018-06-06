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
#include <cfloat>
#include <inttypes.h>
#include <lib/libplctag.h>
#include <platform.h>
#include <lib/Tag.h>
#include <ab/ABTag.h>
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
    int64_t timeoutEnd = time_ms() + timeout;
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting");
    
    if(!attrib_str || strlen(attrib_str) == 0) {
         pdebug(DEBUG_WARN,"The tag string must not be zero length or null!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(timeout < 0) {
        pdebug(DEBUG_WARN,"Timeout must be greater than or equal to zero and is in milliseconds.");
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
        
        id = ABTag::Create(attribs);
        if(id > 0 || id != PLCTAG_ERR_UNSUPPORTED) {
            break;
        }
    } while(0);
    
    attr_destroy(attribs);
    
    // was there an error?
    if(id < 0) {
        pdebug(DEBUG_WARN,"Unable to create tag, error %s (%d)!", plc_tag_decode_error(id), id);
        return id;
    }
    
    rc = plc_tag_status(id);
    
    while(rc == PLCTAG_STATUS_PENDING && timeoutEnd > time_ms()) {
        sleep_ms(1); // just release the CPU
        rc = plc_tag_status(id);
    }
    
    if(rc != PLCTAG_STATUS_OK) {
        // clean up tag.
        plc_tag_destroy(id);

        if(rc == PLCTAG_STATUS_PENDING) {
            pdebug(DEBUG_WARN,"Tag creation timed out!");
            
            rc = PLCTAG_ERR_TIMEOUT;
        }
        
        // pass along the status
        id = rc;
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
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    // trigger the abort.
    tag->abort();
    
    return PLCTAG_STATUS_OK;
}




/*
 * plc_tag_destroy
 *
 * This frees all resources associated with the tag.  Internally, it may result in closed
 * connections etc.   This calls through to a protocol-specific function.
 *
 * This is a function provided by the underlying protocol implementation via the 
 * reference counting framework.
 */
LIB_EXPORT int plc_tag_destroy(tag_id id)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID not found or missing!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    // remove it from the Tag hash table.
    return Tag::removeTag(id);
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
    std::shared_ptr<Tag> tag = Tag::getTag(id);
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
        rc = tag->getStatus();
    }
    
    if(rc == PLCTAG_STATUS_PENDING) {
        rc = PLCTAG_ERR_TIMEOUT;
        tag->abort();
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
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID not found or missing!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    // lock the API mutex.
    std::lock_guard<std::mutex> guard(tag->apiMutex);

    return tag->getStatus();
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
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    int64_t timeoutEnd = time_ms() + timeout;
    int rc;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    // trigger the write.
    rc = tag->write();
    
    while(rc == PLCTAG_STATUS_PENDING && timeoutEnd > time_ms()) {
        sleep_ms(1);
        rc = tag->getStatus();
    }
    
    if(rc == PLCTAG_STATUS_PENDING) {
        rc = PLCTAG_ERR_TIMEOUT;
        tag->abort();
    }
    
    return rc;
}





/*
 * Tag data accessors.
 */

LIB_EXPORT int plc_tag_get_size(tag_id id)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID not found or missing!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    // lock the API mutex.
    std::lock_guard<std::mutex> guard(tag->apiMutex);

    return tag->getSize();
}



LIB_EXPORT uint64_t plc_tag_get_uint64(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t result = UINT64_MAX;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 8, &result);
    
    if(rc != PLCTAG_STATUS_OK) {
        result = UINT64_MAX;
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_uint64(tag_id id, int offset, uint64_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 8, val);
}


LIB_EXPORT int64_t plc_tag_get_int64(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    int64_t result = INT64_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 8, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<int64_t>(tmp);
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_int64(tag_id id, int offset, int64_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 8, static_cast<uint64_t>(val));
}



LIB_EXPORT uint32_t plc_tag_get_uint32(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    uint32_t result = UINT32_MAX;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 4, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<uint32_t>(tmp);
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_uint32(tag_id id, int offset, uint32_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 4, static_cast<uint64_t>(val));
}


LIB_EXPORT int32_t plc_tag_get_int32(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    int32_t result = INT32_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 4, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<int32_t>(static_cast<uint32_t>(tmp));
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_int32(tag_id id, int offset, int32_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 4, static_cast<uint64_t>(static_cast<uint32_t>(val)));
}


LIB_EXPORT uint16_t plc_tag_get_uint16(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    uint16_t result = UINT16_MAX;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 2, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<uint16_t>(tmp);
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_uint16(tag_id id, int offset, uint16_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 2, static_cast<uint64_t>(val));
}


LIB_EXPORT int16_t plc_tag_get_int16(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    int16_t result = INT16_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 2, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<int16_t>(static_cast<uint16_t>(tmp));
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_int16(tag_id id, int offset, int16_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 2, static_cast<uint64_t>(static_cast<uint16_t>(val)));
}


LIB_EXPORT uint8_t plc_tag_get_uint8(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    uint8_t result = UINT8_MAX;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 1, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<uint8_t>(tmp);
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_uint8(tag_id id, int offset, uint8_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 1, static_cast<uint64_t>(val));
}


LIB_EXPORT int8_t plc_tag_get_int8(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    uint64_t tmp = 0;
    int8_t result = INT8_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getInt(offset, 1, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {
        result = static_cast<int8_t>(static_cast<uint8_t>(tmp));
    }
    
    return result;
}

LIB_EXPORT int plc_tag_set_int8(tag_id id, int offset, int8_t val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    return tag->setInt(offset, 1, static_cast<uint64_t>(static_cast<uint8_t>(val)));
}



LIB_EXPORT double plc_tag_get_float64(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    double result = DBL_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getFloat(offset, 8, &result);
    
    if(rc != PLCTAG_STATUS_OK) {    
        result = DBL_MIN;
    }
    
    return result;
}


LIB_EXPORT int plc_tag_set_float64(tag_id id, int offset, double val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);

    return tag->setFloat(offset, 8, val);
}


LIB_EXPORT float plc_tag_get_float32(tag_id id, int offset)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    double tmp = 0;
    float result = FLT_MIN;
    int rc = PLCTAG_STATUS_OK;
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);
    
    rc = tag->getFloat(offset, 4, &tmp);
    
    if(rc == PLCTAG_STATUS_OK) {    
        result = static_cast<float>(tmp);
    }
    
    return result;
}


LIB_EXPORT int plc_tag_set_float32(tag_id id, int offset, float val)
{
    std::shared_ptr<Tag> tag = Tag::getTag(id);
    
    if(!tag) {
        pdebug(DEBUG_WARN,"Tag ID incorrect or tag missing!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    // lock the API lock.
    std::lock_guard<std::mutex> guard(tag->apiMutex);

    return tag->setFloat(offset, 4, static_cast<double>(val));
}


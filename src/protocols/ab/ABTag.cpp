/***************************************************************************
 *   Copyright (C) 2018 by Kyle Hayes                                      *
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


#include <mutex>
#include <thread>
#include <platform.h>
#include <ab/ABTag.h>
#include <ab/LogixTag.h>
#include <lib/libplctag.h>
#include <util/attr.h>
#include <util/debug.h>



int ABTag::Create(attr attribs)
{
    const char *plcStr = attr_get_str(attribs, "plc", attr_get_str(attribs, "cpu", NULL));
    tag_id id = PLCTAG_ERR_UNSUPPORTED;

    pdebug(DEBUG_INFO, "Starting.");
    
    if(!plcStr || str_length(plcStr) == 0) {
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    if(!plcStr || str_length(plcStr) == 0) {
        pdebug(DEBUG_WARN,"Tags must have a PLC type!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    if(str_cmp_i(plcStr, "lgx") == 0 
       || str_cmp_i(plcStr, "logix") == 0 
       || str_cmp_i(plcStr, "controllogix") == 0 
       || str_cmp_i(plcStr, "compactlogix") == 0) 
    {
        LogixTag *tag = new LogixTag(attribs);
        
        if(!tag) {
            pdebug(DEBUG_WARN,"Unable to create a Logix tag!");
            return PLCTAG_ERR_CREATE;
        }
        
        id = Tag::addTag(tag);
        
        if(id < 0) {
            pdebug(DEBUG_WARN,"Error adding tag!");
            delete tag;
        }
    }
    
    pdebug(DEBUG_INFO, "Done.");
    
    return id;
}



ABTag::~ABTag()
{
    pdebug(DEBUG_INFO,"Starting.");
    if(this->data) {
        delete[] this->data;
        this->data = nullptr;
    }
    pdebug(DEBUG_INFO,"Done.");
}


int ABTag::read()
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    if(this->status != PLCTAG_STATUS_OK) {
        if(this->status == PLCTAG_STATUS_PENDING) {
            return PLCTAG_ERR_BUSY;
        } 
    } else {
        this->readRequested = true;
        this->status = PLCTAG_STATUS_PENDING;
    }
    
    return this->status;
}

int ABTag::write()
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    if(this->status != PLCTAG_STATUS_OK) {
        if(this->status == PLCTAG_STATUS_PENDING) {
            return PLCTAG_ERR_BUSY;
        } 
    } else {
        this->writeRequested = true;
        this->status = PLCTAG_STATUS_PENDING;
    }
    
    return this->status;
}



int ABTag::getSize()
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    return this->dataSize;
}


int ABTag::getInt(int offset, int bytes, uint64_t *result)
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    return ABTag::getIntImpl(this->data, this->dataSize, offset, bytes, result);;
}


int ABTag::setInt(int offset, int bytes, uint64_t value)
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    return ABTag::setIntImpl(this->data, this->dataSize, offset, bytes, value);;
}


int ABTag::getFloat(int offset, int bytes, double *result)
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    return ABTag::getFloatImpl(this->data, this->dataSize, offset, bytes, result);;
}


int ABTag::setFloat(int offset, int bytes, double value)
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);
    
    return ABTag::setFloatImpl(this->data, this->dataSize, offset, bytes, value);;
}



// Static functions.

int ABTag::getIntImpl(uint8_t *data, int dataSize, int offset, int bytes, uint64_t *result)
{
    int rc = PLCTAG_STATUS_OK;
    
    if(!data) {
        pdebug(DEBUG_WARN,"Data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset < 0 || dataSize < (offset + bytes)) {
        pdebug(DEBUG_WARN, "Offset out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    // zero out the result before we start.
    *result = 0;

    // use fall through.  This is intentional.
    switch(bytes) {
        case 8:
            *result |= (static_cast<uint64_t>(data[offset+7])) << 56;
            *result |= (static_cast<uint64_t>(data[offset+6])) << 48;
            *result |= (static_cast<uint64_t>(data[offset+5])) << 40;
            *result |= (static_cast<uint64_t>(data[offset+4])) << 32;
        case 4:
            *result |= (static_cast<uint64_t>(data[offset+3])) << 24;
            *result |= (static_cast<uint64_t>(data[offset+2])) << 16;
        case 2:
            *result |= (static_cast<uint64_t>(data[offset+1])) << 8;
        case 1:
            *result |= (static_cast<uint64_t>(data[offset+0])) << 0;
            break;
        default:
            pdebug(DEBUG_WARN, "Unsupported integer size %d!", bytes);
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }

    return rc;
}



int ABTag::setIntImpl(uint8_t *data, int dataSize, int offset, int bytes, uint64_t value)
{
    int rc = PLCTAG_STATUS_OK;
    
    if(!data) {
        pdebug(DEBUG_WARN,"Data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset < 0 || dataSize < (offset + bytes)) {
        pdebug(DEBUG_WARN, "Offset out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    // use fall through.  This is intentional.
    switch(bytes) {
        case 8:
            data[offset+7] = static_cast<uint8_t>((value >> 56) & static_cast<uint64_t>(0xFF));
            data[offset+6] = static_cast<uint8_t>((value >> 48) & static_cast<uint64_t>(0xFF));
            data[offset+5] = static_cast<uint8_t>((value >> 40) & static_cast<uint64_t>(0xFF));
            data[offset+4] = static_cast<uint8_t>((value >> 32) & static_cast<uint64_t>(0xFF));
        case 4:
            data[offset+3] = static_cast<uint8_t>((value >> 24) & static_cast<uint64_t>(0xFF));
            data[offset+2] = static_cast<uint8_t>((value >> 16) & static_cast<uint64_t>(0xFF));
        case 2:
            data[offset+1] = static_cast<uint8_t>((value >> 8) & static_cast<uint64_t>(0xFF));
        case 1:
            data[offset+0] = static_cast<uint8_t>((value >> 0) & static_cast<uint64_t>(0xFF));
        default:
            pdebug(DEBUG_WARN, "Unsupported integer size %d!", bytes);
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }

    return rc;
}


int ABTag::getFloatImpl(uint8_t *data, int dataSize, int offset, int bytes, double *result)
{
    int rc = PLCTAG_STATUS_OK;
    
    if(!data) {
        pdebug(DEBUG_WARN,"Data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset < 0 || dataSize < (offset + bytes)) {
        pdebug(DEBUG_WARN, "Offset out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    if(bytes == 4) {
        uint32_t uintVal;
        float floatVal;
        
        uintVal = 0;        
        uintVal |= (static_cast<uint32_t>(data[offset+3])) << 24;
        uintVal |= (static_cast<uint32_t>(data[offset+2])) << 16;
        uintVal |= (static_cast<uint32_t>(data[offset+1])) << 8;
        uintVal |= (static_cast<uint32_t>(data[offset+0])) << 0;
        
        mem_copy(&floatVal, &uintVal, (sizeof(uintVal) < sizeof(floatVal) ? sizeof(uintVal) : sizeof(floatVal)));
        
        *result = static_cast<double>(floatVal);
    } else if(bytes == 8) {
        uint64_t uintVal;
        double floatVal;
        
        uintVal = 0;        
        uintVal |= (static_cast<uint64_t>(data[offset+7])) << 56;
        uintVal |= (static_cast<uint64_t>(data[offset+6])) << 48;
        uintVal |= (static_cast<uint64_t>(data[offset+5])) << 40;
        uintVal |= (static_cast<uint64_t>(data[offset+4])) << 32;
        uintVal |= (static_cast<uint64_t>(data[offset+3])) << 24;
        uintVal |= (static_cast<uint64_t>(data[offset+2])) << 16;
        uintVal |= (static_cast<uint64_t>(data[offset+1])) << 8;
        uintVal |= (static_cast<uint64_t>(data[offset+0])) << 0;
        
        mem_copy(&floatVal, &uintVal, (sizeof(uintVal) < sizeof(floatVal) ? sizeof(uintVal) : sizeof(floatVal)));
        
        *result = static_cast<double>(floatVal);
    } else {
        pdebug(DEBUG_WARN, "Unsupported floating point size %d!", bytes);
        rc = PLCTAG_ERR_UNSUPPORTED;
    }
    
    return rc;
}


int ABTag::setFloatImpl(uint8_t *data, int dataSize, int offset, int bytes, double value)
{
    int rc = PLCTAG_STATUS_OK;
    
    if(!data) {
        pdebug(DEBUG_WARN,"Data pointer is null!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(offset < 0 || dataSize < (offset + bytes)) {
        pdebug(DEBUG_WARN, "Offset out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    if(bytes == 4) {
        uint32_t uintVal;
        float floatVal;

        floatVal = static_cast<float>(value);

        mem_copy(&uintVal, &floatVal, (sizeof(uintVal) < sizeof(floatVal) ? sizeof(uintVal) : sizeof(floatVal)));
        
        data[offset+3] = static_cast<uint8_t>((uintVal >> 24) & static_cast<uint32_t>(0xFF));
        data[offset+2] = static_cast<uint8_t>((uintVal >> 16) & static_cast<uint32_t>(0xFF));
        data[offset+1] = static_cast<uint8_t>((uintVal >> 8) & static_cast<uint32_t>(0xFF));
        data[offset+0] = static_cast<uint8_t>((uintVal >> 0) & static_cast<uint32_t>(0xFF));
    } else if(bytes == 8) {
        uint64_t uintVal;
        double floatVal;

        floatVal = static_cast<double>(value);

        mem_copy(&uintVal, &floatVal, (sizeof(uintVal) < sizeof(floatVal) ? sizeof(uintVal) : sizeof(floatVal)));
        
        data[offset+7] = static_cast<uint8_t>((uintVal >> 56) & static_cast<uint64_t>(0xFF));
        data[offset+6] = static_cast<uint8_t>((uintVal >> 48) & static_cast<uint64_t>(0xFF));
        data[offset+5] = static_cast<uint8_t>((uintVal >> 40) & static_cast<uint64_t>(0xFF));
        data[offset+4] = static_cast<uint8_t>((uintVal >> 32) & static_cast<uint64_t>(0xFF));
        data[offset+3] = static_cast<uint8_t>((uintVal >> 24) & static_cast<uint64_t>(0xFF));
        data[offset+2] = static_cast<uint8_t>((uintVal >> 16) & static_cast<uint64_t>(0xFF));
        data[offset+1] = static_cast<uint8_t>((uintVal >> 8) & static_cast<uint64_t>(0xFF));
        data[offset+0] = static_cast<uint8_t>((uintVal >> 0) & static_cast<uint64_t>(0xFF));
    } else {
        pdebug(DEBUG_WARN, "Unsupported floating point size %d!", bytes);
        rc = PLCTAG_ERR_UNSUPPORTED;
    }
    
    return rc;
}


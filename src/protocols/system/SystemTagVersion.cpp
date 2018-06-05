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

#include <lib/libplctag.h>
#include <system/SystemTagVersion.h>
#include <platform.h>
#include <util/attr.h>
#include <util/debug.h>


int SystemTagVersion::read()
{
    this->versionArray[0] = Tag::VERSION_ARRAY[0];
    this->versionArray[1] = Tag::VERSION_ARRAY[1];
    this->versionArray[2] = Tag::VERSION_ARRAY[2];
    
    return PLCTAG_STATUS_OK;
}


int SystemTagVersion::getSize()
{
    return sizeof(this->versionArray);
}


int SystemTagVersion::getInt(int offset, int bytes, uint64_t *result)
{
    if(offset != 0 && offset != 4 && offset != 8) {
        pdebug(DEBUG_WARN,"Only offsets of 0, 4 and 8 bytes are supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(bytes != 4) {
        pdebug(DEBUG_WARN,"Only integers of size 4 bytes are supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(result == nullptr) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    pdebug(DEBUG_DETAIL,"getting version array element %d with value %d",(offset/4),this->versionArray[offset/4]);
    
    *result = static_cast<uint64_t>(static_cast<int64_t>(this->versionArray[offset/4]));
    
    return PLCTAG_STATUS_OK;
}


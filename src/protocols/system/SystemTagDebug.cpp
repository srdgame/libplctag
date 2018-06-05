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

#include <inttypes.h>
#include <lib/libplctag.h>
#include <system/SystemTagDebug.h>
#include <platform.h>
#include <util/attr.h>
#include <util/debug.h>


int SystemTagDebug::read()
{
    this->debugLevel = get_debug_level();
    
    return PLCTAG_STATUS_OK;
}

int SystemTagDebug::write()
{
    set_debug_level(this->debugLevel);
    
    return PLCTAG_STATUS_OK;
}

int SystemTagDebug::getSize()
{
    return sizeof(int32_t);
}


int SystemTagDebug::getInt(int offset, int bytes, uint64_t *result)
{
    if(offset != 0) {
        pdebug(DEBUG_WARN,"Only offset 0 is supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(bytes != 4) {
        pdebug(DEBUG_WARN,"Only integers of size 4 bytes are supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(result == nullptr) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    *result = static_cast<uint64_t>(static_cast<int64_t>(this->debugLevel));
    
    return PLCTAG_STATUS_OK;
}


int SystemTagDebug::setInt(int offset, int bytes, uint64_t value)
{
    int32_t newLevel = static_cast<int32_t>(static_cast<int64_t>(value));
    
    if(offset != 0) {
        pdebug(DEBUG_WARN,"Only offset 0 is supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(bytes != 4) {
        pdebug(DEBUG_WARN,"Only integers of size 4 bytes are supported.");
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    if(newLevel < 0 || newLevel >= DEBUG_END) {
        return PLCTAG_ERR_BAD_PARAM;
    }

    this->debugLevel = newLevel;
}

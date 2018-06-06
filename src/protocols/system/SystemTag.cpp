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
#include <system/SystemTag.h>
#include <system/SystemTagDebug.h>
#include <system/SystemTagVersion.h>
#include <platform.h>
#include <util/attr.h>
#include <util/debug.h>


SystemTag::~SystemTag()
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO,"Done.");
}

int SystemTag::Create(attr attribs)
{
    const char *plcStr = attr_get_str(attribs, "plc", attr_get_str(attribs, "cpu", NULL));
    const char *name = NULL;
    tag_id id = PLCTAG_ERR_UNSUPPORTED;

    pdebug(DEBUG_INFO, "Starting.");
    
    if(!plcStr || str_length(plcStr) == 0) {
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    // compare the plc string.
    if(str_cmp_i(plcStr, "system") != 0) {
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    // it is a system tag.  Check the name.
    name = attr_get_str(attribs, "name", NULL);
    if(!name || str_length(name) == 0) {
        pdebug(DEBUG_WARN,"System tags must be 'debug' or 'version'!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    if(str_cmp_i(name, "debug") == 0) {
        SystemTagDebug *tag = new SystemTagDebug();
        
        if(!tag) {
            pdebug(DEBUG_WARN,"Unable to create a debug System tag!");
            return PLCTAG_ERR_CREATE;
        }
        
        id = Tag::addTag(tag);
        
        if(id < 0) {
            pdebug(DEBUG_WARN,"Error adding tag!");
            delete tag;
        }
    } else if(str_cmp_i(name, "version") == 0) {
        SystemTagVersion *tag = new SystemTagVersion();
        
        if(!tag) {
            pdebug(DEBUG_WARN,"Unable to create a version System tag!");
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


void SystemTag::abort() {}

int SystemTag::read()
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}

int SystemTag::write()
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


int SystemTag::getSize()
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}

int SystemTag::getInt(int offset, int bytes, uint64_t *result)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}

int SystemTag::setInt(int offset, int bytes, uint64_t value)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}

int SystemTag::getFloat(int offset, int bytes, double *result)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}

int SystemTag::setFloat(int offset, int bytes, double value)
{
    return PLCTAG_ERR_NOT_IMPLEMENTED;
}


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
#include <lib/Tag.h>
#include <util/debug.h>
#include <mutex>
#include <map>

static std::mutex tagMutex;
static std::map<int, ref<Tag> > tagMap;


static const int MAX_ID = 100000000;



// Must be called with the mutex held! 

static int nextId()
{
    static int lastID = 1;
    int iterations = MAX_ID;
    
    do {
        lastID++;
        
        if(lastID > MAX_ID) {
            lastID = 1;
        }
        
        if(tagMap.find(lastID) == tagMap.end()) {
            return lastID;
        }
    } while((--iterations) > 0);
    
    return PLCTAG_ERR_NO_RESOURCES;
}


int Tag::addTag(Tag *tag)
{
    // get the mutex first.
    std::lock_guard<std::mutex> guard(tagMutex);    
    int newID = nextId();
    
    if(newID > 0) {
        ref<Tag> tmp(tag);
        
        tag->id = newID;
        tagMap[newID] = tmp;
    }
    
    return newID;
}

int Tag::removeTag(int id)
{
    // get the mutex first.
    std::lock_guard<std::mutex> guard(tagMutex);
    std::map<int, ref<Tag> >::iterator it = tagMap.find(id);
    
    if(it != tagMap.end()) {
        if(it->second->id == id) {
            pdebug(DEBUG_DETAIL, "Removing tag %p from global map.", it->second);
            tagMap.erase(it);
            return PLCTAG_STATUS_OK;
        } else {
            pdebug(DEBUG_WARN, "Tag found (%d) does not have correct ID (%d)!", it->second->id, id);
            return PLCTAG_ERR_NOT_FOUND;
        }
    } else {
        pdebug(DEBUG_WARN, "Tag (%d) not found!", id);
        return PLCTAG_ERR_NOT_FOUND;
    }
}


ref<Tag> Tag::getTag(int id)
{
    // get the mutex first.
    std::lock_guard<std::mutex> guard(tagMutex);
    ref<Tag> result;
    std::map<int, ref<Tag> >::iterator it = tagMap.find(id);
    
    if(it != tagMap.end()) {
        if(it->second->id == id) {
            return it->second;
        } else {
            pdebug(DEBUG_WARN, "Tag found (%d) does not have correct ID (%d)!", it->second->id, id);
            return result;
        }
    } else {
        pdebug(DEBUG_WARN, "Tag (%d) not found!", id);
        return result;
    }
}

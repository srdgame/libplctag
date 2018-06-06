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
#include <thread>
#include <map>

static std::mutex tagMutex;
static std::map<int, std::shared_ptr<Tag> > tagMap;



// throwaway class to help force the handler thread to stop.
class TagHandlerWrapper {
public:
    TagHandlerWrapper();
    ~TagHandlerWrapper();
    
private:
    std::thread tagHandlerThread;
    std::atomic<bool> terminate;
    
    void tagHandler();
};

TagHandlerWrapper::TagHandlerWrapper()
{
    this->terminate = false;
    this->tagHandlerThread = std::thread(TagHandlerWrapper::tagHandler, this);
}

TagHandlerWrapper::~TagHandlerWrapper()
{
    this->terminate = true;
    this->tagHandlerThread.join();
}

void TagHandlerWrapper::tagHandler(void)
{
    // FIXME - this seems dangerous.  Will it ever terminate?
    // FIXME - this is probably not exception safe.
    while(!this->terminate) {
        // get the mutex first.
        tagMutex.lock();

        // iterate over the tags calling run().
        std::map<int, std::shared_ptr<Tag> >::iterator it;
        for(it = tagMap.begin(); it != tagMap.end(); it++) {
            std::shared_ptr<Tag> tag = it->second;
            
            if(tag) {
                pdebug(DEBUG_SPEW, "Calling run() on tag.");
                tag->run();
            }
        }
        
        tagMutex.unlock();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

static TagHandlerWrapper tagHandlerWrapper;

static const int MAX_ID = 100000000;

const char* Tag::VERSION_STR = "2.0.0";
const int32_t Tag::VERSION_ARRAY[3] = {2, 0, 0};




Tag::~Tag() 
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO, "Done.");
}


void Tag::lock()
{
    this->apiMutex.lock();
}

void Tag::unlock()
{
    this->apiMutex.unlock();    
}




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
        std::shared_ptr<Tag> tmp(tag);
        
        tag->id = newID;
        tagMap[newID] = tmp;
    }
    
    return newID;
}

int Tag::removeTag(int id)
{
    // get the mutex first.
    std::lock_guard<std::mutex> guard(tagMutex);
    std::map<int, std::shared_ptr<Tag> >::iterator it = tagMap.find(id);
    
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


std::shared_ptr<Tag> Tag::getTag(int id)
{
    // get the mutex first.
    std::lock_guard<std::mutex> guard(tagMutex);
    std::shared_ptr<Tag> result;
    std::map<int, std::shared_ptr<Tag> >::iterator it = tagMap.find(id);
    
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



/*
 * Stub implementations for all virtual methods.
 */
 
void Tag::run(void) {} 

 
void Tag::abort() {}
int Tag::read() { return PLCTAG_ERR_UNSUPPORTED; }
int Tag::write() { return PLCTAG_ERR_UNSUPPORTED; }

int Tag::getStatus() { return this->status; }
int Tag::setStatus(int newStatus) 
{
    int oldStatus = this->status; 
    
    this->status = newStatus; 
    
    return oldStatus;
}

    // tag data methods
int Tag::getSize() { return PLCTAG_ERR_UNSUPPORTED; }
int Tag::getInt(int offset, int bytes, uint64_t *result) { return PLCTAG_ERR_UNSUPPORTED; }
int Tag::setInt(int offset, int bytes, uint64_t value) { return PLCTAG_ERR_UNSUPPORTED; }
int Tag::getFloat(int offset, int bytes, double *result) { return PLCTAG_ERR_UNSUPPORTED; }
int Tag::setFloat(int offset, int bytes, double value) { return PLCTAG_ERR_UNSUPPORTED; }


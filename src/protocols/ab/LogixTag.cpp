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

#include <ab/LogixTag.h>
#include <util/debug.h>

LogixTag::LogixTag(attr attribs)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting.");
    
    this->status = PLCTAG_STATUS_OK;
    this->state = IDLE;
    
    this->name = attr_get_str(attribs,"name","");
    if(name.size() == 0) {
        pdebug(DEBUG_WARN,"All tags must have a name!");
        this->status = PLCTAG_ERR_BAD_PARAM;
    } else { 
        this->elemCount = attr_get_int(attribs,"elem_count",1);

        if(elemCount <= 0) {
            pdebug(DEBUG_WARN,"Element count must be a greater than zero!");
            this->status = PLCTAG_ERR_BAD_PARAM;
        } else {
            // get the PLC.
            this->plc = LogixPLC::Get(attribs);
            
            // is there a problem with the PLC?
            if(!this->plc) {
                pdebug(DEBUG_WARN,"Unable to get/create PLC, check PLC path!");
                this->setStatus(PLCTAG_ERR_BAD_GATEWAY);
            }
         }
    }
    
    pdebug(DEBUG_INFO,"Done.");
}

LogixTag::~LogixTag()
{
    pdebug(DEBUG_INFO,"Starting.");
    
    // clean up the reference to the PLC.
    if(this->plc) {
        this->plc = nullptr;
    }
    
    pdebug(DEBUG_INFO,"Done.");
}


    
void LogixTag::run(void)
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);

    switch(this->state) {
        case IDLE:
            // any new actions pending?
            if(this->plc->getStatus() == PLCTAG_STATUS_OK && this->status == PLCTAG_STATUS_PENDING) {
                // catch aborts early.
                if(this->readRequested) {
                    this->byteOffset = 0;
                    this->state = READING;
                } else if(this->writeRequested) {
                    this->byteOffset = 0;
                    this->state = WRITING;
                }
            }
            
            break;
            
        case READING:
            if(!this->request) {
                    // start a new read.
                    this->startRead();
            } else {
                int readStatus = this->request->getStatus();
                
                // is the request done?
                if(readStatus == PLCTAG_STATUS_OK) {
                    // this will set up another read if necessary.
                    this->processRead();
                } else if(readStatus != PLCTAG_STATUS_PENDING) {
                    // error of some sort.
                    pdebug(DEBUG_WARN,"Error, %s, while doing read!", plc_tag_decode_error(readStatus));
                }  // else just keep waiting.
            }
        
            break;
            
        case WRITING:
            if(!this->request) {
                    // start a new write.
                    this->startWrite();
            } else {
                int writeStatus = this->request->getStatus();
                
                // is the request done?
                if(writeStatus == PLCTAG_STATUS_OK) {
                    // this will set up another write if necessary.
                    this->processWrite();
                } else if(writeStatus != PLCTAG_STATUS_PENDING) {
                    // error of some sort.
                    pdebug(DEBUG_WARN,"Error, %s, while doing read!", plc_tag_decode_error(writeStatus));
                }  // else just keep waiting.
            }
        
            break;
    }
}
    
    
void LogixTag::abort()
{
    std::lock_guard<std::mutex> guard(this->sharedStateMutex);

    this->readRequested = false;
    this->writeRequested = false;
    this->request = nullptr;
    this->byteOffset = 0;
    
    // only change the status if we were doing something.
    if(this->status == PLCTAG_STATUS_PENDING) {
        this->status = PLCTAG_STATUS_OK;
    }
    
    this->state = IDLE;
}

void LogixTag::startRead()
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO, "Done.");
}


void LogixTag::processRead()
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO, "Done.");
}


void LogixTag::startWrite()
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO, "Done.");
}


void LogixTag::processWrite()
{
    pdebug(DEBUG_INFO,"Starting.");
    pdebug(DEBUG_INFO, "Done.");
}

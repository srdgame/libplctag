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

#include <platform.h>
#include <util/debug.h>
#include <ab/ABRequest.h>

ABRequest::ABRequest(int capacity)
{
    pdebug(DEBUG_DETAIL,"Starting.");

    this->data = new uint8_t[capacity+1];
    this->capacity = capacity;
    
    this->data[capacity] = UINT8_MAX;
    
    pdebug(DEBUG_DETAIL, "Done.");
}


ABRequest::~ABRequest()
{
    pdebug(DEBUG_DETAIL,"Starting.");
    
    if(this->data) {
        delete[] this->data;
        this->data = nullptr;
    }
    
    pdebug(DEBUG_DETAIL, "Done.");
}



int ABRequest::getStatus()
{
    return this->status.load();
}

int ABRequest::setStatus(int newStatus)
{
    int oldStatus = this->status.load();
    
    this->status.store(newStatus);
    
    return oldStatus;
}

void ABRequest::clear()
{
    mem_set(this->data, 0, this->capacity);
}

int ABRequest::size()
{ 
    return this->capacity;
}

uint8_t& ABRequest::operator[](int index)
{
    if(index >= 0 && index < capacity) {
        return this->data[index];
    }
    
    // extra byte for padding.
    return this->data[capacity];
}

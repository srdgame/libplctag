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

#pragma once

#include <mutex>
#include <util/RcObject.h>

class Tag : public RcObject {
public:
    static const char* VERSION_STR;
    static const int32_t VERSION_ARRAY[3];

    static int addTag(Tag *tag);
    static int removeTag(int id);
    static ref<Tag> getTag(int id);
    
    // generic
    void lock();
    void unlock();
    
    // subclass responsibility below.
    
    // tag operation methods
    virtual int abort();
    virtual int read();
    virtual int write();
    virtual int status();

    // tag data methods
    virtual int getSize();
    virtual int getInt(int offset, int bytes, uint64_t *result);
    virtual int setInt(int offset, int bytes, uint64_t value);
    virtual int getFloat(int offset, int bytes, double *result);
    virtual int setFloat(int offset, int bytes, double value);

    std::mutex apiMutex;
    
protected:
    int id = PLCTAG_ERR_NOT_FOUND;
};
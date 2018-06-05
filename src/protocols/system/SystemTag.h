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

#include <lib/libplctag.h>
#include <lib/Tag.h>
#include <util/attr.h>
#include <util/debug.h>

class SystemTag : public Tag
{
public:
    static Create(attr attribs);
    
    // tag operation methods, override these.
    virtual int abort() override;
    virtual int read() override;
    virtual int write() override;
    virtual int status() override;

    // tag data methods
    virtual int getSize() override;
    virtual int getInt(int offset, int bytes, uint64_t *result) override;
    virtual int setInt(int offset, int bytes, uint64_t value) override;
    virtual int getFloat(int offset, int bytes, double *result) override;
    virtual int setFloat(int offset, int bytes, double value) override;
};


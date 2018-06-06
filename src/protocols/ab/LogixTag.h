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

#include <atomic>
#include <ab/ABTag.h>
#include <ab/LogixPLC.h>
#include <util/attr.h>
#include <memory>

class LogixTag : public ABTag
{
public:
    LogixTag(attr attribs);
    virtual ~LogixTag();
    
    virtual void run(void) override;
    virtual void abort(void) override;

protected:
    void startRead();
    void processRead();
    void startWrite();
    void processWrite();

    std::string name;
    int elemCount;
    std::shared_ptr<LogixPLC> plc;
    std::shared_ptr<ABRequest> request;
    int byteOffset;
    enum { IDLE, READING, WRITING } state;
};


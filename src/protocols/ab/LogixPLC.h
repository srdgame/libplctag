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

#include <inttypes.h>
#include <thread>
#include <mutex>
#include <ab/ABRequest.h>
#include <util/attr.h>

class LogixPLC
{
public:
    static std::shared_ptr<LogixPLC> Get(attr attribs);

    LogixPLC(std::string path);
    virtual ~LogixPLC();

    int getStatus();
    
    const char *decodeCIPErrorShort(uint8_t *data);
    const char *decodeCIPErrorLong(uint8_t *data);
    int decodeCIPErrorCode(uint8_t *data);

protected:

    void run(void);
    int setStatus(int newStatus);
    
    bool terminate;
    std::mutex requestMutex;
    std::thread plcThread;
    
    // state
    std::atomic<int> status;
    enum { START } state;
    std::string path;
};


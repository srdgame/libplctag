/***************************************************************************
 * The following code was copied from the NanoGUI project.   The license   *
 * is governed by the file NanoGUI-LICENSE.txt in the root directory.      *
 *                                                                         *
 *   All modifications copyright (C) 2018 by Kyle Hayes                    *
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


#include <util/debug.h>
#include <util/RcObject.h>

/* the following was spliced out of NanoGUI's common.cpp */

/*
    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the NanoGUI-LICENSE file.
*/

void RcObject::decRef(bool dealloc) const noexcept {
    --m_refCount;
    
    pdebug(DEBUG_DETAIL,"m_refCount is now %d", m_refCount.load());
    
    if (m_refCount == 0 && dealloc) {
        pdebug(DEBUG_DETAIL,"Calling object destructor.");
        delete this;
    } else if (m_refCount < 0) {
        pdebug(DEBUG_WARN, "Internal error: Object reference count < 0!\n");
    }
}

RcObject::~RcObject() { }

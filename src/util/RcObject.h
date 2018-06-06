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

#pragma once

#include <atomic>
#include <util/debug.h>

/*
    nanogui/object.h -- Object base class with support for reference counting

    NanoGUI was developed by Wenzel Jakob <wenzel.jakob@epfl.ch>.
    The widget drawing code is based on the NanoVG demo application
    by Mikko Mononen.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the NanoGUI-LICENSE file.
*/
/** \file */


/**
 * \class Object object.h nanogui/object.h
 *
 * \brief Reference counted object base class.
 */
class RcObject {
public:
    /// Default constructor
    RcObject() { }

    /// Copy constructor
    RcObject(const RcObject &) : strongCount(0), weakCount(0) {}

    /// Return the current reference count
    int getStrongCount() const { return strongCount; };
    int getWeakCount() const { return strongCount; };

    /// Increase the object's reference count by one
    void incRef() const { ++strongCount; pdebug(DEBUG_DETAIL,"strongCount is now %d", strongCount.load()); }

    /** \brief Decrease the reference count of
     * the object and possibly deallocate it.
     *
     * The object will automatically be deallocated once
     * the reference count reaches zero.
     */
    void decRef(bool dealloc = true) const noexcept;
protected:
    /** \brief Virtual protected deconstructor.
     * (Will only be called by \ref ref)
     */
    virtual ~RcObject();
private:
    mutable std::atomic<int> strongCount { 0 };
    mutable std::atomic<int> weakCount { 0 };    
};

/**
 * \class ref object.h nanogui/object.h
 *
 * \brief Reference counting helper.
 *
 * The \a ref template is a simple wrapper to store a pointer to an object. It
 * takes care of increasing and decreasing the object's reference count as
 * needed. When the last reference goes out of scope, the associated object
 * will be deallocated.
 *
 * The advantage over C++ solutions such as ``std::shared_ptr`` is that
 * the reference count is very compactly integrated into the base object
 * itself.
 */
template <typename T> class ref {
public:
    /// Create a ``nullptr``-valued reference
    ref() { }

    /// Construct a reference from a pointer
    ref(T *ptr) : m_ptr(ptr) {
        if (m_ptr)
            ((RcObject *) m_ptr)->incRef();
    }

    /// Copy constructor
    ref(const ref &r) : m_ptr(r.m_ptr) {
        if (m_ptr)
            ((RcObject *) m_ptr)->incRef();
    }

    /// Move constructor
    ref(ref &&r) noexcept : m_ptr(r.m_ptr) {
        r.m_ptr = nullptr;
    }

    /// Destroy this reference
    ~ref() {
        if (m_ptr)
            ((RcObject *) m_ptr)->decRef();
    }

    /// Move another reference into the current one
    ref& operator=(ref&& r) noexcept {
        if (&r != this) {
            if (m_ptr)
                ((RcObject *) m_ptr)->decRef();
            m_ptr = r.m_ptr;
            r.m_ptr = nullptr;
        }
        return *this;
    }

    /// Overwrite this reference with another reference
    ref& operator=(const ref& r) noexcept {
        if (m_ptr != r.m_ptr) {
            if (r.m_ptr)
                ((RcObject *) r.m_ptr)->incRef();
            if (m_ptr)
                ((RcObject *) m_ptr)->decRef();
            m_ptr = r.m_ptr;
        }
        return *this;
    }

    /// Overwrite this reference with a pointer to another object
    ref& operator=(T *ptr) noexcept {
        if (m_ptr != ptr) {
            if (ptr)
                ((RcObject *) ptr)->incRef();
            if (m_ptr)
                ((RcObject *) m_ptr)->decRef();
            m_ptr = ptr;
        }
        return *this;
    }

    /// Compare this reference with another reference
    bool operator==(const ref &r) const { return m_ptr == r.m_ptr; }

    /// Compare this reference with another reference
    bool operator!=(const ref &r) const { return m_ptr != r.m_ptr; }

    /// Compare this reference with a pointer
    bool operator==(const T* ptr) const { return m_ptr == ptr; }

    /// Compare this reference with a pointer
    bool operator!=(const T* ptr) const { return m_ptr != ptr; }

    /// Access the object referenced by this reference
    T* operator->() { return m_ptr; }

    /// Access the object referenced by this reference
    const T* operator->() const { return m_ptr; }

    /// Return a C++ reference to the referenced object
    T& operator*() { return *m_ptr; }

    /// Return a const C++ reference to the referenced object
    const T& operator*() const { return *m_ptr; }

    /// Return a pointer to the referenced object
    operator T* () { return m_ptr; }

    /// Return a const pointer to the referenced object
    T* get() { return m_ptr; }

    /// Return a pointer to the referenced object
    const T* get() const { return m_ptr; }

    /// Check if the object is defined
    operator bool() const { return m_ptr != nullptr; }
private:
    T *m_ptr = nullptr;
};

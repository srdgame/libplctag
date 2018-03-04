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
#include <lib/libplctag.h>
#include <util/debug.h>
#include <util/mem.h>
#include <util/refcount.h>




/*
 * This is a rc struct that we use to make sure that we are able to align
 * the remaining part of the allocated block.
 */

struct rc {
    int strong_count;
    int weak_count;
    lock_t lock;
    rc_cleanup_func cleanup_func;
};

/* round up to the nearest 32 byte chunk */
#define RC_HEADER_SIZE (((int)((sizeof(struct rc)+31)/32))*32)

#define DATA_FROM_HEADER(rc) ((void*)(((uint8_t *)rc) + RC_HEADER_SIZE))
#define HEADER_FROM_DATA(data)  ((struct rc *)(((uint8_t *)data) - RC_HEADER_SIZE))


/*
 * rc_alloc3
 *
 * Allocate memory, replacing mem_alloc, and prepend reference counting data to the
 * memory block.
 */


void *rc_alloc(int size, rc_cleanup_func cleanup_func)
{
    int total_size = size + RC_HEADER_SIZE;  /* allocate a 32-byte chunk to align the "real" data. */
    struct rc *rc = mem_alloc(total_size);

    pdebug(DEBUG_DETAIL,"Allocating %d byte from a request of %d with result pointer %p",total_size, size, rc);

    if(!rc) {
        pdebug(DEBUG_WARN,"Unable to allocate sufficient memory!");
        
        return NULL;
    }
    
    rc->strong_count = 1;
    rc->weak_count = 0;
    rc->lock = LOCK_INIT;
    rc->cleanup_func = cleanup_func;
        
    /* return the address _past_ the rc struct. */
    return DATA_FROM_HEADER(rc);
}


/*
 * return the original pointer or NULL.
 * This is for usage like:
 * my_struct->some_field = rc_inc(rc_obj);
 */

void *rc_inc(void *data)
{
    struct rc *rc = NULL;
    int strong_count = 0;
    int weak_count = 0;

    if(!data) {
        pdebug(DEBUG_WARN,"Null pointer passed!");
        return (void *)data;
    }

    /*
     * The struct rc we want is before the memory pointer we get.
     * Thus we cast and subtract.
     */
    rc = HEADER_FROM_DATA(data);

    /* loop until we get the lock */
    while (!lock_acquire(&rc->lock)) {
        ; /* do nothing, just spin */
    }

    if(rc->strong_count > 0) {
        rc->strong_count++;
    } else {
        pdebug(DEBUG_WARN,"rc_inc() called on object, %p, that was already released!", data);
    }
    
    strong_count = rc->strong_count;
    weak_count = rc->weak_count;

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_SPEW,"Refcount on %p is now %d (strong), %d (weak)", data, strong_count, weak_count);

    return (void *)(strong_count == 0 ? NULL : data);
}



/*
 * return NULL.
 * This is for usage like:
 * my_struct->some_field = rc_dec(rc_obj);
 *
 * Note that the clean up function _MUST_ free the data pointer
 * passed to it.   It must clean up anything referenced by that data,
 * and the block itself using rc_free();
 */

void *rc_dec(void *data)
{
    struct rc *rc = NULL;
    int strong_count = 0;
    int weak_count = 0;

    if(!data) {
        pdebug(DEBUG_WARN,"Null pointer passed!");
        return NULL;
    }

    /*
     * The struct rc we want is before the memory pointer we get.
     * Thus we cast and subtract.
     *
     * This gets rid of the "const" part!
     */
    rc = HEADER_FROM_DATA(data);

    /* loop until we get the lock */
    while (!lock_acquire(&rc->lock)) {
        ; /* do nothing, just spin */
    }

    if(rc->strong_count > 0) {
        rc->strong_count--;
    } else {
        pdebug(DEBUG_WARN,"rc_dec() called on object that was already deleted!");
    }

    /* total count is what matters. */
    strong_count = rc->strong_count;
    weak_count = rc->weak_count;

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_SPEW,"Refcount on %p is now %d (strong), %d (weak)", data, strong_count, weak_count);

    if((strong_count + weak_count) <= 0) {
        pdebug(DEBUG_DETAIL,"Calling cleanup function.");

        rc->cleanup_func(data);
                
        /* all done, free the memory for the object itself. */
        mem_free(rc);
    }

    return NULL;
}





/*
 * return the original pointer or NULL.
 * This is for usage like:
 * my_struct->some_field = rc_weak_inc(rc_obj);
 * 
 * Return NULL if the object is already dead due to the
 * strong_count being zero.
 */

void *rc_weak_inc(void *data)
{
    struct rc *rc = NULL;
    int strong_count = 0;
    int weak_count = 0;

    if(!data) {
        pdebug(DEBUG_WARN,"Null pointer passed!");
        return (void *)data;
    }

    /*
     * The struct rc we want is before the memory pointer we get.
     * Thus we cast and subtract.
     */
    rc = HEADER_FROM_DATA(data);

    /* loop until we get the lock */
    while (!lock_acquire(&rc->lock)) {
        ; /* do nothing, just spin */
    }

    if(rc->strong_count > 0) {
        rc->weak_count++;
    } else {
        pdebug(DEBUG_WARN,"rc_weak_inc() called on object, %p, that was already released!", data);
    }

    strong_count = rc->strong_count;
    weak_count = rc->weak_count;

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_SPEW,"Refcount on %p is now %d (strong), %d (weak)", data, strong_count, weak_count);

    return (void *)(strong_count == 0 ? NULL : data);
}



/*
 * return NULL.
 * This is for usage like:
 * my_struct->some_field = rc_weak_dec(rc_obj);
 */

void *rc_weak_dec(void *data)
{
    struct rc *rc = NULL;
    int strong_count = 0;
    int weak_count = 0;

    if(!data) {
        pdebug(DEBUG_WARN,"Null pointer passed!");
        return NULL;
    }

    /*
     * The struct rc we want is before the memory pointer we get.
     * Thus we cast and subtract.
     *
     * This gets rid of the "const" part!
     */
    rc = HEADER_FROM_DATA(data);

    /* loop until we get the lock */
    while (!lock_acquire(&rc->lock)) {
        ; /* do nothing, just spin */
    }

    if(rc->weak_count > 0) {
        rc->weak_count--;
    } else {
        pdebug(DEBUG_WARN,"rc_weak_dec() called on object that was already deleted!");
    }

    /* total count is what matters. */
    strong_count = rc->strong_count;
    weak_count = rc->weak_count;

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_SPEW,"Refcount on %p is now %d (strong),  %d (weak)", data, strong_count, weak_count);

    if((strong_count + weak_count) <= 0) {
        pdebug(DEBUG_DETAIL,"Calling cleanup function.");

        rc->cleanup_func(data);
        
        /* all done, free the memory for the object itself. */
        mem_free(rc);
    }

    return NULL;
}


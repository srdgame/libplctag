/***************************************************************************
 *   Copyright (C) 2016 by Kyle Hayes                                      *
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
#include <platform.h>
#include <util/mem.h>
#include <util/refcount.h>
#include <util/debug.h>


typedef struct cleanup_entry *cleanup_entry_p;

struct cleanup_entry {
    cleanup_entry_p next;
    callback_func_t cleanup_func;
    
    /* extra arguments */
    void *arg2;
    void *arg3;
};


/*
 * This is a rc struct that we use to make sure that we are able to align
 * the remaining part of the allocated block.
 */

struct rc {
    int count;
    lock_t lock;
    cleanup_entry_p cleanup_callbacks;

//    union {
//        uint8_t dummy_u8;
//        uint16_t dummy_u16;
//        uint32_t dummy_u32;
//        uint64_t dummy_u64;
//        double dummy_double;
//        void *dummy_ptr;
//        void (*dummy_func)(void);
//    } dummy_align[];
};

#define RC_HEADER_SIZE (32)

#define DATA_FROM_HEADER(rc) ((void*)(((uint8_t *)rc) + RC_HEADER_SIZE))
#define HEADER_FROM_DATA(data)  ((struct rc *)(((uint8_t *)data) - RC_HEADER_SIZE))


/*
 * rc_alloc3
 *
 * Allocate memory, replacing mem_alloc, and prepend reference counting data to the
 * memory block.
 *
 * The clean up function must free the passed data using rc_free.   This will be a pointer into
 * the middle of a malloc'ed block of memory.
 */


void *rc_alloc2(int size, callback_func_t cleanup_func, void *arg2, void *arg3)
{
    int total_size = size + RC_HEADER_SIZE;  /* allocate a 32-byte chunk to align the "real" data. */
    struct rc *rc = mem_alloc(total_size);
    cleanup_entry_p cleanup_entry = mem_alloc(sizeof(*cleanup_entry));

    pdebug(DEBUG_DETAIL,"Allocating %d byte from a request of %d with result pointer %p",total_size, size, rc);

    if(!rc) {
        pdebug(DEBUG_WARN,"Unable to allocate sufficient memory!");
        
        if(cleanup_entry) {
            /* seems a bit weird that this would have succeeded, but it is possible. */
            mem_free(cleanup_entry);
        }
        
        return NULL;
    }
    
    if(!cleanup_entry) {
        pdebug(DEBUG_WARN, "Unable to allocate memory for cleanup callback entry!");
        
        mem_free(rc);
        
        return NULL;
    }

    rc->count = 1;
    rc->lock = LOCK_INIT;
    
    /* set up the clean up callback */
    cleanup_entry->cleanup_func = cleanup_func;
    cleanup_entry->arg2 = arg2;
    cleanup_entry->arg3 = arg3;
    
    rc->cleanup_callbacks = cleanup_entry;
    
    /* return the address _past_ the rc struct. */
    return DATA_FROM_HEADER(rc);
}



/* wrappers to make it easier to call with fewer arguments. */

void *rc_alloc1(int size, callback_func_t cleanup_func, void *arg1)
{
    return rc_alloc2(size, cleanup_func, arg1, NULL);
}


void *rc_alloc(int size, callback_func_t cleanup_func)
{
    return rc_alloc2(size, cleanup_func, NULL, NULL);
}



/*
 * return the original pointer or NULL.
 * This is for usage like:
 * my_struct->some_field = rc_inc(rc_obj);
 */

void *rc_inc(void *data)
{
    struct rc *rc = NULL;
    int count = 0;

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

    if(rc->count > 0) {
        rc->count++;
        count = rc->count;
    } else {
        pdebug(DEBUG_WARN,"rc_inc() called on object, %p, that was already released!", data);
        data = NULL;
    }

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_DETAIL,"Ref count on %p is now %d",data, count);

    return (void *)data;
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
    int count;

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

    if(rc->count > 0) {
        rc->count--;
    } else {
        pdebug(DEBUG_WARN,"rc_dec() called on object that was already deleted!");
    }

    count = rc->count;

    /* release the lock so that other things can get to it. */
    lock_release(&rc->lock);

    pdebug(DEBUG_DETAIL,"Refcount on %p is now %d", data, count);

    if(count <= 0) {
        pdebug(DEBUG_DETAIL,"Calling cleanup functions.");
        
        while(rc->cleanup_callbacks) {
            cleanup_entry_p entry = rc->cleanup_callbacks;
            
            /* step the pointer forward for the next loop. */
            rc->cleanup_callbacks = rc->cleanup_callbacks->next;
            
            /* we ignore the return value */
            entry->cleanup_func(data, entry->arg2, entry->arg3);
            
            mem_free(entry);
        }
        
        /* all done, free the memory for the object itself. */
        mem_free(rc);
    }

    return NULL;
}




int rc_add_cleanup(void *data, callback_func_t cleanup_func, void *arg2, void *arg3)
{
    struct rc *rc = NULL;
    cleanup_entry_p entry = mem_alloc(sizeof(*entry));

    if(!data) {
        pdebug(DEBUG_WARN,"Null pointer passed!");
        return PLCTAG_ERR_NULL_PTR;
    }
    
    if(!entry) {
        pdebug(DEBUG_WARN,"Unable to allocate memory for callback entry!");
        return PLCTAG_ERR_NO_MEM;
    }

    /*
     * The struct rc we want is before the memory pointer we get.
     * This gets rid of the "const" part!
     */
    rc = HEADER_FROM_DATA(data);
    
    /* set up the clean up callback */
    entry->cleanup_func = cleanup_func;
    entry->arg2 = arg2;
    entry->arg3 = arg3;
    entry->next = rc->cleanup_callbacks;
    rc->cleanup_callbacks = entry;
    
    return PLCTAG_STATUS_OK;
}

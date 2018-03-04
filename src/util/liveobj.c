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

#include <platform.h>
#include <lib/libplctag.h>
#include <lib/tag.h>
#include <util/debug.h>
#include <util/liveobj.h>
#include <util/refcount.h>


#define MAX_LIVE_OBJS (8192)

//typedef struct {
//    int type;
////    lock_t lock;
//    rc_ptr obj;
//} liveobject_t;



#define TYPE_FROM_NEXT(n) (uint16_t)(-n)

static liveobj_p liveobjs[MAX_LIVE_OBJS];
static volatile int last_alloc_index = (MAX_LIVE_OBJS/2);
static mutex_p liveobj_mutex = NULL;
static thread_p liveobj_thread = NULL;

static THREAD_FUNC(liveobj_tickler);
static int find_next_free_unsafe(int initial_index);
//static void mark_free_unsafe(int index);




int liveobj_add(liveobj_p obj, int type, liveobj_func obj_func)
{
    int index = PLCTAG_ERR_NO_RESOURCES;
    
    critical_block(liveobj_mutex) {
        index = find_next_free_unsafe(last_alloc_index);
        
        if(index != PLCTAG_ERR_NO_RESOURCES) {
            last_alloc_index = index;
            
            obj->type = type;
            obj->obj_func = obj_func;
            
            /* put the object into the live object array. */
            liveobjs[index] = rc_weak_inc(obj);

            break;
        }
    }
    
    return index;
}


liveobj_p liveobj_get(int id)
{
    liveobj_p obj = NULL;
    liveobj_p dead_obj = NULL;
    
    if(id < 0 || id >= MAX_LIVE_OBJS) {
        pdebug(DEBUG_WARN,"Illegal index!");
        return NULL;
    }
    
    critical_block(liveobj_mutex) {
        /* is there something here? */
        if(liveobjs[id]) {
            /* try to get a strong reference. */
            obj = rc_inc(liveobjs[id]);
            
            /* was there only a weak ref? */
            if(!obj && liveobjs[id]) {
                /* save this for later. */
                dead_obj = liveobjs[id];

                /* make sure no one gets it by accident. */
                liveobjs[id] = NULL;
            }
        }
    }
    
    if(dead_obj) {
        rc_weak_dec(dead_obj);
    }
    
    return obj;
}


int liveobj_remove(int id)
{
    int result = PLCTAG_STATUS_OK;
    
    if(id < 0 || id >= MAX_LIVE_OBJS) {
        pdebug(DEBUG_WARN,"Illegal index!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    critical_block(liveobj_mutex) {
        if(liveobjs[id]) {
            liveobjs[id] = rc_weak_dec(liveobjs[id]);
        } else {
            result = PLCTAG_ERR_NOT_FOUND;
        }
    }
    
    return result;
}




liveobj_p liveobj_find(liveobj_find_func finder, void *arg)
{
    rc_ptr result = NULL;
    
    critical_block(liveobj_mutex) {
        for(int index = 0; index < MAX_LIVE_OBJS; index++) {
            if(liveobjs[index]) {
                liveobj_p obj = rc_inc(liveobjs[index]);
                
                if(obj && finder(obj, obj->type, arg) == PLCTAG_STATUS_OK) {
                    /* found! */
                    result = obj;
                    break;
                }
            }
        }
    }
        
    return result;
}



/* Support functions. */


int liveobj_setup()
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");
    
    /* set up the object array. */
    for(int i=0; i < MAX_LIVE_OBJS; i++) {
        //liveobjs[i].lock = LOCK_INIT;
        //liveobjs[i].type = LIVEOBJ_TYPE_FREE;
        liveobjs[i] = NULL;
    }

    /* we do not initialize the last one, that is a sentinal */
    
    /* set up first new allocation location */
    last_alloc_index = (MAX_LIVE_OBJS/2);
    
    rc = mutex_create(&liveobj_mutex);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to create mutex!");
        return rc;
    }
   
    /* setup the thread. */
    rc = thread_create(&liveobj_thread, liveobj_tickler, 32*1024, NULL);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to create thread!");
        return rc;
    }
    
    pdebug(DEBUG_INFO,"Done.");
    
    return rc;
}


void liveobj_teardown()
{
    pdebug(DEBUG_INFO,"Starting.");
    
    thread_join(liveobj_thread);
    thread_destroy(&liveobj_thread);
    
    mutex_destroy(&liveobj_mutex);
    
    /* purge all the remaining objects. */
    for(int i=0; i < MAX_LIVE_OBJS; i++) {
        if(liveobjs[i]) {
            rc_weak_dec(liveobjs[i]);
        }
    }
    
    pdebug(DEBUG_INFO,"Done.");
}


int find_next_free_unsafe(int initial_index)
{
    for(int offset=0; offset < MAX_LIVE_OBJS; offset++) {
        int index = (offset + initial_index) % MAX_LIVE_OBJS;
        
        if(!liveobjs[index]) {
            return index;
        }
    }
    
    return PLCTAG_ERR_NO_RESOURCES;
}

///* hold the mutex before calling! */
//void mark_free_unsafe(int index)
//{
//    if(index <0 || index >= MAX_LIVE_OBJS) {
//        pdebug(DEBUG_WARN, "Called with illegal index %d!", index);
//        return;
//    }
//    
//    liveobjs[index].obj = NULL;
//    liveobjs[index].type = LIVEOBJ_TYPE_FREE;
//}


THREAD_FUNC(liveobj_tickler)
{
    (void) arg;
    
    pdebug(DEBUG_INFO,"Starting.");
    
    while(!library_terminating) {
        for(int index=0; index < MAX_LIVE_OBJS; index++) {
            liveobj_p obj = NULL;
            
            /* see if there is an object, if so, run it. */
            obj = liveobj_get(index);
            
            if(obj) {
                pdebug(DEBUG_DETAIL,"Found something to run! %p",obj);
                /* something is there, run it. */
                obj->obj_func(obj);
                
                /* release the reference. */
                rc_dec(obj);
            }
        }

        /* give up CPU */
        sleep_ms(1);
    }
    
    /* any weak refs still around on termination are cleaned up at library cleanup. */
    
    pdebug(DEBUG_INFO,"Done.");
    
    THREAD_RETURN(NULL);
}
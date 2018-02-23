/***************************************************************************
 *   Copyright (C) 2018 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Lesser General Public License  as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Lesser General Public License  for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <lib/libplctag.h>
#include <util/atomic.h>



#define ATOMIC_UNLOCK_VAL ((lock_t)(0))
#define ATOMIC_LOCK_VAL ((lock_t)(1))

/*
 * lock_acquire
 *
 * Tries to write a non-zero value into the lock atomically.
 *
 * Returns non-zero on success.
 *
 * Warning: do not pass null pointers!
 */

#ifdef _WIN32
int lock_acquire(lock_t *lock)
{
    LONG rc = InterlockedExchange(lock, ATOMIC_LOCK_VAL);

    if(rc != ATOMIC_LOCK_VAL) {
        /* we got the lock */
        /*pdebug("got lock");*/
        return 1;
    } else {
        /* we did not get the lock */
        /*pdebug("did not get lock");*/
        return 0;
    }
}
    
void lock_release(lock_t *lock)
{
    InterlockedExchange(lock, ATOMIC_UNLOCK_VAL);
}



#else

/* POSIX version */
    
    
int lock_acquire(lock_t *lock)
{
    int rc = __sync_lock_test_and_set((int*)lock, ATOMIC_LOCK_VAL);

    if(rc != ATOMIC_LOCK_VAL) {
        /* we got the lock */
        /*pdebug("got lock");*/
        return 1;
    } else {
        /* we did not get the lock */
        /*pdebug("did not get lock");*/
        return 0;
    }
}

void lock_release(lock_t *lock)
{
    __sync_lock_release((int*)lock);
}



#endif




int lock_acquire_wait(lock_t *lock)
{
    while(!lock_acquire(lock)) {
        ; /* do nothing but spin. */
    }
    
    return PLCTAG_STATUS_OK;
}



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

#pragma once

#ifndef PLCTAG_CAT2
#define PLCTAG_CAT2(a,b) a##b
#endif

#ifndef PLCTAG_CAT
#define PLCTAG_CAT(a,b) PLCTAG_CAT2(a,b)
#endif

#ifndef LINE_ID
#define LINE_ID(base) PLCTAG_CAT(base,__LINE__)
#endif

#define spin_block(lock) \
for(int LINE_ID(__sync_flag_nargle_) = 1; LINE_ID(__sync_flag_nargle_); LINE_ID(__sync_flag_nargle_) = 0, lock_release(lock))  for(int LINE_ID(__sync_rc_nargle_) = lock_acquire_wait(lock); LINE_ID(__sync_rc_nargle_) == PLCTAG_STATUS_OK && LINE_ID(__sync_flag_nargle_) ; LINE_ID(__sync_flag_nargle_) = 0)

    
    /* atomic operations */
#ifdef _WIN32
typedef volatile long int lock_t;
#else
typedef volatile int lock_t;
#endif 

#define LOCK_INIT ((lock_t)0)

/* returns non-zero when lock acquired, zero when lock operation failed */
extern int lock_acquire(lock_t *lock);
extern void lock_release(lock_t *lock);
extern int lock_acquire_wait(lock_t *lock);

#define MAKE_ATOMIC_TYPE(atomic_type, val_type) typedef struct { lock_t lock; val_type val; } atomic_type; \
inline static val_type atomic_type##_get(atomic_type *atomic_val) { val_type result; spin_block(&(atomic_val->lock)) { result = atomic_val->val; } return result; } \
inline static void atomic_type##_set(atomic_type *atomic_val, val_type val) { spin_block(&(atomic_val->lock)) { atomic_val->val = val; } } \
inline static void atomic_type##_init(atomic_type *atomic_val, val_type initial_val) { atomic_val->lock = LOCK_INIT; atomic_val->val = initial_val; }


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
#include <util/bitmap.h>
#include <util/debug.h>

struct bitmap_t {
    int size;
    uint64_t data[];
};


bitmap_p bitmap_create(int size)
{
    bitmap_p result = NULL;
    int size_in_bits = 0;
    int size_in_bytes = 0;
    
    if(size < 0) {
        pdebug(DEBUG_WARN,"Size must be positive!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    /* round size up to the nearest chunk of 64 bits. */
    size_in_bytes = (size + (64 - (size % 64)))/8;
    
    result = mem_alloc(sizeof(struct bitmap_t) + size_in_bytes);
    if(!result) {
        pdebug(DEBUG_ERROR,"Unable to allocate bitmap!");
        return PLCTAG_ERR_NO_MEM;
    }
    
}
int bitmap_set(bitmap_p bm, int index, int val);
int bitmap_get(bitmap_p bm, int index);
int bitmap_find_first(bitmap_p bm, int start_index, int val);
int bitmap_destroy(bitmap_p bm);


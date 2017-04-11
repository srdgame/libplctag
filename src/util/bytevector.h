/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
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


#ifndef __UTIL_BYTEVECTOR_H__
#define __UTIL_BYTEVECTOR_H__

typedef struct byte_vector_t *byte_vector;

extern byte_vector byte_vector_create(size_t capacity, size_t inc);
extern void byte_vector_destroy(byte_vector vec);
extern size_t byte_vector_size(byte_vector vec);
extern size_t byte_vector_capacity(byte_vector vec);
extern uint8_t *byte_vector_elements(byte_vector vec);
extern size_t byte_vector_ensure_capacity(byte_vector vec, size_t new_size) 
extern size_t byte_vector_splice(byte_vector vec, size_t loc, uint8_t *data, size_t data_len);
extern size_t byte_vector_cut(byte_vector vec, size_t loc, size_t len);
extern uint8_t byte_vector_get(byte_vector vec, size_t loc);
extern size_t byte_vector_set(byte_vector vec, size_t loc, uint8_t val);
extern size_t byte_vector_push(byte_vector vec, uint8_t val);
extern size_t byte_vector_pop(byte_vector vec, uint8_t *val);
extern size_t byte_vector_append(byte_vector vec, uint8_t val);
extern size_t byte_vector_trunc(byte_vector vec, uint8_t *val);

#endif


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


#include <platform.h>
#include <util/debug.h>
#include <util/bytevector.h>
#include <util/encdec.h>




struct byte_vector_t { 
	size_t capacity; 
	size_t inc; 
	size_t size; 
	uint8_t *elements; 
}; 


byte_vector byte_vector_create(size_t capacity, size_t inc) 
{ 
	byte_vector new_vec = NULL;
	
	if(capacity <= 0) { 
		return NULL; 
	} 
		
	new_vec = mem_alloc(sizeof(struct byte_vector_t)); 
	if(!new_vec) { 
		return NULL; 
	} 
	
	new_vec->elements = mem_alloc(capacity * sizeof(uint8_t)); 
	if(!new_vec->elements) { 
		mem_free(new_vec); 
		return NULL; 
	} 
	
	new_vec->capacity = capacity; 
	new_vec->inc = inc; 
	new_vec->size = 0; 
	
	return new_vec; 
} 

void byte_vector_destroy(byte_vector vec) 
{ 
	if(!vec) return; 
	if(vec->elements) mem_free(vec->elements); 
	mem_free(vec); 
} 

size_t byte_vector_size(byte_vector vec) 
{ 
	if(vec) return vec->size; 
	else return SIZE_MAX; 
} 

size_t byte_vector_capacity(byte_vector vec) 
{ 
	if(vec) return vec->capacity; 
	else return SIZE_MAX; 
} 

uint8_t *byte_vector_elements(byte_vector vec) 
{ 
	if(!vec) return NULL; 
	return vec->elements; 
} 

size_t byte_vector_ensure_capacity(byte_vector vec, size_t new_size) 
{ 
	if(!vec) return SIZE_MAX; 
	if(new_size > vec->capacity) { 
		size_t new_cap = ((new_size + vec->inc - 1) / vec->inc) * vec->inc; 
		uint8_t *new_elements = mem_alloc(new_cap * sizeof(uint8_t)); 
		
		if(!new_elements) return SIZE_MAX; 
		
		for(size_t elem=0; elem < vec->size; elem++) { 
			new_elements[elem] = vec->elements[elem]; 
		} 
		
		mem_free(vec->elements); 
		
		vec->elements = new_elements; 
		vec->capacity = new_cap; 
	} 
	
	return vec->capacity; 
} 

size_t byte_vector_splice(byte_vector vec, size_t loc, uint8_t *data, size_t data_len) 
{ 
	if(!vec || loc > vec->size) return SIZE_MAX; 
	
	size_t new_size = vec->size + data_len; 
	
	if(byte_vector_ensure_capacity(vec, new_size) == SIZE_MAX) return SIZE_MAX; 
	
	size_t move_up_amt = vec->size - loc; 
	
	/* FIXME - this should be converted to mem_move()!! */
	for( ; move_up_amt; move_up_amt--) { 
		vec->elements[loc + move_up_amt + data_len - 1] = vec->elements[loc + move_up_amt - 1]; 
	} 
	
	for(size_t elem = 0; elem < data_len; elem++) { 
		vec->elements[elem+loc] = data[elem]; 
	} 
	
	vec->size = new_size; 
	
	return loc + data_len; 
}

size_t byte_vector_cut(byte_vector vec, size_t loc, size_t len) 
{ 
	if(!vec) return SIZE_MAX; 
	if(loc > vec->size) return SIZE_MAX; 
	
	if((loc + len) > vec->size) { 
		len = vec->size - loc; 
	} 
	
	for(size_t elem = loc + len; elem < vec->size; elem++) { 
		vec->elements[elem - len] = vec->elements[elem]; 
	} 
	
	vec->size = vec->size - len; 
	
	return loc; 
} 

uint8_t byte_vector_get(byte_vector vec, size_t loc) 
{ 
	return vec->elements[loc]; 
} 

size_t byte_vector_set(byte_vector vec, size_t loc, uint8_t val) 
{ 
	if(!vec || loc >= vec->size) return SIZE_MAX; 
	
	vec->elements[loc] = (val); 
	
	return loc; 
} 

size_t byte_vector_push(byte_vector vec, uint8_t val) 
{ 
	if(!vec) return SIZE_MAX; 
	
	return byte_vector_splice(vec, 0, &val, (size_t)1); 
} 

size_t byte_vector_pop(byte_vector vec, uint8_t *val) 
{ 
	if(!vec || vec->size <= 0) return SIZE_MAX; 
	
	*val = byte_vector_get(vec, (size_t)0); 
	
	return byte_vector_cut(vec, (size_t)0, (size_t)1); 
} 

size_t byte_vector_append(byte_vector vec, uint8_t val) 
{ 
	if(!vec || byte_vector_ensure_capacity(vec, vec->size+1) == SIZE_MAX) return SIZE_MAX; 
	
	vec->elements[vec->size] = val; vec->size++; 
	
	return vec->size; 
} 
	
size_t byte_vector_trunc(byte_vector vec, uint8_t *val) 
{ 
	if(!vec && vec->size <= 0) return SIZE_MAX; 
	
	*val = vec->elements[vec->size - 1]; 
	vec->size--; 
	
	return vec->size; 
}

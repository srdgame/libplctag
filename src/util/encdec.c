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
#include <util/encdec.h>



/********************************************************************************
 *************************  Byte order functions ********************************
 *******************************************************************************/
 
 

int int8_encode(uint8_t *data, size_t max_size, int offset, int8_t val)
{
    /* is there enough space */
    if((offset < 0) || ((size_t)offset >= max_size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    data[offset] = (uint8_t)(val);

	return offset + sizeof(int8_t);
}

int int8_decode(uint8_t *data, size_t max_size, int offset, int8_t *val)
{
    /* is there enough data */
    if((offset < 0) || ((size_t)offset >= max_size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    *val = (int8_t)(data[offset]);
    
    return offset + sizeof(int8_t);
}


int uint8_encode(uint8_t *data, size_t max_size, int offset, uint8_t val)
{
    /* is there enough space */
    if((offset < 0) || ((size_t)offset >= max_size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    data[offset] = val;

	return offset + sizeof(uint8_t);	
}

int uint8_decode(uint8_t *data, size_t max_size, int offset, uint8_t *val)
{
    /* is there enough data */
    if((offset < 0) || ((size_t)offset >= max_size)) {
        pdebug(DEBUG_WARN,"Offset is out of bounds!");
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }

    *val = (data[offset]);
    
    return offset + sizeof(uint8_t);	
}

#define INT_ENCODE_DECODE_TEMPLATE(I_TYPE)                             \
int I_TYPE ## _encode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(I_TYPE ## _t)], I_TYPE ## _t val)\
{                                                                      \
	size_t i;                                                          \
                                                                       \
    /* is there enough data space to write the value? */               \
    if((offset < 0) || ((size_t)offset + sizeof(I_TYPE ## _t) > max_size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return PLCTAG_ERR_OUT_OF_BOUNDS;                               \
    }                                                                  \
                                                                       \
    for(i=0; i < sizeof(I_TYPE ## _t); i++) {                          \
		data[offset + byte_order[i]] = (uint8_t)((val >> (8*i)) & 0xFF);\
	}                                                                  \
	                                                                   \
	return offset + sizeof(I_TYPE ## _t);                              \
}                                                                      \
int I_TYPE ## _decode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(I_TYPE ## _t)], I_TYPE ## _t *val)\
{                                                                      \
	size_t i;                                                          \
                                                                       \
    /* is there enough data space to read the value? */                \
    if((offset < 0) || ((size_t)offset + sizeof(I_TYPE ## _t) > max_size)) {\
        pdebug(DEBUG_WARN,"Offset is out of bounds!");                 \
        return PLCTAG_ERR_OUT_OF_BOUNDS;                               \
    }                                                                  \
                                                                       \
    *val = 0;                                                          \
    for(i=0; i < sizeof(I_TYPE ## _t); i++) {                          \
		*val += ((I_TYPE ## _t)(data[offset + byte_order[i]]) << (8*i));\
	}                                                                  \
	                                                                   \
	return offset + sizeof(I_TYPE ## _t);                              \
}                                                                      \

INT_ENCODE_DECODE_TEMPLATE(int16)

INT_ENCODE_DECODE_TEMPLATE(uint16)

INT_ENCODE_DECODE_TEMPLATE(int32)

INT_ENCODE_DECODE_TEMPLATE(uint32)

INT_ENCODE_DECODE_TEMPLATE(int64)

INT_ENCODE_DECODE_TEMPLATE(uint64)


int float_encode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(float)], float val)
{ 
	size_t i;
	uint32_t uval;
	
	if((offset < 0) || ((size_t)offset + sizeof(float) > max_size)) { 
		pdebug(DEBUG_WARN,"Offset is out of bounds!"); 
		return PLCTAG_ERR_OUT_OF_BOUNDS; 
	} 
	
	mem_copy(&uval,&val,sizeof(uint32_t));	
	for(i=0; i < sizeof(float); i++) { 
		data[offset + byte_order[i]] = (uint8_t)((uval >> (8*i)) & 0xFF); 
	} 
	
	return offset + sizeof(float); 
} 

int float_decode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(float)], float *val)
{ 
	size_t i; 
	uint32_t uval;
	
	if((offset < 0) || ((size_t)offset + sizeof(float) > max_size)) { 
		pdebug(DEBUG_WARN,"Offset is out of bounds!"); 
		return PLCTAG_ERR_OUT_OF_BOUNDS; 
	} 
	
	uval = 0; 
	for(i=0; i < sizeof(float); i++) { 
		uval += ((uint32_t)(data[offset + byte_order[i]]) << (8*i)); 
	}
	mem_copy(val,&uval,sizeof(float));
	
	return offset + sizeof(float); 
}


int double_encode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(double)], double val)
{ 
	size_t i;
	uint64_t uval;
	
	if((offset < 0) || ((size_t)offset + sizeof(double) > max_size)) { 
		pdebug(DEBUG_WARN,"Offset is out of bounds!"); 
		return PLCTAG_ERR_OUT_OF_BOUNDS; 
	} 
	
	mem_copy(&uval,&val,sizeof(uint64_t));	
	for(i=0; i < sizeof(double); i++) { 
		data[offset + byte_order[i]] = (uint8_t)((uval >> (8*i)) & 0xFF); 
	} 
	
	return offset + sizeof(double); 
}

int double_decode(uint8_t *data, size_t max_size, int offset, uint8_t byte_order[sizeof(double)], double *val)
{ 
	size_t i; 
	uint64_t uval;
	
	if((offset < 0) || ((size_t)offset + sizeof(double) > max_size)) { 
		pdebug(DEBUG_WARN,"Offset is out of bounds!"); 
		return PLCTAG_ERR_OUT_OF_BOUNDS; 
	} 
	
	uval = 0; 
	for(i=0; i < sizeof(double); i++) { 
		uval += ((uint64_t)(data[offset + byte_order[i]]) << (8*i)); 
	}
	mem_copy(val,&uval,sizeof(double));
	
	return offset + sizeof(double); 
}




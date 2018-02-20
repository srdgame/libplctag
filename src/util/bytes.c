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
#include <util/bytes.h>
#include <util/mem.h>
#include <stdint.h>

int encode_int16_le(uint8_t *buf, int16_t val)
{
    uint16_t uval = (uint16_t)val;
    
    buf[0] = (uval & 0xFF);
    buf[1] = ((uval >> 8) & 0xFF);
    
    return PLCTAG_STATUS_OK;
}

int decode_int16_le(uint8_t *buf, int16_t *val)
{
    uint16_t uval = 0;
    
    uval = ((uint16_t)buf[1] << 8) | ((uint16_t)buf[0]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}



int encode_int32_le(uint8_t *buf, int32_t val)
{
    uint32_t uval = (uint32_t)val;
    
    buf[0] = (uval & 0xFF);
    buf[1] = ((uval >> 8) & 0xFF);
    buf[2] = ((uval >> 16) & 0xFF);
    buf[3] = ((uval >> 24) & 0xFF);
    
    return PLCTAG_STATUS_OK;
}

int decode_int32_le(uint8_t *buf, int32_t *val)
{
    uint32_t uval = 0;
    
    uval = ((uint32_t)buf[3] << 24) | ((uint32_t)buf[2] << 16) | ((uint32_t)buf[1] << 8) | ((uint32_t)buf[0]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}




int encode_int64_le(uint8_t *buf, int64_t val)
{
    uint64_t uval = (uint64_t)val;
    
    buf[0] = (uval & 0xFF);
    buf[1] = ((uval >> 8) & 0xFF);
    buf[2] = ((uval >> 16) & 0xFF);
    buf[3] = ((uval >> 24) & 0xFF);
    buf[4] = ((uval >> 32) & 0xFF);
    buf[5] = ((uval >> 40) & 0xFF);
    buf[6] = ((uval >> 48) & 0xFF);
    buf[7] = ((uval >> 56) & 0xFF);

    return PLCTAG_STATUS_OK;
}

int decode_int64_le(uint8_t *buf, int64_t *val)
{
    uint64_t uval = 0;
    
    uval =  ((uint64_t)buf[7] << 56) | ((uint64_t)buf[6] << 48) | ((uint64_t)buf[5] << 40) | ((uint64_t)buf[4] << 32) | 
            ((uint64_t)buf[3] << 24) | ((uint64_t)buf[2] << 16) | ((uint64_t)buf[1] << 8) | ((uint64_t)buf[0]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}





int encode_float32_le(uint8_t *buf, float val)
{
    uint32_t uval = 0;
    
    mem_copy(&uval, &val, sizeof(uval));
    
    return encode_int32_le(buf, (int32_t)uval);
}


int decode_float32_le(uint8_t *buf, float *val)
{
    uint32_t uval = 0;
    int rc = PLCTAG_STATUS_OK;
    
    rc = decode_int32_le(buf, (int32_t*)&uval);
    
    if(rc == PLCTAG_STATUS_OK) {
        mem_copy(val, &uval, sizeof(*val));
    }
    
    return rc;
}






int encode_float64_le(uint8_t *buf, double val)
{
    uint64_t uval = 0;
    
    mem_copy(&uval, &val, sizeof(uval));
    
    return encode_int64_le(buf, (int64_t)uval);
}


int decode_float64_le(uint8_t *buf, double *val)
{
    uint64_t uval = 0;
    int rc = PLCTAG_STATUS_OK;
    
    rc = decode_int64_le(buf, (int64_t*)&uval);
    
    if(rc == PLCTAG_STATUS_OK) {
        mem_copy(val, &uval, sizeof(*val));
    }
    
    return rc;
}







int encode_int16_be(uint8_t *buf, int16_t val)
{
    uint16_t uval = (uint16_t)val;
    
    buf[1] = (uval & 0xFF);
    buf[0] = ((uval >> 8) & 0xFF);
    
    return PLCTAG_STATUS_OK;
}

int decode_int16_be(uint8_t *buf, int16_t *val)
{
    uint16_t uval = 0;
    
    uval = ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}



int encode_int32_be(uint8_t *buf, int32_t val)
{
    uint32_t uval = (uint32_t)val;
    
    buf[3] = (uval & 0xFF);
    buf[2] = ((uval >> 8) & 0xFF);
    buf[1] = ((uval >> 16) & 0xFF);
    buf[0] = ((uval >> 24) & 0xFF);
    
    return PLCTAG_STATUS_OK;
}

int decode_int32_be(uint8_t *buf, int32_t *val)
{
    uint32_t uval = 0;
    
    uval = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}




int encode_int64_be(uint8_t *buf, int64_t val)
{
    uint64_t uval = (uint64_t)val;
    
    buf[7] = (uval & 0xFF);
    buf[6] = ((uval >> 8) & 0xFF);
    buf[5] = ((uval >> 16) & 0xFF);
    buf[4] = ((uval >> 24) & 0xFF);
    buf[3] = ((uval >> 32) & 0xFF);
    buf[2] = ((uval >> 40) & 0xFF);
    buf[1] = ((uval >> 48) & 0xFF);
    buf[0] = ((uval >> 56) & 0xFF);

    return PLCTAG_STATUS_OK;
}

int decode_int64_be(uint8_t *buf, int64_t *val)
{
    uint64_t uval = 0;
    
    uval =  ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) | ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) | 
            ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) | ((uint64_t)buf[6] << 8) | ((uint64_t)buf[7]);

    *val = (int16_t)uval;
    
    return PLCTAG_STATUS_OK;
}





int encode_float32_be(uint8_t *buf, float val)
{
    uint32_t uval = 0;
    
    mem_copy(&uval, &val, sizeof(uval));
    
    return encode_int32_be(buf, (int32_t)uval);
}


int decode_float32_be(uint8_t *buf, float *val)
{
    uint32_t uval = 0;
    int rc = PLCTAG_STATUS_OK;
    
    rc = decode_int32_be(buf, (int32_t*)&uval);
    
    if(rc == PLCTAG_STATUS_OK) {
        mem_copy(val, &uval, sizeof(*val));
    }
    
    return rc;
}






int encode_float64_be(uint8_t *buf, double val)
{
    uint64_t uval = 0;
    
    mem_copy(&uval, &val, sizeof(uval));
    
    return encode_int64_be(buf, (int64_t)uval);
}


int decode_float64_be(uint8_t *buf, double *val)
{
    uint64_t uval = 0;
    int rc = PLCTAG_STATUS_OK;
    
    rc = decode_int64_be(buf, (int64_t*)&uval);
    
    if(rc == PLCTAG_STATUS_OK) {
        mem_copy(val, &uval, sizeof(*val));
    }
    
    return rc;
}



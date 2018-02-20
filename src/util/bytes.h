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

#include <stdint.h>

extern int encode_int16_le(uint8_t *buf, int16_t val);
extern int decode_int16_le(uint8_t *buf, int16_t *val);

extern int encode_int32_le(uint8_t *buf, int32_t val);
extern int decode_int32_le(uint8_t *buf, int32_t *val);

extern int encode_int64_le(uint8_t *buf, int64_t val);
extern int decode_int64_le(uint8_t *buf, int64_t *val);

extern int encode_float32_le(uint8_t *buf, float val);
extern int decode_float32_le(uint8_t *buf, float *val);

extern int encode_float64_le(uint8_t *buf, double val);
extern int decode_float64_le(uint8_t *buf, double *val);


extern int encode_int16_be(uint8_t *buf, int16_t val);
extern int decode_int16_be(uint8_t *buf, int16_t *val);

extern int encode_int32_be(uint8_t *buf, int32_t val);
extern int decode_int32_be(uint8_t *buf, int32_t *val);

extern int encode_int64_be(uint8_t *buf, int64_t val);
extern int decode_int64_be(uint8_t *buf, int64_t *val);

extern int encode_float32_be(uint8_t *buf, float val);
extern int decode_float32_be(uint8_t *buf, float *val);

extern int encode_float64_be(uint8_t *buf, double val);
extern int decode_float64_be(uint8_t *buf, double *val);




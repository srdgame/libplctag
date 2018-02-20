/***************************************************************************
 *   Copyright (C) 2015 by OmanTek                                         *
 *   Author Kyle Hayes  kylehayes@omantek.com                              *
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

/**************************************************************************
 * CHANGE LOG                                                             *
 *                                                                        *
 * 2015-09-12  KRH - Created file.                                        *
 *                                                                        *
 **************************************************************************/

#ifndef __PLCTAG_AB_AB_COMMON_H__
#define __PLCTAG_AB_AB_COMMON_H__ 1

#include <lib/libplctag.h>
#include <lib/tag.h>
#include <inttypes.h>

/* CIP embedded packet commands */
#define AB_EIP_CMD_CIP_READ             ((uint8_t)0x4C)
#define AB_EIP_CMD_CIP_WRITE            ((uint8_t)0x4D)
#define AB_EIP_CMD_CIP_READ_FRAG        ((uint8_t)0x52)
#define AB_EIP_CMD_CIP_WRITE_FRAG       ((uint8_t)0x53)

/* CIP packet status */
#define AB_CIP_STATUS_OK                ((uint8_t)0x00)
#define AB_CIP_STATUS_FRAG              ((uint8_t)0x06)

/* CIP data type byte values */
#define AB_CIP_DATA_BIT         ((uint8_t)0xC1) /* Boolean value, 1 bit */
#define AB_CIP_DATA_SINT        ((uint8_t)0xC2) /* Signed 8–bit integer value */
#define AB_CIP_DATA_INT         ((uint8_t)0xC3) /* Signed 16–bit integer value */
#define AB_CIP_DATA_DINT        ((uint8_t)0xC4) /* Signed 32–bit integer value */
#define AB_CIP_DATA_LINT        ((uint8_t)0xC5) /* Signed 64–bit integer value */
#define AB_CIP_DATA_USINT       ((uint8_t)0xC6) /* Unsigned 8–bit integer value */
#define AB_CIP_DATA_UINT        ((uint8_t)0xC7) /* Unsigned 16–bit integer value */
#define AB_CIP_DATA_UDINT       ((uint8_t)0xC8) /* Unsigned 32–bit integer value */
#define AB_CIP_DATA_ULINT       ((uint8_t)0xC9) /* Unsigned 64–bit integer value */
#define AB_CIP_DATA_REAL        ((uint8_t)0xCA) /* 32–bit floating point value, IEEE format */
#define AB_CIP_DATA_LREAL       ((uint8_t)0xCB) /* 64–bit floating point value, IEEE format */
#define AB_CIP_DATA_STIME       ((uint8_t)0xCC) /* Synchronous time value */
#define AB_CIP_DATA_DATE        ((uint8_t)0xCD) /* Date value */
#define AB_CIP_DATA_TIME_OF_DAY ((uint8_t)0xCE) /* Time of day value */
#define AB_CIP_DATA_DATE_AND_TIME ((uint8_t)0xCF) /* Date and time of day value */
#define AB_CIP_DATA_STRING      ((uint8_t)0xD0) /* Character string, 1 byte per character */
#define AB_CIP_DATA_BYTE        ((uint8_t)0xD1) /* 8-bit bit string */
#define AB_CIP_DATA_WORD        ((uint8_t)0xD2) /* 16-bit bit string */
#define AB_CIP_DATA_DWORD       ((uint8_t)0xD3) /* 32-bit bit string */
#define AB_CIP_DATA_LWORD       ((uint8_t)0xD4) /* 64-bit bit string */
#define AB_CIP_DATA_STRING2     ((uint8_t)0xD5) /* Wide char character string, 2 bytes per character */
#define AB_CIP_DATA_FTIME       ((uint8_t)0xD6) /* High resolution duration value */
#define AB_CIP_DATA_LTIME       ((uint8_t)0xD7) /* Medium resolution duration value */
#define AB_CIP_DATA_ITIME       ((uint8_t)0xD8) /* Low resolution duration value */
#define AB_CIP_DATA_STRINGN     ((uint8_t)0xD9) /* N-byte per char character string */
#define AB_CIP_DATA_SHORT_STRING ((uint8_t)0xDA) /* Counted character sting with 1 byte per character and 1 byte length indicator */
#define AB_CIP_DATA_TIME        ((uint8_t)0xDB) /* Duration in milliseconds */
#define AB_CIP_DATA_EPATH       ((uint8_t)0xDC) /* CIP path segment(s) */
#define AB_CIP_DATA_ENGUNIT     ((uint8_t)0xDD) /* Engineering units */
#define AB_CIP_DATA_STRINGI     ((uint8_t)0xDE) /* International character string (encoding?) */

/* aggregate data type byte values */
#define AB_CIP_DATA_ABREV_STRUCT    ((uint8_t)0xA0) /* Data is an abbreviated struct type, i.e. a CRC of the actual type descriptor */
#define AB_CIP_DATA_ABREV_ARRAY     ((uint8_t)0xA1) /* Data is an abbreviated array type. The limits are left off */
#define AB_CIP_DATA_FULL_STRUCT     ((uint8_t)0xA2) /* Data is a struct type descriptor */
#define AB_CIP_DATA_FULL_ARRAY      ((uint8_t)0xA3) /* Data is an array type descriptor */


#define MAX_CIP_NAME_SIZE (256)
#define MAX_PACKET_SIZE (512)
#define MAX_PACKET_SIZE_RECV (600)   /* it seems to be bigger than we think */
#define MAX_CIP_PACKET_SIZE (MAX_PACKET_SIZE - /*sizeof(eip_cip_co_req)*/46)


int cip_encode_name(uint8_t **tag_buf,const char *name);


//typedef struct ab_tag_t *ab_tag_p;
//#define AB_TAG_NULL ((ab_tag_p)NULL)
//
//typedef struct ab_connection_t *ab_connection_p;
//#define AB_CONNECTION_NULL ((ab_connection_p)NULL)
//
//typedef struct ab_session_t *ab_session_p;
//#define AB_SESSION_NULL ((ab_session_p)NULL)
//
//typedef struct ab_request_t *ab_request_p;
//#define AB_REQUEST_NULL ((ab_request_p)NULL)


//extern volatile ab_session_p sessions;
//extern volatile mutex_p global_session_mut;
//extern volatile thread_p io_handler_thread;


//int ab_tag_abort(ab_tag_p tag);
//int ab_tag_destroy(ab_tag_p p_tag);
//int check_cpu(ab_tag_p tag, attr attribs);
//int check_tag_name(ab_tag_p tag, const char *name);
//int check_mutex(int debug);

//
//#ifdef _WIN32
//DWORD __stdcall request_handler_func(LPVOID not_used);
//#else
//void *request_handler_func(void *not_used);
//#endif
//#ifdef _WIN32
//DWORD __stdcall request_handler_func(LPVOID not_used);
//#else
//void *request_handler_func(void *not_used);
//#endif

#endif

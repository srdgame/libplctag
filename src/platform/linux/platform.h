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
  * 2012-11-27  KRH - Created file from old platform dependent code.       *
  *                   Genericized thread creation.                         *
  *                                                                        *
  **************************************************************************/

#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <stdarg.h>

/* common definitions */
#define ZLA_SIZE    0
#define START_PACK
#define END_PACK __attribute__((__packed__))
#define ZLA_SIZE 0
#define USE_GNU_VARARG_MACROS 1



/* memory functions/defs */
extern void *mem_alloc(int size);
extern void mem_free(const void *mem);
extern void mem_set(void *d1, int c, int size);
extern void mem_copy(void *d1, void *d2, int size);

/* string functions/defs */
extern int str_cmp(const char *first, const char *second);
extern int str_cmp_i(const char *first, const char *second);
extern int str_copy(char *dst, int dst_size, const char *src);
extern int str_length(const char *str);
extern char *str_dup(const char *str);
extern int str_to_int(const char *str, int *val);
extern int str_to_float(const char *str, float *val);
extern char **str_split(const char *str, const char *sep);


/* socket functions */
typedef struct sock_t *sock_p;
extern int socket_create(sock_p *s);
extern int socket_connect_tcp(sock_p s, const char *host, int port);
extern int socket_read(sock_p s, uint8_t *buf, int size);
extern int socket_write(sock_p s, uint8_t *buf, int size);
extern int socket_close(sock_p s);
extern int socket_destroy(sock_p *s);

/* serial handling */
typedef struct serial_port_t *serial_port_p;
#define PLC_SERIAL_PORT_NULL ((plc_serial_port)NULL)
extern serial_port_p plc_lib_open_serial_port(const char *path, int baud_rate, int data_bits, int stop_bits, int parity_type);
extern int plc_lib_close_serial_port(serial_port_p serial_port);
extern int plc_lib_serial_port_read(serial_port_p serial_port, uint8_t *data, int size);
extern int plc_lib_serial_port_write(serial_port_p serial_port, uint8_t *data, int size);


/* endian functions */

extern uint16_t h2le16(uint16_t v);
extern uint16_t le2h16(uint16_t v);
extern uint16_t h2be16(uint16_t v);
extern uint16_t be2h16(uint16_t v);

extern uint32_t h2le32(uint32_t v);
extern uint32_t le2h32(uint32_t v);
extern uint32_t h2be32(uint32_t v);
extern uint32_t be2h32(uint32_t v);

/* misc functions */
extern int sleep_ms(int ms);
extern int64_t time_ms(void);

#define snprintf_platform snprintf

#endif /* _PLATFORM_H_ */

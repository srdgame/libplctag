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
 * 2013-11-19  KRH - Created file.                                        *
 **************************************************************************/

#ifndef __LIBPLCTAG_AB_CIP_H__
#define __LIBPLCTAG_AB_CIP_H__

#include <lib/libplctag.h>
#include <ab/ab_common.h>
#include <ab/defs.h>


//int cip_encode_path(ab_tag_p tag, const char *path);
extern int cip_encode_path(const char *path, int needs_connection, plc_type_t plc_type, uint8_t **conn_path, uint8_t *conn_path_size, uint16_t *dhp_dest);

//~ char *cip_decode_status(int status);
extern int cip_encode_tag_name(ab_tag_p tag,const char *name);



#endif

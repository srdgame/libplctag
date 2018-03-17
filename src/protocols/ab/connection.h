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
 * 2012-06-20  KRH - Created file.                                        *
 *                                                                        *
 **************************************************************************/


#ifndef __AB_AB_COMMON_H__
#define __AB_AB_COMMON_H__


#include <platform.h>
#include <ab/ab_common.h>
#include <util/attr.h>
#include <util/refcount.h>
#include <ab/plc.h>
#include <ab/tag.h>





extern int connection_find_or_create(ab_tag_p tag, attr attribs);
extern int connection_status(ab_connection_p connection);
extern const char *connection_path(ab_connection_p connection);
extern uint32_t connection_targ_id(ab_connection_p connection);
extern uint32_t connection_orig_id(ab_connection_p connection);
extern uint16_t connection_next_seq(ab_connection_p connection);


#endif

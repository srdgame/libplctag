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


#ifndef __LIBPLCTAG_AB_H__
#define __LIBPLCTAG_AB_H__ 1


#include <lib/libplctag.h>
#include <lib/tag.h>
#include <util/attr.h>
#include <ab/init.h>
#include <ab/teardown.h>

extern int ab_init(void);
extern void ab_teardown(void);
extern plc_tag_p ab_logix_conn_group_create(attr attribs);
extern plc_tag_p ab_logix_conn_create(attr attribs);
extern plc_tag_p ab_logix_unconn_group_create(attr attribs);
extern plc_tag_p ab_logix_unconn_create(attr attribs);
extern plc_tag_p ab_micro800_create(attr attribs);
extern plc_tag_p ab_eip_pccc_dhp_bridge_create(attr attribs);
extern plc_tag_p ab_eip_pccc_create(attr attribs);

#endif

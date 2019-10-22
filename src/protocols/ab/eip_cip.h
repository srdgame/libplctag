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


#ifndef __LIBPLCTAG_AB_EIP_CIP_H__
#define __LIBPLCTAG_AB_EIP_CIP_H__

#include <ab/ab_common.h>

//extern int eip_cip_tag_status(ab_tag_p tag);
//extern int eip_cip_tag_read_start(ab_tag_p tag);
//extern int eip_cip_tag_write_start(ab_tag_p tag);
//extern int eip_cip_tag_tickler(ab_tag_p tag);

extern struct tag_vtable_t eip_cip_vtable;

/* tag listing helpers */
extern int setup_tag_listing(ab_tag_p tag, const char *name);


#endif

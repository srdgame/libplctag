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

#ifndef __PLCTAG_LIB_GLOBALS_H__
#define __PLCTAG_LIB_GLOBALS_H__

#include <platform.h>
#include <util/attr.h>
#include <lib/tag.h>

extern const char *VERSION;

extern lock_t library_initialization_lock;
extern volatile int library_initialized;
extern mutex_p global_library_mutex;


extern tag_create_function find_tag_create_func(attr attributes);

extern plc_tag_p map_id_to_tag(int tag_id);
extern int allocate_new_tag_to_id_mapping(plc_tag_p tag);
extern int release_tag_to_id_mapping(plc_tag_p tag);


#endif

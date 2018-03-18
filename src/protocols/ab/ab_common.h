/***************************************************************************
 *   Copyright (C) 2018 by Kyle Hayes                                      *
 *   Author Kyle Hayes  kyle.hayes@gmail.com                               *
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

#pragma once

#include <lib/libplctag.h>
#include <lib/tag.h>
//#include <ab/plc.h>

typedef struct ab_tag_t *ab_tag_p;
#define AB_TAG_NULL ((ab_tag_p)NULL)

//typedef struct ab_connection_t *ab_connection_p;
//#define AB_CONNECTION_NULL ((ab_connection_p)NULL)

typedef struct ab_plc_t *ab_plc_p;
#define AB_PLC_NULL ((ab_plc_p)NULL)

typedef struct ab_request_t *ab_request_p;
#define AB_REQUEST_NULL ((ab_request_p)NULL)


typedef enum { AB_PLC_TYPE_NONE, AB_PLC_TYPE_PLC, AB_PLC_TYPE_PLC_DHP, AB_PLC_TYPE_MLGX, AB_PLC_TYPE_LGX, AB_PLC_TYPE_MLGX800 } plc_type_t;



//extern volatile ab_session_p sessions;
//extern volatile mutex_p global_session_mut;
//extern volatile thread_p io_handler_thread;


extern int tag_abort(ab_tag_p tag);
//extern int ab_tag_destroy(ab_tag_p p_tag);

extern plc_type_t check_cpu(/*ab_tag_p tag, */attr attribs);
extern int check_tag_name(ab_tag_p tag, const char *name);
extern int check_mutex(int debug);

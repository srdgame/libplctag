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

#include <platform.h>

typedef void *rc_ptr;
typedef void (*rc_cleanup_func)(rc_ptr arg);

#define rc_alloc(size, cleanup_func) rc_alloc_impl(__func__, __LINE__, size, cleanup_func)
extern rc_ptr rc_alloc_impl(const char *function_name, int line, int size, rc_cleanup_func cleanup_func);

extern rc_ptr rc_inc(rc_ptr data);

#define rc_dec(data) rc_dec_impl(__func__, __LINE__, data)
extern rc_ptr rc_dec_impl(const char *function_name, int line, rc_ptr data);

extern rc_ptr rc_weak_inc(rc_ptr data);

#define rc_weak_dec(data) rc_weak_dec_impl(__func__, __LINE__, data)
extern rc_ptr rc_weak_dec_impl(const char *function_name, int line, rc_ptr data);

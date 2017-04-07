/***************************************************************************
 *   Copyright (C) 2017 by Kyle Hayes                                      *
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
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/


#include <stdio.h>
#include "../lib/libplctag.h"
#include "utils.h"



plc_tag tags[5];

void create_tags()
{
    tags[0] = plc_tag_create("protocol=ab_eip&gateway=10.206.1.37&path=1,5&cpu=micro800&elem_size=4&elem_count=100&name=TestBigArray[0]&read_group=test_group&debug=3", 500);
    tags[1] = plc_tag_create("protocol=ab_eip&gateway=10.206.1.37&path=1,5&cpu=micro800&elem_size=4&elem_count=100&name=TestBigArray[100]&read_group=test_group&debug=3", 500);
    tags[2] = plc_tag_create("protocol=ab_eip&gateway=10.206.1.37&path=1,5&cpu=micro800&elem_size=4&elem_count=100&name=TestBigArray[200]&read_group=test_group&debug=3", 500);
    tags[3] = plc_tag_create("protocol=ab_eip&gateway=10.206.1.37&path=1,5&cpu=micro800&elem_size=4&elem_count=100&name=TestBigArray[300]&read_group=test_group&debug=3", 500);
    tags[4] = plc_tag_create("protocol=ab_eip&gateway=10.206.1.37&path=1,5&cpu=micro800&elem_size=4&elem_count=100&name=TestBigArray[400]&read_group=test_group&debug=3", 500);
}

void close_tags()
{
    plc_tag_destroy(tags[0]);
    plc_tag_destroy(tags[1]);
    plc_tag_destroy(tags[2]);
    plc_tag_destroy(tags[3]);
    plc_tag_destroy(tags[4]);
}



int main()
{
    int rc;
    create_tags();

    /* trigger a read on one tag.  Should read on all */
    rc = plc_tag_read(tags[1], 500);

    if(rc != PLCTAG_STATUS_OK) {
        fprintf(stderr,"Read failed! Status %d %s\n",rc, plc_tag_decode_error(rc));
        close_tags();
        return 0;
    }

    /* trigger a read on one and delete another. */
    rc = plc_tag_read(tags[0],0); /* cannot wait here, need to continue to be able to delete the tag in the middle */

    if(rc != PLCTAG_STATUS_OK) {
        fprintf(stderr,"Read failed! Status %d %s\n",rc, plc_tag_decode_error(rc));
        close_tags();
        return 0;
    }

    plc_tag_destroy(tags[1]);

    sleep_ms(500);

    rc = plc_tag_status(tags[0]);

    if(rc != PLCTAG_STATUS_OK) {
        fprintf(stderr,"Read failed! Status %d %s\n",rc, plc_tag_decode_error(rc));
        close_tags();
        return 0;
    }

    return 0;
}



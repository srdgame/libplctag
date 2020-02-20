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


#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <lib/libplctag.h>
#include <platform.h>
#include <ab/ab_common.h>
#include <ab/cip.h>
#include <ab/tag.h>
#include <ab/defs.h>
#include <util/debug.h>


static int match_channel(const char **p, int *dhp_channel)
{
    switch(**p) {
    case 'A':
    case 'a':
    case '2':
        *dhp_channel = 1;
        *p = *p + 1;
        return 1;
        break;
    case 'B':
    case 'b':
    case '3':
        *dhp_channel = 2;
        *p = *p + 1;
        return 1;
        break;

    default:
        return 0;
        break;
    }

    return 0;
}


static int match_colon(const char **p)
{
    if(**p == ':') {
        *p = *p + 1;
        return 1;
    }

    return 0;
}


static int match_int(const char **p, int *val)
{
    int result = 0;
    int digits = 3;

    if(! (**p >= '0' && **p <= '9')) {
        return 0;
    }

    while(**p >= '0' && **p <= '9' && digits > 0) {
        result = (result * 10) + (**p - '0');
        *p = *p + 1;
        digits--;
    }

    /* FIXME - what is the maximum DH+ ID we can have? 255? */

    *val = result;

    return 1;
}



/* match_dhp_node()
 *
 * Match a string with the format c:d:d where c is a single character and d is a number.
 */

int match_dhp_node(const char *dhp_str, int *dhp_channel, int *src_node, int *dest_node)
{
    const char *p = dhp_str;

    if(!match_channel(&p, dhp_channel)) {
        pdebug(DEBUG_INFO, "Not DH+ route.  Expected DH+ channel identifier (A/2 or B/3)");
        return 0;
    }

    if(!match_colon(&p)) {
        pdebug(DEBUG_INFO, "Not DH+ route.  Expected : in route.");
        return 0;
    }

    /* we have seen enough to commit to this being a DHP node. */

    if(!match_int(&p, src_node)) {
        pdebug(DEBUG_WARN, "Bad syntax in DH+ route.  Expected source address!");
        return 0;
    }

    if(!match_colon(&p)) {
        pdebug(DEBUG_WARN, "Bad syntax in DH+ route.  Expected colon!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(!match_int(&p, dest_node)) {
        pdebug(DEBUG_WARN, "Bad syntax in DH+ route.  Expected destination address!");
        return PLCTAG_ERR_BAD_PARAM;
    }

    pdebug(DEBUG_DETAIL, "parsed DH+ connection string %s as channel %d, source node %d and destination node %d", dhp_str, *dhp_channel, *src_node, *dest_node);

    return 1;
}

/*
 * cip_encode_path()
 *
 * This function takes a path string of comma separated components that are numbers or
 * colon-separated triples that designate a DHP connection.  It converts the path
 * into a path segment in the passed tag.
 *
 * If the protocol type is for a PLC5 series and the last hop in the path is
 * DH+, then we need to set up a different message routing path.
 *
 * Note that it is possible that the path passed is null.  That is OK for Micro850,
 * for example.  In that case, we still need to put the message routing info at
 * the end.
 *
 * FIXME - This should be factored out into a separate function.
 */

//int cip_encode_path(ab_tag_p tag, const char *path)
int cip_encode_path(const char *path, int needs_connection, plc_type_t plc_type, uint8_t **conn_path, uint8_t *conn_path_size, uint16_t *dhp_dest)
{
    int ioi_size=0;
    int last_is_dhp=0;
    int has_dhp=0;
    int dhp_channel=0;
    int src_addr=0;
    int dest_addr=0;
    int tmp=0;
    char **links=NULL;
    char *link=NULL;
    uint8_t tmp_path[MAX_CONN_PATH+16];
    uint8_t *data = &tmp_path[0];

    /* split the path */
    if(path) {
        links = str_split(path,",");
    }

    if(links != NULL) {
        int link_index=0;

        /* work along each string. */
        link = links[link_index];

        while(link && ioi_size < MAX_CONN_PATH) {   /* MAGIC -2 to allow for padding */
            int rc = match_dhp_node(link,&dhp_channel,&src_addr,&dest_addr);
            if(rc > 0) {
                /* we matched a DH+ route node */
                pdebug(DEBUG_DETAIL,"Found DH+ routing, need connection. Conn path length=%d",ioi_size);
                last_is_dhp = 1;
                has_dhp = 1;
            } else if (rc < 0) {
                /* matched part of a DH+ node, but then failed.  Syntax error. */
                pdebug(DEBUG_WARN, "Syntax error in DH+ route path.");
                if(links) mem_free(links);
                return PLCTAG_ERR_BAD_PARAM;
            } else {
                /* did not match a DH+ route node, but no error. */
                last_is_dhp = 0;
                has_dhp = 0;

                if(str_to_int(link, &tmp) != 0) {
                    /* syntax error */
                    pdebug(DEBUG_WARN, "Syntax error in path, expected number!");
                    if(links) mem_free(links);
                    return PLCTAG_ERR_BAD_PARAM;
                }

                *data = (uint8_t)tmp;

                /*printf("convert_links() link(%d)=%s (%d)\n",i,*links,tmp);*/

                data++;
                ioi_size++;
                pdebug(DEBUG_DETAIL,"Found regular routing. Conn path length=%d",ioi_size);
            }
            /* FIXME - handle case where IP address is in path */

            link_index++;
            link = links[link_index];
        }

        /* we do not need the split string anymore. */
        if(links) {
            mem_free(links);
            links = NULL;
        }
    }

    /* Add to the path based on the protocol type and
      * whether the last part is DH+.  Only some combinations of
      * DH+ and PLC type work.
      */
    if(last_is_dhp && plc_type == AB_PROTOCOL_PLC) {
        /* We have to make the difference from the more
         * generic case.
         */

        /* try adding this onto the end of the routing path */
        *data = 0x20;
        data++;
        *data = 0xA6;
        data++;
        *data = 0x24;
        data++;
        *data = (uint8_t)dhp_channel;
        data++;
        *data = 0x2C;
        data++;
        *data = 0x01;
        data++;
        ioi_size += 6;

        *dhp_dest = (uint16_t)dest_addr;
    } else if(!has_dhp) {
        if(needs_connection) {
            /*
             * we do a generic path to the router
             * object in the PLC.  But only if the PLC is
             * one that needs a connection.  For instance a
             * Micro850 needs to work in connected mode.
             */
            *data = 0x20;   /* class */
            data++;
            *data = 0x02;   /* message router class */
            data++;
            *data = 0x24;   /* instance */
            data++;
            *data = 0x01;   /* message router class instance #1 */
            ioi_size += 4;
        }

        *dhp_dest = 0;
    } else {
        /* we had the special DH+ format and it was
         * either not last or not a PLC5/SLC.  That
         * is an error.
         */

        *dhp_dest = 0;

        return PLCTAG_ERR_BAD_PARAM;
    }

    /*
     * zero out the last byte if we need to.
     * This pads out the path to a multiple of 16-bit
     * words.
     */
    pdebug(DEBUG_DETAIL,"ioi_size before %d", ioi_size);
    if(ioi_size & 0x01) {
        *data = 0;
        ioi_size++;
    }

    /* allocate space for the connection path */
    *conn_path = mem_alloc(ioi_size);
    if(! *conn_path) {
        pdebug(DEBUG_WARN, "Unable to allocate connection path!");
        return PLCTAG_ERR_NO_MEM;
    }

    mem_copy(*conn_path, &tmp_path[0], ioi_size);

    *conn_path_size = (uint8_t)ioi_size;

    pdebug(DEBUG_INFO, "Done.");

    return PLCTAG_STATUS_OK;
}





typedef enum { START, ARRAY, DOT, NAME } encode_state_t;

/*
 * cip_encode_tag_name()
 *
 * This takes a LGX-style tag name like foo[14].blah and
 * turns it into an IOI path/string.
 */

int cip_encode_tag_name(ab_tag_p tag,const char *name)
{
    uint8_t *data = tag->encoded_name;
    const char *p = name;
    uint8_t *word_count = NULL;
    uint8_t *dp = NULL;
    uint8_t *name_len;
    encode_state_t state;
    int first_num = 1;

    /* reserve room for word count for IOI string. */
    word_count = data;
    dp = data + 1;

    state = START;

    while(*p && (dp - data) < MAX_TAG_NAME) {
        switch(state) {
        case START:

            /* must start with an alpha character or _ or :. */
            if(isalpha(*p) || *p == '_' || *p == ':') {
                state = NAME;
            } else if(*p == '.') {
                state = DOT;
            } else if(*p == '[') {
                state = ARRAY;
            } else {
                return 0;
            }

            break;

        case NAME:
            *dp = 0x91; /* start of ASCII name */
            dp++;
            name_len = dp;
            *name_len = 0;
            dp++;

            while(isalnum(*p) || *p == '_' || *p == ':') {
                *dp = (uint8_t)*p;
                dp++;
                p++;
                (*name_len)++;
            }

            /* must pad the name to a multiple of two bytes */
            if(*name_len & 0x01) {
                *dp = 0;
                dp++;
            }

            state = START;

            break;

        case ARRAY:
            /* move the pointer past the [ character */
            p++;

            do {
                long int val;
                char *np = NULL;

                /* skip past a commas. */
                if(!first_num && *p == ',') {
                    p++;
                }

                /* get the numeric value. */
                val = strtol(p, &np, 10);

                if(np == p) {
                    /* we must have a number */
                    pdebug(DEBUG_WARN, "Expected number in tag name string!");
                    return 0;
                } else {
                    pdebug(DEBUG_DETAIL, "got number %ld.", val);
                }

                if(val < 0) {
                    pdebug(DEBUG_WARN, "Array index must be greater than or equal to zero!");
                    return 0;
                }

                first_num = 0;

                p = np;

                if(val > 0xFFFF) {
                    *dp = 0x2A;
                    dp++;  /* 4-byte value */
                    *dp = 0;
                    dp++;     /* padding */

                    /* copy the value in little-endian order */
                    *dp = (uint8_t)val & 0xFF;
                    dp++;
                    *dp = (uint8_t)((val >> 8) & 0xFF);
                    dp++;
                    *dp = (uint8_t)((val >> 16) & 0xFF);
                    dp++;
                    *dp = (uint8_t)((val >> 24) & 0xFF);
                    dp++;
                } else if(val > 0xFF) {
                    *dp = 0x29;
                    dp++;  /* 2-byte value */
                    *dp = 0;
                    dp++;     /* padding */

                    /* copy the value in little-endian order */
                    *dp = (uint8_t)val & 0xFF;
                    dp++;
                    *dp = (uint8_t)((val >> 8) & 0xFF);
                    dp++;
                } else {
                    *dp = 0x28;
                    dp++;  /* 1-byte value */
                    *dp = (uint8_t)val;
                    dp++;     /* value */
                }

                /* eat spaces */
                while(*p == ' ') {
                    p++;
                }
            } while(*p == ',');

            if(*p != ']') {
                return 0;
            }

            p++;

            state = START;

            break;

        case DOT:
            p++;
            state = START;
            break;

        default:
            /* this should never happen */
            return 0;

            break;
        }
    }

    if((dp - data) >= MAX_TAG_NAME) {
        pdebug(DEBUG_WARN,"Encoded tag name is too long!  Length=%d", (dp - data));
        return 0;
    }

    /* word_count is in units of 16-bit integers, do not
     * count the word_count value itself.
     */
    *word_count = (uint8_t)((dp - data)-1)/2;

    /* store the size of the whole result */
    tag->encoded_name_size = (int)(dp - data);

    return 1;
}

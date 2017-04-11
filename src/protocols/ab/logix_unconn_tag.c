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


#include <lib/libplctag.h>
#include <platform.h>
#include <lib/tag.h>
#include <util/attr.h>
#include <util/debug.h>
#include <ab/tag.h>
#include <ab/session.h>


/* forward declarations. */

static int tag_setup(job_p job, void *context);
static int encode_tag_name(uint8_t *data, int *data_size, const char *name);

/* set up the tag vtable */
static int tag_abort(plc_tag_p tag);
static int tag_destroy(plc_tag_p tag);
static int tag_read(plc_tag_p tag);
static int tag_write(plc_tag_p tag);

struct tag_vtable_t debug_vtable = { tag_abort, tag_destroy, tag_read, tag_write};



plc_tag_p ab_logix_unconn_create(attr attribs)
{
	int elem_count = 1;
	const char *name = NULL;
	session_p session = NULL;
	ab_tag_p tag = NULL;
	
	pdebug(DEBUG_INFO,"Starting.");

	/* check all the inputs we can. */
	elem_count = attr_get_int(attribs,"elem_count",1);
	name = attr_get_str(attribs,"name",NULL);
	
	if(!name || str_length(name) == 0) {
		pdebug(DEBUG_WARN,"Tag name missing or zero length!");
		return NULL;
	}
	
	/* allocate memory for the tag. */
	tag = (ab_tag_p)mem_alloc(sizeof(struct ab_tag_t));
	if(!tag) {
		pdebug(DEBUG_WARN,"Error trying to allocate memory for new tag!");
		return NULL;
	}

	/* set up the vtable */
	tag->vtable = &vtable;

	/* set up the byte order arrays.  AB is all little-endian */
	for(i=0; i < (int)sizeof(tag->int64_byte_order); i++) {
		if(i < (int)sizeof(tag->int16_byte_order)) {
			tag->int16_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->int32_byte_order)) {
			tag->int32_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->int64_byte_order)) {
			tag->int64_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->float_byte_order)) {
			tag->float_byte_order[i] = i;
		}
		if(i < (int)sizeof(tag->double_byte_order)) {
			tag->double_byte_order[i] = i;
		}
	}

    /* get the name and copy it */
    tag->name = str_dup(name);
    if(!tag->name) {
		pdebug(DEBUG_WARN,"Unable to copy tag name!");
		mem_free(tag);
		return NULL;
	}
	
	/* mark this tag as pending. */
	tag->status = PLCTAG_STATUS_PENDING;
	
	/* fire off a job to finish up the set up. */
	tag->attributes = attr_copy(attribs);
	tag->current_job = job_create("AB tag setup", tag, tag_setup, NULL);
	if(!tag->current_job) {
		pdebug(DEBUG_WARN,"Unable to create tag setup job!");
		mem_free(tag->name);
		mem_free(tag);
		return NULL;
	}
	
	pdebug(DEBUG_INFO,"Done.");
	return tag;
}





int tag_abort(plc_tag_p tag);
int tag_destroy(plc_tag_p tag);
int tag_read(plc_tag_p tag);
int tag_write(plc_tag_p tag);

int tag_setup(job_p job, void *context)
{
	int rc = PLCTAG_STATUS_OK;
	int encoded_name_size = 0;
	
	/* first try to encode the tag name */
	rc = encode_tag_name(&tag->data, &encoded_name_size, tag->name);
}



#define MAX_ENCODED_LENGTH (128)

#ifdef START
#undef START
#endif
#define START 1

#ifdef ARRAY
#undef ARRAY
#endif
#define ARRAY 2

#ifdef DOT
#undef DOT
#endif
#define DOT 3

#ifdef NAME
#undef NAME
#endif
#define NAME 4



/*
 * encode_tag_name()
 *
 * This takes a LGX-style tag name like foo[14].blah and
 * turns it into an IOI path/string.
 */

int encode_tag_name(uint8_t **tag_data, int *data_size, const char *name)
{
    const char *p = name;
    uint8_t *word_count = NULL;
    uint8_t *dp = NULL;
    uint8_t *name_len;
    int state;
    uint8_t data[MAX_ENCODED_LENGTH] = {0,};

    /* reserve room for word count for IOI string. */
    word_count = data;
    dp = data + 1;

    state = START;

    while(*p) {
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
					pdebug(DEBUG_WARN,"Illegal character for name, '%c'.",*p);
                    return PLCTAG_ERR_ENCODE;
                }

                break;

            case NAME:
                *dp = 0x91; /* start of ASCII name */
                dp++;
                name_len = dp;
                *name_len = 0;
                dp++;

                while(isalnum(*p) || *p == '_' || *p == ':') {
                    *dp = *p;
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
                    uint32_t val;
                    char *np = NULL;
                    val = (uint32_t)strtol(p,&np,0);

                    if(np == p) {
                        /* we must have a number */
                        pdebug(DEBUG_WARN,"Expected number in array section.");
                        return PLCTAG_ERR_ENCODE;
                    }

                    p = np;

                    if(val > 0xFFFF) {
                        *dp = 0x2A;
                        dp++;  /* 4-byte value */
                        *dp = 0;
                        dp++;     /* padding */

                        /* copy the value in little-endian order */
                        *dp = val & 0xFF;
                        dp++;
                        *dp = (val >> 8) & 0xFF;
                        dp++;
                        *dp = (val >> 16) & 0xFF;
                        dp++;
                        *dp = (val >> 24) & 0xFF;
                        dp++;
                    } else if(val > 0xFF) {
                        *dp = 0x29;
                        dp++;  /* 2-byte value */
                        *dp = 0;
                        dp++;     /* padding */

                        /* copy the value in little-endian order */
                        *dp = val & 0xFF;
                        dp++;
                        *dp = (val >> 8) & 0xFF;
                        dp++;
                    } else {
                        *dp = 0x28;
                        dp++;  /* 1-byte value */
                        *dp = val;
                        dp++;     /* value */
                    }

                    /* eat up whitespace */
                    while(isspace(*p)) p++;
                } while(*p == ',');

                if(*p != ']') {
					pdebug(DEBUG_WARN,"Unexpected character '%c', expected ']'.",*p);
                    return PLCTAG_ERR_ENCODE;
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

    /* word_count is in units of 16-bit integers, do not
     * count the word_count value itself.
     */
    *word_count = (uint8_t)((dp - data)-1)/2;

    /* store the size of the whole result */
    *name_size = (int)(dp - data);
    
    *tag_data = mem_alloc(1 + (dp - data));
    if(! *tag_data) {
		pdebug(DEBUG_ERROR,"Unable to allocate space for encoded tag name!");
		return PLCTAG_ERR_NO_MEM;
	}
	
	/* copy the data into the buffer */
	mem_copy(*tag_data, &data[0], 1 + (dp - data));

    return PLCTAG_STATUS_OK;
}

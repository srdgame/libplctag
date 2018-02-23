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
 * 2012-02-23  KRH - Created file.                                        *
 *                                                                        *
 * 2012-06-15  KRH - Rename file and includes for code re-org to get      *
 *                   ready for DF1 implementation.  Refactor some common  *
 *                   parts into ab/util.c.                                *
 *                                                                        *
 * 2012-06-20  KRH - change plc_err() calls for new error API.            *
 *                                                                        *
 * 2012-12-19   KRH - Start refactoring for threaded API and attributes.   *
 *                                                                        *
 **************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <platform.h>
#include <lib/libplctag.h>
#include <lib/tag.h>
//#include <ab/eip.h>
//#include <ab/common.h>
//#include <ab/cip.h>
//#include <ab/tag.h>
//#include <ab/session.h>
//#include <ab/eip_cip.h>
#include <ab/common.h>
#include <ab/error_codes.h>
#include <util/atomic.h>
#include <util/attr.h>
#include <util/bytes.h>
#include <util/debug.h>
#include <util/job.h>
#include <util/mem.h>
#include <util/refcount.h>


//int allocate_request_slot(ab_tag_p tag);
//int allocate_read_request_slot(ab_tag_p tag);
//int allocate_write_request_slot(ab_tag_p tag);
//int build_read_request_connected(ab_tag_p tag, int slot, int byte_offset);
//int build_read_request_unconnected(ab_tag_p tag, int slot, int byte_offset);
//int build_write_request_connected(ab_tag_p tag, int slot, int byte_offset);
//int build_write_request_unconnected(ab_tag_p tag, int slot, int byte_offset);
//static int check_read_status_connected(ab_tag_p tag);
//static int check_read_status_unconnected(ab_tag_p tag);
//static int check_write_status_connected(ab_tag_p tag);
//static int check_write_status_unconnected(ab_tag_p tag);
//int calculate_write_sizes(ab_tag_p tag);


MAKE_ATOMIC_TYPE(atomic_int, int)

struct req_t {
    int state;
    int offset;
    int size;
    int response_received;
    uint8_t data[MAX_PACKET_SIZE_RECV];
};

typedef struct req_t *req_p;
static void request_destroy(void *req_arg);



struct plc_t {
    const char *name;
    
    uint32_t session_id;
    uint64_t session_seq;
    uint32_t connection_id;
    uint16_t connection_seq;
    
    uint8_t data[MAX_PACKET_SIZE_RECV];
};

typedef struct plc_t *plc_p;

static int plc_get(const char *path, plc_p *plc);
static int plc_send_request(plc_p plc, req_p req);




struct logix_tag_t {
    TAG_BASE_STRUCT;
    
    const char *name;
    uint8_t *encoded_name; /* a counted bytestring */
    uint8_t *type_info;
    plc_p plc;
    atomic_int abort_op;
    atomic_int job_status;
    job_p job;
    int deleted;
    int elem_size;
    int elem_count;
    uint8_t *data;
};

typedef struct logix_tag_t *logix_tag_p;

static int tag_abort(plc_tag_p tag);
static int tag_read(plc_tag_p tag);
static int tag_write(plc_tag_p tag);
static int tag_get_int(plc_tag_p tag, int offset, int size, int64_t *val);
static int tag_set_int(plc_tag_p tag, int offset, int size, int64_t val);
static int tag_get_float(plc_tag_p tag, int offset, int size, double *val);
static int tag_set_float(plc_tag_p tag, int offset, int size, double val);
static int tag_status(plc_tag_p tag);
static int tag_size(plc_tag_p tag);


static struct tag_vtable_t logix_vtable = {tag_abort, tag_read, tag_write, tag_get_int, tag_set_int, tag_get_float, tag_set_float, tag_status, tag_size};

static void tag_destroy(void *tag_arg);





typedef enum { READ_START, READ_QUEUE, READ_WAIT, READ_STOP } read_job_state_t;
typedef enum { WRITE_START, WRITE_QUEUE, WRITE_WAIT, WRITE_STOP } write_job_state_t;



static int read_job_func(void *job_arg, void *req_arg, void *tag_arg);
static int cip_read_request_encode(req_p req, logix_tag_p tag);
static int cip_read_response_process(req_p req, logix_tag_p tag);
static int cip_response_check(uint8_t *data,  int *packet_offset, uint8_t *cip_status, uint16_t *cip_extended_status);
static int cip_read_copy_type_data(uint8_t *data, int *packet_offset, uint8_t **tag_type_info);


static int write_job_func(void *job_arg, void *req_arg, void *tag_arg);
static int cip_write_request_encode(req_p req, logix_tag_p tag);
static int cip_write_response_process(req_p req);






int logix_tag_create(attr attribs, plc_tag_p *ptag)
{
    int rc = PLCTAG_STATUS_OK;
    const char *path = attr_get_str(attribs, "path", NULL);
    const char *name = attr_get_str(attribs, "name", NULL);
    int elem_count = attr_get_int(attribs, "elem_count", 1);
    int elem_size = attr_get_int(attribs, "elem_size", 0);
    logix_tag_p tag = NULL;
    
    if(!path || str_length(path) == 0) {
        pdebug(DEBUG_WARN,"Tag path is missing or zero length!");
        *ptag = NULL;
        return PLCTAG_ERR_BAD_GATEWAY;
    }
    
    if(!name || str_length(name) == 0) {
        pdebug(DEBUG_WARN,"Tag name is missing or zero length!");
        *ptag = NULL;
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    if(elem_size <= 0) {
        pdebug(DEBUG_WARN, "Tag element size is missing or zero!");
        *ptag = NULL;
        return PLCTAG_ERR_BAD_PARAM;
    }

    /* we have enough data to make the tag now.   Allocate it all at once. */
    tag = (logix_tag_p)rc_alloc((sizeof(struct logix_tag_t) + (elem_size * elem_count)), tag_destroy);
    if(!tag) {
        pdebug(DEBUG_ERROR, "Unable to allocate tag memory!");
        *ptag = NULL;
        return PLCTAG_ERR_NO_MEM;
    }
    
    /* initialize the base tag */
    rc = plc_tag_init((plc_tag_p)tag, attribs, &logix_vtable);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Base tag initialization failed!");
        *ptag = NULL;
        rc_dec(tag);
        return rc;
    }
    
    /* point the data to the right place */
    tag->data = (uint8_t*)(tag+1);

    /* copy the name */
    tag->name = str_dup(name);
    if(!tag->name) {
        pdebug(DEBUG_WARN,"Tag name cannot be copied!");
        *ptag = NULL;
        rc_dec(tag);
        return rc;
    }
    
    /* encode the name. */
    rc = cip_encode_name(&tag->encoded_name, name);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Tag name is malformed or not able to be encoded!");
        *ptag = NULL;
        rc_dec(tag);
        return rc;
    }
    
    /* get the tag plc */
    rc = plc_get(path, &tag->plc);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to get session!");
        *ptag = NULL;
        rc_dec(tag);
        return rc;        
    }
    
    /* set up the atomic int */
    atomic_int_init(&tag->abort_op, 0);
    
    /* everything worked so far, return the tag */
    *ptag = (plc_tag_p)tag;
    
    return rc;
}




/*************************************************************************
 *************************** VTable Functions ****************************
 ************************************************************************/




int tag_abort(plc_tag_p ptag)
{
    logix_tag_p tag = (logix_tag_p)ptag;

    /* make sure that no operations are in flight */
    atomic_int_set(&tag->abort_op, 1);
    job_join(tag->job);
    atomic_int_set(&tag->abort_op, 0);

    tag->job = NULL;
    
    return PLCTAG_STATUS_OK;
}



int tag_read(plc_tag_p ptag)
{
    logix_tag_p tag = (logix_tag_p)ptag;
    int rc = tag_status(ptag);
    req_p req = NULL;
    char request_name[128] = {0,}; /* MAGIC */
    
    if(rc != PLCTAG_STATUS_OK) {
        return PLCTAG_ERR_BUSY;
    }

    if(tag->job) {
        pdebug(DEBUG_WARN,"An operation is already in progress.");
        return PLCTAG_ERR_BUSY;
    }
    
    req = (req_p)rc_alloc(sizeof(*req), request_destroy);
    if(!req) {
        pdebug(DEBUG_WARN,"Unable to allocate new request!");
        return PLCTAG_ERR_NO_MEM;
    }
    
    req->state = READ_START;
    
    atomic_int_set(&tag->job_status, PLCTAG_STATUS_PENDING);
    
    /* now make a job to handle this. */
    snprintf(request_name, sizeof(request_name), "read request for tag %s",tag->name);
    tag->job = job_create(request_name, read_job_func, rc_weak_inc(tag), req, NULL);
    if(!tag->job) {
        pdebug(DEBUG_WARN,"Unable to create new job!");

        rc_dec(req);
        
        /* we acquired a weak reference to the tag for the job, but it never got created so we
         * need to remove the weak reference to the tag.
         */
        rc_weak_dec(tag);
    }

    return PLCTAG_STATUS_PENDING;
}





int tag_write(plc_tag_p ptag)
{
    logix_tag_p tag = (logix_tag_p)ptag;
    int rc = tag_status(ptag);
    req_p req = NULL;
    char request_name[128] = {0,}; /* MAGIC */
    
    if(rc != PLCTAG_STATUS_OK) {
        return PLCTAG_ERR_BUSY;
    }

    if(tag->job) {
        pdebug(DEBUG_WARN,"An operation is already in progress.");
        return PLCTAG_ERR_BUSY;
    }
    
    req = (req_p)rc_alloc(sizeof(*req), request_destroy);
    if(!req) {
        pdebug(DEBUG_WARN,"Unable to allocate new request!");
        return PLCTAG_ERR_NO_MEM;
    }
    
    req->state = WRITE_START;
    
    atomic_int_set(&tag->job_status, PLCTAG_STATUS_PENDING);
    
    /* now make a job to handle this. */
    snprintf(request_name, sizeof(request_name), "write request for tag %s",tag->name);
    tag->job = job_create(request_name, write_job_func, rc_weak_inc(tag), req, NULL);
    if(!tag->job) {
        pdebug(DEBUG_WARN,"Unable to create new job!");
        
        rc_dec(req);
        
        /* we acquired a weak reference to the tag for the job, but it never got created so we
         * need to remove the weak reference to the tag.
         */
        rc_weak_dec(tag);
    }

    return PLCTAG_STATUS_PENDING;
}



int tag_get_int(plc_tag_p ptag, int offset, int size, int64_t *val)
{
    int rc = PLCTAG_STATUS_OK;
    logix_tag_p tag = (logix_tag_p)ptag;
    int t_size = tag_size(ptag);
    
    if(offset < 0 || size <= 0 || (size + offset) > t_size) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    switch(size) {
        case 1:
            *val = (int64_t)(int8_t)tag->data[offset];
            break;
            
        case 2: {
            int16_t tmp = 0;
            rc = decode_int16_le(&(tag->data[offset]), &tmp);
            *val = (int64_t)tmp;
        }
        break;
        
        case 4: {
            int32_t tmp = 0;
            rc = decode_int32_le(&(tag->data[offset]), &tmp);
            *val = (int64_t)tmp;
        }
        break;
        
        case 8: 
            rc = decode_int64_le(&(tag->data[offset]), val);
            break;
        
        default:
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }
    
    return rc;
}


int tag_set_int(plc_tag_p ptag, int offset, int size, int64_t val)
{
    int rc = PLCTAG_STATUS_OK;
    logix_tag_p tag = (logix_tag_p)ptag;
    int t_size = tag_size(ptag);
    
    if(offset < 0 || size <= 0 || (size + offset) > t_size) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    switch(size) {
        case 1:
            tag->data[offset] = (uint8_t)(int8_t)val;
            break;
            
        case 2:
            rc = encode_int16_le(&(tag->data[offset]), (int16_t)val);
            break;

        case 4:
            rc = encode_int32_le(&(tag->data[offset]), (int32_t)val);
            break;

        case 8:
            rc = encode_int64_le(&(tag->data[offset]), val);
            break;
            
        default:
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }
    
    return rc;            
}


int tag_get_float(plc_tag_p ptag, int offset, int size, double *val)
{
    int rc = PLCTAG_STATUS_OK;
    logix_tag_p tag = (logix_tag_p)ptag;
    int t_size = tag_size(ptag);
    
    if(offset < 0 || size <= 0 || (size + offset) > t_size) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    switch(size) {
        case 4: {
            float tmp = 0.0;
            rc = decode_float32_le(&(tag->data[offset]), &tmp);
            *val = (double)tmp;
        }
        break;
        
        case 8:
            rc = decode_float64_le(&(tag->data[offset]), val);
            break;

        default:
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }
    
    return rc;                        
}




int tag_set_float(plc_tag_p ptag, int offset, int size, double val)
{
    int rc = PLCTAG_STATUS_OK;
    logix_tag_p tag = (logix_tag_p)ptag;
    int t_size = tag_size(ptag);
    
    if(offset < 0 || size <= 0 || (size + offset) > t_size) {
        return PLCTAG_ERR_OUT_OF_BOUNDS;
    }
    
    switch(size) {
        case 4:
            rc = encode_float32_le(&(tag->data[offset]), (float)val);
            break;
            
        case 8:
            rc = encode_float64_le(&(tag->data[offset]), val);
            break;
            
        default:
            rc = PLCTAG_ERR_UNSUPPORTED;
            break;
    }
    
    return rc;                                    
}




int tag_status(plc_tag_p ptag)
{
    logix_tag_p tag = (logix_tag_p)ptag;
    int rc = PLCTAG_STATUS_OK;
    
    if(tag->job) {
        rc = atomic_int_get(&tag->job_status);
        
        if(rc != PLCTAG_STATUS_PENDING) {
            tag_abort(ptag);
        }
    }
    
    return rc;
}


int tag_size(plc_tag_p ptag)
{
    logix_tag_p tag = (logix_tag_p)ptag;

    return tag->elem_count * tag->elem_size;
}







/*************************************************************************
 *************************** Helper Functions ****************************
 ************************************************************************/

#define MAX_TAG_TYPE_INFO   (8)



void tag_destroy(void *tag_arg)
{
    logix_tag_p tag = tag_arg;
    
    /*
     * If we are here then there are no other references to this tag and
     * all operations have been aborted.   We can clean up all the data.
     */

    tag->job = rc_dec(tag->job);
     
    tag->plc = rc_dec(tag->plc);
    
    mem_free(tag->name);
    tag->name = NULL;
    
    mem_free(tag->encoded_name);
    tag->encoded_name = NULL;
    
    mem_free(tag->type_info);
    tag->type_info = NULL;
}




int read_job_func(void *tag_arg, void *req_arg, void *unused_arg)
{
    logix_tag_p tag = (logix_tag_p)rc_inc(tag_arg);
    req_p req = (req_p)req_arg;
    int rc = PLCTAG_STATUS_OK;
    int abort = atomic_int_get(&tag->abort_op);
    
    (void)unused_arg;
    
    
    /* only set when the job is being destroyed, time to clean up. */
    if(req->state == READ_STOP || !tag || abort) {
        if(req->state == READ_STOP) {
            pdebug(DEBUG_INFO, "Read succeeded, job exiting.");
            atomic_int_set(&tag->job_status,PLCTAG_STATUS_OK);
        } else if(!tag) {
            pdebug(DEBUG_INFO,"Tag is being destroyed unexpectedly!");
        } else {
            pdebug(DEBUG_WARN,"Operation being aborted!");
            atomic_int_set(&tag->job_status,PLCTAG_ERR_ABORT);
        }
        
        rc_dec(req_arg);
        rc_weak_dec(tag_arg);
        
        return JOB_STOP;
    }
    
    
    /* what are we doing? */
    switch(req->state) {
        case READ_START:
            pdebug(DEBUG_INFO, "Starting to build packet.");
            
            /* clear request for use. */
            req->response_received = 0;
            mem_set(&req->data[0], 0, sizeof(req->data));
            
            /* build the read packet. We start with non-fragmented. */
            rc = cip_read_request_encode(req, tag);
            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Error while trying to build CIP read packet!");
                atomic_int_set(&tag->job_status, rc);
                req->state = READ_STOP;
            } else {
                pdebug(DEBUG_INFO,"Packet built, now trying to queue packet.");
                req->state = READ_QUEUE;
            }
            break;
            
        case READ_QUEUE:
            /* send the request to the PLC session */
            
            /* FIXME
             * 
             * Need to fix the handling of the request.  It is now created with mem_alloc but
             * needs to be done with rc_alloc because the plc job will maintain a reference to
             * it so that it can be held and any operation in progress completed before the 
             * memory goes away.
             */
            rc = plc_send_request(tag->plc, req);
            if(rc == PLCTAG_STATUS_OK) {
                pdebug(DEBUG_INFO,"Request queued.  Waiting.");
                /* the request got queued immediately. Wait for response. */
                req->state = READ_WAIT;
            } else if(rc == PLCTAG_STATUS_PENDING) {
                pdebug(DEBUG_SPEW, "Still waiting to queue request.");
            } else {
                pdebug(DEBUG_WARN,"Error while trying to queue request!");
                atomic_int_set(&tag->job_status, rc);
                req->state = READ_STOP;
            }
            break;
            
        case READ_WAIT:
            pdebug(DEBUG_SPEW,"Waiting for read to complete.");
            if(req->response_received) {
                pdebug(DEBUG_INFO,"Got response packet!");
                rc = cip_read_response_process(req, tag);
                
                /* communicate the status. */
                atomic_int_set(&tag->job_status, rc);
                
                if(rc == PLCTAG_STATUS_PENDING) {
                    /* need to keep going. */
                    req->state = READ_START;
                } else {
                    /* we are done either because we failed or succeeded. */
                    req->state = READ_STOP;
                }
            }
            break;
            
        case READ_STOP:
            /* wait here until we are terminated. */
            pdebug(DEBUG_SPEW, "We are done with the read.");
            break;
            
        default:
            pdebug(DEBUG_SPEW,"Unknown state!");
            break;
    }
    
    /* done with the tag for now. */
    rc_dec(tag);
    
    return JOB_RUN;
}






//int read_job_cleanup_func(void *job_arg, void *req_arg, void *tag_arg);


int cip_read_request_encode(req_p req, logix_tag_p tag)
{
    int packet_offset = 0;
    int encoded_length = 0;
    
    /* set up the CIP Read request */
    req->data[packet_offset] = (req->offset ? AB_EIP_CMD_CIP_READ_FRAG  : AB_EIP_CMD_CIP_READ);
    packet_offset++;
    
    /* copy the encoded tag name. */
    encoded_length = tag->encoded_name[0] * 2; /* value in 16-bit words */
    mem_copy(&req->data[packet_offset], &tag->encoded_name[0], encoded_length);
    packet_offset += encoded_length;

    /* add the count of elements to read. */
    encode_int16_le(&req->data[packet_offset], (int16_t)tag->elem_count);
    packet_offset += sizeof(uint16_t);

    if(req->offset) {
        /* add the byte offset for this request */
        encode_int32_le(&req->data[packet_offset], (int32_t)0);
        packet_offset += sizeof(uint32_t);
    }
    
    req->size = packet_offset;
    
    return PLCTAG_STATUS_OK;
}



int cip_read_response_process(req_p req, logix_tag_p tag)
{
    int rc = PLCTAG_STATUS_OK;
    int packet_offset = 0;
    uint8_t cip_status = 0;
    uint16_t cip_extended_status = 0;
    
    rc = cip_response_check(req->data, &packet_offset, &cip_status, &cip_extended_status);
    if(rc == PLCTAG_STATUS_OK || rc == PLCTAG_STATUS_PENDING) {
        /* copy the type data. */
        rc = cip_read_copy_type_data(req->data, &packet_offset, &tag->type_info);
        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Error decoding/copying tag type data!");
            return rc;
        } else {
            /* copy the data that is left in the packet */
            int data_len = req->size - packet_offset;
            
            if(data_len + req->offset > tag_size((plc_tag_p)tag)) {
                pdebug(DEBUG_WARN,"Too much data");
                return PLCTAG_ERR_TOO_LONG;
            } else {
                mem_copy(&tag->data[req->offset], &req->data[packet_offset], data_len);
                req->offset += data_len;
                
                if(req->offset < tag_size((plc_tag_p)tag)) {
                    /* there is more left. */
                    return PLCTAG_STATUS_PENDING;
                } else {
                    /* we are done. */
                    return PLCTAG_STATUS_OK;
                }
            }
        }
    } else {
        /* read failed! */
        pdebug(DEBUG_WARN,"Error reading tag!");
        return PLCTAG_ERR_READ;
    }
}


int cip_response_check(uint8_t *data,  int *packet_offset, uint8_t *cip_status, uint16_t *cip_extended_status)
{
    int rc = PLCTAG_STATUS_OK;
    
//    The CIP response header looks like this:
//    
//    uint8_t reply_service;          /* 0xCC CIP READ Reply */
//    uint8_t reserved;               /* 0x00 in reply */
//    uint8_t status;                 /* 0x00 for success */
//    uint8_t num_status_words;       /* number of 16-bit words in status */

    *cip_status = 0;
    *cip_extended_status = 0;

    /* 0 or 6 are good.  The latter means we got an incomplete result and need to request more. */
    if(data[2] == AB_CIP_STATUS_OK || data[2] == AB_CIP_STATUS_FRAG) {
        /* all good. */
        if(data[2] == AB_CIP_STATUS_OK) {
            rc = PLCTAG_STATUS_OK;
        } else {
            rc = PLCTAG_STATUS_PENDING;
        }
        
        *packet_offset += 4;  /* MAGIC */

       return rc;
    } else {
        *cip_status = data[2];
        
        if(data[3] > 0) {
            *cip_extended_status = data[4] | (data[5] << 8);
        }
        
        pdebug(DEBUG_WARN,"Read response failed with error %s -- %s", decode_cip_error_str(*cip_status, *cip_extended_status, 1), decode_cip_error_str(*cip_status, *cip_extended_status, 0));
        
        rc = decode_cip_error_status(*cip_status, *cip_extended_status);
    }
    
    return rc;
}


int cip_read_copy_type_data(uint8_t *data, int *packet_offset, uint8_t **tag_type_info)
{
    uint8_t type_info[MAX_TAG_TYPE_INFO] = {0,};
    int type_length = 0;
    
    /* short circuit this if we already copied the type data */
    if(tag_type_info && *tag_type_info) {
        /* first byte is the length */
        type_length = (*tag_type_info)[0];
        *packet_offset += type_length;
        return PLCTAG_STATUS_OK;
    }
    
    /*
     * AB has a relatively complicated scheme for data typing.  The type is
     * required when writing.  Most of the types are basic types and occupy
     * a known amount of space.  Aggregate types like structs and arrays
     * occupy a variable amount of space.  In addition, structs and arrays
     * can be in two forms: full and abbreviated.  Full form for structs includes
     * all data types (in full) for fields of the struct.  Abbreviated form for
     * structs includes a two byte CRC calculated across the full form.  For arrays,
     * full form includes index limits and base data type.  Abbreviated arrays
     * drop the limits and encode any structs as abbreviate structs.  At least
     * we think this is what is happening.
     *
     * Luckily, we do not actually care what these bytes mean, we just need
     * to copy them and skip past them for the actual data.
     */

    /* check for a simple/base type */
    if ((*data) >= AB_CIP_DATA_BIT && (*data) <= AB_CIP_DATA_STRINGI) {
        type_length = 2;
    } else if ((*data) == AB_CIP_DATA_ABREV_STRUCT || (*data) == AB_CIP_DATA_ABREV_ARRAY ||
               (*data) == AB_CIP_DATA_FULL_STRUCT || (*data) == AB_CIP_DATA_FULL_ARRAY) {
        /* this is an aggregate type of some sort, the type info is variable length */
        type_length = *(data + 1) + 2; /*
                                            * MAGIC
                                            * add 2 to get the total length including
                                            * the type byte and the length byte.
                                            */

        /* check for extra long types */
        if (type_length >= MAX_TAG_TYPE_INFO) {
            pdebug(DEBUG_WARN, "Read data type info is too long (%d)!", type_length);
            return PLCTAG_ERR_TOO_LONG;
        }
    } else {
        pdebug(DEBUG_WARN, "Unsupported data type returned, type byte=%d", *data);
        return PLCTAG_ERR_UNSUPPORTED;
    }
    
    /* try to allocate space */
    *tag_type_info = mem_alloc(type_length + 1);
    if(! *tag_type_info) {
        pdebug(DEBUG_WARN,"Unable to allocate memory for tag type info!");
        return PLCTAG_ERR_NO_MEM;
    }

    (*tag_type_info)[0] = (uint8_t)type_length;
    mem_copy(&((*tag_type_info)[1]),data, type_info[0]);
    *packet_offset += type_length;
    
    return PLCTAG_STATUS_OK;
}





int write_job_func(void *tag_arg, void *req_arg, void *unused_arg)
{
    logix_tag_p tag = (logix_tag_p)rc_inc(tag_arg);
    req_p req = (req_p)req_arg;
    int rc = PLCTAG_STATUS_OK;
    int abort = atomic_int_get(&tag->abort_op);
    
    (void)unused_arg;
    
    
    /* only set when the job is being destroyed, time to clean up. */
    if(req->state == WRITE_STOP || !tag || abort) {
        if(req->state == WRITE_STOP) {
            pdebug(DEBUG_INFO, "Write succeeded, job exiting.");
            atomic_int_set(&tag->job_status,PLCTAG_STATUS_OK);
        } else if(!tag) {
            pdebug(DEBUG_INFO,"Tag is being destroyed unexpectedly!");
        } else {
            pdebug(DEBUG_WARN,"Operation being aborted!");
            atomic_int_set(&tag->job_status,PLCTAG_ERR_ABORT);
        }

        rc_dec(req_arg);
        rc_weak_dec(tag_arg);
        
        return JOB_STOP;
    }
    
    
    /* what are we doing? */
    switch(req->state) {
        case WRITE_START:
            pdebug(DEBUG_INFO, "Starting to build packet.");
            
            /* clear request for use. */
            req->response_received = 0;
            mem_set(&req->data[0], 0, sizeof(req->data));
            
            /* build the write packet. We start with non-fragmented. */
            rc = cip_write_request_encode(req, tag);
            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Error while trying to build CIP write packet!");
                atomic_int_set(&tag->job_status, rc);
                req->state = WRITE_STOP;
            } else {
                pdebug(DEBUG_INFO,"Packet built, now trying to queue packet.");
                req->state = WRITE_QUEUE;
            }
            break;
            
        case WRITE_QUEUE:
            /* send the request to the PLC session */            
            rc = plc_send_request(tag->plc, req);
            if(rc == PLCTAG_STATUS_OK) {
                pdebug(DEBUG_INFO,"Request queued.  Waiting.");
                /* the request got queued immediately. Wait for response. */
                req->state = WRITE_WAIT;
            } else if(rc == PLCTAG_STATUS_PENDING) {
                pdebug(DEBUG_SPEW, "Still waiting to queue request.");
            } else {
                pdebug(DEBUG_WARN,"Error while trying to queue request!");
                atomic_int_set(&tag->job_status, rc);
                req->state = WRITE_STOP;
            }
            break;
            
        case WRITE_WAIT:
            pdebug(DEBUG_SPEW,"Waiting for write to complete.");
            if(req->response_received) {
                pdebug(DEBUG_INFO,"Got response packet!");
                rc = cip_write_response_process(req);
                
                /* communicate the status. */
                atomic_int_set(&tag->job_status, rc);
                
                if(rc == PLCTAG_STATUS_PENDING) {
                    /* need to keep going. */
                    req->state = WRITE_START;
                } else {
                    /* we are done either because we failed or succeeded. */
                    req->state = WRITE_STOP;
                }
            }
            break;
            
        case WRITE_STOP:
            /* wait here until we are terminated. */
            pdebug(DEBUG_SPEW, "We are done with the read.");
            break;
            
        default:
            pdebug(DEBUG_SPEW,"Unknown state!");
            req->state = WRITE_STOP;
            break;
    }
    
    /* done with the tag for now. */
    rc_dec(tag);
    
    return JOB_RUN;
}




int cip_write_request_encode(req_p req, logix_tag_p tag)
{
    int packet_offset = 0;
    int encoded_length = 0;
    int command_index = 0;
    int type_len = 0;
    int remaining_space = 0;
    int remaining_data = 0;
    int write_size = 0;
    
    /* set up the CIP Write request */
    req->data[packet_offset] = AB_EIP_CMD_CIP_WRITE;
    command_index = packet_offset;
    packet_offset++;
    
    /* copy the encoded tag name. */
    encoded_length = tag->encoded_name[0] * 2; /* value in 16-bit words */
    mem_copy(&req->data[packet_offset], &tag->encoded_name[0], encoded_length);
    packet_offset += encoded_length;

    /* copy encoded type info */
    type_len = tag->type_info[0];    
    mem_copy(&req->data[packet_offset], &tag->type_info[1], type_len);
    packet_offset += type_len;
    
    /* add the count of elements to read. */
    encode_int16_le(&req->data[packet_offset], (int16_t)tag->elem_count);
    packet_offset += sizeof(uint16_t);

    /* do we need to fragment? */
    remaining_space = MAX_CIP_PACKET_SIZE - packet_offset;    
    if(remaining_space <= 0) {
        pdebug(DEBUG_WARN,"No space left in packet after CIP request header!");
        return PLCTAG_ERR_TOO_LONG;
    }
    
    if(req->offset || remaining_space < tag_size((plc_tag_p)tag)) {
        /* need to fragment. */
        pdebug(DEBUG_DETAIL,"Fragmenting write packet.");
        
        req->data[command_index] = AB_EIP_CMD_CIP_WRITE_FRAG;
        
        /* add the byte offset for this request */
        encode_int32_le(&req->data[packet_offset], (int32_t)0);
        packet_offset += sizeof(uint32_t);        
    }
    
    /* how much data do we copy? Round down to nearest multiple of 8 bytes. */
    remaining_space = (MAX_CIP_PACKET_SIZE - packet_offset) & 0xFF8;
    remaining_data = tag_size((plc_tag_p)tag) - req->offset;
    
    if(remaining_space < remaining_data) {
        /* less space than we need. */
        write_size = remaining_space;
    } else {
        /* almost done */
        write_size = remaining_data;
    }
    
    mem_copy(&req->data[packet_offset], &tag->data[req->offset], write_size);
    
    /* bump the offset in the request. */
    req->offset += write_size;
    
    /* make sure the request size is correct. */
    req->size = packet_offset + write_size;
    
    return PLCTAG_STATUS_OK;
}



int cip_write_response_process(req_p req)
{
    int packet_offset = 0;
    uint8_t cip_status = 0;
    uint16_t cip_extended_status = 0;
    
    return cip_response_check(req->data, &packet_offset, &cip_status, &cip_extended_status);
}









void request_destroy(void *req_arg)
{
    (void)req_arg;
    
    pdebug(DEBUG_INFO,"Starting.");
    /* nothing to do. */
    
    pdebug(DEBUG_INFO,"Done.");
}





int plc_get(const char *path, plc_p *plc)
{
    (void)path;
    (void)plc;
    
    return PLCTAG_STATUS_OK;
}


int plc_send_request(plc_p plc, req_p req)
{
        (void)plc;
        (void)req;
        
        return PLCTAG_STATUS_OK;
}


/*************************************************************************
 **************************** API Functions ******************************
 ************************************************************************/


//
//
//
///*
// * eip_cip_tag_status
// *
// * CIP-specific status.  This functions as a "tickler" routine
// * to check on the completion of async requests.
// */
//int eip_cip_tag_status(ab_tag_p tag)
//{
//    int rc = PLCTAG_STATUS_OK;
//    int session_rc = PLCTAG_STATUS_OK;
//    int connection_rc = PLCTAG_STATUS_OK;
//
//    if (tag->read_in_progress) {
//        if(tag->connection) {
//            rc = check_read_status_connected(tag);
//        } else {
//            rc = check_read_status_unconnected(tag);
//        }
//
//        return rc;
//    }
//
//    if (tag->write_in_progress) {
//        if(tag->connection) {
//            rc = check_write_status_connected(tag);
//        } else {
//            rc = check_write_status_unconnected(tag);
//        }
//
//        return rc;
//    }
//
//    /* We need to treat the session and connection statuses
//     * as async because we might not be the thread creating those
//     * objects.  In that case, we propagate the status back up
//     * to the tag.
//     */
//
//    /*
//     * session  connection  tag     result
//     *   OK         OK       OK       OK
//     *   OK         OK       N        N
//     *   OK        PEND      OK      PEND
//     *   OK        PEND      N        N
//     *   OK         N       OK        N
//     *   OK         N1      N2        N1
//     *  PEND        OK       OK      PEND
//     *  PEND        OK       N        N
//     *  PEND        PEND     OK      PEND
//     *  PEND        PEND     N        N
//     *  PEND        N        OK       N
//     *  PEND        N1       N2       N1
//     *   N          OK      OK        N
//     *   N1         OK      N2        N1
//     *   N          PEND    OK        N
//     *   N1         PEND    N2        N1
//     *   N1         N2      OK        N1
//     *   N1         N2      N3        N1
//     */
//    if (tag->session) {
//        session_rc = tag->session->status;
//    } else {
//        /* this is not OK.  This is fatal! */
//        session_rc = PLCTAG_ERR_CREATE;
//    }
//
//    if(tag->needs_connection) {
//        if(tag->connection) {
//            connection_rc = tag->connection->status;
//        } else {
//            /* fatal! */
//            connection_rc = PLCTAG_ERR_CREATE;
//        }
//    } else {
//        connection_rc = PLCTAG_STATUS_OK;
//    }
//
//    /* now collect the status.  Highest level wins. */
//    rc = session_rc;
//
//    if(rc == PLCTAG_STATUS_OK) {
//        rc = connection_rc;
//    }
//
//    if(rc == PLCTAG_STATUS_OK) {
//        rc = tag->status;
//    }
//
//    return rc;
//}
//
///*
// * eip_cip_tag_read_start
// *
// * This function must be called only from within one thread, or while
// * the tag's mutex is locked.
// *
// * The function starts the process of getting tag data from the PLC.
// */
//
//int eip_cip_tag_read_start(ab_tag_p tag)
//{
//    int rc = PLCTAG_STATUS_OK;
//    int i;
//    int byte_offset = 0;
//
//    pdebug(DEBUG_INFO, "Starting");
//
//    /* is this the first read? */
//    if (tag->first_read) {
//        /*
//         * On a new tag, the first time we read, we go through and
//         * request the maximum possible (up to the size of the tag)
//         * each time.  We record what we actually get back in the
//         * tag->read_req_sizes array.  The next time we read, we
//         * use that array to make the new requests.
//         */
//
//        rc = allocate_read_request_slot(tag);
//
//        if (rc != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to allocate read request slot!");
//            return rc;
//        }
//
//        /*
//         * The PLC may not send back as much data as we would like.
//         * So, we attempt to determine what the size will be by
//         * single-stepping through the requests the first time.
//         * This will be slow, but subsequent reads will be pipelined.
//         */
//
//        /* determine the byte offset this time. */
//        byte_offset = 0;
//
//        /* scan and add the byte offsets */
//        for (i = 0; i < tag->num_read_requests && tag->reqs[i]; i++) {
//            byte_offset += tag->read_req_sizes[i];
//        }
//
//        pdebug(DEBUG_DETAIL, "First read tag->num_read_requests=%d, byte_offset=%d.", tag->num_read_requests, byte_offset);
//
//        /* i is the index of the first new request */
//        if(tag->connection) {
//            rc = build_read_request_connected(tag, i, byte_offset);
//        } else {
//            rc = build_read_request_unconnected(tag, i, byte_offset);
//        }
//
//        if (rc != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to build read request!");
//            return rc;
//        }
//
//    } else {
//        /* this is not the first read, so just set up all the requests at once. */
//        byte_offset = 0;
//
//        for (i = 0; i < tag->num_read_requests; i++) {
//            if(tag->connection) {
//                rc = build_read_request_connected(tag, i, byte_offset);
//            } else {
//                rc = build_read_request_unconnected(tag, i, byte_offset);
//            }
//
//            if (rc != PLCTAG_STATUS_OK) {
//                pdebug(DEBUG_WARN,"Unable to build read request!");
//                return rc;
//            }
//
//            byte_offset += tag->read_req_sizes[i];
//        }
//    }
//
//    /* mark the tag read in progress */
//    tag->read_in_progress = 1;
//
//    pdebug(DEBUG_INFO, "Done.");
//
//    return PLCTAG_STATUS_PENDING;
//}
//
///*
// * eip_cip_tag_write_start
// *
// * This must be called from one thread alone, or while the tag mutex is
// * locked.
// *
// * The routine starts the process of writing to a tag.
// */
//
//int eip_cip_tag_write_start(ab_tag_p tag)
//{
//    int rc = PLCTAG_STATUS_OK;
//    int i;
//    int byte_offset = 0;
//
//    pdebug(DEBUG_INFO, "Starting");
//
//    /*
//     * if the tag has not been read yet, read it.
//     *
//     * This gets the type data and sets up the request
//     * buffers.
//     */
//
//    if (tag->first_read) {
//        pdebug(DEBUG_DETAIL, "No read has completed yet, doing pre-read to get type information.");
//
//        tag->pre_write_read = 1;
//
//        return eip_cip_tag_read_start(tag);
//    }
//
//    /*
//     * calculate the number and size of the write requests
//     * if we have not already done so.
//     */
//    if (!tag->num_write_requests) {
//        rc = calculate_write_sizes(tag);
//    }
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_WARN,"Unable to calculate write sizes!");
//        return rc;
//    }
//
//    /* set up all the requests at once. */
//    byte_offset = 0;
//
//    for (i = 0; i < tag->num_write_requests; i++) {
//        if(tag->connection) {
//            rc = build_write_request_connected(tag, i, byte_offset);
//        } else {
//            rc = build_write_request_unconnected(tag, i, byte_offset);
//        }
//
//        if (rc != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to build write request!");
//            return rc;
//        }
//
//        byte_offset += tag->write_req_sizes[i];
//    }
//
//    /* the write is now pending */
//    tag->write_in_progress = 1;
//
//    pdebug(DEBUG_INFO, "Done.");
//
//    return PLCTAG_STATUS_PENDING;
//}
//
///*
// * allocate_request_slot
// *
// * Increase the number of available request slots.
// */
//int allocate_request_slot(ab_tag_p tag)
//{
//    int* old_sizes;
//    ab_request_p* old_reqs;
//    int i;
//    int old_max = tag->max_requests;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    /* bump up the number of allowed requests */
//    tag->max_requests += DEFAULT_MAX_REQUESTS;
//
//    pdebug(DEBUG_DETAIL, "setting max_requests = %d", tag->max_requests);
//
//    /* (re)allocate the read size array */
//    old_sizes = tag->read_req_sizes;
//    tag->read_req_sizes = (int*)mem_alloc(tag->max_requests * sizeof(int));
//
//    if (!tag->read_req_sizes) {
//        mem_free(old_sizes);
//        pdebug(DEBUG_WARN,"Unable to allocate read sizes array!");
//        return PLCTAG_ERR_NO_MEM;
//    }
//
//    /* copy the size data */
//    if (old_sizes) {
//        for (i = 0; i < old_max; i++) {
//            tag->read_req_sizes[i] = old_sizes[i];
//        }
//
//        mem_free(old_sizes);
//    }
//
//    /* (re)allocate the write size array */
//    old_sizes = tag->write_req_sizes;
//    tag->write_req_sizes = (int*)mem_alloc(tag->max_requests * sizeof(int));
//
//    if (!tag->write_req_sizes) {
//        mem_free(old_sizes);
//        pdebug(DEBUG_WARN,"Unable to allocate write sizes array!");
//        return PLCTAG_ERR_NO_MEM;
//    }
//
//    /* copy the size data */
//    if (old_sizes) {
//        for (i = 0; i < old_max; i++) {
//            tag->write_req_sizes[i] = old_sizes[i];
//        }
//
//        mem_free(old_sizes);
//    }
//
//    /* do the same for the request array */
//    old_reqs = tag->reqs;
//    tag->reqs = (ab_request_p*)mem_alloc(tag->max_requests * sizeof(ab_request_p));
//
//    if (!tag->reqs) {
//        pdebug(DEBUG_WARN,"Unable to allocate requests array!");
//        return PLCTAG_ERR_NO_MEM;
//    }
//
//    /* copy the request data, there shouldn't be anything here I think... */
//    if (old_reqs) {
//        for (i = 0; i < old_max; i++) {
//            tag->reqs[i] = old_reqs[i];
//        }
//
//        mem_free(old_reqs);
//    }
//
//    pdebug(DEBUG_DETAIL, "Done.");
//
//    return PLCTAG_STATUS_OK;
//}
//
//int allocate_read_request_slot(ab_tag_p tag)
//{
//    /* increase the number of available request slots */
//    tag->num_read_requests++;
//
//    if (tag->num_read_requests > tag->max_requests) {
//        return allocate_request_slot(tag);
//    }
//
//    return PLCTAG_STATUS_OK;
//}
//
//int allocate_write_request_slot(ab_tag_p tag)
//{
//    /* increase the number of available request slots */
//    tag->num_write_requests++;
//
//    if (tag->num_write_requests > tag->max_requests) {
//        return allocate_request_slot(tag);
//    }
//
//    return PLCTAG_STATUS_OK;
//}
//
//int build_read_request_connected(ab_tag_p tag, int slot, int byte_offset)
//{
//    eip_cip_co_req* cip = NULL;
//    uint8_t* data = NULL;
//    ab_request_p req = NULL;
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    /* get a request buffer */
//    rc = request_create(&req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to get new request.  rc=%d", rc);
//        return rc;
//    }
//
//    req->num_retries_left = tag->num_retries;
//    req->retry_interval = tag->default_retry_interval;
//
//    /* point the request struct at the buffer */
//    cip = (eip_cip_co_req*)(req->data);
//
//    /* point to the end of the struct */
//    data = (req->data) + sizeof(eip_cip_co_req);
//
//    /*
//     * set up the embedded CIP read packet
//     * The format is:
//     *
//     * uint8_t cmd
//     * LLA formatted name
//     * uint16_t # of elements to read
//     */
//
//    //embed_start = data;
//
//    /* set up the CIP Read request */
//    *data = AB_EIP_CMD_CIP_READ_FRAG;
//    data++;
//
//    /* copy the tag name into the request */
//    mem_copy(data, tag->encoded_name, tag->encoded_name_size);
//    data += tag->encoded_name_size;
//
//    /* add the count of elements to read. */
//    *((uint16_t*)data) = h2le16(tag->elem_count);
//    data += sizeof(uint16_t);
//
//    /* add the byte offset for this request */
//    *((uint32_t*)data) = h2le32(byte_offset);
//    data += sizeof(uint32_t);
//
//    /* now we go back and fill in the fields of the static part */
//
//    /* encap fields */
//    cip->encap_command = h2le16(AB_EIP_CONNECTED_SEND); /* ALWAYS 0x0070 Unconnected Send*/
//
//    /* router timeout */
//    cip->router_timeout = h2le16(1); /* one second timeout, enough? */
//
//    /* Common Packet Format fields for unconnected send. */
//    cip->cpf_item_count = h2le16(2);                 /* ALWAYS 2 */
//    cip->cpf_cai_item_type = h2le16(AB_EIP_ITEM_CAI);/* ALWAYS 0x00A1 connected address item */
//    cip->cpf_cai_item_length = h2le16(4);            /* ALWAYS 4, size of connection ID*/
//    cip->cpf_cdi_item_type = h2le16(AB_EIP_ITEM_CDI);/* ALWAYS 0x00B1 - connected Data Item */
//    cip->cpf_cdi_item_length = h2le16(data - (uint8_t*)(&cip->cpf_conn_seq_num)); /* REQ: fill in with length of remaining data. */
//
//    /* set the size of the request */
//    req->request_size = data - (req->data);
//
//    /* store the connection */
//    req->connection = tag->connection;
//
//    /* mark it as ready to send */
//    req->send_request = 1;
//
//    /* this request is connected, so it needs the session exclusively */
//    req->connected_request = 1;
//
//    /* add the request to the session's list. */
//    rc = session_add_request(tag->session, req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to add request to session! rc=%d", rc);
//        tag->reqs[slot] = rc_dec(req);
//        return rc;
//    }
//
//    /* save the request for later */
//    tag->reqs[slot] = req;
//
//    pdebug(DEBUG_INFO, "Done");
//
//    return PLCTAG_STATUS_OK;
//}
//
//
//
//int build_read_request_unconnected(ab_tag_p tag, int slot, int byte_offset)
//{
//    eip_cip_uc_req* cip;
//    uint8_t* data;
//    uint8_t* embed_start, *embed_end;
//    ab_request_p req = NULL;
//    int rc;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    /* get a request buffer */
//    rc = request_create(&req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to get new request.  rc=%d", rc);
//        return rc;
//    }
//
//    req->num_retries_left = tag->num_retries;
//    req->retry_interval = tag->default_retry_interval;
//
//    /* point the request struct at the buffer */
//    cip = (eip_cip_uc_req*)(req->data);
//
//    /* point to the end of the struct */
//    data = (req->data) + sizeof(eip_cip_uc_req);
//
//    /*
//     * set up the embedded CIP read packet
//     * The format is:
//     *
//     * uint8_t cmd
//     * LLA formatted name
//     * uint16_t # of elements to read
//     */
//
//    embed_start = data;
//
//    /* set up the CIP Read request */
//    *data = AB_EIP_CMD_CIP_READ_FRAG;
//    data++;
//
//    /* copy the tag name into the request */
//    mem_copy(data, tag->encoded_name, tag->encoded_name_size);
//    data += tag->encoded_name_size;
//
//    /* add the count of elements to read. */
//    *((uint16_t*)data) = h2le16(tag->elem_count);
//    data += sizeof(uint16_t);
//
//    /* add the byte offset for this request */
//    *((uint32_t*)data) = h2le32(byte_offset);
//    data += sizeof(uint32_t);
//
//    /* mark the end of the embedded packet */
//    embed_end = data;
//
//    /* Now copy in the routing information for the embedded message */
//    /*
//     * routing information.  Format:
//     *
//     * uint8_t path_size in 16-bit words
//     * uint8_t reserved/pad (zero)
//     * uint8_t[...] path (padded to even number of bytes)
//     */
//    if(tag->conn_path_size > 0) {
//        *data = (tag->conn_path_size) / 2; /* in 16-bit words */
//        data++;
//        *data = 0; /* reserved/pad */
//        data++;
//        mem_copy(data, tag->conn_path, tag->conn_path_size);
//        data += tag->conn_path_size;
//    }
//
//    /* now we go back and fill in the fields of the static part */
//
//    /* encap fields */
//    cip->encap_command = h2le16(AB_EIP_READ_RR_DATA); /* ALWAYS 0x0070 Unconnected Send*/
//
//    /* router timeout */
//    cip->router_timeout = h2le16(1); /* one second timeout, enough? */
//
//    /* Common Packet Format fields for unconnected send. */
//    cip->cpf_item_count = h2le16(2);                  /* ALWAYS 2 */
//    cip->cpf_nai_item_type = h2le16(AB_EIP_ITEM_NAI); /* ALWAYS 0 */
//    cip->cpf_nai_item_length = h2le16(0);             /* ALWAYS 0 */
//    cip->cpf_udi_item_type = h2le16(AB_EIP_ITEM_UDI); /* ALWAYS 0x00B2 - Unconnected Data Item */
//    cip->cpf_udi_item_length = h2le16(data - (uint8_t*)(&cip->cm_service_code)); /* REQ: fill in with length of remaining data. */
//
//    /* CM Service Request - Connection Manager */
//    cip->cm_service_code = AB_EIP_CMD_UNCONNECTED_SEND; /* 0x52 Unconnected Send */
//    cip->cm_req_path_size = 2;                          /* 2, size in 16-bit words of path, next field */
//    cip->cm_req_path[0] = 0x20;                         /* class */
//    cip->cm_req_path[1] = 0x06;                         /* Connection Manager */
//    cip->cm_req_path[2] = 0x24;                         /* instance */
//    cip->cm_req_path[3] = 0x01;                         /* instance 1 */
//
//    /* Unconnected send needs timeout information */
//    cip->secs_per_tick = AB_EIP_SECS_PER_TICK; /* seconds per tick */
//    cip->timeout_ticks = AB_EIP_TIMEOUT_TICKS; /* timeout = src_secs_per_tick * src_timeout_ticks */
//
//    /* size of embedded packet */
//    cip->uc_cmd_length = h2le16(embed_end - embed_start);
//
//    /* set the size of the request */
//    req->request_size = data - (req->data);
//
//    /* mark it as ready to send */
//    req->send_request = 1;
//
//    /* add the request to the session's list. */
//    rc = session_add_request(tag->session, req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to add request to session! rc=%d", rc);
//        tag->reqs[slot] = rc_dec(req);
//        return rc;
//    }
//
//    /* save the request for later */
//    tag->reqs[slot] = req;
//
//    pdebug(DEBUG_INFO, "Done");
//
//    return PLCTAG_STATUS_OK;
//}
//
//
//
//
//int build_write_request_connected(ab_tag_p tag, int slot, int byte_offset)
//{
//    int rc = PLCTAG_STATUS_OK;
//    eip_cip_co_req* cip = NULL;
//    uint8_t* data = NULL;
//    ab_request_p req = NULL;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    /* get a request buffer */
//    rc = request_create(&req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to get new request.  rc=%d", rc);
//        return rc;
//    }
//
//    req->num_retries_left = tag->num_retries;
//    req->retry_interval = tag->default_retry_interval;
//
//    cip = (eip_cip_co_req*)(req->data);
//
//    /* point to the end of the struct */
//    data = (req->data) + sizeof(eip_cip_co_req);
//
//    /*
//     * set up the embedded CIP read packet
//     * The format is:
//     *
//     * uint8_t cmd
//     * LLA formatted name
//     * data type to write
//     * uint16_t # of elements to write
//     * data to write
//     */
//
//    /*
//     * set up the CIP Read request type.
//     * Different if more than one request.
//     *
//     * This handles a bug where attempting fragmented requests
//     * does not appear to work with a single boolean.
//     */
//    *data = (tag->num_write_requests > 1) ? AB_EIP_CMD_CIP_WRITE_FRAG : AB_EIP_CMD_CIP_WRITE;
//    data++;
//
//    /* copy the tag name into the request */
//    mem_copy(data, tag->encoded_name, tag->encoded_name_size);
//    data += tag->encoded_name_size;
//
//    /* copy encoded type info */
//    if (tag->encoded_type_info_size) {
//        mem_copy(data, tag->encoded_type_info, tag->encoded_type_info_size);
//        data += tag->encoded_type_info_size;
//    } else {
//        pdebug(DEBUG_WARN,"Data type unsupported!");
//        return PLCTAG_ERR_UNSUPPORTED;
//    }
//
//    /* copy the item count, little endian */
//    *((uint16_t*)data) = h2le16(tag->elem_count);
//    data += 2;
//
//    if (tag->num_write_requests > 1) {
//        /* put in the byte offset */
//        *((uint32_t*)data) = h2le32(byte_offset);
//        data += 4;
//    }
//
//    /* now copy the data to write */
//    mem_copy(data, tag->data + byte_offset, tag->write_req_sizes[slot]);
//    data += tag->write_req_sizes[slot];
//
//    /* need to pad data to multiple of 16-bits */
//    if (tag->write_req_sizes[slot] & 0x01) {
//        *data = 0;
//        data++;
//    }
//
//    /* now we go back and fill in the fields of the static part */
//
//    /* encap fields */
//    cip->encap_command = h2le16(AB_EIP_CONNECTED_SEND); /* ALWAYS 0x0070 Unconnected Send*/
//
//    /* router timeout */
//    cip->router_timeout = h2le16(1); /* one second timeout, enough? */
//
//    /* Common Packet Format fields for unconnected send. */
//    cip->cpf_item_count = h2le16(2);                 /* ALWAYS 2 */
//    cip->cpf_cai_item_type = h2le16(AB_EIP_ITEM_CAI);/* ALWAYS 0x00A1 connected address item */
//    cip->cpf_cai_item_length = h2le16(4);            /* ALWAYS 4, size of connection ID*/
//    cip->cpf_cdi_item_type = h2le16(AB_EIP_ITEM_CDI);/* ALWAYS 0x00B1 - connected Data Item */
//    cip->cpf_cdi_item_length = h2le16(data - (uint8_t*)(&cip->cpf_conn_seq_num)); /* REQ: fill in with length of remaining data. */
//
//    /* set the size of the request */
//    req->request_size = data - (req->data);
//
//    /* mark it as ready to send */
//    req->send_request = 1;
//
//    /* store the connection */
//    req->connection = tag->connection;
//
//    /* mark the request as a connected request */
//    req->connected_request = 1;
//
//    /* add the request to the session's list. */
//    rc = session_add_request(tag->session, req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to add request to session! rc=%d", rc);
//        tag->reqs[slot] = rc_dec(req);
//        return rc;
//    }
//
//    /* save the request for later */
//    tag->reqs[slot] = req;
//
//    pdebug(DEBUG_INFO, "Done");
//
//    return PLCTAG_STATUS_OK;
//}
//
//
//int build_write_request_unconnected(ab_tag_p tag, int slot, int byte_offset)
//{
//    int rc = PLCTAG_STATUS_OK;
//    eip_cip_uc_req* cip;
//    uint8_t* data;
//    uint8_t* embed_start, *embed_end;
//    ab_request_p req = NULL;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    /* get a request buffer */
//    rc = request_create(&req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to get new request.  rc=%d", rc);
//        return rc;
//    }
//
//    req->num_retries_left = tag->num_retries;
//    req->retry_interval = tag->default_retry_interval;
//
//    /* point the request struct at the buffer */
//    cip = (eip_cip_uc_req*)(req->data);
//
//    /* point to the end of the struct */
//    data = (req->data) + sizeof(eip_cip_uc_req);
//
//    /*
//     * set up the embedded CIP read packet
//     * The format is:
//     *
//     * uint8_t cmd
//     * LLA formatted name
//     * data type to write
//     * uint16_t # of elements to write
//     * data to write
//     */
//
//    embed_start = data;
//
//    /*
//     * set up the CIP Read request type.
//     * Different if more than one request.
//     *
//     * This handles a bug where attempting fragmented requests
//     * does not appear to work with a single boolean.
//     */
//    *data = (tag->num_write_requests > 1) ? AB_EIP_CMD_CIP_WRITE_FRAG : AB_EIP_CMD_CIP_WRITE;
//    data++;
//
//    /* copy the tag name into the request */
//    mem_copy(data, tag->encoded_name, tag->encoded_name_size);
//    data += tag->encoded_name_size;
//
//    /* copy encoded type info */
//    if (tag->encoded_type_info_size) {
//        mem_copy(data, tag->encoded_type_info, tag->encoded_type_info_size);
//        data += tag->encoded_type_info_size;
//    } else {
//        pdebug(DEBUG_WARN,"Data type unsupported!");
//        return PLCTAG_ERR_UNSUPPORTED;
//    }
//
//    /* copy the item count, little endian */
//    *((uint16_t*)data) = h2le16(tag->elem_count);
//    data += 2;
//
//    if (tag->num_write_requests > 1) {
//        /* put in the byte offset */
//        *((uint32_t*)data) = h2le32(byte_offset);
//        data += 4;
//    }
//
//    /* now copy the data to write */
//    mem_copy(data, tag->data + byte_offset, tag->write_req_sizes[slot]);
//    data += tag->write_req_sizes[slot];
//
//    /* need to pad data to multiple of 16-bits */
//    if (tag->write_req_sizes[slot] & 0x01) {
//        *data = 0;
//        data++;
//    }
//
//    /* mark the end of the embedded packet */
//    embed_end = data;
//
//    /*
//     * after the embedded packet, we need to tell the message router
//     * how to get to the target device.
//     */
//
//    /* Now copy in the routing information for the embedded message */
//    *data = (tag->conn_path_size) / 2; /* in 16-bit words */
//    data++;
//    *data = 0;
//    data++;
//    mem_copy(data, tag->conn_path, tag->conn_path_size);
//    data += tag->conn_path_size;
//
//    /* now fill in the rest of the structure. */
//
//    /* encap fields */
//    cip->encap_command = h2le16(AB_EIP_READ_RR_DATA); /* ALWAYS 0x006F Unconnected Send*/
//
//    /* router timeout */
//    cip->router_timeout = h2le16(1); /* one second timeout, enough? */
//
//    /* Common Packet Format fields for unconnected send. */
//    cip->cpf_item_count = h2le16(2);                  /* ALWAYS 2 */
//    cip->cpf_nai_item_type = h2le16(AB_EIP_ITEM_NAI); /* ALWAYS 0 */
//    cip->cpf_nai_item_length = h2le16(0);             /* ALWAYS 0 */
//    cip->cpf_udi_item_type = h2le16(AB_EIP_ITEM_UDI); /* ALWAYS 0x00B2 - Unconnected Data Item */
//    cip->cpf_udi_item_length = h2le16(data - (uint8_t*)(&(cip->cm_service_code))); /* REQ: fill in with length of remaining data. */
//
//    /* CM Service Request - Connection Manager */
//    cip->cm_service_code = AB_EIP_CMD_UNCONNECTED_SEND; /* 0x52 Unconnected Send */
//    cip->cm_req_path_size = 2;                          /* 2, size in 16-bit words of path, next field */
//    cip->cm_req_path[0] = 0x20;                         /* class */
//    cip->cm_req_path[1] = 0x06;                         /* Connection Manager */
//    cip->cm_req_path[2] = 0x24;                         /* instance */
//    cip->cm_req_path[3] = 0x01;                         /* instance 1 */
//
//    /* Unconnected send needs timeout information */
//    cip->secs_per_tick = AB_EIP_SECS_PER_TICK; /* seconds per tick */
//    cip->timeout_ticks = AB_EIP_TIMEOUT_TICKS; /* timeout = srd_secs_per_tick * src_timeout_ticks */
//
//    /* size of embedded packet */
//    cip->uc_cmd_length = h2le16(embed_end - embed_start);
//
//    /* set the size of the request */
//    req->request_size = data - (req->data);
//
//    /* mark it as ready to send */
//    req->send_request = 1;
//
//    /* add the request to the session's list. */
//    rc = session_add_request(tag->session, req);
//
//    if (rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_ERROR, "Unable to add request to session! rc=%d", rc);
//        tag->reqs[slot] = rc_dec(req);
//        return rc;
//    }
//
//    /* save the request for later */
//    tag->reqs[slot] = req;
//
//    pdebug(DEBUG_INFO, "Done");
//
//    return PLCTAG_STATUS_OK;
//}
//
//
//
//
///*
// * check_read_status_connected
// *
// * This routine checks for any outstanding requests and copies in data
// * that has arrived.  At the end of the request, it will clean up the request
// * buffers.  This is not thread-safe!  It should be called with the tag mutex
// * locked!
// */
//
//static int check_read_status_connected(ab_tag_p tag)
//{
//    int rc = PLCTAG_STATUS_OK;
//    eip_cip_co_resp* cip_resp;
//    uint8_t* data;
//    uint8_t* data_end;
//    int i;
//    ab_request_p req;
//    int byte_offset = 0;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    /* is there an outstanding request? */
//    if (!tag->reqs) {
//        tag->read_in_progress = 0;
//        pdebug(DEBUG_WARN,"Read in progress, but no requests in flight!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    for (i = 0; i < tag->num_read_requests; i++) {
//        if (tag->reqs[i] && !tag->reqs[i]->resp_received) {
//            return PLCTAG_STATUS_PENDING;
//        }
//    }
//
//    /*
//     * process each request.  If there is more than one request, then
//     * we need to make sure that we copy the data into the right part
//     * of the tag's data buffer.
//     */
//    for (i = 0; i < tag->num_read_requests; i++) {
//        req = tag->reqs[i];
//
//        if (!req) {
//            rc = PLCTAG_ERR_NULL_PTR;
//            break;
//        }
//
//        /* skip if already processed */
//        if (req->processed) {
//            byte_offset += tag->read_req_sizes[i];
//            continue;
//        }
//
//        req->processed = 1;
//
//        pdebug(DEBUG_DETAIL, "processing request %d", i);
//
//        /* point to the data */
//        cip_resp = (eip_cip_co_resp*)(req->data);
//
//        /* point to the start of the data */
//        data = (req->data) + sizeof(eip_cip_co_resp);
//
//        /* point the end of the data */
//        data_end = (req->data + le2h16(cip_resp->encap_length) + sizeof(eip_encap_t));
//
//        /* check the status */
//        if (le2h16(cip_resp->encap_command) != AB_EIP_CONNECTED_SEND) {
//            pdebug(DEBUG_WARN, "Unexpected EIP packet type received: %d!", cip_resp->encap_command);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (le2h16(cip_resp->encap_status) != AB_EIP_OK) {
//            pdebug(DEBUG_WARN, "EIP command failed, response code: %d", cip_resp->encap_status);
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//
//        /*
//         * FIXME
//         *
//         * It probably should not be necessary to check for both as setting the type to anything other
//         * than fragmented is error-prone.
//         */
//
//        if (cip_resp->reply_service != (AB_EIP_CMD_CIP_READ_FRAG | AB_EIP_CMD_CIP_OK)
//            && cip_resp->reply_service != (AB_EIP_CMD_CIP_READ | AB_EIP_CMD_CIP_OK) ) {
//            pdebug(DEBUG_WARN, "CIP response reply service unexpected: %d", cip_resp->reply_service);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (cip_resp->status != AB_CIP_STATUS_OK && cip_resp->status != AB_CIP_STATUS_FRAG) {
//            pdebug(DEBUG_WARN, "CIP read failed with status: 0x%x %s", cip_resp->status, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_SHORT));
//            pdebug(DEBUG_INFO, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_LONG));
//
//            switch (cip_resp->status) {
//                case 0x04: /* FIXME - should be defined constants */
//                case 0x05:
//                case 0x13:
//                case 0x1C:
//                    rc = PLCTAG_ERR_BAD_PARAM;
//                    break;
//
//                default:
//                    rc = PLCTAG_ERR_REMOTE_ERR;
//                    break;
//            }
//
//            break;
//        }
//
//        /* the first byte of the response is a type byte. */
//        pdebug(DEBUG_DETAIL, "type byte = %d (%x)", (int)*data, (int)*data);
//
//        /*
//         * AB has a relatively complicated scheme for data typing.  The type is
//         * required when writing.  Most of the types are basic types and occupy
//         * a known amount of space.  Aggregate types like structs and arrays
//         * occupy a variable amount of space.  In addition, structs and arrays
//         * can be in two forms: full and abbreviated.  Full form for structs includes
//         * all data types (in full) for fields of the struct.  Abbreviated form for
//         * structs includes a two byte CRC calculated across the full form.  For arrays,
//         * full form includes index limits and base data type.  Abbreviated arrays
//         * drop the limits and encode any structs as abbreviate structs.  At least
//         * we think this is what is happening.
//         *
//         * Luckily, we do not actually care what these bytes mean, we just need
//         * to copy them and skip past them for the actual data.
//         */
//
//        /* check for a simple/base type */
//        if ((*data) >= AB_CIP_DATA_BIT && (*data) <= AB_CIP_DATA_STRINGI) {
//            /* copy the type info for later. */
//            if (tag->encoded_type_info_size == 0) {
//                tag->encoded_type_info_size = 2;
//                mem_copy(tag->encoded_type_info, data, tag->encoded_type_info_size);
//            }
//
//            /* skip the type byte and zero length byte */
//            data += 2;
//        } else if ((*data) == AB_CIP_DATA_ABREV_STRUCT || (*data) == AB_CIP_DATA_ABREV_ARRAY ||
//                   (*data) == AB_CIP_DATA_FULL_STRUCT || (*data) == AB_CIP_DATA_FULL_ARRAY) {
//            /* this is an aggregate type of some sort, the type info is variable length */
//            int type_length =
//                *(data + 1) + 2; /*
//                                    * MAGIC
//                                    * add 2 to get the total length including
//                                    * the type byte and the length byte.
//                                    */
//
//            /* check for extra long types */
//            if (type_length > MAX_TAG_TYPE_INFO) {
//                pdebug(DEBUG_WARN, "Read data type info is too long (%d)!", type_length);
//                rc = PLCTAG_ERR_TOO_LONG;
//                break;
//            }
//
//            /* copy the type info for later. */
//            if (tag->encoded_type_info_size == 0) {
//                tag->encoded_type_info_size = type_length;
//                mem_copy(tag->encoded_type_info, data, tag->encoded_type_info_size);
//            }
//
//            data += type_length;
//        } else {
//            pdebug(DEBUG_WARN, "Unsupported data type returned, type byte=%d", *data);
//            rc = PLCTAG_ERR_UNSUPPORTED;
//            break;
//        }
//
//        /* copy data into the tag. */
//        if ((byte_offset + (data_end - data)) > tag->size) {
//            pdebug(DEBUG_WARN,
//                   "Read data is too long (%d bytes) to fit in tag data buffer (%d bytes)!",
//                   byte_offset + (int)(data_end - data),
//                   tag->size);
//            rc = PLCTAG_ERR_TOO_LONG;
//            break;
//        }
//
//        pdebug(DEBUG_DETAIL, "Got %d bytes of data", (data_end - data));
//
//        /*
//         * copy the data, but only if this is not
//         * a pre-read for a subsequent write!  We do not
//         * want to overwrite the data the upstream has
//         * put into the tag's data buffer.
//         */
//        if (!tag->pre_write_read) {
//            mem_copy(tag->data + byte_offset, data, (data_end - data));
//        }
//
//        /* save the size of the response for next time */
//        tag->read_req_sizes[i] = (data_end - data);
//
//        /*
//         * did we get any data back? a zero-length response is
//         * an error here.
//         */
//
//        if ((data_end - data) == 0) {
//            rc = PLCTAG_ERR_NO_DATA;
//            break;
//        } else {
//            /* bump the byte offset */
//            byte_offset += (data_end - data);
//
//            /* set the return code */
//            rc = PLCTAG_STATUS_OK;
//        }
//    } /* end of for(i = 0; i < tag->num_requests; i++) */
//
//    /* are we actually done? */
//    if (rc == PLCTAG_STATUS_OK) {
//        if (byte_offset < tag->size) {
//            /* no, not yet */
//            if (tag->first_read) {
//                /* call read start again to get the next piece */
//                pdebug(DEBUG_DETAIL, "calling ab_rag_read_cip_start_unsafe() to get the next chunk.");
//                rc = eip_cip_tag_read_start(tag);
//            } else {
//                pdebug(DEBUG_WARN, "Insufficient data read for tag!");
//                ab_tag_abort(tag);
//                rc = PLCTAG_ERR_READ;
//            }
//        } else {
//            /* done! */
//            tag->first_read = 0;
//
//            tag->read_in_progress = 0;
//
//            /* have the IO thread take care of the request buffers */
//            ab_tag_abort(tag);
//
//            /* if this is a pre-read for a write, then pass off the the write routine */
//            if (tag->pre_write_read) {
//                pdebug(DEBUG_DETAIL, "Restarting write call now.");
//
//                tag->pre_write_read = 0;
//                rc = eip_cip_tag_write_start(tag);
//            }
//        }
//    } else {
//        /* error ! */
//        pdebug(DEBUG_WARN, "Error received!");
//    }
//
//    pdebug(DEBUG_INFO, "Done.");
//
//    return rc;
//}
//
//
//
//static int check_read_status_unconnected(ab_tag_p tag)
//{
//    int rc = PLCTAG_STATUS_OK;
//    eip_cip_uc_resp* cip_resp;
//    uint8_t* data;
//    uint8_t* data_end;
//    int i;
//    ab_request_p req;
//    int byte_offset = 0;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    if(!tag) {
//        pdebug(DEBUG_ERROR,"Null tag pointer passed!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    /* is there an outstanding request? */
//    if (!tag->reqs) {
//        tag->read_in_progress = 0;
//        pdebug(DEBUG_WARN,"Read in progress, but no requests in flight!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    for (i = 0; i < tag->num_read_requests; i++) {
//        if (tag->reqs[i] && !tag->reqs[i]->resp_received) {
//            return PLCTAG_STATUS_PENDING;
//        }
//    }
//
//    /*
//     * process each request.  If there is more than one request, then
//     * we need to make sure that we copy the data into the right part
//     * of the tag's data buffer.
//     */
//    for (i = 0; i < tag->num_read_requests; i++) {
//        req = tag->reqs[i];
//
//        if (!req) {
//            rc = PLCTAG_ERR_NULL_PTR;
//            break;
//        }
//
//        /* skip if already processed */
//        if (req->processed) {
//            byte_offset += tag->read_req_sizes[i];
//            continue;
//        }
//
//        req->processed = 1;
//
//        pdebug(DEBUG_INFO, "processing request %d", i);
//
//        /* point to the data */
//        cip_resp = (eip_cip_uc_resp*)(req->data);
//
//        /* point to the start of the data */
//        data = (req->data) + sizeof(eip_cip_uc_resp);
//
//        /* point the end of the data */
//        data_end = (req->data + le2h16(cip_resp->encap_length) + sizeof(eip_encap_t));
//
//        /* check the status */
//        if (le2h16(cip_resp->encap_command) != AB_EIP_READ_RR_DATA) {
//            pdebug(DEBUG_WARN, "Unexpected EIP packet type received: %d!", cip_resp->encap_command);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (le2h16(cip_resp->encap_status) != AB_EIP_OK) {
//            pdebug(DEBUG_WARN, "EIP command failed, response code: %d", cip_resp->encap_status);
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//
//        /*
//         * FIXME
//         *
//         * It probably should not be necessary to check for both as setting the type to anything other
//         * than fragmented is error-prone.
//         */
//
//        if (cip_resp->reply_service != (AB_EIP_CMD_CIP_READ_FRAG | AB_EIP_CMD_CIP_OK)
//            && cip_resp->reply_service != (AB_EIP_CMD_CIP_READ | AB_EIP_CMD_CIP_OK) ) {
//            pdebug(DEBUG_WARN, "CIP response reply service unexpected: %d", cip_resp->reply_service);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (cip_resp->status != AB_CIP_STATUS_OK && cip_resp->status != AB_CIP_STATUS_FRAG) {
//            pdebug(DEBUG_WARN, "CIP read failed with status: 0x%x %s", cip_resp->status, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_SHORT));
//            pdebug(DEBUG_INFO, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_LONG));
//
//            switch (cip_resp->status) {
//                case 0x04: /* FIXME - should be defined constants */
//                case 0x05:
//                case 0x13:
//                case 0x1C:
//                    rc = PLCTAG_ERR_BAD_PARAM;
//                    break;
//
//                default:
//                    rc = PLCTAG_ERR_REMOTE_ERR;
//                    break;
//            }
//
//            break;
//        }
//
//        /* the first byte of the response is a type byte. */
//        pdebug(DEBUG_DETAIL, "type byte = %d (%x)", (int)*data, (int)*data);
//
//        /*
//         * AB has a relatively complicated scheme for data typing.  The type is
//         * required when writing.  Most of the types are basic types and occupy
//         * a known amount of space.  Aggregate types like structs and arrays
//         * occupy a variable amount of space.  In addition, structs and arrays
//         * can be in two forms: full and abbreviated.  Full form for structs includes
//         * all data types (in full) for fields of the struct.  Abbreviated form for
//         * structs includes a two byte CRC calculated across the full form.  For arrays,
//         * full form includes index limits and base data type.  Abbreviated arrays
//         * drop the limits and encode any structs as abbreviate structs.  At least
//         * we think this is what is happening.
//         *
//         * Luckily, we do not actually care what these bytes mean, we just need
//         * to copy them and skip past them for the actual data.
//         */
//
//        /* check for a simple/base type */
//        if ((*data) >= AB_CIP_DATA_BIT && (*data) <= AB_CIP_DATA_STRINGI) {
//            /* copy the type info for later. */
//            if (tag->encoded_type_info_size == 0) {
//                tag->encoded_type_info_size = 2;
//                mem_copy(tag->encoded_type_info, data, tag->encoded_type_info_size);
//            }
//
//            /* skip the type byte and zero length byte */
//            data += 2;
//        } else if ((*data) == AB_CIP_DATA_ABREV_STRUCT || (*data) == AB_CIP_DATA_ABREV_ARRAY ||
//                   (*data) == AB_CIP_DATA_FULL_STRUCT || (*data) == AB_CIP_DATA_FULL_ARRAY) {
//            /* this is an aggregate type of some sort, the type info is variable length */
//            int type_length =
//                *(data + 1) + 2;  /*
//                                   * MAGIC
//                                   * add 2 to get the total length including
//                                   * the type byte and the length byte.
//                                   */
//
//            /* check for extra long types */
//            if (type_length > MAX_TAG_TYPE_INFO) {
//                pdebug(DEBUG_WARN, "Read data type info is too long (%d)!", type_length);
//                rc = PLCTAG_ERR_TOO_LONG;
//                break;
//            }
//
//            /* copy the type info for later. */
//            if (tag->encoded_type_info_size == 0) {
//                tag->encoded_type_info_size = type_length;
//                mem_copy(tag->encoded_type_info, data, tag->encoded_type_info_size);
//            }
//
//            data += type_length;
//        } else {
//            pdebug(DEBUG_WARN, "Unsupported data type returned, type byte=%d", *data);
//            rc = PLCTAG_ERR_UNSUPPORTED;
//            break;
//        }
//
//        /* copy data into the tag. */
//        if ((byte_offset + (data_end - data)) > tag->size) {
//            pdebug(DEBUG_WARN,
//                   "Read data is too long (%d bytes) to fit in tag data buffer (%d bytes)!",
//                   byte_offset + (int)(data_end - data),
//                   tag->size);
//            pdebug(DEBUG_WARN,"byte_offset=%d, data size=%d", (int)byte_offset, (int)(data_end - data));
//            rc = PLCTAG_ERR_TOO_LONG;
//            break;
//        }
//
//        pdebug(DEBUG_INFO, "Got %d bytes of data", (data_end - data));
//
//        /*
//         * copy the data, but only if this is not
//         * a pre-read for a subsequent write!  We do not
//         * want to overwrite the data the upstream has
//         * put into the tag's data buffer.
//         */
//        if (!tag->pre_write_read) {
//            mem_copy(tag->data + byte_offset, data, (data_end - data));
//        }
//
//        /* save the size of the response for next time */
//        tag->read_req_sizes[i] = (data_end - data);
//
//        /*
//         * did we get any data back? a zero-length response is
//         * an error here.
//         */
//
//        if ((data_end - data) == 0) {
//            rc = PLCTAG_ERR_NO_DATA;
//            break;
//        } else {
//            /* bump the byte offset */
//            byte_offset += (data_end - data);
//
//            /* set the return code */
//            rc = PLCTAG_STATUS_OK;
//        }
//    } /* end of for(i = 0; i < tag->num_requests; i++) */
//
//    /* are we actually done? */
//    if (rc == PLCTAG_STATUS_OK) {
//        if (byte_offset < tag->size) {
//            /* no, not yet */
//            if (tag->first_read) {
//                /* call read start again to get the next piece */
//                pdebug(DEBUG_DETAIL, "calling eip_cip_tag_read_start() to get the next chunk.");
//                rc = eip_cip_tag_read_start(tag);
//            } else {
//                pdebug(DEBUG_WARN, "Insufficient data read for tag!");
//                ab_tag_abort(tag);
//                rc = PLCTAG_ERR_READ;
//            }
//        } else {
//            /* done! */
//            tag->first_read = 0;
//
//            tag->read_in_progress = 0;
//
//            /* have the IO thread take care of the request buffers */
//            ab_tag_abort(tag);
//
//            /* if this is a pre-read for a write, then pass off the the write routine */
//            if (tag->pre_write_read) {
//                pdebug(DEBUG_INFO, "Restarting write call now.");
//
//                tag->pre_write_read = 0;
//                rc = eip_cip_tag_write_start(tag);
//            }
//        }
//    } else {
//        /* error ! */
//        pdebug(DEBUG_WARN, "Error received!");
//    }
//
//    pdebug(DEBUG_DETAIL, "Done.");
//
//    return rc;
//}
//
//
//
///*
// * check_write_status_connected
// *
// * This routine must be called with the tag mutex locked.  It checks the current
// * status of a write operation.  If the write is done, it triggers the clean up.
// */
//
//static int check_write_status_connected(ab_tag_p tag)
//{
//    eip_cip_co_resp* cip_resp;
//    int rc = PLCTAG_STATUS_OK;
//    int i;
//    ab_request_p req;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    if(!tag) {
//        pdebug(DEBUG_ERROR,"Null tag pointer passed!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    /* is there an outstanding request? */
//    if (!tag->reqs) {
//        tag->write_in_progress = 0;
//        pdebug(DEBUG_INFO, "Write in progress but noo outstanding requests!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    for (i = 0; i < tag->num_write_requests; i++) {
//        if (tag->reqs[i] && !tag->reqs[i]->resp_received) {
//            return PLCTAG_STATUS_PENDING;
//        }
//    }
//
//    /*
//     * process each request.  If there is more than one request, then
//     * we need to make sure that we copy the data into the right part
//     * of the tag's data buffer.
//     */
//    for (i = 0; i < tag->num_write_requests; i++) {
//        int reply_service;
//
//        req = tag->reqs[i];
//
//        if (!req) {
//            rc = PLCTAG_ERR_NULL_PTR;
//            break;
//        }
//
//        /* point to the data */
//        cip_resp = (eip_cip_co_resp*)(req->data);
//
//        if (le2h16(cip_resp->encap_command) != AB_EIP_CONNECTED_SEND) {
//            pdebug(DEBUG_WARN, "Unexpected EIP packet type received: %d!", cip_resp->encap_command);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (le2h16(cip_resp->encap_status) != AB_EIP_OK) {
//            pdebug(DEBUG_WARN, "EIP command failed, response code: %d", cip_resp->encap_status);
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//
//        /* if we have fragmented the request, we need to look for a different return code */
//        reply_service = ((tag->num_write_requests > 1) ? (AB_EIP_CMD_CIP_WRITE_FRAG | AB_EIP_CMD_CIP_OK) :
//                         (AB_EIP_CMD_CIP_WRITE | AB_EIP_CMD_CIP_OK));
//
//        if (cip_resp->reply_service != reply_service) {
//            pdebug(DEBUG_WARN, "CIP response reply service unexpected: %d", cip_resp->reply_service);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (cip_resp->status != AB_CIP_STATUS_OK && cip_resp->status != AB_CIP_STATUS_FRAG) {
//            pdebug(DEBUG_WARN, "CIP read failed with status: 0x%x %s", cip_resp->status, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_SHORT));
//            pdebug(DEBUG_INFO, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_LONG));
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//    }
//
//    /* this triggers the clean up */
//    ab_tag_abort(tag);
//
//    tag->write_in_progress = 0;
//
//    pdebug(DEBUG_DETAIL, "Done.");
//
//    return rc;
//}
//
//
//int check_write_status_unconnected(ab_tag_p tag)
//{
//    eip_cip_uc_resp* cip_resp;
//    int rc = PLCTAG_STATUS_OK;
//    int i;
//    ab_request_p req;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    /* is there an outstanding request? */
//    if (!tag->reqs) {
//        tag->write_in_progress = 0;
//        pdebug(DEBUG_WARN,"Write in progress, but no requests in flight!");
//        return PLCTAG_ERR_NULL_PTR;
//    }
//
//    for (i = 0; i < tag->num_write_requests; i++) {
//        if (tag->reqs[i] && !tag->reqs[i]->resp_received) {
//            return PLCTAG_STATUS_PENDING;
//        }
//    }
//
//    /*
//     * process each request.  If there is more than one request, then
//     * we need to make sure that we copy the data into the right part
//     * of the tag's data buffer.
//     */
//    for (i = 0; i < tag->num_write_requests; i++) {
//        int reply_service;
//
//        req = tag->reqs[i];
//
//        if (!req) {
//            rc = PLCTAG_ERR_NULL_PTR;
//            break;
//        }
//
//        /* point to the data */
//        cip_resp = (eip_cip_uc_resp*)(req->data);
//
//        if (le2h16(cip_resp->encap_command) != AB_EIP_READ_RR_DATA) {
//            pdebug(DEBUG_WARN, "Unexpected EIP packet type received: %d!", cip_resp->encap_command);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (le2h16(cip_resp->encap_status) != AB_EIP_OK) {
//            pdebug(DEBUG_WARN, "EIP command failed, response code: %d", cip_resp->encap_status);
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//
//        /* if we have fragmented the request, we need to look for a different return code */
//        reply_service = ((tag->num_write_requests > 1) ? (AB_EIP_CMD_CIP_WRITE_FRAG | AB_EIP_CMD_CIP_OK) :
//                         (AB_EIP_CMD_CIP_WRITE | AB_EIP_CMD_CIP_OK));
//
//        if (cip_resp->reply_service != reply_service) {
//            pdebug(DEBUG_WARN, "CIP response reply service unexpected: %d", cip_resp->reply_service);
//            rc = PLCTAG_ERR_BAD_DATA;
//            break;
//        }
//
//        if (cip_resp->status != AB_CIP_STATUS_OK && cip_resp->status != AB_CIP_STATUS_FRAG) {
//            pdebug(DEBUG_WARN, "CIP read failed with status: 0x%x %s", cip_resp->status, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_SHORT));
//            pdebug(DEBUG_INFO, decode_cip_error((uint8_t *)&cip_resp->status, AB_ERROR_STR_LONG));
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//    }
//
//    /* this triggers the clean up */
//    ab_tag_abort(tag);
//
//    tag->write_in_progress = 0;
//
//    pdebug(DEBUG_DETAIL, "Done.");
//
//    return rc;
//}
//
//
//
//int calculate_write_sizes(ab_tag_p tag)
//{
//    int overhead;
//    int data_per_packet;
//    int num_reqs;
//    int rc = PLCTAG_STATUS_OK;
//    int i;
//    int byte_offset;
//
//    pdebug(DEBUG_DETAIL, "Starting.");
//
//    if (tag->num_write_requests > 0) {
//        pdebug(DEBUG_DETAIL, "Early termination, write sizes already calculated.");
//        return rc;
//    }
//
//    /* if we are here, then we have all the type data etc. */
//    if(tag->connection) {
//        overhead = sizeof(eip_cip_co_req);
//    } else {
//        overhead = sizeof(eip_cip_uc_req);
//    }
//
//    /* we want to over-estimate here. */
//    overhead = overhead                      /* base packet size */
//               + 1                           /* service request, one byte */
//               + tag->encoded_name_size      /* full encoded name */
//               + tag->encoded_type_info_size /* encoded type size */
//               + tag->conn_path_size + 2     /* encoded device path size plus two bytes for length and padding */
//               + 2                           /* element count, 16-bit int */
//               + 4                           /* byte offset, 32-bit int */
//               + 8;                          /* MAGIC fudge factor */
//
//    data_per_packet = MAX_EIP_PACKET_SIZE - overhead;
//
//    /* we want a multiple of 4 bytes */
//    /* FIXME - this might be undefined behavior.  Need to check for negative first and then mask. */
//    data_per_packet &= 0xFFFFFFFC;
//
//    if (data_per_packet <= 0) {
//        pdebug(DEBUG_WARN,
//               "Unable to send request.  Packet overhead, %d bytes, is too large for packet, %d bytes!",
//               overhead,
//               MAX_EIP_PACKET_SIZE);
//        return PLCTAG_ERR_TOO_LONG;
//    }
//
//    num_reqs = (tag->size + (data_per_packet - 1)) / data_per_packet;
//
//    pdebug(DEBUG_DETAIL, "We need %d requests.", num_reqs);
//
//    byte_offset = 0;
//
//    for (i = 0; i < num_reqs && rc == PLCTAG_STATUS_OK; i++) {
//        /* allocate a new slot */
//        rc = allocate_write_request_slot(tag);
//
//        if (rc == PLCTAG_STATUS_OK) {
//            /* how much data are we going to write in this packet? */
//            if ((tag->size - byte_offset) > data_per_packet) {
//                tag->write_req_sizes[i] = data_per_packet;
//            } else {
//                tag->write_req_sizes[i] = (tag->size - byte_offset);
//            }
//
//            pdebug(DEBUG_DETAIL, "Request %d is of size %d.", i, tag->write_req_sizes[i]);
//
//            /* update the byte offset for the next packet */
//            byte_offset += tag->write_req_sizes[i];
//        }
//    }
//
//    pdebug(DEBUG_DETAIL, "Done.");
//
//    return rc;
//}
//
///*#ifdef __cplusplus
//}
//#endif
//*/

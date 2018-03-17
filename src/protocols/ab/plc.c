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
 * 2015-09-12  KRH - Created file.                                        *
 *                                                                        *
 **************************************************************************/


#include <platform.h>
#include <ab/ab_common.h>
#include <ab/plc.h>
#include <ab/connection.h>
#include <ab/request.h>
#include <ab/eip.h>
#include <util/debug.h>
#include <util/mem.h>
#include <util/refcount.h>
#include <stdlib.h>
#include <time.h>





struct ab_plc_t {
    /* required for live objects */
    struct liveobj_t liveobj;
    int id;
    
    int registered;

    thread_p setup_thread;
    
    
    mutex_p mutex;
    
    /* gateway connection related info */
    char host[MAX_SESSION_HOST];
    int port;
    sock_p sock;
    int is_connected;
    int status;

    /* registration info */
    uint32_t session_handle;

    /* Sequence ID for requests. */
    uint64_t session_seq_id;

    /* current request being sent, only one at a time */
    ab_request_p current_request;

    /* list of outstanding requests for this PLC */
    ab_request_p requests;

    /* counter for number of messages in flight */
    int num_reqs_in_flight;
    //int64_t next_packet_time_us;
    //int64_t next_packet_interval_us;

    int64_t retry_interval;

    /* short cumulative period for calculating round trip time. */
    int64_t round_trip_samples[SESSION_NUM_ROUND_TRIP_SAMPLES];
    int round_trip_sample_index;

    /* data for receiving messages */
    uint64_t resp_seq_id;
    int has_response;
    uint32_t recv_offset;
    uint8_t recv_data[MAX_REQ_RESP_SIZE];

    /* connection for this PLC */
    ab_connection_p connection;
    uint32_t conn_serial_number; /* id for the next connection */
};



/*
 * Externally visible global variables
 */
mutex_p global_plc_mut = NULL;


/* local variables. */



static ab_plc_p plc_create_unsafe(const char* host, int gw_port);
static THREAD_FUNC(plc_init);
static int add_plc_unsafe(ab_plc_p n);
static int find_plc_by_host_unsafe(liveobj_p obj, int type, void *host_arg);
static int plc_connect(ab_plc_p plc);
static void plc_destroy(rc_ptr plc);
static int session_register(ab_plc_p plc);
static int session_unregister_unsafe(ab_plc_p plc);

//static int plc_remove_connection_unsafe(ab_plc_p session, ab_connection_p connection);
static int plc_remove_request_unsafe(ab_plc_p plc, ab_request_p req);

static void plc_handler(ab_plc_p plc);
static int plc_check_incoming_data(ab_plc_p plc);
static int plc_check_outgoing_data(ab_plc_p plc);


/*
 * plc_get_new_seq_id_unsafe
 *
 * A wrapper to get a new session sequence ID.  Not thread safe.
 */

uint64_t plc_get_new_seq_id_unsafe(ab_plc_p sess)
{
    return sess->session_seq_id++;
}

/*
 * plc_get_new_seq_id
 *
 * A thread-safe function to get a new session sequence ID.
 */

uint64_t plc_get_new_seq_id(ab_plc_p sess)
{
    uint16_t res = 0;

    //pdebug(DEBUG_DETAIL, "entering critical block %p",global_plc_mut);
    critical_block(sess->mutex) {
        res = (uint16_t)plc_get_new_seq_id_unsafe(sess);
    }
    //pdebug(DEBUG_DETAIL, "leaving critical block %p", global_plc_mut);

    return res;
}


//static int connection_is_usable_unsafe(ab_connection_p connection)
//{
//    if(!connection) {
//        return 0;
//    }
//
////    if(connection->exclusive) {
////        return 0;
////    }
////
////    if(connection->disconnect_in_progress) {
////        return 0;
////    }
//
//    return 1;
//}



ab_connection_p plc_find_connection_by_path_unsafe(ab_plc_p plc,const char *path)
{
    ab_connection_p connection;
    
    (void)path;

    connection = plc->connection;

    /*
     * there are a lot of conditions.
     * We do not want to use connections that are in the process of shutting down.
     * We do not want to use connections that are used exclusively by one tag.
     * We want to use connections that have the same path as the tag.
     */
//    while (connection && !connection_is_usable_unsafe(connection) && str_cmp_i(connection_path(connection), path) != 0) {
//        connection = connection->next;
//    }


//    /* add to the ref count since we found an existing one. */
//    if(connection) {
//        rc_inc(connection);
//    }

    return rc_inc(connection);
}



int plc_add_connection_unsafe(ab_plc_p plc, ab_connection_p connection)
{
    pdebug(DEBUG_DETAIL, "Starting");

    /* add the connection to the list in the PLC */
//    connection->next = session->connections;
//    session->connections = connection;

    if(plc->connection) {
        pdebug(DEBUG_WARN, "A connection already exists!");
    } else {
        plc->connection = connection;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
}




int plc_add_connection(ab_plc_p plc, ab_connection_p connection)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(plc) {
        critical_block(plc->mutex) {
            rc = plc_add_connection_unsafe(plc, connection);
        }
    } else {
        pdebug(DEBUG_WARN, "PLC ptr is null!");
        rc = PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

/* must have the PLC mutex held here. */
int plc_remove_connection_unsafe(ab_plc_p plc, ab_connection_p connection)
{
//    ab_connection_p cur;
//    ab_connection_p prev;
    /* int debug = plc->debug; */
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");
    
    if(plc->connection != connection) {
        pdebug(DEBUG_WARN,"Wrong connection!");
        return PLCTAG_ERR_NOT_FOUND;
    }
    
    plc->connection = NULL;

//    cur = session->connections;
//    prev = NULL;
//
//    while (cur && cur != connection) {
//        prev = cur;
//        cur = cur->next;
//    }
//
//    if (cur && cur == connection) {
//        if (prev) {
//            prev->next = cur->next;
//        } else {
//            session->connections = cur->next;
//        }
//
//        rc = PLCTAG_STATUS_OK;
//    } else {
//        rc = PLCTAG_ERR_NOT_FOUND;
//    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

int plc_remove_connection(ab_plc_p plc, ab_connection_p connection)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(plc) {
        critical_block(plc->mutex) {
            rc = plc_remove_connection_unsafe(plc, connection);
        }
    } else {
        rc = PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}


uint32_t plc_get_new_connection_id_unsafe(ab_plc_p plc)
{
    ++(plc->conn_serial_number);
    
    return plc->conn_serial_number;
}


uint32_t plc_get_new_connection_id(ab_plc_p plc)
{
    uint32_t result = 0;
    
    critical_block(plc->mutex) {
        result = plc_get_new_connection_id_unsafe(plc);
    }
    
    return result;
}


int plc_status(ab_plc_p plc)
{
    int result = PLCTAG_STATUS_OK;
    
    if(!plc) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    critical_block(plc->mutex) {
        result = plc->status;
    }
    
    return result;
}


int plc_find_or_create(ab_plc_p *tag_plc, attr attribs)
{
    /*int debug = attr_get_int(attribs,"debug",0);*/
    const char* plc_gw = attr_get_str(attribs, "gateway", "");
    int plc_gw_port = attr_get_int(attribs, "gateway_port", AB_EIP_DEFAULT_PORT);
    ab_plc_p plc = AB_PLC_NULL;
    //int new_plc = 0;
    int shared_session = attr_get_int(attribs, "share_session", 1); /* share the session by default. */
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    critical_block(global_plc_mut) {
        /* if we are to share sessions, then look for an existing one. */
        if (shared_session) {
            /* this returns a strong reference */
            plc = (ab_plc_p)liveobj_find(find_plc_by_host_unsafe, (void *)plc_gw);
        } else {
            /* no sharing, create a new one */
            plc = AB_PLC_NULL;
        }

        if (plc == AB_PLC_NULL) {
            pdebug(DEBUG_DETAIL,"Creating new PLC.");
            plc = plc_create_unsafe(plc_gw, plc_gw_port);

            if (plc == AB_PLC_NULL) {
                pdebug(DEBUG_WARN, "unable to create or find a PLC object!");
                rc = PLCTAG_ERR_BAD_GATEWAY;
            } 
        } else {
            pdebug(DEBUG_DETAIL,"Reusing existing PLC object.");
        }
    }

    /* store it into the tag */
    *tag_plc = plc;

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

int add_plc_unsafe(ab_plc_p plc)
{
    pdebug(DEBUG_DETAIL, "Starting");

    plc->id = liveobj_add((liveobj_p)plc, LIVEOBJ_TYPE_AB_PLC, (liveobj_func)plc_handler);
    
    if(plc->id < 0) {
        /* error! */
        pdebug(DEBUG_WARN,"Unable to add PLC to live objects!");
    }

    pdebug(DEBUG_DETAIL, "Done");

    return plc->id;
}

int add_plc(ab_plc_p s)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.");

    critical_block(global_plc_mut) {
        rc = add_plc_unsafe(s);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


static int plc_match_valid(const char *host, ab_plc_p plc)
{
    if(!plc) {
        return 0;
    }

    if(plc->status !=  PLCTAG_STATUS_OK && plc->status != PLCTAG_STATUS_PENDING) {
        return 0;
    }

    if(str_cmp_i(host,plc->host)) {
        return 0;
    }

    return 1;
}


int find_plc_by_host_unsafe(liveobj_p obj, int type, void *host_arg)
{
    ab_plc_p plc;
    
    pdebug(DEBUG_SPEW,"Starting.");

    if(type == LIVEOBJ_TYPE_AB_PLC) {
        plc = (ab_plc_p)obj;
        
        if(plc_match_valid((const char*)host_arg, plc)) {
            /* this is the right one! */
            pdebug(DEBUG_INFO,"Found PLC!");
            
            return PLCTAG_STATUS_OK;
        }
    }
    
    pdebug(DEBUG_SPEW,"Done.");
    
    return PLCTAG_ERR_NOT_FOUND;
}



ab_plc_p plc_create_unsafe(const char* host, int gw_port)
{
    ab_plc_p plc = AB_PLC_NULL;
    static volatile uint32_t srand_setup = 0;
    static volatile uint32_t connection_id = 0;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting");

    pdebug(DEBUG_DETAIL, "Warning: not using passed port %d", gw_port);

    plc = rc_alloc(sizeof(struct ab_plc_t), plc_destroy);

    if (!plc) {
        pdebug(DEBUG_WARN, "Error allocating new PLC.");
        return AB_PLC_NULL;
    }
    
    /* set the function to handle this object */
    //plc->plc_obj_func = (liveobj_func)plc_handler;

    str_copy(plc->host, MAX_SESSION_HOST, host);

    plc->status = PLCTAG_STATUS_PENDING;

    /* check for ID set up */
    if(srand_setup == 0) {
        srand((unsigned int)time_ms());
        srand_setup = 1;
    }

    if(connection_id == 0) {
        connection_id = rand();
    }

    plc->session_seq_id =  rand();

    /*
     * Why is connection_id global?  Because it looks like the PLC might
     * be treating it globally.  I am seeing ForwardOpen errors that seem
     * to be because of duplicate connection IDs even though the session
     * was closed.
     *
     * So, this is more or less unique across all invocations of the library.
     * FIXME - this could collide.  The probability is low, but it could happen
     * as there are only 32 bits.
     */
    plc->conn_serial_number = ++connection_id;

    /* set up packet round trip information */
    for(int index=0; index < SESSION_NUM_ROUND_TRIP_SAMPLES; index++) {
        plc->round_trip_samples[index] = SESSION_DEFAULT_RESEND_INTERVAL_MS;
    }
    plc->retry_interval = SESSION_DEFAULT_RESEND_INTERVAL_MS;

    /* add the new PLC to the list. */
    add_plc_unsafe(plc);
    
    
    /* make sure we have a mutex */
    rc = mutex_create(&plc->mutex);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to create PLC mutex! rc= %d", rc);
        plc->status = rc;
    }

    pdebug(DEBUG_INFO, "Done");

    return plc;
}


/*
 * plc_init
 *
 * This calls several blocking methods and so must not keep the main mutex
 * locked during them.
 */
THREAD_FUNC(plc_init)
{
    ab_plc_p plc = (ab_plc_p)arg;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    /* we must connect to the gateway and register */
    if ((rc = plc_connect(plc)) != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "PLC connect failed!");
        plc->status = rc;
    }

    if (rc == PLCTAG_STATUS_OK && (rc = session_register(plc)) != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Session registration failed!");
        plc->status = rc;
    }
    
    /* record how we did. */
    plc->status = rc;    
    
    pdebug(DEBUG_INFO, "Done.");

    THREAD_RETURN(0);
}



/*
 * plc_connect()
 *
 * Connect to the host/port passed via TCP.
 */

int plc_connect(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    /* Open a socket for communication with the gateway. */
    rc = socket_create(&(plc->sock));

    if (rc) {
        pdebug(DEBUG_WARN, "Unable to create socket for PLC!");
        return 0;
    }

    /* FIXME - this can hang! */
    rc = socket_connect_tcp(plc->sock, plc->host, AB_EIP_DEFAULT_PORT);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Unable to connect socket for PLC!");
        return rc;
    }

    /* everything is OK.  We have a TCP stream open to a gateway. */
    plc->is_connected = 1;

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}


/* ref counted destructor */
void plc_destroy(rc_ptr plc_arg)
{
    ab_plc_p plc = plc_arg;
    ab_request_p req = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    if (!plc) {
        pdebug(DEBUG_WARN, "PLC ptr is null!");

        return;
    }
    
    /* remove the PLC from the list of live objects. */
    liveobj_remove(plc->id);
    
    /* not needed, liveobj already handles this. */
    //remove_plc_unsafe(plc);
    /* check to see if there is a thread. */
    if(plc->setup_thread) {
        thread_join(plc->setup_thread);
        thread_destroy(&plc->setup_thread);
    }
    
    mutex_destroy(&plc->mutex);

    /* unregister and close the socket. */
    session_unregister_unsafe(plc);

    /* remove any remaining requests, they are dead */
    req = plc->requests;

    while(req) {
        plc_remove_request_unsafe(plc, req);
        req = plc->requests;
    }

    pdebug(DEBUG_INFO, "Done.");

    return;
}


int session_register(ab_plc_p plc)
{
    eip_session_reg_req* req;
    eip_encap_t* resp;
    int rc = PLCTAG_STATUS_OK;
    uint32_t data_size = 0;
    int64_t timeout_time;

    pdebug(DEBUG_INFO, "Starting.");

    /*
     * clear the PLC data buffer.
     *
     * We use the receiving buffer because we do not have a request and nothing can
     * be coming in (we hope) on the socket yet.
     */
    mem_set(plc->recv_data, 0, sizeof(eip_session_reg_req));

    req = (eip_session_reg_req*)(plc->recv_data);

    /* fill in the fields of the request */
    req->encap_command = h2le16(AB_EIP_REGISTER_SESSION);
    req->encap_length = h2le16(sizeof(eip_session_reg_req) - sizeof(eip_encap_t));
    req->encap_session_handle = plc->session_handle;
    req->encap_status = h2le32(0);
    req->encap_sender_context = (uint64_t)0;
    req->encap_options = h2le32(0);

    req->eip_version = h2le16(AB_EIP_VERSION);
    req->option_flags = 0;

    /*
     * socket ops here are _ASYNCHRONOUS_!
     *
     * This is done this way because we do not have everything
     * set up for a request to be handled by the thread.  I think.
     */

    /* send registration to the gateway */
    data_size = sizeof(eip_session_reg_req);
    plc->recv_offset = 0;
    timeout_time = time_ms() + SESSION_REGISTRATION_TIMEOUT;

    pdebug(DEBUG_INFO, "sending data:");
    pdebug_dump_bytes(DEBUG_INFO, plc->recv_data, data_size);

    while (timeout_time > time_ms() && plc->recv_offset < data_size) {
        rc = socket_write(plc->sock, plc->recv_data + plc->recv_offset, data_size - plc->recv_offset);

        if (rc < 0) {
            pdebug(DEBUG_WARN, "Unable to send session registration packet! rc=%d", rc);
            plc->recv_offset = 0;
            return rc;
        }

        plc->recv_offset += rc;

        /* don't hog the CPU */
        if (plc->recv_offset < data_size) {
            sleep_ms(1);
        }
    }

    if (plc->recv_offset != data_size) {
        plc->recv_offset = 0;
        return PLCTAG_ERR_TIMEOUT;
    }

    /* get the response from the gateway */

    /* ready the input buffer */
    plc->recv_offset = 0;
    mem_set(plc->recv_data, 0, MAX_REQ_RESP_SIZE);

    timeout_time = time_ms() + SESSION_REGISTRATION_TIMEOUT;

    while (timeout_time > time_ms()) {
        if (plc->recv_offset < sizeof(eip_encap_t)) {
            data_size = sizeof(eip_encap_t);
        } else {
            data_size = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(plc->recv_data))->encap_length);
        }

        if (plc->recv_offset < data_size) {
            rc = socket_read(plc->sock, plc->recv_data + plc->recv_offset, data_size - plc->recv_offset);

            if (rc < 0) {
                if (rc != PLCTAG_ERR_NO_DATA) {
                    /* error! */
                    pdebug(DEBUG_WARN, "Error reading socket! rc=%d", rc);
                    return rc;
                }
            } else {
                plc->recv_offset += rc;

                /* recalculate the amount of data needed if we have just completed the read of an encap header */
                if (plc->recv_offset >= sizeof(eip_encap_t)) {
                    data_size = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(plc->recv_data))->encap_length);
                }
            }
        }

        /* did we get all the data? */
        if (plc->recv_offset == data_size) {
            break;
        } else {
            /* do not hog the CPU */
            sleep_ms(1);
        }
    }

    if (plc->recv_offset != data_size) {
        plc->recv_offset = 0;
        return PLCTAG_ERR_TIMEOUT;
    }

    /* set the offset back to zero for the next packet */
    plc->recv_offset = 0;

    pdebug(DEBUG_INFO, "received response:");
    pdebug_dump_bytes(DEBUG_INFO, plc->recv_data, data_size);

    /* encap header is at the start of the buffer */
    resp = (eip_encap_t*)(plc->recv_data);

    /* check the response status */
    if (le2h16(resp->encap_command) != AB_EIP_REGISTER_SESSION) {
        pdebug(DEBUG_WARN, "EIP unexpected response packet type: %d!", resp->encap_command);
        return PLCTAG_ERR_BAD_DATA;
    }

    if (le2h16(resp->encap_status) != AB_EIP_OK) {
        pdebug(DEBUG_WARN, "EIP command failed, response code: %d", resp->encap_status);
        return PLCTAG_ERR_REMOTE_ERR;
    }

    /*
     * after all that, save the session handle, we will
     * use it in future packets.
     */
    plc->session_handle = resp->encap_session_handle; /* opaque to us */
    plc->registered = 1;

    pdebug(DEBUG_INFO, "Done.");

    return PLCTAG_STATUS_OK;
}

int session_unregister_unsafe(ab_plc_p plc)
{
    if (plc->sock) {
        plc->is_connected = 0;
        plc->registered = 0;
        socket_close(plc->sock);
        socket_destroy(&(plc->sock));
        plc->sock = NULL;
    }

    return PLCTAG_STATUS_OK;
}



/*
 * plc_add_request_unsafe
 *
 * You must hold the mutex before calling this!
 */
int plc_add_request_unsafe(ab_plc_p plc, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p cur, prev;
    int total_requests = 0;

    pdebug(DEBUG_INFO, "Starting.");

    if(!plc) {
        pdebug(DEBUG_WARN, "Session is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* make sure the request points to the session */
    req->plc = plc;

    /* we add the request to the end of the list. */
    cur = plc->requests;
    prev = NULL;

    while (cur) {
        prev = cur;
        cur = cur->next;
        total_requests++;
    }

    if (!prev) {
        plc->requests = req;
    } else {
        prev->next = req;
    }

    /* update the request's refcount as we point to it. */
    rc_inc(req);

    pdebug(DEBUG_INFO,"Total requests in the queue: %d",total_requests);

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}

/*
 * plc_add_request
 *
 * This is a thread-safe version of the above routine.
 */
int plc_add_request(ab_plc_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting. sess=%p, req=%p", sess, req);

    critical_block(sess->mutex) {
        rc = plc_add_request_unsafe(sess, req);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


/*
 * plc_remove_request_unsafe
 *
 * You must hold the mutex before calling this!
 */
int plc_remove_request_unsafe(ab_plc_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p cur, prev;

    pdebug(DEBUG_DETAIL, "Starting.");

    if(sess == NULL || req == NULL) {
        return rc;
    }

    /* find the request and remove it from the list. */
    cur = sess->requests;
    prev = NULL;

    while (cur && cur != req) {
        prev = cur;
        cur = cur->next;
    }

    if (cur == req) {
        if (!prev) {
            sess->requests = cur->next;
        } else {
            prev->next = cur->next;
        }
    } /* else not found */

    req->next = NULL;
    req->plc = NULL;

    /* release the request refcount */
    rc_dec(req);

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}



/*
 * plc_remove_request
 *
 * This is a thread-safe version of the above routine.
 */
int plc_remove_request(ab_plc_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.  PLC=%p, req=%p", sess, req);

    if(sess == NULL || req == NULL) {
        return rc;
    }

    critical_block(sess->mutex) {
        rc = plc_remove_request_unsafe(sess, req);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


/* this must be called with the PLC mutex held */
int send_eip_request_unsafe(ab_request_p req)
{
    int rc;

    pdebug(DEBUG_DETAIL, "Starting.");

    if(!req) {
        pdebug(DEBUG_WARN,"Called with null request!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* if we have not already started, then start the send */
    if (!req->send_in_progress) {
        eip_encap_t* encap = (eip_encap_t*)(req->data);
        int payload_size = req->request_size - sizeof(eip_encap_t);

        /* set up the session sequence ID for this transaction */
        if(encap->encap_command == h2le16(AB_EIP_READ_RR_DATA)) {
            /* get new ID */
            req->plc->session_seq_id++;

            req->session_seq_id = req->plc->session_seq_id;
            encap->encap_sender_context = req->plc->session_seq_id; /* link up the request seq ID and the packet seq ID */

            /* mark the session as being used if this is a serialized packet */
            //~ mark_session_for_request(req);

            pdebug(DEBUG_INFO,"Sending unconnected packet with session sequence ID %llx",req->plc->session_seq_id);
        } else {
            eip_cip_co_req *conn_req = (eip_cip_co_req*)(req->data);

            /* set up the connection information */
            conn_req->cpf_targ_conn_id = h2le32(connection_targ_id(req->connection));
            req->conn_id = connection_orig_id(req->connection);

            //req->connection->conn_seq_num++;
            req->conn_seq = connection_next_seq(req->connection);
            conn_req->cpf_conn_seq_num = h2le16(req->conn_seq);
            

            /* mark the connection as being used. */
            //~ mark_connection_for_request(req);

            pdebug(DEBUG_INFO,"Sending connected packet with connection ID %x and sequence ID %u(%x)",req->conn_id, req->conn_seq, req->conn_seq);
        }

        /* set up the rest of the request */
        req->current_offset = 0; /* nothing written yet */

        /* fill in the header fields. */
        encap->encap_length = h2le16(payload_size);
        encap->encap_session_handle = req->plc->session_handle;
        encap->encap_status = h2le32(0);
        encap->encap_options = h2le32(0);

        /* display the data */
        pdebug(DEBUG_INFO,"Sending packet of size %d",req->request_size);
        pdebug_dump_bytes(DEBUG_INFO, req->data, req->request_size);

        req->send_in_progress = 1;
    }

    /* send the packet */
    rc = socket_write(req->plc->sock, req->data + req->current_offset, req->request_size - req->current_offset);

    if (rc >= 0) {
        req->current_offset += rc;

        /* are we done? */
        if (req->current_offset >= req->request_size) {
            req->send_request = 0;
            req->send_in_progress = 0;
            req->current_offset = 0;

            req->time_sent = time_ms();
            req->send_count++;

            /* set this request up for a receive action */
            if(req->abort_after_send) {
                req->abort_request = 1; /* for one shots */
            } else {
                req->recv_in_progress = 1;
            }
        }

        rc = PLCTAG_STATUS_OK;
    } else {
        /* oops, error of some sort. */
        req->status = rc;
        req->send_request = 0;
        req->send_in_progress = 0;
        req->recv_in_progress = 0;
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}

/*
 * recv_eip_response
 *
 * Look at the passed PLC and read any data we can
 * to fill in a packet.  If we already have a full packet,
 * punt.
 */
int recv_eip_response_unsafe(ab_plc_p plc)
{
    uint32_t data_needed = 0;
    int rc = PLCTAG_STATUS_OK;

    /* skip the rest if we already have a packet waiting in the PLC buffer. */
    if(plc->has_response) {
        return PLCTAG_STATUS_OK;
    }

    /*pdebug(DEBUG_DETAIL,"Starting.");*/

    /*
     * Determine the amount of data to get.  At a minimum, we
     * need to get an encap header.  This will determine
     * whether we need to get more data or not.
     */
    data_needed = (plc->recv_offset < sizeof(eip_encap_t)) ?
                                            sizeof(eip_encap_t) :
                                            sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(plc->recv_data))->encap_length);

    if (plc->recv_offset < data_needed) {
        /* read everything we can */
        do {
            rc = socket_read(plc->sock, plc->recv_data + plc->recv_offset,
                             data_needed - plc->recv_offset);

            /*pdebug(DEBUG_DETAIL,"socket_read rc=%d",rc);*/

            if (rc < 0) {
                if (rc != PLCTAG_ERR_NO_DATA) {
                    /* error! */
                    pdebug(DEBUG_WARN,"Error reading socket! rc=%d",rc);
                    return rc;
                }
            } else {
                plc->recv_offset += rc;

                /*pdebug_dump_bytes(session->debug, session->recv_data, session->recv_offset);*/

                /* recalculate the amount of data needed if we have just completed the read of an encap header */
                if (plc->recv_offset >= sizeof(eip_encap_t)) {
                    data_needed = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(plc->recv_data))->encap_length);
                }
            }
        } while (rc > 0 && plc->recv_offset < data_needed);
    }

    /* did we get all the data? */
    if (plc->recv_offset >= data_needed) {
        plc->resp_seq_id = ((eip_encap_t*)(plc->recv_data))->encap_sender_context;
        plc->has_response = 1;

        rc = PLCTAG_STATUS_OK;

        pdebug(DEBUG_DETAIL, "request received all needed data.");

        if(((eip_encap_t*)(plc->recv_data))->encap_command == h2le16(AB_EIP_READ_RR_DATA)) {
            eip_encap_t *encap = (eip_encap_t*)(plc->recv_data);
            pdebug(DEBUG_INFO,"Received unconnected packet with session sequence ID %llx",encap->encap_sender_context);
        } else {
            eip_cip_co_resp *resp = (eip_cip_co_resp*)(plc->recv_data);
            pdebug(DEBUG_INFO,"Received connected packet with connection ID %x and sequence ID %u(%x)",le2h32(resp->cpf_orig_conn_id), le2h16(resp->cpf_conn_seq_num), le2h16(resp->cpf_conn_seq_num));
        }
    }

    return rc;
}



/*******************************************************************************
 ************************** PLC Library Functions ******************************
 ******************************************************************************/
 
 
int plc_setup()
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting.");
    
    /* this is a mutex used to synchronize most activities in this protocol */
    rc = mutex_create((mutex_p*)&global_plc_mut);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create PLC mutex!");
        return rc;
    }

//    /* create the background IO handler thread */
//    rc = thread_create((thread_p*)&io_handler_thread, request_handler_func, 32*1024, NULL);
//
//    if(rc != PLCTAG_STATUS_OK) {
//        pdebug(DEBUG_INFO,"Unable to create request handler thread!");
//        return rc;
//    }

    pdebug(DEBUG_INFO,"Done.");

    return rc;
}


void plc_teardown()
{
    pdebug(DEBUG_INFO,"Terminating IO thread.");

//    /* wait for the thread to die */
//    thread_join(io_handler_thread);
//    thread_destroy((thread_p*)&io_handler_thread);

    pdebug(DEBUG_INFO,"Freeing global PLC mutex.");
    /* clean up the mutex */
    mutex_destroy((mutex_p*)&global_plc_mut);

    pdebug(DEBUG_INFO,"Done.");
}




/*******************************************************************************
 **************************** PLC Helper Functions *****************************
 ******************************************************************************/

 


///*
// * setup_session_mutex
// *
// * check to see if the global mutex is set up.  If not, do an atomic
// * lock and set it up.
// */
//int setup_session_mutex(void)
//{
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    critical_block(global_library_mutex) {
//        /* first see if the mutex is there. */
//        if (!global_session_mut) {
//            rc = mutex_create((mutex_p*)&global_session_mut);
//
//            if (rc != PLCTAG_STATUS_OK) {
//                pdebug(DEBUG_ERROR, "Unable to create global tag mutex!");
//            }
//        }
//    }
//
//    pdebug(DEBUG_INFO, "Done.");
//
//    return rc;
//}
//

static int match_request_and_response(ab_request_p request, eip_cip_co_resp *response)
{
    int connected_response = (response->encap_command == le2h16(AB_EIP_CONNECTED_SEND) ? 1 : 0);

    /*
     * AB decided not to use the 64-bit sender context in connected messages.  No idea
     * why they did this, but it means that we need to look at the connection details
     * instead.
     */
    if(connected_response && request->conn_id == le2h32(response->cpf_orig_conn_id) && request->conn_seq == le2h16(response->cpf_conn_seq_num)) {
        /* if it is a connected packet, match the connection ID and sequence num. */
        return 1;
    } else if(!connected_response && response->encap_sender_context != (uint64_t)0 && response->encap_sender_context == request->session_seq_id) {
        /* if it is not connected, match the sender context, note that this is sent in host order. */
        return 1;
    }

    /* no match */
    return 0;
}



static void update_resend_samples(ab_plc_p plc, int64_t round_trip_time)
{
    int index;
    int64_t round_trip_sum = 0;
    int64_t round_trip_avg = 0;

    /* store the round trip information */
    plc->round_trip_sample_index = ((++ plc->round_trip_sample_index) >= SESSION_NUM_ROUND_TRIP_SAMPLES ? 0 : plc->round_trip_sample_index);
    plc->round_trip_samples[plc->round_trip_sample_index] = round_trip_time;

    /* calculate the retry interval */
    for(index=0; index < SESSION_NUM_ROUND_TRIP_SAMPLES; index++) {
        round_trip_sum += plc->round_trip_samples[index];
    }

    /* round up and double */
    round_trip_avg = (round_trip_sum + (SESSION_NUM_ROUND_TRIP_SAMPLES/2))/SESSION_NUM_ROUND_TRIP_SAMPLES;
    plc->retry_interval = 3 * (round_trip_avg < SESSION_MIN_RESEND_INTERVAL ? SESSION_MIN_RESEND_INTERVAL : round_trip_avg);

    pdebug(DEBUG_INFO,"Packet round trip time %lld, running average round trip time is %lld",round_trip_time, plc->retry_interval);
}



static void receive_response_unsafe(ab_plc_p plc, ab_request_p request)
{
    /*
     * We received a packet.  Modify the packet interval downword slightly
     * to get to the maximum value.  We want to get to the point where we lose
     * a packet once in a while.
     */

    /*plc->next_packet_interval_us -= SESSION_PACKET_RECEIVE_INTERVAL_DEC;
    if(plc->next_packet_interval_us < SESSION_MIN_PACKET_INTERVAL) {
        plc->next_packet_interval_us = SESSION_MIN_PACKET_INTERVAL;
    }
    pdebug(DEBUG_INFO,"Packet received, so decreasing packet interval to %lldus", plc->next_packet_interval_us);
    */

    pdebug(DEBUG_INFO,"Packet sent initially %dms ago and was sent %d times",(int)(time_ms() - request->time_sent), request->send_count);

    update_resend_samples(plc, time_ms() - request->time_sent);

    /* set the packet ready for processing. */
    pdebug(DEBUG_INFO, "got full packet of size %d", plc->recv_offset);
    pdebug_dump_bytes(DEBUG_INFO, plc->recv_data, plc->recv_offset);

    /* copy the data from the PLC's buffer */
    mem_copy(request->data, plc->recv_data, plc->recv_offset);
    request->request_size = plc->recv_offset;

    request->resp_received = 1;
    request->send_in_progress = 0;
    request->send_request = 0;
    request->recv_in_progress = 0;

    /* clear the request from the PLC as it is done. Note we hold the mutex here. */
    plc_remove_request_unsafe(plc, request);
}




int ok_to_resend(ab_plc_p plc, ab_request_p request)
{
    if(!plc) {
        return 0;
    }

    if(!request) {
        return 0;
    }

    /* was it already sent? */
    if(!request->recv_in_progress) {
        return 0;
    }

    /* did it have a response? */
    if(request->resp_received) {
        return 0;
    }

    /* does it want a resend? */
    if(request->no_resend) {
        return 0;
    }

    /* was the request aborted or should it be aborted? */
    if(request->abort_request || request->abort_after_send) {
        return 0;
    }

    /* have we waited enough time to resend? */
    if((request->time_sent + request->retry_interval) > time_ms()) {
        return 0;
    }

    if(request->num_retries_left <= 0) {
        return 0;
    }

    pdebug(DEBUG_INFO,"Request waited %lldms, and has %d retries left need to resend.",(time_ms() - request->time_sent), request->num_retries_left);

    /* track how many times we've retried. */
    request->num_retries_left--;

    return 1;
}


int process_response_packet_unsafe(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;
    eip_cip_co_resp *response = (eip_cip_co_resp*)(&plc->recv_data[0]);
    ab_request_p request = plc->requests;

    /* find the request for which there is a response pending. */
    while(request) {
        /* need to get the next request now because we might be removing it in receive_response_unsafe */
        ab_request_p next_req = request->next;

        if(match_request_and_response(request, response)) {
            receive_response_unsafe(plc, request);
        }

        request = next_req;
    }

    return rc;
}


int plc_check_incoming_data(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_SPEW,"Starting.");
    
    /* take the mutex for the PLC to prevent anything from changing. */
    
    /* FIXME - is this needed here? */
    
    critical_block(plc->mutex) {
        /*
         * check for data.
         *
         * Read a packet into the PLC's buffer and find the
         * request it is for.  Repeat while there is data.
         */

        do {
            rc = recv_eip_response_unsafe(plc);

            /* did we get a packet? */
            if(rc == PLCTAG_STATUS_OK && plc->has_response) {
                rc = process_response_packet_unsafe(plc);

                /* reset the PLC's buffer */
                mem_set(plc->recv_data, 0, MAX_REQ_RESP_SIZE);
                plc->recv_offset = 0;
                plc->resp_seq_id = 0;
                plc->has_response = 0;
            }
        } while(rc == PLCTAG_STATUS_OK);

        /* No data is not an error */
        if(rc == PLCTAG_ERR_NO_DATA) {
            rc = PLCTAG_STATUS_OK;
        }

        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Error while trying to receive a response packet.  rc=%d",rc);
        }
    }

    pdebug(DEBUG_SPEW,"Done.");
    
    return rc;
}



static int plc_send_current_request(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;

    if(!plc->current_request) {
        return PLCTAG_STATUS_OK;
    }

    rc = send_eip_request_unsafe(plc->current_request);

    if(rc != PLCTAG_STATUS_OK) {
        /*error sending packet!*/
        return rc;
    }

    /* if we are done, then clean up */
    if(!plc->current_request->send_in_progress) {
        /* release the refcount on the request, we are not referencing it anymore */
        plc->current_request = rc_dec(plc->current_request);
    }

    return rc;
}


static int plc_check_outgoing_data(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p request = plc->requests;
    int connected_requests_in_flight = 0;
    int unconnected_requests_in_flight = 0;
    
    pdebug(DEBUG_SPEW,"Starting.");
    
    critical_block(plc->mutex) {
        /* loop over the requests and process them one at a time. */
        while(request && rc == PLCTAG_STATUS_OK) {
            if(request->abort_request) {
                ab_request_p old_request = request;

                /* skip to the next one */
                request = request->next;

                //~ rc = handle_abort_request(old_request);
                rc = plc_remove_request_unsafe(plc,old_request);

                continue;
            }

            /* check resending */
            if(ok_to_resend(plc, request)) {
                //~ handle_resend(session, request);
                if(request->connected_request) {
                    pdebug(DEBUG_INFO,"Requeuing connected request.");
                } else {
                    pdebug(DEBUG_INFO,"Requeuing unconnected request.");
                }

                request->recv_in_progress = 0;
                request->send_request = 1;
            }

            /* count requests in flight */
            if(request->recv_in_progress) {
                if(request->connected_request) {
                    connected_requests_in_flight++;
                    pdebug(DEBUG_SPEW,"%d connected requests in flight.", connected_requests_in_flight);
                } else {
                    unconnected_requests_in_flight++;
                    pdebug(DEBUG_SPEW,"%d unconnected requests in flight.", unconnected_requests_in_flight);
                }
            }


            /* is there a request ready to send and can we send? */
            if(!plc->current_request && request->send_request) {
                if(request->connected_request) {
                    if(connected_requests_in_flight < SESSION_MAX_CONNECTED_REQUESTS_IN_FLIGHT) {
                        pdebug(DEBUG_INFO,"Readying connected packet to send.");

                        /* increment the refcount since we are storing a pointer to the request */
                        plc->current_request = rc_inc(request);

                        connected_requests_in_flight++;

                        pdebug(DEBUG_INFO,"sending packet, so %d connected requests in flight.", connected_requests_in_flight);
                    }
                } else {
                    if(unconnected_requests_in_flight < SESSION_MAX_UNCONNECTED_REQUESTS_IN_FLIGHT) {
                        pdebug(DEBUG_INFO,"Readying unconnected packet to send.");

                        /* increment the refcount since we are storing a pointer to the request */
                        plc->current_request = rc_inc(request);

                        unconnected_requests_in_flight++;

                        pdebug(DEBUG_INFO,"sending packet, so %d unconnected requests in flight.", unconnected_requests_in_flight);
                    }
                }
            }

            /* call this often to make sure we get the data out. */
            rc = plc_send_current_request(plc);

            /* get the next request to process */
            request = request->next;
        }
    }
    
    pdebug(DEBUG_SPEW,"Done.");
    
    return rc;
}


void plc_handler(ab_plc_p plc)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_SPEW, "Checking for things to do with PLC %p", plc);
    
    /* fake exceptions */
    do {
        /* do we need to initialize the session? */
        if(!plc->registered) {
            if(!plc->setup_thread) {
                pdebug(DEBUG_INFO,"Starting thread to set up session.");
                /* kick off a thread to handle the blocking parts of the session creation. */
                plc->status = PLCTAG_STATUS_PENDING;
                
                rc = thread_create(&plc->setup_thread,plc_init,32*1024, (void*)plc);
                if(rc != PLCTAG_STATUS_OK) {
                    pdebug(DEBUG_WARN,"Unable to start background PLC initialization thread! rc= %d", rc);
                    plc->status = rc;
                } else {
                    pdebug(DEBUG_INFO, "PLC not registered, kicked off registration thread.");
                }
                
                break;
            } 
        } 
        
        
        /* punt if we are still setting up the PLC. */
        if(plc->status == PLCTAG_STATUS_PENDING) {
            break;
        }
        
        if(plc->setup_thread) {
            /* the thread is done, clean up. */
            thread_join(plc->setup_thread);
            thread_destroy(&plc->setup_thread);
        }
        
        /* punt if we do not have a good PLC. */
        if(plc->status != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_SPEW, "Bad PLC set up.");
            
            /*
             * FIXME - should do some sort of retry here.   For instance if the Ethernet cable
             * is unplugged, this should recover.
             */
             
            break;
        }

        /*
         * FIXME - check to see if we need to hold the mutex here.  
         * The old code did?
         */
        
        rc = plc_check_incoming_data(plc);
        if (rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN, "Error when checking for incoming PLC data! %d", rc);
            /* FIXME - do something useful with this error */
        }

        /* check for incoming data. */
        rc = plc_check_outgoing_data(plc);
        if (rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN, "Error when checking for outgoing PLC data! %d", rc);
            /* FIXME - do something useful with this error */
        }
    } while(0);
}


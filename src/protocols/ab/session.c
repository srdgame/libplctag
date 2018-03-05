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
#include <ab/session.h>
#include <ab/connection.h>
#include <ab/request.h>
#include <ab/eip.h>
#include <util/debug.h>
#include <util/mem.h>
#include <util/refcount.h>
#include <stdlib.h>
#include <time.h>


typedef enum { PLC_START, PLC_OPEN_SOCKET, PLC_WAIT_SOCKET, PLC_OPEN_SESSION, PLC_WAIT_SESSION, PLC_CLOSE_SESSION, PLC_OPEN_CONNECTION, PLC_WAIT_CONNECTION, PLC_CLOSE_CONNECTION, PLC_WAIT_CLOSE_CONNECTION, PLC_PROCESS_REQUEST} plc_state_t;


struct ab_session_t {
    /* required for live objects */
    struct liveobj_t liveobj;
    int liveobj_id;
    
    int registered;
    
    plc_state_t state;

    thread_p setup_thread;
    
    attr creation_attributes;
    
    mutex_p mutex;
    
    /* gateway connection related info */
    char host[MAX_SESSION_HOST];
    int port;
    sock_p sock;
    int is_connected;
    int status;
    int init_status;

    /* registration info */
    uint32_t session_handle;

    /* Sequence ID for requests. */
    uint64_t session_seq_id;

    /* current request being sent, only one at a time */
    ab_request_p current_request;

    /* list of outstanding requests for this session */
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

    /* connection info for this session */
    //ab_connection_p connection;
    uint32_t conn_serial_number; /* id for the next connection */
    uint32_t targ_connection_id; /* the ID the target uses for this connection. */
    uint32_t orig_connection_id; /* the ID we use for this connection */
    uint16_t conn_seq_num;
    uint8_t conn_path[MAX_CONN_PATH];
    uint8_t conn_path_size;

    
};



/*
 * Externally visible global variables
 */
mutex_p global_session_mut = NULL;


/* local variables. */



static ab_session_p session_create_unsafe(attr attribs);
static THREAD_FUNC(session_init);
static int add_session_unsafe(ab_session_p n);
static int find_session_by_host_unsafe(liveobj_p obj, int type, void *host_arg);
static int session_connect(ab_session_p session);
static void session_destroy(rc_ptr session);
static int session_register(ab_session_p session);
static int session_unregister_unsafe(ab_session_p session);

//static int session_remove_connection_unsafe(ab_session_p session, ab_connection_p connection);
static int session_remove_request_unsafe(ab_session_p session, ab_request_p req);

static void session_handler(ab_session_p session);
static int session_check_incoming_data(ab_session_p session);
static int session_check_outgoing_data(ab_session_p session);
static int queue_forward_open_req(ab_session_p session);
static int open_connection(ab_session_p session);
/*
 * session_get_new_seq_id_unsafe
 *
 * A wrapper to get a new session sequence ID.  Not thread safe.
 */

uint64_t session_get_new_seq_id_unsafe(ab_session_p sess)
{
    return sess->session_seq_id++;
}

/*
 * session_get_new_seq_id
 *
 * A thread-safe function to get a new session sequence ID.
 */

uint64_t session_get_new_seq_id(ab_session_p sess)
{
    uint16_t res = 0;

    //pdebug(DEBUG_DETAIL, "entering critical block %p",global_session_mut);
    critical_block(sess->mutex) {
        res = (uint16_t)session_get_new_seq_id_unsafe(sess);
    }
    //pdebug(DEBUG_DETAIL, "leaving critical block %p", global_session_mut);

    return res;
}


static int connection_is_usable_unsafe(ab_connection_p connection)
{
    if(!connection) {
        return 0;
    }

//    if(connection->exclusive) {
//        return 0;
//    }
//
//    if(connection->disconnect_in_progress) {
//        return 0;
//    }

    return 1;
}


//
//ab_connection_p session_find_connection_by_path_unsafe(ab_session_p session,const char *path)
//{
//    ab_connection_p connection;
//    
//    (void)path;
//
//    connection = session->connection;
//
//    /*
//     * there are a lot of conditions.
//     * We do not want to use connections that are in the process of shutting down.
//     * We do not want to use connections that are used exclusively by one tag.
//     * We want to use connections that have the same path as the tag.
//     */
////    while (connection && !connection_is_usable_unsafe(connection) && str_cmp_i(connection_path(connection), path) != 0) {
////        connection = connection->next;
////    }
//
//
////    /* add to the ref count since we found an existing one. */
////    if(connection) {
////        rc_inc(connection);
////    }
//
//    return rc_inc(connection);
//}
//
//
//
//int session_add_connection_unsafe(ab_session_p session, ab_connection_p connection)
//{
//    pdebug(DEBUG_DETAIL, "Starting");
//
//    /* add the connection to the list in the session */
////    connection->next = session->connections;
////    session->connections = connection;
//
//    if(session->connection) {
//        pdebug(DEBUG_WARN, "A connection already exists!");
//    } else {
//        session->connection = connection;
//    }
//
//    pdebug(DEBUG_DETAIL, "Done");
//
//    return PLCTAG_STATUS_OK;
//}
//
//
//
//
//int session_add_connection(ab_session_p session, ab_connection_p connection)
//{
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_DETAIL, "Starting");
//
//    if(session) {
//        critical_block(session->mutex) {
//            rc = session_add_connection_unsafe(session, connection);
//        }
//    } else {
//        pdebug(DEBUG_WARN, "Session ptr is null!");
//        rc = PLCTAG_ERR_NULL_PTR;
//    }
//
//    pdebug(DEBUG_DETAIL, "Done");
//
//    return rc;
//}
//
///* must have the session mutex held here. */
//int session_remove_connection_unsafe(ab_session_p session, ab_connection_p connection)
//{
////    ab_connection_p cur;
////    ab_connection_p prev;
//    /* int debug = session->debug; */
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_DETAIL, "Starting");
//    
//    if(session->connection != connection) {
//        pdebug(DEBUG_WARN,"Wrong connection!");
//        return PLCTAG_ERR_NOT_FOUND;
//    }
//    
//    session->connection = NULL;
//
////    cur = session->connections;
////    prev = NULL;
////
////    while (cur && cur != connection) {
////        prev = cur;
////        cur = cur->next;
////    }
////
////    if (cur && cur == connection) {
////        if (prev) {
////            prev->next = cur->next;
////        } else {
////            session->connections = cur->next;
////        }
////
////        rc = PLCTAG_STATUS_OK;
////    } else {
////        rc = PLCTAG_ERR_NOT_FOUND;
////    }
//
//    pdebug(DEBUG_DETAIL, "Done");
//
//    return rc;
//}
//
//int session_remove_connection(ab_session_p session, ab_connection_p connection)
//{
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_DETAIL, "Starting");
//
//    if(session) {
//        critical_block(session->mutex) {
//            rc = session_remove_connection_unsafe(session, connection);
//        }
//    } else {
//        rc = PLCTAG_ERR_NULL_PTR;
//    }
//
//    pdebug(DEBUG_DETAIL, "Done");
//
//    return rc;
//}
//

uint32_t session_get_new_connection_id_unsafe(ab_session_p session)
{
    ++(session->conn_serial_number);
    
    return session->conn_serial_number;
}


uint32_t session_get_new_connection_id(ab_session_p session)
{
    uint32_t result = 0;
    
    critical_block(session->mutex) {
        result = session_get_new_connection_id_unsafe(session);
    }
    
    return result;
}


int session_status(ab_session_p session)
{
    int result = PLCTAG_STATUS_OK;
    
    if(!session) {
        return PLCTAG_ERR_NULL_PTR;
    }
    
    critical_block(session->mutex) {
        result = session->status;
    }
    
    return result;
}


ab_session_p session_find_or_create(attr attribs)
{
    /*int debug = attr_get_int(attribs,"debug",0);*/
    const char* session_gw = attr_get_str(attribs, "gateway", "");
    int session_gw_port = attr_get_int(attribs, "gateway_port", AB_EIP_DEFAULT_PORT);
    ab_session_p session = AB_SESSION_NULL;
    //int new_session = 0;
    int shared_session = attr_get_int(attribs, "share_session", 1); /* share the session by default. */
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");
    


    critical_block(global_session_mut) {
        /* if we are to share sessions, then look for an existing one. */
        if (shared_session) {
            /* this returns a strong reference */
            session = (ab_session_p)liveobj_find(find_session_by_host_unsafe, (void *)session_gw);
        } else {
            /* no sharing, create a new one */
            session = AB_SESSION_NULL;
        }

        if (session == AB_SESSION_NULL) {
            pdebug(DEBUG_DETAIL,"Creating new session.");
            /*
             * This does not completely set up the session.   We go through an initial phase first via the
             * session handler function.
             */
            session = session_create_unsafe(attribs);

            if (session == AB_SESSION_NULL) {
                pdebug(DEBUG_WARN, "unable to create or find a session!");
                rc = PLCTAG_ERR_BAD_GATEWAY;
            } else {
                /* queue the initial ForwardOpen */
                session->state = PLC_START;
                
                rc = queue_forward_open_req(session);
                if(rc != PLCTAG_STATUS_OK) {
                    pdebug(DEBUG_WARN, "Initial queue of ForwardOpen failed!");
                }
            }
        } else {
            pdebug(DEBUG_DETAIL,"Reusing existing session.");
        }
    }

    /* store it into the tag */
    *tag_session = session;

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

int add_session_unsafe(ab_session_p session)
{
    pdebug(DEBUG_DETAIL, "Starting");

    session->liveobj_id = liveobj_add((liveobj_p)session, LIVEOBJ_TYPE_AB_PLC, (liveobj_func)session_handler);
    
    if(session->liveobj_id < 0) {
        /* error! */
        pdebug(DEBUG_WARN,"Unable to add session to live objects!");
    }

    pdebug(DEBUG_DETAIL, "Done");

    return session->liveobj_id;
}

int add_session(ab_session_p s)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.");

    critical_block(global_session_mut) {
        rc = add_session_unsafe(s);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


static int session_match_valid(const char *host, ab_session_p session)
{
    if(!session) {
        return 0;
    }

    if(session->status !=  PLCTAG_STATUS_OK && session->status != PLCTAG_STATUS_PENDING) {
        return 0;
    }

    if(str_cmp_i(host,session->host)) {
        return 0;
    }

    return 1;
}


int find_session_by_host_unsafe(liveobj_p obj, int type, void *host_arg)
{
    ab_session_p session;
    
    pdebug(DEBUG_SPEW,"Starting.");

    if(type == LIVEOBJ_TYPE_AB_SESSION) {
        session = (ab_session_p)obj;
        
        if(session_match_valid((const char*)host_arg, session)) {
            /* this is the right one! */
            pdebug(DEBUG_INFO,"Found session!");
            
            return PLCTAG_STATUS_OK;
        }
    }
    
    pdebug(DEBUG_SPEW,"Done.");
    
    return PLCTAG_ERR_NOT_FOUND;
}



ab_session_p session_create_unsafe(attr attribs)
{
    ab_session_p session = AB_SESSION_NULL;
    static volatile uint32_t srand_setup = 0;
    static volatile uint32_t connection_id = 0;
    //const char* plc_path = attr_get_str(attribs, "path", "");
    //int session_gw_port = attr_get_int(attribs, "gateway_port", AB_EIP_DEFAULT_PORT);
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting");

    //pdebug(DEBUG_DETAIL, "Warning: not using passed port %d", session_gw_port);

    session = rc_alloc(sizeof(struct ab_session_t), session_destroy);

    if (!session) {
        pdebug(DEBUG_WARN, "Error allocating new session.");
        return AB_SESSION_NULL;
    }
    
    /* set the function to handle this object */
    //session->session_obj_func = (liveobj_func)session_handler;

    //str_copy(session->host, MAX_SESSION_HOST, session_gw);
    //session->host = str_dup(session_gw);

    session->state = PLC_START;
    session->status = PLCTAG_STATUS_PENDING;
    session->init_status = PLCTAG_STATUS_PENDING;
    session->creation_attributes = rc_inc(attribs);

    /* check for ID set up */
    if(srand_setup == 0) {
        srand((unsigned int)time_ms());
        srand_setup = 1;
    }

    if(connection_id == 0) {
        connection_id = rand();
    }

    session->session_seq_id =  rand();

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
    session->conn_serial_number = ++connection_id;

    /* set up packet round trip information */
    for(int index=0; index < SESSION_NUM_ROUND_TRIP_SAMPLES; index++) {
        session->round_trip_samples[index] = SESSION_DEFAULT_RESEND_INTERVAL_MS;
    }
    session->retry_interval = SESSION_DEFAULT_RESEND_INTERVAL_MS;

    /* add the new session to the list. */
    rc = add_session_unsafe(session);
    
    
    /* make sure we have a mutex */
    rc = mutex_create(&session->mutex);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to create session mutex! rc= %d", rc);
        session->status = rc;
    }

    pdebug(DEBUG_INFO, "Done");

    return session;
}




//int open_connection(ab_session_p session)
//{
//    ab_request_p req;
//    int64_t timeout_time;
//    int rc = PLCTAG_STATUS_OK;
//
//    pdebug(DEBUG_INFO, "Starting.");
//
//    /* get a request buffer */
//    rc = request_create(&req);
//
//    do {
//        if(rc != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to get new request.  rc=%d",rc);
//            rc = 0;
//            break;
//        }
//
//        /* send the ForwardOpen command to the PLC */
//        if((rc = send_forward_open_req(session, req)) != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to send ForwardOpen packet!");
//            break;
//        }
//
//        /* wait for a response */
//        timeout_time = time_ms() + CONNECTION_SETUP_TIMEOUT;
//
//        while (timeout_time > time_ms() && !req->resp_received) {
//            sleep_ms(1);
//        }
//
//        /* timeout? */
//        if(!req->resp_received) {
//            pdebug(DEBUG_WARN,"Timed out waiting for ForwardOpen response!");
//            rc = PLCTAG_ERR_TIMEOUT_ACK;
//            break;
//        }
//
//        /* check for the ForwardOpen response. */
//        if((rc = recv_forward_open_resp(session, req)) != PLCTAG_STATUS_OK) {
//            pdebug(DEBUG_WARN,"Unable to use ForwardOpen response!");
//            rc = PLCTAG_ERR_REMOTE_ERR;
//            break;
//        }
//    } while(0);
//
//    /* clean up the request */
//    if(req) {
//        req = rc_dec(req);
//    }
//
//    pdebug(DEBUG_INFO, "Done.");
//
//    return rc;
//}


int queue_forward_open_req(ab_session_p session)
{
    eip_forward_open_request_t *fo = NULL;
    uint8_t *data = NULL;
    int rc = PLCTAG_STATUS_OK;
    ab_request_p req = NULL;

    pdebug(DEBUG_INFO,"Starting");
    
    rc = request_create(&req);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Unable to allocate request!");
        return rc;
    }

    fo = (eip_forward_open_request_t*)(req->data);

    /* point to the end of the struct */
    data = (req->data) + sizeof(eip_forward_open_request_t);

    /* set up the path information. */
    mem_copy(data, session->conn_path, session->conn_path_size);
    data += session->conn_path_size;

    /* fill in the static parts */

    /* encap header parts */
    fo->encap_command = h2le16(AB_EIP_READ_RR_DATA); /* 0x006F EIP Send RR Data command */
    fo->encap_length = h2le16(data - (uint8_t*)(&fo->interface_handle)); /* total length of packet except for encap header */
    fo->router_timeout = h2le16(1);                       /* one second is enough ? */

    /* CPF parts */
    fo->cpf_item_count = h2le16(2);                  /* ALWAYS 2 */
    fo->cpf_nai_item_type = h2le16(AB_EIP_ITEM_NAI); /* null address item type */
    fo->cpf_nai_item_length = h2le16(0);             /* no data, zero length */
    fo->cpf_udi_item_type = h2le16(AB_EIP_ITEM_UDI); /* unconnected data item, 0x00B2 */
    fo->cpf_udi_item_length = h2le16(data - (uint8_t*)(&fo->cm_service_code)); /* length of remaining data in UC data item */

    /* Connection Manager parts */
    fo->cm_service_code = AB_EIP_CMD_FORWARD_OPEN; /* 0x54 Forward Open Request */
    fo->cm_req_path_size = 2;                      /* size of path in 16-bit words */
    fo->cm_req_path[0] = 0x20;                     /* class */
    fo->cm_req_path[1] = 0x06;                     /* CM class */
    fo->cm_req_path[2] = 0x24;                     /* instance */
    fo->cm_req_path[3] = 0x01;                     /* instance 1 */

    /* Forward Open Params */
    fo->secs_per_tick = AB_EIP_SECS_PER_TICK;         /* seconds per tick, no used? */
    fo->timeout_ticks = AB_EIP_TIMEOUT_TICKS;         /* timeout = srd_secs_per_tick * src_timeout_ticks, not used? */
    fo->orig_to_targ_conn_id = h2le32(0);             /* is this right?  Our connection id on the other machines? */
    fo->targ_to_orig_conn_id = h2le32(session->orig_connection_id); /* connection id in the other direction. */
    /* this might need to be globally unique */
    fo->conn_serial_number = h2le16((uint16_t)(intptr_t)(session)); /* our connection ??? number. */
    fo->orig_vendor_id = h2le16(AB_EIP_VENDOR_ID);               /* our unique :-) vendor ID */
    fo->orig_serial_number = h2le32(AB_EIP_VENDOR_SN);           /* our serial number. */
    fo->conn_timeout_multiplier = AB_EIP_TIMEOUT_MULTIPLIER;     /* timeout = mult * RPI */
    fo->orig_to_targ_rpi = h2le32(AB_EIP_RPI); /* us to target RPI - Request Packet Interval in microseconds */
    fo->orig_to_targ_conn_params = h2le16(session->conn_params); /* packet size and some other things, based on protocol/cpu type */
    fo->targ_to_orig_rpi = h2le32(AB_EIP_RPI); /* target to us RPI - not really used for explicit messages? */
    fo->targ_to_orig_conn_params = h2le16(session->conn_params); /* packet size and some other things, based on protocol/cpu type */
    fo->transport_class = AB_EIP_TRANSPORT_CLASS_T3; /* 0xA3, server transport, class 3, application trigger */
    fo->path_size = session->conn_path_size/2; /* size in 16-bit words */

    /* set the size of the request */
    req->request_size = data - (req->data);

    /* mark it as ready to send */
    req->send_request = 1;

    /*
     * make sure the session serializes this with respect to other
     * control packets.  Apparently, the connection manager has no
     * buffers.
     */
    req->connected_request = 1;
    req->no_resend = 1; /* do not resend this, leads to problems.*/

    /* note the request for later processing when a response is received. */
    session->fo_req = req;

    /* add the request to the session's list. */
    rc = session_add_request(session, req);
    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN,"Unable to queue request!");
        session->fo_req = NULL;
        rc_dec(req);
        return rc;
    }

    pdebug(DEBUG_INFO, "Done");

    return rc;
}


int recv_forward_open_resp(ab_session_p session, ab_request_p req)
{
    eip_forward_open_response_t *fo_resp = NULL;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO,"Starting");

    fo_resp = (eip_forward_open_response_t*)(req->data);

    do {
        if(le2h16(fo_resp->encap_command) != AB_EIP_READ_RR_DATA) {
            pdebug(DEBUG_WARN,"Unexpected EIP packet type received: %d!",fo_resp->encap_command);
            rc = PLCTAG_ERR_BAD_DATA;
            break;
        }

        if(le2h16(fo_resp->encap_status) != AB_EIP_OK) {
            pdebug(DEBUG_WARN,"EIP command failed, response code: %d",fo_resp->encap_status);
            rc = PLCTAG_ERR_REMOTE_ERR;
            break;
        }

        if(fo_resp->general_status != AB_EIP_OK) {
            pdebug(DEBUG_WARN,"Forward Open command failed, response code: %d",fo_resp->general_status);
            rc = PLCTAG_ERR_REMOTE_ERR;
            break;
        }

        session->targ_connection_id = le2h32(fo_resp->orig_to_targ_conn_id);
        session->orig_connection_id = le2h32(fo_resp->targ_to_orig_conn_id);
        session->is_connected = 1;
        session->status = PLCTAG_STATUS_OK; /* open the floodgates! */

        pdebug(DEBUG_INFO,"ForwardOpen succeeded with our connection ID %x and the PLC connection ID %x",session->orig_connection_id, session->targ_connection_id);

        rc = PLCTAG_STATUS_OK;
    } while(0);

    pdebug(DEBUG_INFO,"Done.");

    return rc;
}






/*
 * session_init
 *
 * This calls several blocking methods and so must not keep the main mutex
 * locked during them.
 */
THREAD_FUNC(session_init)
{
    ab_session_p session = (ab_session_p)arg;
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");
    
    
    do {
        /* we must connect to the gateway and register */
        rc = session_connect(session);
        if (rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN, "session connect failed!");
            break;
        }

        if ((rc = session_register(session)) != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN, "session registration failed!");
            break;
        }
    } while(0);
        
    /* record how we did. */
    session->init_status = rc;    
    
    pdebug(DEBUG_INFO, "Done.");

    THREAD_RETURN(0);
}



/*
 * session_connect()
 *
 * Connect to the host/port passed via TCP.
 */

int session_connect(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    /* Open a socket for communication with the gateway. */
    rc = socket_create(&(session->sock));

    if (rc) {
        pdebug(DEBUG_WARN, "Unable to create socket for session!");
        return 0;  /* FIXME this should be a PLCTAG_ERR_XXYZ value */
    }

    /* FIXME - this can hang! */
    rc = socket_connect_tcp(session->sock, session->host, AB_EIP_DEFAULT_PORT);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Unable to connect socket for session!");
        return rc;
    }

    /* everything is OK.  We have a TCP stream open to a gateway. */
    session->is_connected = 1;

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}


/* ref counted destructor */
void session_destroy(rc_ptr session_arg)
{
    ab_session_p session = session_arg;
    ab_request_p req = NULL;

    pdebug(DEBUG_INFO, "Starting.");

    if (!session) {
        pdebug(DEBUG_WARN, "Session ptr is null!");

        return;
    }
    
    /* remove the session from the list of live objects. */
    liveobj_remove(session->liveobj.id);
    
    /* not needed, liveobj already handles this. */
    //remove_session_unsafe(session);
    /* check to see if there is a thread. */
    if(session->setup_thread) {
        thread_join(session->setup_thread);
        thread_destroy(&session->setup_thread);
    }
    
    mutex_destroy(&session->mutex);

    /* unregister and close the socket. */
    session_unregister_unsafe(session);

    /* remove any remaining requests, they are dead */
    req = session->requests;

    while(req) {
        session_remove_request_unsafe(session, req);
        req = session->requests;
    }

    pdebug(DEBUG_INFO, "Done.");

    return;
}


int session_register(ab_session_p session)
{
    eip_session_reg_req* req;
    eip_encap_t* resp;
    int rc = PLCTAG_STATUS_OK;
    uint32_t data_size = 0;
    int64_t timeout_time;

    pdebug(DEBUG_INFO, "Starting.");

    /*
     * clear the session data.
     *
     * We use the receiving buffer because we do not have a request and nothing can
     * be coming in (we hope) on the socket yet.
     */
    mem_set(session->recv_data, 0, sizeof(eip_session_reg_req));

    req = (eip_session_reg_req*)(session->recv_data);

    /* fill in the fields of the request */
    req->encap_command = h2le16(AB_EIP_REGISTER_SESSION);
    req->encap_length = h2le16(sizeof(eip_session_reg_req) - sizeof(eip_encap_t));
    req->encap_session_handle = session->session_handle;
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
    session->recv_offset = 0;
    timeout_time = time_ms() + SESSION_REGISTRATION_TIMEOUT;

    pdebug(DEBUG_INFO, "sending data:");
    pdebug_dump_bytes(DEBUG_INFO, session->recv_data, data_size);

    while (timeout_time > time_ms() && session->recv_offset < data_size) {
        rc = socket_write(session->sock, session->recv_data + session->recv_offset, data_size - session->recv_offset);

        if (rc < 0) {
            pdebug(DEBUG_WARN, "Unable to send session registration packet! rc=%d", rc);
            session->recv_offset = 0;
            return rc;
        }

        session->recv_offset += rc;

        /* don't hog the CPU */
        if (session->recv_offset < data_size) {
            sleep_ms(1);
        }
    }

    if (session->recv_offset != data_size) {
        session->recv_offset = 0;
        return PLCTAG_ERR_TIMEOUT;
    }

    /* get the response from the gateway */

    /* ready the input buffer */
    session->recv_offset = 0;
    mem_set(session->recv_data, 0, MAX_REQ_RESP_SIZE);

    timeout_time = time_ms() + SESSION_REGISTRATION_TIMEOUT;

    while (timeout_time > time_ms()) {
        if (session->recv_offset < sizeof(eip_encap_t)) {
            data_size = sizeof(eip_encap_t);
        } else {
            data_size = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(session->recv_data))->encap_length);
        }

        if (session->recv_offset < data_size) {
            rc = socket_read(session->sock, session->recv_data + session->recv_offset, data_size - session->recv_offset);

            if (rc < 0) {
                if (rc != PLCTAG_ERR_NO_DATA) {
                    /* error! */
                    pdebug(DEBUG_WARN, "Error reading socket! rc=%d", rc);
                    return rc;
                }
            } else {
                session->recv_offset += rc;

                /* recalculate the amount of data needed if we have just completed the read of an encap header */
                if (session->recv_offset >= sizeof(eip_encap_t)) {
                    data_size = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(session->recv_data))->encap_length);
                }
            }
        }

        /* did we get all the data? */
        if (session->recv_offset == data_size) {
            break;
        } else {
            /* do not hog the CPU */
            sleep_ms(1);
        }
    }

    if (session->recv_offset != data_size) {
        session->recv_offset = 0;
        return PLCTAG_ERR_TIMEOUT;
    }

    /* set the offset back to zero for the next packet */
    session->recv_offset = 0;

    pdebug(DEBUG_INFO, "received response:");
    pdebug_dump_bytes(DEBUG_INFO, session->recv_data, data_size);

    /* encap header is at the start of the buffer */
    resp = (eip_encap_t*)(session->recv_data);

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
    session->session_handle = resp->encap_session_handle; /* opaque to us */
    session->registered = 1;

    pdebug(DEBUG_INFO, "Done.");

    return PLCTAG_STATUS_OK;
}


int session_unregister_unsafe(ab_session_p session)
{
    if (session->sock) {
        session->is_connected = 0;
        session->registered = 0;
        socket_close(session->sock);
        socket_destroy(&(session->sock));
        session->sock = NULL;
    }

    return PLCTAG_STATUS_OK;
}



/*
 * session_add_request_unsafe
 *
 * You must hold the mutex before calling this!
 */
int session_add_request_unsafe(ab_session_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p cur, prev;
    int total_requests = 0;

    pdebug(DEBUG_INFO, "Starting.");

    if(!sess) {
        pdebug(DEBUG_WARN, "Session is null!");
        return PLCTAG_ERR_NULL_PTR;
    }

    /* make sure the request points to the session */
    req->session = sess;

    /* we add the request to the end of the list. */
    cur = sess->requests;
    prev = NULL;

    while (cur) {
        prev = cur;
        cur = cur->next;
        total_requests++;
    }

    if (!prev) {
        sess->requests = req;
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
 * session_add_request
 *
 * This is a thread-safe version of the above routine.
 */
int session_add_request(ab_session_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting. sess=%p, req=%p", sess, req);

    critical_block(sess->mutex) {
        rc = session_add_request_unsafe(sess, req);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


/*
 * session_remove_request_unsafe
 *
 * You must hold the mutex before calling this!
 */
int session_remove_request_unsafe(ab_session_p sess, ab_request_p req)
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
    req->session = NULL;

    /* release the request refcount */
    rc_dec(req);

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}



/*
 * session_remove_request
 *
 * This is a thread-safe version of the above routine.
 */
int session_remove_request(ab_session_p sess, ab_request_p req)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.  session=%p, req=%p", sess, req);

    if(sess == NULL || req == NULL) {
        return rc;
    }

    critical_block(sess->mutex) {
        rc = session_remove_request_unsafe(sess, req);
    }

    pdebug(DEBUG_DETAIL, "Done.");

    return rc;
}


/* this must be called with the session mutex held */
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
            req->session->session_seq_id++;

            req->session_seq_id = req->session->session_seq_id;
            encap->encap_sender_context = req->session->session_seq_id; /* link up the request seq ID and the packet seq ID */

            /* mark the session as being used if this is a serialized packet */
            //~ mark_session_for_request(req);

            pdebug(DEBUG_INFO,"Sending unconnected packet with session sequence ID %llx",req->session->session_seq_id);
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
        encap->encap_session_handle = req->session->session_handle;
        encap->encap_status = h2le32(0);
        encap->encap_options = h2le32(0);

        /* display the data */
        pdebug(DEBUG_INFO,"Sending packet of size %d",req->request_size);
        pdebug_dump_bytes(DEBUG_INFO, req->data, req->request_size);

        req->send_in_progress = 1;
    }

    /* send the packet */
    rc = socket_write(req->session->sock, req->data + req->current_offset, req->request_size - req->current_offset);

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
 * Look at the passed session and read any data we can
 * to fill in a packet.  If we already have a full packet,
 * punt.
 */
int recv_eip_response_unsafe(ab_session_p session)
{
    uint32_t data_needed = 0;
    int rc = PLCTAG_STATUS_OK;

    /* skip the rest if we already have a packet waiting in the session buffer. */
    if(session->has_response) {
        return PLCTAG_STATUS_OK;
    }

    /*pdebug(DEBUG_DETAIL,"Starting.");*/

    /*
     * Determine the amount of data to get.  At a minimum, we
     * need to get an encap header.  This will determine
     * whether we need to get more data or not.
     */
    data_needed = (session->recv_offset < sizeof(eip_encap_t)) ?
                                            sizeof(eip_encap_t) :
                                            sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(session->recv_data))->encap_length);

    if (session->recv_offset < data_needed) {
        /* read everything we can */
        do {
            rc = socket_read(session->sock, session->recv_data + session->recv_offset,
                             data_needed - session->recv_offset);

            /*pdebug(DEBUG_DETAIL,"socket_read rc=%d",rc);*/

            if (rc < 0) {
                if (rc != PLCTAG_ERR_NO_DATA) {
                    /* error! */
                    pdebug(DEBUG_WARN,"Error reading socket! rc=%d",rc);
                    return rc;
                }
            } else {
                session->recv_offset += rc;

                /*pdebug_dump_bytes(session->debug, session->recv_data, session->recv_offset);*/

                /* recalculate the amount of data needed if we have just completed the read of an encap header */
                if (session->recv_offset >= sizeof(eip_encap_t)) {
                    data_needed = sizeof(eip_encap_t) + le2h16(((eip_encap_t*)(session->recv_data))->encap_length);
                }
            }
        } while (rc > 0 && session->recv_offset < data_needed);
    }

    /* did we get all the data? */
    if (session->recv_offset >= data_needed) {
        session->resp_seq_id = ((eip_encap_t*)(session->recv_data))->encap_sender_context;
        session->has_response = 1;

        rc = PLCTAG_STATUS_OK;

        pdebug(DEBUG_DETAIL, "request received all needed data.");

        if(((eip_encap_t*)(session->recv_data))->encap_command == h2le16(AB_EIP_READ_RR_DATA)) {
            eip_encap_t *encap = (eip_encap_t*)(session->recv_data);
            pdebug(DEBUG_INFO,"Received unconnected packet with session sequence ID %llx",encap->encap_sender_context);
        } else {
            eip_cip_co_resp *resp = (eip_cip_co_resp*)(session->recv_data);
            pdebug(DEBUG_INFO,"Received connected packet with connection ID %x and sequence ID %u(%x)",le2h32(resp->cpf_orig_conn_id), le2h16(resp->cpf_conn_seq_num), le2h16(resp->cpf_conn_seq_num));
        }
    }

    return rc;
}



/*******************************************************************************
 ************************** Session Library Functions ************************
 ******************************************************************************/
 
 
int session_setup()
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting.");
    
    /* this is a mutex used to synchronize most activities in this protocol */
    rc = mutex_create((mutex_p*)&global_session_mut);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_ERROR, "Unable to create session mutex!");
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


void session_teardown()
{
    pdebug(DEBUG_INFO,"Terminating IO thread.");

//    /* wait for the thread to die */
//    thread_join(io_handler_thread);
//    thread_destroy((thread_p*)&io_handler_thread);

    pdebug(DEBUG_INFO,"Freeing global session mutex.");
    /* clean up the mutex */
    mutex_destroy((mutex_p*)&global_session_mut);

    pdebug(DEBUG_INFO,"Done.");
}




/*******************************************************************************
 ************************** Session Helper Functions ************************
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



static void update_resend_samples(ab_session_p session, int64_t round_trip_time)
{
    int index;
    int64_t round_trip_sum = 0;
    int64_t round_trip_avg = 0;

    /* store the round trip information */
    session->round_trip_sample_index = ((++ session->round_trip_sample_index) >= SESSION_NUM_ROUND_TRIP_SAMPLES ? 0 : session->round_trip_sample_index);
    session->round_trip_samples[session->round_trip_sample_index] = round_trip_time;

    /* calculate the retry interval */
    for(index=0; index < SESSION_NUM_ROUND_TRIP_SAMPLES; index++) {
        round_trip_sum += session->round_trip_samples[index];
    }

    /* round up and double */
    round_trip_avg = (round_trip_sum + (SESSION_NUM_ROUND_TRIP_SAMPLES/2))/SESSION_NUM_ROUND_TRIP_SAMPLES;
    session->retry_interval = 3 * (round_trip_avg < SESSION_MIN_RESEND_INTERVAL ? SESSION_MIN_RESEND_INTERVAL : round_trip_avg);

    pdebug(DEBUG_INFO,"Packet round trip time %lld, running average round trip time is %lld",round_trip_time, session->retry_interval);
}



static void receive_response_unsafe(ab_session_p session, ab_request_p request)
{
    /*
     * We received a packet.  Modify the packet interval downword slightly
     * to get to the maximum value.  We want to get to the point where we lose
     * a packet once in a while.
     */

    /*session->next_packet_interval_us -= SESSION_PACKET_RECEIVE_INTERVAL_DEC;
    if(session->next_packet_interval_us < SESSION_MIN_PACKET_INTERVAL) {
        session->next_packet_interval_us = SESSION_MIN_PACKET_INTERVAL;
    }
    pdebug(DEBUG_INFO,"Packet received, so decreasing packet interval to %lldus", session->next_packet_interval_us);
    */

    pdebug(DEBUG_INFO,"Packet sent initially %dms ago and was sent %d times",(int)(time_ms() - request->time_sent), request->send_count);

    update_resend_samples(session, time_ms() - request->time_sent);

    /* set the packet ready for processing. */
    pdebug(DEBUG_INFO, "got full packet of size %d", session->recv_offset);
    pdebug_dump_bytes(DEBUG_INFO, session->recv_data, session->recv_offset);

    /* copy the data from the session's buffer */
    mem_copy(request->data, session->recv_data, session->recv_offset);
    request->request_size = session->recv_offset;

    request->resp_received = 1;
    request->send_in_progress = 0;
    request->send_request = 0;
    request->recv_in_progress = 0;
    
    if(request == session->fo_req) {
        /* this is a ForwardOpen response.  */
        rc = recv_forward_open_resp(session, req);
    }

    /* clear the request from the session as it is done. Note we hold the mutex here. */
    session_remove_request_unsafe(session, request);
}




int ok_to_resend(ab_session_p session, ab_request_p request)
{
    if(!session) {
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


int process_response_packet_unsafe(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;
    eip_cip_co_resp *response = (eip_cip_co_resp*)(&session->recv_data[0]);
    ab_request_p request = session->requests;

    /* find the request for which there is a response pending. */
    while(request) {
        /* need to get the next request now because we might be removing it in receive_response_unsafe */
        ab_request_p next_req = request->next;

        if(match_request_and_response(request, response)) {
            receive_response_unsafe(session, request);
        }

        request = next_req;
    }

    return rc;
}


int session_check_incoming_data(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_SPEW,"Starting.");
    
    /* take the mutex for the session to prevent anything from changing. */
    
    /* FIXME - is this needed here? */
    
    critical_block(session->mutex) {
        /*
         * check for data.
         *
         * Read a packet into the session's buffer and find the
         * request it is for.  Repeat while there is data.
         */

        do {
            rc = recv_eip_response_unsafe(session);

            /* did we get a packet? */
            if(rc == PLCTAG_STATUS_OK && session->has_response) {
                rc = process_response_packet_unsafe(session);

                /* reset the session's buffer */
                mem_set(session->recv_data, 0, MAX_REQ_RESP_SIZE);
                session->recv_offset = 0;
                session->resp_seq_id = 0;
                session->has_response = 0;
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



static int session_send_current_request(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

    if(!session->current_request) {
        return PLCTAG_STATUS_OK;
    }

    rc = send_eip_request_unsafe(session->current_request);

    if(rc != PLCTAG_STATUS_OK) {
        /*error sending packet!*/
        return rc;
    }

    /* if we are done, then clean up */
    if(!session->current_request->send_in_progress) {
        /* release the refcount on the request, we are not referencing it anymore */
        session->current_request = rc_dec(session->current_request);
    }

    return rc;
}


static int session_check_outgoing_data(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p request = session->requests;
    int connected_requests_in_flight = 0;
    int unconnected_requests_in_flight = 0;
    
    pdebug(DEBUG_SPEW,"Starting.");
    
    critical_block(session->mutex) {
        /* loop over the requests and process them one at a time. */
        while(request && rc == PLCTAG_STATUS_OK) {
            if(request->abort_request) {
                ab_request_p old_request = request;

                /* skip to the next one */
                request = request->next;

                //~ rc = handle_abort_request(old_request);
                rc = session_remove_request_unsafe(session,old_request);

                continue;
            }

            /* check resending */
            if(ok_to_resend(session, request)) {
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
            if(!session->current_request && request->send_request) {
                if(request->connected_request) {
                    if(connected_requests_in_flight < SESSION_MAX_CONNECTED_REQUESTS_IN_FLIGHT) {
                        pdebug(DEBUG_INFO,"Readying connected packet to send.");

                        /* increment the refcount since we are storing a pointer to the request */
                        session->current_request = rc_inc(request);

                        connected_requests_in_flight++;

                        pdebug(DEBUG_INFO,"sending packet, so %d connected requests in flight.", connected_requests_in_flight);
                    }
                } else {
                    if(unconnected_requests_in_flight < SESSION_MAX_UNCONNECTED_REQUESTS_IN_FLIGHT) {
                        pdebug(DEBUG_INFO,"Readying unconnected packet to send.");

                        /* increment the refcount since we are storing a pointer to the request */
                        session->current_request = rc_inc(request);

                        unconnected_requests_in_flight++;

                        pdebug(DEBUG_INFO,"sending packet, so %d unconnected requests in flight.", unconnected_requests_in_flight);
                    }
                }
            }

            /* call this often to make sure we get the data out. */
            rc = session_send_current_request(session);

            /* get the next request to process */
            request = request->next;
        }
    }
    
    pdebug(DEBUG_SPEW,"Done.");
    
    return rc;
}


/* determine the type of PLC */

int check_plc(attr attribs, ab_session_p session)
{
    const char* cpu_type = attr_get_str(attribs, "cpu", "NONE");

    if (!str_cmp_i(cpu_type, "plc") || !str_cmp_i(cpu_type, "plc5") || !str_cmp_i(cpu_type, "slc") ||
        !str_cmp_i(cpu_type, "slc500")) {
        session->plc_type = AB_PROTOCOL_PLC;
    } else if (!str_cmp_i(cpu_type, "micrologix800") || !str_cmp_i(cpu_type, "mlgx800") || !str_cmp_i(cpu_type, "micro800")) {
        session->plc_type = AB_PROTOCOL_MLGX800;
    } else if (!str_cmp_i(cpu_type, "micrologix") || !str_cmp_i(cpu_type, "mlgx")) {
        session->plc_type = AB_PROTOCOL_MLGX;
    } else if (!str_cmp_i(cpu_type, "compactlogix") || !str_cmp_i(cpu_type, "clgx") || !str_cmp_i(cpu_type, "lgx") ||
               !str_cmp_i(cpu_type, "controllogix") || !str_cmp_i(cpu_type, "contrologix") ||
               !str_cmp_i(cpu_type, "flexlogix") || !str_cmp_i(cpu_type, "flgx")) {
        session->plc_type = AB_PROTOCOL_LGX;
    } else {
        pdebug(DEBUG_WARN, "Unsupported device type: %s", cpu_type);

        return PLCTAG_ERR_BAD_DEVICE;
    }

    return PLCTAG_STATUS_OK;
}


int check_path(attr attribs, session)
{
    int rc = PLCTAG_STATUS_OK;
    const char *path = NULL;
    const char **path_segs = NULL;
    int num_segs = 0;
    
    /* get the connection path, punt if there is not one and we have a Logix-class PLC. */
    path = attr_get_str(attribs,"path",NULL);
    
    if(!path) {
        pdebug(DEBUG_WARN,"You must include a path to the PLC!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    path_segs = str_split(path,",");
    
    if(!path_segs) {
        pdebug(DEBUG_WARN,"Unable to split path into segments!");
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    /* count the number of segments. */
    for(num_segs=0; path_segs[num_segs]; num_segs++) { ; }

    if(num_segs < 3 && session->plc_type == AB_PROTOCOL_LGX) {
        pdebug(DEBUG_WARN,"The path must include the internal path in the PLC chassis for Logix-class PLCs!");
        mem_free(path_segs);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(num_segs > 1 && session->plc_type == AB_PROTOCOL_MICRO800) {
        pdebug(DEBUG_WARN,"Micro800 series may not have a path beyond the gateway!");
        mem_free(path_segs);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(num_segs > 1 && session->plc_type == AB_PROTOCOL_PLC5) {
        pdebug(DEBUG_WARN,"PLC/5, SLC series may not have a path beyond the gateway!");
        mem_free(path_segs);
        return PLCTAG_ERR_BAD_PARAM;
    }

    if(num_segs > 1 && session->plc_type == AB_PROTOCOL_MLGX) {
        pdebug(DEBUG_WARN,"Micrologix series may not have a path beyond the gateway!");
        mem_free(path_segs);
        return PLCTAG_ERR_BAD_PARAM;
    }
    
    /* save the entire path for later so that we can match it. */
    session->path = str_dup(path);

    /* store the gateway host for later. */
    session->gateway_host = str_dup(path_segs[0]);
    
    /* parse the path into the session buffer if we need to. */
    if(num_segs > 1) {
        rc = parse_path_segs(&path_segs[1], num_segs-1, session);
        if(rc != PLCTAG_STATUS_OK) {
            pdebug(DEBUG_WARN,"Unable to part PLC path!");
            mem_free(path_segs);
            return PLCTAG_ERR_BAD_PARAM;
        }
    }
    
    return PLCTAG_STATUS_OK;
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

int parse_path_segments(const char **links, int num_segs, ab_session_p session)
{
    int ioi_size=0;
    int link_index=0;
    int last_is_dhp=0;
    int has_dhp=0;
    int dhp_channel=0;
    int src_addr=0, dest_addr=0;
    int tmp=0;
    char *link=NULL;
    //uint8_t *data = tag->conn_path;
    uint8_t data[MAX_CONN_PATH+10] = {0,};  /* MAGIC - 10 for extra buffer. */
    int data_index = 0;
    
    pdebug(DEBUG_INFO,"Starting");

    for(link_index=0; links[link_index] && data_index < MAX_CONN_PATH; link_index++) {
        /* work along each string. */
        link = links[link_index];

        pdebug(DEBUG_DETAIL,"Processing path segment '%s'", link);
        
        /* check the first character of each part.  That will determine what kind it is.  For now. */
        if(is_digit(link[0])) {
            if(str_to_int(link, &tmp) != 0) {
                /* syntax error */
                pdebug(DEBUG_WARN, "Syntax error in path, expected number, got '%s'!", link);
                return PLCTAG_ERR_BAD_PARAM;
            }

            data[data_index] = tmp;
            data_index++;
        } else {
            switch(link[0]) {
                case 'a':
                case 'A':
                    dhp_channel = 2;
                    break;
                case 'b':
                case 'B':
                    dhp_channel = 3;
                    break;
                    
                default:
                    pdebug(DEBUG_WARN, "Unknown path segment type '%s'!",link);
                    return PLCTAG_ERR_BAD_PARAM;
            }

            /* should be a DH+ link. This must be the last segment. */
            if(link_index != (num_segs -1)) {
                /* nope. */
                pdebug(DEBUG_WARN,"A DH+ bridge segment must be at the end of the path!");
                return PLCTAG_ERR_BAD_PARAM;
            }
            
            /* check the formatting. */
            if(link[1] != ":") {
                pdebug(DEBUG_WARN,"A DH+ segment is formatted with A:n where A is the port (A or B) and n is the node id, IN DECIMAL!  Instead found '%s'", link);
                return PLCTAG_ERR_BAD_PARAM;
            }
            
            /* get the node ID. */
            if(str_to_int(&link[2], &tmp) != 0) {
                /* syntax error */
                pdebug(DEBUG_WARN, "Syntax error in DH+ path segment, expected number, got '%s'!", &link[2]);
                return PLCTAG_ERR_BAD_PARAM;
            }

            dest_addr = tmp;
            last_is_dhp = 1;
        }
    }
        
    /* Add to the path based on the protocol type and
      * whether the last part is DH+.  Only some combinations of
      * DH+ and PLC type work.
      */
    if(last_is_dhp && session->plc_type == AB_PROTOCOL_PLC) {
        /* We have to make the difference from the more
         * generic case.
         */
        //tag->routing_path[0] = 0x20; /* class */
        //tag->routing_path[1] = 0xA6; /* DH+ */
        //tag->routing_path[2] = 0x24; /* instance */
        //tag->routing_path[3] = dhp_channel;  /* 1 = Channel A, 2 = Channel B */
        //tag->routing_path[4] = 0x2C; /* ? */
        //tag->routing_path[5] = 0x01; /* ? */
        //tag->routing_path_size = 6;

        /* try adding this onto the end of the routing path */
        data[data_index] = 0x20;
        data_index++;
        data[data_index] = 0xA6;
        data_index++;
        data[data_index] = 0x24;
        data_index++;
        data[data_index] = dhp_channel;
        data_index++;
        data[data_index] = 0x2C;
        data_index++;
        data[data_index] = 0x01;
        data_index++;
        //ioi_size += 6;

        //tag->dhp_src  = src_addr;
        session->dhp_dest = dest_addr;
        session->use_dhp_direct = 1;
    } else {
        if(tag->needs_connection) {
            /*
             * we do a generic path to the router
             * object in the PLC.  But only if the PLC is
             * one that needs a connection.  For instance a
             * Micro850 needs to work in connected mode.
             */
            data[data_index] = 0x20;   /* class */
            data_index++;
            data[data_index] = 0x02;   /* message router class */
            data_index++;
            data[data_index] = 0x24;   /* instance */
            data_index++;
            data[data_index] = 0x01;   /* message router class instance #1 */
            data_index++;
            //ioi_size += 4;
        }

//        tag->dhp_src  = 0;
//        tag->dhp_dest = 0;
//        tag->use_dhp_direct = 0;
    } 

    /*
     * zero out the last byte if we need to.
     * This pads out the path to a multiple of 16-bit
     * words.
     */
    pdebug(DEBUG_DETAIL,"Path size before %d", data_index);
    if(data_index & 0x01) {
        data[data_index] = 0;
        data_index++;
    }
    
    /* allocate the path. */
    session->conn_path = mem_alloc(data_index);
    if(!session->conn_path) {
        pdebug(DEBUG_WARN,"Unable to allocate PLC connection path!");
        return PLCTAG_ERR_NO_MEM;
    }
    
    /* make sure the session has a copy. */
    mem_copy(session->conn_path, &data[0], data_index);

    /* set the connection path size */
    session->conn_path_size = data_index;

    return PLCTAG_STATUS_OK;
}






int process_creation_params(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;
    
    pdebug(DEBUG_INFO,"Starting");
    
    rc = check_plc(session->creation_attributes);
    if(rc < 0) {
        pdebug(DEBUG_WARN,"Unsupported or missing PLC type!");
        return PLCTAG_ERR_BAD_DEVICE;
    }
    
    session->plc_type = rc;
    
    
    
}

void session_handler(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_SPEW, "Checking for things to do with session %p", session);
    
    switch(session->state) {
       case PLC_START:
            rc = process_creation_params(session);
            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Bad or missing parameters.  Unable to create PLC object!");
                session->retry_time_ms = time_ms() + PLC_RETRY_TIME;
                session->state = PLC_WAIT_RETRY;                
            } else {
                pdebug(DEBUG_INFO,"PLC creation params all check out.");
                session->state = PLC_OPEN_SESSION;
            }
            
            break;
            
       case PLC_OPEN_SESSION:
            pdebug(DEBUG_INFO,"Starting thread to set up session.");
            /* kick off a thread to handle the blocking parts of the session creation. */
            session->status = PLCTAG_STATUS_PENDING;
            
            rc = thread_create(&session->setup_thread,session_init,32*1024, (void*)session);
            if(rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Unable to start background session initialization thread! rc= %d", rc);
                session->status = PLCTAG_STATUS_PENDING;
                session->retry_time_ms = time_ms() + PLC_RETRY_TIME;
                session->state = PLC_WAIT_RETRY;
            } else {
                pdebug(DEBUG_INFO, "Session not registered, kicked off registration thread.");
                session->state = PLC_WAIT_SESSION;
            }
            
            break;
        
        case PLC_WAIT_SESSION:
            if(session->init_status != PLCTAG_STATUS_PENDING) {
                /* thread is done. Might be error, might not, but clean up the thread ASAP. */
                if(session->setup_thread) {
                    thread_join(session->setup_thread);
                    thread_destroy(&session->setup_thread);
                }
            }
            
            if(session->init_status != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN,"Failure trying to set up session!");
                session->state = PLC_WAIT_RETRY;
                session->init_status = PLCTAG_STATUS_PENDING;
                session->retry_time_ms = time_ms() + PLC_RETRY_TIME;
            } else {
                pdebug(DEBUG_INFO,"Session set up.");
                session->status = PLCTAG_STATUS_PENDING;
                session->state = PLC_PROCESS_REQUEST;
            }
            
            break;
        
        case PLC_PROCESS_REQUEST:
            rc = session_check_incoming_data(session);
            if (rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN, "Error when checking for incoming session data! %d", rc);
                /* FIXME - do something useful with this error */
            }

            /* check for incoming data. */
            rc = session_check_outgoing_data(session);
            if (rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_WARN, "Error when checking for outgoing session data! %d", rc);
                /* FIXME - do something useful with this error */
            }

            break;
        
        case PLC_CLOSE_CONNECTION:
        
        case PLC_WAIT_CLOSE_CONNECTION:
        
        case PLC_CLOSE_SESSION:
        
        case PLC_WAIT_RETRY:
    }
    
    
}


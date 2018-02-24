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
#include <stdlib.h>
#include <time.h>



/*
 * Externally visible global variables
 */
mutex_p global_session_mut = NULL;


/* local variables. */

static ab_session_p sessions = NULL;

/* request/response handling thread */
static thread_p io_handler_thread = NULL;



static ab_session_p session_create_unsafe(const char* host, int gw_port);
static int session_init(ab_session_p session);
static int add_session_unsafe(ab_session_p n);
//~ static int add_session(ab_session_p s);
static int remove_session_unsafe(ab_session_p n);
//~ static int remove_session(ab_session_p s);
static ab_session_p find_session_by_host_unsafe(const char  *t);
//~ static int session_add_tag_unsafe(ab_session_p session, ab_tag_p tag);
//~ static int session_add_tag(ab_session_p session, ab_tag_p tag);
//~ static int session_remove_tag_unsafe(ab_session_p session, ab_tag_p tag);
//~ static int session_remove_tag(ab_session_p session, ab_tag_p tag);
static int session_connect(ab_session_p session);
//~ static int session_cleanup_unsafe(ab_session_p session);
static void session_cleanup(void *session);
//~ static int session_is_empty(ab_session_p session);
static int session_register(ab_session_p session);
static int session_unregister_unsafe(ab_session_p session);



#ifdef _WIN32
DWORD __stdcall request_handler_func(LPVOID not_used);
#else
void *request_handler_func(void *not_used);
#endif

static int session_check_incoming_data_unsafe(ab_session_p session);
//static int request_check_outgoing_data_unsafe(ab_session_p session, ab_request_p req);


/*
 * session_get_new_seq_id_unsafe
 *
 * A wrapper to get a new session sequence ID.  Not thread safe.
 *
 * Note that this is dangerous to use in threaded applications
 * because 32-bit processors will not implement a 64-bit
 * integer as an atomic entity.
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
    critical_block(global_session_mut) {
        res = (uint16_t)session_get_new_seq_id_unsafe(sess);
    }
    //pdebug(DEBUG_DETAIL, "leaving critical block %p", global_session_mut);

    return res;
}


static int connection_is_usable(ab_connection_p connection)
{
    if(!connection) {
        return 0;
    }

    if(connection->exclusive) {
        return 0;
    }

    if(connection->disconnect_in_progress) {
        return 0;
    }

    return 1;
}



ab_connection_p session_find_connection_by_path_unsafe(ab_session_p session,const char *path)
{
    ab_connection_p connection;

    connection = session->connections;

    /*
     * there are a lot of conditions.
     * We do not want to use connections that are in the process of shutting down.
     * We do not want to use connections that are used exclusively by one tag.
     * We want to use connections that have the same path as the tag.
     */
    while (connection && !connection_is_usable(connection) && str_cmp_i(connection->path, path) != 0) {
        connection = connection->next;
    }

    /* add to the ref count since we found an existing one. */
    if(connection) {
        rc_inc(connection);
    }

    return connection;
}



int session_add_connection_unsafe(ab_session_p session, ab_connection_p connection)
{
    pdebug(DEBUG_DETAIL, "Starting");

    /* add the connection to the list in the session */
    connection->next = session->connections;
    session->connections = connection;

    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
}




int session_add_connection(ab_session_p session, ab_connection_p connection)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(session) {
        critical_block(global_session_mut) {
            rc = session_add_connection_unsafe(session, connection);
        }
    } else {
        pdebug(DEBUG_WARN, "Session ptr is null!");
        rc = PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

/* must have the session mutex held here. */
int session_remove_connection_unsafe(ab_session_p session, ab_connection_p connection)
{
    ab_connection_p cur;
    ab_connection_p prev;
    /* int debug = session->debug; */
    int rc;

    pdebug(DEBUG_DETAIL, "Starting");

    cur = session->connections;
    prev = NULL;

    while (cur && cur != connection) {
        prev = cur;
        cur = cur->next;
    }

    if (cur && cur == connection) {
        if (prev) {
            prev->next = cur->next;
        } else {
            session->connections = cur->next;
        }

        rc = PLCTAG_STATUS_OK;
    } else {
        rc = PLCTAG_ERR_NOT_FOUND;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

int session_remove_connection(ab_session_p session, ab_connection_p connection)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    if(session) {
        critical_block(global_session_mut) {
            rc = session_remove_connection_unsafe(session, connection);
        }
    } else {
        rc = PLCTAG_ERR_NULL_PTR;
    }

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}


int session_find_or_create(ab_session_p *tag_session, attr attribs)
{
    /*int debug = attr_get_int(attribs,"debug",0);*/
    const char* session_gw = attr_get_str(attribs, "gateway", "");
    int session_gw_port = attr_get_int(attribs, "gateway_port", AB_EIP_DEFAULT_PORT);
    ab_session_p session = AB_SESSION_NULL;
    int new_session = 0;
    int shared_session = attr_get_int(attribs, "share_session", 1); /* share the session by default. */
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting");

    critical_block(global_session_mut) {
        /* if we are to share sessions, then look for an existing one. */
        if (shared_session) {
            session = find_session_by_host_unsafe(session_gw);
        } else {
            /* no sharing, create a new one */
            session = AB_SESSION_NULL;
        }

        if (session == AB_SESSION_NULL) {
            pdebug(DEBUG_DETAIL,"Creating new session.");
            session = session_create_unsafe(session_gw, session_gw_port);

            if (session == AB_SESSION_NULL) {
                pdebug(DEBUG_WARN, "unable to create or find a session!");
                rc = PLCTAG_ERR_BAD_GATEWAY;
            } else {
                new_session = 1;
            }
        } else {
            pdebug(DEBUG_DETAIL,"Reusing existing session.");
        }
    }

    /*
     * do this OUTSIDE the mutex in order to let other threads not block if
     * the session creation process blocks.
     */

    if(new_session) {
        rc = session_init(session);

        if(rc != PLCTAG_STATUS_OK) {
            /* failed to set up the session! */
            //session_cleanup(session);
            session = rc_dec(session);
            session = AB_SESSION_NULL;
        } else {
            /* save the status */
            session->status = rc;
        }
    }

    /* store it into the tag */
    *tag_session = session;

    pdebug(DEBUG_DETAIL, "Done");

    return rc;
}

int add_session_unsafe(ab_session_p n)
{
    pdebug(DEBUG_DETAIL, "Starting");

    if (!n) {
        return PLCTAG_ERR_NULL_PTR;
    }

    n->prev = NULL;
    n->next = sessions;

    if (sessions) {
        sessions->prev = n;
    }

    sessions = n;

    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
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

int remove_session_unsafe(ab_session_p n)
{
    ab_session_p tmp;

    pdebug(DEBUG_DETAIL, "Starting");

    if (!n || !sessions) {
        return 0;
    }

    tmp = sessions;

    while (tmp && tmp != n)
        tmp = tmp->next;

    if (!tmp || tmp != n) {
        pdebug(DEBUG_WARN, "Session not found!");
        return PLCTAG_ERR_NOT_FOUND;
    }

    if (n->next) {
        n->next->prev = n->prev;
    }

    if (n->prev) {
        n->prev->next = n->next;
    } else {
        sessions = n->next;
    }

    n->next = NULL;
    n->prev = NULL;

    pdebug(DEBUG_DETAIL, "Done");

    return PLCTAG_STATUS_OK;
}

int remove_session(ab_session_p s)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_DETAIL, "Starting.");

    critical_block(global_session_mut) {
        rc = remove_session_unsafe(s);
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


ab_session_p find_session_by_host_unsafe(const char* t)
{
    ab_session_p tmp;

    tmp = sessions;

    while (tmp && !session_match_valid(t, tmp)) {
        tmp = tmp->next;
    }

    if (!tmp) {
        return (ab_session_p)NULL;
    }

    if(tmp) {
        /* found the session, so increase the ref count. */
        rc_inc(tmp);
    }

    return tmp;
}



ab_session_p session_create_unsafe(const char* host, int gw_port)
{
    ab_session_p session = AB_SESSION_NULL;
    static volatile uint32_t srand_setup = 0;
    static volatile uint32_t connection_id = 0;

    pdebug(DEBUG_INFO, "Starting");

    pdebug(DEBUG_DETAIL, "Warning: not using passed port %d", gw_port);

    session = rc_alloc(sizeof(struct ab_session_t), session_cleanup);

    if (!session) {
        pdebug(DEBUG_WARN, "Error allocating new session.");
        return AB_SESSION_NULL;
    }

    str_copy(session->host, MAX_SESSION_HOST, host);

    session->status = PLCTAG_STATUS_PENDING;

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

    /* set up the packet interval to a reasonable default */
    //session->next_packet_interval_us = SESSION_DEFAULT_PACKET_INTERVAL;

    /* set up packet round trip information */
    for(int index=0; index < SESSION_NUM_ROUND_TRIP_SAMPLES; index++) {
        session->round_trip_samples[index] = SESSION_DEFAULT_RESEND_INTERVAL_MS;
    }
    session->retry_interval = SESSION_DEFAULT_RESEND_INTERVAL_MS;

    /* FIXME */
    pdebug(DEBUG_DETAIL, "Refcount is now %d", rc_count(session));

    /* add the new session to the list. */
    add_session_unsafe(session);

    pdebug(DEBUG_INFO, "Done");

    return session;
}


/*
 * session_init
 *
 * This calls several blocking methods and so must not keep the main mutex
 * locked during them.
 */
int session_init(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    /* we must connect to the gateway and register */
    if ((rc = session_connect(session)) != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "session connect failed!");
        session->status = rc;
        return rc;
    }

    if ((rc = session_register(session)) != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "session registration failed!");
        session->status = rc;
        return rc;
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
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
        return 0;
    }

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

/* must have the session mutex held here */
void session_cleanup(void *session_arg)
{
    ab_session_p session = session_arg;
    int really_destroy = 1;

    pdebug(DEBUG_INFO, "Starting.");

    if (!session) {
        pdebug(DEBUG_WARN, "Session ptr is null!");

        return;
    }

    /*
     * Work around the race condition here.  Another thread could have looked up the
     * session before this thread got to this point, but after this thread saw the
     * session's ref count go to zero.  So, we need to check again after preventing
     * other threads from getting a reference.
     */

    critical_block(global_session_mut) {
        if(rc_count(session) > 0) {
            pdebug(DEBUG_WARN,"Another thread got a reference to this session before it could be deleted.  Aborting deletion");
            really_destroy = 0;
            break;
        }

        /* still good, so remove the session from the list so no one else can reference it. */
        remove_session_unsafe(session);
    }

    /*
     * if we are really destroying the session then we know that this is
     * the last reference.  So, we can use the unsafe variants.
     */
    if(really_destroy) {
        ab_request_p req;

        /* unregister and close the socket. */
        session_unregister_unsafe(session);

        /* remove any remaining requests, they are dead */
        req = session->requests;

        while(req) {
            session_remove_request_unsafe(session, req);
            req = session->requests;
        }

        rc_free(session);
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

    critical_block(global_session_mut) {
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

    critical_block(global_session_mut) {
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
            conn_req->cpf_targ_conn_id = h2le32(req->connection->targ_connection_id);
            req->conn_id = req->connection->orig_connection_id;

            req->connection->conn_seq_num++;
            conn_req->cpf_conn_seq_num = h2le16(req->connection->conn_seq_num);
            req->conn_seq = req->connection->conn_seq_num;

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


/*
int session_acquire(ab_session_p session)
{
    pdebug(DEBUG_INFO, "Acquire session=%p", session);

    if(!session) {
        return PLCTAG_ERR_NULL_PTR;
    }

    return rc_inc(&session->rc);
}


int session_release(ab_session_p session)
{
    pdebug(DEBUG_INFO, "Release session=%p", session);

    if(!session) {
        return PLCTAG_ERR_NULL_PTR;
    }

    return refcount_release(&session->rc);
}
*/


/*******************************************************************************
 ************************** Session IO Thread Functions ************************
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

    /* create the background IO handler thread */
    rc = thread_create((thread_p*)&io_handler_thread, request_handler_func, 32*1024, NULL);

    if(rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_INFO,"Unable to create request handler thread!");
        return rc;
    }

    pdebug(DEBUG_INFO,"Done.");

    return rc;
}


void session_teardown()
{
    pdebug(DEBUG_INFO,"Terminating IO thread.");

    /* wait for the thread to die */
    thread_join(io_handler_thread);
    thread_destroy((thread_p*)&io_handler_thread);

    pdebug(DEBUG_INFO,"Freeing global session mutex.");
    /* clean up the mutex */
    mutex_destroy((mutex_p*)&global_session_mut);

    pdebug(DEBUG_INFO,"Done.");
}


 


/*
 * setup_session_mutex
 *
 * check to see if the global mutex is set up.  If not, do an atomic
 * lock and set it up.
 */
int setup_session_mutex(void)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_INFO, "Starting.");

    critical_block(global_library_mutex) {
        /* first see if the mutex is there. */
        if (!global_session_mut) {
            rc = mutex_create((mutex_p*)&global_session_mut);

            if (rc != PLCTAG_STATUS_OK) {
                pdebug(DEBUG_ERROR, "Unable to create global tag mutex!");
            }
        }
    }

    pdebug(DEBUG_INFO, "Done.");

    return rc;
}


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



int session_check_incoming_data_unsafe(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

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

    return rc;
}

//int request_check_outgoing_data_unsafe(ab_session_p session, ab_request_p request)
//{
//    int rc = PLCTAG_STATUS_OK;
//
//    /*pdebug(DEBUG_DETAIL,"Starting.");*/
//
//    /*
//     * Check to see if we can send something.
//     */
//
//    if(!session->current_request && request->send_request /*&& session->next_packet_time_us < (time_ms() * 1000)*/) {
//        /* nothing being sent and this request is outstanding */
//
//        /* refcount++ since we are storing a pointer */
//        session->current_request = rc_inc(request);
//
//        //session->next_packet_time_us = (time_ms()*1000) + session->next_packet_interval_us;
//
//        pdebug(DEBUG_INFO,"request->send_request=%d",request->send_request);
//        //pdebug(DEBUG_INFO,"session->next_packet_time_us=%lld and time=%lld",session->next_packet_time_us, (time_ms()*1000));
//        //pdebug(DEBUG_INFO,"Setting up request for sending, next packet in %lldus",(session->next_packet_time_us - (time_ms()*1000)));
//
//        if(request->send_count == 0) {
//            request->time_sent = time_ms();
//        }
//
//        request->send_count++;
//    }
//
//    /* if we are already sending this request, check its status */
//    if (session->current_request == request) {
//        /* is the request done? */
//        if (request->send_request) {
//            /* not done, try sending more */
//            send_eip_request_unsafe(request);
//            /* FIXME - handle return code! */
//        } else {
//            /*
//             * done in some manner, remove it from the session to let
//             * another request get sent.
//             */
//            /* we need to release the request since we are removing a pointer to it. */
//            session->current_request = rc_dec(session->current_request);
//        }
//    }
//
//    /*pdebug(DEBUG_DETAIL,"Done.");*/
//
//    return rc;
//}
//


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


static int session_check_outgoing_data_unsafe(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;
    ab_request_p request = session->requests;
    int connected_requests_in_flight = 0;
    int unconnected_requests_in_flight = 0;

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

    return rc;
}


static void process_session_tasks_unsafe(ab_session_p session)
{
    int rc = PLCTAG_STATUS_OK;

    pdebug(DEBUG_SPEW, "Checking for things to do with session %p", session);


    if(!session->registered) {
        return;
    }

    /* check for incoming data. */
    rc = session_check_incoming_data_unsafe(session);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Error when checking for incoming session data! %d", rc);
        /* FIXME - do something useful with this error */
    }

    /* check for incoming data. */
    rc = session_check_outgoing_data_unsafe(session);

    if (rc != PLCTAG_STATUS_OK) {
        pdebug(DEBUG_WARN, "Error when checking for outgoing session data! %d", rc);
        /* FIXME - do something useful with this error */
    }
}




#ifdef _WIN32
DWORD __stdcall request_handler_func(LPVOID not_used)
#else
void* request_handler_func(void* not_used)
#endif
{
    ab_session_p cur_sess;

    /* garbage code to stop compiler from whining about unused variables */
    pdebug(DEBUG_DETAIL,"Starting with arg %p",not_used);

    while (!library_terminating) {
        /* we need the mutex */
        if (global_session_mut == NULL) {
            pdebug(DEBUG_ERROR, "global_session_mut is NULL!");
            break;
        }

        /*pdebug(DEBUG_INFO,"entering critical block %p",global_session_mut);*/
        critical_block(global_session_mut) {
            /*
             * loop over the sessions.  For each session, see if we can read some
             * data.  If we can, read it in and try to update a request.  If the
             * session has outstanding requests that need to be sent, try to send
             * them.
             */

            cur_sess = sessions;

            while (cur_sess) {
                /* process incoming and outgoing data for the session. */
                process_session_tasks_unsafe(cur_sess);

                /*  move to the next session */
                /*pdebug(DEBUG_INFO,"cur_sess=%p, cur_sess->next=%p",cur_sess, cur_sess->next);*/
                cur_sess = cur_sess->next;
            }
        } /* end synchronized block */
        /*pdebug(DEBUG_INFO,"leaving critical block %p",global_session_mut);*/

        /*
         * give up the CPU. 1ms is not really going to happen.  Usually it is more based on the OS
         * default time and is usually around 10ms.  But, this sleep usually causes context switch.
         */
        sleep_ms(1);
    }

    thread_stop();

    /* FIXME -- this should be factored out as a platform dependency.*/
#ifdef _WIN32
    return (DWORD)0;
#else
    return NULL;
#endif
}



/*
 * iperf, Copyright (c) 2014-2018, The Regents of the University of
 * California, through Lawrence Berkeley National Laboratory (subject
 * to receipt of any required approvals from the U.S. Dept. of
 * Energy).  All rights reserved.
 *
 * If you have questions about your rights to use or distribute this
 * software, please contact Berkeley Lab's Technology Transfer
 * Department at TTD@lbl.gov.
 *
 * NOTICE.  This software is owned by the U.S. Department of Energy.
 * As such, the U.S. Government has been granted for itself and others
 * acting on its behalf a paid-up, nonexclusive, irrevocable,
 * worldwide license in the Software to reproduce, prepare derivative
 * works, and perform publicly and display publicly.  Beginning five
 * (5) years after the date permission to assert copyright is obtained
 * from the U.S. Department of Energy, and subject to any subsequent
 * five (5) year renewals, the U.S. Government is granted for itself
 * and others acting on its behalf a paid-up, nonexclusive,
 * irrevocable, worldwide license in the Software to reproduce,
 * prepare derivative works, distribute copies to the public, perform
 * publicly and display publicly, and to permit others to do so.
 *
 * This code is distributed under a BSD style license, see the LICENSE
 * file for complete information.
 */
#ifndef        __MPERF_API_H
#define        __MPERF_API_H

#include <sys/time.h>
#include <setjmp.h>
#include <stdio.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" { /* open extern "C" */
#endif


struct mperf_test;
//struct iperf_stream_result;
//struct mperf_interval_results;
//struct mperf_stream;

/* default settings */
#define Ptcp SOCK_STREAM
#define Pudp SOCK_DGRAM
#define Psctp 12
#define DEFAULT_UDP_BLKSIZE 1460 /* default is dynamically set, else this */
#define DEFAULT_TCP_BLKSIZE (128 * 1024)  /* default read/write block size */
#define DEFAULT_SCTP_BLKSIZE (64 * 1024)

/* short option equivalents, used to support options that only have long form */
#define OPT_SCTP 1
#define OPT_LOGFILE 2
#define OPT_GET_SERVER_OUTPUT 3
#define OPT_UDP_COUNTERS_64BIT 4
#define OPT_CLIENT_PORT 5
#define OPT_NUMSTREAMS 6
#define OPT_FORCEFLUSH 7
#define OPT_NO_FQ_SOCKET_PACING 9 /* UNUSED */
#define OPT_FQ_RATE 10
#define OPT_DSCP 11
#define OPT_CLIENT_USERNAME 12
#define OPT_CLIENT_RSA_PUBLIC_KEY 13
#define OPT_SERVER_RSA_PRIVATE_KEY 14
#define OPT_SERVER_AUTHORIZED_USERS 15
#define OPT_PACING_TIMER 16
#define OPT_CONNECT_TIMEOUT 17

/* states */
#define TEST_START 1
#define TEST_RUNNING 2
#define RESULT_REQUEST 3 /* not used */
#define TEST_END 4
#define STREAM_BEGIN 5 /* not used */
#define STREAM_RUNNING 6 /* not used */
#define STREAM_END 7 /* not used */
#define ALL_STREAMS_END 8 /* not used */
#define PARAM_EXCHANGE 9
#define CREATE_STREAMS 10
#define SERVER_TERMINATE 11
#define CLIENT_TERMINATE 12
#define EXCHANGE_RESULTS 13
#define DISPLAY_RESULTS 14
#define MPERF_START 15
#define MPERF_DONE 16
#define ACCESS_DENIED (-1)
#define SERVER_ERROR (-2)

/* Error routines. */
void mperf_err(struct mperf_test *test, const char *format, ...)
		__attribute__ ((format(printf,2,3)));

void mperf_errexit(struct mperf_test *test, const char *format, ...)
		__attribute__ ((format(printf,2,3),noreturn));

char *mperf_strerror(int);

extern int m_errno;

enum {
	MENONE = 0,             // No error
    /* Parameter errors */
    MESERVCLIENT = 1,       // Iperf cannot be both server and client
    MENOROLE = 2,           // Iperf must either be a client (-c) or server (-s)
    MESERVERONLY = 3,       // This option is server only
    MECLIENTONLY = 4,       // This option is client only
    MEDURATION = 5,         // test duration too long. Maximum value = %dMAX_TIME
    MENUMSTREAMS = 6,       // Number of parallel streams too large. Maximum value = %dMAX_STREAMS
    MEBLOCKSIZE = 7,        // Block size too large. Maximum value = %dMAX_BLOCKSIZE
    MEBUFSIZE = 8,          // Socket buffer size too large. Maximum value = %dMAX_TCP_BUFFER
    MEINTERVAL = 9,         // Invalid report interval (min = %gMIN_INTERVAL, max = %gMAX_INTERVAL seconds)
    MEMSS = 10,             // MSS too large. Maximum value = %dMAX_MSS
    MENOSENDFILE = 11,      // This OS does not support sendfile
    MEOMIT = 12,            // Bogus value for --omit
    MEUNIMP = 13,           // Not implemented yet
    MEFILE = 14,            // -F file couldn't be opened
    MEBURST = 15,           // Invalid burst count. Maximum value = %dMAX_BURST
    MEENDCONDITIONS = 16,   // Only one test end condition (-t, -n, -k) may be specified
    MELOGFILE = 17,	    // Can't open log file
    MENOSCTP = 18,	    // No SCTP support available
    MEBIND = 19,	    // UNUSED:  Local port specified with no local bind option
    MEUDPBLOCKSIZE = 20,    // Block size invalid
    MEBADTOS = 21,	    // Bad TOS value
    MESETCLIENTAUTH = 22,   // Bad configuration of client authentication
    MESETSERVERAUTH = 23,   // Bad configuration of server authentication
    MEBADFORMAT = 24,	    // Bad format argument to -f
    /* Test errors */
    MENEWTEST = 100,        // Unable to create a new test (check perror)
    MEINITTEST = 101,       // Test initialization failed (check perror)
    MELISTEN = 102,         // Unable to listen for connections (check perror)
    MECONNECT = 103,        // Unable to connect to server (check herror/perror) [from netdial]
    MEACCEPT = 104,         // Unable to accept connection from client (check herror/perror)
    MESENDCOOKIE = 105,     // Unable to send cookie to server (check perror)
    MERECVCOOKIE = 106,     // Unable to receive cookie from client (check perror)
    MECTRLWRITE = 107,      // Unable to write to the control socket (check perror)
    MECTRLREAD = 108,       // Unable to read from the control socket (check perror)
    MECTRLCLOSE = 109,      // Control socket has closed unexpectedly
    MEMESSAGE = 110,        // Received an unknown message
    MESENDMESSAGE = 111,    // Unable to send control message to client/server (check perror)
    MERECVMESSAGE = 112,    // Unable to receive control message from client/server (check perror)
    MESENDPARAMS = 113,     // Unable to send parameters to server (check perror)
    MERECVPARAMS = 114,     // Unable to receive parameters from client (check perror)
    MEPACKAGERESULTS = 115, // Unable to package results (check perror)
    MESENDRESULTS = 116,    // Unable to send results to client/server (check perror)
    MERECVRESULTS = 117,    // Unable to receive results from client/server (check perror)
    MESELECT = 118,         // Select failed (check perror)
    MECLIENTTERM = 119,     // The client has terminated
    MESERVERTERM = 120,     // The server has terminated
    MEACCESSDENIED = 121,   // The server is busy running a test. Try again later.
    MESETNODELAY = 122,     // Unable to set TCP/SCTP NODELAY (check perror)
    MESETMSS = 123,         // Unable to set TCP/SCTP MSS (check perror)
    MESETBUF = 124,         // Unable to set socket buffer size (check perror)
    MESETTOS = 125,         // Unable to set IP TOS (check perror)
    MESETCOS = 126,         // Unable to set IPv6 traffic class (check perror)
    MESETFLOW = 127,        // Unable to set IPv6 flow label
    MEREUSEADDR = 128,      // Unable to set reuse address on socket (check perror)
    MENONBLOCKING = 129,    // Unable to set socket to non-blocking (check perror)
    MESETWINDOWSIZE = 130,  // Unable to set socket window size (check perror)
    MEPROTOCOL = 131,       // Protocol does not exist
    MEAFFINITY = 132,       // Unable to set CPU affinity (check perror)
    MEDAEMON = 133,	    // Unable to become a daemon process
    MESETCONGESTION = 134,  // Unable to set TCP_CONGESTION
    MEPIDFILE = 135,	    // Unable to write PID file
    MEV6ONLY = 136,  	    // Unable to set/unset IPV6_V6ONLY (check perror)
    MESETSCTPDISABLEFRAG = 137, // Unable to set SCTP Fragmentation (check perror)
    MESETSCTPNSTREAM= 138,  //  Unable to set SCTP number of streams (check perror)
    MESETSCTPBINDX= 139,    // Unable to process sctp_bindx() parameters
    MESETPACING= 140,       // Unable to set socket pacing rate
    MESETBUF2= 141,	    // Socket buffer size incorrect (written value != read value)
    MEAUTHTEST = 142,       // Test authorization failed
    /* Stream errors */
    MECREATESTREAM = 200,   // Unable to create a new stream (check herror/perror)
    MEINITSTREAM = 201,     // Unable to initialize stream (check herror/perror)
    MESTREAMLISTEN = 202,   // Unable to start stream listener (check perror) 
    MESTREAMCONNECT = 203,  // Unable to connect stream (check herror/perror)
    MESTREAMACCEPT = 204,   // Unable to accepte stream connection (check perror)
    MESTREAMWRITE = 205,    // Unable to write to stream socket (check perror)
    MESTREAMREAD = 206,     // Unable to read from stream (check perror)
    MESTREAMCLOSE = 207,    // Stream has closed unexpectedly
    MESTREAMID = 208,       // Stream has invalid ID
    /* Timer errors */
    MENEWTIMER = 300,       // Unable to create new timer (check perror)
    MEUPDATETIMER = 301,    // Unable to update timer (check perror)
};

/* Server routines */
int mperf_create_pidfile(struct mperf_test *);
int mperf_delete_pidfile(struct mperf_test *);

/* JSON output routines */
int mperf_json_start(struct mperf_test *);
int mperf_json_finish(struct mperf_test *);

#ifdef __cplusplus
} /* close extern "C" */
#endif


#endif /* !__MPERF_API_H */

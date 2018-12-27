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
#include <unistd.h>
#include <string.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif

#include "mperf_thread.h"

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

/* Log printer: Error level */
#define LOG_ERROR(format, ...) \
	fprintf(stderr, "\033[31m[ERROR][T%u]\033[0m %s %d: " format "\n",	\
					thread_id, __FILE__, __LINE__, ##__VA_ARGS__);

/* Log printer: Information level */
#define LOG_INFO(format, ...) \
		fprintf(stdout, "[INFO][T%u] %s %d: " format "\n", \
						thread_id, __FILE__, __LINE__, ##__VA_ARGS__);

#ifdef TCPS_DEBUG
/* Log printer: Warning level */
#define LOG_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG][T%u] %s %d: " format "\n", \
						thread_id, __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_DEBUG(format, ...)
#endif


/* Command line rontines */
void mperf_usage(FILE *f);
int mperf_parse_args(struct mperf_test *test, int argc, char **argv);

/* Initialization */
void mperf_set_default(struct mperf_test *test);


#ifdef __cplusplus
} /* close extern "C" */
#endif


#endif /* !__MPERF_API_H */

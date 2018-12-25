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
 * NOTICE.	This software is owned by the U.S. Department of Energy.
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
 * This code is distributed under a BSD style license, see the LICENSE file
 * for complete information.
 */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#define __USE_GNU

#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>

#include <sys/param.h>

#include "mperf.h"
#include "mperf_api.h"
#include "mperf_util.h"

#include "units.h"
#include "cjson.h"

static inline int
__parse_ipaddr(struct in_addr *ip, const char *ipstr)
{
	int ret = 0;

	ret = inet_aton(ipstr, ip);
	if (!ret)
		return -1;
	return 0;
}

void
mperf_usage(FILE *f)
{
	fprintf(f, "Usage: mperf -m <mtcp_conf_file> [-s ip|-c ip] [options]\n"
			"	   mperf [-h|--help] [-v|--version]\n\n"
			"Server or Client:\n"
			"  -m, --mtcp-conf	<file>		MTCP configuration file\n"
			"  -p, --port		#			server port to listen on/connect to\n"
			"  -f, --format	 	[kmgtKMGT] 	format to report: Kbits, Mbits, Gbits, Tbits\n"
			"  -V, --verbose				more detailed output\n"
			"  -j, --json					output in JSON format\n"
			"  --logfile		<file>		send output to a log file\n"
			"  -v, --version				show version information and quit\n"
			"  -h, --help					show this message and quit\n"
			"Server specific:\n"
			"  -s, --server		<ip>		run in server mode, bind to <ip>\n"
			"Client specific:\n"
			"  -c, --client		<ip>		run in client mode, connecting to <ip>\n"
			"  -b, --bitrate	#[KMG][/#]	target bitrate in bits/sec (0 for unlimited)\n"
			"								(default unlimited for TCP)\n"
			"								(optional slash and packet count for burst mode)\n"
			"  -t, --time	  	#			time in seconds to transmit for (default %d secs)\n"
			"  -n, --bytes	  	#[KMG]		number of bytes to transmit (instead of -t)\n"
			"  -k, --blockcount #[KMG]		number of blocks (packets) to transmit (instead of -t or -n)\n"
			"  -l, --length	  	#[KMG]		length of buffer to read or write\n"
			"								(default %d KB for TCP)\n"
			"  -w, --window	  	#[KMG]		set window size / socket buffer size\n"
			"  -M, --set-mss   	#			set TCP maximum segment size (MTU - 40 bytes)\n"
			"  -N, --no-delay			set TCP no delay, disabling Nagle's Algorithm\n",
			MAX_TIME, DEFAULT_TCP_BLKSIZE);
}

int
mperf_parse_args(struct mperf_test *test, int argc, char **argv)
{
	static struct option longopts[] = {
		{"mtcp-conf", required_argument, NULL, 'm'},
		{"port", required_argument, NULL, 'p'},
		{"format", required_argument, NULL, 'f'},
		{"verbose", no_argument, NULL, 'V'},
		{"json", no_argument, NULL, 'j'},
		{"version", no_argument, NULL, 'v'},
		{"server", required_argument, NULL, 's'},
		{"client", required_argument, NULL, 'c'},
		{"bandwidth", required_argument, NULL, 'b'},
		{"time", required_argument, NULL, 't'},
		{"bytes", required_argument, NULL, 'n'},
		{"blockcount", required_argument, NULL, 'k'},
		{"length", required_argument, NULL, 'l'},
		{"window", required_argument, NULL, 'w'},
		{"set-mss", required_argument, NULL, 'M'},
		{"logfile", required_argument, NULL, OPT_LOGFILE},
		{"no-delay", no_argument, NULL, 'N'},
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	int flag;
	char *slash = NULL;
	uint32_t blksize = 0;
	uint8_t duration_flag = 0, mtcp_flag = 0;

	while ((flag = getopt_long(argc, argv, "m:p:f:Vjvs:c:b:t:n:k:l:w:M:Nh",
									longopts, NULL)) != -1) {
		switch (flag) {
			case 'm':
				test->mtcp_conf = strdup(optarg);
				mtcp_flag = 1;
				break;

			case 'p':
				test->sport = (uint16_t)atoi(optarg);
				break;

			case 'f':
				if (!optarg) {
					LOG_ERROR(NULL, "No format specifier specified for option -f,"
									" valid formats are in the set [kmgtKMGT]");
					return -1;
				}
				test->settings.unit_format = *optarg;
				if (test->settings.unit_format == 'k' ||
					test->settings.unit_format == 'K' ||
					test->settings.unit_format == 'm' ||
					test->settings.unit_format == 'M' ||
					test->settings.unit_format == 'g' ||
					test->settings.unit_format == 'G' ||
					test->settings.unit_format == 't' ||
					test->settings.unit_format == 'T') {
					break;
				}
				else {
					LOG_ERROR(NULL, "Bad format specifier %c,"
									" valid formats are in the set [kmgtKMGT]",
									optarg[0]);
					return -1;
				}

			case 'V':
				test->verbose = 1;
				break;

			case 'j':
				test->json_output = 1;
				break;

			case 'v':
				LOG_INFO(stdout, "%s (cJSON %s)\n%s",
								PACKAGE_STRING, cJSON_Version(),
								get_system_info());
				exit(0);

			case 's':
				test->role_server = 1;
				if (__parse_ipaddr(&(test->saddr), optarg) < 0) {
					LOG_ERROR(NULL, "Invalid IP address %s", optarg);
					return -1;
				}
				break;

			case 'c':
				test->role_client = 1;
				if (__parse_ipaddr(&(test->saddr), optarg) < 0) {
					LOG_ERROR(NULL, "Invalid IP address %s", optarg);
					return -1;
				}
				break;

			/* -b <bits/sec>[/<burst_size] */
			case 'b':
				slash = strchr(optarg, '/');
				// parse burst
				if (slash) {
					*slash = '\0';
					slash += 1;
					test->settings.burst = atoi(slash);
					if (test->settings.burst <= 0 ||
									test->settings.burst > MAX_BURST) {
						LOG_ERROR(NULL, "Invalid burst count %s (maximum %u)",
										optarg, MAX_BURST);
						return -1;
					}
				}

				// parse bit rate
				test->settings.rate = unit_atou_rate(optarg);
				break;

			case 't':
				test->settings.duration = (uint32_t)atoi(optarg);
				if (test->settings.duration > MAX_TIME) {
					LOG_ERROR(NULL, "Test duration is too long (maximum = %u sec)",
									MAX_TIME);
					return -1;
				}
				duration_flag = 1;
				break;

			case 'n':
				test->settings.bytes = unit_atolu(optarg);
				break;

			case 'k':
				test->settings.blocks = unit_atolu(optarg);
				break;

			case 'l':
				blksize = (uint32_t)unit_atolu(optarg);
				break;

			case 'w':
				test->settings.window_size = (uint32_t)unit_atolu(optarg);
				if (test->settings.window_size > MAX_TCP_BUFFER) {
					LOG_ERROR(NULL, "TCP window size is too big (maximum = %u bytes)",
									MAX_TCP_BUFFER);
					return -1;
				}
				break;

			case 'M':
				test->settings.mss = (uint32_t)unit_atolu(optarg);
				if (test->settings.mss > MAX_MSS) {
					LOG_ERROR(NULL, "TCP MSS is too big (maximum = %u bytes)",
									MAX_MSS);
					return -1;
				}
				break;

			case 'N':
				test->no_delay = 1;
				break;

			case OPT_LOGFILE:
				test->logfile = strdup(optarg);
				break;

			case 'h':
				mperf_usage(stdout);
				return 1;

			default:
				mperf_usage(stdout);
				return 1;
		}
	}

	if (!mtcp_flag) {
		LOG_ERROR(NULL, "No MTCP configuration file specified.");
		return -1;
	}

	// set logging to a file if specified, othrewise use the default (stdout)
	if (test->logfile) {
		test->outfile = fopen(test->logfile, "a+");
		if (test->outfile == NULL) {
			LOG_ERROR(NULL, "Failed to open logfile %s", test->logfile);
			return -1;
		}
	}
	else {
		test->outfile = stdout;
	}

	if (blksize == 0)
		blksize = DEFAULT_TCP_BLKSIZE;
	test->settings.blksize = blksize;


	if ((test->settings.bytes != 0 || test->settings.blocks != 0)
					&& !duration_flag)
		test->settings.duration = 0;

	optind = 0;

	return 0;
}

int
mperf_json_start(struct mperf_test *test)
{
	test->json_top = cJSON_CreateObject();
	if (test->json_top == NULL)
		return -1;
	test->json_start = cJSON_CreateObject();
	if (test->json_start == NULL)
		return -1;
	cJSON_AddItemToObject(test->json_top, "start", test->json_start);
	test->json_connected = cJSON_CreateArray();
	if (test->json_connected == NULL)
		return -1;
	cJSON_AddItemToObject(test->json_start, "connected", test->json_connected);
	test->json_intervals = cJSON_CreateArray();
	if (test->json_intervals == NULL)
		return -1;
	cJSON_AddItemToObject(test->json_top, "intervals", test->json_intervals);
	test->json_end = cJSON_CreateObject();
	if (test->json_end == NULL)
		return -1;
	cJSON_AddItemToObject(test->json_top, "end", test->json_end);
	return 0;
}

int
mperf_json_finish(struct mperf_test *test)
{
	/* Include server output */
	if (test->json_server_output) {
		cJSON_AddItemToObject(test->json_top, "server_output_json",
						test->json_server_output);
	}

	if (test->server_output_text) {
		cJSON_AddStringToObject(test->json_top, "server_output_text",
						test->server_output_text);
	}

	test->json_output_string = cJSON_Print(test->json_top);
	if (test->json_output_string == NULL)
		return -1;

	fprintf(test->outfile, "%s\n", test->json_output_string);
	fflush(test->outfile);
	cJSON_Delete(test->json_top);
	test->json_top = NULL;
	test->json_start = NULL;
	test->json_connected = NULL;
	test->json_intervals = NULL;
	test->json_server_output = NULL;
	test->json_end = NULL;
	return 0;
}

void
mperf_set_default(struct mperf_test *test)
{
	// settings
	test->settings.blksize = DEFAULT_TCP_BLKSIZE;
	test->settings.unit_format = 'a';
	test->settings.duration = DURATION;

	test->sport = PORT;
	test->outfile = stdout;
}

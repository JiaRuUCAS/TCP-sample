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
#include <stdint.h>
#include <netinet/tcp.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sched.h>
#include <setjmp.h>
#include <stdarg.h>

#include <sys/param.h>

#include "mperf_config.h"
#include "mperf_util.h"
#include "mperf_api.h"
#include "mperf_worker.h"

#include "cjson.h"
#include "units.h"

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
			"  -f, --format		[kmgtKMGT]	format to report: Kbits, Mbits, Gbits, Tbits\n"
			"  -V, --verbose				more detailed output\n"
			"  -v, --version				show version information and quit\n"
			"  -h, --help					show this message and quit\n"
			"Server specific:\n"
			"  -s, --server		<ip>		run in server mode, bind to <ip>\n"
			"Client specific:\n"
			"  -c, --client		<ip>		run in client mode, connecting to <ip>\n"
			"  -b, --bitrate	#[KMG][/#]	target bitrate in bits/sec (0 for unlimited)\n"
			"								(default unlimited for TCP)\n"
			"								(optional slash and packet count for burst mode)\n"
			"  -t, --time		#			time in seconds to transmit for (default %d secs)\n"
			"  -n, --bytes		#[KMG]		number of bytes to transmit (instead of -t)\n"
			"  -k, --blockcount #[KMG]		number of blocks (packets) to transmit (instead of -t or -n)\n"
			"  -l, --length		#[KMG]		length of buffer to read or write\n"
			"								(default %d KB for TCP)\n"
			"  -w, --window		#[KMG]		set window size / socket buffer size\n"
			"  -M, --set-mss	#			set TCP maximum segment size (MTU - 40 bytes)\n",
			MAX_TIME, DEFAULT_TCP_BLKSIZE);
}

int
mperf_parse_args(int argc, char **argv)
{
	static struct option longopts[] = {
		{"mtcp-conf", required_argument, NULL, 'm'},
		{"port", required_argument, NULL, 'p'},
		{"format", required_argument, NULL, 'f'},
		{"verbose", no_argument, NULL, 'V'},
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
		{"help", no_argument, NULL, 'h'},
		{NULL, 0, NULL, 0}
	};
	int flag;
	char *slash = NULL;
	uint32_t blksize = 0;
	uint8_t duration_flag = 0, mtcp_flag = 0, unit;
	struct mperf_config *conf = &global_conf;

	while ((flag = getopt_long(argc, argv, "m:p:f:Vvs:c:b:t:n:k:l:w:M:h",
									longopts, NULL)) != -1) {
		switch (flag) {
			case 'm':
				conf->mtcp_conf = strdup(optarg);
				mtcp_flag = 1;
				break;

			case 'p':
				conf->sport = (uint16_t)atoi(optarg);
				break;

			case 'f':
				if (!optarg) {
					LOG_ERROR("No format specifier specified for option -f,"
									" valid formats are in the set [kmgtKMGT]");
					return -1;
				}
				unit = unit_match(optarg[0]);
				if (unit < UNIT_MAX)
					conf->unit = unit;
				else {
					LOG_ERROR("Bad format specifier %c,"
									" valid formats are in the set [kmgtKMGT]",
									optarg[0]);
					return -1;
				}

			case 'V':
				conf->verbose = 1;
				break;

			case 'v':
				fprintf(stdout, "%s (cJSON %s)\n%s\n",
								PACKAGE_STRING, cJSON_Version(),
								get_system_info());
				exit(0);

			case 's':
				conf->role = ROLE_SERVER;
				if (__parse_ipaddr(&(conf->saddr), optarg) < 0) {
					LOG_ERROR("Invalid IP address %s", optarg);
					return -1;
				}
				break;

			case 'c':
				conf->role = ROLE_CLIENT;
				if (__parse_ipaddr(&(conf->saddr), optarg) < 0) {
					LOG_ERROR("Invalid IP address %s", optarg);
					return -1;
				}
				break;

			/* -b <bits>[/<burst_size] */
			case 'b':
				slash = strchr(optarg, '/');
				// parse burst
				if (slash) {
					*slash = '\0';
					slash += 1;
					conf->conn_conf.burst = atoi(slash);
					if (conf->conn_conf.burst <= 0 ||
									conf->conn_conf.burst > MAX_BURST) {
						LOG_ERROR("Invalid burst count %s (maximum %u)",
										optarg, MAX_BURST);
						return -1;
					}
				}

				// parse bit rate
				conf->conn_conf.rate = unit_atolu(optarg);
				break;

			case 't':
				conf->conn_conf.duration = (uint32_t)atoi(optarg);
				if (conf->conn_conf.duration > MAX_TIME) {
					LOG_ERROR("Test duration is too long (maximum = %u sec)",
									MAX_TIME);
					return -1;
				}
				duration_flag = 1;
				break;

			case 'n':
				conf->conn_conf.bytes = unit_atolu(optarg);
				break;

			case 'k':
				conf->conn_conf.blocks = unit_atolu(optarg);
				break;

			case 'l':
				blksize = (uint32_t)unit_atolu(optarg);
				break;

			case 'w':
				conf->conn_conf.window_size = (uint32_t)unit_atolu(optarg);
				if (conf->conn_conf.window_size > MAX_TCP_BUFFER) {
					LOG_ERROR("TCP window size is too big (maximum = %u bytes)",
									MAX_TCP_BUFFER);
					return -1;
				}
				break;

			case 'M':
				conf->conn_conf.mss = (uint32_t)unit_atolu(optarg);
				if (conf->conn_conf.mss > MAX_MSS) {
					LOG_ERROR("TCP MSS is too big (maximum = %u bytes)",
									MAX_MSS);
					return -1;
				}
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
		LOG_ERROR("No MTCP configuration file specified.");
		return -1;
	}

	if (blksize == 0)
		blksize = DEFAULT_TCP_BLKSIZE;
	conf->conn_conf.blksize = blksize;


	if ((conf->conn_conf.bytes != 0
					|| conf->conn_conf.blocks != 0)
					&& !duration_flag)
		conf->conn_conf.duration = 0;

	optind = 0;

	return 0;
}

int
mperf_set_test_state(uint8_t state)
{
	struct worker_context *ctx = mperf_get_worker();

	ctx->test_state = state;

	if (nwrite(ctx->ctlfd, (char *)&state, sizeof(state)) < 0) {
		LOG_ERROR("Failed write state %u to ctl sock", state);
		return -1;
	}
	return 0;
}

/* parameter exchange between client and server */
int
mperf_exchange_conf(struct worker_context *ctx)
{
	int ret = 0;

	if (ctx->role == ROLE_CLIENT)
		return mperf_send_conf(&ctx->conn_conf);

	// server part
	ret = mperf_recv_conf(&ctx->conn_conf);
	if (ret < 0) {
		LOG_ERROR("Failed to recv parameter from client");
		return ret;
	}

	// send the controll message to create streams and start the test
	if (mperf_set_test_state(TEST_STATE_CREATE_STREAM) < 0)
		return -1;

	return 0;
}

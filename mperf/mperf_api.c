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

#include "../build/config.h"

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

#include "cjson.h"

/* Try to write a PID file if requested, return -1 on an error. */
int
mperf_create_pidfile(struct mperf_test *test)
{
	int fd;
	char buf[8];

	if (!test->pidfile)
		return 0;

	/* See if the file already exists and we can read it. */
	fd = open(test->pidfile, O_RDONLY, 0);
	if (fd >= 0) {
		if (read(fd, buf, sizeof(buf) - 1) >= 0) {

			/* We read some bytes, see if they correspond to a valid PID */
			pid_t pid;

			pid = atoi(buf);
			if (pid > 0) {

				/* See if the process exists. */
				if (kill(pid, 0) == 0) {
					/*
					 * Make sure not to try to delete existing PID file by
					 * scribbling over the pathname we'd use to refer to it.
					 * Then exit with an error.
					 */
					free(test->pidfile);
					test->pidfile = NULL;
					mperf_errexit(test, "Another instance of mperf "
										"appears to be running");
				}
			}
		}
	}
	
	/*
	 * File didn't exist, we couldn't read it, or it didn't correspond to 
	 * a running process.  Try to create it. 
	 */
	fd = open(test->pidfile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd < 0)
		return -1;

	snprintf(buf, sizeof(buf), "%d", getpid()); /* no trailing newline */
	if (write(fd, buf, strlen(buf) + 1) < 0)
		return -1;

	if (close(fd) < 0)
		return -1;

	return 0;
}

/* Get rid of a PID file, return -1 on error. */
int
mperf_delete_pidfile(struct mperf_test *test)
{
	if (test->pidfile) {
		if (unlink(test->pidfile) < 0) {
			return -1;
		}
	}
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
	if (test->title)
		cJSON_AddStringToObject(test->json_top, "title", test->title);

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


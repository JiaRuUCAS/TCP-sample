#ifndef __MPERF_H__
#define __MPERF_H__

#include "../build/config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "cjson.h"
#include "queue.h"

typedef uint16_t mperf_size_t;

/* default settings */
#define PORT 5210  /* default port to listen on */
#define uS_TO_NS 1000
#define SEC_TO_US 1000000LL
#define UDP_RATE (1024 * 1024) /* 1 Mbps */
#define OMIT 0 /* seconds */
#define DURATION 10 /* seconds */

#define SEC_TO_NS 1000000000LL	/* too big for enum/const on some platforms */
#define MAX_RESULT_STRING 4096

#define UDP_BUFFER_EXTRA 1024

/* constants for command line arg sanity checks */
#define MB (1024 * 1024)
#define MAX_TCP_BUFFER (512 * MB)
#define MAX_BLOCKSIZE MB
/* Minimum size UDP send is the size of two 32-bit ints followed by a 64-bit int */
#define MIN_UDP_BLOCKSIZE (4 + 4 + 8)
/* Maximum size UDP send is (64K - 1) - IP and UDP header sizes */
#define MAX_UDP_BLOCKSIZE (65535 - 8 - 20)
#define MIN_INTERVAL 0.1
#define MAX_INTERVAL 60.0
#define MAX_TIME 86400
#define MAX_BURST 1000
#define MAX_MSS (9 * 1024)
#define MAX_STREAMS 128

struct mperf_textline {
	char *line;
	TAILQ_ENTRY(mperf_textline) textlineentries;
};

struct mperf_test {
	/* 'c'lient or 's'erver */
	char role;

	/* options */
	/* Flags */
	union {
		struct {
			/* -D option */
			uint32_t deamon			: 1;
			/* -J option - JSON output */
			uint32_t json_output	: 1;
		};
		uint32_t flags;
	};

	/* -T option */
	char *title;
	/* -P option */
	char *pidfile;

	/* file */
	FILE *outfile;

	/* cJSON handles for use when in -J mode */
	cJSON *json_top;
	cJSON *json_start;
	cJSON *json_connected;
	cJSON *json_intervals;
	cJSON *json_end;

	/* rendered JSON output if json_output is set */
	char *json_output_string;

	/* Server output (use on client side only) */
	char *server_output_text;
	cJSON *json_server_output;

	/* Server output (use on server side only) */
	TAILQ_HEAD(mperf_testlisthead, mperf_textline) server_output_list;
};



#endif /* __MPERF_H__ */

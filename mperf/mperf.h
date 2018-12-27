#ifndef __MPERF_H__
#define __MPERF_H__

#include "../config.h"

#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <mtcp_api.h>

#include "queue.h"

/* default settings */
#define PORT 5210  /* default port to listen on */
#define uS_TO_NS 1000
#define SEC_TO_US 1000000LL
#define UDP_RATE (1024 * 1024) /* 1 Mbps */
#define OMIT 0 /* seconds */
#define DURATION 10 /* seconds */

#define SEC_TO_NS 1000000000LL	/* too big for enum/const on some platforms */
#define MAX_RESULT_STRING 4096

/* constants for command line arg sanity checks */
#define MB (1024 * 1024)
#define MAX_TCP_BUFFER (512 * MB)
#define MAX_BLOCKSIZE MB
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

struct mperf_settings {
	/* TCP window size */
	uint32_t window_size;
	/* size of read/writes (-l) */
	uint32_t blksize;
	/* target data rate */
	uint64_t rate;
	/* packet burst size */
	uint32_t burst;
	/* TCP MSS */
	uint32_t mss;
	/* number of bytes to send */
	uint32_t bytes;
	/* number of blocks (packets) to send */
	uint32_t blocks;
	/* -f */
	char unit_format;
	/* time to send */
	uint32_t duration;
};

struct mperf_test {
	/* MTCP context */
	mctx_t ctx;

	/* server ip */
	struct in_addr saddr;
	/* server port */
	uint16_t sport;

	struct mperf_settings settings;

	/* options */
	/* Flags */
	union {
		struct {
			/* -V option - more detialed output */
			uint8_t verbose		: 1;
			/* run server routines */
			uint8_t role_server	: 1;
			/* run client routines */
			uint8_t role_client	: 1;
			/* -N option - disable Negal algorithm */
			uint8_t no_delay	: 1;
			/* unused bits */
			uint8_t unused		: 4;
		};
		uint8_t flags;
	};

	/* file */
	/* Path to mtcp configuration file */
	char *mtcp_conf;
};



#endif /* __MPERF_H__ */

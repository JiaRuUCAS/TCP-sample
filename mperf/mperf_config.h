#ifndef __MPERF_CONFIG_H__
#define __MPERF_CONFIG_H__

#include "../config.h"

#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include <mtcp_api.h>

#include "queue.h"

/* default settings */
#define PORT 5210  /* default port to listen on */
#define DURATION 10 /* seconds */
#define DEFAULT_TCP_BLKSIZE	(131072)
#define BACKLOG	(4096)

/* constants for command line arg sanity checks */
#define MB (1048576)
#define MAX_TCP_BUFFER (MB << 9)
#define MAX_BLOCKSIZE MB
#define MIN_INTERVAL 0.1
#define MAX_INTERVAL 60.0
#define MAX_TIME 86400
#define MAX_BURST 1000
#define MTU	1500
#define MAX_MSS (MTU - 40)
#define MAX_STREAMS 128

/* Per-connection configuration */
struct mperf_conn_config {
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
	/* time to send */
	uint32_t duration;
};

/* Global configuration */
struct mperf_config {
	/* server address */
	struct in_addr saddr;
	/* server port */
	uint16_t sport;

	/* Flags */
	union {
		struct {
			/* -V option - more detialed output */
			uint8_t verbose		: 1;
			/* Role */
			uint8_t role		: 1;
#define ROLE_SERVER	0
#define ROLE_CLIENT	1
			/* -f Option. Unit of traffic (see units.h) */
			uint8_t unit		: 3;
			/* unused bits */
			uint8_t unused		: 3;
		};
		uint8_t flags;
	};

	/* MTCP configuration file path */
	char *mtcp_conf;

	/* Template: connection configuration */
	struct mperf_conn_config conn_conf;
};

extern struct mperf_config global_conf;

/* client side: send client's connection configuration to server */
int mperf_send_conf(struct mperf_conn_config *conf);

/* server side: recv and apply client's connection configuration */
int mperf_recv_conf(struct mperf_conn_config *conf);

#endif /* __MPERF_CONFIG_H__ */

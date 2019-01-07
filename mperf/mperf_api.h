#ifndef __MPERF_API_H__
#define __MPERF_API_H__

#include <sys/time.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mperf_worker.h"
#include "mperf_config.h"

/* Command line rontines (see mperf_api.c) */
void mperf_usage(FILE *f);
int mperf_parse_args(int argc, char **argv);


/* Common routines (see mperf_api.c) */
/* set local test state and send the new state to another side */
int mperf_set_test_state(uint8_t state);


/* Server routines (see server.c) */
/* server main loop */
int mperf_server_loop(void);
/* create listener socket and bind to user-specified server ip */
int mperf_create_listener(struct worker_context *ctx,
				struct mperf_config *global);
/* accept new connection */
int mperf_accept(struct worker_context *ctx);
/* handle controlling message */
int mperf_handle_server_msg(struct worker_context *ctx);
/* init server context */
void mperf_init_server(struct worker_context *ctx);
/* destroy all server context */
void mperf_destroy_server(struct worker_context *ctx);

/* Client routines (see client.c) */
/* client main loop */
int mperf_client_loop(void);
/* connect to server */
int mperf_connect(struct worker_context *ctx,
				struct mperf_config *global, uint8_t is_ctl);
/* handle controlling message */
int mperf_handle_client_msg(struct worker_context *ctx);
/* init client context */
void mperf_init_client(struct worker_context *ctx);
/* destry client context */
void mperf_destroy_client(struct worker_context *ctx);

#endif /* __MPERF_API_H__ */

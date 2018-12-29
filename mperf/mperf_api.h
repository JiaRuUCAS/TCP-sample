#ifndef __MPERF_API_H__
#define __MPERF_API_H__

#include <sys/time.h>
#include <setjmp.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

struct mperf_config;
struct worker_context;

/* Command line rontines (see mperf_api.c) */
void mperf_usage(FILE *f);
int mperf_parse_args(int argc, char **argv);

/* Common routines (see mperf_api.c) */
/* set local test state and send the new state to another side */
int mperf_set_test_state(uint8_t state);
/* execute test parameter exchange between client and server */
int mperf_exchange_params(struct worker_context *ctx);

/* Server routines (see server.c) */
int mperf_create_listener(struct mperf_config *global);


#endif /* __MPERF_API_H__ */

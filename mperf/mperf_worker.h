#ifndef __MPERF_WORKER_H__
#define __MPERF_WORKER_H__

#include <pthread.h>
#include <mtcp_api.h>

#include "mperf_config.h"

#define THREAD_MAX	17

enum {
	TEST_STATE_UNINIT = 0,
	TEST_STATE_CREATE_STREAM,
	TEST_STATE_DONE,
};

struct worker_context {
	uint8_t core;
	pid_t pid;
	pthread_t tid;
	uint8_t role;

	/* Per-worker (per-connection, one worker only handle one
	 * connection at anytime) test configuration */
	struct mperf_conn_config conn_conf;

	/* mtcp handler */
	mctx_t mctx;
	/* epoll fd */
	int epollfd;
#define EPOLL_EVENT_MAX	1000
	/* Socket: listener fd */
	int listenfd;
	/* Socket: controller */
	int ctlfd;

	/* state of the worker */
	uint8_t worker_state;
#define WORKER_UNUSED	0
#define WORKER_UNINIT	1
#define WORKER_INITED	2
#define WORKER_CLOSE	3
#define WORKER_ERROR	4
	uint8_t done;
	/* state of test */
	uint8_t test_state;

	/* user-specified thread routine */
	void (*routine)(void *);
	void *arg;
};

extern __thread uint8_t thread_id;

struct worker_context *mperf_get_worker(void);

struct worker_context *mperf_init_worker(uint8_t core);

int mperf_create_workers(uint8_t n_worker,
				void (*routine)(void *), void *arg);

void mperf_wait_workers(void);

void mperf_destroy_workers(void);

#endif /* __MPERF_WORKER_H__ */

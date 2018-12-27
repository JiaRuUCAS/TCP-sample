#ifndef __MPERF_THREAD_H__
#define __MPERF_THREAD_H__

#include <pthread.h>
#include <mtcp_api.h>


#define THREAD_MAX	17

struct thread_context {
	uint8_t core;
	pid_t pid;
	pthread_t tid;

	/* mtcp handler */
	mctx_t mctx;
	/* epoll fd */
	int epollfd;
#define EPOLL_EVENT_MAX	1000
	/* Socket: listener fd */
	int listenfd;

	/* state of the thread */
	uint8_t state;
#define WORKER_UNUSED	0
#define WORKER_UNINIT	1
#define WORKER_INITED	2
#define WORKER_CLOSE	3
#define WORKER_ERROR	4
	uint8_t done;

	/* user-specified thread routine */
	void (*routine)(void *);
	void *arg;
};

extern __thread uint8_t thread_id;

struct thread_context *mperf_get_worker(void);

struct thread_context *mperf_init_worker(uint8_t core);

int mperf_create_workers(uint8_t n_worker,
				void (*routine)(void *), void *arg);

void mperf_wait_workers(void);

void mperf_destroy_workers(void);

#endif /* __THREAD_H__ */

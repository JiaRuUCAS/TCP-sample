#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "mperf_config.h"
#include "mperf_worker.h"
#include "mperf_util.h"

__thread uint8_t thread_id = UINT8_MAX;

static uint8_t n_thread;

static struct worker_context threads[THREAD_MAX] = {{
	.core = UINT8_MAX,
	.pid = -1,
	.tid = UINT64_MAX,
	.mctx = NULL,
	.epollfd = -1,
	.worker_state = WORKER_UNUSED,
	.test_state = TEST_STATE_UNINIT,
	.done = 0,}
};

struct worker_context *
mperf_get_worker(void)
{
	return &threads[thread_id - 1];
}

struct worker_context *
mperf_init_worker(uint8_t core) {
	struct worker_context *ctx = NULL;

	thread_id = core + 1;
	ctx = &threads[core];
	ctx->core = core;
	ctx->pid = syscall(SYS_gettid);
	ctx->tid = pthread_self();
	ctx->role = global_conf.role;

	// set cpu affinity
	mtcp_core_affinitize(ctx->core);

	// init mctx
	ctx->mctx = mtcp_create_context(core);
	if (!ctx->mctx) {
		LOG_ERROR("Failed to create mtcp context (core %u) for thread %u",
						core, thread_id);
		return NULL;
	}

	// create epollfd
	ctx->epollfd = mtcp_epoll_create(ctx->mctx, EPOLL_EVENT_MAX);
	if (ctx->epollfd == -1) {
		LOG_ERROR("Failed to create epollfd for thread %u", thread_id);
		destroy_mctx(ctx->mctx);
		return NULL;
	}

	ctx->worker_state = WORKER_INITED;

	return ctx;
}

static void __sig_handler(int signo)
{
	struct worker_context *ctx = NULL;
	uint8_t i = 0;

	LOG_DEBUG("recv SIGINT");
	if (thread_id > 0) {
		ctx = mperf_get_worker();
		ctx->done = 1;
		ctx->worker_state = WORKER_CLOSE;
	}
	else {
		for (i = 0; i < n_thread; i++) {
			ctx = &threads[i];

			if (ctx->worker_state == WORKER_UNINIT ||
							ctx->worker_state == WORKER_INITED)
				pthread_kill(ctx->tid, SIGINT);
		}
	}
}

static void *
__worker_routine(void *arg)
{
	uint8_t core = (uintptr_t)arg;
	struct worker_context *ctx = NULL;

	ctx = mperf_init_worker(core);
	if (!ctx) {
		LOG_ERROR("Failed to init thread %u", thread_id);
		pthread_exit(NULL);
		return NULL;
	}

	LOG_DEBUG("Thread (pid %d, tid %lu) runs on core %u",
					ctx->pid, ctx->tid, ctx->core);
	ctx->routine(ctx->arg);

	close_mtcp_fd(ctx->mctx, ctx->epollfd);
	destroy_mctx(ctx->mctx);

	pthread_exit(NULL);
	ctx->worker_state = WORKER_CLOSE;
	LOG_DEBUG("Thread closed");

	return NULL;
}

int mperf_create_workers(uint8_t n_worker,
				void (*routine)(void *), void *arg)
{
	uint8_t i = 0;
	struct worker_context *ctx = NULL;
	pthread_t tid;

	n_thread = n_worker;

	mtcp_register_signal(SIGINT, __sig_handler);

	for (i = 0; i < n_thread; i++) {

		ctx = &threads[i];
		ctx->worker_state = WORKER_UNINIT;
		ctx->routine = routine;
		ctx->arg = arg;

		LOG_DEBUG("create worker %u", i);
		if (pthread_create(&tid, NULL,
								__worker_routine, (void *)(uintptr_t)i)) {
			LOG_ERROR("Failed to create worker %u", i);
			ctx->worker_state = WORKER_ERROR;
			goto kill_all;
		}
		LOG_DEBUG("worker %u tid %lu", i, tid);
	}

	return 0;

kill_all:
	for (i = 0; i < n_thread; i++) {
		ctx = &threads[i];

		if (ctx->worker_state == WORKER_UNINIT ||
						ctx->worker_state == WORKER_INITED) {
			pthread_kill(ctx->tid, SIGINT);
		}
	}

	return 1;
}

void
mperf_wait_workers(void)
{
	uint8_t i = 0;
	struct worker_context *ctx = NULL;

	for (i = 0; i < n_thread; i++) {
		ctx = &threads[i];

		if (ctx->worker_state == WORKER_UNINIT ||
						ctx->worker_state == WORKER_INITED) {
			pthread_join(ctx->tid, NULL);
		}
	}
}

void
mperf_destroy_workers(void)
{
	uint8_t i = 0;
	struct worker_context *ctx = NULL;

	for (i = 0; i < n_thread; i++) {
		ctx = &threads[i];

		if (ctx->worker_state == WORKER_UNUSED ||
						ctx->worker_state == WORKER_ERROR)
			continue;

		close_mtcp_fd(ctx->mctx, ctx->epollfd);
		destroy_mctx(ctx->mctx);
	}
}

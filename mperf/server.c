#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "mperf.h"
#include "mperf_api.h"
#include "mperf_thread.h"
#include "mperf_util.h"

int
mperf_create_listener(struct mperf_test *test)
{
	struct thread_context *ctx = mperf_get_worker();
	struct mtcp_epoll_event ev;
	struct sockaddr_in saddr;
	int listener = -1, ret;

	if (ctx == NULL || ctx->state != WORKER_INITED) {
		LOG_ERROR("Cannot find thread_context");
		return 1;
	}

	listener = mtcp_socket(ctx->mctx, AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		LOG_ERROR("Failed to create listener, err %d (%s)",
						errno, strerror(errno));
		return 1;
	}

	ret = mtcp_setsock_nonblock(ctx->mctx, listener);
	if (ret < 0) {
		LOG_ERROR("Failed to set listener(%d) to non-blocking mode,"
						" err %d (%s)",
						listener, errno, strerror(errno));
		goto close_listener;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(test->sport);
	memcpy(&saddr.sin_addr, &test->saddr, sizeof(struct in_addr));

	ret = mtcp_bind(ctx->mctx, listener, (struct sockaddr *)&saddr,
					sizeof(struct sockaddr_in));
	if (ret < 0) {
		LOG_ERROR("Failed to bind to the listener(%d), err %d (%s)",
						listener, errno, strerror(errno));
		goto close_listener;
	}

	ret = mtcp_listen(ctx->mctx, listener, BACKLOG);
	if (ret < 0) {
		LOG_ERROR("mtcp_listen(mctx, %d, %u) failed, err %d (%s)",
						listener, BACKLOG, errno, strerror(errno));
		goto close_listener;
	}

	/* wait for incomint accept events */
	memset(&ev, 0, sizeof(ev));
	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = listener;
	mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_ADD, listener, &ev);

	ctx->listenfd = listener;
	return 0;

close_listener:
	close_mtcp_fd(ctx->mctx, listener);
	return 1;
}

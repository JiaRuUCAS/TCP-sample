#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "mperf_api.h"
#include "mperf_config.h"
#include "mperf_worker.h"
#include "mperf_util.h"

int
mperf_connect(struct worker_context *ctx, struct mperf_config *global,
				uint8_t is_ctl)
{
	struct mtcp_epoll_event ev;
	struct sockaddr_in saddr;
	int sock = -1, ret = 0;

	// create socket
	sock = mtcp_socket(ctx->mctx, AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		LOG_ERROR("Failed to create client socket, err %u (%s)",
						errno, strerror(errno));
		return -1;
	}

	// set non-blocking mode
	mtcp_setsock_nonblock(ctx->mctx, sock);

	saddr.sin_family = AF_INET;
	memcpy(&saddr.sin_addr, &global->saddr, sizeof(struct in_addr));
	saddr.sin_port = htons(global->sport);

	ret = mtcp_connect(ctx->mctx, sock, (struct sockaddr *)&saddr,
					sizeof(struct sockaddr_in));
	if (ret < 0 && errno != EINPROGRESS) {
		LOG_ERROR("mtcp_connect() failed, err %u (%s)", errno, strerror(errno));
		mtcp_close(ctx->mctx, sock);
		return -1;
	}

	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = sock;
	mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_ADD, sock, &ev);

	// establish control connection
	if (is_ctl) {
		ctx->ctlfd = sock;
		LOG_INFO("Controlling connection established for server %s:%u",
						inet_ntoa(global->saddr), global->sport);
	}
	// establish data connection
	else {
		ctx->datafd = sock;
		LOG_INFO("Data connection established for server %s:%u",
						inet_ntoa(global->saddr), global->sport);
	}

	return 0;
}

void
mperf_init_client(struct worker_context *ctx)
{
	memcpy(&ctx->conn_conf, &global_conf.conn_conf,
					sizeof(struct mperf_conn_config));

	ctx->ctlfd = -1;
	ctx->datafd = -1;
}

void
mperf_destroy_client(struct worker_context *ctx)
{
	close_mtcp_fd(ctx->mctx, ctx->datafd);
	close_mtcp_fd(ctx->mctx, ctx->ctlfd);
}

int
mperf_handle_client_msg(struct worker_context *ctx)
{
	int ret = 0;
	uint8_t state;

	ret = nread(ctx->ctlfd, (char *)&state, sizeof(uint8_t));
	if (ret == 0) {
		LOG_ERROR("Server has unexpectedly closed the connection");
		ctx->test_state = TEST_STATE_END;
		return 0;
	}
	else if (ret < 0) {
		LOG_ERROR("Failed to recv message from server, err %u (%s)",
						errno, strerror(errno));
		return -1;
	}

	switch (state) {
		case TEST_STATE_PARAM_EXCHANGE:
			if (ctx->test_state == TEST_STATE_START) {
				// send configuration
				ret = mperf_send_conf(&ctx->conn_conf);
				if (ret < 0) {
					LOG_ERROR("Failed to send config to server");
					mperf_set_test_state(TEST_STATE_ERROR);
					return -1;
				}

				// update state
				ctx->test_state = TEST_STATE_PARAM_EXCHANGE;
			}
			else {
				LOG_ERROR("Wrong state %u when receiving PARAM_EXCHANGE",
								ctx->test_state);
				return -1;
			}
			break;

		case TEST_STATE_CREATE_STREAM:
			if (ctx->test_state == TEST_STATE_PARAM_EXCHANGE) {
				// create data connection
				ret = mperf_connect(ctx, &global_conf, 0 /* is_ctl */);
				if (ret < 0) {
					LOG_ERROR("Failed to connect to server");
					mperf_set_test_state(TEST_STATE_ERROR);
					return -1;
				}

				// update state
				ctx->test_state = TEST_STATE_CREATE_STREAM;
			}
			else {
				LOG_ERROR("Wrong state %u when receiving CREATE_STREAM",
								ctx->test_state);
				return -1;
			}
			break;

		case TEST_STATE_RUNNING:
			if (ctx->test_state == TEST_STATE_CREATE_STREAM) {
				ctx->test_state = TEST_STATE_RUNNING;

				// test codes
				mperf_set_test_state(TEST_STATE_END);
				ctx->test_state = TEST_STATE_END;
				return 1;

				// TODO send data
			}
			else {
				LOG_ERROR("Wrong state %u when receiving RUNNING",
								ctx->test_state);
				return -1;
			}

		case TEST_STATE_ERROR:
			ctx->test_state = TEST_STATE_ERROR;
			return 1;

		default:
			LOG_ERROR("Unknown state %u", state);
			return -1;
	}

	return 0;
}

/* client main loop */
int
mperf_client_loop(void)
{
	int ret = 0;
	struct worker_context *ctx = mperf_get_worker();
//	struct client_context *client = &ctx->client;
	struct mtcp_epoll_event events[EPOLL_EVENT_MAX];
	struct mperf_config *global = &global_conf;
	int nevent = 0, i = 0;

	// init client context
	mperf_init_client(ctx);

	// connect to server
	ret = mperf_connect(ctx, global, 1 /* is_ctl */);

	ctx->test_state = TEST_STATE_START;

	while (!ctx->done) {
		nevent = mtcp_epoll_wait(ctx->mctx, ctx->epollfd, events,
						EPOLL_EVENT_MAX, -1);
		if (nevent < 0) {
			if (errno != EINTR) {
				LOG_ERROR("mtcp_epoll_wait() failed, err %u (%s)",
								errno, strerror(errno));
				ret = -1;
			}
			ret = 0;
			break;
		}

		for (i = 0; i < nevent; i++) {
			struct mtcp_epoll_event *pev = &events[i];

			// control message
			if (pev->data.sockid == ctx->ctlfd) {
				ret = mperf_handle_client_msg(ctx);
				if (ret < 0) {
					LOG_ERROR("Failed to handle message");
					ret = -1;
					break;
				}
				else if (ret > 0)
					break;
			}
			// data connection
			else if (pev->data.sockid == ctx->datafd) {
			
			}
		}

		if (ctx->test_state == TEST_STATE_END ||
						ctx->test_state == TEST_STATE_ERROR) {
			if (ctx->test_state == TEST_STATE_END) {
				LOG_INFO("Test (server %s:%u) finished",
								inet_ntoa(global->saddr), global->sport);
				ret = 0;
			}
			else {
				LOG_ERROR("Test (server %s:%u) failed",
								inet_ntoa(global->saddr), global->sport);
				ret = -1;
			}
			break;
		}
	}

	ctx->done = 1;
	mperf_destroy_client(ctx);

	return ret;
}

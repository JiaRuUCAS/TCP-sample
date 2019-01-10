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
mperf_create_listener(struct worker_context *ctx,
				struct mperf_config *global)
{
	struct mtcp_epoll_event ev;
	struct sockaddr_in saddr;
	int listener = -1, ret;

	if (ctx == NULL || ctx->worker_state != WORKER_INITED) {
		LOG_ERROR("Cannot find worker_context");
		return -1;
	}

	listener = mtcp_socket(ctx->mctx, AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		LOG_ERROR("Failed to create listener, err %d (%s)",
						errno, strerror(errno));
		return -1;
	}

	ret = mtcp_setsock_nonblock(ctx->mctx, listener);
	if (ret < 0) {
		LOG_ERROR("Failed to set listener(%d) to non-blocking mode,"
						" err %d (%s)",
						listener, errno, strerror(errno));
		goto close_listener;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(global->sport);
	memcpy(&saddr.sin_addr, &global->saddr, sizeof(struct in_addr));

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

	ctx->server.listenfd = listener;
	return 0;

close_listener:
	close_mtcp_fd(ctx->mctx, listener);
	return -1;
}

int
mperf_accept(struct worker_context *ctx)
{
	struct server_context *server = &ctx->server;
	struct sockaddr_in caddr;
	socklen_t clen;
	int sock = -1;
	uint8_t deny = TEST_STATE_ACCESS_DENY;
	struct mtcp_epoll_event ev;

	clen = sizeof(struct sockaddr_in);
	sock = mtcp_accept(ctx->mctx, server->listenfd,
							(struct sockaddr *)&caddr, &clen);
	if (sock < 0) {
		LOG_ERROR("Failed to accept new client, err %d (%s)",
						errno, strerror(errno));
		return -1;
	}

	/* server is free, accept new client's control connection */
	if (ctx->ctlfd == -1) {
		ctx->ctlfd = sock;

		// set non-blocking mode
		mtcp_setsock_nonblock(ctx->mctx, sock);

		// add controller to epoll list
		memset(&ev, 0, sizeof(struct mtcp_epoll_event));
		ev.events = MTCP_EPOLLIN | MTCP_EPOLLET;
		ev.data.sockid = sock;
		mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_ADD, sock, &ev);

		// set local state and send to the client to start parameter exchange
		if (mperf_set_test_state(TEST_STATE_PARAM_EXCHANGE) < 0) {
			mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_DEL, sock, NULL);
			mtcp_close(ctx->mctx, ctx->ctlfd);
			return -1;
		}

		server->cli_ctl_port = ntohs(caddr.sin_port);
		memcpy(&(server->cli_ip), &caddr.sin_addr, sizeof(struct in_addr));

		LOG_INFO("Controller(%d) established for client %s:%u",
						ctx->ctlfd, inet_ntoa(server->cli_ip), server->cli_ctl_port);
		return 0;
	}
	/* If it's in the CREATE_STREAM stage, accept data connection */
	else if (ctx->test_state == TEST_STATE_CREATE_STREAM && ctx->datafd == -1) {
		// check client ip
		LOG_DEBUG("client ip %s, recv ip %s",
						inet_ntoa(server->cli_ip), inet_ntoa(caddr.sin_addr));
		if (memcmp(&(server->cli_ip), &caddr.sin_addr, sizeof(struct in_addr)) != 0) {
			goto access_deny;
		}

 		// set non-blocking mode
		mtcp_setsock_nonblock(ctx->mctx, sock);

		// add data connection to epoll list
		memset(&ev, 0, sizeof(struct mtcp_epoll_event));
		ev.events = MTCP_EPOLLIN;
		ev.data.sockid = sock;
		mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_ADD, sock, &ev);

		// TODO init test

		// set local state and send to the client to start parameter exchange
		if (mperf_set_test_state(TEST_STATE_RUNNING) < 0) {
			mtcp_epoll_ctl(ctx->mctx, ctx->epollfd, MTCP_EPOLL_CTL_DEL, sock, NULL);
			mtcp_close(ctx->mctx, sock);
			return -1;
		}

		ctx->datafd = sock;
		server->cli_data_port = ntohs(caddr.sin_port);

		LOG_INFO("Data connection(%d) established for client %s:%u",
						ctx->datafd, inet_ntoa(server->cli_ip), server->cli_data_port);
		return 0;

	}

access_deny:
	if (nwrite(sock, (char *)&deny, sizeof(uint8_t)) < 0) {
		LOG_ERROR("Failed to send ACCESS_DENY to client %s:%u, err %d (%s)",
						inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port),
						errno, strerror(errno));
	} else {
		LOG_INFO("Deny access from client %s:%u",
						inet_ntoa(caddr.sin_addr), ntohs(caddr.sin_port));
	}
	close_mtcp_fd(ctx->mctx, sock);

	return -1;
}

int
mperf_handle_server_msg(struct worker_context *ctx)
{
	int ret = 0;
	uint8_t state;

	ret = nread(ctx->ctlfd, (char *)&state, sizeof(uint8_t));
	if (ret == 0) {
		LOG_ERROR("The client has unexpectedly closed the connection,"
						" current state %u", ctx->test_state);
		ctx->test_state = TEST_STATE_END;
		return 0;
	}
	else if (ret < 0) {
		LOG_ERROR("Failed to recv message from client, err %d (%s)",
						errno, strerror(errno));
		return -1;
	}

	switch (state) {
		case TEST_STATE_PARAM_EXCHANGE:
			if (ctx->test_state == TEST_STATE_PARAM_EXCHANGE) {
				// recv configuration
				ret = mperf_recv_conf(&ctx->conn_conf);
				if (ret < 0) {
					LOG_ERROR("Failed to read config from client");
					mperf_set_test_state(TEST_STATE_ERROR);
					return -1;
				}

				// set and send TEST_STATE_CREATE_STREAM
				ret = mperf_set_test_state(TEST_STATE_CREATE_STREAM);
				if (ret < 0) {
					LOG_ERROR("Failed to send CREATE_STREAM");
					ctx->test_state = TEST_STATE_ERROR;
					return -1;
				}
			}
			else {
				LOG_ERROR("Wrong state %u when receiving PARAM_EXCHANGE",
								ctx->test_state);
				mperf_set_test_state(TEST_STATE_ERROR);
				return -1;
			}
			break;

		case TEST_STATE_ERROR:
			ctx->test_state = TEST_STATE_ERROR;
			return 1;

		case TEST_STATE_END:
			ctx->test_state = TEST_STATE_END;
			return 1;

		default:
			LOG_ERROR("Unknown state %u", state);
			return -1;
	}

	return 0;
}

static inline int
mperf_recv(struct worker_context *ctx)
{
	struct server_context *server = &ctx->server;
	ssize_t nread = 0, nleft;

	nleft = ctx->conn_conf.blksize;
	while (nleft > 0) {
		nread = mtcp_read(ctx->mctx, ctx->datafd,
						server->rx_buffer, RX_BUF_SIZE);

		if (nread < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
				return -1;
		}
		else if (nread == 0)
			return -1;

		server->rx_bytes += nread;
		nleft -= nread;
	}

	return 0;
}

void
mperf_init_server(struct worker_context *ctx)
{
	memset(&ctx->conn_conf, 0, sizeof(struct mperf_conn_config));
	ctx->conn_conf.blksize = DEFAULT_TCP_BLKSIZE;
	ctx->conn_conf.duration = DURATION;

	ctx->server.listenfd = -1;
	ctx->server.cli_ip.s_addr = 0;
	ctx->server.cli_ctl_port = 0;
	ctx->server.cli_data_port = 0;
	ctx->server.rx_bytes = 0;
	ctx->ctlfd = -1;
	ctx->datafd = -1;
}

void
mperf_destroy_server(struct worker_context *ctx)
{
	ctx->server.cli_ip.s_addr = 0;
	ctx->server.cli_ctl_port = 0;
	ctx->server.cli_data_port = 0;

	LOG_DEBUG("Clear server");

	close_mtcp_fd(ctx->mctx, ctx->datafd);
	close_mtcp_fd(ctx->mctx, ctx->ctlfd);
	close_mtcp_fd(ctx->mctx, ctx->server.listenfd);
}

/* server main loop */
int
mperf_server_loop(void)
{
	int ret = 0;
	struct worker_context *ctx = mperf_get_worker();
	struct server_context *server = &(ctx->server);
	struct mperf_config *global = &global_conf;
//	struct mperf_conn_config *local = &(ctx->conn_conf);
//	uint8_t accepted = 0;
	struct mtcp_epoll_event events[EPOLL_EVENT_MAX];
	int nevent = 0, i = 0;

	// init server context
	mperf_init_server(ctx);

	ret = mperf_create_listener(ctx, global);
	if (ret) {
		LOG_ERROR("Failed to create listener");
		return 1;
	}

	ctx->test_state = TEST_STATE_START;

	while (!ctx->done) {
		nevent = mtcp_epoll_wait(ctx->mctx, ctx->epollfd,
						events, EPOLL_EVENT_MAX, -1);
		if (nevent < 0) {
			if (errno != EINTR) {
				LOG_ERROR("mtcp_epoll_wait() failed, err %d (%s)",
								errno, strerror(errno));
				ret = -1;
			}
			ret = 0;
			break;
		}

		for (i = 0; i < nevent; i++) {
			struct mtcp_epoll_event *pev = &events[i];

			/* if the event is for the listener, new connection arrived. */
			if (pev->data.sockid == server->listenfd) {
				LOG_DEBUG("listenfd EPOLLIN");
				if (mperf_accept(ctx) < 0) {
					LOG_ERROR("mperf_accept() failed, err %d (%s)",
									errno, strerror(errno));
					ret = -1;
					break;
				}
			}
			/* if the event is for the controller, we receive a new state */
			else if (pev->data.sockid == ctx->ctlfd) {
				LOG_DEBUG("ctlfd EPOLLIN");

				ret = mperf_handle_server_msg(ctx);

				if (ret < 0) {
					LOG_ERROR("Failed to handle message");
					break;
				}
				else if (ret > 0)
					break;
			}
			/* if the event is for the data connection, receive data */
			else if (pev->data.sockid == ctx->datafd) {
				LOG_DEBUG("datafd EPOLLIN");

				ret = mperf_recv(ctx);
				if (ret < 0) {
					ctx->test_state = TEST_STATE_ERROR;
					LOG_DEBUG("Failed to recv data");
					break;
				}
			}
		}

		if (ctx->test_state == TEST_STATE_END ||
						ctx->test_state == TEST_STATE_ERROR) {
			if (ctx->test_state == TEST_STATE_END) {
				LOG_INFO("Test (client %s:%u) finished",
								inet_ntoa(server->cli_ip), server->cli_data_port);
			}
			else {
				LOG_ERROR("Test (client %s:%u) failed",
								inet_ntoa(server->cli_ip), server->cli_data_port);
			}

			// TODO stop statistics and print summary
			break;
		}
	}

	ctx->done = 1;
	mperf_destroy_server(ctx);
	return ret;
}

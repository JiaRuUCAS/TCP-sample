#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#define ECHO_PORT		31700
#define LISTEN_BACKLOG	16
#define MAX_EVENT_NB	32
#define BUF_SIZE		2048
#define SERVER_CORE		0

//static struct in_addr server_ip = {
//	.s_addr = 0,
//};

static char *mtcp_conf_file = NULL;

static int loop = 1;
static pthread_t server_thread;

static void sighandler(int signo)
{
	pthread_t pid;

	if (signo == SIGINT) {
		pid = pthread_self();

		fprintf(stdout, "thread %u recv SIGINT\n", (unsigned)pid);

		if (pid == server_thread) {
			loop = 0;
		}
		else {
			if (loop) {
				pthread_kill(server_thread, signo);
			}
		}
	}
}

static void usage(void)
{
//	fprintf(stdout, "./echo-mtcp-server -s <server ip> -f <mtcp configuration file>\n");
	fprintf(stdout, "./echo-mtcp-server -f <mtcp configuration file>\n");
}

static int
parse_options(int argc, char **argv)
{
	int o = 0;

	while (-1 != (o = getopt(argc, argv, "f:h"))) {
		switch (o) {
//			case 's':
//				inet_aton(optarg, &server_ip);
//				break;
			case 'f':
				mtcp_conf_file = strdup(optarg);
				break;
			case 'h':
				usage();
				exit(1);
			default:
				usage();
				break;
		}
	}

//	if (server_ip.s_addr == 0) {
//		fprintf(stderr, "Server IP is not set.\n");
//		return 1;
//	}

	if (mtcp_conf_file == NULL) {
		fprintf(stderr, "No mtcp configuration file.\n");
		return 1;
	}

	return 0;
}

static int
create_listener(mctx_t mctx, int epollfd)
{
	int listener, ret = 0;
	struct sockaddr_in saddr;
	struct mtcp_epoll_event ev;

	listener = mtcp_socket(mctx, AF_INET, SOCK_STREAM, 0);
	if (listener < 0) {
		fprintf(stderr, "Failed to create listening socket\n");
		return -1;
	}

	ret = mtcp_setsock_nonblock(mctx, listener);
	if (ret < 0) {
		fprintf(stderr, "Failed to set socket to nonblocking mode.\n");
		goto close_listener;
	}

	saddr.sin_family = AF_INET;
//	memcpy(&(saddr.sin_addr.s_addr), &server_ip, sizeof(server_ip));
	saddr.sin_addr.s_addr = INADDR_ANY;
	saddr.sin_port = htons(ECHO_PORT);

	ret = mtcp_bind(mctx, listener, (struct sockaddr *)&saddr,
					sizeof(struct sockaddr_in));
	if (ret < 0) {
		fprintf(stderr, "Failed to bind listening socket\n");
		goto close_listener;
	}

	ret = mtcp_listen(mctx, listener, LISTEN_BACKLOG);
	if (ret < 0) {
		fprintf(stderr, "mtcp_listen() failed\n");
		goto close_listener;
	}

	ev.events = MTCP_EPOLLIN;
	ev.data.sockid = listener;
	mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, listener, &ev);

	return listener;

close_listener:
	mtcp_close(mctx, listener);
	return -1;
}

static void
close_conn(mctx_t mctx, int epollfd, int sockid)
{
	mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_DEL, sockid, NULL);
	mtcp_close(mctx, sockid);
}

static void *
server_loop(void *arg)
{
	mctx_t mctx;
	int epollfd = -1, listener = -1, connfd = -1;
	int i, ret;
	struct mtcp_epoll_event events[MAX_EVENT_NB], ev;
	struct sockaddr_in caddr;
	socklen_t clen;
	char buf[BUF_SIZE] = {0};

	mtcp_core_affinitize(SERVER_CORE);

	mctx = mtcp_create_context(SERVER_CORE);
	if (!mctx) {
		fprintf(stderr, "Failed to create mtcp context.\n");
		loop = 0;
		return NULL;
	}

	epollfd = mtcp_epoll_create(mctx, MAX_EVENT_NB);
	if (epollfd < 0) {
		fprintf(stderr, "Failed to create epoll descriptor\n");
		goto destroy_ctx;
	}

	listener = create_listener(mctx, epollfd);
	if (listener < 0) {
		fprintf(stderr, "Failed to create listener\n");
		goto close_epoll;
	}

	while (loop) {
		int nb_fd;

		nb_fd = mtcp_epoll_wait(mctx, epollfd, events, MAX_EVENT_NB, -1);
		if (nb_fd < 0) {
			fprintf(stderr, "mtcp_epoll_wait(epollfd) failed , errno %u\n", errno);
			break;
		}

		for (i = 0; i < nb_fd; i++) {
			if (events[i].data.sockid == listener) {
				clen = sizeof(caddr);
				while ((connfd = mtcp_accept(mctx, listener, (struct sockaddr *)&caddr,
												&clen)) > 0) {
					ret = mtcp_setsock_nonblock(mctx, connfd);
					if (ret < 0) {
						fprintf(stderr, "mtcp_setsock_nonblock(connfd) failed.\n");
						continue;
					}

					fprintf(stdout, "Connect from %s:%u\n",
									inet_ntoa(caddr.sin_addr),
									ntohs(caddr.sin_port));
					ev.data.sockid = connfd;
					ev.events = MTCP_EPOLLIN;

					mtcp_epoll_ctl(mctx, epollfd, MTCP_EPOLL_CTL_ADD, connfd, &ev);
				}
			}
			else if (events[i].events & MTCP_EPOLLERR) {
				int err;
				socklen_t errlen = sizeof(err);

				fprintf(stderr, "Error on socket %d", events[i].data.sockid);
				if (mtcp_getsockopt(mctx, events[i].data.sockid, SOL_SOCKET, SO_ERROR,
										(void *)&err, &errlen) == 0) {
					if (err != ETIMEDOUT) {
						fprintf(stderr, "Error on socket %d: %s\n",
										events[i].data.sockid, strerror(err));
					}
				}
				else {
					perror("mtcp_getsockopt");
				}
				close_conn(mctx, epollfd, events[i].data.sockid);
			}
			else {
				int nsend, nread, offset = 0;

				connfd = events[i].data.sockid;

				if (connfd < 0)
					continue;

				memset(buf, 0, sizeof(buf));

				nread = mtcp_recv(mctx, connfd, buf, BUF_SIZE, 0);
				if (nread <= 0) {
					if (nread == 0)
						fprintf(stdout, "Recv FIN from client\n");
					close_conn(mctx, epollfd, connfd);
					continue;
				}

				nsend = nread;
				while (nsend > 0) {
					ret = mtcp_write(mctx, connfd, buf + offset, nsend);
					if (ret <= 0) {
						break;
					}
					offset += ret;
					nsend -= ret;
				}
			}
		}	// for each event
	}	// while (1)

	mtcp_close(mctx, listener);

close_epoll:
	mtcp_close(mctx, epollfd);

destroy_ctx:
	mtcp_destroy_context(mctx);

	pthread_exit(NULL);

	return NULL;
}

int main(int argc, char **argv)
{
	int ret;

	ret = parse_options(argc, argv);
	if (ret) {
		usage();
		return 1;
	}

	ret = mtcp_init(mtcp_conf_file);
	if (ret) {
		fprintf(stderr, "Failed to init mtcp\n");
		return 1;
	}

	mtcp_register_signal(SIGINT, sighandler);

	if (pthread_create(&server_thread, NULL, server_loop, NULL)) {
		perror("pthread_create");
		return 1;
	}

	pthread_join(server_thread, NULL);

	mtcp_destroy();

	return 0;
}

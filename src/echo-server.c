#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>

#define ECHO_PORT		31700
#define LISTEN_BACKLOG	16
#define MAX_EVENT_NB	32
#define BUF_SIZE		2048

static int
__set_nonblocking(int sock)
{
	int opts;

	opts=fcntl(sock, F_GETFL);

	if (opts < 0) {
		perror("fcntl(sock,GETFL)");
		return -1;
	}

	opts = opts | O_NONBLOCK;

	if(fcntl(sock, F_SETFL, opts) < 0) {  
		perror("fcntl(sock,SETFL,opts)");
		return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int i, ret;
	int listenfd, connfd, sockfd, epollfd;
	char buf[BUF_SIZE];
	socklen_t cli_len;
	struct epoll_event ev, events[MAX_EVENT_NB];
	struct sockaddr_in server_addr, cli_addr;

	if (argc < 2) {
		fprintf(stderr, "Usage: ./echo-server <server ip>\n");
		return 1;
	}

	epollfd = epoll_create(256);
	if (epollfd == -1) {
		fprintf(stderr, "epoll_create() failed, errno %u\n", errno);
		return 1;
	}

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if (listenfd == -1) {
		fprintf(stderr, "socket() failed, errno %u\n", errno);
		return 1;
	}

	ret = __set_nonblocking(listenfd);
	if (ret < 0) {
		fprintf(stderr, "__set_nonblocking() failed, ret %d\n", ret);
		return 1;
	}

	ev.data.fd = listenfd;
	ev.events = EPOLLIN;
	ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev);
	if (ret < 0) {
		fprintf(stderr, "epoll_ctl(add listenfd) failed, ret %d\n", ret);
		return 1;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	inet_aton(argv[1], &(server_addr.sin_addr));
	server_addr.sin_port = htons(ECHO_PORT);

	ret = bind(listenfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if (ret < 0) {
		fprintf(stderr, "bind() failed, ret %d\n", ret);
		return 1;
	}

	ret = listen(listenfd, LISTEN_BACKLOG);
	if (ret < 0) {
		fprintf(stderr, "listen() failed, ret %d\n", ret);
		return 1;
	}

	while (1) {
		int nb_fd;

		nb_fd = epoll_wait(epollfd, events, MAX_EVENT_NB, -1);
		if (nb_fd == -1) {
			fprintf(stderr, "epoll_wait(epollfd) failed , errno %u\n", errno);
			break;
		}

		for (i = 0; i < nb_fd; i++) {
			if (events[i].data.fd == listenfd) {
				fprintf(stdout, "listenfd EPOLLIN\n");
				cli_len = sizeof(cli_addr);
				while ((connfd = accept(listenfd, (struct sockaddr *)&cli_addr,
												&cli_len)) > 0) {
					ret = __set_nonblocking(connfd);
					if (ret < 0) {
						fprintf(stderr, "__set_nonblocking(connfd) failed.\n");
						continue;
					}

					fprintf(stdout, "Connect from %s:%u\n",
									inet_ntoa(cli_addr.sin_addr),
									ntohs(cli_addr.sin_port));
					ev.data.fd = connfd;
					ev.events = EPOLLIN;

					ret = epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev);
					if (ret < 0) {
						fprintf(stderr, "epoll_ctl(add connfd) failed, errno %u\n",
										errno);
						continue;
					}
				}
			}
			else {
				sockfd = events[i].data.fd;

				if (sockfd < 0)
					continue;

				memset(buf, 0, sizeof(buf));

				ret = recv(events[i].data.fd, buf, BUF_SIZE, 0);
				if (ret <= 0) {
					if (ret == 0)
						fprintf(stdout, "Recv FIN from client\n");
					close(events[i].data.fd);
					epoll_ctl(epollfd, EPOLL_CTL_DEL,
									events[i].data.fd, &ev);
					continue;
				}

				ret = send(events[i].data.fd, buf, (size_t)ret, 0);
				if (ret < 0) {
					fprintf(stderr, "Failed to read\n");
				}
			}
		}	// for each event
	}	// while (1)

	close(epollfd);
	close(listenfd);

	return 0;
}

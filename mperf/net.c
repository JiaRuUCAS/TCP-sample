#include "../config.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <assert.h>
#include <string.h>
#include <limits.h>

#include <mtcp_api.h>
#include <mtcp_epoll.h>

#include "timer.h"
#include "net.h"
#include "mperf.h"
#include "mperf_api.h"

/* make tcp connection to server */
int
netdial(struct mperf_test *test)
{
	struct sockaddr_in addr;
	int sockid, ret = 0;

	sockid = mtcp_socket(test->ctx, AF_INET, SOCK_STREAM, 0);
	if (sockid < 0) {
		LOG_ERROR(test->outfile, "Failed to create mtcp socket");
		return -1;
	}

	ret = mtcp_setsock_nonblock(test->ctx, sockid);
	if (ret < 0) {
		LOG_ERROR(test->outfile, "Failed to set socket %d to non-blocking",
						sockid);
		mtcp_close(test->ctx, sockid);
		return -1;
	}

	addr.sin_family = AF_INET;
	memcpy(&(addr.sin_addr.s_addr), &(test->saddr), sizeof(test->saddr));
	addr.sin_port = htons(test->sport);

	ret = mtcp_connect(test->ctx, sockid, (struct sockaddr *)&addr,
					sizeof(struct sockaddr_in));
	if (ret < 0) {
		if (errno != EINPROGRESS) {
			LOG_ERROR(test->outfile, "Failed to exec mtcp_connect");
			mtcp_close(test->ctx, sockid);
			return -1;
		}
	}

	return sockid;
}

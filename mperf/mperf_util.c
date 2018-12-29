#include "../config.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>

#include <mtcp_api.h>

#include "mperf_util.h"
#include "mperf_worker.h"

const char *
get_system_info(void)
{
	static char buf[1024];
	struct utsname uts;

	memset(buf, 0, 1024);
	uname(&uts);

	snprintf(buf, sizeof(buf), "%s %s %s %s %s",
					uts.sysname, uts.nodename, uts.release,
					uts.version, uts.machine);

	return buf;
}

int
nread(int fd, char *buf, size_t count)
{
	register ssize_t r;
	register size_t nleft = count;
	mctx_t mctx = mperf_get_worker()->mctx;

	while (nleft > 0) {
		r = mtcp_read(mctx, fd, buf, nleft);
		if (r < 0) {
			if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			else
				return NET_HARDERROR;
		}
		else if (r == 0)
			break;

		nleft -= r;
		buf += r;
	}

	return count - nleft;
}

int
nwrite(int fd, const char *buf, size_t count)
{
	register ssize_t r;
	register size_t nleft = count;
	mctx_t mctx = mperf_get_worker()->mctx;

	while (nleft > 0) {
		r = mtcp_write(mctx, fd, buf, nleft);
		if (r < 0) {
			switch (errno) {
				case EINTR:
				case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
				case EWOULDBLOCK:
#endif
					return count - nleft;
				case ENOBUFS:
					return NET_SOFTERROR;
				default:
					return NET_HARDERROR;
			}
		}
		else if (r == 0)
			return NET_SOFTERROR;

		nleft -= r;
		buf += r;
	}

	return count;
}

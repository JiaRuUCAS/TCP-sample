#include "../config.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <sys/utsname.h>
#include <errno.h>

#include "mperf.h"
#include "mperf_api.h"
#include "mperf_util.h"

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

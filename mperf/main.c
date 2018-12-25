#include "../config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#ifdef HAVE_STDINT_H
#include <stdint.h>
#endif
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <mtcp_api.h>

#include "mperf.h"
#include "mperf_api.h"


int main(int argc, char **argv)
{
	struct mperf_test *test;
	int ret = 0;

	test = (struct mperf_test *)malloc(sizeof(struct mperf_test));
	if (test == NULL) {
		LOG_ERROR(NULL, "Failed to allocate memory for mperf_test");
		return 1;
	}
	memset(test, 0, sizeof(struct mperf_test));

	mperf_set_default(test);

	ret = mperf_parse_args(test, argc, argv);
	if (ret) {
		LOG_ERROR(NULL, "Invalid parameters");
		if (ret < 0)
			mperf_usage(stdout);
		ret = 1;
		goto free_test;
	}

	LOG_INFO(test->outfile, "init mtcp");
	ret = mtcp_init(test->mtcp_conf);
	if (ret) {
		LOG_ERROR(test->outfile, "Failed to init mtcp");
		ret = 1;
		goto free_test;
	}

	LOG_INFO(test->outfile, "run test");


	mtcp_destroy();

free_test:
	free(test);

	return ret;
}

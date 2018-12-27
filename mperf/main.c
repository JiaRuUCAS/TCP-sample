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
#include "mperf_thread.h"

static void
__worker_thread(void *arg)
{
	struct thread_context *ctx = NULL;

	ctx = mperf_get_worker();

	LOG_DEBUG("Enter __worker_thread loop");

	while (!ctx->done) {
		// TODO
		usleep(100);
	}

	LOG_DEBUG("Exit __worker_thread loop");
}

int main(int argc, char **argv)
{
	struct mperf_test *test = NULL;
	struct mtcp_conf mtcp_cfg;
	uint8_t n_worker = 0;
	int ret = 0;

	// init master thread info
	thread_id = 0;

	test = (struct mperf_test *)malloc(sizeof(struct mperf_test));
	if (test == NULL) {
		LOG_ERROR("Failed to allocate memory for mperf_test");
		return 1;
	}
	memset(test, 0, sizeof(struct mperf_test));

	mperf_set_default(test);

	ret = mperf_parse_args(test, argc, argv);
	if (ret) {
		if (ret < 0) {
			mperf_usage(stdout);
			LOG_ERROR("Invalid parameters");
		}
		ret = 1;
		goto free_test;
	}

	LOG_DEBUG("init mtcp");
	ret = mtcp_init(test->mtcp_conf);
	if (ret) {
		LOG_ERROR("Failed to init mtcp");
		ret = 1;
		goto free_test;
	}

	mtcp_getconf(&mtcp_cfg);
	n_worker = mtcp_cfg.num_cores > 0 ? mtcp_cfg.num_cores : 1;
	LOG_DEBUG("use %u cores --> create %u worker threads",
					mtcp_cfg.num_cores, n_worker);

	// create worker thread
	ret = mperf_create_workers(n_worker, __worker_thread, NULL);
	if (ret) {
		LOG_ERROR("Failed to create workers");
		goto exit;
	}


	LOG_DEBUG("Master thread run.....");

	usleep(100);

exit:
	mperf_wait_workers();
	LOG_DEBUG("All workers exit.");

	mperf_destroy_workers();
	LOG_DEBUG("All workers are destroyed.");

	mtcp_destroy();
	LOG_DEBUG("MTCP destroyed");

free_test:
	free(test);

	return ret;
}

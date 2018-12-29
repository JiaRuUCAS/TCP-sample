#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "mperf_worker.h"
#include "mperf_util.h"
#include "mperf_config.h"

#include "cjson.h"

struct mperf_config global_conf = {
	.saddr.s_addr = 0,
	.sport = PORT,
	.flags = 0,
	.mtcp_conf = NULL,
	.conn_conf = {
		.window_size = 0,
		.blksize = DEFAULT_TCP_BLKSIZE,
		.rate = 0,
		.burst = 0,
		.mss = 0,
		.bytes = 0,
		.blocks = 0,
		.duration = DURATION,
	},
};

static cJSON *
JSON_read(int fd)
{
	uint32_t hsize, nsize;
	char *str;
	cJSON *json = NULL;
	int rc;

	/*
	 * Read a four-byte integer, which is the length of the JSON to follow.
	 * Then read the JSON into a buffer and parse it. Return a parsed JSON
	 * structure, NULL if there was an error.
	 */
	if (nread(fd, (char*) &nsize, sizeof(nsize)) >= 0) {
		hsize = ntohl(nsize);

		/* Allocate a buffer to hold the JSON */
		/* +1 for trailing null */
		str = (char *)calloc(sizeof(char), hsize + 1);
		if (str != NULL) {
			rc = nread(fd, str, hsize);
			if (rc >= 0) {
				/* We should be reading in the number of bytes corresponding
				 * to the length in that 4-byte integer. If we don't the
				 * socket might have prematurely closed. Only do the JSON
				 * parsing if we got the correct number of bytes.
				 */
				if (rc == hsize) {
					json = cJSON_Parse(str);
				}
				else {
					LOG_INFO("WARNING:  Size of data read does not correspond to"
									" offered length\n");
				}
			}
		}
		free(str);
	}
	return json;
}

static int
JSON_write(int fd, cJSON *json)
{
	uint32_t hsize, nsize;
	char *str;
	int r = 0;

	str = cJSON_PrintUnformatted(json);
	if (str == NULL)
		r = -1;
	else {
		hsize = strlen(str);
		nsize = htonl(hsize);
		if (nwrite(fd, (char*) &nsize, sizeof(nsize)) < 0)
			r = -1;
		else {
			if (nwrite(fd, str, hsize) < 0)
				r = -1;
		}
		free(str);
    }
    return r;
}

/* client side: send client's connection configuration to server */
int
mperf_send_conf(struct mperf_conn_config *conf)
{
	int ret = 0;
	cJSON *json = NULL;

	json = cJSON_CreateObject();
	if (json == NULL) {
		LOG_ERROR("Failed to create JSON object");
		return -1;
	}

	cJSON_AddNumberToObject(json, "duration", conf->duration);
	if (conf->window_size)
		cJSON_AddNumberToObject(json, "window", conf->window_size);
	if (conf->blksize)
		cJSON_AddNumberToObject(json, "blksize", conf->blksize);
	if (conf->rate)
		cJSON_AddNumberToObject(json, "rate", conf->rate);
	if (conf->burst)
		cJSON_AddNumberToObject(json, "burst", conf->burst);
	if (conf->bytes)
		cJSON_AddNumberToObject(json, "bytes", conf->bytes);
	if (conf->blocks)
		cJSON_AddNumberToObject(json, "blocks", conf->blocks);
	if (conf->mss)
		cJSON_AddNumberToObject(json, "mss", conf->mss);
	cJSON_AddStringToObject(json, "client_version", PACKAGE_STRING);

	LOG_DEBUG("Send params:");
	LOG_DEBUG("\t%s", cJSON_Print(json));

	if (JSON_write(mperf_get_worker()->ctlfd, json) < 0) {
		LOG_ERROR("Failed to write params to ctlfd");
		ret = -1;
	}

	cJSON_Delete(json);
	return ret;
}

/* server side: recv and apply client's connection configuration */
int
mperf_recv_conf(struct mperf_conn_config *conf)
{
	cJSON *json = NULL, *item = NULL;
	struct worker_context *ctx = mperf_get_worker();

	json = JSON_read(ctx->ctlfd);
	if (json == NULL) {
		LOG_ERROR("Failed to recv JSON string from client");
		return -1;
	}

	LOG_DEBUG("Recv params:");
	LOG_DEBUG("\t%s", cJSON_Print(json));

	if ((item = cJSON_GetObjectItem(json, "duration")) != NULL)
		conf->duration = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "window")) != NULL)
		conf->window_size = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "blksize")) != NULL)
		conf->blksize = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "rate")) != NULL)
		conf->rate = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "burst")) != NULL)
		conf->burst = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "bytes")) != NULL)
		conf->bytes = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "blocks")) != NULL)
		conf->blocks = item->valueint;
	if ((item = cJSON_GetObjectItem(json, "mss")) != NULL)
		conf->mss = item->valueint;

	if ((item = cJSON_GetObjectItem(json, "client_version")) != NULL) {
		if (memcmp(item->valuestring, PACKAGE_STRING,
								strlen(PACKAGE_STRING)) == 0) {
			cJSON_Delete(json);
			return 0;
		}
		else {
			LOG_ERROR("Unmatched version: client %s, server %s",
							item->valuestring, PACKAGE_STRING);
		}
	}

	LOG_ERROR("Unknown client version");
	cJSON_Delete(json);
	return -1;
}

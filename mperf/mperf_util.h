#ifndef __MPERF_UTIL_H__
#define __MPERF_UTIL_H__

/* Error routines. */

/* Log printer: Error level */
#define LOG_ERROR(format, ...) \
	fprintf(stderr, "\033[31m[ERROR][T%u]\033[0m %s %d: " format "\n",	\
					thread_id, __FILE__, __LINE__, ##__VA_ARGS__);

/* Log printer: Information level */
#define LOG_INFO(format, ...) \
		fprintf(stdout, "[INFO][T%u] %s %d: " format "\n", \
						thread_id, __FILE__, __LINE__, ##__VA_ARGS__);

#ifdef TCPS_DEBUG
/* Log printer: Warning level */
#define LOG_DEBUG(format, ...) \
		fprintf(stderr, "[DEBUG][T%u] %s %d: " format "\n", \
						thread_id, __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define LOG_DEBUG(format, ...)
#endif

/* MTCP wrappers */
/* close MTCP fd */
#define close_mtcp_fd(mctx, fd)	{			\
	if (fd != -1) {							\
		mtcp_close(mctx, fd);				\
		fd = -1;							\
	}}

/* destroy mtcp context */
#define destroy_mctx(mctx)	{			\
	if (mctx) {							\
		mtcp_destroy_context(mctx);		\
		mctx = NULL;					\
	}}

#define NET_HARDERROR -1
#define NET_SOFTERROR -2

/* Obtain system information */
const char *get_system_info(void);

/* reads 'count' bytes from a socket */
int nread(int fd, char *buf, size_t count);

/* write 'count' bytes to a socket */
int nwrite(int fd, const char *buf, size_t count);

#endif /* __MPERF_UTIL_H__ */

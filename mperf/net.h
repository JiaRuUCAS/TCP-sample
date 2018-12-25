#ifndef __NET_H__
#define __NET_H__

#define NET_SOFTERROR -1
#define NET_HARDERROR -2

struct mperf_test;

int netdial(struct mperf_test *test);

#endif /* __NET_H__ */

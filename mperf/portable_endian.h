// "License": Public Domain
// I, Mathias Panzenböck, place this file hereby into the public domain. Use it at your own risk for whatever you like.

#ifndef PORTABLE_ENDIAN_H__
#define PORTABLE_ENDIAN_H__

#include <../config.h>

#if defined(HAVE_ENDIAN_H)
#include <endian.h>

#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>

#if defined(__OpenBSD__)

#define be16toh(x) betoh16(x)
#define le16toh(x) letoh16(x)

#define be32toh(x) betoh32(x)
#define le32toh(x) letoh32(x)

#define be64toh(x) betoh64(x)
#define le64toh(x) letoh64(x)

#elif defined(__sgi)

#include <netinet/in.h>
#include <inttypes.h>

#define be64toh(x) (x)
#define htobe64(x) (x)

#endif	/* if defined(__OpenBSD__) */

#else

// Unsupported platforms.
// Intended to support CentOS 5 but hopefully not too far from
// the truth because we use the homebrew htonll, et al. implementations
// that were originally the sole implementation of this functionality
// in iperf 3.0.
#warning platform not supported

#include <endian.h>

#if BYTE_ORDER == BIG_ENDIAN
#define HTONLL(n) (n)
#define NTOHLL(n) (n)
#else
#define HTONLL(n) ((((unsigned long long)(n) & 0xFF) << 56) | \
                   (((unsigned long long)(n) & 0xFF00) << 40) | \
                   (((unsigned long long)(n) & 0xFF0000) << 24) | \
                   (((unsigned long long)(n) & 0xFF000000) << 8) | \
                   (((unsigned long long)(n) & 0xFF00000000) >> 8) | \
                   (((unsigned long long)(n) & 0xFF0000000000) >> 24) | \
                   (((unsigned long long)(n) & 0xFF000000000000) >> 40) | \
                   (((unsigned long long)(n) & 0xFF00000000000000) >> 56))

#define NTOHLL(n) ((((unsigned long long)(n) & 0xFF) << 56) | \
                   (((unsigned long long)(n) & 0xFF00) << 40) | \
                   (((unsigned long long)(n) & 0xFF0000) << 24) | \
                   (((unsigned long long)(n) & 0xFF000000) << 8) | \
                   (((unsigned long long)(n) & 0xFF00000000) >> 8) | \
                   (((unsigned long long)(n) & 0xFF0000000000) >> 24) | \
                   (((unsigned long long)(n) & 0xFF000000000000) >> 40) | \
                   (((unsigned long long)(n) & 0xFF00000000000000) >> 56))
#endif	/* if BYTE_ORDER == BIG_ENDIAN */

#define htobe64(n) HTONLL(n)
#define be64toh(n) NTOHLL(n)

#endif	/* if defined(HAVE_ENDIAN_H) */

#endif	/* ifndef PORTABLE_ENDIAN_H__ */

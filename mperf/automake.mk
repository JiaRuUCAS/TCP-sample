if TCPS_DPDK
if TCPS_MTCP
lib_LIBRARIES += libmperf.a

libmperf_a_CFLAGS = $(AM_CFLAGS)
libmperf_a_CPPFLAGS = $(AM_CPPFLAGS)
libmperf_a_include_HEADERS =
libmperf_a_includedir = $(includedir)/mperf
libmperf_a_SOURCES =  mperf/cjson.c			\
					  mperf/mperf_api.c		\
					  mperf/mperf_thread.c	\
					  mperf/mperf_util.c	\
					  mperf/units.c			\
					  mperf/server.c		\
					  mperf/timer.c


bin_PROGRAMS += mperf-bin

mperf_bin_LDFLAGS = $(AM_LDFLAGS) $(DPDK_LDFLAGS) $(MTCP_LDFLAGS)
mperf_bin_LDADD = libmperf.a
mperf_bin_CFLAGS = $(AM_CFLAGS)
mperf_bin_CPPFLAGS = $(AM_CPPFLAGS)
mperf_bin_SOURCES = mperf/main.c

endif
endif


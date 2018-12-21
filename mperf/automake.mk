if TCPS_DPDK
if TCPS_MTCP
lib_LTLIBRARIES += libmperf.la

libmperf_la_CFLAGS = $(AM_CFLAGS)
libmperf_la_CPPFLAGS = $(AM_CPPFLAGS)
libmperf_la_include_HEADERS =
libmperf_la_includedir = $(includedir)/mperf
libmperf_la_SOURCES = mperf/cjson.c		\
					  mperf/units.c		\
					  mperf/timer.c
endif
endif


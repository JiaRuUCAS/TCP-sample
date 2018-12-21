AUTOMAKE_OPTIONS = foreign subdir-objects
#SUBDIRS = \
#		  ipsec

AM_CFLAGS = -Wstrict-prototypes
AM_CFLAGS += -Werror
AM_CFLAGS += -O2 -g -Wall -lrt -lpthread
AM_CFLAGS += $(TCPS_CFLAGS)
#AM_CFLAGS += -D_FILE_OFFSET_BITS=64

#PS_CFLAGS += -Wall -Wstrict_prototypes

AM_LDFLAGS = $(TCPS_LDFLAGS)

if TCPS_DEBUG
AM_CPPFLAGS = -DTCPS_DEBUG
else
AM_CPPFLAGS =
AM_CFLAGS += -fomit-frame-pointer
endif


bin_PROGRAMS =
lib_LIBRARIES =

include src/automake.mk

bin_PROGRAMS += echo-server echo-client

echo_server_LDFLAGS = $(AM_LDFLAGS)
echo_server_CFLAGS = $(AM_CFLAGS)
echo_server_CPPFLAGS = $(AM_CPPFLAGS) -I src/
echo_server_SOURCES = src/echo-server.c

echo_client_LDFLAGS = $(AM_LDFLAGS)
echo_client_CFLAGS = $(AM_CFLAGS)
echo_client_CPPFLAGS = $(AM_CPPFLAGS) -I src/
echo_client_SOURCES = src/echo-client.c

if TCPS_DPDK
if TCPS_MTCP
bin_PROGRAMS += echo-mtcp-server

echo_mtcp_server_LDFLAGS = $(AM_LDFLAGS) $(DPDK_LDFLAGS) $(MTCP_LDFLAGS)
echo_mtcp_server_CFLAGS = $(AM_CFLAGS)
echo_mtcp_server_CPPFLAGS = $(AM_CPPFLAGS) -I src/
echo_mtcp_server_SOURCES = src/echo-mtcp-server.c
endif
endif


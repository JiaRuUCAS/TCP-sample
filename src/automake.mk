bin_PROGRAMS += echo-server echo-client

echo_server_LDFLAGS = $(AM_LDFLAGS)
echo_server_CFLAGS = $(AM_CFLAGS)
echo_server_CPPFLAGS = $(AM_CPPFLAGS) -I src/
echo_server_SOURCES = src/echo-server.c

echo_client_LDFLAGS = $(AM_LDFLAGS)
echo_client_CFLAGS = $(AM_CFLAGS)
echo_client_CPPFLAGS = $(AM_CPPFLAGS) -I src/
echo_client_SOURCES = src/echo-server.c

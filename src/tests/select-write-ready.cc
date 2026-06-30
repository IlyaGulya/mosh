/*
    Mosh: the mobile shell
    Copyright 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include <cerrno>
#include <cstdio>

#include <fcntl.h>
#include <unistd.h>

#include "src/util/select.h"

static int set_nonblocking( int fd )
{
  int flags = fcntl( fd, F_GETFL, 0 );
  if ( flags < 0 ) {
    return -1;
  }

  return fcntl( fd, F_SETFL, flags | O_NONBLOCK );
}

int main( void )
{
  int pipe_fds[2];
  if ( pipe( pipe_fds ) < 0 ) {
    perror( "pipe" );
    return 1;
  }

  if ( set_nonblocking( pipe_fds[0] ) < 0 || set_nonblocking( pipe_fds[1] ) < 0 ) {
    perror( "fcntl" );
    return 1;
  }

  Select& sel = Select::get_instance();
  sel.clear_fds();
  sel.add_write_fd( pipe_fds[1] );
  if ( sel.select( 0 ) < 0 || !sel.write( pipe_fds[1] ) ) {
    fprintf( stderr, "fresh pipe was not write-ready\n" );
    return 1;
  }

  const char buf[4096] = { 0 };
  while ( write( pipe_fds[1], buf, sizeof( buf ) ) > 0 ) {}
  if ( errno != EAGAIN && errno != EWOULDBLOCK ) {
    perror( "write" );
    return 1;
  }

  sel.clear_fds();
  sel.add_write_fd( pipe_fds[1] );
  if ( sel.select( 0 ) < 0 || sel.write( pipe_fds[1] ) ) {
    fprintf( stderr, "full pipe was reported write-ready\n" );
    return 1;
  }

  char read_buf[4096];
  if ( read( pipe_fds[0], read_buf, sizeof( read_buf ) ) <= 0 ) {
    perror( "read" );
    return 1;
  }

  sel.clear_fds();
  sel.add_write_fd( pipe_fds[1] );
  if ( sel.select( 0 ) < 0 || !sel.write( pipe_fds[1] ) ) {
    fprintf( stderr, "drained pipe was not write-ready\n" );
    return 1;
  }

  return 0;
}

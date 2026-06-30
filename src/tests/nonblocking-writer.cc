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
#include <cstdlib>
#include <cstring>
#include <string>

#include <fcntl.h>
#include <unistd.h>

#include "src/util/nonblocking_writer.h"

static int set_nonblocking( int fd )
{
  int flags = fcntl( fd, F_GETFL, 0 );
  if ( flags < 0 ) {
    return -1;
  }

  return fcntl( fd, F_SETFL, flags | O_NONBLOCK );
}

static void require( bool ok, const char* message )
{
  if ( !ok ) {
    fprintf( stderr, "%s\n", message );
    exit( 1 );
  }
}

static std::string read_available( int fd )
{
  std::string out;
  char buf[1024];

  while ( true ) {
    ssize_t bytes_read = read( fd, buf, sizeof( buf ) );
    if ( bytes_read > 0 ) {
      out.append( buf, bytes_read );
      continue;
    }
    if ( bytes_read < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) {
      return out;
    }
    if ( bytes_read == 0 ) {
      return out;
    }
    perror( "read" );
    exit( 1 );
  }
}

static void fill_pipe( int fd )
{
  const char buf[4096] = { 0 };

  while ( write( fd, buf, sizeof( buf ) ) > 0 ) {}
  require( errno == EAGAIN || errno == EWOULDBLOCK, "pipe fill failed unexpectedly" );
}

int main( void )
{
  int pipe_fds[2];
  if ( pipe( pipe_fds ) < 0 ) {
    perror( "pipe" );
    return 1;
  }

  require( set_nonblocking( pipe_fds[0] ) == 0, "failed to make pipe read end nonblocking" );
  require( set_nonblocking( pipe_fds[1] ) == 0, "failed to make pipe write end nonblocking" );

  NonblockingWriter writer;
  writer.append( "abc" );
  writer.append( "" );
  writer.append( "def" );
  require( writer.size() == 6, "writer size after append is wrong" );
  require( writer.flush( pipe_fds[1] ), "writer flush failed" );
  require( writer.empty(), "writer did not drain on an empty pipe" );
  require( read_available( pipe_fds[0] ) == "abcdef", "writer changed chunk order" );

  fill_pipe( pipe_fds[1] );
  writer.append( std::string( 8192, 'x' ) );
  require( writer.flush( pipe_fds[1] ), "writer treated EAGAIN as fatal" );
  require( writer.size() == 8192, "writer should not drain while pipe is full" );

  read_available( pipe_fds[0] );
  require( writer.flush( pipe_fds[1] ), "writer flush after drain failed" );
  require( writer.empty(), "writer did not drain after pipe became writable" );
  require( read_available( pipe_fds[0] ) == std::string( 8192, 'x' ), "writer corrupted queued payload" );

  close( pipe_fds[0] );
  close( pipe_fds[1] );
  return 0;
}

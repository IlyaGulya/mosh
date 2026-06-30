/*
    Mosh: the mobile shell
    Copyright 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#include "src/util/nonblocking_writer.h"

#include <cerrno>
#include <cstdio>

#include <unistd.h>

NonblockingWriter::NonblockingWriter()
  : chunks(), next_byte( 0 ), queued_bytes( 0 )
{}

void NonblockingWriter::append( const std::string& data )
{
  if ( data.empty() ) {
    return;
  }

  chunks.push_back( data );
  queued_bytes += data.size();
}

bool NonblockingWriter::flush( int fd )
{
  while ( !empty() ) {
    std::string& front = chunks.front();
    const ssize_t bytes_written = write( fd, front.data() + next_byte, front.size() - next_byte );

    if ( bytes_written > 0 ) {
      next_byte += bytes_written;
      queued_bytes -= bytes_written;
      if ( next_byte == front.size() ) {
        chunks.pop_front();
        next_byte = 0;
      }
      continue;
    }

    if ( bytes_written < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR ) ) {
      return true;
    }

    perror( "write" );
    return false;
  }

  return true;
}

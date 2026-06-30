/*
    Mosh: the mobile shell
    Copyright 2026

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
*/

#ifndef NONBLOCKING_WRITER_HPP
#define NONBLOCKING_WRITER_HPP

#include <cstddef>
#include <deque>
#include <string>

class NonblockingWriter
{
public:
  NonblockingWriter();

  bool empty( void ) const { return queued_bytes == 0; }
  size_t size( void ) const { return queued_bytes; }

  void append( const std::string& data );
  bool flush( int fd );

private:
  std::deque<std::string> chunks;
  size_t next_byte = 0;
  size_t queued_bytes = 0;
};

#endif

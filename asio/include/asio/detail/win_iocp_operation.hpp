//
// win_iocp_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003, 2004 Christopher M. Kohlhoff (chris@kohlhoff.com)
//
// Permission to use, copy, modify, distribute and sell this software and its
// documentation for any purpose is hereby granted without fee, provided that
// the above copyright notice appears in all copies and that both the copyright
// notice and this permission notice appear in supporting documentation. This
// software is provided "as is" without express or implied warranty, and with
// no claim as to its suitability for any purpose.
//

#ifndef ASIO_DETAIL_WIN_IOCP_OPERATION_HPP
#define ASIO_DETAIL_WIN_IOCP_OPERATION_HPP

#include "asio/detail/push_options.hpp"

#if defined(_WIN32) // This class is only supported on Win32

#include "asio/detail/socket_types.hpp"

namespace asio {
namespace detail {

class win_iocp_demuxer_service;

// Base class for all IOCP operations. A function pointer is used instead of
// virtual functions to avoid the associated overhead.
struct win_iocp_operation
  : public OVERLAPPED
{
  typedef void (*func_type)(win_iocp_operation*, win_iocp_demuxer_service&,
      HANDLE, DWORD, size_t);

  win_iocp_operation(func_type func)
    : func_(func)
  {
    Internal = 0;
    InternalHigh = 0;
    Offset = 0;
    OffsetHigh = 0;
    hEvent = 0;
  }

  void do_completion(win_iocp_demuxer_service& demuxer_service, HANDLE iocp,
      DWORD last_error, size_t bytes_transferred)
  {
    func_(this, demuxer_service, iocp, last_error, bytes_transferred);
  }

protected:
  // Prevent deletion through this type.
  ~win_iocp_operation()
  {
  }

private:
  func_type func_;
};

} // namespace detail
} // namespace asio

#endif // defined(_WIN32)

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_WIN_IOCP_OPERATION_HPP
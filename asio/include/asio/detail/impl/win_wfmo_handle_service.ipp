//
// detail/impl/win_wfmo_handle_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_WIN_WFMO_HANDLE_SERVICE_IPP
#define ASIO_DETAIL_IMPL_WIN_WFMO_HANDLE_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include <boost/assert.hpp>
#include "asio/detail/win_wfmo_handle_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

win_wfmo_handle_service::win_wfmo_handle_service(
    asio::io_service& io_service)
  : wfmo_service_(asio::use_service<win_wfmo_event_service>(io_service))
{
}

void win_wfmo_handle_service::shutdown_service()
{
}

void win_wfmo_handle_service::construct(
    win_wfmo_handle_service::implementation_type& impl)
{
}

void win_wfmo_handle_service::destroy(
    win_wfmo_handle_service::implementation_type& impl)
{
}

asio::error_code win_wfmo_handle_service::assign(
    win_wfmo_handle_service::implementation_type& impl,
    const native_handle_type& native_handle, asio::error_code& ec)
{
  if (is_open(impl))
  {
    ec = asio::error::already_open;
    return ec;
  }

  impl.handle_ = native_handle;
  ec = asio::error_code();
  return ec;
}

asio::error_code win_wfmo_handle_service::close(
    win_wfmo_handle_service::implementation_type& impl,
    asio::error_code& ec)
{
  if (is_open(impl))
  {
    wfmo_service_.cancel(impl.handle_);

    if (!::CloseHandle(impl.handle_))
    {
      DWORD last_error = ::GetLastError();
      ec = asio::error_code(last_error,
          asio::error::get_system_category());
      return ec;
    }

    impl.handle_ = INVALID_HANDLE_VALUE;
  }

  ec = asio::error_code();
  return ec;
}

asio::error_code win_wfmo_handle_service::cancel(
    win_wfmo_handle_service::implementation_type& impl,
    asio::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = asio::error::bad_descriptor;
  }
  else
  {
    wfmo_service_.cancel(impl.handle_);
    ec = asio::error_code();
  }

  return ec;
}

void win_wfmo_handle_service::do_wait(
    win_wfmo_handle_service::implementation_type& impl,
    asio::error_code& ec)
{
  DWORD val = ::WaitForSingleObject(impl.handle_, INFINITE);
  switch (val)
  {
    case WAIT_OBJECT_0:
      ec = asio::error_code();
      break;
    case WAIT_ABANDONED:
      ec = asio::error_code();
      break;
    case WAIT_FAILED:
    {
      DWORD last_error = ::GetLastError();
      ec = asio::error_code(last_error,
          asio::error::get_system_category());
      break;
    }
  }
}

void win_wfmo_handle_service::start_wait_op(
    win_wfmo_handle_service::implementation_type& impl,
    win_wfmo_operation* op)
{
  wfmo_service_.work_started();

  if (!is_open(impl))
  {
    wfmo_service_.on_completion(op, asio::error::bad_descriptor);
  }
  else
  {
    wfmo_service_.on_pending(op);
  }
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_IMPL_WIN_WFMO_HANDLE_SERVICE_IPP

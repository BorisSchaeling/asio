//
// detail/win_wfmo_handle_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_WFMO_HANDLE_SERVICE_HPP
#define ASIO_DETAIL_WIN_WFMO_HANDLE_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include <boost/utility/addressof.hpp>
#include "asio/error.hpp"
#include "asio/io_service.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/win_wfmo_operation.hpp"
#include "asio/detail/win_wfmo_wait_op.hpp"
#include "asio/detail/win_wfmo_event_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class win_wfmo_handle_service
{
public:
  // The native type of an object handle.
  typedef HANDLE native_handle_type;

  // The implementation type of the object handle.
  class implementation_type
  {
   public:
    // Default constructor.
    implementation_type()
      : handle_(INVALID_HANDLE_VALUE)
    {
    }

  private:
    // Only this service will have access to the internal values.
    friend class win_wfmo_handle_service;

    // The native object handle representation.
    native_handle_type handle_;
  };

  ASIO_DECL win_wfmo_handle_service(asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown_service();

  // Construct a new handle implementation.
  ASIO_DECL void construct(implementation_type& impl);

  // Destroy a handle implementation.
  ASIO_DECL void destroy(implementation_type& impl);

  // Assign a native handle to a handle implementation.
  ASIO_DECL asio::error_code assign(implementation_type& impl,
      const native_handle_type& native_handle, asio::error_code& ec);

  // Determine whether the handle is open.
  bool is_open(const implementation_type& impl) const
  {
    return impl.handle_ != INVALID_HANDLE_VALUE;
  }

  // Destroy a handle implementation.
  ASIO_DECL asio::error_code close(implementation_type& impl,
      asio::error_code& ec);

  // Get the native handle representation.
  native_handle_type native_handle(const implementation_type& impl) const
  {
    return impl.handle_;
  }

  // Cancel all operations associated with the handle.
  ASIO_DECL asio::error_code cancel(implementation_type& impl,
      asio::error_code& ec);

  // Wait for a signaled state.
  void wait(implementation_type& impl, asio::error_code& ec)
  {
    do_wait(impl, ec);
  }

  /// Start an asynchronous wait.
  template <typename Handler>
  void async_wait(implementation_type& impl, Handler handler)
  {
    // Allocate and construct an operation to wrap the handler.
    typedef win_wfmo_wait_op<Handler> op;
    typename op::ptr p = { boost::addressof(handler),
      asio_handler_alloc_helpers::allocate(
      sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.handle_, handler);

    start_wait_op(impl, p.p);
    p.v = p.p = 0;
  }

private:
  // Helper function to perform a synchronous wait operation.
  ASIO_DECL void do_wait(implementation_type& impl,
      asio::error_code& ec);

  // Helper function to start a wait operation.
  ASIO_DECL void start_wait_op(implementation_type& impl,
      win_wfmo_operation* op);

  // The WFMO service used for running asynchronous operations and dispatching
  // handlers.
  win_wfmo_event_service& wfmo_service_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/win_wfmo_handle_service.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_WIN_WFMO_HANDLE_SERVICE_HPP

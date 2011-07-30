//
// detail/win_wait_object_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_WAIT_OBJECT_SERVICE_HPP
#define ASIO_DETAIL_WIN_WAIT_OBJECT_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include <boost/utility/addressof.hpp>
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/operation.hpp"
#include "asio/detail/shared_ptr.hpp"
#include "asio/detail/win_wait_op.hpp"
#include "asio/error.hpp"
#include "asio/io_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class win_wait_object_service
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
      : handle_(INVALID_HANDLE_VALUE),
        wait_handle_(new HANDLE(0)),
        next_(0),
        prev_(0)
    {
    }

  private:
    // Only this service will have access to the internal values.
    friend class win_wait_object_service;

    // The native object handle representation.
    native_handle_type handle_;

    // A wait handle to cancel a wait operation.
    shared_ptr<HANDLE> wait_handle_;

    // Pointers to adjacent handle implementations in linked list.
    implementation_type* next_;
    implementation_type* prev_;
  };

  ASIO_DECL win_wait_object_service(asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown_service();

  // Construct a new handle implementation.
  ASIO_DECL void construct(implementation_type& impl);

  // Move-construct a new handle implementation.
  ASIO_DECL void move_construct(implementation_type& impl,
      implementation_type& other_impl);

  // Move-assign from another handle implementation.
  ASIO_DECL void move_assign(implementation_type& impl,
      win_wait_object_service& other_service,
      implementation_type& other_impl);

  // Destroy a handle implementation.
  ASIO_DECL void destroy(implementation_type& impl);

  // Assign a native handle to a handle implementation.
  ASIO_DECL asio::error_code assign(implementation_type& impl,
      const native_handle_type& handle, asio::error_code& ec);

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
    typedef win_wait_op<Handler> op;
    typename op::ptr p = { boost::addressof(handler),
        asio_handler_alloc_helpers::allocate(
        sizeof(op), handler), 0 };
    p.p = new (p.v) op(impl.wait_handle_, io_service_mutex_,
        io_service_ptr_, handler);

    ASIO_HANDLER_CREATION((p.p, "handle", &impl, "async_wait"));

    start_wait_op<Handler>(impl, p.p);
    p.v = p.p = 0;
  }

private:
  // Helper function to perform a synchronous wait operation.
  ASIO_DECL void do_wait(implementation_type& impl,
      asio::error_code& ec);

  // Helper function to start a wait operation.
  template <typename Handler>
  void start_wait_op(implementation_type& impl, operation* op)
  {
    io_service_.work_started();

    if (!is_open(impl))
    {
      io_service_.on_completion(op, asio::error::bad_descriptor);
    }
    else
    {
      BOOL ok = ::RegisterWaitForSingleObject(impl.wait_handle_.get(),
          impl.handle_, &win_wait_op<Handler>::wait_or_timer_callback,
          op, INFINITE, WT_EXECUTEONLYONCE);
      if (!ok)
      {
        io_service_.on_completion(op, ::GetLastError());
      }
    }
  }

  // Helper function to close a handle when the associated object is being
  // destroyed.
  ASIO_DECL void close_for_destruction(implementation_type& impl);

  // The io_service implementation used to post completions.
  io_service_impl& io_service_;

  // Mutex to protect access to the linked list of implementations.
  mutex impl_list_mutex_;

  // The head of a linked list of all implementations.
  implementation_type* impl_list_;

  // Mutex to protect access to the io_service.
  shared_ptr<mutex> io_service_mutex_;

  // Shared pointer to the io_service.
  shared_ptr<io_service_impl*> io_service_ptr_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/win_wait_object_service.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // ASIO_DETAIL_WIN_WAIT_OBJECT_SERVICE_HPP

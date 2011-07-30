//
// detail/impl/win_wait_object_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_WIN_WAIT_OBJECT_SERVICE_IPP
#define ASIO_DETAIL_IMPL_WIN_WAIT_OBJECT_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include "asio/detail/win_wait_object_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

win_wait_object_service::win_wait_object_service(
    asio::io_service& io_service)
  : io_service_(asio::use_service<io_service_impl>(io_service)),
    impl_list_mutex_(),
    impl_list_(0),
    io_service_mutex_(new mutex()),
    io_service_ptr_(new io_service_impl*(&io_service_))
{
}

void win_wait_object_service::shutdown_service()
{
  // Prevent Windows thread from accessing io_service while shutting down.
  asio::detail::mutex::scoped_lock io_service_lock(*io_service_mutex_);
  *io_service_ptr_ = 0;
  io_service_lock.unlock();

  // Close all implementations, causing all operations to complete.
  asio::detail::mutex::scoped_lock impl_list_lock(impl_list_mutex_);
  implementation_type* impl = impl_list_;
  while (impl)
  {
    close_for_destruction(*impl);
    impl = impl->next_;
  }
}

void win_wait_object_service::construct(
    win_wait_object_service::implementation_type& impl)
{
  impl.handle_ = INVALID_HANDLE_VALUE;
  *impl.wait_handle_ = 0;

  // Insert implementation into linked list of all implementations.
  asio::detail::mutex::scoped_lock lock(impl_list_mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void win_wait_object_service::move_construct(
    win_wait_object_service::implementation_type& impl,
    win_wait_object_service::implementation_type& other_impl)
{
  impl.handle_ = other_impl.handle_;
  other_impl.handle_ = INVALID_HANDLE_VALUE;

  *impl.wait_handle_ = 0;
  *other_impl.wait_handle_ = 0;

  // Insert implementation into linked list of all implementations.
  asio::detail::mutex::scoped_lock lock(impl_list_mutex_);
  impl.next_ = impl_list_;
  impl.prev_ = 0;
  if (impl_list_)
    impl_list_->prev_ = &impl;
  impl_list_ = &impl;
}

void win_wait_object_service::move_assign(
    win_wait_object_service::implementation_type& impl,
    win_wait_object_service& other_service,
    win_wait_object_service::implementation_type& other_impl)
{
  close_for_destruction(impl);

  if (this != &other_service)
  {
    // Remove implementation from linked list of all implementations.
    asio::detail::mutex::scoped_lock lock(impl_list_mutex_);
    if (impl_list_ == &impl)
      impl_list_ = impl.next_;
    if (impl.prev_)
      impl.prev_->next_ = impl.next_;
    if (impl.next_)
      impl.next_->prev_= impl.prev_;
    impl.next_ = 0;
    impl.prev_ = 0;
  }

  impl.handle_ = other_impl.handle_;
  other_impl.handle_ = INVALID_HANDLE_VALUE;

  *impl.wait_handle_ = 0;
  *other_impl.wait_handle_ = 0;

  if (this != &other_service)
  {
    // Insert implementation into linked list of all implementations.
    asio::detail::mutex::scoped_lock lock(other_service.impl_list_mutex_);
    impl.next_ = other_service.impl_list_;
    impl.prev_ = 0;
    if (other_service.impl_list_)
      other_service.impl_list_->prev_ = &impl;
    other_service.impl_list_ = &impl;
  }
}

void win_wait_object_service::destroy(
    win_wait_object_service::implementation_type& impl)
{
  close_for_destruction(impl);

  // Remove implementation from linked list of all implementations.
  asio::detail::mutex::scoped_lock lock(impl_list_mutex_);
  if (impl_list_ == &impl)
    impl_list_ = impl.next_;
  if (impl.prev_)
    impl.prev_->next_ = impl.next_;
  if (impl.next_)
    impl.next_->prev_= impl.prev_;
  impl.next_ = 0;
  impl.prev_ = 0;
}

asio::error_code win_wait_object_service::assign(
    win_wait_object_service::implementation_type& impl,
    const native_handle_type& handle, asio::error_code& ec)
{
  if (is_open(impl))
  {
    ec = asio::error::already_open;
    return ec;
  }

  impl.handle_ = handle;
  ec = asio::error_code();
  return ec;
}

asio::error_code win_wait_object_service::close(
    win_wait_object_service::implementation_type& impl,
    asio::error_code& ec)
{
  if (is_open(impl))
  {
    ASIO_HANDLER_OPERATION(("handle", &impl, "close"));

    HANDLE wait_handle =
        InterlockedExchangePointer(impl.wait_handle_.get(), 0);
    if (wait_handle)
    {
      ::UnregisterWait(wait_handle);
      io_service_.work_finished();
    }

    if (!::CloseHandle(impl.handle_))
    {
      DWORD last_error = ::GetLastError();
      ec = asio::error_code(last_error,
          asio::error::get_system_category());
    }
    else
    {
      ec = asio::error_code();
    }

    impl.handle_ = INVALID_HANDLE_VALUE;
  }
  else
  {
    ec = asio::error_code();
  }

  return ec;
}

asio::error_code win_wait_object_service::cancel(
    win_wait_object_service::implementation_type& impl,
    asio::error_code& ec)
{
  if (!is_open(impl))
  {
    ec = asio::error::bad_descriptor;
    return ec;
  }

  ASIO_HANDLER_OPERATION(("handle", &impl, "cancel"));

  HANDLE wait_handle =
      InterlockedExchangePointer(impl.wait_handle_.get(), 0);
  if (wait_handle)
  {
    ::UnregisterWait(wait_handle);
    io_service_.work_finished();
  }
  else
  {
    ec = asio::error_code();
  }

  return ec;
}

void win_wait_object_service::do_wait(
    win_wait_object_service::implementation_type& impl,
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

void win_wait_object_service::close_for_destruction(implementation_type& impl)
{
  if (is_open(impl))
  {
    ASIO_HANDLER_OPERATION(("handle", &impl, "close"));

    HANDLE wait_handle =
        InterlockedExchangePointer(impl.wait_handle_.get(), 0);
    if (wait_handle)
    {
      ::UnregisterWait(wait_handle);
      io_service_.work_finished();
    }

    ::CloseHandle(impl.handle_);
    impl.handle_ = INVALID_HANDLE_VALUE;
  }
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_IMPL_WIN_WAIT_OBJECT_SERVICE_IPP

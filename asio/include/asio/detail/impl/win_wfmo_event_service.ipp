//
// detail/impl/win_wfmo_event_service.ipp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_IMPL_WIN_WFMO_EVENT_SERVICE_IPP
#define ASIO_DETAIL_IMPL_WIN_WFMO_EVENT_SERVICE_IPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include <algorithm>
#include "asio/error.hpp"
#include "asio/io_service.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/throw_error.hpp"
#include "asio/detail/win_wfmo_event_service.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

struct win_wfmo_event_service::worker_thread_function
{
  worker_thread_function(win_wfmo_event_service *wfmo_service)
    : wfmo_service_(wfmo_service)
  {
  }

  void operator()()
  {
    wfmo_service_->work_thread();
  }

  win_wfmo_event_service *wfmo_service_;
};

win_wfmo_event_service::win_wfmo_event_service(
    asio::io_service &io_service)
  : service_base<win_wfmo_event_service>(io_service),
    io_service_impl_(asio::use_service<io_service_impl>(io_service)),
    count_(0)
{
}

void win_wfmo_event_service::shutdown_service()
{
  stop_service();
}

void win_wfmo_event_service::work_started()
{
  io_service_impl_.work_started();
}

void win_wfmo_event_service::on_pending(win_wfmo_operation* op)
{
  asio::detail::mutex::scoped_lock lock(mutex_);
  if (count_ == MAXIMUM_WAIT_OBJECTS)
  {
    io_service_impl_.on_completion(op, asio::error::invalid_argument);
    return;
  }
  start_service();
  interrupt();
  cond_.wait(lock);
  handles_[count_] = op->get_handle();
  ops_[count_ - 2] = op;
  ++count_;
  cond_.notify_all();
}

void win_wfmo_event_service::on_completion(win_wfmo_operation* op,
    const asio::error_code& ec)
{
  io_service_impl_.on_completion(op, ec);
}

void win_wfmo_event_service::cancel(HANDLE handle)
{
  asio::detail::mutex::scoped_lock lock(mutex_);
  if (count_)
  {
    interrupt();
    cond_.wait(lock);
    for (std::size_t i = 2; i < count_; ++i)
    {
      if (handles_[i] == handle)
      {
        on_completion(ops_[i - 2], asio::error::operation_aborted);
        std::swap(handles_[i], handles_[count_ - 1]);
        std::swap(ops_[i - 2], ops_[count_ - 3]);
        --count_;
        break;
      }
    }
    cond_.notify_all();
  }
}

std::size_t win_wfmo_event_service::pending_operations()
{
  asio::detail::mutex::scoped_lock lock(mutex_);
  return count_ > 2 ? count_ - 2 : 0;
}

void win_wfmo_event_service::start_service()
{
  if (!work_thread_)
  {
    handles_[0] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (handles_[0] == NULL)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "wfmo.start");
    }
    handles_[1] = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    if (handles_[1] == NULL)
    {
      DWORD last_error = ::GetLastError();
      ::CloseHandle(handles_[0]);
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "wfmo.start");
    }
    count_ = 2;
    work_thread_.reset(new asio::detail::thread(
        worker_thread_function(this), 65536));
  }
}

void win_wfmo_event_service::stop_service()
{
  asio::detail::mutex::scoped_lock lock(mutex_);
  if (work_thread_)
  {
    ::SetEvent(handles_[1]);
    lock.unlock();
    work_thread_->join();
    work_thread_.reset();
    ::CloseHandle(handles_[0]);
    ::CloseHandle(handles_[1]);
    for (std::size_t i = 0; i < count_ - 2; ++i)
    {
      io_service_impl_.work_finished();
      delete ops_[i];
    }
    count_ = 0;
  }
}

void win_wfmo_event_service::interrupt()
{
  if (!::SetEvent(handles_[0]))
  {
    DWORD last_error = ::GetLastError();
    asio::error_code ec(last_error,
        asio::error::get_system_category());
    asio::detail::throw_error(ec, "wfmo.interrupt");
  }
}

void win_wfmo_event_service::work_thread()
{
  for (;;)
  {
    DWORD res = ::WaitForMultipleObjects(count_,
        &handles_[0], FALSE, INFINITE);
    if (res == WAIT_FAILED)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::mutex::scoped_lock lock(mutex_);
      for (std::size_t i = 0; i < count_ - 2; ++i)
        on_completion(ops_[i], ec);
      count_ = 2;
    }
    else if (res > WAIT_OBJECT_0 + 1 &&
        res < WAIT_OBJECT_0 + count_)
    {
      asio::detail::mutex::scoped_lock lock(mutex_);
      on_completion(ops_[res - WAIT_OBJECT_0 - 2],
          asio::error_code());
      std::swap(ops_[res - WAIT_OBJECT_0 - 2],
          ops_[count_ - 3]);
      std::swap(handles_[res - WAIT_OBJECT_0],
          handles_[count_ - 1]);
      --count_;
    }
    else if (res >= WAIT_ABANDONED_0 &&
        res < WAIT_ABANDONED_0 + count_)
    {
      asio::detail::mutex::scoped_lock lock(mutex_);
      on_completion(ops_[res - WAIT_ABANDONED_0 - 2],
          asio::error_code());
      std::swap(ops_[res - WAIT_ABANDONED_0 - 2],
          ops_[count_ - 3]);
      std::swap(handles_[res - WAIT_ABANDONED_0],
          handles_[count_ - 1]);
      --count_;
    }
    else if (res == WAIT_OBJECT_0)
    {
      asio::detail::mutex::scoped_lock lock(mutex_);
      cond_.notify_all();
      cond_.wait(lock);
    }
    else if (res == WAIT_OBJECT_0 + 1)
    {
      break;
    }
  }
}

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_IMPL_WIN_WFMO_EVENT_SERVICE_IPP

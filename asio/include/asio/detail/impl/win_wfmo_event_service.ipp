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

#include <boost/next_prior.hpp>
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
    io_service_impl_(asio::use_service<io_service_impl>(io_service))
{
  handles_.reserve(MAXIMUM_WAIT_OBJECTS);
  ops_.reserve(MAXIMUM_WAIT_OBJECTS - 2);
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
  start_service();
  interrupt();
  cond_.wait(lock);
  handles_.push_back(op->get_handle());
  ops_.push_back(op);
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
  interrupt();
  cond_.wait(lock);
  typedef std::vector<HANDLE> handles;
  for (handles::size_type i = 2; i < handles_.size(); ++i)
  {
    if (handles_[i] == handle)
    {
      handles_.erase(boost::next(handles_.begin(), i));
      typedef std::vector<win_wfmo_operation*> ops;
      ops::iterator it = boost::next(ops_.begin(), i - 2);
      on_completion(*it, asio::error::operation_aborted);
      ops_.erase(it);
      break;
    }
  }
  cond_.notify_all();
}

void win_wfmo_event_service::start_service()
{
  if (!work_thread_)
  {
    handles_.push_back(::CreateEvent(NULL, FALSE, FALSE, NULL));
    if (handles_.front() == NULL)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "wfmo.start");
    }
    handles_.push_back(::CreateEvent(NULL, FALSE, FALSE, NULL));
    if (handles_.back() == NULL)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "wfmo.start");
    }
    work_thread_.reset(new asio::detail::thread(
        worker_thread_function(this)));
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
    handles_.clear();
    typedef std::vector<win_wfmo_operation*> op_vector;
    for (op_vector::iterator it = ops_.begin(); it != ops_.end(); ++it)
    {
      io_service_impl_.work_finished();
      delete *it;
    }
    ops_.clear();
  }
}

void win_wfmo_event_service::interrupt()
{
  if (!::SetEvent(handles_.front()))
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
    DWORD res = ::WaitForMultipleObjects(handles_.size(),
        &handles_.front(), FALSE, INFINITE);
    if (res == WAIT_FAILED)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::mutex::scoped_lock lock(mutex_);
      typedef std::vector<win_wfmo_operation*> op_vector;
      for (op_vector::iterator it = ops_.begin(); it != ops_.end(); ++it)
        on_completion(*it, ec);
      ops_.clear();
      handles_.erase(boost::next(handles_.begin(), 2), handles_.end());
    }
    else if (res > WAIT_OBJECT_0 + 1 &&
        res < WAIT_OBJECT_0 + handles_.size())
    {
      asio::detail::mutex::scoped_lock lock(mutex_);
      typedef std::vector<win_wfmo_operation*> op_vector;
      op_vector::iterator it = boost::next(ops_.begin(),
          res - WAIT_OBJECT_0 - 2);
      on_completion(*it, asio::error_code());
      ops_.erase(it);
      std::vector<HANDLE>::iterator it2 = boost::next(
          handles_.begin(), res - WAIT_OBJECT_0);
      handles_.erase(it2);
    }
    else if (res >= WAIT_ABANDONED_0 &&
        res < WAIT_ABANDONED_0 + handles_.size())
    {
      asio::detail::mutex::scoped_lock lock(mutex_);
      typedef std::vector<win_wfmo_operation*> op_vector;
      op_vector::iterator it = boost::next(ops_.begin(),
          res - WAIT_ABANDONED_0 - 2);
      on_completion(*it, asio::error_code());
      ops_.erase(it);
      std::vector<HANDLE>::iterator it2 = boost::next(
          handles_.begin(), res - WAIT_ABANDONED_0);
      handles_.erase(it2);
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

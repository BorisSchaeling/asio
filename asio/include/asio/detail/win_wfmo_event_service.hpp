//
// detail/win_wfmo_event_service.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_WFMO_EVENT_SERVICE_HPP
#define ASIO_DETAIL_WIN_WFMO_EVENT_SERVICE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include <vector>
#include <boost/scoped_ptr.hpp>
#include "asio/io_service.hpp"
#include "asio/detail/thread.hpp"
#include "asio/detail/mutex.hpp"
#include "asio/detail/condition_variable.hpp"
#include "asio/detail/win_iocp_io_service_fwd.hpp"
#include "asio/detail/win_wfmo_operation.hpp"
#include "asio/detail/thread.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

class win_wfmo_event_service
  : public asio::detail::service_base<win_wfmo_event_service>
{
public:
  // Constructor.
  ASIO_DECL win_wfmo_event_service(asio::io_service& io_service);

  // Destroy all user-defined handler objects owned by the service.
  ASIO_DECL void shutdown_service();

  // Notify that some work has started.
  ASIO_DECL void work_started();

  // Called after starting a WFMO operation that did not complete immediately.
  // The caller must have already called work_started() prior to starting the
  // operation.
  ASIO_DECL void on_pending(win_wfmo_operation* op);

  // Called after starting a WFMO operation that completed immediately.
  // The caller must have already called work_started() prior to starting the
  // operation.
  ASIO_DECL void on_completion(win_wfmo_operation* op,
      const asio::error_code& ec);

  // Cancel pending asynchronous operations.
  ASIO_DECL void cancel(HANDLE handle);

private:
  // Called when first operation is started and background thread must
  // be started to call WaitForMultipleObjects.
  ASIO_DECL void start_service();

  // Called when service is shutdown and background thread must be stopped.
  ASIO_DECL void stop_service();

  // Called when blocking call to WaitForMultipleObjects must be interrupted
  // in the background thread because handles must be updated (or service
  // must be shutdown).
  ASIO_DECL void interrupt();

  // Background thread with a loop to call WaitForMultipleObjects until
  // service is shutdown.
  ASIO_DECL void work_thread();

  // Function object for processing timeouts in a background thread.
  struct worker_thread_function;
  friend struct worker_thread_function;

  // The io_service implementation used to post completions.
  asio::detail::io_service_impl &io_service_impl_;

  // Worker thread.
  boost::scoped_ptr<thread> work_thread_;

  // Mutex for protecting access to handles and pending operations.
  mutex mutex_;

  // Condition variable to synchronize service with worker thread.
  condition_variable cond_;

  // Handles WaitForMultipleObjects is called for.
  std::vector<HANDLE> handles_;

  // Pending operations.
  std::vector<win_wfmo_operation*> ops_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#if defined(ASIO_HEADER_ONLY)
# include "asio/detail/impl/win_wfmo_event_service.ipp"
#endif // defined(ASIO_HEADER_ONLY)

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_WIN_WFMO_EVENT_SERVICE_HPP

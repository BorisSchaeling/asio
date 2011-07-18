//
// detail/win_condition_variable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_CONDITION_VARIABLE_HPP
#define ASIO_DETAIL_WIN_CONDITION_VARIABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(BOOST_WINDOWS) && !defined(UNDER_CE)

#include "asio/error.hpp"
#include "asio/detail/noncopyable.hpp"
#include "asio/detail/mutex.hpp"
#include "asio/detail/throw_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

#if defined(_WIN32_WINNT) && (_WIN32_WINNT >= 0x0600)
// Building for Windows Vista or later.

class win_condition_variable
  : private noncopyable
{
public:
  // Constructor.
  win_condition_variable()
  {
    ::InitializeConditionVariable(&cond_);
  }

  // Waits for the condition variable.
  template <typename Lock>
  void wait(Lock& lock)
  {
    if (!::SleepConditionVariableCS(&cond_, &lock.mutex().crit_section_, INFINITE))
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "condvar");
    }
  }

  // Wake all threads waiting on the condition variable.
  void notify_all()
  {
    ::WakeAllConditionVariable(&cond_);
  }

private:
  ::CONDITION_VARIABLE cond_;
};

#else
// Building for Windows 2003 or earlier.
// Based on http://www1.cse.wustl.edu/~schmidt/win32-cv-1.html,
// "3.3 The Generation Count Solution" and on
// http://www.lambdacs.com/cpt/FAQ.html#Q89,
// "How to implement POSIX Condition variables in Win32?"

class win_condition_variable
  : private noncopyable
{
public:
  // Constructor.
  win_condition_variable()
    : waiters_count_(0),
      wait_generation_count_(0),
      release_count_(0)
  {
    event_ = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (event_ == NULL)
    {
      DWORD last_error = ::GetLastError();
      asio::error_code ec(last_error,
          asio::error::get_system_category());
      asio::detail::throw_error(ec, "condvar");
    }
    ::InitializeCriticalSection(&waiters_count_lock_);
  }

  // Destructor.
  ~win_condition_variable()
  {
    ::DeleteCriticalSection(&waiters_count_lock_);
    ::CloseHandle(event_);
  }

  // Waits for the condition variable.
  template <typename Lock>
  void wait(Lock& lock)
  {
    ::EnterCriticalSection(&waiters_count_lock_);
    ++waiters_count_;
    int my_generation = wait_generation_count_;
    ::LeaveCriticalSection(&waiters_count_lock_);
    ::LeaveCriticalSection(&lock.mutex().crit_section_);

    for (;;)
    {
      ::WaitForSingleObject(event_, INFINITE);
      ::EnterCriticalSection(&waiters_count_lock_);
      int wait_done = release_count_ > 0 &&
          wait_generation_count_ != my_generation;
      ::LeaveCriticalSection(&waiters_count_lock_);
      if (wait_done)
        break;
    }

    ::EnterCriticalSection(&lock.mutex().crit_section_);
    ::EnterCriticalSection(&waiters_count_lock_);
    --waiters_count_;
    --release_count_;
    int last_waiter = release_count_ == 0;
    ::LeaveCriticalSection(&waiters_count_lock_);

    if (last_waiter)
      ::ResetEvent(event_);
  }

  // Wake all threads waiting on the condition variable.
  void notify_all()
  {
    ::EnterCriticalSection(&waiters_count_lock_);
    if (waiters_count_ > 0)
    {
      ::SetEvent(event_);
      release_count_ = waiters_count_;
      ++wait_generation_count_;
    }
    ::LeaveCriticalSection(&waiters_count_lock_);
  }

private:
  int waiters_count_;
  int wait_generation_count_;
  int release_count_;
  HANDLE event_;
  ::CRITICAL_SECTION waiters_count_lock_;
};

#endif

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(BOOST_WINDOWS) && !defined(UNDER_CE)

#endif // ASIO_DETAIL_WIN_CONDITION_VARIABLE_HPP

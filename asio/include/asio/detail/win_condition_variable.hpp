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
  void wait(Lock &lock)
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

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(BOOST_WINDOWS) && !defined(UNDER_CE)

#endif // ASIO_DETAIL_WIN_CONDITION_VARIABLE_HPP

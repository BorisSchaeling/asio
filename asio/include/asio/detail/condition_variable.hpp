//
// detail/condition_variable.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_CONDITION_VARIABLE_HPP
#define ASIO_DETAIL_CONDITION_VARIABLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(BOOST_HAS_THREADS) && !defined(ASIO_DISABLE_THREADS)
# if defined(BOOST_WINDOWS)
#  include "asio/detail/win_condition_variable.hpp"
# else
#  error Only Windows is supported!
# endif
#endif

namespace asio {
namespace detail {

#if defined(BOOST_HAS_THREADS) && !defined(ASIO_DISABLE_THREADS)
# if defined(BOOST_WINDOWS)
typedef win_condition_variable condition_variable;
# endif
#endif

} // namespace detail
} // namespace asio

#endif // ASIO_DETAIL_CONDITION_VARIABLE_HPP

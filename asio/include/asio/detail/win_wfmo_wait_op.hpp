//
// detail/win_wfmo_operation.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_WFMO_WAIT_OP_HPP
#define ASIO_DETAIL_WIN_WFMO_WAIT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_IOCP)

#include <boost/utility/addressof.hpp>
#include "asio/error.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/win_wfmo_operation.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Handler>
class win_wfmo_wait_op : public win_wfmo_operation
{
public:
  ASIO_DEFINE_HANDLER_PTR(win_wfmo_wait_op);

  win_wfmo_wait_op(HANDLE handle, Handler handler)
    : win_wfmo_operation(handle, &win_wfmo_wait_op::do_complete),
      handler_(handler)
  {
  }

  static void do_complete(io_service_impl *owner, operation *base,
      asio::error_code ec, std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the operation object.
    win_wfmo_wait_op *o = static_cast<win_wfmo_wait_op*>(base);
    ptr p = { boost::addressof(o->handler_), o, o };

    detail::binder1<Handler, asio::error_code>
        handler(o->handler_, ec);
    p.h = boost::addressof(handler.handler_);
    p.reset();

    if (owner)
    {
      asio::detail::fenced_block b;
      asio_handler_invoke_helpers::invoke(handler, handler.handler_);
    }
  }

private:
  Handler handler_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_IOCP)

#endif // ASIO_DETAIL_WIN_WFMO_WAIT_OP_HPP

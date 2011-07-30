//
// detail/win_wait_op.hpp
// ~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_DETAIL_WIN_WAIT_OP_HPP
#define ASIO_DETAIL_WIN_WAIT_OP_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#include <boost/utility/addressof.hpp>
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_invoke_helpers.hpp"
#include "asio/detail/mutex.hpp"
#include "asio/detail/operation.hpp"
#include "asio/detail/shared_ptr.hpp"
#include "asio/error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace detail {

template <typename Handler>
class win_wait_op : public operation
{
public:
  ASIO_DEFINE_HANDLER_PTR(win_wait_op);

  win_wait_op(shared_ptr<HANDLE>& wait_handle, shared_ptr<mutex>& mutex,
      shared_ptr<io_service_impl*>& ios, Handler& handler)
    : operation(&win_wait_op::do_complete),
      wait_handle_(wait_handle),
      mutex_(mutex),
      io_service_impl_(ios),
      handler_(ASIO_MOVE_CAST(Handler)(handler))
  {
  }

  static void do_complete(io_service_impl* owner, operation* base,
      asio::error_code /*ec*/, std::size_t /*bytes_transferred*/)
  {
    // Take ownership of the operation object.
    win_wait_op* o(static_cast<win_wait_op*>(base));
    ptr p = { boost::addressof(o->handler_), o, o };

    ASIO_HANDLER_COMPLETION((o));

    // Make a copy of the handler so that the memory can be deallocated
    // before the upcall is made. Even if we're not about to make an upcall,
    // a sub-object of the handler may be the true owner of the memory
    // associated with the handler. Consequently, a local copy of the handler
    // is required to ensure that any owning sub-object remains valid until
    // after we have deallocated the memory here.
    detail::binder1<Handler, asio::error_code>
        handler(o->handler_, asio::error_code());
    p.h = boost::addressof(handler.handler_);
    p.reset();

    // Make the upcall if required.
    if (owner)
    {
      asio::detail::fenced_block b;
      ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      asio_handler_invoke_helpers::invoke(handler, handler.handler_);
      ASIO_HANDLER_INVOCATION_END;
    }
  }

  static VOID CALLBACK wait_or_timer_callback(PVOID param,
      BOOLEAN /*timeout*/)
  {
    // Take ownership of the operation object.
    win_wait_op* o(static_cast<win_wait_op*>(param));
    ptr p = { boost::addressof(o->handler_), o, o };

    asio::detail::mutex::scoped_lock lock(*o->mutex_);
    if (*o->io_service_impl_)
    {
      HANDLE wait_handle =
          InterlockedExchangePointer(o->wait_handle_.get(), 0);
      if (wait_handle)
      {
        ::UnregisterWait(wait_handle);

        // Pass operation back to main io_service for completion.
        (*o->io_service_impl_)->post_deferred_completion(o);
        p.v = p.p = 0;
      }
      else
      {
        // Clean up.
        (*o->io_service_impl_)->work_finished();
        p.reset();
      }
    }
    else
    {
      p.reset();
    }
  }

private:
  shared_ptr<HANDLE> wait_handle_;
  shared_ptr<mutex> mutex_;
  shared_ptr<io_service_impl*> io_service_impl_;
  Handler handler_;
};

} // namespace detail
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // ASIO_DETAIL_WIN_WAIT_OP_HPP

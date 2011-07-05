//
// windows/basic_object_handle.hpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2011 Christopher M. Kohlhoff (chris at kohlhoff dot com)
// Copyright (c) 2011 Boris Schaeling (boris@highscore.de)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef ASIO_WINDOWS_BASIC_OBJECT_HANDLE_HPP
#define ASIO_WINDOWS_BASIC_OBJECT_HANDLE_HPP

#if defined(_MSC_VER) && (_MSC_VER >= 1200)
# pragma once
#endif // defined(_MSC_VER) && (_MSC_VER >= 1200)

#include "asio/detail/config.hpp"

#if defined(ASIO_HAS_WINDOWS_OBJECT_HANDLE) \
  || defined(GENERATING_DOCUMENTATION)

#include "asio/error.hpp"
#include "asio/windows/basic_handle.hpp"
#include "asio/windows/object_handle_service.hpp"
#include "asio/detail/throw_error.hpp"

#include "asio/detail/push_options.hpp"

namespace asio {
namespace windows {

/// Provides object-oriented handle functionality.
/**
 * The windows::basic_object_handle class template provides asynchronous and
 * blocking object-oriented handle functionality.
 *
 * @par Thread Safety
 * @e Distinct @e objects: Safe.@n
 * @e Shared @e objects: Unsafe.
 */
template <typename ObjectHandleService = object_handle_service>
class basic_object_handle
  : public basic_handle<ObjectHandleService>
{
public:
  /// The native representation of a handle.
  typedef typename ObjectHandleService::native_handle_type native_handle_type;

  /// Construct a basic_object_handle without opening it.
  /**
   * This constructor creates an object handle without opening it.
   *
   * @param io_service The io_service object that the object handle will use to
   * dispatch handlers for any asynchronous operations performed on the handle.
   */
  explicit basic_object_handle(asio::io_service& io_service)
    : basic_handle<ObjectHandleService>(io_service)
  {
  }

  /// Construct a basic_object_handle on an existing native handle.
  /**
   * This constructor creates an object handle object to hold an existing
   * native handle.
   *
   * @param io_service The io_service object that the object handle will use to
   * dispatch handlers for any asynchronous operations performed on the handle.
   *
   * @param native_handle The new underlying handle implementation.
   *
   * @throws asio::system_error Thrown on failure.
   */
  basic_object_handle(asio::io_service& io_service,
      const native_handle_type& native_handle)
    : basic_handle<ObjectHandleService>(io_service, native_handle)
  {
  }

  /// Perform a blocking wait on the object handle.
  /**
   * This function is used to wait for the object handle to be set to the
   * signaled state. This function blocks and does not return until the object
   * handle has been set to the signaled state.
   *
   * @throws asio::system_error Thrown on failure.
   */
  void wait()
  {
    asio::error_code ec;
    this->service.wait(this->implementation, ec);
    asio::detail::throw_error(ec);
  }

  /// Perform a blocking wait on the object handle.
  /**
   * This function is used to wait for the object handle to be set to the
   * signaled state. This function blocks and does not return until the object
   * handle has been set to the signaled state.
   *
   * @param ec Set to indicate what error occurred, if any.
   */
  void wait(asio::error_code& ec)
  {
    this->service.wait(this->implementation, ec);
  }

  /// Start an asynchronous wait on the object handle.
  /**
   * This function is be used to initiate an asynchronous wait against the
   * object handle. It always returns immediately.
   *
   * @param handler The handler to be called when the object handle is set to
   * the signaled state. Copies will be made of the handler as required. The
   * function signature of the
   * handler must be:
   * @code void handler(
   *   const asio::error_code& error // Result of operation.
   * ); @endcode
   * Regardless of whether the asynchronous operation completes immediately or
   * not, the handler will not be invoked from within this function. Invocation
   * of the handler will be performed in a manner equivalent to using
   * asio::io_service::post().
   */
  template <typename WaitHandler>
  void async_wait(WaitHandler handler)
  {
    this->service.async_wait(this->implementation, handler);
  }
};

} // namespace windows
} // namespace asio

#include "asio/detail/pop_options.hpp"

#endif // defined(ASIO_HAS_WINDOWS_OBJECT_HANDLE)
       //   || defined(GENERATING_DOCUMENTATION)

#endif // ASIO_WINDOWS_BASIC_OBJECT_HANDLE_HPP

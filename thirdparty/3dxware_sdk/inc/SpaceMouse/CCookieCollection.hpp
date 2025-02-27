#ifndef CCookieCollection_HPP_INCLUDED
#define CCookieCollection_HPP_INCLUDED
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2018-2021 3Dconnexion.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file CCookieCollection.hpp
 * @brief Cookie collection.
 */
#include <navlib/navlib_types.h>
// stdlib
#include <map>
#include <memory>
#include <stdexcept>
#if (!defined(_MSC_VER) || (_MSC_VER > 1600))
#include <mutex>
#include <chrono>
#else
#pragma warning(disable : 4482) // non-standard
#include <boost/thread/lock_guard.hpp>
#include <boost/thread/mutex.hpp>
namespace std {
using boost::lock_guard;
using boost::mutex;
using boost::unique_lock;
} // namespace std
#endif

namespace TDx {
namespace SpaceMouse {
/// <summary>
/// The class maps a cookie to a weak_ptr<typeparam name="T"></typeparam>.
/// </summary>
template <class T> class CCookieCollection : protected std::map<navlib::param_t, std::weak_ptr<T>> {
  typedef std::map<navlib::param_t, std::weak_ptr<T>> map_t;

public:
  typedef typename map_t::size_type size_type;

  /// <summary>
  /// Gets the shared pointer to class T corresponding to the passed in cookie.
  /// </summary>
  /// <param name="cookie">The <see cref="navlib::param_t"/> to search for.</param>
  /// <returns>The shared pointer to the instance of T</returns>
  /// <exception cref="std::out_of_range">If the cookie does not exist.</exception>
  std::shared_ptr<T> at(const navlib::param_t &cookie) {
    std::lock_guard<std::mutex> guard(m_mutex);
    typename map_t::iterator iter = map_t::find(cookie);
    if (iter != map_t::end()) {
      return iter->second.lock();
    }

    throw std::out_of_range("Cookie does not exist in the Collection");
  }

  /// <summary>
  /// Removes the elements that match the cookie.
  /// </summary>
  /// <param name="cookie">The cookie entry to remove.</param>
  /// <returns>The number of elements that have been removed.</returns>
  size_type erase(const navlib::param_t &cookie) {
    std::lock_guard<std::mutex> guard(m_mutex);
    return map_t::erase(cookie);
  }

  /// <summary>
  /// Inserts a weak pointer (to class T) and returns a cookie that is needed to retrieve it later.
  /// </summary>
  /// <param name="sp">Shared pointer to class T.</param>
  /// <returns>A cookie that is needed to find the shared pointer.</returns>
  navlib::param_t insert(const std::shared_ptr<T> &sp) {
    navlib::param_t param = 0;
    if (sp) {
      std::lock_guard<std::mutex> guard(m_mutex);
      do {
        using namespace std::chrono;
        param =
            duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count();
      } while (map_t::find(param) != map_t::end());
      (*this)[param] = sp;
    }
    return param;
  }

protected:
  /// <summary>
  /// When changing the contents of the collection always use the mutex as a guard
  /// </summary>
  std::mutex m_mutex;
};
} // namespace SpaceMouse
} // namespace TDx
#endif // CCookieCollection_HPP_INCLUDED
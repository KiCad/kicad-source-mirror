#ifndef NAVLIB_ERROR_H_INCLUDED_
#define NAVLIB_ERROR_H_INCLUDED_
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (c) 2014-2021 3Dconnexion.
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
 * @file navlib_error.h
 * @brief defines the classes used for error reporting.
 */

#include <system_error>

#include <navlib/navlib_types.h>

#ifndef NOEXCEPT
#if defined(_MSC_VER) && _MSC_VER < 1800
#ifdef _NOEXCEPT
#define NOEXCEPT _NOEXCEPT
#else
#define NOEXCEPT
#endif
#else
#define NOEXCEPT noexcept
#endif
#endif

namespace std {
template <> struct is_error_code_enum<::navlib::navlib_errc::navlib_errc_t> : true_type {};
} // namespace std

namespace { // Anonymous namespace
/// <summary>
/// Navigation library error category.
/// </summary>
struct navlib_error_category : public std::error_category {
  typedef std::error_category base_type;

public:
  navlib_error_category() NOEXCEPT {
  }

  const char *name() const NOEXCEPT override {
    return "navlib";
  }

  std::string message(int errorValue) const override {
    namespace navlib_errc = navlib::navlib_errc;
    switch (static_cast<navlib_errc::navlib_errc_t>(errorValue)) {
    case navlib_errc::property_not_found:
      return "Cannot locate the requested navlib property.";

    case navlib_errc::invalid_function:
      return "The requested function is not valid.";

    case navlib_errc::insufficient_buffer:
      return "Insufficient buffer space.";

    default:
      return std::generic_category().message(errorValue);
    }
  }
};

/// <summary>
/// Navigation library error category.
/// </summary>
static const navlib_error_category navlib_category;
} // namespace

namespace navlib {
/// <summary>
/// Makes a <see cref="std::error_code"/>.
/// </summary>
/// <param name="errc">The Navigation library error.</param>
/// <returns>A <see cref="std::error_code"/> with the Navigation library category.</returns>
inline std::error_code make_error_code(navlib_errc::navlib_errc_t errc) {
  std::error_code ec(static_cast<int>(errc), navlib_category);
  return ec;
}
} // namespace navlib
#endif /* NAVLIB_ERROR_H_INCLUDED_ */
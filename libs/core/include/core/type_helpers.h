/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <type_traits>

/**
 * @brief A type that is always false.
 *
 * Useful for static_asserts, when 'false' is no good as the
 * compiler likes to believe that the assert could pass in
 * some cases:
 *
 * if constexpr ( something<T>() )
 * {
 *     // Say we know that this branch must always be chosen.
 * }
 * else
 * {
 *     // Even if the above if's are made such this branch is never chosen
 *     // compilers may still complain if this were to use a literal 'false'.
 *     static_assert( always_false<T>::value, "This should never happen" );
 * }
 */
template <typename T>
struct always_false : std::false_type
{
};

template <typename E>
constexpr auto to_underlying(E e) noexcept {
    return static_cast<std::underlying_type_t<E>>(e);
}
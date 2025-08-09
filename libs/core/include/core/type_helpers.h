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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

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
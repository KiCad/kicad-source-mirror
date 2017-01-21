/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file make_unique.h
 * @brief Implementation of std::make_unique for pre C++14 compilation
 * environments
 */

#ifndef  MAKE_UNIQUE_H
#define  MAKE_UNIQUE_H

// Define std::make_unique if the compiler is C++11, but not C++14
// (which provides this itself)
// When C++14 support is a KiCad pre-requisite, this entire file
// can be removed.
#if __cplusplus == 201103L

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>

// It's a bit naughty to add things to std::, but the point is to
// "polyfill" this function in only if std:: doesn't have it
//
// This implementation is the one proposed by Stephan T. Lavavej
// in N3656: https://isocpp.org/files/papers/N3656.txt
// This is more or less exactly how it is implemented in GCC (e.g. 6.3.1)
// when C++14 is enabled.
namespace std
{
    template<class T> struct _Unique_if {
        typedef unique_ptr<T> _Single_object;
    };

    template<class T> struct _Unique_if<T[]> {
        typedef unique_ptr<T[]> _Unknown_bound;
    };

    template<class T, size_t N> struct _Unique_if<T[N]> {
        typedef void _Known_bound;
    };

    /// std::make_unique for single objects
    template<class T, class... Args>
        typename _Unique_if<T>::_Single_object
        make_unique(Args&&... args) {
            return unique_ptr<T>(new T(std::forward<Args>(args)...));
        }

    /// std::make_unique for arrays of unknown bound
    template<class T>
        typename _Unique_if<T>::_Unknown_bound
        make_unique(size_t n) {
            typedef typename remove_extent<T>::type U;
            return unique_ptr<T>(new U[n]());
        }

    /// Disable std::make_unique for arrays of known bound
    template<class T, class... Args>
        typename _Unique_if<T>::_Known_bound
        make_unique(Args&&...) = delete;
}

#endif // __cplusplus == 201103L

#endif  // WXSTRUCT_H_

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

#ifndef __SCOPED_SET_RESET_H
#define __SCOPED_SET_RESET_H

#include <functional>

/**
 * RAII class that sets an value at construction and resets it to the original value
 * at destruction.
 *
 * @note There is no type deduction for template classes until C++17, \
 * so you can't do this:
 *
 *     int target = 0;
 *     SCOPED_SET_RESET( target, 42 );
 *
 * Instead, you can use a type alias, for example:
 *
 *     using SCOPED_INT_SET_RESET = SCOPED_SET_RESET<int>;
 *     int target = 0;
 *     SCOPED_INT_SET_RESET( target , 42 );
 */
template <typename VAL_TYPE>
class SCOPED_SET_RESET
{
public:
    SCOPED_SET_RESET( VAL_TYPE& target, VAL_TYPE value ) : m_target( target )
    {
        m_original = target;
        m_target = value;
    }

    /**
     * Destruct the class, and return the target to its original value.
     */
    ~SCOPED_SET_RESET() { m_target = m_original; }

private:
    VAL_TYPE  m_original;
    VAL_TYPE& m_target;
};


/**
 * RAII class that executes a function at construction and another at destruction.
 *
 * Useful to ensure cleanup code is executed even if an exception is thrown.
 */
template <typename Func>
class SCOPED_EXECUTION
{
public:
    SCOPED_EXECUTION( Func initFunc, Func destroyFunc ) :
            m_initFunc( initFunc ), m_destroyFunc( destroyFunc )
    {
        m_initFunc();
    }

    ~SCOPED_EXECUTION() { m_destroyFunc(); }

private:
    Func m_initFunc;
    Func m_destroyFunc;
};

#endif // __SCOPED_SET_RESET_H

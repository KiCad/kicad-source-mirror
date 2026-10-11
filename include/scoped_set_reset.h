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

#include <utility>

/**
 * RAII class that sets an value at construction and resets it to the original value
 * at destruction.
 *
 * @note There is type deduction for template classes from C++17,
 * so you can do this without `<int>`:
 *
 *     int target = 0;
 *     SCOPED_SET_RESET targetResetter( target, 42 );
 */
template <typename VAL_TYPE>
class SCOPED_SET_RESET
{
public:
    /**
     * Construct the class, set the target to the specified value, and
     * remember the original value.
     *
     * Whatever the caller does with the target, it will be restored
     * to its original value upon destruction.
     */
    SCOPED_SET_RESET( VAL_TYPE& target, VAL_TYPE value ) :
            m_original( target ),
            m_target( target )
    {
        m_target = std::move( value );
    }

    /**
     * Construct the class and remember the original value of the target,
     * but without changing it immediately.
     *
     * In any case, the original value will be restored upon destruction.
     */
    SCOPED_SET_RESET( VAL_TYPE& target ) :
            m_original( target ),
            m_target( target )
    {
    }

    /**
     * Destruct the class, and return the target to its original value.
     */
    ~SCOPED_SET_RESET() { m_target = m_original; }

    // No copy operations allowed or the destructor might reset
    // the target to its original value multiple times.

    SCOPED_SET_RESET( SCOPED_SET_RESET const& ) = delete;
    SCOPED_SET_RESET& operator=( SCOPED_SET_RESET const& ) = delete;

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

    // No copy operations allowed or the destructor might execute
    // the destroy function multiple times.

    SCOPED_EXECUTION( SCOPED_EXECUTION const& ) = delete;
    SCOPED_EXECUTION& operator=( SCOPED_EXECUTION const& ) = delete;

    ~SCOPED_EXECUTION() { m_destroyFunc(); }

private:
    Func m_initFunc;
    Func m_destroyFunc;
};

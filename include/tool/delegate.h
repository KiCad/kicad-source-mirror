/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __DELEGATE_H
#define __DELEGATE_H


/**
 * class DELEGATE
 * A trivial delegate (pointer to member method of an object) pattern implementation.
 * Check delegate_example.cpp for a coding sample.
 */

template <class ReturnType, class Arg>
class DELEGATE
{
public:
    typedef ReturnType (DELEGATE<ReturnType, Arg>::* MemberPointer)( Arg );
    typedef ReturnType  _ReturnType;
    typedef Arg         _ArgType;

    DELEGATE()
    {
    }

    template <class T>
    DELEGATE( T* aObject, ReturnType(T::* aPtr)( Arg ) )
    {
        m_ptr = reinterpret_cast<MemberPointer>( aPtr );
        m_object = reinterpret_cast<void*>( aObject );
    };


    ReturnType operator()( Arg aA ) const
    {
        DELEGATE<ReturnType, Arg>* casted = reinterpret_cast<DELEGATE<ReturnType, Arg>*>( m_object );
        return (casted->*m_ptr)( aA );
    }

private:
    MemberPointer m_ptr;
    void* m_object;
};

/**
 * Class DELEGATE0
 * Same as DELEGATE, but with no arguments.
 */
template <class ReturnType>
class DELEGATE0
{
public:
    typedef ReturnType ( DELEGATE0<ReturnType>::* MemberPointer )();
    typedef ReturnType _ReturnType;

    DELEGATE0()
    {
    }

    template <class T>
    DELEGATE0( T* aObject, ReturnType(T::* aPtr)() )
    {
        m_ptr = reinterpret_cast<MemberPointer>( aPtr );
        m_object = reinterpret_cast<void*>( aObject );
    };


    ReturnType operator()() const
    {
        DELEGATE0<ReturnType>* casted = reinterpret_cast<DELEGATE0<ReturnType>*>( m_object );
        return ( casted->*m_ptr )();
    }

private:
    MemberPointer m_ptr;
    void* m_object;
};

#endif

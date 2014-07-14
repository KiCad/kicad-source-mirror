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

#ifndef __COROUTINE_H
#define __COROUTINE_H

#include <cstdlib>

#include <boost/context/fcontext.hpp>

#include "delegate.h"

/**
 *  Class COROUNTINE.
 *  Implements a coroutine. Wikipedia has a good explanation:
 *
 *  "Coroutines are computer program components that generalize subroutines to
 *  allow multiple entry points for suspending and resuming execution at certain locations.
 *  Coroutines are well-suited for implementing more familiar program components such as cooperative
 *  tasks, exceptions, event loop, iterators, infinite lists and pipes."
 *
 *  In other words, a coroutine can be considered a lightweight thread - which can be
 *  preempted only when it deliberately yields the control to the caller. This way,
 *  we avoid concurrency problems such as locking / race conditions.
 *
 *  Uses boost::context library to do the actual context switching.
 *
 *  This particular version takes a DELEGATE as an entry point, so it can invoke
 *  methods within a given object as separate coroutines.
 *
 *  See coroutine_example.cpp for sample code.
 */

template <class ReturnType, class ArgType>
class COROUTINE
{
public:
    COROUTINE() :
        m_saved( NULL ), m_self( NULL ), m_stack( NULL ), m_stackSize( c_defaultStackSize ),
        m_running( false )
    {
    }

    /**
     * Constructor
     * Creates a coroutine from a member method of an object
     */
    template <class T>
    COROUTINE( T* object, ReturnType(T::* ptr)( ArgType ) ) :
        m_func( object, ptr ), m_self( NULL ), m_saved( NULL ), m_stack( NULL ),
        m_stackSize( c_defaultStackSize ), m_running( false )
    {
    }

    /**
     * Constructor
     * Creates a coroutine from a delegate object
     */
    COROUTINE( DELEGATE<ReturnType, ArgType> aEntry ) :
        m_func( aEntry ), m_saved( NULL ), m_self( NULL ), m_stack( NULL ),
        m_stackSize( c_defaultStackSize ), m_running( false )
    {
    }

    ~COROUTINE()
    {
        if( m_saved )
            delete m_saved;

        if( m_stack )
            free( m_stack );
    }

    /**
     * Function Yield()
     *
     * Stops execution of the coroutine and returns control to the caller.
     * After a yield, Call() or Resume() methods invoked by the caller will
     * immediately return true, indicating that we are not done yet, just asleep.
     */
    void Yield()
    {
        boost::context::jump_fcontext( m_self, m_saved, 0 );
    }

    /**
     * Function Yield()
     *
     * Yield with a value - passes a value of given type to the caller.
     * Useful for implementing generator objects.
     */
    void Yield( ReturnType& aRetVal )
    {
        m_retVal = aRetVal;
        boost::context::jump_fcontext( m_self, m_saved, 0 );
    }

    /**
     * Function SetEntry()
     *
     * Defines the entry point for the coroutine, if not set in the constructor.
     */
    void SetEntry( DELEGATE<ReturnType, ArgType> aEntry )
    {
        m_func = aEntry;
    }

    /* Function Call()
     *
     * Starts execution of a coroutine, passing args as its arguments.
     * @return true, if the coroutine has yielded and false if it has finished its
     * execution (returned).
     */
    bool Call( ArgType aArgs )
    {
        // fixme: Clean up stack stuff. Add a guard
        m_stack = malloc( c_defaultStackSize );

        // align to 16 bytes
        void* sp = (void*) ( ( ( (ptrdiff_t) m_stack ) + m_stackSize - 0xf ) & ( ~0x0f ) );

        // correct the stack size
        m_stackSize -= ( (size_t) m_stack + m_stackSize - (size_t) sp );

        assert( m_self == NULL );
        assert( m_saved == NULL );

        m_args = &aArgs;
        m_self = boost::context::make_fcontext( sp, m_stackSize, callerStub );
        m_saved = new boost::context::fcontext_t();

        m_running = true;
        // off we go!
        boost::context::jump_fcontext( m_saved, m_self, reinterpret_cast<intptr_t>( this ) );
        return m_running;
    }

    /**
     * Function Resume()
     *
     * Resumes execution of a previously yielded coroutine.
     * @return true, if the coroutine has yielded again and false if it has finished its
     * execution (returned).
     */
    bool Resume()
    {
        boost::context::jump_fcontext( m_saved, m_self, 0 );

        return m_running;
    }

    /**
     * Function ReturnValue()
     *
     * Returns the yielded value (the argument Yield() was called with)
     */
    const ReturnType& ReturnValue() const
    {
        return m_retVal;
    }

    /**
     * Function Running()
     *
     * @return true, if the coroutine is active
     */
    bool Running() const
    {
        return m_running;
    }

private:
    static const int c_defaultStackSize = 2000000;    // fixme: make configurable

    /* real entry point of the coroutine */
    static void callerStub( intptr_t aData )
    {
        // get pointer to self
        COROUTINE<ReturnType, ArgType>* cor = reinterpret_cast<COROUTINE<ReturnType, ArgType>*>( aData );

        // call the coroutine method
        cor->m_retVal = cor->m_func( *cor->m_args );
        cor->m_running = false;

        // go back to wherever we came from.
        boost::context::jump_fcontext( cor->m_self, cor->m_saved, 0 );    // reinterpret_cast<intptr_t>( this ));
    }

    template <typename T>
    struct strip_ref
    {
        typedef T result;
    };

    template <typename T>
    struct strip_ref<T&>
    {
        typedef T result;
    };

    DELEGATE<ReturnType, ArgType> m_func;

    ///< pointer to coroutine entry arguments. Stripped of references
    ///< to avoid compiler errors.
    typename strip_ref<ArgType>::result* m_args;
    ReturnType m_retVal;

    ///< saved caller context
    boost::context::fcontext_t* m_saved;

    ///< saved coroutine context
    boost::context::fcontext_t* m_self;

    ///< coroutine stack
    void* m_stack;

    size_t m_stackSize;

    bool m_running;
};

#endif

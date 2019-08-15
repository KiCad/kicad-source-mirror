/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <cassert>
#include <cstdlib>
#include <type_traits>

#ifdef KICAD_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif

#include <advanced_config.h>
#include <system/libcontext.h>
#include <memory>

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
 *  Uses libcontext library to do the actual context switching.
 *
 *  This particular version takes a DELEGATE as an entry point, so it can invoke
 *  methods within a given object as separate coroutines.
 *
 *  See coroutine_example.cpp for sample code.
 */

template <typename ReturnType, typename ArgType>
class COROUTINE
{
private:
    class CALL_CONTEXT;

    struct INVOCATION_ARGS
    {
        enum
        {
            FROM_ROOT,      // a stub was called/a coroutine was resumed from the main-stack context
            FROM_ROUTINE,   // a stub was called/a coroutine was resumed from a coroutine context
            CONTINUE_AFTER_ROOT // a function sent a request to invoke a function on the main
                                // stack context
        } type; // invocation type
        COROUTINE*    destination;  // stores the coroutine pointer for the stub OR the coroutine
                                    // ptr for the coroutine to be resumed if a
                                    // root(main-stack)-call-was initiated.
        CALL_CONTEXT* context;      // pointer to the call context of the current callgraph this
                                    // call context holds a reference to the main stack context
    };

    using CONTEXT_T = libcontext::fcontext_t;
    using CALLEE_STORAGE = CONTEXT_T;

    class CALL_CONTEXT
    {
    public:
        void SetMainStack( CONTEXT_T* aStack )
        {
            m_mainStackContext = aStack;
        }

        void RunMainStack( COROUTINE* aCor, std::function<void()> aFunc )
        {
            m_mainStackFunction = std::move( aFunc );
            INVOCATION_ARGS args{ INVOCATION_ARGS::CONTINUE_AFTER_ROOT, aCor, this };

            libcontext::jump_fcontext( &aCor->m_callee, *m_mainStackContext,
                reinterpret_cast<intptr_t>( &args ) );
        }

        void Continue( INVOCATION_ARGS* args )
        {
            while( args->type == INVOCATION_ARGS::CONTINUE_AFTER_ROOT )
            {
                m_mainStackFunction();
                args->type = INVOCATION_ARGS::FROM_ROOT;
                args = args->destination->doResume( args );
            }
        }

    private:
        CONTEXT_T*              m_mainStackContext;
        std::function<void()>   m_mainStackFunction;
    };

public:
    COROUTINE() :
        COROUTINE( nullptr )
    {
    }

    /**
     * Constructor
     * Creates a coroutine from a member method of an object
     */
    template <class T>
    COROUTINE( T* object, ReturnType(T::*ptr)( ArgType ) ) :
        COROUTINE( std::bind( ptr, object, std::placeholders::_1 ) )
    {
    }

    /**
     * Constructor
     * Creates a coroutine from a delegate object
     */
    COROUTINE( std::function<ReturnType(ArgType)> aEntry ) :
        m_func( std::move( aEntry ) ),
        m_running( false ),
        m_args( 0 ),
        m_caller( nullptr ),
        m_callContext( nullptr ),
        m_callee( nullptr ),
        m_retVal( 0 )
#ifdef KICAD_USE_VALGRIND
        ,valgrind_stack( 0 )
#endif
    {
        m_stacksize = ADVANCED_CFG::GetCfg().m_coroutineStackSize;
    }

    ~COROUTINE()
    {
#ifdef KICAD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER( valgrind_stack );
#endif
    }

public:
    /**
     * Function KiYield()
     *
     * Stops execution of the coroutine and returns control to the caller.
     * After a yield, Call() or Resume() methods invoked by the caller will
     * immediately return true, indicating that we are not done yet, just asleep.
     */
    void KiYield()
    {
        jumpOut();
    }

    /**
     * Function KiYield()
     *
     * KiYield with a value - passes a value of given type to the caller.
     * Useful for implementing generator objects.
     */
    void KiYield( ReturnType& aRetVal )
    {
        m_retVal = aRetVal;
        jumpOut();
    }

    /**
     * Function SetEntry()
     *
     * Defines the entry point for the coroutine, if not set in the constructor.
     */
    void SetEntry( std::function<ReturnType(ArgType)> aEntry )
    {
        m_func = std::move( aEntry );
    }

    /**
     * Function RunMainStack()
     *
     * Run a functor inside the application main stack context
     * Call this function for example if the operation will spawn a webkit browser instance which
     * will walk the stack to the upper border of the address space on mac osx systems because
     * its javascript needs garbage collection (for example if you paste text into an edit box).
     */
    void RunMainStack( std::function<void()> func )
    {
        assert( m_callContext );
        m_callContext->RunMainStack( this, std::move( func ) );
    }

   /**
    * Function Call()
    *
    * Starts execution of a coroutine, passing args as its arguments. Call this method
    * from the application main stack only.
    * @return true, if the coroutine has yielded and false if it has finished its
    * execution (returned).
    */
    bool Call( ArgType aArg )
    {
        CALL_CONTEXT ctx;
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROOT, this, &ctx };
        ctx.Continue( doCall( &args, aArg ) );

        return Running();
    }

   /**
    * Function Call()
    *
    * Starts execution of a coroutine, passing args as its arguments. Call this method
    * for a nested coroutine invocation.
    * @return true, if the coroutine has yielded and false if it has finished its
    * execution (returned).
    */
    bool Call( const COROUTINE& aCor, ArgType aArg )
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, this, aCor.m_callContext };
        doCall( &args, aArg );
        // we will not be asked to continue

        return Running();
    }

    /**
    * Function Resume()
    *
    * Resumes execution of a previously yielded coroutine. Call this method only
    * from the main application stack.
    * @return true, if the coroutine has yielded again and false if it has finished its
    * execution (returned).
    */
    bool Resume()
    {
        CALL_CONTEXT ctx;
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROOT, this, &ctx };
        ctx.Continue( doResume( &args ) );

        return Running();
    }

    /**
    * Function Resume()
    *
    * Resumes execution of a previously yielded coroutine. Call this method
    * for a nested coroutine invocation.
    * @return true, if the coroutine has yielded again and false if it has finished its
    * execution (returned).
    */
    bool Resume( const COROUTINE& aCor )
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, this, aCor.m_callContext };
        doResume( &args );
        // we will not be asked to continue

        return Running();
    }

    /**
     * Function ReturnValue()
     *
     * Returns the yielded value (the argument KiYield() was called with)
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
    INVOCATION_ARGS* doCall( INVOCATION_ARGS* aInvArgs, ArgType aArgs )
    {
        assert( m_func );
        assert( !m_callee );

        m_args = &aArgs;

        assert( m_stack == nullptr );

        size_t stackSize = m_stacksize;
        void* sp = nullptr;

        #ifndef LIBCONTEXT_HAS_OWN_STACK
        // fixme: Clean up stack stuff. Add a guard
        m_stack.reset( new char[stackSize] );

        // align to 16 bytes
        sp = (void*)((((ptrdiff_t) m_stack.get()) + stackSize - 0xf) & (~0x0f));

        // correct the stack size
        stackSize -= size_t( ( (ptrdiff_t) m_stack.get() + stackSize ) - (ptrdiff_t) sp );

#ifdef KICAD_USE_VALGRIND
        valgrind_stack = VALGRIND_STACK_REGISTER( sp, m_stack.get() );
#endif
        #endif

        m_callee = libcontext::make_fcontext( sp, stackSize, callerStub );
        m_running = true;

        // off we go!
        return jumpIn( aInvArgs );
    }

    INVOCATION_ARGS* doResume( INVOCATION_ARGS* args )
    {
        return jumpIn( args );
    }

    /* real entry point of the coroutine */
    static void callerStub( intptr_t aData )
    {
        INVOCATION_ARGS& args = *reinterpret_cast<INVOCATION_ARGS*>( aData );
        // get pointer to self
        COROUTINE* cor     = args.destination;
        cor->m_callContext = args.context;

        if( args.type == INVOCATION_ARGS::FROM_ROOT )
            cor->m_callContext->SetMainStack( &cor->m_caller );

        // call the coroutine method
        cor->m_retVal = cor->m_func( *(cor->m_args) );
        cor->m_running = false;

        // go back to wherever we came from.
        cor->jumpOut();
    }

    INVOCATION_ARGS* jumpIn( INVOCATION_ARGS* args )
    {
        args = reinterpret_cast<INVOCATION_ARGS*>(
            libcontext::jump_fcontext( &m_caller, m_callee,
                                           reinterpret_cast<intptr_t>( args ) )
            );

        return args;
    }

    void jumpOut()
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, nullptr, nullptr };
        INVOCATION_ARGS* ret;
        ret = reinterpret_cast<INVOCATION_ARGS*>(
            libcontext::jump_fcontext( &m_callee, m_caller,
                                           reinterpret_cast<intptr_t>( &args ) )
            );

        m_callContext = ret->context;

        if( ret->type == INVOCATION_ARGS::FROM_ROOT )
        {
            m_callContext->SetMainStack( &m_caller );
        }
    }

    ///< coroutine stack
    std::unique_ptr<char[]> m_stack;

    int m_stacksize;

    std::function<ReturnType( ArgType )> m_func;

    bool m_running;

    ///< pointer to coroutine entry arguments. Stripped of references
    ///< to avoid compiler errors.
    typename std::remove_reference<ArgType>::type* m_args;

    ///< saved caller context
    CONTEXT_T m_caller;

    ///< main stack information
    CALL_CONTEXT* m_callContext;

    ///< saved coroutine context
    CALLEE_STORAGE m_callee;

    ReturnType m_retVal;

#ifdef KICAD_USE_VALGRIND
    uint32_t valgrind_stack;
#endif
};

#endif

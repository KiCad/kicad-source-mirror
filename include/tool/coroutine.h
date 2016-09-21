/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/version.hpp>
#include <type_traits>

#if BOOST_VERSION < 106100
#include <boost/context/fcontext.hpp>
#else
#include <boost/context/execution_context.hpp>
#include <boost/context/protected_fixedsize_stack.hpp>
#endif

/**
 * Note: in the history of boost, two changes to the context interface happened.
 * [1.54, 1.56)
 * http://www.boost.org/doc/libs/1_55_0/libs/context/doc/html/context/context/boost_fcontext.html
 *       intptr_t    jump_fcontext(
 *                       fcontext_t* ofc,
 *                       fcontext_t const* nfc,
 *                       intptr_t vp,
 *                       bool preserve_fpu = true
 *                   );
 *
 *       fcontext_t* make_fcontext(
 *                       void* sp,
 *                       std::size_t size,
 *                       void (*fn)(intptr_t)
 *                   );
 *
 * [1.56, 1.61)
 * http://www.boost.org/doc/libs/1_56_0/libs/context/doc/html/context/context/boost_fcontext.html
 *       intptr_t    jump_fcontext(
 *                       fcontext_t* ofc,
 *                       fcontext_t nfc,            <-----
 *                       intptr_t vp,
 *                       bool preserve_fpu = true
 *                   );
 *
 *       fcontext_t  make_fcontext(                 <-----
 *                       void* sp,
 *                       std::size_t size,
 *                       void(*fn)(intptr_t)
 *                   );
 *
 * [1.61, oo)
 * http://www.boost.org/doc/libs/1_61_0/libs/context/doc/html/context/ecv2.html
 *       fcontext_t is hidden away behind the boost::execution_context(_v2) and the stack is created on behalf of
 *       the user.
 */

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

template <typename ReturnType, typename ArgType>
class COROUTINE
{
private:
    class CALL_CONTEXT;

    struct INVOCATION_ARGS
    {
        enum
        {
            FROM_ROOT,      // a stub was called/a corutine was resumed from the main-stack context
            FROM_ROUTINE,   // a stub was called/a coroutine was resumed fron a coroutine context
            CONTINUE_AFTER_ROOT // a function sent a request to invoke a function on the main
                                // stack context
        } type; // invocation type
        COROUTINE*    destination;  // stores the coroutine pointer for the stub OR the coroutine
                                    // ptr for the coroutine to be resumed if a
                                    // root(main-stack)-call-was initiated.
        CALL_CONTEXT* context;      // pointer to the call context of the current callgraph this
                                    // call context holds a reference to the main stack context
    };

#if BOOST_VERSION < 106100
    using CONTEXT_T = boost::context::fcontext_t;
#else
    using CONTEXT_T = boost::context::execution_context<INVOCATION_ARGS*>;
#endif

#if BOOST_VERSION < 105600
    using CALLEE_STORAGE = CONTEXT_T*;
#else
    using CALLEE_STORAGE = CONTEXT_T;
#endif

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

#if BOOST_VERSION < 105600
            boost::context::jump_fcontext( aCor->m_callee, m_mainStackContext,
                reinterpret_cast<intptr_t>( &args ) );
#elif BOOST_VERSION < 106100
            boost::context::jump_fcontext( &aCor->m_callee, *m_mainStackContext,
                reinterpret_cast<intptr_t>( &args ) );
#else
            *m_mainStackContext = std::get<0>( ( *m_mainStackContext )( &args ) );
#endif
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
#if BOOST_VERSION < 106100 // -> m_callee = void* or void**
        m_callee( nullptr ),
#endif
        m_retVal( 0 )
    {
    }

    ~COROUTINE()
    {
    }

public:
    /**
     * Function Yield()
     *
     * Stops execution of the coroutine and returns control to the caller.
     * After a yield, Call() or Resume() methods invoked by the caller will
     * immediately return true, indicating that we are not done yet, just asleep.
     */
    void Yield()
    {
        jumpOut();
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
    INVOCATION_ARGS* doCall( INVOCATION_ARGS* aInvArgs, ArgType aArgs )
    {
        assert( m_func );
        assert( !m_callee );

        m_args = &aArgs;

#if BOOST_VERSION < 106100
        assert( m_stack == nullptr );

        // fixme: Clean up stack stuff. Add a guard
        size_t stackSize = c_defaultStackSize;
        m_stack.reset( new char[stackSize] );

        // align to 16 bytes
        void* sp = (void*)((((ptrdiff_t) m_stack.get()) + stackSize - 0xf) & (~0x0f));

        // correct the stack size
        stackSize -= size_t( ( (ptrdiff_t) m_stack.get() + stackSize ) - (ptrdiff_t) sp );

        m_callee = boost::context::make_fcontext( sp, stackSize, callerStub );
#else
        m_callee = CONTEXT_T(
            std::allocator_arg_t(),
            boost::context::protected_fixedsize_stack( c_defaultStackSize ),
            &COROUTINE::callerStub
        );
#endif

        m_running = true;

        // off we go!
        return jumpIn( aInvArgs );
    }

    INVOCATION_ARGS* doResume( INVOCATION_ARGS* args )
    {
        return jumpIn( args );
    }

    /* real entry point of the coroutine */
#if BOOST_VERSION < 106100
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
#else
    /* real entry point of the coroutine */
    static CONTEXT_T callerStub( CONTEXT_T caller, INVOCATION_ARGS* aArgsPtr )
    {
        const auto& args = *aArgsPtr;
        auto* cor = args.destination;

        cor->m_caller      = std::move( caller );
        cor->m_callContext = args.context;

        if( args.type == INVOCATION_ARGS::FROM_ROOT )
            cor->m_callContext->SetMainStack( &cor->m_caller );

        // call the coroutine method
        cor->m_retVal = cor->m_func( *(cor->m_args) );
        cor->m_running = false;

        // go back to wherever we came from.
        return std::move( cor->m_caller );
    }
#endif

    INVOCATION_ARGS* jumpIn( INVOCATION_ARGS* args )
    {
#if BOOST_VERSION < 105600
        args = reinterpret_cast<INVOCATION_ARGS*>(
            boost::context::jump_fcontext( &m_caller, m_callee,
                                           reinterpret_cast<intptr_t>( args ) )
            );
#elif BOOST_VERSION < 106100
        args = reinterpret_cast<INVOCATION_ARGS*>(
            boost::context::jump_fcontext( &m_caller, m_callee,
                                           reinterpret_cast<intptr_t>( args ) )
            );
#else
        std::tie( m_callee, args ) = m_callee( args );
#endif

        return args;
    }

    void jumpOut()
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, nullptr, nullptr };
        INVOCATION_ARGS* ret;
#if BOOST_VERSION < 105600
        ret = reinterpret_cast<INVOCATION_ARGS*>(
            boost::context::jump_fcontext( m_callee, &m_caller,
                                           reinterpret_cast<intptr_t>( &args ) )
            );
#elif BOOST_VERSION < 106100
        ret = reinterpret_cast<INVOCATION_ARGS*>(
            boost::context::jump_fcontext( &m_callee, m_caller,
                                           reinterpret_cast<intptr_t>( &args ) )
            );
#else
        std::tie( m_caller, ret ) = m_caller( &args );
#endif

        m_callContext = ret->context;

        if( ret->type == INVOCATION_ARGS::FROM_ROOT )
        {
            m_callContext->SetMainStack( &m_caller );
        }
    }

    static constexpr int c_defaultStackSize = 2000000;    // fixme: make configurable

#if BOOST_VERSION < 106100
    ///< coroutine stack
    std::unique_ptr<char[]> m_stack;
#endif

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
};

#endif

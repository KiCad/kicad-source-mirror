/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#include <cassert>
#include <cstdlib>
#include <type_traits>

#ifdef KICAD_USE_VALGRIND
#include <valgrind/valgrind.h>
#endif
#ifdef KICAD_SANITIZE_THREADS
#include <sanitizer/tsan_interface.h>
#endif
#ifdef KICAD_SANITIZE_ADDRESS
#include <sanitizer/asan_interface.h>
#endif

#include <libcontext.h>
#include <functional>
#include <optional>
#include <memory>
#include <advanced_config.h>

#include <trace_helpers.h>
#include <wx/log.h>

#ifdef _WIN32
#include <windows.h>
#else                   // Linux, BSD, MacOS
#include <unistd.h>     // getpagesize
#include <sys/mman.h>   // mmap, mprotect, munmap
#endif

/**
 *  Implement a coroutine.
 *
 * Wikipedia has a good explanation:
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

    struct CONTEXT_T
    {
        libcontext::fcontext_t ctx;    // The context itself

#ifdef KICAD_SANITIZE_THREADS
        void* tsan_fiber;               // The TSAN fiber for this context
        bool  own_tsan_fiber;           // Do we own this TSAN fiber? (we only delete fibers we own)
#endif

        CONTEXT_T() :
            ctx( nullptr )
#ifdef KICAD_SANITIZE_THREADS
            ,tsan_fiber( nullptr )
            ,own_tsan_fiber( true )
#endif
            {}

        ~CONTEXT_T()
        {
#ifdef KICAD_SANITIZE_THREADS
            // Only destroy the fiber when we own it
            if( own_tsan_fiber )
                __tsan_destroy_fiber( tsan_fiber );
#endif
        }
    };

    class CALL_CONTEXT
    {
    public:
        CALL_CONTEXT() :
            m_mainStackContext( nullptr )
        {
        }

        ~CALL_CONTEXT()
        {
            if( m_mainStackContext )
                libcontext::release_fcontext( m_mainStackContext->ctx );
        }


        void SetMainStack( CONTEXT_T* aStack )
        {
            m_mainStackContext = aStack;
        }

        void RunMainStack( COROUTINE* aCor, std::function<void()> aFunc )
        {
            m_mainStackFunction = std::move( aFunc );
            INVOCATION_ARGS args{ INVOCATION_ARGS::CONTINUE_AFTER_ROOT, aCor, this };

#ifdef KICAD_SANITIZE_THREADS
            // Tell TSAN we are changing fibers
            __tsan_switch_to_fiber( m_mainStackContext->tsan_fiber, 0 );
#endif

            libcontext::jump_fcontext( &( aCor->m_callee.ctx ), m_mainStackContext->ctx,
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
     * Create a coroutine from a member method of an object.
     */
    template <class T>
    COROUTINE( T* object, ReturnType(T::*ptr)( ArgType ) ) :
        COROUTINE( std::bind( ptr, object, std::placeholders::_1 ) )
    {
    }

    /**
     * Create a coroutine from a delegate object.
     */
    COROUTINE( std::function<ReturnType( ArgType )> aEntry ) :
        m_func( std::move( aEntry ) ),
        m_running( false ),
        m_args( nullptr ),
        m_caller(),
        m_callContext( nullptr ),
        m_callee(),
        m_retVal( 0 )
#ifdef KICAD_USE_VALGRIND
        ,m_valgrind_stack( 0 )
#endif
#ifdef KICAD_SANITIZE_ADDRESS
        ,asan_stack( nullptr )
#endif
    {
        m_stacksize = ADVANCED_CFG::GetCfg().m_CoroutineStackSize;
    }

    ~COROUTINE()
    {
#ifdef KICAD_USE_VALGRIND
        VALGRIND_STACK_DEREGISTER( m_valgrind_stack );
#endif

        if( m_caller.ctx )
            libcontext::release_fcontext( m_caller.ctx );

        if( m_callee.ctx )
            libcontext::release_fcontext( m_callee.ctx );
    }

public:
    /**
     * Stop execution of the coroutine and returns control to the caller.
     *
     * After a yield, Call() or Resume() methods invoked by the caller will
     * immediately return true, indicating that we are not done yet, just asleep.
     */
    void KiYield()
    {
        jumpOut();
    }

    /**
     * KiYield with a value.
     *
     * Passe a value of given type to the caller.  Useful for implementing generator objects.
     */
    void KiYield( ReturnType& aRetVal )
    {
        m_retVal = aRetVal;
        jumpOut();
    }

    /**
     * Run a functor inside the application main stack context.
     *
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
    * Start execution of a coroutine, passing args as its arguments.
    *
    * Call this method from the application main stack only.
    *
    * @return true if the coroutine has yielded and false if it has finished its
    *         execution (returned).
    */
    bool Call( ArgType aArg )
    {
        CALL_CONTEXT ctx;
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROOT, this, &ctx };

#ifdef KICAD_SANITIZE_THREADS
        // Get the TSAN fiber for the current stack here
        m_caller.tsan_fiber     = __tsan_get_current_fiber();
        m_caller.own_tsan_fiber = false;
#endif

        wxLogTrace( kicadTraceCoroutineStack,  "COROUTINE::Call (from root)" );

        ctx.Continue( doCall( &args, aArg ) );

        return Running();
    }

   /**
    * Start execution of a coroutine, passing args as its arguments.
    *
    * Call this method for a nested coroutine invocation.
    *
    * @return true if the coroutine has yielded and false if it has finished its
    *         execution (returned).
    */
    bool Call( const COROUTINE& aCor, ArgType aArg )
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, this, aCor.m_callContext };

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::Call (from routine)" ) );

        doCall( &args, aArg );

        // we will not be asked to continue
        return Running();
    }

    /**
     * Resume execution of a previously yielded coroutine.
     *
     * Call this method only from the main application stack.
     *
     * @return true if the coroutine has yielded again and false if it has finished its
     *         execution (returned).
     */
    bool Resume()
    {
        CALL_CONTEXT ctx;
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROOT, this, &ctx };

#ifdef KICAD_SANITIZE_THREADS
        // Get the TSAN fiber for the current stack here
        m_caller.tsan_fiber     = __tsan_get_current_fiber();
        m_caller.own_tsan_fiber = false;
#endif

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::Resume (from root)" ) );

        ctx.Continue( doResume( &args ) );

        return Running();
    }

    /**
     * Resume execution of a previously yielded coroutine.
     *
     * Call this method for a nested coroutine invocation.
     *
     * @return true if the coroutine has yielded again and false if it has finished its
     *         execution (returned).
     */
    bool Resume( const COROUTINE& aCor )
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, this, aCor.m_callContext };

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::Resume (from routine)" ) );

        doResume( &args );

        // we will not be asked to continue
        return Running();
    }

    /**
     * Return the yielded value (the argument KiYield() was called with).
     */
    const ReturnType& ReturnValue() const
    {
        return m_retVal;
    }

    /**
     * @return true if the coroutine is active.
     */
    bool Running() const
    {
        return m_running;
    }

private:
    INVOCATION_ARGS* doCall( INVOCATION_ARGS* aInvArgs, ArgType aArgs )
    {
        assert( m_func );
        assert( !( m_callee.ctx ) );

        m_args = &aArgs;

        std::size_t stackSize = m_stacksize;
        void* sp = nullptr;

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::doCall" ) );

#ifndef LIBCONTEXT_HAS_OWN_STACK
        assert( !m_stack );

        const std::size_t systemPageSize = SystemPageSize();

        // calculate the correct number of pages to allocate based on request stack size
        std::size_t pages = ( m_stacksize + systemPageSize - 1 ) / systemPageSize;

        // we allocate an extra page for the guard
        stackSize = ( pages + 1 ) * systemPageSize;

        m_stack.reset( static_cast<char*>( MapMemory( stackSize ) ) );
        m_stack.get_deleter().SetSize( stackSize );

        // now configure the first page (by only specifying a single page_size from vp)
        // that will act as the guard page
        // the stack will grow from the end and hopefully never into this guarded region
        GuardMemory( m_stack.get(), systemPageSize );

        sp = static_cast<char*>( m_stack.get() ) + stackSize;

#ifdef KICAD_USE_VALGRIND
        m_valgrind_stack = VALGRIND_STACK_REGISTER( sp, m_stack.get() );
#endif
#endif

#ifdef KICAD_SANITIZE_THREADS
        // Create a new fiber to go with the new context
        m_callee.tsan_fiber     = __tsan_create_fiber( 0 );
        m_callee.own_tsan_fiber = true;

        __tsan_set_fiber_name( m_callee.tsan_fiber, "Coroutine fiber" );
#endif

        m_callee.ctx = libcontext::make_fcontext( sp, stackSize, callerStub );
        m_running = true;

        // off we go!
        return jumpIn( aInvArgs );
    }

#ifndef LIBCONTEXT_HAS_OWN_STACK
    /// A functor that frees the stack.
    struct STACK_DELETER
    {
#ifdef _WIN32
        void SetSize( std::size_t ) {}
        void operator()( void* aMem ) noexcept { ::VirtualFree( aMem, 0, MEM_RELEASE ); }
#else
        std::size_t m_size = 0;

        void SetSize( std::size_t aSize ) { m_size = aSize; }
        void operator()( void* aMem ) noexcept { ::munmap( aMem, m_size ); }
#endif
    };

    /// The size of the mappable memory page size.
    static inline size_t SystemPageSize()
    {
        static std::optional<size_t> systemPageSize;

        if( !systemPageSize.has_value() )
        {
#ifdef _WIN32
            SYSTEM_INFO si;
            ::GetSystemInfo( &si );
            systemPageSize = static_cast<size_t>( si.dwPageSize );
#else
            int size = getpagesize();
            systemPageSize = static_cast<size_t>( size );
#endif
        }

        return systemPageSize.value();
    }

    /// Map a page-aligned memory region into our address space.
    static inline void* MapMemory( size_t aAllocSize )
    {
#ifdef _WIN32
        void* mem = ::VirtualAlloc( 0, aAllocSize, MEM_COMMIT, PAGE_READWRITE );

        if( !mem )
            throw std::bad_alloc();
#else
        void* mem = ::mmap( 0, aAllocSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0 );

        if( mem == (void*) -1 )
            throw std::bad_alloc();
#endif
        return mem;
    }

    /// Change protection of memory page(s) to act as stack guards.
    static inline void GuardMemory( void* aAddress, size_t aGuardSize )
    {
#ifdef _WIN32
        DWORD old_prot; // dummy var since the arg cannot be NULL
        BOOL res = ::VirtualProtect( aAddress, aGuardSize,
                                    PAGE_READWRITE | PAGE_GUARD, &old_prot );
#else
        bool res = ( 0 == ::mprotect( aAddress, aGuardSize, PROT_NONE ) );
#endif
        if( !res )
            wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::GuardMemory has failed" ) );
    }
#endif // LIBCONTEXT_HAS_OWN_STACK

    INVOCATION_ARGS* doResume( INVOCATION_ARGS* args )
    {
        return jumpIn( args );
    }

    // real entry point of the coroutine
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
#ifdef KICAD_SANITIZE_THREADS
        // Tell TSAN we are changing fibers to the callee
        __tsan_switch_to_fiber( m_callee.tsan_fiber, 0 );
#endif

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::jumpIn" ) );

        args = reinterpret_cast<INVOCATION_ARGS*>(
            libcontext::jump_fcontext( &( m_caller.ctx ), m_callee.ctx,
                                       reinterpret_cast<intptr_t>( args ) )
            );

        return args;
    }

    void jumpOut()
    {
        INVOCATION_ARGS args{ INVOCATION_ARGS::FROM_ROUTINE, nullptr, nullptr };
        INVOCATION_ARGS* ret;

#ifdef KICAD_SANITIZE_THREADS
        // Tell TSAN we are changing fibers back to the caller
        __tsan_switch_to_fiber( m_caller.tsan_fiber, 0 );
#endif

        wxLogTrace( kicadTraceCoroutineStack, wxT( "COROUTINE::jumpOut" ) );

        ret = reinterpret_cast<INVOCATION_ARGS*>(
            libcontext::jump_fcontext( &( m_callee.ctx ), m_caller.ctx,
                                       reinterpret_cast<intptr_t>( &args ) )
            );

        m_callContext = ret->context;

        if( ret->type == INVOCATION_ARGS::FROM_ROOT )
        {
            m_callContext->SetMainStack( &m_caller );
        }
    }

#ifndef LIBCONTEXT_HAS_OWN_STACK
    /// Coroutine stack.
    std::unique_ptr<char[], struct STACK_DELETER> m_stack;
#endif

    int m_stacksize;

    std::function<ReturnType( ArgType )> m_func;

    bool m_running;

    /// Pointer to coroutine entry arguments stripped of references to avoid compiler errors.
    typename std::remove_reference<ArgType>::type* m_args;

    /// Saved caller context.
    CONTEXT_T m_caller;

    /// Main stack information.
    CALL_CONTEXT* m_callContext;

    /// Saved coroutine context.
    CONTEXT_T m_callee;

    ReturnType m_retVal;

#ifdef KICAD_USE_VALGRIND
    uint32_t m_valgrind_stack;
#endif

#ifdef KICAD_SANITIZE_ADDRESS
    void* asan_stack;
#endif
};

#endif

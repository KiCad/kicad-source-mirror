/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <navlib_safe_init.h>

#include <wx/log.h>

#include <atomic>
#include <exception>
#include <mutex>

#if !defined( __WINDOWS__ )
#include <signal.h>
#include <setjmp.h>
#endif

static std::atomic<bool> s_driverCrashed( false );

#if !defined( __WINDOWS__ )

// SIGABRT disposition and the recovery jump buffer are process-wide, so concurrent
// initialization attempts must be serialized. The saved disposition uses static storage
// because an automatic local modified after sigsetjmp() is indeterminate after siglongjmp().
static std::mutex s_initMutex;
static sigjmp_buf s_jumpBuffer;
static struct sigaction s_savedSigAbrtAction;

static void navlibSigAbrtHandler( int aSignal )
{
    siglongjmp( s_jumpBuffer, 1 );
}

#endif


bool SafeNavlibInit( const std::function<void()>& aInitFunc )
{
    if( s_driverCrashed.load() )
        return false;

#if !defined( __WINDOWS__ )
    // Install a temporary SIGABRT handler to recover from abort() calls triggered by
    // buggy 3Dconnexion drivers. The driver on some macOS versions calls std::terminate()
    // from a noexcept context inside NlCreate, which calls abort(). By catching SIGABRT
    // we can recover instead of crashing the entire application.
    std::lock_guard<std::mutex> lock( s_initMutex );

    // Re-check under the lock in case a concurrent caller latched the crash flag while we
    // waited on the mutex.
    if( s_driverCrashed.load() )
        return false;

    struct sigaction newAction;

    newAction.sa_handler = navlibSigAbrtHandler;
    sigemptyset( &newAction.sa_mask );
    newAction.sa_flags = 0;

    // Arm the jump buffer before installing the handler so an abort() that fires the
    // instant the handler is active always lands here with a valid buffer.
    if( sigsetjmp( s_jumpBuffer, 1 ) != 0 )
    {
        // We got here via siglongjmp from the SIGABRT handler.
        sigaction( SIGABRT, &s_savedSigAbrtAction, nullptr );
        s_driverCrashed.store( true );
        wxLogWarning( wxT( "3Dconnexion driver crashed during initialization. "
                           "SpaceMouse support will be disabled for this session." ) );
        return false;
    }

    sigaction( SIGABRT, &newAction, &s_savedSigAbrtAction );

    try
    {
        aInitFunc();
        sigaction( SIGABRT, &s_savedSigAbrtAction, nullptr );
        return true;
    }
    catch( const std::exception& e )
    {
        sigaction( SIGABRT, &s_savedSigAbrtAction, nullptr );
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "3Dconnexion initialization failed: %s" ), e.what() );
        return false;
    }
    catch( ... )
    {
        sigaction( SIGABRT, &s_savedSigAbrtAction, nullptr );
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "3Dconnexion initialization failed with unknown exception" ) );
        return false;
    }
#else
    // On Windows, VEH can handle structured exceptions but std::terminate from a noexcept
    // context is not recoverable. We still catch C++ exceptions.
    try
    {
        aInitFunc();
        return true;
    }
    catch( const std::exception& e )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "3Dconnexion initialization failed: %s" ), e.what() );
        return false;
    }
    catch( ... )
    {
        wxLogTrace( wxT( "KI_TRACE_NAVLIB" ),
                    wxT( "3Dconnexion initialization failed with unknown exception" ) );
        return false;
    }
#endif
}


bool NavlibDriverCrashed()
{
    return s_driverCrashed.load();
}

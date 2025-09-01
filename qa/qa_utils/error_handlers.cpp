/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <csignal>
#include <string>
#include <iostream>
#include <qa_utils/error_handlers.h>
#ifdef __APPLE__
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#endif
#ifdef __linux__
#define BOOST_STACKTRACE_USE_ADDR2LINE
#endif
#include <boost/stacktrace.hpp>
#include <boost/test/unit_test_monitor.hpp>


static std::string get_signal_string( int signum )
{
#ifdef _WIN32
    switch( signum )
    {
    case SIGABRT: return "SIGABRT (Abnormal termination)";
    case SIGFPE: return "SIGFPE (Floating-point exception)";
    case SIGILL: return "SIGILL (Illegal instruction)";
    case SIGINT: return "SIGINT (Interrupt)";
    case SIGSEGV: return "SIGSEGV (Segmentation fault)";
    case SIGTERM: return "SIGTERM (Termination request)";
    default: return "Unknown signal";
    }
#else
    return strsignal( signum );
#endif
}


static void signal_handler( int signum )
{
    // Associate the signal number with a name
    ::signal( signum, SIG_DFL ); // Re-register the default handler

    std::cerr << std::endl << "Signal caught: " << get_signal_string( signum ) << " (" << signum << ")" << std::endl;
    std::cerr << "Stack trace:" << std::endl;

    // Print the stack trace to standard error
    std::cerr << boost::stacktrace::stacktrace() << std::endl;

    // Let the default handler terminate the program
    ::raise( signum );
}


KI_SIGNAL_HANDLER_FIXTURE::KI_SIGNAL_HANDLER_FIXTURE()
{
    ::signal( SIGSEGV, &signal_handler );
    ::signal( SIGABRT, &signal_handler );
    ::signal( SIGFPE, &signal_handler );
};


static void translate_std_exception( const std::exception& e )
{
    std::cerr << std::endl << "Caught exception: " << e.what() << std::endl << std::endl;

    // Print the stack trace
    std::cerr << "Stack trace:" << std::endl;
    std::cerr << boost::stacktrace::stacktrace() << std::endl << std::endl;

    // Re-throw the exception to let Boost.Test handle it and fail the test
    throw;
}


KI_STACK_TRACE_FIXTURE::KI_STACK_TRACE_FIXTURE()
{
    boost::unit_test::unit_test_monitor.register_exception_translator<std::exception>( &translate_std_exception );
}
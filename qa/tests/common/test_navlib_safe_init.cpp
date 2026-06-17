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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <navlib_safe_init.h>
#include <stdexcept>

#if !defined( __WINDOWS__ )
#include <sys/wait.h>
#include <unistd.h>
#endif

BOOST_AUTO_TEST_SUITE( NavlibSafeInit )


BOOST_AUTO_TEST_CASE( SuccessfulInit )
{
    bool called = false;

    bool result = SafeNavlibInit( [&called]()
    {
        called = true;
    } );

    BOOST_CHECK( result );
    BOOST_CHECK( called );
    BOOST_CHECK( !NavlibDriverCrashed() );
}


BOOST_AUTO_TEST_CASE( ExceptionHandling )
{
    bool result = SafeNavlibInit( []()
    {
        throw std::runtime_error( "test exception" );
    } );

    BOOST_CHECK( !result );
    BOOST_CHECK( !NavlibDriverCrashed() );
}


BOOST_AUTO_TEST_CASE( SystemErrorHandling )
{
    bool result = SafeNavlibInit( []()
    {
        throw std::system_error( std::make_error_code( std::errc::connection_refused ),
                                 "test system error" );
    } );

    BOOST_CHECK( !result );
    BOOST_CHECK( !NavlibDriverCrashed() );
}


BOOST_AUTO_TEST_CASE( UnknownExceptionHandling )
{
    bool result = SafeNavlibInit( []()
    {
        throw 42;
    } );

    BOOST_CHECK( !result );
    BOOST_CHECK( !NavlibDriverCrashed() );
}


#if !defined( __WINDOWS__ )
BOOST_AUTO_TEST_CASE( AbortRecovery )
{
    // Test that SafeNavlibInit recovers from abort() by running the test in a
    // child process. The parent checks the child's exit status.
    pid_t pid = fork();

    if( pid == 0 )
    {
        // Child process: test that SafeNavlibInit catches abort()
        bool result = SafeNavlibInit( []()
        {
            abort();
        } );

        // If we get here, recovery worked
        _exit( result ? 1 : 0 );
    }
    else
    {
        BOOST_REQUIRE( pid > 0 );

        int status = 0;
        waitpid( pid, &status, 0 );

        // The child should have exited normally (not killed by signal) with code 0
        // (indicating SafeNavlibInit returned false after catching the abort)
        BOOST_CHECK_MESSAGE( WIFEXITED( status ),
                             "Child process should exit normally after abort recovery" );

        if( WIFEXITED( status ) )
        {
            BOOST_CHECK_EQUAL( WEXITSTATUS( status ), 0 );
        }
    }
}
#endif


BOOST_AUTO_TEST_SUITE_END()

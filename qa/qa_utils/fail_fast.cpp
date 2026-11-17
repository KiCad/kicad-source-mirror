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

// kicad_qa_fail_fast compiles this into each QA test executable

#if defined( _WIN32 )

#include <windows.h>

#include <cstdio>
#include <cstdlib>

#if defined( _MSC_VER ) && defined( _DEBUG )
#include <crtdbg.h>
#endif

namespace
{
#if defined( _MSC_VER ) && defined( _DEBUG )
// Ends the run on a CRT assert or error, which report mode alone would log and carry on past
int failOnCrtReport( int aType, char* aMessage, int* )
{
    if( aType == _CRT_WARN )
        return FALSE;

    std::fputs( aMessage, stderr );

    // _Exit skips the stdio flush, so buffered test output would be lost
    std::fflush( nullptr );
    std::_Exit( 3 );
}
#endif

// A fault, abort or assert dialog waits for a click no test runner gives, so the suite hangs instead of failing
[[maybe_unused]] const bool g_failFast = []()
{
    SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX );

#if defined( _MSC_VER )
    _set_error_mode( _OUT_TO_STDERR );
    _set_abort_behavior( 0, _CALL_REPORTFAULT );
#endif

#if defined( _MSC_VER ) && defined( _DEBUG )
    _CrtSetReportHook2( _CRT_RPTHOOK_INSTALL, failOnCrtReport );
#endif

    return true;
}();
} // namespace

#endif

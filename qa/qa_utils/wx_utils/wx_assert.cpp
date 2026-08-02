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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/wx_assert.h>

#include <cstdlib>
#include <iostream>
#include <sstream>

#if defined( __APPLE__ ) || defined( __FreeBSD__ )
#define BOOST_STACKTRACE_GNU_SOURCE_NOT_REQUIRED
#endif
#ifdef __linux__
#define BOOST_STACKTRACE_USE_ADDR2LINE
#endif
#include <boost/stacktrace.hpp>

namespace KI_TEST
{
WX_ASSERT_ERROR::WX_ASSERT_ERROR( const wxString& aFile, int aLine, const wxString& aFunc,
        const wxString& aCond, const wxString& aMsg )
        : m_file( aFile ), m_line( aLine ), m_func( aFunc ), m_cond( aCond ), m_msg( aMsg )
{
    std::ostringstream ss;

    ss << "WX assertion in " << m_file << ":" << m_line << "\n"
       << "in function " << m_func << "\n"
       << "failed condition: " << m_cond;

    if( m_msg.size() )
        ss << "\n"
           << "with message: " << m_msg;

    m_format_msg = ss.str();
}

const char* WX_ASSERT_ERROR::what() const noexcept
{
    return m_format_msg.c_str();
}


void ReportAssertOffMainThread( const wxString& aFile, int aLine, const wxString& aFunc,
                                const wxString& aCond, const wxString& aMsg )
{
    WX_ASSERT_ERROR error( aFile, aLine, aFunc, aCond, aMsg );

    std::cerr << std::endl
              << "Assertion on a worker thread: " << error.what() << std::endl
              << std::endl
              << "Stack trace:" << std::endl
              << boost::stacktrace::stacktrace() << std::endl;
    std::cerr.flush();

    // _Exit rather than abort so that Boost.Test's SIGABRT handler cannot long jump out of
    // this thread and into the main thread's test-runner context
    std::_Exit( EXIT_FAILURE );
}

} // namespace KI_TEST

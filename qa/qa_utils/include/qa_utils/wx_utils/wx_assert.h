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

#ifndef UNIT_TEST_UTILS_WX_ASSERT__H
#define UNIT_TEST_UTILS_WX_ASSERT__H

#include <wx/string.h>

#include <exception>
#include <string>

namespace KI_TEST
{

/**
 * An exception class to represent a WX assertion.
 *
 * In normal programs, this is popped as a dialog, but in unit tests, it
 * prints a fairly unhelpful stack trace and otherwise doesn't inform the
 * test runner.
 *
 * We want to raise a formal exception to allow us to catch it with
 * things like BOOST_CHECK_THROW if expected, or for the test case to fail if
 * not expected.
 */
class WX_ASSERT_ERROR : public std::exception
{
public:
    WX_ASSERT_ERROR( const wxString& aFile, int aLine, const wxString& aFunc, const wxString& aCond,
            const wxString& aMsg );

    const char* what() const noexcept override;

    // Public, so catchers can have a look (though be careful as the function
    // names can change over time!)
    std::string m_file;
    int         m_line;
    std::string m_func;
    std::string m_cond;
    std::string m_msg;

    std::string m_format_msg;
};


/*
 * Simple function to handle a WX assertion and throw a real exception.
 *
 * This is useful when you want to check assertions fire in unit tests.
 */
inline void wxAssertThrower( const wxString& aFile, int aLine, const wxString& aFunc,
                             const wxString& aCond, const wxString& aMsg )
{
    throw KI_TEST::WX_ASSERT_ERROR( aFile, aLine, aFunc, aCond, aMsg );
}

} // namespace KI_TEST

#endif // UNIT_TEST_UTILS_WX_ASSERT__H
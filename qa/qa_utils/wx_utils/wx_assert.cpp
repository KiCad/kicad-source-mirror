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

#include <qa_utils/wx_utils/wx_assert.h>

#include <sstream>

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

} // namespace KI_TEST
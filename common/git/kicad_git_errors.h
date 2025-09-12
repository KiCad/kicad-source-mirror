/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_GIT_ERRORS_H
#define KICAD_GIT_ERRORS_H

#include <vector>
#include <import_export.h>

#include <wx/translation.h>

class APIEXPORT KIGIT_ERRORS
{
public:

    KIGIT_ERRORS() = default;
    virtual ~KIGIT_ERRORS() = default;

    const std::vector<wxString>& GetErrorStrings() const
    {
        return m_errorStrings;
    }

    const wxString& PeekErrorString() const
    {
        if( m_errorStrings.empty() )
            return _( "No error" );
        else
            return m_errorStrings.back();
    }

    wxString GetErrorString()
    {
        if( m_errorStrings.empty() )
            return _( "No error" );

        const wxString errorString( m_errorStrings.back() );
        m_errorStrings.pop_back();
        return errorString;
    }

    void AddErrorString( const wxString aErrorString )
    {
        m_errorStrings.emplace_back( aErrorString );
    }

    void AddErrorString( const std::string aErrorString )
    {
        m_errorStrings.emplace_back( aErrorString );
    }

    void ClearErrorStrings()
    {
        m_errorStrings.clear();
    }

private:

    std::vector<wxString> m_errorStrings;

};

#endif // KICAD_GIT_ERRORS_H

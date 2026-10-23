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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_GIT_ERRORS_H
#define KICAD_GIT_ERRORS_H

#include <vector>
#include <import_export.h>

#include <wx/translation.h>


/// A summary the user can act on, with the raw text that explains it kept apart for the details.
struct KIGIT_ERROR_MESSAGE
{
    wxString summary;
    wxString details;
};


class APIEXPORT KIGIT_ERRORS
{
public:

    KIGIT_ERRORS() = default;
    virtual ~KIGIT_ERRORS() = default;

    const std::vector<wxString>& GetErrorStrings() const
    {
        return m_errorStrings;
    }

    wxString GetErrorString()
    {
        if( m_errorStrings.empty() )
            return _( "No error" );

        const wxString errorString( m_errorStrings.back() );
        m_errorStrings.pop_back();
        m_errorDetails.pop_back();
        return errorString;
    }

    /// Take the newest error with its details. Both are empty when none is queued.
    KIGIT_ERROR_MESSAGE PopError()
    {
        if( m_errorStrings.empty() )
            return {};

        KIGIT_ERROR_MESSAGE message{ m_errorStrings.back(), m_errorDetails.back() };
        m_errorStrings.pop_back();
        m_errorDetails.pop_back();
        return message;
    }

    /// A summary the user can act on, with the raw text that explains it kept apart for the details.
    /// Both are shown, so any credentials in a URL inside either one are removed here.
    void AddError( const wxString& aSummary, const wxString& aDetails );

    void AddError( const KIGIT_ERROR_MESSAGE& aMessage ) { AddError( aMessage.summary, aMessage.details ); }

    /// Queue the summary with libgit2's last error as its details.
    void AddGitError( const wxString& aSummary );

    void AddErrorString( const wxString aErrorString )
    {
        AddError( aErrorString, wxEmptyString );
    }

    void AddErrorString( const std::string aErrorString )
    {
        AddError( wxString( aErrorString ), wxEmptyString );
    }

private:

    std::vector<wxString> m_errorStrings;
    std::vector<wxString> m_errorDetails; ///< One per entry in m_errorStrings

};

#endif // KICAD_GIT_ERRORS_H

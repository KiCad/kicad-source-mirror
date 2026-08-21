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

#ifndef KI_ERROR_H_
#define KI_ERROR_H_

#include <chrono>
#include <fmt/format.h>

#include <kicommon.h>
#include <wx/string.h>
#include <widgets/report_severity.h>


/**
 * Holds a structured error message.  Used to report errors in situations where you want
 * to split up the title from a detailed description.  Could also be used for an error log
 * in the future if we want one.
 */
class KICOMMON_API KI_ERROR
{
public:
    KI_ERROR() :
            m_severity( RPT_SEVERITY_UNDEFINED ),
            m_timestamp( std::chrono::system_clock::now() )
    {}

    explicit KI_ERROR( SEVERITY aSeverity, const wxString& aTitle = wxEmptyString ) :
            m_title( aTitle ),
            m_severity( aSeverity ),
            m_timestamp( std::chrono::system_clock::now() )
    {}

    const wxString& GetTitle() const { return m_title; }
    const wxString& GetDescription() const { return m_description; }
    const wxString& GetExtraText() const { return m_debugText; }
    SEVERITY        GetSeverity() const { return m_severity; }

    std::chrono::system_clock::time_point GetTimestamp() const { return m_timestamp; }

    KI_ERROR& SetTitle( const wxString& aTitle )
    {
        m_title = aTitle;
        return *this;
    }

    KI_ERROR& SetDescription( const wxString& aDescription )
    {
        m_description = aDescription;
        return *this;
    }

    KI_ERROR& SetDebugText( const wxString& aDebugText )
    {
        m_debugText = aDebugText;
        return *this;
    }

    KI_ERROR& SetSeverity( SEVERITY aSeverity )
    {
        m_severity = aSeverity;
        return *this;
    }

    bool HasTitle() const { return !m_title.empty(); }
    bool HasDescription() const { return !m_description.empty(); }
    bool HasDebugText() const   { return !m_debugText.empty(); }

    wxString AsString() const;

private:
    wxString m_title;
    wxString m_description;
    wxString m_debugText;
    SEVERITY m_severity;

    std::chrono::system_clock::time_point m_timestamp;
};


// Provides {fmt} formatting
auto format_as( const KI_ERROR& aError );

#endif // KI_ERROR_H_

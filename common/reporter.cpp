/**
 * @file reporter.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see change_log.txt for contributors.
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

#include <macros.h>
#include <reporter.h>
#include <wx_html_report_panel.h>

REPORTER& REPORTER::Report( const char* aText, REPORTER::SEVERITY aSeverity )
{
    Report( FROM_UTF8( aText ) );
    return *this;
}


REPORTER& WX_TEXT_CTRL_REPORTER::Report( const wxString& aText, REPORTER::SEVERITY aSeverity )
{
    wxCHECK_MSG( m_textCtrl != NULL, *this,
                 wxT( "No wxTextCtrl object defined in WX_TEXT_CTRL_REPORTER." ) );

    m_textCtrl->AppendText( aText );
    return *this;
}

REPORTER& WX_STRING_REPORTER::Report( const wxString& aText, REPORTER::SEVERITY aSeverity )
{
    wxCHECK_MSG( m_string != NULL, *this,
                 wxT( "No wxString object defined in WX_STRING_REPORTER." ) );

    *m_string << aText;
    return *this;
}

REPORTER& WX_HTML_PANEL_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    wxCHECK_MSG( m_panel != NULL, *this,
                 wxT( "No WX_HTML_REPORT_PANEL object defined in WX_HTML_PANEL_REPORTER." ) );

    m_panel->Report( aText, aSeverity );
    return *this;
}

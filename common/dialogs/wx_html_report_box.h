/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see change_log.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
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

#ifndef WX_HTML_REPORT_BOX_H
#define WX_HTML_REPORT_BOX_H

#include <wx/wx.h>
#include <reporter.h>
#include <vector>
#include <wx/html/htmlwin.h>

/**
 * WX_HTML_REPORT_BOX
 *
 * A slimmed down version of WX_HTML_REPORT_BOX
 */
class WX_HTML_REPORT_BOX : public wxHtmlWindow, public REPORTER
{
public:
    WX_HTML_REPORT_BOX( wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL );

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override { return !m_messages.empty(); }

    void Flush();
    void Clear();

private:
    wxString addHeader( const wxString& aBody );
    wxString generateHtml( const wxString& aLine );

    void scrollToBottom();

    ///> copy of the report, stored for filtering
    std::vector<wxString> m_messages;
};

#endif //WX_HTML_REPORT_BOX_H

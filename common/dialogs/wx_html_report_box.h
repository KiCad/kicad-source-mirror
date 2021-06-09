/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <reporter.h>
#include <vector>
#include <wx/html/htmlwin.h>
#include <eda_units.h>

/**
 * A slimmed down version of #WX_HTML_REPORT_PANEL
 */
class WX_HTML_REPORT_BOX : public wxHtmlWindow, public REPORTER
{
public:
    WX_HTML_REPORT_BOX( wxWindow* parent, wxWindowID id = wxID_ANY,
                        const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL );

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override { return !m_messages.empty(); }

    void SetUnits( EDA_UNITS aUnits ) { m_units = aUnits; }
    EDA_UNITS GetUnits() const override { return m_units; }

    /**
     * In immediate mode, messages are flushed as they are added.
     * Required for progress-related reports, but can be very slow for larger reports.
     */
    void SetImmediateMode() { m_immediateMode = true; }

    /**
     * Build the HTML messages page.
     * Call it if the immediate mode is not activated to be able to display them
     */
    void Flush();

    /**
     * Delete the stored messages
     */
    void Clear();

private:
    wxString addHeader( const wxString& aBody );
    wxString generateHtml( const wxString& aLine );

    EDA_UNITS             m_units;

    // Indicates messages should be flushed as they are added.  Required for progress-related
    // reports, but can be very slow for larger reports.
    bool                  m_immediateMode;

    ///< copy of the report, stored for filtering
    std::vector<wxString> m_messages;
};

#endif //WX_HTML_REPORT_BOX_H

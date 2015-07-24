/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __WX_HTML_REPORT_PANEL_H__
#define __WX_HTML_REPORT_PANEL_H__

#include <wx/wx.h>
#include <reporter.h>
#include <vector>

#include "wx_html_report_panel_base.h"


/**
 * Class WX_HTML_REPORT_PANEL
 *
 * A widget for browsing a rich text error/status report. Used in numerous
 * dialogs in eeschema and pcbnew. Provides error filtering functionality
 * and saving report files.
 *
 * The messages are reported throuth a REPORTER object
 */
class WX_HTML_REPORT_PANEL : public WX_HTML_REPORT_PANEL_BASE
{
public:
    WX_HTML_REPORT_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL );
    ~WX_HTML_REPORT_PANEL();

    ///> returns the reporter object that reports to this panel
    REPORTER& Reporter();

    ///> reports a string directly.
    void Report( const wxString& aText, REPORTER::SEVERITY aSeverity );

    ///> clears the report panel
    void Clear();

    ///> sets the frame label
    void SetLabel( const wxString& aLabel );

    ///> Sets the lasy update. If this mode is on, messages are stored but the display
    ///> is not updated (Updating display can be very time consumming if there are many messages)
    ///> A call to Flush() will be needed after build the report
    void SetLazyUpdate( bool aLazyUpdate );

    ///> Forces updating the HTML page, after the report is built in lazy mode
    void Flush();

    ///> Set the visible severity filter.
    ///> if aSeverities < 0 the m_showAll option is set
    void SetVisibleSeverities( int aSeverities );

    ///> @return the visible severity filter.
    ///> If the m_showAll option is set, the mask is < 0
    int GetVisibleSeverities();

private:
    struct REPORT_LINE
    {
        REPORTER::SEVERITY severity;
        wxString message;
    };

    typedef std::vector<REPORT_LINE> REPORT_LINES;

    wxString addHeader( const wxString& aBody );
    wxString generateHtml( const REPORT_LINE& aLine );
    wxString generatePlainText( const REPORT_LINE& aLine );

    void refreshView();
    void scrollToBottom();
    void syncCheckboxes();

    void onCheckBoxShowAll( wxCommandEvent& event );
    void onCheckBoxShowWarnings( wxCommandEvent& event );
    void onCheckBoxShowErrors( wxCommandEvent& event );
    void onCheckBoxShowInfos( wxCommandEvent& event );
    void onCheckBoxShowActions( wxCommandEvent& event );

    void onBtnSaveToFile( wxCommandEvent& event );

    ///> copy of the report, stored for filtering
    REPORT_LINES m_report;

    ///> the reporter
    WX_HTML_PANEL_REPORTER m_reporter;

    ///> message severities to display (mask)
    int m_severities;

    ///> show all messages flag (overrides m_severities)
    bool m_showAll;

    wxString m_html;

    bool m_lazyUpdate;
};

#endif //__WX_HTML_REPORT_PANEL_H__

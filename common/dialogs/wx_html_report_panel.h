/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2015-2021 KiCad Developers, see change_log.txt for contributors.
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

#include <reporter.h>
#include <vector>

#include "wx_html_report_panel_base.h"

/**
 * A widget for browsing a rich text error/status report. Used in numerous
 * dialogs in eeschema and pcbnew. Provides error filtering functionality
 * and saving report files.
 *
 * The messages are reported through a REPORTER object
 */
class WX_HTML_REPORT_PANEL : public WX_HTML_REPORT_PANEL_BASE
{
public:
    WX_HTML_REPORT_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxSize( 500,300 ), long style = wxTAB_TRAVERSAL );
    ~WX_HTML_REPORT_PANEL();

    ///< Set the min size of the area which displays html messages:
    void MsgPanelSetMinSize( const wxSize& aMinSize );

    ///< returns the reporter object that reports to this panel
    REPORTER& Reporter();

    /**
     * Reports the string
     * @param aText string message to report
     * @param aSeverity string classification level bitfield
     * @param aLocation REPORTER::LOCATION enum for placement of message
     */
    void Report( const wxString& aText, SEVERITY aSeverity,
                 REPORTER::LOCATION aLocation = REPORTER::LOC_BODY );

    ///< clears the report panel
    void Clear();

    ///< return the number of messages matching the given severity mask.
    int Count( int severityMask );

    ///< sets the frame label
    void SetLabel( const wxString& aLabel ) override;

    ///< Sets the lazy update. If this mode is on, messages are stored but the display
    ///< is not updated (Updating display can be very time consuming if there are many messages)
    ///< A call to Flush() will be needed after build the report
    void SetLazyUpdate( bool aLazyUpdate );

    ///< Forces updating the HTML page, after the report is built in lazy mode
    ///< If aSort = true, the body messages will be ordered by severity
    void Flush( bool aSort = false );

    ///< Set the visible severity filter.
    ///< if aSeverities < 0 the m_showAll option is set
    void SetVisibleSeverities( int aSeverities );

    ///< @return the visible severity filter.
    ///< If the m_showAll option is set, the mask is < 0
    int GetVisibleSeverities() const;

    ///< @return the visible severity filter.
    ///< If the m_showAll option is set, the mask is < 0
    void SetShowSeverity( SEVERITY aSeverity, bool aValue );

    ///< Set the report full file name to the string
    void SetFileName( const wxString& aReportFileName );

    ///< @return reference to the current report fill file name string.
    wxString& GetFileName( void );


private:
    struct REPORT_LINE
    {
        SEVERITY severity;
        wxString message;
    };

    typedef std::vector<REPORT_LINE> REPORT_LINES;

    wxString addHeader( const wxString& aBody );
    wxString generateHtml( const REPORT_LINE& aLine );
    wxString generatePlainText( const REPORT_LINE& aLine );
    void updateBadges();

    void scrollToBottom();
    void syncCheckboxes();

    void onRightClick( wxMouseEvent& event ) override;
    void onMenuEvent( wxMenuEvent& event );
    void onCheckBoxShowAll( wxCommandEvent& event ) override;
    void onCheckBoxShowWarnings( wxCommandEvent& event ) override;
    void onCheckBoxShowErrors( wxCommandEvent& event ) override;
    void onCheckBoxShowInfos( wxCommandEvent& event ) override;
    void onCheckBoxShowActions( wxCommandEvent& event ) override;

    void onBtnSaveToFile( wxCommandEvent& event ) override;

private:
    WX_HTML_PANEL_REPORTER m_reporter;

    REPORT_LINES     m_report;          ///< copy of the report, stored for filtering
    REPORT_LINES     m_reportTail;      ///< Lines to print at the end, regardless of sorting
    REPORT_LINES     m_reportHead;      ///< ... and at the beginning, regardless of sorting

    int              m_severities;      ///< message severities to display (mask)
    bool             m_lazyUpdate;

    wxString         m_reportFileName;  ///< defaults to the not very useful /bin/report.txt
};

#endif //__WX_HTML_REPORT_PANEL_H__

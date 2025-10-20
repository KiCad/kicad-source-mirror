/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <reporter.h>
#include <vector>
#include "wx_html_report_panel_base.h"

class WX_HTML_REPORT_PANEL;

/**
 * A wrapper for reporting to a wx HTML window.
 */
class KICOMMON_API WX_HTML_PANEL_REPORTER : public REPORTER
{
public:
    WX_HTML_PANEL_REPORTER( WX_HTML_REPORT_PANEL* aPanel ) :
            REPORTER(),
            m_panel( aPanel )
    {}

    virtual ~WX_HTML_PANEL_REPORTER() {}

    REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
    REPORTER& ReportTail( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;
    REPORTER& ReportHead( const wxString& aText, SEVERITY aSeverity = RPT_SEVERITY_UNDEFINED ) override;

    bool HasMessage() const override;
    bool HasMessageOfSeverity( int aSeverityMask ) const override;

private:
    WX_HTML_REPORT_PANEL* m_panel;
};

/**
 * A widget for browsing a rich text error/status report.
 *
 * Used in numerous dialogs in Eeschema and Pcbnew. Provides error filtering functionality
 * and saving report files.  The messages are reported through a #REPORTER object
 */
class KICOMMON_API WX_HTML_REPORT_PANEL : public WX_HTML_REPORT_PANEL_BASE
{
public:
    WX_HTML_REPORT_PANEL( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxSize( 500, 300 ), long style = wxTAB_TRAVERSAL );

    ~WX_HTML_REPORT_PANEL();

    /// Set the min size of the area which displays html messages.
    void MsgPanelSetMinSize( const wxSize& aMinSize );

    /// Return the reporter object that reports to this panel.
    REPORTER& Reporter();

    /**
     * Report the string.
     *
     * @param aText string message to report.
     * @param aSeverity string classification level bitfield.
     * @param aLocation REPORTER::LOCATION enum for placement of message.
     */
    void Report( const wxString& aText, SEVERITY aSeverity, REPORTER::LOCATION aLocation = REPORTER::LOC_BODY );

    /// Clears the report panel.
    void Clear();

    /// Return the number of messages matching the given severity mask.
    int Count( int severityMask );

    /// Set the frame label.
    void SetLabel( const wxString& aLabel ) override;

    /// Set the lazy update. If this mode is on, messages are stored but the display
    /// is not updated (Updating display can be very time consuming if there are many messages)
    /// A call to Flush() will be needed after build the report
    void SetLazyUpdate( bool aLazyUpdate ) { m_lazyUpdate = aLazyUpdate; }

    /// Force updating the HTML page, after the report is built in lazy mode
    /// If aSort = true, the body messages will be ordered by severity
    void Flush( bool aSort = false );

    /// @return the visible severity filter.
    /// If the m_showAll option is set, the mask is < 0
    int GetVisibleSeverities() const;

    /// Set the report full file name to the string.
    void SetFileName( const wxString& aReportFileName ) { m_reportFileName = aReportFileName; }

    /// @return reference to the current report fill file name string.
    wxString& GetFileName( void ) { return m_reportFileName; }

private:
    struct REPORT_LINE
    {
        SEVERITY severity;
        wxString message;
    };

    typedef std::vector<REPORT_LINE> REPORT_LINES;

    wxString generateHtml( const REPORT_LINE& aLine );
    wxString generatePlainText( const REPORT_LINE& aLine );
    void updateBadges();

    void scrollToBottom();

    void onRightClick( wxMouseEvent& event ) override;
    void onMenuEvent( wxMenuEvent& event );
    void onCheckBox( wxCommandEvent& event ) override;

    void onBtnSaveToFile( wxCommandEvent& event ) override;

    void onThemeChanged( wxSysColourChangedEvent &aEvent );

private:
    WX_HTML_PANEL_REPORTER m_reporter;

    REPORT_LINES     m_report;          ///< copy of the report, stored for filtering
    REPORT_LINES     m_reportTail;      ///< Lines to print at the end, regardless of sorting
    REPORT_LINES     m_reportHead;      ///< ... and at the beginning, regardless of sorting

    bool             m_lazyUpdate;

    wxString         m_reportFileName;  ///< defaults to the not very useful /bin/report.txt
};

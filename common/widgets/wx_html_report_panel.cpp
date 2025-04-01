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

#include <algorithm>

#include "wx_html_report_panel.h"

#include <wildcards_and_files_ext.h>
#include <gal/color4d.h>
#include <wx/clipbrd.h>
#include <string_utils.h>
#include <wx/ffile.h>
#include <wx/log.h>
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/menu.h>
#include <wx/textctrl.h>
#include <kiplatform/ui.h>
#include <kiway_holder.h>
#include <project.h>

WX_HTML_REPORT_PANEL::WX_HTML_REPORT_PANEL( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                            const wxSize& size, long style ) :
        WX_HTML_REPORT_PANEL_BASE( parent, id, pos, size, style ),
        m_reporter( this ),
        m_severities( -1 ),
        m_lazyUpdate( false )
{
    syncCheckboxes();
    m_htmlView->SetFont( KIUI::GetInfoFont( m_htmlView ) );
    Flush();

    Connect( wxEVT_COMMAND_MENU_SELECTED, wxMenuEventHandler( WX_HTML_REPORT_PANEL::onMenuEvent ),
             nullptr, this );

    m_htmlView->Bind( wxEVT_SYS_COLOUR_CHANGED,
                      wxSysColourChangedEventHandler( WX_HTML_REPORT_PANEL::onThemeChanged ),
                      this );
}


WX_HTML_REPORT_PANEL::~WX_HTML_REPORT_PANEL()
{
}


void WX_HTML_REPORT_PANEL::onThemeChanged( wxSysColourChangedEvent &aEvent )
{
    Flush();

    aEvent.Skip();
}


void WX_HTML_REPORT_PANEL::MsgPanelSetMinSize( const wxSize& aMinSize )
{
    m_fgSizer->SetMinSize( aMinSize );
    GetSizer()->SetSizeHints( this );
}


REPORTER& WX_HTML_REPORT_PANEL::Reporter()
{
    return m_reporter;
}


void WX_HTML_REPORT_PANEL::Report( const wxString& aText, SEVERITY aSeverity,
                                   REPORTER::LOCATION aLocation )
{
    REPORT_LINE line;
    line.message = aText;
    line.severity = aSeverity;

    if( aLocation == REPORTER::LOC_HEAD )
        m_reportHead.push_back( line );
    else if( aLocation == REPORTER::LOC_TAIL )
        m_reportTail.push_back( line );
    else
        m_report.push_back( line );

    if( !m_lazyUpdate )
    {
        m_htmlView->AppendToPage( generateHtml( line ) );
        scrollToBottom();
    }
}


void WX_HTML_REPORT_PANEL::SetLazyUpdate( bool aLazyUpdate )
{
    m_lazyUpdate = aLazyUpdate;
}


void WX_HTML_REPORT_PANEL::Flush( bool aSort )
{
    wxString html;

    if( aSort )
    {
        std::sort( m_report.begin(), m_report.end(),
                []( const REPORT_LINE& a, const REPORT_LINE& b)
                {
                    return a.severity < b.severity;
                });
    }

    for( const auto& line : m_reportHead )
        html += generateHtml( line );

    for( const auto& line : m_report )
        html += generateHtml( line );

    for( const auto& line : m_reportTail )
        html += generateHtml( line );

    m_htmlView->SetPage( html );
    scrollToBottom();
}


void WX_HTML_REPORT_PANEL::scrollToBottom()
{
    int x, y, xUnit, yUnit;

    m_htmlView->GetVirtualSize( &x, &y );
    m_htmlView->GetScrollPixelsPerUnit( &xUnit, &yUnit );
    m_htmlView->Scroll( 0, y / yUnit );

    updateBadges();
}


void WX_HTML_REPORT_PANEL::updateBadges()
{
    int count = Count(RPT_SEVERITY_ERROR );
    m_errorsBadge->UpdateNumber( count, RPT_SEVERITY_ERROR );

    count = Count(RPT_SEVERITY_WARNING );
    m_warningsBadge->UpdateNumber( count, RPT_SEVERITY_WARNING );
}


int WX_HTML_REPORT_PANEL::Count( int severityMask )
{
    int count = 0;

    for( const auto& reportLineArray : { m_report, m_reportHead, m_reportTail } )
    {
        for( const REPORT_LINE& reportLine : reportLineArray )
        {
            if( severityMask & reportLine.severity )
                count++;
        }
    }

    return count;
}


wxString WX_HTML_REPORT_PANEL::generateHtml( const REPORT_LINE& aLine )
{
    wxString retv;

    if( !( m_severities & aLine.severity ) )
        return retv;

    if( KIPLATFORM::UI::IsDarkTheme() )
    {
        switch( aLine.severity )
        {
        case RPT_SEVERITY_ERROR:
            retv = wxS( "<font color=#F04040 size=3>" ) + _( "Error:" ) + wxS( " </font>" )
                   + wxS( "<font size=3>" ) + aLine.message + wxS( "</font><br>" );
            break;
        case RPT_SEVERITY_WARNING:
            retv = wxS( "<font size=3>" ) + _( "Warning:" ) + wxS( " " ) + aLine.message
                   + wxS( "</font><br>" );
            break;
        case RPT_SEVERITY_INFO:
            retv = wxS( "<font color=#909090 size=3>" ) + aLine.message + wxS( "</font><br>" );
            break;
        case RPT_SEVERITY_ACTION:
            retv = wxS( "<font color=#60D060 size=3>" ) + aLine.message + wxS( "</font><br>" );
            break;
        default:
            retv = wxS( "<font size=3>" ) + aLine.message + wxS( "</font><br>" );
        }
    }
    else
    {
        switch( aLine.severity )
        {
        case RPT_SEVERITY_ERROR:
            retv = wxS( "<font color=#D00000 size=3>" ) + _( "Error:" ) + wxS( " </font>" )
                   + wxS( "<font size=3>" ) + aLine.message + wxS( "</font><br>" );
            break;
        case RPT_SEVERITY_WARNING:
            retv = wxS( "<font size=3>" ) + _( "Warning:" ) + wxS( " " ) + aLine.message
                   + wxS( "</font><br>" );
            break;
        case RPT_SEVERITY_INFO:
            retv = wxS( "<font color=#808080 size=3>" ) + aLine.message + wxS(  "</font><br>" );
            break;
        case RPT_SEVERITY_ACTION:
            retv = wxS( "<font color=#008000 size=3>" ) + aLine.message + wxS( "</font><br>" );
            break;
        default:
            retv = wxS( "<font size=3>" ) + aLine.message + wxS( "</font><br>" );
        }
    }

    // wxHtmlWindow fails to do correct baseline alignment between Japanese/Chinese cells and
    // Roman cells.  This keeps the line in a single cell.
    retv.Replace( wxS( " " ), wxS( "&nbsp;" ) );

    return retv;
}


wxString WX_HTML_REPORT_PANEL::generatePlainText( const REPORT_LINE& aLine )
{
    switch( aLine.severity )
    {
    case RPT_SEVERITY_ERROR:   return _( "Error:" ) + wxS( " " ) + aLine.message + wxT( "\n" );
    case RPT_SEVERITY_WARNING: return _( "Warning:" ) + wxS( " " ) + aLine.message + wxT( "\n" );
    case RPT_SEVERITY_INFO:    return _( "Info:" ) + wxS( " " ) + aLine.message + wxT( "\n" );
    default:                   return aLine.message + wxT( "\n" );
    }
}


void WX_HTML_REPORT_PANEL::onRightClick( wxMouseEvent& event )
{
    wxMenu popup;
    popup.Append( wxID_COPY, "Copy" );
    PopupMenu( &popup );
}


void WX_HTML_REPORT_PANEL::onMenuEvent( wxMenuEvent& event )
{
    if( event.GetId() == wxID_COPY )
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        if( wxTheClipboard->Open() )
        {
            bool primarySelection = wxTheClipboard->IsUsingPrimarySelection();
            wxTheClipboard->UsePrimarySelection( false );   // required to use the main clipboard
            wxTheClipboard->SetData( new wxTextDataObject( m_htmlView->SelectionToText() ) );
            wxTheClipboard->Flush(); // Allow data to be available after closing KiCad
            wxTheClipboard->Close();
            wxTheClipboard->UsePrimarySelection( primarySelection );
        }
    }
}


// Don't globally define this; different facilities use different definitions of "ALL"
static int RPT_SEVERITY_ALL = RPT_SEVERITY_WARNING | RPT_SEVERITY_ERROR | RPT_SEVERITY_INFO |
                              RPT_SEVERITY_ACTION;


void WX_HTML_REPORT_PANEL::onCheckBoxShowAll( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities = RPT_SEVERITY_ALL;
    else
        m_severities = RPT_SEVERITY_ERROR;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::syncCheckboxes()
{
    m_checkBoxShowAll->SetValue( m_severities == RPT_SEVERITY_ALL );
    m_checkBoxShowWarnings->SetValue( m_severities & RPT_SEVERITY_WARNING );
    m_checkBoxShowErrors->SetValue( m_severities & RPT_SEVERITY_ERROR );
    m_checkBoxShowInfos->SetValue( m_severities & RPT_SEVERITY_INFO );
    m_checkBoxShowActions->SetValue( m_severities & RPT_SEVERITY_ACTION );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowWarnings( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= RPT_SEVERITY_WARNING;
    else
        m_severities &= ~RPT_SEVERITY_WARNING;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowErrors( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= RPT_SEVERITY_ERROR;
    else
        m_severities &= ~RPT_SEVERITY_ERROR;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowInfos( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= RPT_SEVERITY_INFO;
    else
        m_severities &= ~RPT_SEVERITY_INFO;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowActions( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= RPT_SEVERITY_ACTION;
    else
        m_severities &= ~RPT_SEVERITY_ACTION;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onBtnSaveToFile( wxCommandEvent& event )
{
    wxFileName fn;

    if( m_reportFileName.empty() )
    {
        fn = wxT( "report.txt" );

        if( KIWAY_HOLDER* parent = dynamic_cast<KIWAY_HOLDER*>( m_parent ) )
            fn.SetPath( parent->Prj().GetProjectPath() );
    }
    else
    {
        fn = m_reportFileName;
    }

    wxWindow* topLevelParent = wxGetTopLevelParent( this );

    wxFileDialog dlg( topLevelParent, _( "Save Report File" ), fn.GetPath(), fn.GetFullName(),
                      FILEEXT::TextFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( "txt" );

    wxFFile f( fn.GetFullPath(), "wb" );

    if( !f.IsOpened() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write report to file '%s'." ),
                    fn.GetFullPath().GetData() );
        wxMessageBox( msg, _( "File save error" ), wxOK | wxICON_ERROR,
                      wxGetTopLevelParent( this ) );
        return;
    }

    for( const REPORT_LINES& section : { m_reportHead, m_report, m_reportTail } )
    {
        for( const REPORT_LINE& l : section )
        {
            wxString s = generatePlainText( l );

            ConvertSmartQuotesAndDashes( &s );
            f.Write( s );
        }
    }

    m_reportFileName = fn.GetFullPath();
    f.Close();
}


void WX_HTML_REPORT_PANEL::Clear()
{
    m_report.clear();
    m_reportHead.clear();
    m_reportTail.clear();
}


void WX_HTML_REPORT_PANEL::SetLabel( const wxString& aLabel )
{
    m_box->GetStaticBox()->SetLabel( aLabel );
}


void WX_HTML_REPORT_PANEL::SetVisibleSeverities( int aSeverities )
{
    if( aSeverities < 0 )
        m_severities = RPT_SEVERITY_ALL;
    else
        m_severities = aSeverities;

    syncCheckboxes();
}


int WX_HTML_REPORT_PANEL::GetVisibleSeverities() const
{
    return m_severities;
}


void WX_HTML_REPORT_PANEL::SetFileName( const wxString& aReportFileName )
{
    m_reportFileName = aReportFileName;
}


wxString& WX_HTML_REPORT_PANEL::GetFileName( void )
{
    return ( m_reportFileName );
}


void WX_HTML_REPORT_PANEL::SetShowSeverity( SEVERITY aSeverity, bool aValue )
{
    switch( aSeverity )
    {
    case RPT_SEVERITY_INFO:    m_checkBoxShowInfos->SetValue( aValue );    break;
    case RPT_SEVERITY_ACTION:  m_checkBoxShowActions->SetValue( aValue );  break;
    case RPT_SEVERITY_WARNING: m_checkBoxShowWarnings->SetValue( aValue ); break;
    default:                   m_checkBoxShowErrors->SetValue( aValue );   break;
    }
}


REPORTER& WX_HTML_PANEL_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::Report( aText, aSeverity );

    wxCHECK_MSG( m_panel != nullptr, *this,
                 wxT( "No WX_HTML_REPORT_PANEL object defined in WX_HTML_PANEL_REPORTER." ) );

    m_panel->Report( aText, aSeverity );
    return *this;
}


REPORTER& WX_HTML_PANEL_REPORTER::ReportTail( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::ReportTail( aText, aSeverity );

    wxCHECK_MSG( m_panel != nullptr, *this,
                 wxT( "No WX_HTML_REPORT_PANEL object defined in WX_HTML_PANEL_REPORTER." ) );

    m_panel->Report( aText, aSeverity, LOC_TAIL );
    return *this;
}


REPORTER& WX_HTML_PANEL_REPORTER::ReportHead( const wxString& aText, SEVERITY aSeverity )
{
    REPORTER::ReportHead( aText, aSeverity );

    wxCHECK_MSG( m_panel != nullptr, *this,
                 wxT( "No WX_HTML_REPORT_PANEL object defined in WX_HTML_PANEL_REPORTER." ) );

    m_panel->Report( aText, aSeverity, LOC_HEAD );
    return *this;
}


bool WX_HTML_PANEL_REPORTER::HasMessage() const
{
    // Check just for errors and warnings for compatibility
    return HasMessageOfSeverity( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING );
}


bool WX_HTML_PANEL_REPORTER::HasMessageOfSeverity( int aSeverityMask ) const
{
    return m_panel->Count( aSeverityMask ) > 0;
}

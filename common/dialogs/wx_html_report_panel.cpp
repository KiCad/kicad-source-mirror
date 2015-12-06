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

#include "wx_html_report_panel.h"

#include <wildcards_and_files_ext.h>
#include <boost/foreach.hpp>

WX_HTML_REPORT_PANEL::WX_HTML_REPORT_PANEL( wxWindow*      parent,
                                            wxWindowID     id,
                                            const wxPoint& pos,
                                            const wxSize&  size,
                                            long           style ) :
    WX_HTML_REPORT_PANEL_BASE( parent, id, pos, size, style ),
    m_reporter( this ),
    m_severities( -1 ),
    m_showAll( true ),
    m_lazyUpdate( false )
{
    syncCheckboxes();
    m_htmlView->SetPage( addHeader( "" ) );
}


WX_HTML_REPORT_PANEL::~WX_HTML_REPORT_PANEL()
{
}


void WX_HTML_REPORT_PANEL::MsgPanelSetMinSize( const wxSize& aMinSize )
{
    m_htmlView->SetMinSize( aMinSize );
    GetSizer()->SetSizeHints( this );
}


REPORTER& WX_HTML_REPORT_PANEL::Reporter()
{
    return m_reporter;
}


void WX_HTML_REPORT_PANEL::Report( const wxString& aText, REPORTER::SEVERITY aSeverity )
{
    REPORT_LINE line;
    line.message = aText;
    line.severity = aSeverity;

    m_report.push_back( line );

    m_html += generateHtml( line );

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


void WX_HTML_REPORT_PANEL::Flush()
{
    m_htmlView->SetPage( addHeader( m_html ) );
    scrollToBottom();
}


void WX_HTML_REPORT_PANEL::scrollToBottom()
{
    int x, y, xUnit, yUnit;

    m_htmlView->GetVirtualSize( &x, &y );
    m_htmlView->GetScrollPixelsPerUnit( &xUnit, &yUnit );
    m_htmlView->Scroll( 0, y / yUnit );
}


void WX_HTML_REPORT_PANEL::refreshView()
{
    wxString html;

    BOOST_FOREACH( REPORT_LINE l, m_report )
    {
        html += generateHtml( l );
    }

    m_htmlView->SetPage( addHeader( html ) );
    scrollToBottom();
}


wxString WX_HTML_REPORT_PANEL::addHeader( const wxString& aBody )
{
    wxColour bgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour fgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
    wxString s = "<html><body bgcolor=\"" + bgcolor.GetAsString( wxC2S_HTML_SYNTAX ) +
                 "\" text=\"" + fgcolor.GetAsString( wxC2S_HTML_SYNTAX ) + "\">";
    s += aBody;
    s += "</body></html>";

    return s;
}


wxString WX_HTML_REPORT_PANEL::generateHtml( const REPORT_LINE& aLine )
{
    if( !m_showAll && ! ( m_severities & aLine.severity ) )
        return wxEmptyString;

    switch( aLine.severity )
    {
    case REPORTER::RPT_ERROR:
        return wxString( "<font color=\"red\" size=2>" ) + _( "<b>Error: </b></font><font size=2>" ) + aLine.message + wxString( "</font><br>" );
    case REPORTER::RPT_WARNING:
        return wxString( "<font color=\"orange\" size=2>" ) + _( "<b>Warning: </b></font><font size=2>" ) + aLine.message + wxString( "</font><br>" );
    case REPORTER::RPT_INFO:
        return wxString( "<font color=\"gray\" size=2>" ) + _( "<b>Info: </b>" ) + aLine.message + wxString( "</font><br>" );
    case REPORTER::RPT_ACTION:
        return wxString( "<font color=\"darkgreen\" size=2>" ) + aLine.message + wxString( "</font><br>" );
    default:
        return wxString( "<font size=2>" ) + aLine.message + wxString( "</font><br>" );
    }
}


wxString WX_HTML_REPORT_PANEL::generatePlainText( const REPORT_LINE& aLine )
{
    switch( aLine.severity )
    {
    case REPORTER::RPT_ERROR:
        return _( "Error: " ) + aLine.message + wxT( "\n" );
    case REPORTER::RPT_WARNING:
        return _( "Warning: " ) + aLine.message + wxT( "\n" );
    case REPORTER::RPT_INFO:
        return _( "Info: " ) + aLine.message + wxT( "\n" );
    default:
        return aLine.message + wxT( "\n" );
    }
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowAll( wxCommandEvent& event )
{
    if ( event.IsChecked() )
         m_showAll = true;
     else
         m_showAll = false;

    syncCheckboxes();
    refreshView();
}


void WX_HTML_REPORT_PANEL::syncCheckboxes()
{
    m_checkBoxShowAll->SetValue( m_showAll );
    m_checkBoxShowWarnings->Enable( !m_showAll );
    m_checkBoxShowWarnings->SetValue( m_severities & REPORTER::RPT_WARNING );
    m_checkBoxShowErrors->Enable( !m_showAll );
    m_checkBoxShowErrors->SetValue( m_severities & REPORTER::RPT_ERROR );
    m_checkBoxShowInfos->Enable( !m_showAll );
    m_checkBoxShowInfos->SetValue( m_severities & REPORTER::RPT_INFO );
    m_checkBoxShowActions->Enable( !m_showAll );
    m_checkBoxShowActions->SetValue( m_severities & REPORTER::RPT_ACTION );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowWarnings( wxCommandEvent& event )
{
    if ( event.IsChecked() )
        m_severities |= REPORTER::RPT_WARNING;
    else
        m_severities &= ~REPORTER::RPT_WARNING;

     refreshView();
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowErrors( wxCommandEvent& event )
{
    if ( event.IsChecked() )
         m_severities |= REPORTER::RPT_ERROR;
     else
         m_severities &= ~REPORTER::RPT_ERROR;

     refreshView();
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowInfos( wxCommandEvent& event )
{
    if ( event.IsChecked() )
         m_severities |= REPORTER::RPT_INFO;
     else
         m_severities &= ~REPORTER::RPT_INFO;

     refreshView();
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowActions( wxCommandEvent& event )
{
    if ( event.IsChecked() )
         m_severities |= REPORTER::RPT_ACTION;
     else
         m_severities &= ~REPORTER::RPT_ACTION;

     refreshView();
}


void WX_HTML_REPORT_PANEL::onBtnSaveToFile( wxCommandEvent& event )
{
    wxFileName fn( "./report.txt" );

    wxFileDialog dlg( this, _( "Save report to file" ), fn.GetPath(), fn.GetName(),
                      TextWildcard, wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( wxT( "txt" ) );

    wxFile f( fn.GetFullPath(), wxFile::write );

    if( !f.IsOpened() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write report to file '%s'." ),
                    fn.GetFullPath().GetData() );
        wxMessageBox( msg, _( "File save error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    BOOST_FOREACH( REPORT_LINE l, m_report )
    {
        f.Write( generatePlainText( l ) );
    }

    f.Close();
}


void WX_HTML_REPORT_PANEL::Clear()
{
    m_html.clear();
    m_report.clear();
}


void WX_HTML_REPORT_PANEL::SetLabel( const wxString& aLabel )
{
    m_box->GetStaticBox()->SetLabel( aLabel );
}


void WX_HTML_REPORT_PANEL::SetVisibleSeverities( int aSeverities )
{
    if( aSeverities < 0 )
        m_showAll = true;
    else
    {
        m_showAll = false;
        m_severities = aSeverities;
    }

    syncCheckboxes();
}


int WX_HTML_REPORT_PANEL::GetVisibleSeverities()
{
    return m_showAll ? m_severities | 0x80000000 : m_severities & ~0x80000000;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2015-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

WX_HTML_REPORT_PANEL::WX_HTML_REPORT_PANEL( wxWindow*      parent,
                                            wxWindowID     id,
                                            const wxPoint& pos,
                                            const wxSize&  size,
                                            long           style ) :
    WX_HTML_REPORT_PANEL_BASE( parent, id, pos, size, style ),
    m_reporter( this ),
    m_severities( -1 ),
    m_lazyUpdate( false )
{
    syncCheckboxes();
    m_htmlView->SetPage( addHeader( "" ) );

    Connect( wxEVT_COMMAND_MENU_SELECTED,
            wxMenuEventHandler( WX_HTML_REPORT_PANEL::onMenuEvent ), NULL, this );
}


WX_HTML_REPORT_PANEL::~WX_HTML_REPORT_PANEL()
{
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


void WX_HTML_REPORT_PANEL::Report( const wxString& aText, REPORTER::SEVERITY aSeverity,
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

    m_htmlView->SetPage( addHeader( html ) );
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


#define BADGE_SIZE       20
#define BADGE_FONT_SIZE  10

static wxBitmap makeBadge( REPORTER::SEVERITY aStyle, int aCount, wxWindow* aWindow )
{
    wxSize      size( BADGE_SIZE, BADGE_SIZE );
    wxBitmap    bitmap( size );
    wxBrush     brush;
    wxMemoryDC  badgeDC;
    wxColour    badgeColour;
    wxColour    textColour;
    int         fontSize = BADGE_FONT_SIZE;

    if( aCount > 99 )
        fontSize--;

    badgeDC.SelectObject( bitmap );

    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    // We're one level deep in staticBoxes; each level is darkened by 210
    brush.SetColour( aWindow->GetParent()->GetBackgroundColour().MakeDisabled( 210 ) );
    badgeDC.SetBackground( brush );
    badgeDC.Clear();

    if( aCount == 0 )
        return bitmap;

    switch( aStyle )
    {
    case REPORTER::RPT_ERROR:
        badgeColour = *wxRED;
        textColour = *wxWHITE;
        break;
    case REPORTER::RPT_WARNING:
        badgeColour = wxColour( 235, 120, 80 );    // Orange
        textColour = *wxWHITE;
        break;
    case REPORTER::RPT_ACTION:
        badgeColour = *wxGREEN;
        textColour = *wxWHITE;
        break;
    case REPORTER::RPT_INFO:
    default:
        badgeColour = *wxLIGHT_GREY;
        textColour = *wxBLACK;
        break;
    }

    brush.SetStyle( wxBRUSHSTYLE_SOLID );
    brush.SetColour( badgeColour );
    badgeDC.SetBrush( brush );
    badgeDC.SetPen( wxPen( badgeColour, 0 ) );
    badgeDC.DrawCircle( size.x / 2 - 1, size.y / 2, ( std::max( size.x, size.y ) / 2 ) - 1 );

    wxFont font( fontSize, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD );
    badgeDC.SetFont( font );

    wxString text = wxString::Format( wxT( "%d" ), aCount );
    wxSize textExtent = badgeDC.GetTextExtent( text );

    badgeDC.SetTextForeground( textColour );
    badgeDC.DrawText( text, size.x / 2 - textExtent.x / 2 - 1, size.y / 2 - textExtent.y / 2 );

    return bitmap;
}


void WX_HTML_REPORT_PANEL::updateBadges()
{
    int count = Count( REPORTER::RPT_ERROR );
    m_errorsBadge->SetBitmap( makeBadge( REPORTER::RPT_ERROR, count, m_errorsBadge ) );

    count = Count( REPORTER::RPT_WARNING );
    m_warningsBadge->SetBitmap( makeBadge( REPORTER::RPT_WARNING, count, m_warningsBadge ) );
}


wxString WX_HTML_REPORT_PANEL::addHeader( const wxString& aBody )
{
    wxColour bgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
    wxColour fgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );

    return wxString::Format( wxT( "<html><body bgcolor='%s' text='%s'>%s</body></html>" ),
                             bgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                             fgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                             aBody );
}


int WX_HTML_REPORT_PANEL::Count( int severityMask )
{
    int count = 0;

    for( const REPORT_LINE& reportLine : m_report )
        if( severityMask & reportLine.severity )
            count++;

    return count;
}


wxString WX_HTML_REPORT_PANEL::generateHtml( const REPORT_LINE& aLine )
{
    wxString retv;

    if( !( m_severities & aLine.severity ) )
        return retv;

    switch( aLine.severity )
    {
    case REPORTER::RPT_ERROR:
        retv = "<font color=\"red\" size=2><b>" + _( "Error: " ) + "</b></font><font size=2>" +
               aLine.message + "</font><br>";
        break;
    case REPORTER::RPT_WARNING:
        retv = "<font color=\"orange\" size=2><b>" + _( "Warning: " ) +
               "</b></font><font size=2>" + aLine.message + "</font><br>";
        break;
    case REPORTER::RPT_INFO:
        retv = "<font color=\"dark gray\" size=2><b>" + _( "Info: " ) + "</b>" + aLine.message +
               "</font><br>";
        break;
    case REPORTER::RPT_ACTION:
        retv = "<font color=\"dark green\" size=2>" + aLine.message + "</font><br>";
        break;
    default:
        retv = "<font size=2>" + aLine.message + "</font><br>";
    }

    return retv;
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
        if( wxTheClipboard->Open() )
        {
            bool primarySelection = wxTheClipboard->IsUsingPrimarySelection();
            wxTheClipboard->UsePrimarySelection( false );   // required to use the main clipboard
            wxTheClipboard->SetData( new wxTextDataObject( m_htmlView->SelectionToText() ) );
            wxTheClipboard->Close();
            wxTheClipboard->UsePrimarySelection( primarySelection );
        }
    }
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowAll( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities = REPORTER::RPT_ALL;
    else
        m_severities = 0;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::syncCheckboxes()
{
    m_checkBoxShowAll->SetValue( m_severities == REPORTER::RPT_ALL );
    m_checkBoxShowWarnings->SetValue( m_severities & REPORTER::RPT_WARNING );
    m_checkBoxShowErrors->SetValue( m_severities & REPORTER::RPT_ERROR );
    m_checkBoxShowInfos->SetValue( m_severities & REPORTER::RPT_INFO );
    m_checkBoxShowActions->SetValue( m_severities & REPORTER::RPT_ACTION );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowWarnings( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= REPORTER::RPT_WARNING;
    else
        m_severities &= ~REPORTER::RPT_WARNING;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowErrors( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= REPORTER::RPT_ERROR;
    else
        m_severities &= ~REPORTER::RPT_ERROR;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowInfos( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= REPORTER::RPT_INFO;
    else
        m_severities &= ~REPORTER::RPT_INFO;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onCheckBoxShowActions( wxCommandEvent& event )
{
    if( event.IsChecked() )
        m_severities |= REPORTER::RPT_ACTION;
    else
        m_severities &= ~REPORTER::RPT_ACTION;

    syncCheckboxes();
    Flush( true );
}


void WX_HTML_REPORT_PANEL::onBtnSaveToFile( wxCommandEvent& event )
{
    wxFileName fn( "./report.txt" );

    wxFileDialog dlg( this, _( "Save Report to File" ), fn.GetPath(), fn.GetFullName(),
                      TextFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() != wxID_OK )
        return;

    fn = dlg.GetPath();

    if( fn.GetExt().IsEmpty() )
        fn.SetExt( "txt" );

    wxFile f( fn.GetFullPath(), wxFile::write );

    if( !f.IsOpened() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write report to file \"%s\"." ),
                    fn.GetFullPath().GetData() );
        wxMessageBox( msg, _( "File save error" ), wxOK | wxICON_ERROR, this );
        return;
    }

    for( const REPORT_LINE& l : m_report )
    {
        f.Write( generatePlainText( l ) );
    }

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
        m_severities = REPORTER::RPT_ALL;
    else
        m_severities = aSeverities;

    syncCheckboxes();
}


int WX_HTML_REPORT_PANEL::GetVisibleSeverities()
{
    return m_severities;
}

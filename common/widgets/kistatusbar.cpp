/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mark Roszko <mark.roszko@gmail.com>
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/tokenzr.h>
#include <fmt/format.h>
#include <array>
#include <widgets/kistatusbar.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/bitmap_button.h>
#include <widgets/ui_common.h>
#include <pgm_base.h>
#include <background_jobs_monitor.h>
#include <notifications_manager.h>
#include <bitmaps.h>
#include <reporter.h>
#include <dialog_HTML_reporter_base.h>
#include <trace_helpers.h>
#include <wx/dcclient.h>


KISTATUSBAR::KISTATUSBAR( int aNumberFields, wxWindow* parent, wxWindowID id, STYLE_FLAGS aFlags ) :
        wxStatusBar( parent, id ),
        m_backgroundStopButton( nullptr ),
        m_notificationsButton( nullptr ),
        m_warningButton( nullptr ),
        m_normalFieldsCount( aNumberFields ),
        m_styleFlags( aFlags )
{
#ifdef __WXOSX__
    // we need +1 extra field on OSX to offset from the rounded corner on the right
    // OSX doesn't use resize grippers like the other platforms and the statusbar field
    // includes the rounded part
    int extraFields = 3;
#else
    int extraFields = 2;
#endif

    bool showNotification = ( m_styleFlags & NOTIFICATION_ICON );
    bool showCancel = ( m_styleFlags & CANCEL_BUTTON );
    bool showWarning = ( m_styleFlags & WARNING_ICON );

    if( showCancel )
        extraFields++;

    if( showWarning )
        extraFields++;

    if( showNotification )
        extraFields++;

    SetFieldsCount( aNumberFields + extraFields );

    int* widths = new int[aNumberFields + extraFields];

    for( int i = 0; i < aNumberFields; i++ )
        widths[i] = -1;

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_LABEL ) )
        widths[aNumberFields + *idx] = -1;  // background status text field (variable size)

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_GAUGE ) )
        widths[aNumberFields + *idx] = 75;      // background progress button

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_CANCEL ) )
        widths[aNumberFields + *idx] = 20;     // background stop button

    if( std::optional<int> idx = fieldIndex( FIELD::WARNING ) )
        widths[aNumberFields + *idx] = 20;  // warning button

    if( std::optional<int> idx = fieldIndex( FIELD::NOTIFICATION ) )
        widths[aNumberFields + *idx] = 20;  // notifications button

#ifdef __WXOSX__
    // offset from the right edge
    widths[aNumberFields + extraFields - 1] = 10;
#endif

    SetStatusWidths( aNumberFields + extraFields, widths );
    delete[] widths;

    int* styles = new int[aNumberFields + extraFields];

    for( int i = 0; i < aNumberFields + extraFields; i++ )
        styles[i] = wxSB_FLAT;

    SetStatusStyles( aNumberFields + extraFields, styles );
    delete[] styles;

    m_backgroundTxt = new wxStaticText( this, wxID_ANY, wxT( "" ) );

    m_backgroundProgressBar = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize,
                                           wxGA_HORIZONTAL | wxGA_SMOOTH );

    if( showCancel )
    {
        m_backgroundStopButton = new wxButton( this, wxID_ANY, "X", wxDefaultPosition,
                                               wxDefaultSize, wxBU_EXACTFIT );
    }

    if( showNotification )
    {
        m_notificationsButton = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                                   wxDefaultSize, wxBU_EXACTFIT );

        m_notificationsButton->SetPadding( 0 );
        m_notificationsButton->SetBitmap( KiBitmapBundle( BITMAPS::notifications ) );
        m_notificationsButton->SetShowBadge( true );
        m_notificationsButton->SetBitmapCentered( true );

        m_notificationsButton->Bind( wxEVT_BUTTON, &KISTATUSBAR::onNotificationsIconClick, this );
    }

    if( showWarning )
    {
        m_warningButton = new BITMAP_BUTTON( this, wxID_ANY, wxNullBitmap, wxDefaultPosition,
                                             wxDefaultSize, wxBU_EXACTFIT );

        m_warningButton->SetPadding( 0 );
        m_warningButton->SetBitmap( KiBitmapBundle( BITMAPS::small_warning ) );
        m_warningButton->SetBitmapCentered( true );
        m_warningButton->SetToolTip( _( "View load messages" ) );
        m_warningButton->Hide();

        m_warningButton->Bind( wxEVT_BUTTON, &KISTATUSBAR::onLoadWarningsIconClick, this );
    }

    Bind( wxEVT_SIZE, &KISTATUSBAR::onSize, this );
    m_backgroundProgressBar->Bind( wxEVT_LEFT_DOWN, &KISTATUSBAR::onBackgroundProgressClick, this );

    HideBackgroundProgressBar();
    Layout();
}


KISTATUSBAR::~KISTATUSBAR()
{
    if( m_notificationsButton )
        m_notificationsButton->Unbind( wxEVT_BUTTON, &KISTATUSBAR::onNotificationsIconClick, this );

    if( m_warningButton )
        m_warningButton->Unbind( wxEVT_BUTTON, &KISTATUSBAR::onLoadWarningsIconClick, this );

    Unbind( wxEVT_SIZE, &KISTATUSBAR::onSize, this );
    m_backgroundProgressBar->Unbind( wxEVT_LEFT_DOWN, &KISTATUSBAR::onBackgroundProgressClick,
                                     this );
}


void KISTATUSBAR::onNotificationsIconClick( wxCommandEvent& aEvent )
{
    wxCHECK( m_notificationsButton, /* void */ );
    wxPoint pos = m_notificationsButton->GetScreenPosition();

    wxRect r;
    if( std::optional<int> idx = fieldIndex( FIELD::NOTIFICATION ) )
    {
        GetFieldRect( m_normalFieldsCount + *idx, r );
        pos.x += r.GetWidth();
    }

    Pgm().GetNotificationsManager().ShowList( this, pos );
}


void KISTATUSBAR::onBackgroundProgressClick( wxMouseEvent& aEvent )
{
    wxPoint pos = m_backgroundProgressBar->GetScreenPosition();

    wxRect r;
    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_GAUGE ) )
    {
        GetFieldRect( m_normalFieldsCount + *idx, r );
        pos.x += r.GetWidth();
    }

    Pgm().GetBackgroundJobMonitor().ShowList( this, pos );
}


void KISTATUSBAR::onSize( wxSizeEvent& aEvent )
{
    constexpr int padding = 5;

    wxRect r;
    GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_LABEL ), r );
    int x = r.GetLeft();
    int y = r.GetTop();
    int textHeight = KIUI::GetTextSize( wxT( "bp" ), this ).y;

    if( r.GetHeight() > textHeight )
        y += ( r.GetHeight() - textHeight ) / 2;

    m_backgroundTxt->SetPosition( { x, y } );

    GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_GAUGE ), r );
    x = r.GetLeft();
    y = r.GetTop();
    int           w = r.GetWidth();
    int           h = r.GetHeight();
    wxSize buttonSize( 0, 0 );

    if( m_backgroundStopButton )
    {
        buttonSize = m_backgroundStopButton->GetEffectiveMinSize();
        m_backgroundStopButton->SetPosition( { x + w - buttonSize.GetWidth(), y } );
        m_backgroundStopButton->SetSize( buttonSize.GetWidth(), h );
        buttonSize.x += padding;
    }

    m_backgroundProgressBar->SetPosition( { x + padding, y } );
    m_backgroundProgressBar->SetSize( w - buttonSize.GetWidth() - padding, h );

    if( m_notificationsButton )
    {
        GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::NOTIFICATION ), r );
        x = r.GetLeft();
        y = r.GetTop();
        h = r.GetHeight();
        buttonSize = m_notificationsButton->GetEffectiveMinSize();
        m_notificationsButton->SetPosition( { x, y } );
        m_notificationsButton->SetSize( buttonSize.GetWidth() + 6, h );
    }

    if( m_warningButton )
    {
        GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::WARNING ), r );
        x = r.GetLeft();
        y = r.GetTop();
        h = r.GetHeight();
        buttonSize = m_warningButton->GetEffectiveMinSize();
        m_warningButton->SetPosition( { x, y } );
        m_warningButton->SetSize( buttonSize.GetWidth() + 6, h );
    }
}


void KISTATUSBAR::ShowBackgroundProgressBar( bool aCancellable )
{
    m_backgroundProgressBar->Show();

    if( m_backgroundStopButton )
        m_backgroundStopButton->Show( aCancellable );
}


void KISTATUSBAR::HideBackgroundProgressBar()
{
    m_backgroundProgressBar->Hide();

    if( m_backgroundStopButton )
        m_backgroundStopButton->Hide();
}


void KISTATUSBAR::SetBackgroundProgress( int aAmount )
{
    int value = m_backgroundProgressBar->GetRange();

    if( value <= aAmount )
        value = aAmount;

    m_backgroundProgressBar->SetValue( value );
}


void KISTATUSBAR::SetBackgroundProgressMax( int aAmount )
{
    m_backgroundProgressBar->SetRange( aAmount );
}


void KISTATUSBAR::SetBackgroundStatusText( const wxString& aTxt )
{
    m_backgroundTxt->SetLabel( aTxt );
}


void KISTATUSBAR::SetNotificationCount( int aCount )
{
    wxCHECK( m_notificationsButton, /* void */ );
    wxString cnt = "";

    if( aCount > 0 )
        cnt = fmt::format( "{}", aCount );

    m_notificationsButton->SetBadgeText( cnt );

    // force a repaint or it wont until it gets activity
    Refresh();
}


void KISTATUSBAR::SetLoadWarningMessages( const wxString& aMessages )
{
    {
        std::lock_guard<std::mutex> lock( m_loadWarningMutex );
        m_loadWarningMessages.clear();

        wxStringTokenizer tokenizer( aMessages, wxS( "\n" ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            LOAD_MESSAGE msg;
            msg.message = tokenizer.GetNextToken();
            msg.severity = RPT_SEVERITY_WARNING;  // Default to warning for font substitutions
            m_loadWarningMessages.push_back( msg );
        }
    }

    updateWarningUI();
}


void KISTATUSBAR::AddLoadWarningMessages( const std::vector<LOAD_MESSAGE>& aMessages )
{
    wxLogTrace( traceLibraries, "KISTATUSBAR::AddLoadWarningMessages: this=%p, count=%zu",
                this, aMessages.size() );

    if( aMessages.empty() )
        return;

    {
        std::lock_guard<std::mutex> lock( m_loadWarningMutex );
        m_loadWarningMessages.insert( m_loadWarningMessages.end(), aMessages.begin(), aMessages.end() );
        wxLogTrace( traceLibraries, "  -> total messages now=%zu", m_loadWarningMessages.size() );
    }

    // Update UI on main thread
    wxLogTrace( traceLibraries, "  -> calling CallAfter for updateWarningUI" );
    CallAfter( [this]() { updateWarningUI(); } );
}


size_t KISTATUSBAR::GetLoadWarningCount() const
{
    std::lock_guard<std::mutex> lock( m_loadWarningMutex );
    return m_loadWarningMessages.size();
}


void KISTATUSBAR::updateWarningUI()
{
    wxLogTrace( traceLibraries, "KISTATUSBAR::updateWarningUI: this=%p, m_warningButton=%p",
                this, m_warningButton );

    if( !m_warningButton )
    {
        wxLogTrace( traceLibraries, "  -> no warning button, returning early" );
        return;
    }

    size_t messageCount;
    {
        std::lock_guard<std::mutex> lock( m_loadWarningMutex );
        messageCount = m_loadWarningMessages.size();
    }

    wxLogTrace( traceLibraries, "  -> message count=%zu, showing button=%s",
                messageCount, messageCount > 0 ? "true" : "false" );

    m_warningButton->Show( messageCount > 0 );

    if( messageCount > 0 )
    {
        m_warningButton->SetToolTip( wxString::Format( _( "View %zu load message(s)" ), messageCount ) );

        // Show count badge on the warning button
        m_warningButton->SetShowBadge( true );
        wxString badgeText = messageCount > 99
                ? wxString( "99+" )
                : wxString::Format( wxS( "%zu" ), messageCount );
        m_warningButton->SetBadgeText( badgeText );

        wxLogTrace( traceLibraries, "  -> badge set to '%s'", badgeText );
    }

    Layout();
    Refresh();
    wxLogTrace( traceLibraries, "  -> Layout and Refresh complete" );
}


void KISTATUSBAR::ClearLoadWarningMessages()
{
    {
        std::lock_guard<std::mutex> lock( m_loadWarningMutex );
        m_loadWarningMessages.clear();
    }

    if( m_warningButton )
    {
        m_warningButton->Hide();
        m_warningButton->SetShowBadge( false );
        m_warningButton->SetBadgeText( wxEmptyString );
        Layout();
        Refresh();
    }
}


void KISTATUSBAR::onLoadWarningsIconClick( wxCommandEvent& aEvent )
{
    // Copy messages under lock to avoid holding lock during modal dialog
    std::vector<LOAD_MESSAGE> messages;
    {
        std::lock_guard<std::mutex> lock( m_loadWarningMutex );
        messages = m_loadWarningMessages;
    }

    if( messages.empty() )
        return;

    DIALOG_HTML_REPORTER dlg( GetParent(), wxID_ANY, _( "Load Messages" ) );

    for( const LOAD_MESSAGE& msg : messages )
        dlg.m_Reporter->Report( msg.message, msg.severity );

    dlg.m_Reporter->Flush();
    dlg.ShowModal();
}

void KISTATUSBAR::SetEllipsedTextField( const wxString& aText, int aFieldId )
{
    wxRect       fieldRect;
    int          width = -1;
    wxString     etext = aText;

    // Only GetFieldRect() returns the current size for variable size fields
    // Other methods return -1 for the width of these fields.
    if( GetFieldRect( aFieldId, fieldRect ) )
        width = fieldRect.GetWidth();

    if( width > 20 )
    {
        wxClientDC dc( this );

        // Gives a margin to the text to be sure it is not clamped at its end
        int margin = KIUI::GetTextSize( wxT( "XX" ), this ).x;
        etext = wxControl::Ellipsize( etext, dc, wxELLIPSIZE_MIDDLE, width - margin );
    }

    SetStatusText( etext, aFieldId );
}


std::optional<int> KISTATUSBAR::fieldIndex( FIELD aField ) const
{
    switch( aField )
    {
    case FIELD::BGJOB_LABEL:  return 0;
    case FIELD::BGJOB_GAUGE:  return 1;
    case FIELD::BGJOB_CANCEL:
    {
        if( m_styleFlags & CANCEL_BUTTON )
            return 2;

        break;
    }
    case FIELD::WARNING:
    {
        if( m_styleFlags & WARNING_ICON )
        {
            int offset = 2;

            if( m_styleFlags & CANCEL_BUTTON )
                offset++;

            return offset;
        }

        break;
    }
    case FIELD::NOTIFICATION:
    {
        if( m_styleFlags & NOTIFICATION_ICON )
        {
            int offset = 2;

            if( m_styleFlags & CANCEL_BUTTON )
                offset++;

            if( m_styleFlags & WARNING_ICON )
                offset++;

            return offset;
        }

        break;
    }
    }

    return std::nullopt;
}

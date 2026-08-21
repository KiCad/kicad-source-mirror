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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <wx/button.h>
#include <wx/statusbr.h>
#include <wx/gauge.h>
#include <wx/stattext.h>
#include <wx/statbmp.h>
#include <wx/scrolwin.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/font.h>
#include <wx/artprov.h>
#include <wx/tokenzr.h>
#include <fmt/format.h>
#include <array>
#include <ranges>
#include <widgets/kistatusbar.h>
#include <widgets/bitmap_button.h>
#include <widgets/ui_common.h>
#include <widgets/wx_collapsible_pane.h>
#include <wx/frame.h>
#include <wx/time.h>
#include <kiplatform/ui.h>
#include <map>
#include <pgm_base.h>
#include <background_jobs_monitor.h>
#include <notifications_manager.h>
#include <bitmaps.h>
#include <reporter.h>
#include <trace_helpers.h>
#include <wx/dcclient.h>


class ERROR_CARD : public wxPanel
{
public:
    ERROR_CARD( wxWindow* aParent, const KI_ERROR& aError, int aWrapWidth ) :
            wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxSize( -1, -1 ), wxBORDER_NONE )
    {
        wxColour fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );
        SetBackgroundColour( bg );
        SetForegroundColour( fg );

        wxBoxSizer* outerSizer = new wxBoxSizer( wxHORIZONTAL );

        wxArtID artId = wxART_WARNING;

        switch( aError.GetSeverity() )
        {
        case RPT_SEVERITY_ERROR:   artId = wxART_ERROR;       break;
        case RPT_SEVERITY_INFO:    artId = wxART_INFORMATION; break;
        default:
            break;
        }

        wxStaticBitmap* icon = new wxStaticBitmap(
                this, wxID_ANY, wxArtProvider::GetBitmapBundle( artId, wxART_OTHER, FromDIP( wxSize( 16, 16 ) ) ) );
        icon->SetBackgroundColour( bg );
        outerSizer->Add( icon, 0, wxALL, 4 );

        wxBoxSizer* textSizer = new wxBoxSizer( wxVERTICAL );

        if( aError.HasTitle() )
        {
            wxStaticText* title = new wxStaticText( this, wxID_ANY, aError.GetTitle() );
            title->SetFont( KIUI::GetControlFont( this ).Bold() );

            if( aWrapWidth > 0 )
                title->Wrap( aWrapWidth );

            textSizer->Add( title, 0, wxALL | wxEXPAND, 1 );
        }

        if( aError.HasDescription() )
        {
            wxStaticText* desc = new wxStaticText( this, wxID_ANY, aError.GetDescription() );

            if( aWrapWidth > 0 )
                desc->Wrap( aWrapWidth );

            textSizer->Add( desc, 0, wxALL | wxEXPAND, 1 );
        }

        if( aError.HasDebugText() )
        {
            WX_COLLAPSIBLE_PANE* pane = new WX_COLLAPSIBLE_PANE( this, wxID_ANY,
                                                                 _( "Additional information" ) );
            pane->Collapse();
            pane->SetBackgroundColour( bg );
            textSizer->Add( pane, 0, wxEXPAND | wxALL, 1 );

            wxWindow* paneWin = pane->GetPane();
            paneWin->SetBackgroundColour( bg );
            const wxString& text = aError.GetDebugText();
            int paneHeight = GetCharHeight() * ( 2 + text.Freq( '\n' ) );

            wxTextCtrl* debugText = new wxTextCtrl( paneWin, wxID_ANY, text, wxDefaultPosition,
                                                    FromDIP( wxSize( -1, paneHeight ) ),
                                                    wxTE_MULTILINE | wxTE_READONLY | wxTE_DONTWRAP );

            wxFont monoFont = debugText->GetFont();
            monoFont.SetFamily( wxFONTFAMILY_TELETYPE );
            debugText->SetFont( monoFont );

            wxBoxSizer* paneSizer = new wxBoxSizer( wxVERTICAL );
            paneSizer->Add( debugText, 1, wxEXPAND | wxALL, 2 );
            paneWin->SetSizer( paneSizer );
            paneWin->Layout();

            pane->Bind( WX_COLLAPSIBLE_PANE_CHANGED,
                        [this]( wxCommandEvent& aEvt )
                        {
                            aEvt.Skip();

                            wxWindow* scrolled = GetParent();
                            wxWindow* frame = scrolled->GetParent();

                            scrolled->Layout();

                            if( wxSizer* sizer = scrolled->GetSizer() )
                                sizer->Fit( scrolled );

                            frame->Layout();
                            frame->Refresh();
                        } );
        }

        outerSizer->Add( textSizer, 1, wxEXPAND | wxTOP, 3 );

        SetSizer( outerSizer );
        Layout();
    }
};


static long long g_warning_list_closed_timer = 0;


class STATUSBAR_WARNING_LIST : public wxFrame
{
public:
    STATUSBAR_WARNING_LIST( KISTATUSBAR* aStatusBar, wxWindow* aParent ) :
            wxFrame( aParent, wxID_ANY, wxEmptyString, wxDefaultPosition, FromDIP( wxSize( 600, 200 ) ),
                     wxFRAME_NO_TASKBAR | wxBORDER_STATIC ),
            m_statusBar( aStatusBar )
    {
        SetSizeHints( FromDIP( wxSize( 600, 200 ) ), wxDefaultSize );

        wxColour fg, bg;
        KIPLATFORM::UI::GetInfoBarColours( fg, bg );
        SetBackgroundColour( bg );

        wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );

        m_scrolledWindow =
                new wxScrolledWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL | wxBORDER_SIMPLE );
        m_scrolledWindow->SetScrollRate( 5, 5 );
        m_scrolledWindow->SetBackgroundColour( bg );
        m_scrolledWindow->SetForegroundColour( fg );

        m_contentSizer = new wxBoxSizer( wxVERTICAL );
        m_scrolledWindow->SetSizer( m_contentSizer );
        m_scrolledWindow->Layout();

        mainSizer->Add( m_scrolledWindow, 1, wxEXPAND, 0 );

        wxBoxSizer* btnSizer = new wxBoxSizer( wxHORIZONTAL );
        btnSizer->AddStretchSpacer( 1 );
        wxButton* clearButton = new wxButton( this, wxID_CLEAR, _( "Clear Warnings" ) );
        clearButton->Bind( wxEVT_BUTTON, &STATUSBAR_WARNING_LIST::onClearButtonClick, this );
        btnSizer->Add( clearButton, 0, wxALL, 5 );
        mainSizer->Add( btnSizer, 0, wxEXPAND | wxTOP | wxRIGHT | wxLEFT | wxBOTTOM, 5 );

        SetSizer( mainSizer );
        Layout();

        Bind( wxEVT_KILL_FOCUS, &STATUSBAR_WARNING_LIST::onFocusLoss, this );
        m_scrolledWindow->Bind( wxEVT_KILL_FOCUS, &STATUSBAR_WARNING_LIST::onFocusLoss, this );
        Bind( wxEVT_CHAR_HOOK, &STATUSBAR_WARNING_LIST::onCharHook, this );
        Bind( wxEVT_CLOSE_WINDOW, &STATUSBAR_WARNING_LIST::onClose, this );

        rebuildMessages();
    }


    ~STATUSBAR_WARNING_LIST() override
    {
        Unbind( wxEVT_KILL_FOCUS, &STATUSBAR_WARNING_LIST::onFocusLoss, this );
        m_scrolledWindow->Unbind( wxEVT_KILL_FOCUS, &STATUSBAR_WARNING_LIST::onFocusLoss, this );
        Unbind( wxEVT_CHAR_HOOK, &STATUSBAR_WARNING_LIST::onCharHook, this );
        Unbind( wxEVT_CLOSE_WINDOW, &STATUSBAR_WARNING_LIST::onClose, this );
    }


    void rebuildMessages()
    {
        m_contentSizer->Clear( true );

        int wrapWidth = m_scrolledWindow->GetClientSize().x;

        if( wrapWidth < 100 )
            wrapWidth = FromDIP( 580 );
        else
            wrapWidth -= FromDIP( 40 );  // Card borders/margins + severity icon + padding

        auto messages = m_statusBar->GetWarningMessages();

        for( const auto& msgs : messages | std::views::values )
        {
            for( const KI_ERROR& msg : msgs )
            {
                ERROR_CARD* card = new ERROR_CARD( m_scrolledWindow, msg, wrapWidth );
                m_contentSizer->Add( card, 0, wxEXPAND | wxALL, 2 );
            }
        }

        m_scrolledWindow->Layout();
        m_contentSizer->Fit( m_scrolledWindow );
        Layout();
        Refresh();
    }


private:
    void onFocusLoss( wxFocusEvent& aEvent )
    {
        if( !IsDescendant( aEvent.GetWindow() ) )
        {
            Close( true );
            g_warning_list_closed_timer = wxGetLocalTimeMillis().GetValue();
        }

        aEvent.Skip();
    }


    void onCharHook( wxKeyEvent& aEvent )
    {
        if( aEvent.GetKeyCode() == WXK_ESCAPE )
        {
            Close( true );
            g_warning_list_closed_timer = wxGetLocalTimeMillis().GetValue();
            return;
        }

        aEvent.Skip();
    }


    void onClose( wxCloseEvent& aEvent )
    {
        if( m_statusBar )
            m_statusBar->CloseWarningList();

        aEvent.Skip();
    }


    void onClearButtonClick( wxCommandEvent& aEvent )
    {
        if( m_statusBar )
            m_statusBar->ClearWarningMessages();

        // ClearWarningMessages triggers updateWarningUI which will close the panel
    }

    KISTATUSBAR*      m_statusBar;
    wxScrolledWindow* m_scrolledWindow;
    wxBoxSizer*       m_contentSizer;
};


KISTATUSBAR::KISTATUSBAR( int aNumberFields, wxWindow* parent, wxWindowID id, STYLE_FLAGS aFlags ) :
        wxStatusBar( parent, id, wxSTB_SIZEGRIP | wxSTB_ELLIPSIZE_MIDDLE | wxSTB_SHOW_TIPS | wxFULL_REPAINT_ON_RESIZE ),
        m_backgroundStopButton( nullptr ),
        m_notificationsButton( nullptr ),
        m_warningButton( nullptr ),
        m_warningList( nullptr ),
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

    m_backgroundTxt = new wxStaticText( this, wxID_ANY, wxT( "" ), wxDefaultPosition,
                                        wxDefaultSize, wxALIGN_RIGHT | wxST_NO_AUTORESIZE );

    m_backgroundProgressBar = new wxGauge( this, wxID_ANY, 100, wxDefaultPosition, wxDefaultSize,
                                           wxGA_HORIZONTAL | wxGA_SMOOTH );

    if( showCancel )
    {
        m_backgroundStopButton = new wxButton( this, wxID_ANY, "X", wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
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

    m_fieldWidths.assign( aNumberFields + extraFields, -1 );

#ifdef __WXOSX__
    // offset from the right edge
    int padding = KIUI::GetTextSize( wxT( "M" ), this ).x;
    m_fieldWidths[aNumberFields + extraFields - 1] = padding;
#endif

    SetFieldsCount( m_fieldWidths.size(), m_fieldWidths.data() );

    std::vector<int> styles( aNumberFields + extraFields, wxSB_FLAT );
    SetStatusStyles( styles.size(), styles.data() );

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

    CloseWarningList();

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
    layoutControls();
}


void KISTATUSBAR::layoutControls()
{
    constexpr int padding = 5;

    wxRect r;
    int sbField = m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_LABEL );

    if( sbField >= 0 && sbField < GetFieldsCount() )
    {
        GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_LABEL ), r );
        int x = r.GetLeft();
        int y = r.GetTop();
        int textHeight = KIUI::GetTextSize( wxT( "bp" ), this ).y;

        if( r.GetHeight() > textHeight )
            y += ( r.GetHeight() - textHeight ) / 2;

        m_backgroundTxt->SetPosition( { x, y } );
        m_backgroundTxt->SetSize( r.GetWidth(), textHeight );
        updateBackgroundText();
    }

    sbField = m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_GAUGE );

    if( sbField >= 0 && sbField < GetFieldsCount() )
    {
        GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_GAUGE ), r );
        int x = r.GetLeft();
        int y = r.GetTop();
        int w = r.GetWidth();
        int h = r.GetHeight();
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
            sbField = m_normalFieldsCount + *fieldIndex( FIELD::NOTIFICATION );

            if( sbField >= 0 && sbField < GetFieldsCount() )
            {
                GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::NOTIFICATION ), r );
                x = r.GetLeft();
                y = r.GetTop();
                h = r.GetHeight();
                buttonSize = m_notificationsButton->GetEffectiveMinSize();
                m_notificationsButton->SetPosition( { x, y } );
                m_notificationsButton->SetSize( buttonSize.GetWidth() + 6, h );
            }
        }
    }

    if( m_warningButton )
    {
        sbField = m_normalFieldsCount + *fieldIndex( FIELD::WARNING );

        if( sbField >= 0 && sbField < GetFieldsCount() )
        {
            GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::WARNING ), r );
            int x = r.GetLeft();
            int y = r.GetTop();
            int h = r.GetHeight();
            wxSize buttonSize = m_warningButton->GetEffectiveMinSize();
            m_warningButton->SetPosition( { x, y } );
            m_warningButton->SetSize( buttonSize.GetWidth() + 6, h );
        }
    }
}


void KISTATUSBAR::ShowBackgroundProgressBar( bool aCancellable )
{
    m_backgroundProgressBar->Show();

    if( m_backgroundStopButton )
        m_backgroundStopButton->Show( aCancellable );

    updateAuxFieldWidths();
}


void KISTATUSBAR::HideBackgroundProgressBar()
{
    m_backgroundProgressBar->Hide();

    if( m_backgroundStopButton )
        m_backgroundStopButton->Hide();

    updateAuxFieldWidths();
}


void KISTATUSBAR::SetBackgroundProgress( int aAmount )
{
    int range = m_backgroundProgressBar->GetRange();

    if( aAmount > range )
        aAmount = range;

    m_backgroundProgressBar->SetValue( aAmount );
}


void KISTATUSBAR::SetBackgroundProgressMax( int aAmount )
{
    m_backgroundProgressBar->SetRange( aAmount );
}


void KISTATUSBAR::SetBackgroundStatusText( const wxString& aTxt )
{
    m_backgroundRawText = aTxt;
    updateBackgroundText();

    // When there are multiple normal fields, the last normal field (typically used for
    // file watcher status on Windows) can visually overlap with the background job label
    // since both have variable width. Save and clear that field when showing background
    // text, and restore it when the background text is cleared.
    if( m_normalFieldsCount > 1 )
    {
        int      adjacentField = m_normalFieldsCount - 1;
        wxString currentText = GetStatusText( adjacentField );

        if( !aTxt.empty() )
        {
            if( !currentText.empty() )
                m_savedStatusText = currentText;

            SetStatusText( wxEmptyString, adjacentField );
        }
        else if( !m_savedStatusText.empty() )
        {
            SetStatusText( m_savedStatusText, adjacentField );
            m_savedStatusText.clear();
        }
    }
}


void KISTATUSBAR::updateAuxFieldWidths()
{
    if( m_fieldWidths.empty() )
        return;

    int padding = KIUI::GetTextSize( wxT( "M" ), this ).x;

    // The background job label and gauge only carry content while a job is running. When idle
    // they must collapse to zero so they do not consume stretch reserved for the normal fields.
    bool jobActive = m_backgroundProgressBar && m_backgroundProgressBar->IsShown();

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_LABEL ) )
        m_fieldWidths[m_normalFieldsCount + *idx] = jobActive ? -1 : 0;

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_GAUGE ) )
        m_fieldWidths[m_normalFieldsCount + *idx] = jobActive ? 75 : 0;

    if( std::optional<int> idx = fieldIndex( FIELD::BGJOB_CANCEL ) )
    {
        if( m_backgroundStopButton && m_backgroundStopButton->IsShown() )
            m_fieldWidths[m_normalFieldsCount + *idx] = m_backgroundStopButton->GetSize().x + padding;
        else
            m_fieldWidths[m_normalFieldsCount + *idx] = 0;
    }

    if( std::optional<int> idx = fieldIndex( FIELD::WARNING ) )
    {
        if( m_warningButton && m_warningButton->IsShown() )
            m_fieldWidths[m_normalFieldsCount + *idx] = m_warningButton->GetSize().x + padding;
        else
            m_fieldWidths[m_normalFieldsCount + *idx] = 0;
    }

    if( std::optional<int> idx = fieldIndex( FIELD::NOTIFICATION ) )
    {
        if( m_notificationsButton && m_notificationsButton->IsShown() )
            m_fieldWidths[m_normalFieldsCount + *idx] = m_notificationsButton->GetSize().x + padding;
        else
            m_fieldWidths[m_normalFieldsCount + *idx] = 0;
    }

    SetStatusWidths( static_cast<int>( m_fieldWidths.size() ), m_fieldWidths.data() );
    layoutControls();
    updateBackgroundText();
}


void KISTATUSBAR::updateBackgroundText()
{
    wxRect r;

    if( !GetFieldRect( m_normalFieldsCount + *fieldIndex( FIELD::BGJOB_LABEL ), r ) )
        return;

    wxString text = m_backgroundRawText;

    if( !text.empty() && r.GetWidth() > 4 )
    {
        wxClientDC dc( this );
        int margin = KIUI::GetTextSize( wxT( "XX" ), this ).x;
        text = wxControl::Ellipsize( text, dc, wxELLIPSIZE_END, std::max( 0, r.GetWidth() - margin ) );
    }

    m_backgroundTxt->SetLabel( text );
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


void KISTATUSBAR::AddWarningMessages( const wxString& aSource, const wxString& aMessages )
{
    {
        std::lock_guard<std::mutex> lock( m_warningMutex );

        wxStringTokenizer tokenizer( aMessages, wxS( "\n" ), wxTOKEN_STRTOK );

        while( tokenizer.HasMoreTokens() )
        {
            KI_ERROR msg;
            msg.SetTitle( tokenizer.GetNextToken() );
            msg.SetSeverity( RPT_SEVERITY_WARNING );  // Default to warning for font substitutions
            m_warningMessages[aSource].push_back( msg );
        }
    }

    updateWarningUI();
}


void KISTATUSBAR::AddWarningMessages( const wxString& aSource, const std::vector<KI_ERROR>& aMessages )
{
    wxLogTrace( traceLibraries, "KISTATUSBAR::AddWarningMessages: this=%p, count=%zu",
                this, aMessages.size() );

    if( aMessages.empty() )
        return;

    size_t totalMessageCount = 0;

    {
        std::lock_guard<std::mutex> lock( m_warningMutex );
        m_warningMessages[aSource].insert( m_warningMessages[aSource].end(), aMessages.begin(), aMessages.end() );

        for( const auto& [source, messages] : m_warningMessages )
            totalMessageCount += messages.size();
    }

    wxLogTrace( traceLibraries, "  -> total messages now=%zu", totalMessageCount );

    // Update UI on main thread
    wxLogTrace( traceLibraries, "  -> calling CallAfter for updateWarningUI" );
    CallAfter( [this]() { updateWarningUI(); } );
}


size_t KISTATUSBAR::GetLoadWarningCount() const
{
    std::lock_guard<std::mutex> lock( m_warningMutex );

    size_t count = 0;

    for( const auto& [source, messages] : m_warningMessages )
        count += messages.size();

    return count;
}


std::map<wxString, std::vector<KI_ERROR>> KISTATUSBAR::GetWarningMessages() const
{
    std::lock_guard<std::mutex> lock( m_warningMutex );

    // TODO(JE) this is NG, we should use a vector not an unordered map
    // Copy into a std::map so sources are sorted by name for stable display order.
    return { m_warningMessages.begin(), m_warningMessages.end() };
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
        std::lock_guard<std::mutex> lock( m_warningMutex );

        messageCount = 0;

        for( const std::vector<KI_ERROR>& messages : m_warningMessages | std::views::values )
            messageCount += messages.size();
    }

    wxLogTrace( traceLibraries, "  -> message count=%zu, showing button=%s",
                messageCount, messageCount > 0 ? "true" : "false" );

    m_warningButton->Show( messageCount > 0 );
    m_warningButton->SetShowBadge( messageCount > 0 );
    updateAuxFieldWidths();

    if( messageCount > 0 )
    {
        m_warningButton->SetToolTip( wxString::Format( _( "View %zu message(s)" ), messageCount ) );

        // Show count badge on the warning button
        wxString badgeText = messageCount > 99
                ? wxString( "99+" )
                : wxString::Format( wxS( "%zu" ), messageCount );
        m_warningButton->SetBadgeText( badgeText );

        wxLogTrace( traceLibraries, "  -> badge set to '%s'", badgeText );
    }
    else
    {
        m_warningButton->SetBadgeText( wxEmptyString );
        m_warningButton->SetToolTip( _( "View messages" ) );
    }

    if( m_warningList )
    {
        if( messageCount > 0 )
            m_warningList->rebuildMessages();
        else
            CloseWarningList();
    }

    Layout();
    Refresh();
}


void KISTATUSBAR::ClearWarningMessages( const wxString& aSource )
{
    {
        std::lock_guard<std::mutex> lock( m_warningMutex );

        if( aSource.IsEmpty() )
            m_warningMessages.clear();
        else if( auto it = m_warningMessages.find( aSource ); it != m_warningMessages.end() )
                m_warningMessages.erase( it );
    }

    updateWarningUI();
}


void KISTATUSBAR::onLoadWarningsIconClick( wxCommandEvent& aEvent )
{
    // Debounce clicking on the icon with a list already showing
    if( wxGetLocalTimeMillis().GetValue() - g_warning_list_closed_timer < 300 )
    {
        g_warning_list_closed_timer = 0;
        return;
    }

    if( m_warningList )
    {
        CloseWarningList();
        return;
    }

    if( GetLoadWarningCount() == 0 )
        return;

    openWarningList();
}


void KISTATUSBAR::openWarningList()
{
    wxCHECK( m_warningButton, /* void */ );

    m_warningList = new STATUSBAR_WARNING_LIST( this, GetParent() );
    PositionWarningPanel();
    m_warningList->Show();
    KIPLATFORM::UI::ForceFocus( m_warningList );
}


void KISTATUSBAR::PositionWarningPanel()
{
    if( !m_warningList || !m_warningButton )
        return;

    wxRect iconRect = m_warningButton->GetScreenRect();

    wxSize windowSize = m_warningList->GetSize();
    wxPoint pos;
    pos.x = iconRect.GetRight() + 1 - windowSize.GetWidth();
    pos.y = iconRect.GetTop() - windowSize.GetHeight();

    m_warningList->SetPosition( pos );
    m_warningList->Layout();
}


void KISTATUSBAR::CloseWarningList()
{
    if( !m_warningList )
        return;

    m_warningList->Destroy();
    m_warningList = nullptr;
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


void KISTATUSBAR::SetStatusWidths( int aSize, const int* aWidths )
{
    wxStatusBar::SetStatusWidths( aSize, aWidths );

    for( int i = 0; ( i < aSize ) && ( i < static_cast<int>( m_fieldWidths.size() ) ); i++ )
        m_fieldWidths[i] = *( aWidths + i );
}

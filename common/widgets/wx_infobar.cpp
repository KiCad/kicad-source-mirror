/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <class_draw_panel_gal.h>
#include <id.h>
#include <kiplatform/ui.h>
#include <widgets/wx_infobar.h>
#include "wx/artprov.h"
#include <wx/bmpbuttn.h>
#include <wx/debug.h>
#include <wx/hyperlink.h>
#include <wx/infobar.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/weakref.h>
#include <wx/dcclient.h>

#include <algorithm>
#include <utility>

#ifdef __WXMSW__
#include <dpi_scaling_common.h>
#endif


wxDEFINE_EVENT( KIEVT_SHOW_INFOBAR,    wxCommandEvent );
wxDEFINE_EVENT( KIEVT_DISMISS_INFOBAR, wxCommandEvent );

BEGIN_EVENT_TABLE( WX_INFOBAR, wxInfoBarGeneric )
    EVT_COMMAND( wxID_ANY, KIEVT_SHOW_INFOBAR,    WX_INFOBAR::onShowInfoBar )
    EVT_COMMAND( wxID_ANY, KIEVT_DISMISS_INFOBAR, WX_INFOBAR::onDismissInfoBar )

    EVT_SYS_COLOUR_CHANGED( WX_INFOBAR::onThemeChange )
    EVT_BUTTON( ID_CLOSE_INFOBAR, WX_INFOBAR::onCloseButton )
    EVT_TIMER(  ID_CLOSE_INFOBAR, WX_INFOBAR::onTimer )
END_EVENT_TABLE()


static const wxString OVERLAY_HOST_NAME = wxS( "InfobarOverlayHost" );


// Overlay infobars on the same window share one host where the platform needs one
static wxWindow* overlayParent( wxWindow* aParent, bool aOverlay )
{
    if( !aOverlay )
        return aParent;

    for( wxWindow* child : aParent->GetChildren() )
    {
        if( child->GetName() == OVERLAY_HOST_NAME )
            return child;
    }

    if( wxWindow* host = KIPLATFORM::UI::CreateCanvasOverlay( aParent ) )
    {
        host->SetName( OVERLAY_HOST_NAME );
        return host;
    }

    return aParent;
}


WX_INFOBAR::WX_INFOBAR( wxWindow* aParent, wxWindowID aWinid, bool aOverlay )
        : wxInfoBarGeneric( overlayParent( aParent, aOverlay ), aWinid ),
          m_showTime( 0 ),
          m_updateLock( false ),
          m_overlay( aOverlay ),
          m_anchor( aParent ),
          m_showTimer( nullptr ),
          m_type( MESSAGE_TYPE::GENERIC ),
          m_buttonArea( nullptr ),
          m_messageText( nullptr )
{
    m_showTimer = new wxTimer( this, ID_CLOSE_INFOBAR );

    wxColour fg, bg;
    KIPLATFORM::UI::GetInfoBarColours( fg, bg );
    SetBackgroundColour( bg );
    SetForegroundColour( fg );

#ifdef __WXMAC__
    // Infobar is broken on Mac without the effects
    SetShowHideEffects( wxSHOW_EFFECT_ROLL_TO_BOTTOM, wxSHOW_EFFECT_ROLL_TO_TOP );
    SetEffectDuration( 200 );
#else
    // Infobar freezes canvas on Windows with the effect, and GTK looks bad with it
    SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );
#endif

    // The infobar seems to start too small, so increase its height
    int sx, sy;
    GetSize( &sx, &sy );
    sy = 1.5 * sy;

    // The bitmap gets cutoff sometimes with the default size, so force it to be the same
    // height as the infobar.
    wxSizer* sizer    = GetSizer();
    wxSize   iconSize = wxArtProvider::GetSizeHint( wxART_BUTTON );

#ifdef __WXMSW__
    DPI_SCALING_COMMON dpi( nullptr, aParent );
    iconSize.x *= dpi.GetContentScaleFactor();
    sx *= dpi.GetContentScaleFactor();
    sy *= dpi.GetContentScaleFactor();
#endif

    SetSize( sx, sy );

    sizer->SetItemMinSize( (size_t) 0, iconSize.x, sy );

    // The default close button doesn't work with the AUI manager update scheme, so take it out
    // of the layout to ensure any close button displayed is ours.  It must not be destroyed
    // because wxInfoBarGeneric keeps a private pointer to it and recolors it on every system
    // color change.
    wxSizerItem* lastItem = sizer->GetItem( sizer->GetItemCount() - 1 );

    if( wxWindow* defaultCloseButton = lastItem ? lastItem->GetWindow() : nullptr )
    {
        sizer->Detach( defaultCloseButton );
        defaultCloseButton->Hide();
    }

    // One gap before the area separates the message from its links, however many links follow
    m_buttonArea = new wxBoxSizer( wxHORIZONTAL );
    sizer->Add( m_buttonArea, wxSizerFlags().Expand().Border( wxLEFT ) );

    for( wxWindow* child : GetChildren() )
    {
        if( wxStaticText* text = dynamic_cast<wxStaticText*>( child ) )
        {
            m_messageText = text;
            break;
        }
    }

    Layout();

    m_anchor->Bind( wxEVT_SIZE, &WX_INFOBAR::onSize, this );
}


WX_INFOBAR::~WX_INFOBAR()
{
    m_anchor->Unbind( wxEVT_SIZE, &WX_INFOBAR::onSize, this );

    m_afterDismiss.clear();
    delete m_showTimer;
}


void WX_INFOBAR::SetShowTime( int aTime )
{
    m_showTime = aTime;
}


void WX_INFOBAR::QueueShowMessage( const wxString& aMessage, int aFlags )
{
    wxCommandEvent* evt = new wxCommandEvent( KIEVT_SHOW_INFOBAR );

    evt->SetString( aMessage.c_str() );
    evt->SetInt( aFlags );

    GetEventHandler()->QueueEvent( evt );
}


void WX_INFOBAR::QueueDismiss()
{
    wxCommandEvent* evt = new wxCommandEvent( KIEVT_DISMISS_INFOBAR );

    GetEventHandler()->QueueEvent( evt );
}


void WX_INFOBAR::ShowMessageFor( const wxString& aMessage, int aTime, int aFlags,
                                 MESSAGE_TYPE aType )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_showTime = aTime;
    ShowMessage( aMessage, aFlags );

    m_type = aType;
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_updateLock = true;

    // What waited on the message being replaced, even under a hidden ancestor, has nothing left to wait for
    if( IsShown() )
        m_afterDismiss.clear();

    m_message = aMessage;
    m_message.Trim();

    wxInfoBarGeneric::ShowMessage( m_message, aFlags );
    setMessageToolTip( m_message );

    KIPLATFORM::UI::KeepAboveExternalRendering( this );

    // Force infoBar to full-width.  Yes, this should happen automatically, and it does -- sometimes.
    FixSize();
    stackOverlays();

    if( m_showTime > 0 )
        m_showTimer->StartOnce( m_showTime );

    m_type = MESSAGE_TYPE::GENERIC;
    m_updateLock = false;
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags, MESSAGE_TYPE aType )
{
    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    ShowMessage( aMessage, aFlags );

    m_type = aType;
}


void WX_INFOBAR::Dismiss()
{
    if( !IsShownOnScreen() )
        return;

    // Don't do anything if we requested the UI update
    if( m_updateLock )
        return;

    m_updateLock = true;

    wxInfoBarGeneric::Dismiss();
    setMessageToolTip( wxEmptyString );

    stackOverlays();
    refreshParent();

    if( m_callback )
        (*m_callback)();

    m_updateLock = false;

    // Queued, since the dismissal may be inside a handler of the message being dismissed. Any call may
    // destroy the infobar, so each one checks it is still alive
    if( !m_afterDismiss.empty() )
    {
        CallAfter(
                [bar = wxWeakRef<WX_INFOBAR>( this ), pending = std::exchange( m_afterDismiss, {} )]()
                {
                    for( const std::function<void()>& fn : pending )
                    {
                        if( !bar || bar->IsBeingDeleted() )
                            return;

                        fn();
                    }
                } );
    }
}


void WX_INFOBAR::onThemeChange( wxSysColourChangedEvent& aEvent )
{
    wxColour fg, bg;
    KIPLATFORM::UI::GetInfoBarColours( fg, bg );
    SetBackgroundColour( bg );
    SetForegroundColour( fg );

    // The close button bitmaps are filled with the old background, so put a new button in its place
    if( wxBitmapButton* btn = GetCloseButton() )
    {
        wxBitmapButton* fresh = wxBitmapButton::NewCloseButton( this, ID_CLOSE_INFOBAR );
#ifdef __WXMAC__
        fresh->SetWindowVariant( wxWINDOW_VARIANT_SMALL );
#endif
        fresh->SetToolTip( btn->GetToolTipText() );
        GetSizer()->Replace( btn, fresh, true );
        btn->Destroy();
        Layout();
    }
}


// Big hack here.  We've had many bugs for a long time with the info bar not being full-width.  Things got
// better in the 9.0 time-frame, but then started to degrade again in 10.0/11.0.
//
// This attempts to fix things by correcting the size every time the infobar is shown.  But even that has
// flies in the ointment, as the "normal" infobar height is calculated before the icon is added to it, and
// is shorter.
//
// So we have to squirrel away the icon, calculate the size, and then re-attach the icon.

void WX_INFOBAR::FixSize()
{
    wxSizer* sizer = GetSizer();

    if( !sizer || sizer->GetItemCount() == 0 )
        return;

#if wxCHECK_VERSION( 3, 3, 0 )
    // On wx 3.3 item 0 is not the icon. It is a nested sizer that holds everything.
    // Detaching it leaves the infobar empty. Then doSize() reads a null item and crashes.
    doSize();
    Layout();
#else
    wxWindow* icon = sizer->GetItem( (size_t) 0 )->GetWindow();
    sizer->Detach( 0 );

    doSize();

    sizer->Prepend( icon, 0, wxLEFT | wxRIGHT, 5 );
    Layout();
#endif
}


void WX_INFOBAR::setMessageToolTip( const wxString& aText )
{
    for( wxWindow* child : GetChildren() )
    {
        if( dynamic_cast<wxStaticText*>( child ) )
        {
            // Unset first so a tooltip already showing is rebuilt rather than kept
            child->UnsetToolTip();

            if( !aText.IsEmpty() )
                child->SetToolTip( aText );

            return;
        }
    }
}


bool WX_INFOBAR::Layout()
{
    bool laidOut = wxInfoBarGeneric::Layout();

    if( !m_messageText || !m_buttonArea || !m_buttonArea->AreAnyItemsShown() )
        return laidOut;

    int lineCount = m_messageText->GetLabel().Freq( '\n' ) + 1;

    // Centred links would move down as the message grows, so hold them level with its first line
    if( lineCount > 1 )
    {
        wxRect text = m_messageText->GetRect();
        int    lineCentre = text.y + text.height / ( 2 * lineCount );
        int    height = std::max( 2 * lineCentre, m_buttonArea->GetMinSize().y );
        int    top = std::clamp( lineCentre - height / 2, 0, std::max( 0, GetClientSize().y - height ) );

        m_buttonArea->SetDimension( wxPoint( m_buttonArea->GetPosition().x, top ),
                                    wxSize( m_buttonArea->GetSize().x, height ) );
    }

    return laidOut;
}


void WX_INFOBAR::onSize( wxSizeEvent& aEvent )
{
    doSize();
    stackOverlays();

    aEvent.Skip();
}


void WX_INFOBAR::doSize()
{
    int barWidth = GetSize().GetWidth();
    wxSizer* sizer = GetSizer();

    if( !sizer )
        return;

    // wx3.3 moved the sizer we previously wanted deeper into sizers...
    // do we actually still need this for wx3.3?
    #if wxCHECK_VERSION( 3, 3, 0 )
    wxSizerItem* outerSizer = sizer->GetItem( (size_t) 0 );
    wxSizerItem* textSizer = nullptr;

    if (outerSizer->IsSizer())
    {
        wxBoxSizer* innerSizer1 = dynamic_cast<wxBoxSizer*>( outerSizer->GetSizer() );
        wxBoxSizer* innerSizer2 =
                dynamic_cast<wxBoxSizer*>( innerSizer1->GetItem((size_t)0)->GetSizer() );

        if( innerSizer2 )
            textSizer = innerSizer2->GetItem( 1 );
    }
    #else
    wxSizerItem* textSizer = sizer->GetItem( 1 );
    #endif

    if( textSizer )
    {
        if( wxStaticText* textCtrl = dynamic_cast<wxStaticText*>( textSizer->GetWindow() ) )
            textCtrl->SetLabelText( m_message );
    }

    int parentWidth = m_anchor->GetClientSize().GetWidth();

    if( barWidth != parentWidth )
        SetSize( parentWidth, GetSize().GetHeight() );

    if( textSizer )
    {
        if( wxStaticText* textCtrl = dynamic_cast<wxStaticText*>( textSizer->GetWindow() ) )
        {
            // Re-wrap the text (this is done automatically later but we need it now)
            // And count how many lines we need.  If we have embedded newlines, then
            // multiply the number of lines by the text min height to find the correct
            // min height for the control.  The min height of the text control will be the size
            // of a single line of text.  This assumes that two lines of text are larger
            // than the height of the icon for the bar.
            textCtrl->Wrap( -1 );
            wxString wrapped_text = textCtrl->GetLabel();
            int line_count = wrapped_text.Freq( '\n' ) + 1;
            int txt_h, txt_v;
            wxWindowDC dc( textCtrl );
            dc.GetTextExtent( wxT( "Xp" ), &txt_h, &txt_v );

            int      height = txt_v * line_count;
            int      margins = txt_v - 1;
            SetMinSize( wxSize( GetSize().GetWidth(), height + margins ) );

            textCtrl->Wrap( -1 );
        }
    }
}


void WX_INFOBAR::stackOverlays()
{
    if( !m_overlay )
        return;

    int width = m_anchor->GetClientSize().GetWidth();
    int y = 0;

    for( wxWindow* child : m_parent->GetChildren() )
    {
        WX_INFOBAR* bar = dynamic_cast<WX_INFOBAR*>( child );

        if( !bar || !bar->IsShown() )
            continue;

        int height = bar->GetEffectiveMinSize().GetHeight();

        bar->SetSize( 0, y, width, height );

        // The GAL backend window is a sibling that raises itself whenever it is shown
        bar->Raise();

        y += height;
    }

    if( m_parent != m_anchor )
    {
        if( y > 0 )
            m_parent->SetSize( wxDefaultCoord, wxDefaultCoord, width, y );

        m_parent->Show( y > 0 );
    }

    if( EDA_DRAW_PANEL_GAL* canvas = dynamic_cast<EDA_DRAW_PANEL_GAL*>( m_anchor ) )
        canvas->UpdateOverlayExclusions();
}


void WX_INFOBAR::refreshParent()
{
    if( !m_overlay )
        return;

    // A GAL canvas skips repaints when no target is dirty, leaving the uncovered strip stale
    if( EDA_DRAW_PANEL_GAL* canvas = dynamic_cast<EDA_DRAW_PANEL_GAL*>( m_anchor ) )
    {
        canvas->ForceRefresh();

        // ForceRefresh() gives up silently if the context is busy, so queue a backstop
        canvas->RequestRefresh();
    }
    else
    {
        m_anchor->Refresh();
    }
}


void WX_INFOBAR::AddButton( wxWindowID aId, const wxString& aLabel )
{
    wxButton* button = new wxButton( this, aId, aLabel );

    AddButton( button );
}


void WX_INFOBAR::AddButton( wxButton* aButton )
{
    wxSizer* sizer = GetSizer();

    wxCHECK2( aButton, return );

#ifdef __WXMAC__
    // Based on the code in the original class:
    // smaller buttons look better in the (narrow) info bar under OS X
    aButton->SetWindowVariant( wxWINDOW_VARIANT_SMALL );
#endif // __WXMAC__

    // Centred like the links, so the close button sits level with the message
    m_buttonArea->Add( aButton, wxSizerFlags().CentreVertical() );

    if( IsShownOnScreen() )
    {
        Layout();
        sizer->Fit( this );
    }
}


void WX_INFOBAR::AddLink(const wxString& aLinkText, const std::function<void(wxHyperlinkEvent&)>& aFn )
{
    wxSizer*         sizer = GetSizer();
    wxHyperlinkCtrl* button = new wxHyperlinkCtrl( this, wxID_ANY, aLinkText, wxEmptyString );

    button->Bind( wxEVT_COMMAND_HYPERLINK, aFn );

    // The default border is too narrow to tell neighbouring links apart
    m_buttonArea->Add( button, wxSizerFlags().CentreVertical().Border( wxRIGHT, FromDIP( 12 ) ) );

    if( IsShownOnScreen() )
    {
        Layout();
        sizer->Fit( this );
    }
}


void WX_INFOBAR::AddMenuLink( const wxString& aLinkText, const std::vector<MENU_LINK_ITEM>& aItems )
{
    AddLink( aLinkText + wxS( " \u25BC" ),
             [aItems]( wxHyperlinkEvent& aEvent )
             {
                 constexpr int firstId = wxID_HIGHEST + 1;
                 wxWindow*     link = dynamic_cast<wxWindow*>( aEvent.GetEventObject() );

                 if( !link )
                     return;

                 wxMenu menu;

                 for( size_t ii = 0; ii < aItems.size(); ++ii )
                     menu.Append( firstId + static_cast<int>( ii ), aItems[ii].first );

                 const int chosen = link->GetPopupMenuSelectionFromUser( menu, wxPoint( 0, link->GetSize().y ) );

                 if( chosen < firstId || chosen >= firstId + static_cast<int>( aItems.size() ) )
                     return;

                 // Copied, since the entry may rebuild the infobar and free this handler
                 std::function<void()> run = aItems[chosen - firstId].second;
                 run();
             } );
}


void WX_INFOBAR::AddCloseButton( const wxString& aTooltip )
{
    wxBitmapButton* button = wxBitmapButton::NewCloseButton( this, ID_CLOSE_INFOBAR );

    button->SetToolTip( aTooltip );

    AddButton( button );
}


void WX_INFOBAR::RemoveAllButtons()
{
    m_buttonArea->Clear( true );
}


bool WX_INFOBAR::HasCloseButton() const
{
    return GetCloseButton();
}


wxBitmapButton* WX_INFOBAR::GetCloseButton() const
{
    if( m_buttonArea->GetItemCount() == 0 )
        return nullptr;

    wxSizerItem* item = m_buttonArea->GetItem( m_buttonArea->GetItemCount() - 1 );

    if( item && item->GetWindow() && item->GetWindow()->GetId() == ID_CLOSE_INFOBAR )
        return static_cast<wxBitmapButton*>( item->GetWindow() );

    return nullptr;
}


void WX_INFOBAR::onShowInfoBar( wxCommandEvent& aEvent )
{
    RemoveAllButtons();
    AddCloseButton();
    ShowMessage( aEvent.GetString(), aEvent.GetInt() );
}


void WX_INFOBAR::onDismissInfoBar( wxCommandEvent& aEvent )
{
    Dismiss();
}


void WX_INFOBAR::onCloseButton( wxCommandEvent& aEvent )
{
    Dismiss();
}


void WX_INFOBAR::onTimer( wxTimerEvent& aEvent )
{
    // Reset and clear the timer
    m_showTimer->Stop();
    m_showTime = 0;

    Dismiss();
}


EDA_INFOBAR_PANEL::EDA_INFOBAR_PANEL( wxWindow* aParent, wxWindowID aId, const wxPoint& aPos,
                                      const wxSize& aSize, long aStyle, const wxString& aName )
         : wxPanel( aParent, aId, aPos, aSize, aStyle, aName )
{
    m_mainSizer = new wxFlexGridSizer( 1, 0, 0 );

    m_mainSizer->SetFlexibleDirection( wxBOTH );
    m_mainSizer->AddGrowableCol( 0, 1 );

    SetSizer( m_mainSizer );
}


void EDA_INFOBAR_PANEL::AddInfoBar( WX_INFOBAR* aInfoBar )
{
    wxCHECK2( aInfoBar, return );

    aInfoBar->Reparent( this );
    m_mainSizer->Add( aInfoBar, 1, wxEXPAND, 0 );
    m_mainSizer->Layout();
}


void EDA_INFOBAR_PANEL::AddOtherItem( wxWindow* aOtherItem )
{
    wxCHECK2( aOtherItem, return );

    aOtherItem->Reparent( this );
    m_mainSizer->Add( aOtherItem, 1, wxEXPAND, 0 );

    m_mainSizer->AddGrowableRow( 1, 1 );
    m_mainSizer->Layout();
}


REPORTER& INFOBAR_REPORTER::Report( const wxString& aText, SEVERITY aSeverity )
{
    m_message.reset( new wxString( aText ) );
    m_severity = aSeverity;
    m_messageSet = true;

    return *this;
}


bool INFOBAR_REPORTER::HasMessage() const
{
    return m_message && !m_message->IsEmpty();
}


void INFOBAR_REPORTER::Finalize()
{
    // Don't do anything if no message was ever given
    if( !m_infoBar || !m_messageSet )
        return;

    // Consume the pending report
    m_messageSet = false;

    // Short circuit if the message is empty and it is already hidden
    if( !HasMessage() && !m_infoBar->IsShownOnScreen() )
        return;

    int icon = wxICON_NONE;

    switch( m_severity )
    {
    case RPT_SEVERITY_UNDEFINED: icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_INFO:      icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_EXCLUSION: icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_ACTION:    icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_WARNING:   icon = wxICON_WARNING;     break;
    case RPT_SEVERITY_ERROR:     icon = wxICON_ERROR;       break;
    case RPT_SEVERITY_IGNORE:    icon = wxICON_INFORMATION; break;
    case RPT_SEVERITY_DEBUG:     icon = wxICON_INFORMATION; break;
    }

    if( m_message->EndsWith( wxS( "\n" ) ) )
        *m_message = m_message->Left( m_message->Length() - 1 );

    if( HasMessage() )
        m_infoBar->QueueShowMessage( *m_message, icon );
    else
        m_infoBar->QueueDismiss();
}

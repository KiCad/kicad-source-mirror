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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <id.h>
#include <kiplatform/ui.h>
#include <widgets/wx_infobar.h>
#include "wx/artprov.h"
#include <wx/aui/framemanager.h>
#include <wx/bmpbuttn.h>
#include <wx/debug.h>
#include <wx/hyperlink.h>
#include <wx/infobar.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/timer.h>
#include <wx/dcclient.h>
#include <eda_base_frame.h>

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


WX_INFOBAR::WX_INFOBAR( wxWindow* aParent, wxAuiManager* aMgr, wxWindowID aWinid )
        : wxInfoBarGeneric( aParent, aWinid ),
          m_showTime( 0 ),
          m_updateLock( false ),
          m_showTimer( nullptr ),
          m_auiManager( aMgr ),
          m_type( MESSAGE_TYPE::GENERIC )
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

    // Forcefully remove all existing buttons added by the wx constructors.
    // The default close button doesn't work with the AUI manager update scheme, so this
    // ensures any close button displayed is ours.
    RemoveAllButtons();

    Layout();

    m_parent->Bind( wxEVT_SIZE, &WX_INFOBAR::onSize, this );
}


WX_INFOBAR::~WX_INFOBAR()
{
    m_parent->Unbind( wxEVT_SIZE, &WX_INFOBAR::onSize, this );

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

    m_message = aMessage;
    m_message.Trim();

    wxInfoBarGeneric::ShowMessage( m_message, aFlags );

    if( m_auiManager )
        updateAuiLayout( true );

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

    if( m_auiManager )
        updateAuiLayout( false );

    if( m_callback )
        (*m_callback)();

    m_updateLock = false;
}


void WX_INFOBAR::onThemeChange( wxSysColourChangedEvent& aEvent )
{
    wxColour fg, bg;
    KIPLATFORM::UI::GetInfoBarColours( fg, bg );
    SetBackgroundColour( bg );
    SetForegroundColour( fg );

    if( wxBitmapButton* btn = GetCloseButton() )
    {
        wxString tooltip = btn->GetToolTipText();
        RemoveAllButtons();
        AddCloseButton( tooltip );
    }
}


void WX_INFOBAR::onSize( wxSizeEvent& aEvent )
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

    // Calculate the horizontal size: because the infobar is shown on top of the draw canvas
    // it is adjusted to the canvas width.
    // On Mac, the canvas is the parent
    // On other OS the parent is EDA_BASE_FRAME that contains the canvas
    int parentWidth = m_parent->GetClientSize().GetWidth();
    EDA_BASE_FRAME* frame = dynamic_cast<EDA_BASE_FRAME*>( m_parent );

    if( frame && frame->GetToolCanvas() )
        parentWidth = frame->GetToolCanvas()->GetSize().GetWidth();

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

    aEvent.Skip();
}


void WX_INFOBAR::updateAuiLayout( bool aShow )
{
    wxASSERT( m_auiManager );

    wxAuiPaneInfo& pane = m_auiManager->GetPane( this );

    // If the infobar is in a pane, then show/hide the pane
    if( pane.IsOk() )
    {
        if( aShow )
            pane.Show();
        else
            pane.Hide();
    }

    // Update the AUI manager regardless
    m_auiManager->Update();
}


void WX_INFOBAR::AddButton( wxWindowID aId, const wxString& aLabel )
{
    wxButton* button = new wxButton( this, aId, aLabel );

    AddButton( button );
}


void WX_INFOBAR::AddButton( wxButton* aButton )
{
    wxSizer* sizer = GetSizer();

    wxASSERT( aButton );

#ifdef __WXMAC__
    // Based on the code in the original class:
    // smaller buttons look better in the (narrow) info bar under OS X
    aButton->SetWindowVariant( wxWINDOW_VARIANT_SMALL );
#endif // __WXMAC__

    auto element = sizer->Add( aButton, wxSizerFlags( 0 ).Centre().Border( wxRIGHT ) );

    element->SetFlag( wxSTRETCH_MASK );

    if( IsShownOnScreen() )
    {
        Layout();
        sizer->Fit( this );
    }
}


void WX_INFOBAR::AddButton( wxHyperlinkCtrl* aHypertextButton )
{
    wxSizer* sizer = GetSizer();

    wxASSERT( aHypertextButton );

    sizer->Add( aHypertextButton, wxSizerFlags().Centre().Border( wxRIGHT ).Shaped() );

    if( IsShownOnScreen() )
    {
        Layout();
        sizer->Fit( this );
    }
}


void WX_INFOBAR::AddCloseButton( const wxString& aTooltip )
{
    wxBitmapButton* button = wxBitmapButton::NewCloseButton( this, ID_CLOSE_INFOBAR );

    button->SetToolTip( aTooltip );

    AddButton( button );
}


void WX_INFOBAR::RemoveAllButtons()
{
    wxSizer* sizer = GetSizer();

    if( sizer->GetItemCount() == 0 )
        return;

    // The last item is already the spacer
    if( sizer->GetItem( sizer->GetItemCount() - 1 )->IsSpacer() )
        return;

    for( int i = sizer->GetItemCount() - 1; i >= 0; i-- )
    {
        wxSizerItem* sItem = sizer->GetItem( i );

        // The spacer is the end of the custom buttons
        if( sItem->IsSpacer() )
            break;

        delete sItem->GetWindow();
    }
}


bool WX_INFOBAR::HasCloseButton() const
{
    return GetCloseButton();
}


wxBitmapButton* WX_INFOBAR::GetCloseButton() const
{
    wxSizer* sizer = GetSizer();

    if( !sizer )
        return nullptr;

    if( sizer->GetItemCount() == 0 )
        return nullptr;

    if( sizer->GetItem( sizer->GetItemCount() - 1 )->IsSpacer() )
        return nullptr;

    wxSizerItem* item = sizer->GetItem( sizer->GetItemCount() - 1 );

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
    wxASSERT( aInfoBar );

    aInfoBar->Reparent( this );
    m_mainSizer->Add( aInfoBar, 1, wxEXPAND, 0 );
    m_mainSizer->Layout();
}


void EDA_INFOBAR_PANEL::AddOtherItem( wxWindow* aOtherItem )
{
    wxASSERT( aOtherItem );

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

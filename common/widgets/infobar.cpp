/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney@ieee.org>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <widgets/infobar.h>
#include <wx/aui/framemanager.h>
#include <wx/debug.h>
#include <wx/infobar.h>
#include <wx/sizer.h>
#include <wx/timer.h>


BEGIN_EVENT_TABLE( WX_INFOBAR, wxInfoBarGeneric )
    EVT_BUTTON( ID_CLOSE_INFOBAR, WX_INFOBAR::OnCloseButton )
    EVT_TIMER(  ID_CLOSE_INFOBAR, WX_INFOBAR::OnTimer )
END_EVENT_TABLE()


WX_INFOBAR::WX_INFOBAR( wxWindow* aParent, wxAuiManager *aMgr, wxWindowID aWinid )
        : wxInfoBarGeneric( aParent, aWinid ),
        m_showTime( 0 ),
        m_showTimer( nullptr ),
        m_auiManager( aMgr )
{
    m_showTimer = new wxTimer( this, ID_CLOSE_INFOBAR );

    // Don't use any effects since they leave the sizer area visible under the infobar
    SetShowHideEffects( wxSHOW_EFFECT_NONE, wxSHOW_EFFECT_NONE );

    // On GTK, the infobar seems to start too small, so increase its height
#ifdef __WXGTK__
    int sx, sy;
    GetSize( &sx, &sy );
    SetSize( sx, 1.5 * sy );
#endif
}


void WX_INFOBAR::SetShowTime( int aTime )
{
    m_showTime = aTime;
}


void WX_INFOBAR::ShowMessageFor( const wxString& aMessage, int aTime, int aFlags )
{
    m_showTime = aTime;
    ShowMessage( aMessage, aFlags );
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags )
{
    wxInfoBarGeneric::ShowMessage( aMessage, aFlags );

    if( m_auiManager )
        UpdateAuiLayout( true );

    if( m_showTime > 0 )
        m_showTimer->StartOnce( m_showTime );
}


void WX_INFOBAR::Dismiss()
{
    wxInfoBarGeneric::Dismiss();

    if( m_auiManager )
        UpdateAuiLayout( false );
}


void WX_INFOBAR::UpdateAuiLayout( bool aShow )
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
    sizer->Add( aButton, wxSizerFlags().Centre().Border( wxRIGHT ) );

    if( IsShown() )
        sizer->Layout();
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


void WX_INFOBAR::OnCloseButton( wxCommandEvent& aEvent )
{
    Dismiss();
}


void WX_INFOBAR::OnTimer( wxTimerEvent& aEvent )
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

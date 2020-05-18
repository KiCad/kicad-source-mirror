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
#include <widgets/wx_infobar.h>
#include <wx/aui/framemanager.h>
#include <wx/infobar.h>


BEGIN_EVENT_TABLE( WX_INFOBAR, wxInfoBarGeneric )
    EVT_BUTTON( ID_CLOSE_INFOBAR, WX_INFOBAR::OnCloseButton )
END_EVENT_TABLE()


WX_INFOBAR::WX_INFOBAR( wxWindow* aParent, wxAuiManager *aMgr, wxWindowID aWinid )
        : wxInfoBarGeneric( aParent, aWinid ),
        m_auiManager( aMgr )
{
    // On GTK, the infobar seems to start too small, so increase its height
#ifdef __WXGTK__
    int sx, sy;
    GetSize( &sx, &sy );
    SetSize( sx, 1.5 * sy );
#endif
}


void WX_INFOBAR::ShowMessage( const wxString& aMessage, int aFlags )
{
    wxInfoBarGeneric::ShowMessage( aMessage, aFlags );

    UpdateAuiLayout( true );
}


void WX_INFOBAR::Dismiss()
{
    wxInfoBarGeneric::Dismiss();
    UpdateAuiLayout( false );
}


void WX_INFOBAR::UpdateAuiLayout( bool aShow )
{
    // Update the AUI pane that contains the infobar
    if( m_auiManager )
    {
        if( aShow )
            m_auiManager->GetPane( this ).Show();
        else
            m_auiManager->GetPane( this ).Hide();

        m_auiManager->Update();
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

#ifdef __WXMAC__
    // Based on the code in the original class:
    // smaller buttons look better in the (narrow) info bar under OS X
    aButton->SetWindowVariant( wxWINDOW_VARIANT_SMALL );
#endif // __WXMAC__
    sizer->Add( aButton, wxSizerFlags().Centre() );

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


void WX_INFOBAR::OnCloseButton( wxCommandEvent& aEvt )
{
    Dismiss();
}

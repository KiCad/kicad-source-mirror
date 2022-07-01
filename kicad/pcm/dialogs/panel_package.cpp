/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Andrew Lutsenko, anlutsenko at gmail dot com
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math/util.h>
#include <wx/dcclient.h>

#include "panel_package.h"

PANEL_PACKAGE::PANEL_PACKAGE( wxWindow* parent, const ActionCallback& aCallback,
                              const PACKAGE_VIEW_DATA& aData ) :
        PANEL_PACKAGE_BASE( parent ),
        m_actionCallback( aCallback ), m_data( aData )
{
    // Propagate clicks on static elements to the panel handler.
    m_name->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_PACKAGE::OnClick ), NULL, this );
    m_desc->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_PACKAGE::OnClick ), NULL, this );
    m_bitmap->Connect( wxEVT_LEFT_DOWN, wxMouseEventHandler( PANEL_PACKAGE::OnClick ), NULL, this );

    wxColour bgColor = wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK );
    m_desc->SetBackgroundColour( bgColor );
    m_name->SetBackgroundColour( bgColor );
    m_bitmap->SetBackgroundColour( bgColor );

    m_name->SetFont( m_name->GetFont().Bold() );

    m_name->SetLabel( m_data.package.name );
    m_bitmap->SetBitmap( *m_data.bitmap );

    // Set min width to 0 otherwise wxStaticLabel really doesn't want to shrink on resize
    m_desc->SetMinSize( wxSize( 0, -1 ) );

    m_minHeight = GetMinHeight();

    double descLineHeight = m_desc->GetTextExtent( "X" ).GetHeight() * 1.2 /* leading */;
    m_desc->SetLabel( m_data.package.description );
    descLineHeight = wxSplit( m_desc->GetLabel(), '\n' ).size() * descLineHeight;

    int    nameLineHeight = m_name->GetTextExtent( "X" ).GetHeight();
    wxSize minSize = GetMinSize();
    minSize.y = std::max( nameLineHeight + KiROUND( descLineHeight ) + 15, m_minHeight );
    SetMinSize( minSize );

    m_splitButton->SetLabel( _( "Update" ) );
    m_splitButton->Bind( wxEVT_BUTTON, &PANEL_PACKAGE::OnButtonClicked, this );

    wxMenu*     splitMenu = m_splitButton->GetSplitButtonMenu();
    wxMenuItem* menuItem = splitMenu->Append( wxID_ANY, _( "Uninstall" ) );

    splitMenu->Bind( wxEVT_COMMAND_MENU_SELECTED, &PANEL_PACKAGE::OnUninstallClick, this,
                     menuItem->GetId() );

    SetState( m_data.state );
}


void PANEL_PACKAGE::OnSize( wxSizeEvent& event )
{
    Layout();
}


void PANEL_PACKAGE::SetState( PCM_PACKAGE_STATE aState )
{
    m_data.state = aState;

    switch( aState )
    {
    case PCM_PACKAGE_STATE::PPS_AVAILABLE:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Install" ) );
        m_button->Enable();
        break;
    case PCM_PACKAGE_STATE::PPS_UNAVAILABLE:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Install" ) );
        m_button->Disable();
        break;
    case PCM_PACKAGE_STATE::PPS_INSTALLED:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Uninstall" ) );
        m_button->Enable();
        break;
    case PCM_PACKAGE_STATE::PPS_PENDING_INSTALL:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Install Pending" ) );
        m_button->Disable();
        break;
    case PCM_PACKAGE_STATE::PPS_PENDING_UNINSTALL:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Uninstall Pending" ) );
        m_button->Disable();
        break;
    case PCM_PACKAGE_STATE::PPS_UPDATE_AVAILABLE:
        // The only state where the split button is shown instead of the normal one
        m_button->Hide();
        m_splitButton->Show();
        break;
    case PCM_PACKAGE_STATE::PPS_PENDING_UPDATE:
        m_splitButton->Hide();
        m_button->Show();
        m_button->SetLabel( _( "Update Pending" ) );
        m_button->Disable();
        break;
    }

    // Relayout to change button size to fit the label.
    Layout();
}


void PANEL_PACKAGE::OnButtonClicked( wxCommandEvent& event )
{
    if( m_data.state == PPS_AVAILABLE )
    {
        wxString version = GetPreferredVersion();

        if( version.IsEmpty() )
            return;

        m_actionCallback( m_data, PPA_INSTALL, version );
    }
    else if( m_data.state == PPS_UPDATE_AVAILABLE )
    {
        m_actionCallback( m_data, PPA_UPDATE, m_data.update_version );
    }
    else
    {
        m_actionCallback( m_data, PPA_UNINSTALL, m_data.current_version );
    }
}


void PANEL_PACKAGE::OnUninstallClick( wxCommandEvent& event )
{
    if( m_data.state == PPS_UPDATE_AVAILABLE )
    {
        m_actionCallback( m_data, PPA_UNINSTALL, m_data.current_version );
    }
    else
    {
        // Clicking uninstall menu item of the split button should not be possible
        // for any state other than UPDATE_AVAILABLE
        wxLogError( wxT( "Uninstall clicked in unexpected state" ) );
    }
}


void PANEL_PACKAGE::SetSelectCallback( const std::function<void()>& aCallback )
{
    m_selectCallback = aCallback;
}


void PANEL_PACKAGE::OnClick( wxMouseEvent& event )
{
    m_selectCallback();
}


void PANEL_PACKAGE::OnPaint( wxPaintEvent& event )
{
    wxRect    rect( wxPoint( 1, 1 ), GetClientSize() - wxSize( 1, 1 ) );
    wxPaintDC dc( this );
    dc.SetBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK ) );
    dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ), 1 ) );

    if( m_selected )
    {
        rect.Deflate( 1 );
        dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_HOTLIGHT ), 3 ) );
    }

    dc.DrawRectangle( rect );
}


void PANEL_PACKAGE::SetSelected( bool aSelected )
{
    m_selected = aSelected;
    Refresh();
}


wxString PANEL_PACKAGE::GetPreferredVersion() const
{
    // Versions are already presorted in descending order

    // Find last stable compatible version
    auto ver_it = std::find_if( m_data.package.versions.begin(), m_data.package.versions.end(),
                                []( const PACKAGE_VERSION& ver )
                                {
                                    return ver.compatible && ver.status == PVS_STABLE;
                                } );

    // If not found then find any compatible version
    if( ver_it == m_data.package.versions.end() )
    {
        ver_it = std::find_if( m_data.package.versions.begin(), m_data.package.versions.end(),
                               []( const PACKAGE_VERSION& ver )
                               {
                                   return ver.compatible;
                               } );
    }

    if( ver_it == m_data.package.versions.end() )
        return wxEmptyString; // Shouldn't happen

    return ver_it->version;
}

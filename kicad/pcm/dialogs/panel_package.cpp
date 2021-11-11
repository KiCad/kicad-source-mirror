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

    Layout();

    m_desc->SetLabel( m_data.package.description );
    m_desc->Wrap( m_desc->GetClientSize().GetWidth() );

    SetState( m_data.state );
}


void PANEL_PACKAGE::OnSize( wxSizeEvent& event )
{
    Layout();
    m_desc->SetLabel( m_data.package.description );
    m_desc->Wrap( m_desc->GetClientSize().GetWidth() );
}


void PANEL_PACKAGE::SetState( PCM_PACKAGE_STATE aState )
{
    m_data.state = aState;

    switch( aState )
    {
    case PCM_PACKAGE_STATE::PPS_AVAILABLE:
        m_button->SetLabel( _( "Install" ) );
        m_button->Enable();
        break;
    case PCM_PACKAGE_STATE::PPS_UNAVAILABLE:
        m_button->SetLabel( _( "Install" ) );
        m_button->Disable();
        break;
    case PCM_PACKAGE_STATE::PPS_INSTALLED:
        m_button->SetLabel( _( "Uninstall" ) );
        m_button->Enable();
        break;
    case PCM_PACKAGE_STATE::PPS_PENDING_INSTALL:
        m_button->SetLabel( _( "Pending install" ) );
        m_button->Disable();
        break;
    case PCM_PACKAGE_STATE::PPS_PENDING_UNINSTALL:
        m_button->SetLabel( _( "Pending uninstall" ) );
        m_button->Disable();
        break;
    }

    // Relayout to change button size to fit the label.
    wxSizeEvent dummy;
    OnSize( dummy );
}


void PANEL_PACKAGE::OnButtonClicked( wxCommandEvent& event )
{
    // Versions are already presorted in descending order
    if( m_data.state == PPS_AVAILABLE )
    {
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
            return; // Shouldn't happen

        m_actionCallback( m_data, PPA_INSTALL, ver_it->version );
    }
    else
    {
        m_actionCallback( m_data, PPA_UNINSTALL, m_data.current_version );
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
    wxPaintDC dc( this );
    dc.SetBrush( wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK ) );
    dc.SetPen( wxPen( wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER ), 1 ) );

    if( m_selected )
        dc.SetPen( wxPen( *wxBLACK, 3 ) );

    dc.DrawRectangle( wxPoint( 0, 0 ), GetClientSize() );

    if( !m_selected )
        dc.DrawLine( 0, 0, GetClientSize().GetX(), 0 );
}


void PANEL_PACKAGE::SetSelected( bool aSelected )
{
    m_selected = aSelected;
    Refresh();
}

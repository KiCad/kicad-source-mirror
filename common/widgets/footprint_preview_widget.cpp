/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
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

#include <widgets/footprint_preview_widget.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <kiway.h>


FOOTPRINT_PREVIEW_WIDGET::FOOTPRINT_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway ):
    wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
             wxFULL_REPAINT_ON_RESIZE | wxSUNKEN_BORDER | wxTAB_TRAVERSAL ),
    m_prev_panel( nullptr ),
    m_status_label( nullptr ),
    m_sizer( nullptr )
{
    m_prev_panel = FOOTPRINT_PREVIEW_PANEL_BASE::Create( this, aKiway );

    if( !m_prev_panel )
        return;

    SetBackgroundColour( *wxBLACK );
    SetForegroundColour( *wxWHITE );

    m_status_label = new wxStaticText( this, -1, wxEmptyString );
    m_sizer = new wxBoxSizer( wxVERTICAL );
    m_sizer->Add( 0, 0, 1 );
    m_sizer->Add( m_status_label, 0, wxALL | wxALIGN_CENTER, 0 );
    m_sizer->Add( 0, 0, 1 );

    auto outer_sizer = new wxBoxSizer( wxVERTICAL );
    outer_sizer->Add( m_prev_panel->GetWindow(), 1, wxALL | wxEXPAND, 0 );
    outer_sizer->Add( m_sizer, 1, wxALL | wxALIGN_CENTER, 0 );

    m_sizer->ShowItems( false );
    m_prev_panel->SetStatusHandler( [this]( FOOTPRINT_STATUS s ){ this->OnStatusChange( s ); } );

    SetSizer( outer_sizer );
}


void FOOTPRINT_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    m_status_label->SetLabel( aText );
    m_sizer->ShowItems( true );
    m_prev_panel->GetWindow()->Hide();
    Layout();
}


void FOOTPRINT_PREVIEW_WIDGET::ClearStatus()
{
    m_status_label->SetLabel( wxEmptyString );
    m_prev_panel->GetWindow()->Show();
    m_sizer->ShowItems( false );
    Layout();
}


void FOOTPRINT_PREVIEW_WIDGET::CacheFootprint( const LIB_ID& aFPID )
{
    if( m_prev_panel )
        (void) m_prev_panel->CacheFootprint( aFPID );
}


void FOOTPRINT_PREVIEW_WIDGET::DisplayFootprint( const LIB_ID& aFPID )
{
    if( m_prev_panel )
        (void) m_prev_panel->DisplayFootprint( aFPID );
}


void FOOTPRINT_PREVIEW_WIDGET::OnStatusChange( FOOTPRINT_STATUS aStatus )
{
    switch( aStatus )
    {
    case FPS_NOT_FOUND:
        SetStatusText( _( "Footprint not found" ) );
        break;

    case FPS_LOADING:
        SetStatusText( _( "Loading..." ) );
        break;

    case FPS_READY:
        ClearStatus();
    }

    Refresh();
}


FOOTPRINT_PREVIEW_PANEL_BASE* FOOTPRINT_PREVIEW_PANEL_BASE::Create(
        wxWindow* aParent, KIWAY& aKiway )
{
    FOOTPRINT_PREVIEW_PANEL_BASE* panel = nullptr;

    try {
        KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB );

        auto window = kiface->CreateWindow( aParent, FRAME_PCB_FOOTPRINT_PREVIEW, &aKiway );

        panel = dynamic_cast<FOOTPRINT_PREVIEW_PANEL_BASE*>( window );

        if( window && !panel )
            delete window;
    } catch( ... )
    {}

    return panel;
}

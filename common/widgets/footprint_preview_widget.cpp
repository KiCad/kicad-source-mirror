/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_id.h>
#include <class_draw_panel_gal.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <kiway.h>


FOOTPRINT_PREVIEW_WIDGET::FOOTPRINT_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxFULL_REPAINT_ON_RESIZE | wxTAB_TRAVERSAL ),
        m_prev_panel( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_statusSizer( nullptr ),
        m_outerSizer( nullptr )
{
    m_prev_panel = FOOTPRINT_PREVIEW_PANEL_BASE::Create( this, aKiway );

    m_statusPanel = new wxPanel( this );
    m_status = new wxStaticText( m_statusPanel, wxID_ANY, wxEmptyString );
    m_statusSizer = new wxBoxSizer( wxVERTICAL );
    m_statusSizer->Add( 0, 0, 1 );  // add a spacer
    m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
    m_statusSizer->Add( 0, 0, 1 );  // add a spacer
    m_statusPanel->SetSizer( m_statusSizer );

    if( m_prev_panel )
    {
        // Give the status panel the same color scheme as the canvas so it isn't jarring when
        // switched to
        m_statusPanel->SetBackgroundColour( m_prev_panel->GetBackgroundColor().ToColour() );
        m_statusPanel->SetForegroundColour( m_prev_panel->GetForegroundColor().ToColour() );
        m_status->SetForegroundColour( m_prev_panel->GetForegroundColor().ToColour() );

        // Set our background so wx doesn't render a normal control background momentarily when
        // rapidly navigating with arrow keys
        SetBackgroundColour( m_prev_panel->GetBackgroundColor().ToColour() );
        SetForegroundColour( m_prev_panel->GetForegroundColor().ToColour() );
    }

    m_outerSizer = new wxBoxSizer( wxVERTICAL );

    if( m_prev_panel )
        m_outerSizer->Add( m_prev_panel->GetCanvas(), 1, wxALL | wxEXPAND, 0 );

    m_outerSizer->Add( m_statusPanel, 1, wxALL | wxEXPAND, 0 );

    SetSizer( m_outerSizer );

    SetStatusText( wxEmptyString );
}


void FOOTPRINT_PREVIEW_WIDGET::SetStatusText( wxString const& aText )
{
    m_status->SetLabel( aText );
    m_statusPanel->Show();

    if( m_prev_panel )
        m_prev_panel->GetCanvas()->Hide();

    Layout();
}


void FOOTPRINT_PREVIEW_WIDGET::ClearStatus()
{
    m_status->SetLabel( wxEmptyString );
    m_statusPanel->Hide();

    if( m_prev_panel )
        m_prev_panel->GetCanvas()->Show();

    Layout();
}


void FOOTPRINT_PREVIEW_WIDGET::SetUserUnits( EDA_UNITS aUnits )
{
    if( m_prev_panel )
        m_prev_panel->SetUserUnits( aUnits );
}


void FOOTPRINT_PREVIEW_WIDGET::SetPinFunctions( const std::map<wxString, wxString>& aPinFunctions )
{
    if( m_prev_panel )
        m_prev_panel->SetPinFunctions( aPinFunctions );
}


void FOOTPRINT_PREVIEW_WIDGET::DisplayFootprint( const LIB_ID& aFPID )
{
    if( !m_prev_panel || m_libid == aFPID )
        return;

    wxBusyCursor busy;

    if( m_prev_panel->DisplayFootprint( aFPID ) )
    {
        ClearStatus();
        m_libid = aFPID;
    }
    else
    {
        SetStatusText( _( "Footprint not found." ) );
        m_libid.clear();
    }
}


void FOOTPRINT_PREVIEW_WIDGET::DisplayFootprints( std::shared_ptr<FOOTPRINT> aFootprintA,
                                                  std::shared_ptr<FOOTPRINT> aFootprintB )
{
    ClearStatus();

    if( m_prev_panel )
        m_prev_panel->DisplayFootprints( aFootprintA, aFootprintB );
}


void FOOTPRINT_PREVIEW_WIDGET::RefreshAll()
{
    if( m_prev_panel )
        m_prev_panel->RefreshAll();
}


FOOTPRINT_PREVIEW_PANEL_BASE* FOOTPRINT_PREVIEW_PANEL_BASE::Create( wxWindow* aParent,
                                                                    KIWAY& aKiway )
{
    wxWindow* panel = nullptr;

    try
    {
        if( KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB ) )
            panel = kiface->CreateKiWindow( aParent, FRAME_FOOTPRINT_PREVIEW, &aKiway );
    }
    catch( ... )
    {
    }

    return dynamic_cast<FOOTPRINT_PREVIEW_PANEL_BASE*>( panel );
}

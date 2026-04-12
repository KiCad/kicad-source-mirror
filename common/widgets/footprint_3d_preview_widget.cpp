/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <widgets/footprint_3d_preview_widget.h>

#include <frame_type.h>
#include <kiway.h>

#include <wx/sizer.h>
#include <wx/stattext.h>


FOOTPRINT_3D_PREVIEW_WIDGET::FOOTPRINT_3D_PREVIEW_WIDGET( wxWindow* aParent, KIWAY& aKiway ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                 wxFULL_REPAINT_ON_RESIZE | wxTAB_TRAVERSAL ),
        m_prevPanel( nullptr ),
        m_status( nullptr ),
        m_statusPanel( nullptr ),
        m_statusSizer( nullptr ),
        m_outerSizer( nullptr )
{
    m_prevPanel = FOOTPRINT_3D_PREVIEW_PANEL_BASE::Create( this, aKiway );

    m_statusPanel = new wxPanel( this );
    m_status = new wxStaticText( m_statusPanel, wxID_ANY, wxEmptyString );
    m_statusSizer = new wxBoxSizer( wxVERTICAL );
    m_statusSizer->Add( 0, 0, 1 );
    m_statusSizer->Add( m_status, 0, wxALIGN_CENTER );
    m_statusSizer->Add( 0, 0, 1 );
    m_statusPanel->SetSizer( m_statusSizer );

    m_outerSizer = new wxBoxSizer( wxVERTICAL );

    if( m_prevPanel )
        m_outerSizer->Add( m_prevPanel->GetWindow(), 1, wxALL | wxEXPAND, 0 );

    m_outerSizer->Add( m_statusPanel, 1, wxALL | wxEXPAND, 0 );

    SetSizer( m_outerSizer );
    Layout();

    SetStatusText( wxEmptyString );
}


void FOOTPRINT_3D_PREVIEW_WIDGET::SetStatusText( const wxString& aText )
{
    m_status->SetLabel( aText );
    m_statusPanel->Show();

    if( m_prevPanel )
        m_prevPanel->GetWindow()->Hide();

    Layout();
}


void FOOTPRINT_3D_PREVIEW_WIDGET::ClearStatus()
{
    m_status->SetLabel( wxEmptyString );
    m_statusPanel->Hide();

    if( m_prevPanel )
        m_prevPanel->GetWindow()->Show();

    Layout();
}


void FOOTPRINT_3D_PREVIEW_WIDGET::ClearPreview()
{
    if( m_prevPanel )
        m_prevPanel->ClearPreview();

    m_libid.clear();
    SetStatusText( wxEmptyString );
}


void FOOTPRINT_3D_PREVIEW_WIDGET::DisplayFootprint( const LIB_ID& aFPID )
{
    if( !m_prevPanel || m_libid == aFPID )
        return;

    wxBusyCursor busy;

    switch( m_prevPanel->DisplayFootprint( aFPID ) )
    {
    case FOOTPRINT_3D_PREVIEW_STATUS::DISPLAYED:
        ClearStatus();
        m_libid = aFPID;
        break;

    case FOOTPRINT_3D_PREVIEW_STATUS::NO_3D_MODEL:
        SetStatusText( _( "No 3D model linked." ) );
        m_libid.clear();
        break;

    case FOOTPRINT_3D_PREVIEW_STATUS::FOOTPRINT_NOT_FOUND:
    default:
        SetStatusText( _( "Footprint not found." ) );
        m_libid.clear();
        break;
    }
}


void FOOTPRINT_3D_PREVIEW_WIDGET::RefreshAll()
{
    if( m_prevPanel )
        m_prevPanel->RefreshAll();
}


void FOOTPRINT_3D_PREVIEW_WIDGET::Shutdown()
{
    if( m_prevPanel )
        m_prevPanel->Shutdown();
}


FOOTPRINT_3D_PREVIEW_PANEL_BASE* FOOTPRINT_3D_PREVIEW_PANEL_BASE::Create( wxWindow* aParent,
                                                                          KIWAY& aKiway )
{
    wxWindow* panel = nullptr;

    try
    {
        if( KIFACE* kiface = aKiway.KiFACE( KIWAY::FACE_PCB ) )
            panel = kiface->CreateKiWindow( aParent, FRAME_FOOTPRINT_3D_PREVIEW, &aKiway );
    }
    catch( ... )
    {
    }

    return dynamic_cast<FOOTPRINT_3D_PREVIEW_PANEL_BASE*>( panel );
}

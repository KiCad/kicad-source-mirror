/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <footprint_3d_preview_panel.h>

#include <3d_canvas/eda_3d_canvas.h>
#include <3d_viewer/eda_3d_viewer_settings.h>
#include <base_units.h>
#include <board.h>
#include <board_design_settings.h>
#include <common_ogl/ogl_attr_list.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <project_pcb.h>
#include <settings/settings_manager.h>

#include <wx/sizer.h>


FOOTPRINT_3D_PREVIEW_PANEL::FOOTPRINT_3D_PREVIEW_PANEL( KIWAY* aKiway, wxWindow* aParent ) :
        wxPanel( aParent, wxID_ANY, wxDefaultPosition, wxDefaultSize ),
        KIWAY_HOLDER( aKiway, KIWAY_HOLDER::PANEL ),
        m_dummyBoard( std::make_unique<BOARD>() ),
        m_currentFootprint(),
        m_boardAdapter(),
        m_trackBallCamera( 2 * RANGE_SCALE_3D ),
        m_currentCamera( m_trackBallCamera ),
        m_canvas( nullptr ),
        m_shutdown( false )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_dummyBoard->SetBoardUse( BOARD_USE::FPHOLDER );
    configureDummyBoard();

    m_boardAdapter.SetBoard( m_dummyBoard.get() );
    m_boardAdapter.m_IsBoardView = false;
    m_boardAdapter.m_IsPreviewer = true;
    m_boardAdapter.m_Cfg = GetAppSettings<EDA_3D_VIEWER_SETTINGS>( "3d_viewer" );

    m_canvas = new EDA_3D_CANVAS( this, OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE::AA_8X ),
                                  m_boardAdapter, m_currentCamera,
                                  PROJECT_PCB::Get3DCacheManager( &Prj() ) );

    sizer->Add( m_canvas, 1, wxEXPAND, 0 );
    SetSizer( sizer );
    Layout();
}


FOOTPRINT_3D_PREVIEW_PANEL::~FOOTPRINT_3D_PREVIEW_PANEL()
{
    ClearPreview();
    Shutdown();
}


void FOOTPRINT_3D_PREVIEW_PANEL::configureDummyBoard()
{
    BOARD_DESIGN_SETTINGS& boardSettings = m_dummyBoard->GetDesignSettings();
    boardSettings.SetBoardThickness( pcbIUScale.mmToIU( 1.6 ) );
    boardSettings.SetEnabledLayers( LSET::FrontMask() | LSET::BackMask() );

    BOARD_STACKUP& stackup = boardSettings.GetStackupDescriptor();
    stackup.RemoveAll();
    stackup.BuildDefaultStackupList( &boardSettings, 2 );
}


bool FOOTPRINT_3D_PREVIEW_PANEL::hasRenderableModels( const FOOTPRINT& aFootprint ) const
{
    for( const FP_3DMODEL& model : aFootprint.Models() )
    {
        if( model.m_Show && !model.m_Filename.IsEmpty() )
            return true;
    }

    return false;
}


void FOOTPRINT_3D_PREVIEW_PANEL::ClearPreview()
{
    if( m_dummyBoard )
        m_dummyBoard->DetachAllFootprints();

    m_currentFootprint.reset();

    if( m_canvas && !m_shutdown )
    {
        m_canvas->ReloadRequest( m_dummyBoard.get(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );
        m_canvas->Request_refresh();
    }
}


FOOTPRINT_3D_PREVIEW_STATUS FOOTPRINT_3D_PREVIEW_PANEL::DisplayFootprint( const LIB_ID& aFPID )
{
    ClearPreview();

    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &Prj() );

    try
    {
        m_currentFootprint.reset( adapter->LoadFootprint( aFPID.GetLibNickname(),
                                                          aFPID.GetLibItemName(), false ) );
    }
    catch( ... )
    {
        m_currentFootprint.reset();
    }

    if( !m_currentFootprint )
        return FOOTPRINT_3D_PREVIEW_STATUS::FOOTPRINT_NOT_FOUND;

    if( !hasRenderableModels( *m_currentFootprint ) )
    {
        m_currentFootprint.reset();
        return FOOTPRINT_3D_PREVIEW_STATUS::NO_3D_MODEL;
    }

    m_dummyBoard->Add( m_currentFootprint.get() );
    m_canvas->ReloadRequest( m_dummyBoard.get(), PROJECT_PCB::Get3DCacheManager( &Prj() ) );
    m_canvas->Request_refresh();

    return FOOTPRINT_3D_PREVIEW_STATUS::DISPLAYED;
}


void FOOTPRINT_3D_PREVIEW_PANEL::RefreshAll()
{
    if( m_canvas )
        m_canvas->Request_refresh();
}


void FOOTPRINT_3D_PREVIEW_PANEL::Shutdown()
{
    if( m_shutdown || !m_canvas )
        return;

    m_shutdown = true;

    // Hidden 3D canvases can assert during teardown, so close them while visible.
    m_canvas->Show();

    wxCloseEvent dummy;
    m_canvas->OnCloseWindow( dummy );
}


FOOTPRINT_3D_PREVIEW_PANEL* FOOTPRINT_3D_PREVIEW_PANEL::New( KIWAY* aKiway, wxWindow* aParent )
{
    return new FOOTPRINT_3D_PREVIEW_PANEL( aKiway, aParent );
}

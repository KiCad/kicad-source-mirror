/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "pcb_draw_panel_gal.h"
#include <pcb_view.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>
#include <ws_proxy_view_item.h>
#include <ratsnest_viewitem.h>
#include <ratsnest_data.h>
#include <connectivity/connectivity_data.h>

#include <colors_design_settings.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_marker_pcb.h>
#include <pcb_base_frame.h>
#include <confirm.h>

#include <gal/graphics_abstraction_layer.h>

#include <functional>
#include <thread>
using namespace std::placeholders;

const LAYER_NUM GAL_LAYER_ORDER[] =
{
    LAYER_GP_OVERLAY,
    LAYER_SELECT_OVERLAY,
    LAYER_DRC,
    LAYER_PADS_NETNAMES, LAYER_VIAS_NETNAMES,
    Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts,

    LAYER_MOD_TEXT_FR,
    LAYER_MOD_REFERENCES, LAYER_MOD_VALUES,

    LAYER_RATSNEST, LAYER_ANCHOR,
    LAYER_VIAS_HOLES, LAYER_PADS_PLATEDHOLES, LAYER_NON_PLATEDHOLES,
    LAYER_VIA_THROUGH, LAYER_VIA_BBLIND,
    LAYER_VIA_MICROVIA, LAYER_PADS_TH,

    LAYER_PAD_FR_NETNAMES, LAYER_PAD_FR,
    NETNAMES_LAYER_INDEX( F_Cu ), F_Cu, F_Mask, F_SilkS, F_Paste, F_Adhes, F_CrtYd, F_Fab,

    NETNAMES_LAYER_INDEX( In1_Cu ),   In1_Cu,
    NETNAMES_LAYER_INDEX( In2_Cu ),   In2_Cu,
    NETNAMES_LAYER_INDEX( In3_Cu ),   In3_Cu,
    NETNAMES_LAYER_INDEX( In4_Cu ),   In4_Cu,
    NETNAMES_LAYER_INDEX( In5_Cu ),   In5_Cu,
    NETNAMES_LAYER_INDEX( In6_Cu ),   In6_Cu,
    NETNAMES_LAYER_INDEX( In7_Cu ),   In7_Cu,
    NETNAMES_LAYER_INDEX( In8_Cu ),   In8_Cu,
    NETNAMES_LAYER_INDEX( In9_Cu ),   In9_Cu,
    NETNAMES_LAYER_INDEX( In10_Cu ),  In10_Cu,
    NETNAMES_LAYER_INDEX( In11_Cu ),  In11_Cu,
    NETNAMES_LAYER_INDEX( In12_Cu ),  In12_Cu,
    NETNAMES_LAYER_INDEX( In13_Cu ),  In13_Cu,
    NETNAMES_LAYER_INDEX( In14_Cu ),  In14_Cu,
    NETNAMES_LAYER_INDEX( In15_Cu ),  In15_Cu,
    NETNAMES_LAYER_INDEX( In16_Cu ),  In16_Cu,
    NETNAMES_LAYER_INDEX( In17_Cu ),  In17_Cu,
    NETNAMES_LAYER_INDEX( In18_Cu ),  In18_Cu,
    NETNAMES_LAYER_INDEX( In19_Cu ),  In19_Cu,
    NETNAMES_LAYER_INDEX( In20_Cu ),  In20_Cu,
    NETNAMES_LAYER_INDEX( In21_Cu ),  In21_Cu,
    NETNAMES_LAYER_INDEX( In22_Cu ),  In22_Cu,
    NETNAMES_LAYER_INDEX( In23_Cu ),  In23_Cu,
    NETNAMES_LAYER_INDEX( In24_Cu ),  In24_Cu,
    NETNAMES_LAYER_INDEX( In25_Cu ),  In25_Cu,
    NETNAMES_LAYER_INDEX( In26_Cu ),  In26_Cu,
    NETNAMES_LAYER_INDEX( In27_Cu ),  In27_Cu,
    NETNAMES_LAYER_INDEX( In28_Cu ),  In28_Cu,
    NETNAMES_LAYER_INDEX( In29_Cu ),  In29_Cu,
    NETNAMES_LAYER_INDEX( In30_Cu ),  In30_Cu,

    LAYER_PAD_BK_NETNAMES, LAYER_PAD_BK,
    NETNAMES_LAYER_INDEX( B_Cu ), B_Cu, B_Mask, B_Adhes, B_Paste, B_SilkS, B_CrtYd, B_Fab,

    LAYER_MOD_TEXT_BK,
    LAYER_WORKSHEET
};


PCB_DRAW_PANEL_GAL::PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
        EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    m_view = new KIGFX::PCB_VIEW( true );
    m_view->SetGAL( m_gal );

    m_painter.reset( new KIGFX::PCB_PAINTER( m_gal ) );
    m_view->SetPainter( m_painter.get() );

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    // Load display options (such as filled/outline display of items).
    // Can be made only if the parent window is an EDA_DRAW_FRAME (or a derived class)
    // which is not always the case (namely when it is used from a wxDialog like the pad editor)
    EDA_DRAW_FRAME* frame = GetParentEDAFrame();

    if( frame )
    {
        auto opts = (PCB_DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        static_cast<KIGFX::PCB_VIEW*>( m_view )->UpdateDisplayOptions( opts );
    }
}


PCB_DRAW_PANEL_GAL::~PCB_DRAW_PANEL_GAL()
{
}


void PCB_DRAW_PANEL_GAL::DisplayBoard( BOARD* aBoard )
{

    m_view->Clear();

    auto zones = aBoard->Zones();
    std::atomic<size_t> next( 0 );
    std::atomic<size_t> count_done( 0 );
    size_t parallelThreadCount = std::max<size_t>( std::thread::hardware_concurrency(), 2 );

    for( size_t ii = 0; ii < parallelThreadCount; ++ii )
    {
        std::thread t = std::thread( [ &count_done, &next, &zones ]( )
        {
            for( size_t i = next.fetch_add( 1 ); i < zones.size(); i = next.fetch_add( 1 ) )
                zones[i]->CacheTriangulation();

            count_done++;
        } );

        t.detach();
    }

    if( m_worksheet )
        m_worksheet->SetFileName( TO_UTF8( aBoard->GetFileName() ) );

    // Load drawings
    for( auto drawing : const_cast<BOARD*>(aBoard)->Drawings() )
        m_view->Add( drawing );

    // Load tracks
    for( auto track : aBoard->Tracks() )
        m_view->Add( track );

    // Load modules and its additional elements
    for( auto module : aBoard->Modules() )
        m_view->Add( module );

    // DRC markers
    for( int marker_idx = 0; marker_idx < aBoard->GetMARKERCount(); ++marker_idx )
    {
        m_view->Add( aBoard->GetMARKER( marker_idx ) );
    }

    // Finalize the triangulation threads
    while( count_done < parallelThreadCount )
        std::this_thread::sleep_for( std::chrono::milliseconds( 10 ) );

    // Load zones
    for( auto zone : aBoard->Zones() )
        m_view->Add( zone );

    // Ratsnest
    m_ratsnest.reset( new KIGFX::RATSNEST_VIEWITEM( aBoard->GetConnectivity() ) );
    m_view->Add( m_ratsnest.get() );
}


void PCB_DRAW_PANEL_GAL::SetWorksheet( KIGFX::WS_PROXY_VIEW_ITEM* aWorksheet )
{
    m_worksheet.reset( aWorksheet );
    m_view->Add( m_worksheet.get() );
}


void PCB_DRAW_PANEL_GAL::UseColorScheme( const COLORS_DESIGN_SETTINGS* aSettings )
{
    KIGFX::PCB_RENDER_SETTINGS* rs;
    rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );
    rs->ImportLegacyColors( aSettings );
    m_gal->SetGridColor( aSettings->GetLayerColor( LAYER_GRID ) );
    m_gal->SetCursorColor( aSettings->GetItemColor( LAYER_CURSOR ) );
}


void PCB_DRAW_PANEL_GAL::SetHighContrastLayer( PCB_LAYER_ID aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearActiveLayers();
    rSettings->SetActiveLayer( aLayer );

    if( IsCopperLayer( aLayer ) )
    {
        // Bring some other layers to the front in case of copper layers and make them colored
        // fixme do not like the idea of storing the list of layers here,
        // should be done in some other way I guess..
        LAYER_NUM layers[] = {
                GetNetnameLayer( aLayer ),
                LAYER_VIA_THROUGH, LAYER_VIAS_HOLES, LAYER_VIAS_NETNAMES,
                LAYER_PADS_TH, LAYER_PADS_PLATEDHOLES, LAYER_PADS_NETNAMES,
                LAYER_NON_PLATEDHOLES, LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY,
                LAYER_RATSNEST, LAYER_CURSOR
        };

        for( unsigned int i : layers )
            rSettings->SetActiveLayer( i );

        // Pads should be shown too
        if( aLayer == B_Cu )
        {
            rSettings->SetActiveLayer( LAYER_PAD_BK );
            rSettings->SetActiveLayer( LAYER_MOD_BK );
            rSettings->SetActiveLayer( LAYER_PAD_BK_NETNAMES );
        }
        else if( aLayer == F_Cu )
        {
            rSettings->SetActiveLayer( LAYER_PAD_FR );
            rSettings->SetActiveLayer( LAYER_MOD_FR );
            rSettings->SetActiveLayer( LAYER_PAD_FR_NETNAMES );
        }
    }

    m_view->UpdateAllLayersColor();
}


void PCB_DRAW_PANEL_GAL::SetTopLayer( PCB_LAYER_ID aLayer )
{
    m_view->ClearTopLayers();
    setDefaultLayerOrder();
    m_view->SetTopLayer( aLayer );

    // Layers that should always have on-top attribute enabled
    const std::vector<LAYER_NUM> layers = {
            LAYER_VIA_THROUGH, LAYER_VIAS_HOLES, LAYER_VIAS_NETNAMES,
            LAYER_PADS_TH, LAYER_PADS_PLATEDHOLES, LAYER_PADS_NETNAMES,
            LAYER_NON_PLATEDHOLES, LAYER_SELECT_OVERLAY, LAYER_GP_OVERLAY,
            LAYER_RATSNEST, LAYER_DRC
    };

    for( auto layer : layers )
        m_view->SetTopLayer( layer );

    // Extra layers that are brought to the top if a F.* or B.* is selected
    const std::vector<LAYER_NUM> frontLayers = {
        F_Cu, F_Adhes, F_Paste, F_SilkS, F_Mask, F_Fab, F_CrtYd, LAYER_PAD_FR,
        LAYER_PAD_FR_NETNAMES, NETNAMES_LAYER_INDEX( F_Cu )
    };

    const std::vector<LAYER_NUM> backLayers = {
        B_Cu, B_Adhes, B_Paste, B_SilkS, B_Mask, B_Fab, B_CrtYd, LAYER_PAD_BK,
        LAYER_PAD_BK_NETNAMES, NETNAMES_LAYER_INDEX( B_Cu )
    };

    const std::vector<LAYER_NUM>* extraLayers = NULL;

    // Bring a few more extra layers to the top depending on the selected board side
    if( IsFrontLayer( aLayer ) )
        extraLayers = &frontLayers;
    else if( IsBackLayer( aLayer ) )
        extraLayers = &backLayers;

    if( extraLayers )
    {
        for( auto layer : *extraLayers )
            m_view->SetTopLayer( layer );

        // Move the active layer to the top
        if( !IsCopperLayer( aLayer ) )
            m_view->SetLayerOrder( aLayer, m_view->GetLayerOrder( GAL_LAYER_ORDER[0] ) );
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Display labels for copper layers on the top
        m_view->SetTopLayer( GetNetnameLayer( aLayer ) );
    }

    m_view->EnableTopLayer( true );
    m_view->UpdateAllLayersOrder();
}


void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( const BOARD* aBoard )
{
    // Load layer & elements visibility settings
    for( LAYER_NUM i = 0; i < PCB_LAYER_ID_COUNT; ++i )
        m_view->SetLayerVisible( i, aBoard->IsLayerVisible( PCB_LAYER_ID( i ) ) );

    for( GAL_LAYER_ID i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; ++i )
        m_view->SetLayerVisible( i, aBoard->IsElementVisible( i ) );

    // Always enable netname layers, as their visibility is controlled by layer dependencies
    for( LAYER_NUM i = NETNAMES_LAYER_ID_START; i < NETNAMES_LAYER_ID_END; ++i )
        m_view->SetLayerVisible( i, true );

    // Enable some layers that are GAL specific
    m_view->SetLayerVisible( LAYER_PADS_PLATEDHOLES, true );
    m_view->SetLayerVisible( LAYER_VIAS_HOLES, true );
    m_view->SetLayerVisible( LAYER_GP_OVERLAY, true );
    m_view->SetLayerVisible( LAYER_SELECT_OVERLAY, true );
    m_view->SetLayerVisible( LAYER_RATSNEST, true );
}


void PCB_DRAW_PANEL_GAL::GetMsgPanelInfo( EDA_UNITS_T aUnits, std::vector<MSG_PANEL_ITEM>& aList )
{
    BOARD* board = static_cast<PCB_BASE_FRAME*>( m_parent )->GetBoard();
    wxString txt;
    int viasCount = 0;
    int trackSegmentsCount = 0;

    for( auto item : board->Tracks() )
    {
        if( item->Type() == PCB_VIA_T )
            viasCount++;
        else
            trackSegmentsCount++;
    }

    txt.Printf( wxT( "%d" ), board->GetPadCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Pads" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), viasCount );
    aList.push_back( MSG_PANEL_ITEM( _( "Vias" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), trackSegmentsCount );
    aList.push_back( MSG_PANEL_ITEM( _( "Track Segments" ), txt, DARKGREEN ) );

    txt.Printf( wxT( "%d" ), board->GetNodesCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Nodes" ), txt, DARKCYAN ) );

    txt.Printf( wxT( "%d" ), board->GetNetCount() - 1 /* don't include "No Net" in count */ );
    aList.push_back( MSG_PANEL_ITEM( _( "Nets" ), txt, RED ) );

    txt.Printf( wxT( "%d" ), board->GetConnectivity()->GetUnconnectedCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Unrouted" ), txt, BLUE ) );
}


void PCB_DRAW_PANEL_GAL::OnShow()
{
    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );

    try
    {
        // Check if the current rendering backend can be properly initialized
        m_view->UpdateItems();
    }
    catch( const std::runtime_error& e )
    {
        // Fallback to software renderer
        DisplayError( frame, e.what() );
        SwitchBackend( GAL_TYPE_CAIRO );

        if( frame )
            frame->ActivateGalCanvas();
    }

    if( frame )
    {
        SetTopLayer( frame->GetActiveLayer() );
        PCB_DISPLAY_OPTIONS* displ_opts = (PCB_DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        KIGFX::PAINTER* painter = m_view->GetPainter();
        auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( painter->GetSettings() );
        settings->LoadDisplayOptions( displ_opts, frame->ShowPageLimits() );
    }
}


void PCB_DRAW_PANEL_GAL::setDefaultLayerOrder()
{
    for( LAYER_NUM i = 0; (unsigned) i < sizeof( GAL_LAYER_ORDER ) / sizeof( LAYER_NUM ); ++i )
    {
        LAYER_NUM layer = GAL_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        m_view->SetLayerOrder( layer, i );
    }
}


bool PCB_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    bool rv = EDA_DRAW_PANEL_GAL::SwitchBackend( aGalType );
    setDefaultLayerDeps();
    m_gal->SetWorldUnitLength( 1e-9 /* 1 nm */ / 0.0254 /* 1 inch in meters */ );
    return rv;
}


void PCB_DRAW_PANEL_GAL::RedrawRatsnest()
{
    if( m_ratsnest )
        m_view->Update( m_ratsnest.get() );
}


BOX2I PCB_DRAW_PANEL_GAL::GetDefaultViewBBox() const
{
    if( m_worksheet && m_view->IsLayerVisible( LAYER_WORKSHEET ) )
        return m_worksheet->ViewBBox();

    return BOX2I();
}


void PCB_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    // caching makes no sense for Cairo and other software renderers
    auto target = m_backend == GAL_TYPE_OPENGL ? KIGFX::TARGET_CACHED : KIGFX::TARGET_NONCACHED;

    for( int i = 0; i < KIGFX::VIEW::VIEW_MAX_LAYERS; i++ )
        m_view->SetLayerTarget( i, target );

    for( LAYER_NUM i = 0; (unsigned) i < sizeof( GAL_LAYER_ORDER ) / sizeof( LAYER_NUM ); ++i )
    {
        LAYER_NUM layer = GAL_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        // Set layer display dependencies & targets
        if( IsCopperLayer( layer ) )
            m_view->SetRequired( GetNetnameLayer( layer ), layer );
        else if( IsNetnameLayer( layer ) )
            m_view->SetLayerDisplayOnly( layer );
    }

    m_view->SetLayerTarget( LAYER_ANCHOR, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_ANCHOR );

    // Some more required layers settings
    m_view->SetRequired( LAYER_VIAS_HOLES, LAYER_VIA_THROUGH );
    m_view->SetRequired( LAYER_VIAS_NETNAMES, LAYER_VIA_THROUGH );
    m_view->SetRequired( LAYER_PADS_PLATEDHOLES, LAYER_PADS_TH );
    m_view->SetRequired( LAYER_NON_PLATEDHOLES, LAYER_PADS_TH );
    m_view->SetRequired( LAYER_PADS_NETNAMES, LAYER_PADS_TH );

    // Front modules
    m_view->SetRequired( LAYER_PAD_FR, F_Cu );
    m_view->SetRequired( LAYER_MOD_TEXT_FR, LAYER_MOD_FR );
    m_view->SetRequired( LAYER_PAD_FR_NETNAMES, LAYER_PAD_FR );

    // Back modules
    m_view->SetRequired( LAYER_PAD_BK, B_Cu );
    m_view->SetRequired( LAYER_MOD_TEXT_BK, LAYER_MOD_BK );
    m_view->SetRequired( LAYER_PAD_BK_NETNAMES, LAYER_PAD_BK );

    m_view->SetLayerTarget( LAYER_SELECT_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_SELECT_OVERLAY ) ;
    m_view->SetLayerTarget( LAYER_GP_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;
    m_view->SetLayerTarget( LAYER_RATSNEST, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_RATSNEST );

    m_view->SetLayerTarget( LAYER_WORKSHEET, KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( LAYER_WORKSHEET ) ;
    m_view->SetLayerDisplayOnly( LAYER_GRID );
    m_view->SetLayerDisplayOnly( LAYER_DRC );
}


KIGFX::PCB_VIEW* PCB_DRAW_PANEL_GAL::GetView() const
{
    return static_cast<KIGFX::PCB_VIEW*>( m_view );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>
#include <worksheet_viewitem.h>
#include <ratsnest_viewitem.h>
#include <ratsnest_data.h>

#include <class_colors_design_settings.h>
#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <wxBasePcbFrame.h>

#include <boost/bind.hpp>

const LAYER_NUM GAL_LAYER_ORDER[] =
{
    ITEM_GAL_LAYER( GP_OVERLAY ),
    ITEM_GAL_LAYER( DRC_VISIBLE ),
    NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE ),
    Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts,

    ITEM_GAL_LAYER( MOD_TEXT_FR_VISIBLE ),
    ITEM_GAL_LAYER( MOD_REFERENCES_VISIBLE), ITEM_GAL_LAYER( MOD_VALUES_VISIBLE ),

    ITEM_GAL_LAYER( RATSNEST_VISIBLE ), ITEM_GAL_LAYER( ANCHOR_VISIBLE ),
    ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ), ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ),
    ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE ), ITEM_GAL_LAYER( VIA_BBLIND_VISIBLE ),
    ITEM_GAL_LAYER( VIA_MICROVIA_VISIBLE ), ITEM_GAL_LAYER( PADS_VISIBLE ),

    NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE ), ITEM_GAL_LAYER( PAD_FR_VISIBLE ),
    NETNAMES_GAL_LAYER( F_Cu ), F_Cu, F_Mask, F_SilkS, F_Paste, F_Adhes,

    NETNAMES_GAL_LAYER( In1_Cu ),   In1_Cu,
    NETNAMES_GAL_LAYER( In2_Cu ),   In2_Cu,
    NETNAMES_GAL_LAYER( In3_Cu ),   In3_Cu,
    NETNAMES_GAL_LAYER( In4_Cu ),   In4_Cu,
    NETNAMES_GAL_LAYER( In5_Cu ),   In5_Cu,
    NETNAMES_GAL_LAYER( In6_Cu ),   In6_Cu,
    NETNAMES_GAL_LAYER( In7_Cu ),   In7_Cu,
    NETNAMES_GAL_LAYER( In8_Cu ),   In8_Cu,
    NETNAMES_GAL_LAYER( In9_Cu ),   In9_Cu,
    NETNAMES_GAL_LAYER( In10_Cu ),  In10_Cu,
    NETNAMES_GAL_LAYER( In11_Cu ),  In11_Cu,
    NETNAMES_GAL_LAYER( In12_Cu ),  In12_Cu,
    NETNAMES_GAL_LAYER( In13_Cu ),  In13_Cu,
    NETNAMES_GAL_LAYER( In14_Cu ),  In14_Cu,
    NETNAMES_GAL_LAYER( In15_Cu ),  In15_Cu,
    NETNAMES_GAL_LAYER( In16_Cu ),  In16_Cu,
    NETNAMES_GAL_LAYER( In17_Cu ),  In17_Cu,
    NETNAMES_GAL_LAYER( In18_Cu ),  In18_Cu,
    NETNAMES_GAL_LAYER( In19_Cu ),  In19_Cu,
    NETNAMES_GAL_LAYER( In20_Cu ),  In20_Cu,
    NETNAMES_GAL_LAYER( In21_Cu ),  In21_Cu,
    NETNAMES_GAL_LAYER( In22_Cu ),  In22_Cu,
    NETNAMES_GAL_LAYER( In23_Cu ),  In23_Cu,
    NETNAMES_GAL_LAYER( In24_Cu ),  In24_Cu,
    NETNAMES_GAL_LAYER( In25_Cu ),  In25_Cu,
    NETNAMES_GAL_LAYER( In26_Cu ),  In26_Cu,
    NETNAMES_GAL_LAYER( In27_Cu ),  In27_Cu,
    NETNAMES_GAL_LAYER( In28_Cu ),  In28_Cu,
    NETNAMES_GAL_LAYER( In29_Cu ),  In29_Cu,
    NETNAMES_GAL_LAYER( In30_Cu ),  In30_Cu,

    NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE ), ITEM_GAL_LAYER( PAD_BK_VISIBLE ),
    NETNAMES_GAL_LAYER( B_Cu ), B_Cu, B_Mask, B_Adhes, B_Paste, B_SilkS,

    ITEM_GAL_LAYER( MOD_TEXT_BK_VISIBLE ),
    ITEM_GAL_LAYER( WORKSHEET )
};


PCB_DRAW_PANEL_GAL::PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        GAL_TYPE aGalType ) :
EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aGalType )
{
    m_worksheet = NULL;
    m_ratsnest = NULL;

    setDefaultLayerOrder();
    setDefaultLayerDeps();

    // Load display options (such as filled/outline display of items).
    // Can be made only if the parent window is an EDA_DRAW_FRAME (or a derived class)
    // which is not always the case (namely when it is used from a wxDialog like the pad editor)
    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( aParentWindow );

    if( frame )
    {
        DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() )->LoadDisplayOptions( displ_opts );
    }
}


PCB_DRAW_PANEL_GAL::~PCB_DRAW_PANEL_GAL()
{
    delete m_worksheet;
    delete m_ratsnest;
}


void PCB_DRAW_PANEL_GAL::DisplayBoard( const BOARD* aBoard )
{
    m_view->Clear();

    // Load zones
    for( int i = 0; i < aBoard->GetAreaCount(); ++i )
        m_view->Add( (KIGFX::VIEW_ITEM*) ( aBoard->GetArea( i ) ) );

    // Load drawings
    for( BOARD_ITEM* drawing = aBoard->m_Drawings; drawing; drawing = drawing->Next() )
        m_view->Add( drawing );

    // Load tracks
    for( TRACK* track = aBoard->m_Track; track; track = track->Next() )
        m_view->Add( track );

    // Load modules and its additional elements
    for( MODULE* module = aBoard->m_Modules; module; module = module->Next() )
    {
        module->RunOnChildren( boost::bind( &KIGFX::VIEW::Add, m_view, _1 ) );
        m_view->Add( module );
    }

    // Segzones (equivalent of ZONE_CONTAINER for legacy boards)
    for( SEGZONE* zone = aBoard->m_Zone; zone; zone = zone->Next() )
        m_view->Add( zone );

    // Ratsnest
    if( m_ratsnest )
    {
        m_view->Remove( m_ratsnest );
        delete m_ratsnest;
    }

    m_ratsnest = new KIGFX::RATSNEST_VIEWITEM( aBoard->GetRatsnest() );
    m_view->Add( m_ratsnest );

    // Display settings
    UseColorScheme( aBoard->GetColorsSettings() );

    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );

    if( frame )
    {
        SetTopLayer( frame->GetActiveLayer() );
        DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_view->GetPainter()->GetSettings() )->LoadDisplayOptions( displ_opts );
    }

    m_view->RecacheAllItems( true );
}


void PCB_DRAW_PANEL_GAL::SetWorksheet( KIGFX::WORKSHEET_VIEWITEM* aWorksheet )
{
    if( m_worksheet )
    {
        m_view->Remove( m_worksheet );
        delete m_worksheet;
    }

    m_worksheet = aWorksheet;
    m_view->Add( m_worksheet );
}


void PCB_DRAW_PANEL_GAL::UseColorScheme( const COLORS_DESIGN_SETTINGS* aSettings )
{
    KIGFX::PCB_RENDER_SETTINGS* rs;
    rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );
    rs->ImportLegacyColors( aSettings );
}


void PCB_DRAW_PANEL_GAL::SetHighContrastLayer( LAYER_ID aLayer )
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
                GetNetnameLayer( aLayer ), ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE ),
                ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ), ITEM_GAL_LAYER( PADS_VISIBLE ),
                ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ), NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE ),
                ITEM_GAL_LAYER( GP_OVERLAY ), ITEM_GAL_LAYER( RATSNEST_VISIBLE )
        };

        for( unsigned int i = 0; i < sizeof( layers ) / sizeof( LAYER_NUM ); ++i )
            rSettings->SetActiveLayer( layers[i] );

        // Pads should be shown too
        if( aLayer == B_Cu )
        {
            rSettings->SetActiveLayer( ITEM_GAL_LAYER( PAD_BK_VISIBLE ) );
            rSettings->SetActiveLayer( ITEM_GAL_LAYER( MOD_BK_VISIBLE ) );
            rSettings->SetActiveLayer( NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE ) );
        }
        else if( aLayer == F_Cu )
        {
            rSettings->SetActiveLayer( ITEM_GAL_LAYER( PAD_FR_VISIBLE ) );
            rSettings->SetActiveLayer( ITEM_GAL_LAYER( MOD_FR_VISIBLE ) );
            rSettings->SetActiveLayer( NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE ) );
        }
    }

    m_view->UpdateAllLayersColor();
}


void PCB_DRAW_PANEL_GAL::SetTopLayer( LAYER_ID aLayer )
{
    m_view->ClearTopLayers();
    setDefaultLayerOrder();
    m_view->SetTopLayer( aLayer );

    if( IsCopperLayer( aLayer ) )
    {
        // Bring some other layers to the front in case of copper layers and make them colored
        // fixme do not like the idea of storing the list of layers here,
        // should be done in some other way I guess..
        const LAYER_NUM layers[] = {
                GetNetnameLayer( aLayer ), ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE ),
                ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ), ITEM_GAL_LAYER( PADS_VISIBLE ),
                ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ), NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE ),
                ITEM_GAL_LAYER( GP_OVERLAY ), ITEM_GAL_LAYER( RATSNEST_VISIBLE ), Dwgs_User,
                ITEM_GAL_LAYER( DRC_VISIBLE )
        };

        for( unsigned int i = 0; i < sizeof( layers ) / sizeof( LAYER_NUM ); ++i )
            m_view->SetTopLayer( layers[i] );
    }

    const LAYER_NUM frontLayers[] = {
        F_Cu, F_Adhes, F_Paste, F_SilkS, F_Mask, F_CrtYd, F_Fab,
        ITEM_GAL_LAYER( PAD_FR_VISIBLE ), NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE ), -1
    };

    const LAYER_NUM backLayers[] = {
        B_Cu, B_Adhes, B_Paste, B_SilkS, B_Mask, B_CrtYd, B_Fab,
        ITEM_GAL_LAYER( PAD_BK_VISIBLE ), NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE ), -1
    };

    const LAYER_NUM* extraLayers = NULL;

    // Bring a few more extra layers to the top depending on the selected board side
    if( IsFrontLayer( aLayer ) )
        extraLayers = frontLayers;
    else if( IsBackLayer( aLayer ) )
        extraLayers = backLayers;

    if( extraLayers )
    {
        int topOrder = std::numeric_limits<int>::max();
        const LAYER_NUM* l = extraLayers;

        while( *l >= 0 )
        {
            m_view->SetTopLayer( *l );
            topOrder = std::min( topOrder, m_view->GetLayerOrder( *l ) );
            ++l;
        }

        // Do not cover pads with copper
        if( !IsCopperLayer( aLayer ) )
            m_view->SetLayerOrder( aLayer, topOrder - 1 );

    }

    m_view->UpdateAllLayersOrder();
}


void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( const BOARD* aBoard )
{
    // Load layer & elements visibility settings
    for( LAYER_NUM i = 0; i < LAYER_ID_COUNT; ++i )
    {
        m_view->SetLayerVisible( i, aBoard->IsLayerVisible( LAYER_ID( i ) ) );

        // Synchronize netname layers as well
        if( IsCopperLayer( i ) )
            m_view->SetLayerVisible( GetNetnameLayer( i ), aBoard->IsLayerVisible( LAYER_ID( i ) ) );
    }

    for( LAYER_NUM i = 0; i < END_PCB_VISIBLE_LIST; ++i )
    {
        m_view->SetLayerVisible( ITEM_GAL_LAYER( i ), aBoard->IsElementVisible( i ) );
    }

    // Enable some layers that are GAL specific
    m_view->SetLayerVisible( ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ), true );
    m_view->SetLayerVisible( ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ), true );
    m_view->SetLayerVisible( ITEM_GAL_LAYER( WORKSHEET ), true );
    m_view->SetLayerVisible( ITEM_GAL_LAYER( GP_OVERLAY ), true );
}


void PCB_DRAW_PANEL_GAL::GetMsgPanelInfo( std::vector<MSG_PANEL_ITEM>& aList )
{
    BOARD* board = static_cast<PCB_BASE_FRAME*>( m_parent )->GetBoard();
    wxString txt;
    int viasCount = 0;
    int trackSegmentsCount = 0;

    for( const BOARD_ITEM* item = board->m_Track; item; item = item->Next() )
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

    txt.Printf( wxT( "%d" ), board->GetNetCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Nets" ), txt, RED ) );

    txt.Printf( wxT( "%d" ), board->GetRatsnest()->GetUnconnectedCount() );
    aList.push_back( MSG_PANEL_ITEM( _( "Unconnected" ), txt, BLUE ) );
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


void PCB_DRAW_PANEL_GAL::setDefaultLayerDeps()
{
    for( LAYER_NUM i = 0; (unsigned) i < sizeof( GAL_LAYER_ORDER ) / sizeof( LAYER_NUM ); ++i )
    {
        LAYER_NUM layer = GAL_LAYER_ORDER[i];
        wxASSERT( layer < KIGFX::VIEW::VIEW_MAX_LAYERS );

        if( IsCopperLayer( layer ) )
        {
            // Copper layers are required for netname layers
            m_view->SetRequired( GetNetnameLayer( layer ), layer );
            m_view->SetLayerTarget( layer, KIGFX::TARGET_CACHED );
        }
        else if( IsNetnameLayer( layer ) )
        {
            // Netnames are drawn only when scale is sufficient (level of details)
            // so there is no point in caching them
            m_view->SetLayerTarget( layer, KIGFX::TARGET_NONCACHED );
            m_view->SetLayerDisplayOnly( layer );
        }
    }

    m_view->SetLayerTarget( ITEM_GAL_LAYER( ANCHOR_VISIBLE ), KIGFX::TARGET_NONCACHED );
    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( ANCHOR_VISIBLE ) );

    // Some more required layers settings
    m_view->SetRequired( ITEM_GAL_LAYER( VIAS_HOLES_VISIBLE ), ITEM_GAL_LAYER( VIA_THROUGH_VISIBLE ) );
    m_view->SetRequired( ITEM_GAL_LAYER( PADS_HOLES_VISIBLE ), ITEM_GAL_LAYER( PADS_VISIBLE ) );
    m_view->SetRequired( NETNAMES_GAL_LAYER( PADS_NETNAMES_VISIBLE ), ITEM_GAL_LAYER( PADS_VISIBLE ) );

    // Front modules
    m_view->SetRequired( ITEM_GAL_LAYER( PAD_FR_VISIBLE ), ITEM_GAL_LAYER( MOD_FR_VISIBLE ) );
    m_view->SetRequired( ITEM_GAL_LAYER( MOD_TEXT_FR_VISIBLE ), ITEM_GAL_LAYER( MOD_FR_VISIBLE ) );
    m_view->SetRequired( NETNAMES_GAL_LAYER( PAD_FR_NETNAMES_VISIBLE ), ITEM_GAL_LAYER( PAD_FR_VISIBLE ) );
    m_view->SetRequired( F_Adhes, ITEM_GAL_LAYER( PAD_FR_VISIBLE ) );
    m_view->SetRequired( F_Paste, ITEM_GAL_LAYER( PAD_FR_VISIBLE ) );
    m_view->SetRequired( F_Mask, ITEM_GAL_LAYER( PAD_FR_VISIBLE ) );
    m_view->SetRequired( F_CrtYd, ITEM_GAL_LAYER( MOD_FR_VISIBLE ) );
    m_view->SetRequired( F_Fab, ITEM_GAL_LAYER( MOD_FR_VISIBLE ) );

    // Back modules
    m_view->SetRequired( ITEM_GAL_LAYER( PAD_BK_VISIBLE ), ITEM_GAL_LAYER( MOD_BK_VISIBLE ) );
    m_view->SetRequired( ITEM_GAL_LAYER( MOD_TEXT_BK_VISIBLE ), ITEM_GAL_LAYER( MOD_BK_VISIBLE ) );
    m_view->SetRequired( NETNAMES_GAL_LAYER( PAD_BK_NETNAMES_VISIBLE ), ITEM_GAL_LAYER( PAD_BK_VISIBLE ) );
    m_view->SetRequired( B_Adhes, ITEM_GAL_LAYER( PAD_BK_VISIBLE ) );
    m_view->SetRequired( B_Paste, ITEM_GAL_LAYER( PAD_BK_VISIBLE ) );
    m_view->SetRequired( B_Mask, ITEM_GAL_LAYER( PAD_BK_VISIBLE ) );
    m_view->SetRequired( B_CrtYd, ITEM_GAL_LAYER( MOD_BK_VISIBLE ) );
    m_view->SetRequired( B_Fab, ITEM_GAL_LAYER( MOD_BK_VISIBLE ) );

    m_view->SetLayerTarget( ITEM_GAL_LAYER( GP_OVERLAY ), KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( GP_OVERLAY ) );
    m_view->SetLayerTarget( ITEM_GAL_LAYER( RATSNEST_VISIBLE ), KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( RATSNEST_VISIBLE ) );

    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( WORKSHEET ) );
    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( GRID_VISIBLE ) );
    m_view->SetLayerDisplayOnly( ITEM_GAL_LAYER( DRC_VISIBLE ) );
}

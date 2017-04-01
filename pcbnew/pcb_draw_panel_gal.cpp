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

#include <functional>
using namespace std::placeholders;

const LAYER_NUM GAL_LAYER_ORDER[] =
{
    LAYER_GP_OVERLAY ,
    LAYER_DRC,
    LAYER_PADS_NETNAMES,
    Dwgs_User, Cmts_User, Eco1_User, Eco2_User, Edge_Cuts,

    LAYER_MOD_TEXT_FR,
    LAYER_MOD_REFERENCES, LAYER_MOD_VALUES,

    LAYER_RATSNEST, LAYER_ANCHOR,
    LAYER_VIAS_HOLES, LAYER_PADS_HOLES,
    LAYER_VIA_THROUGH, LAYER_VIA_BBLIND,
    LAYER_VIA_MICROVIA, LAYER_PADS,

    LAYER_PAD_FR_NETNAMES, LAYER_PAD_FR,
    NETNAMES_LAYER_INDEX( F_Cu ), F_Cu, F_Mask, F_SilkS, F_Paste, F_Adhes,

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
    NETNAMES_LAYER_INDEX( B_Cu ), B_Cu, B_Mask, B_Adhes, B_Paste, B_SilkS,

    LAYER_MOD_TEXT_BK,
    LAYER_WORKSHEET
};


PCB_DRAW_PANEL_GAL::PCB_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
EDA_DRAW_PANEL_GAL( aParentWindow, aWindowId, aPosition, aSize, aOptions, aGalType )
{
    setDefaultLayerOrder();
    setDefaultLayerDeps();

    m_painter = new KIGFX::PCB_PAINTER( m_gal );
    m_view->SetPainter( m_painter );

    // Load display options (such as filled/outline display of items).
    // Can be made only if the parent window is an EDA_DRAW_FRAME (or a derived class)
    // which is not always the case (namely when it is used from a wxDialog like the pad editor)
    EDA_DRAW_FRAME* frame = GetParentEDAFrame();

    if( frame )
    {
        DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() )->LoadDisplayOptions( displ_opts );
    }
}


PCB_DRAW_PANEL_GAL::~PCB_DRAW_PANEL_GAL()
{
    delete m_painter;
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
        module->RunOnChildren( std::bind( &KIGFX::VIEW::Add, m_view, _1, -1 ) );
        m_view->Add( module );
    }

    // Segzones (equivalent of ZONE_CONTAINER for legacy boards)
    for( SEGZONE* zone = aBoard->m_Zone; zone; zone = zone->Next() )
        m_view->Add( zone );

    // Ratsnest
    m_ratsnest.reset( new KIGFX::RATSNEST_VIEWITEM( aBoard->GetRatsnest() ) );
    m_view->Add( m_ratsnest.get() );

    // Display settings
    UseColorScheme( aBoard->GetColorsSettings() );
}


void PCB_DRAW_PANEL_GAL::SetWorksheet( KIGFX::WORKSHEET_VIEWITEM* aWorksheet )
{
    m_worksheet.reset( aWorksheet );
    m_view->Add( m_worksheet.get() );
}


void PCB_DRAW_PANEL_GAL::UseColorScheme( const COLORS_DESIGN_SETTINGS* aSettings )
{
    KIGFX::PCB_RENDER_SETTINGS* rs;
    rs = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_view->GetPainter()->GetSettings() );
    rs->ImportLegacyColors( aSettings );
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
                GetNetnameLayer( aLayer ), LAYER_VIA_THROUGH,
                LAYER_VIAS_HOLES, LAYER_PADS,
                LAYER_PADS_HOLES, LAYER_PADS_NETNAMES,
                LAYER_GP_OVERLAY, LAYER_RATSNEST
        };

        for( unsigned int i = 0; i < sizeof( layers ) / sizeof( LAYER_NUM ); ++i )
            rSettings->SetActiveLayer( layers[i] );

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
    const LAYER_NUM layers[] = {
            LAYER_VIA_THROUGH,
            LAYER_VIAS_HOLES, LAYER_PADS,
            LAYER_PADS_HOLES, LAYER_PADS_NETNAMES,
            LAYER_GP_OVERLAY, LAYER_RATSNEST, Dwgs_User,
            LAYER_DRC
    };

    for( unsigned int i = 0; i < sizeof( layers ) / sizeof( LAYER_NUM ); ++i )
        m_view->SetTopLayer( layers[i] );

    // Extra layers that are brought to the top if a F.* or B.* is selected
    const LAYER_NUM frontLayers[] = {
        F_Cu, F_Adhes, F_Paste, F_SilkS, F_Mask, F_CrtYd, F_Fab, LAYER_PAD_FR,
        LAYER_PAD_FR_NETNAMES, NETNAMES_LAYER_INDEX( F_Cu ), -1
    };

    const LAYER_NUM backLayers[] = {
        B_Cu, B_Adhes, B_Paste, B_SilkS, B_Mask, B_CrtYd, B_Fab, LAYER_PAD_BK,
        LAYER_PAD_BK_NETNAMES, NETNAMES_LAYER_INDEX( B_Cu ), -1
    };

    const LAYER_NUM* extraLayers = NULL;

    // Bring a few more extra layers to the top depending on the selected board side
    if( IsFrontLayer( aLayer ) )
        extraLayers = frontLayers;
    else if( IsBackLayer( aLayer ) )
        extraLayers = backLayers;

    if( extraLayers )
    {
        const LAYER_NUM* l = extraLayers;

        while( *l >= 0 )
            m_view->SetTopLayer( *l++ );

        // Move the active layer to the top
        if( !IsCopperLayer( aLayer ) )
            m_view->SetLayerOrder( aLayer, m_view->GetLayerOrder( GAL_LAYER_ORDER[0] ) );
    }
    else if( IsCopperLayer( aLayer ) )
    {
        // Display labels for copper layers on the top
        m_view->SetTopLayer( GetNetnameLayer( aLayer ) );
    }

    m_view->UpdateAllLayersOrder();
}


void PCB_DRAW_PANEL_GAL::SyncLayersVisibility( const BOARD* aBoard )
{
    // Load layer & elements visibility settings
    for( LAYER_NUM i = 0; i < PCB_LAYER_ID_COUNT; ++i )
    {
        m_view->SetLayerVisible( i, aBoard->IsLayerVisible( PCB_LAYER_ID( i ) ) );

        // Synchronize netname layers as well
        if( IsCopperLayer( i ) )
            m_view->SetLayerVisible( GetNetnameLayer( i ), aBoard->IsLayerVisible( PCB_LAYER_ID( i ) ) );
    }

    for( GAL_LAYER_ID i = GAL_LAYER_ID_START; i < GAL_LAYER_ID_END; ++i )
    {
        m_view->SetLayerVisible( i, aBoard->IsElementVisible( i ) );
    }

    // Enable some layers that are GAL specific
    m_view->SetLayerVisible( LAYER_PADS_HOLES, true );
    m_view->SetLayerVisible( LAYER_VIAS_HOLES, true );
    m_view->SetLayerVisible( LAYER_WORKSHEET, true );
    m_view->SetLayerVisible( LAYER_GP_OVERLAY, true );
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


void PCB_DRAW_PANEL_GAL::OnShow()
{
    PCB_BASE_FRAME* frame = dynamic_cast<PCB_BASE_FRAME*>( GetParent() );

    if( frame )
    {
        SetTopLayer( frame->GetActiveLayer() );
        DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*) frame->GetDisplayOptions();
        static_cast<KIGFX::PCB_RENDER_SETTINGS*>(
            m_view->GetPainter()->GetSettings() )->LoadDisplayOptions( displ_opts );
    }

    m_view->RecacheAllItems();
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
    return rv;
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
    m_view->SetRequired( LAYER_PADS_HOLES, LAYER_PADS );
    m_view->SetRequired( LAYER_PADS_NETNAMES, LAYER_PADS );

    // Front modules
    m_view->SetRequired( LAYER_PAD_FR, LAYER_MOD_FR );
    m_view->SetRequired( LAYER_MOD_TEXT_FR, LAYER_MOD_FR );
    m_view->SetRequired( LAYER_PAD_FR_NETNAMES, LAYER_PAD_FR );
    m_view->SetRequired( F_Adhes, LAYER_PAD_FR );
    m_view->SetRequired( F_Paste, LAYER_PAD_FR );
    m_view->SetRequired( F_Mask, LAYER_PAD_FR );
    m_view->SetRequired( F_CrtYd, LAYER_MOD_FR );
    m_view->SetRequired( F_Fab, LAYER_MOD_FR );
    m_view->SetRequired( F_SilkS, LAYER_MOD_FR );

    // Back modules
    m_view->SetRequired( LAYER_PAD_BK, LAYER_MOD_BK );
    m_view->SetRequired( LAYER_MOD_TEXT_BK, LAYER_MOD_BK );
    m_view->SetRequired( LAYER_PAD_BK_NETNAMES, LAYER_PAD_BK );
    m_view->SetRequired( B_Adhes, LAYER_PAD_BK );
    m_view->SetRequired( B_Paste, LAYER_PAD_BK );
    m_view->SetRequired( B_Mask, LAYER_PAD_BK );
    m_view->SetRequired( B_CrtYd, LAYER_MOD_BK );
    m_view->SetRequired( B_Fab, LAYER_MOD_BK );
    m_view->SetRequired( B_SilkS, LAYER_MOD_BK );

    m_view->SetLayerTarget( LAYER_GP_OVERLAY , KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_GP_OVERLAY ) ;
    m_view->SetLayerTarget( LAYER_RATSNEST, KIGFX::TARGET_OVERLAY );
    m_view->SetLayerDisplayOnly( LAYER_RATSNEST );

    m_view->SetLayerDisplayOnly( LAYER_WORKSHEET ) ;
    m_view->SetLayerDisplayOnly( LAYER_GRID );
    m_view->SetLayerDisplayOnly( LAYER_DRC );
}

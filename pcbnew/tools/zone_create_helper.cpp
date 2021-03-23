/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/spinlock.h>
#include <connectivity/connectivity_data.h>
#include <tools/zone_create_helper.h>
#include <tool/tool_manager.h>
#include <zone.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <board_commit.h>
#include <pcb_painter.h>
#include <pcbnew_settings.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <zone_filler.h>

ZONE_CREATE_HELPER::ZONE_CREATE_HELPER( DRAWING_TOOL& aTool, PARAMS& aParams ):
        m_tool( aTool ),
        m_params( aParams ),
        m_parentView( *aTool.getView() )
{
    m_parentView.Add( &m_previewItem );
}


ZONE_CREATE_HELPER::~ZONE_CREATE_HELPER()
{
    // remove the preview from the view
    m_parentView.SetVisible( &m_previewItem, false );
    m_parentView.Remove( &m_previewItem );
}


std::unique_ptr<ZONE> ZONE_CREATE_HELPER::createNewZone( bool aKeepout )
{
    PCB_BASE_EDIT_FRAME*  frame = m_tool.getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD*                board = frame->GetBoard();
    BOARD_ITEM_CONTAINER* parent = m_tool.m_frame->GetModel();
    KIGFX::VIEW_CONTROLS* controls = m_tool.GetManager()->GetViewControls();
    std::set<int>         highlightedNets = board->GetHighLightNetCodes();

    // Get the current default settings for zones
    ZONE_SETTINGS         zoneInfo = frame->GetZoneSettings();
    zoneInfo.m_Layers.reset().set( m_params.m_layer );  // TODO(JE) multilayer defaults?
    zoneInfo.m_NetcodeSelection = highlightedNets.empty() ? -1 : *highlightedNets.begin();
    zoneInfo.SetIsRuleArea( m_params.m_keepout );
    zoneInfo.m_Zone_45_Only = ( m_params.m_leaderMode == POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45 );

    // If we don't have a net from highlighing, maybe we can get one from the selection
    PCB_SELECTION_TOOL* selectionTool = m_tool.GetManager()->GetTool<PCB_SELECTION_TOOL>();

    if( selectionTool && !selectionTool->GetSelection().Empty()
            && zoneInfo.m_NetcodeSelection == -1 )
    {
        EDA_ITEM* item = *selectionTool->GetSelection().GetItems().begin();

        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            zoneInfo.m_NetcodeSelection = bci->GetNetCode();
    }

    if( m_params.m_mode != ZONE_MODE::GRAPHIC_POLYGON )
    {
        // Get the current default settings for zones

        // Show options dialog
        int dialogResult;

        if( m_params.m_keepout )
            dialogResult = InvokeRuleAreaEditor( frame, &zoneInfo );
        else
        {
            // TODO(JE) combine these dialogs?
            if( ( zoneInfo.m_Layers & LSET::AllCuMask() ).any() )
                dialogResult = InvokeCopperZonesEditor( frame, &zoneInfo );
            else
                dialogResult = InvokeNonCopperZonesEditor( frame, &zoneInfo );
        }

        if( dialogResult == wxID_CANCEL )
            return nullptr;

        controls->WarpCursor( controls->GetCursorPosition(), true );
    }

    // The new zone is a ZONE if created in the board editor and a FP_ZONE if created in the
    // footprint editor
    wxASSERT( !m_tool.m_isFootprintEditor || ( parent->Type() == PCB_FOOTPRINT_T ) );

    std::unique_ptr<ZONE> newZone = m_tool.m_isFootprintEditor ?
                                                std::make_unique<FP_ZONE>( parent ) :
                                                std::make_unique<ZONE>( parent );

    // Apply the selected settings
    zoneInfo.ExportSetting( *newZone );

    return newZone;
}


std::unique_ptr<ZONE> ZONE_CREATE_HELPER::createZoneFromExisting( const ZONE& aSrcZone )
{
    BOARD* board = m_tool.getModel<BOARD>();

    std::unique_ptr<ZONE> newZone = std::make_unique<ZONE>( board );

    ZONE_SETTINGS zoneSettings;
    zoneSettings << aSrcZone;

    zoneSettings.ExportSetting( *newZone );

    return newZone;
}


void ZONE_CREATE_HELPER::performZoneCutout( ZONE& aZone, const ZONE& aCutout )
{
    BOARD_COMMIT commit( &m_tool );
    std::vector<ZONE*> newZones;

    // Clear the selection before removing the old zone
    auto toolMgr = m_tool.GetManager();
    toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    SHAPE_POLY_SET originalOutline( *aZone.Outline() );
    originalOutline.BooleanSubtract( *aCutout.Outline(), SHAPE_POLY_SET::PM_FAST );

    // After substracting the hole, originalOutline can have more than one
    // main outline.
    // But a zone can have only one main outline, so create as many zones as
    // originalOutline contains main outlines:
    for( int outline = 0; outline < originalOutline.OutlineCount(); outline++ )
    {
        auto newZoneOutline = new SHAPE_POLY_SET;
        newZoneOutline->AddOutline( originalOutline.Outline( outline ) );

        // Add holes (if any) to thez new zone outline:
        for (int hole = 0; hole < originalOutline.HoleCount( outline ) ; hole++ )
            newZoneOutline->AddHole( originalOutline.CHole( outline, hole ) );

        auto newZone = new ZONE( aZone );
        newZone->SetOutline( newZoneOutline );
        newZone->SetLocalFlags( 1 );
        newZone->HatchBorder();
        newZone->UnFill();
        newZones.push_back( newZone );
        commit.Add( newZone );
    }

    commit.Remove( &aZone );

    // TODO Refill zones when KiCad supports auto re-fill

    commit.Push( _( "Add a zone cutout" ) );

    // Select the new zone and set it as the source for the next cutout
    if( newZones.empty() )
    {
        m_params.m_sourceZone = nullptr;
    }
    else
    {
        m_params.m_sourceZone = newZones[0];
        toolMgr->RunAction( PCB_ACTIONS::selectItem, true, newZones[0] );
    }

}


void ZONE_CREATE_HELPER::commitZone( std::unique_ptr<ZONE> aZone )
{
    switch ( m_params.m_mode )
    {
        case ZONE_MODE::CUTOUT:
            // For cutouts, subtract from the source
            performZoneCutout( *m_params.m_sourceZone, *aZone );
            break;

        case ZONE_MODE::ADD:
        case ZONE_MODE::SIMILAR:
        {
            BOARD_COMMIT commit( &m_tool );
            BOARD*       board = m_tool.getModel<BOARD>();

            aZone->HatchBorder();

            // TODO Refill zones when KiCad supports auto re-fill

            commit.Add( aZone.get() );

            std::lock_guard<KISPINLOCK> lock( board->GetConnectivity()->GetLock() );

            commit.Push( _( "Add a zone" ) );
            m_tool.GetManager()->RunAction( PCB_ACTIONS::selectItem, true, aZone.release() );
            break;
        }

        case ZONE_MODE::GRAPHIC_POLYGON:
        {
            BOARD_COMMIT          commit( &m_tool );
            BOARD*                board = m_tool.getModel<BOARD>();
            PCB_LAYER_ID          layer = m_params.m_layer;
            PCB_SHAPE*            poly;

            if( m_tool.m_isFootprintEditor )
                poly = new FP_SHAPE( static_cast<FOOTPRINT*>( m_tool.m_frame->GetModel() ) );
            else
                poly = new PCB_SHAPE();

            poly->SetShape( S_POLYGON );

            if( layer == Edge_Cuts || layer == F_CrtYd || layer == B_CrtYd )
                poly->SetFilled( false );
            else
                poly->SetFilled( true );

            poly->SetWidth( board->GetDesignSettings().GetLineThickness( m_params.m_layer ) );
            poly->SetLayer( layer );
            poly->SetPolyShape( *aZone->Outline() );

            commit.Add( poly );
            m_tool.GetManager()->RunAction( PCB_ACTIONS::selectItem, true, poly );

            commit.Push( _( "Add a graphical polygon" ) );

            break;
        }
    }
}


bool ZONE_CREATE_HELPER::OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr )
{
    // if we don't have a zone, create one
    // the user's choice here can affect things like the colour of the preview
    if( !m_zone )
    {
        if( m_params.m_sourceZone )
            m_zone = createZoneFromExisting( *m_params.m_sourceZone );
        else
            m_zone = createNewZone( m_params.m_keepout );

        if( m_zone )
        {
            m_tool.GetManager()->RunAction( PCB_ACTIONS::selectionClear, true );

            // set up poperties from zone
            const auto& settings = *m_parentView.GetPainter()->GetSettings();
            COLOR4D color = settings.GetColor( nullptr, m_zone->GetLayer() );

            m_previewItem.SetStrokeColor( COLOR4D::WHITE );
            m_previewItem.SetFillColor( color.WithAlpha( 0.2 ) );

            m_parentView.SetVisible( &m_previewItem, true );

            aMgr.SetLeaderMode( m_zone->GetHV45() ? POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45
                                                  : POLYGON_GEOM_MANAGER::LEADER_MODE::DIRECT );
        }
    }

    return m_zone != nullptr;
}


void ZONE_CREATE_HELPER::OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr )
{
    // send the points to the preview item
    m_previewItem.SetPoints( aMgr.GetLockedInPoints(), aMgr.GetLeaderLinePoints() );
    m_parentView.Update( &m_previewItem, KIGFX::GEOMETRY );
}


void ZONE_CREATE_HELPER::OnComplete( const POLYGON_GEOM_MANAGER& aMgr )
{
    auto& finalPoints = aMgr.GetLockedInPoints();

    if( finalPoints.PointCount() < 3 )
    {
        // just scrap the zone in progress
        m_zone = nullptr;
    }
    else
    {
        // if m_params.m_mode == DRAWING_TOOL::ZONE_MODE::CUTOUT, m_zone
        // will be merged to the existing zone as a new hole.
        m_zone->Outline()->NewOutline();
        auto* outline = m_zone->Outline();

        for( int i = 0; i < finalPoints.PointCount(); ++i )
            outline->Append( finalPoints.CPoint( i ) );

        // In DEG45 mode, we may have intermediate points in the leader that should be
        // included as they are shown in the preview.  These typically maintain the
        // 45 constraint
        if( aMgr.GetLeaderMode() == POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45 )
        {
            const auto& pts = aMgr.GetLeaderLinePoints();
            for( int i = 1; i < pts.PointCount(); i++ )
                outline->Append( pts.CPoint( i ) );
        }

        outline->Outline( 0 ).SetClosed( true );
        outline->RemoveNullSegments();
        outline->Simplify( SHAPE_POLY_SET::PM_FAST );

        // hand the zone over to the committer
        commitZone( std::move( m_zone ) );
        m_zone = nullptr;
    }

    m_parentView.SetVisible( &m_previewItem, false );
}

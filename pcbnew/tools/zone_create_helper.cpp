/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <pcb_painter.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <view/view_controls.h>

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


void ZONE_CREATE_HELPER::setUniquePriority( ZONE_SETTINGS& aZoneInfo )
{
    PCB_BASE_EDIT_FRAME*  frame = m_tool.getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD*                board = frame->GetBoard();

    // By default, new zones get the first unused priority
    std::set<unsigned> priorities;

    for( ZONE* zone : board->Zones() )
    {
        if( zone->GetTeardropAreaType() == TEARDROP_TYPE::TD_NONE
                && ( zone->GetLayerSet() & LSET::AllCuMask() ).any()
                && !zone->GetIsRuleArea() )
        {
            priorities.insert( zone->GetAssignedPriority() );
        }
    }

    unsigned priority = 0;

    for( unsigned exist_priority : priorities )
    {
        if( priority != exist_priority )
            break;

        ++priority;
    }

    aZoneInfo.m_ZonePriority = priority;
}


std::unique_ptr<ZONE> ZONE_CREATE_HELPER::createNewZone( bool aKeepout )
{
    PCB_BASE_EDIT_FRAME*  frame = m_tool.getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD*                board = frame->GetBoard();
    BOARD_ITEM_CONTAINER* parent = m_tool.m_frame->GetModel();
    KIGFX::VIEW_CONTROLS* controls = m_tool.GetManager()->GetViewControls();
    std::set<int>         highlightedNets = board->GetHighLightNetCodes();

    // Get the current default settings for zones
    ZONE_SETTINGS         zoneInfo = board->GetDesignSettings().GetDefaultZoneSettings();
    zoneInfo.m_Layers.reset().set( m_params.m_layer );  // TODO(JE) multilayer defaults?
    zoneInfo.m_LayerProperties.clear();                 // Do not copy over layer properties
    zoneInfo.m_Netcode = highlightedNets.empty() ? -1 : *highlightedNets.begin();
    zoneInfo.SetIsRuleArea( m_params.m_keepout );

    if( m_params.m_mode != ZONE_MODE::GRAPHIC_POLYGON
            && ( zoneInfo.m_Layers & LSET::AllCuMask() ).any() )
    {
        setUniquePriority( zoneInfo );
    }

    // If we don't have a net from highlighting, maybe we can get one from the selection
    PCB_SELECTION_TOOL* selectionTool = m_tool.GetManager()->GetTool<PCB_SELECTION_TOOL>();

    if( selectionTool && !selectionTool->GetSelection().Empty() && zoneInfo.m_Netcode == -1 )
    {
        EDA_ITEM* item = *selectionTool->GetSelection().GetItems().begin();

        if( BOARD_CONNECTED_ITEM* bci = dynamic_cast<BOARD_CONNECTED_ITEM*>( item ) )
            zoneInfo.m_Netcode = bci->GetNetCode();
    }

    if( m_params.m_mode != ZONE_MODE::GRAPHIC_POLYGON )
    {
        // Show options dialog
        int dialogResult;

        if( m_params.m_keepout )
            dialogResult = InvokeRuleAreaEditor( frame, &zoneInfo, m_tool.board() );
        else if( ( zoneInfo.m_Layers & LSET::AllCuMask() ).any() )
            dialogResult = InvokeCopperZonesEditor( frame, nullptr, &zoneInfo );
        else
            dialogResult = InvokeNonCopperZonesEditor( frame, &zoneInfo );

        if( dialogResult == wxID_CANCEL )
            return nullptr;

        controls->WarpMouseCursor( controls->GetCursorPosition(), true );
        frame->GetCanvas()->SetFocus();
    }

    wxASSERT( !m_tool.m_isFootprintEditor || ( parent->Type() == PCB_FOOTPRINT_T ) );

    std::unique_ptr<ZONE> newZone = std::make_unique<ZONE>( parent );

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
    TOOL_MANAGER* toolMgr = m_tool.GetManager();
    toolMgr->RunAction( ACTIONS::selectionClear );

    SHAPE_POLY_SET originalOutline( *aZone.Outline() );
    originalOutline.BooleanSubtract( *aCutout.Outline() );

    // After substracting the hole, originalOutline can have more than one main outline.
    // But a zone can have only one main outline, so create as many zones as originalOutline
    // contains main outlines:
    for( int outline = 0; outline < originalOutline.OutlineCount(); outline++ )
    {
        SHAPE_POLY_SET* newZoneOutline = new SHAPE_POLY_SET;
        newZoneOutline->AddOutline( originalOutline.Outline( outline ) );

        // Add holes (if any) to the new zone outline:
        for (int hole = 0; hole < originalOutline.HoleCount( outline ) ; hole++ )
            newZoneOutline->AddHole( originalOutline.CHole( outline, hole ) );

        ZONE* newZone = new ZONE( aZone );
        newZone->SetOutline( newZoneOutline );  // zone takes ownership
        newZone->SetLocalFlags( 1 );
        newZone->HatchBorder();
        newZone->UnFill();
        newZones.push_back( newZone );
        commit.Add( newZone );
    }

    commit.Remove( &aZone );
    commit.Push( _( "Add Zone Cutout" ) );

    // Select the new zone and set it as the source for the next cutout
    if( newZones.empty() )
    {
        m_params.m_sourceZone = nullptr;
    }
    else
    {
        m_params.m_sourceZone = newZones[0];
        toolMgr->RunAction<EDA_ITEM*>( ACTIONS::selectItem, newZones[0] );
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

            aZone->HatchBorder();

            commit.Add( aZone.get() );
            commit.Push( _( "Draw Zone" ) );

            m_tool.GetManager()->RunAction<EDA_ITEM*>( ACTIONS::selectItem, aZone.release() );
            break;
        }

        case ZONE_MODE::GRAPHIC_POLYGON:
        {
            BOARD_COMMIT  commit( &m_tool );
            BOARD*        board = m_tool.getModel<BOARD>();
            PCB_LAYER_ID  layer = m_params.m_layer;
            PCB_SHAPE*    poly = new PCB_SHAPE( m_tool.m_frame->GetModel() );

            poly->SetShape( SHAPE_T::POLY );
            poly->SetFilled( layer != Edge_Cuts && layer != F_CrtYd && layer != B_CrtYd );

            poly->SetStroke( STROKE_PARAMS( board->GetDesignSettings().GetLineThickness( layer ),
                                            LINE_STYLE::SOLID ) );
            poly->SetLayer( layer );
            poly->SetPolyShape( *aZone->Outline() );

            commit.Add( poly );
            commit.Push( _( "Draw Polygon" ) );

            m_tool.GetManager()->RunAction<EDA_ITEM*>( ACTIONS::selectItem, poly );
            break;
        }
    }
}


bool ZONE_CREATE_HELPER::OnFirstPoint( POLYGON_GEOM_MANAGER& aMgr )
{
    // if we don't have a zone, create one
    if( !m_zone )
    {
        if( m_params.m_sourceZone )
            m_zone = createZoneFromExisting( *m_params.m_sourceZone );
        else
            m_zone = createNewZone( m_params.m_keepout );

        if( m_zone )
        {
            m_tool.GetManager()->RunAction( ACTIONS::selectionClear );

            // set up properties from zone
            const RENDER_SETTINGS& settings = *m_parentView.GetPainter()->GetSettings();
            COLOR4D                color = settings.GetColor( nullptr, m_zone->GetFirstLayer() );

            m_previewItem.SetStrokeColor( COLOR4D::WHITE );
            m_previewItem.SetFillColor( color.WithAlpha( 0.2 ) );

            m_parentView.SetVisible( &m_previewItem, true );

            LEADER_MODE mode = m_tool.GetAngleSnapMode();

            aMgr.SetLeaderMode( mode );
        }
    }

    return m_zone != nullptr;
}


void ZONE_CREATE_HELPER::OnGeometryChange( const POLYGON_GEOM_MANAGER& aMgr )
{
    // Handle a cancel-interactive
    if( m_zone && !aMgr.IsPolygonInProgress() )
    {
        m_zone = nullptr;
        m_parentView.SetVisible( &m_previewItem, false );
        return;
    }

    // send the points to the preview item
    m_previewItem.SetPoints( aMgr.GetLockedInPoints(), aMgr.GetLeaderLinePoints(),
                             aMgr.GetLoopLinePoints() );
    m_parentView.Update( &m_previewItem, KIGFX::GEOMETRY );
}


void ZONE_CREATE_HELPER::OnComplete( const POLYGON_GEOM_MANAGER& aMgr )
{
    const SHAPE_LINE_CHAIN& finalPoints = aMgr.GetLockedInPoints();

    if( finalPoints.PointCount() < 3 )
    {
        // just scrap the zone in progress
        m_zone = nullptr;
    }
    else
    {
        // if m_params.m_mode == DRAWING_TOOL::ZONE_MODE::CUTOUT, m_zone will be merged to the
        // existing zone as a new hole.
        m_zone->Outline()->NewOutline();
        SHAPE_POLY_SET* outline = m_zone->Outline();

        for( int i = 0; i < finalPoints.PointCount(); ++i )
            outline->Append( finalPoints.CPoint( i ) );

        // In DEG45 mode, we may have intermediate points in the leader that should be included
        // as they are shown in the preview.  These typically maintain the 45 constraint
        if( aMgr.GetLeaderMode() == LEADER_MODE::DEG45 || aMgr.GetLeaderMode() == LEADER_MODE::DEG90 )
        {
            const SHAPE_LINE_CHAIN leaderPts = aMgr.GetLeaderLinePoints();

            for( int i = 1; i < leaderPts.PointCount(); i++ )
                outline->Append( leaderPts.CPoint( i ) );

            const SHAPE_LINE_CHAIN loopPts = aMgr.GetLoopLinePoints();

            for( int i = 1; i < loopPts.PointCount() - 1; i++ )
                outline->Append( loopPts.CPoint( i ) );
        }

        SHAPE_LINE_CHAIN& chain = outline->Outline( 0 );

        chain.SetClosed( true );
        chain.Simplify( true );

        // Remove the start point if it lies on the line between neighbouring points.
        // Simplify doesn't handle that currently.
        if( chain.PointCount() >= 3 )
        {
            SEG seg( chain.CLastPoint(), chain.CPoint( 1 ) );

            if( seg.LineDistance( chain.CPoint( 0 ) ) <= 1 )
                chain.Remove( 0 );
        }

        // hand the zone over to the committer
        commitZone( std::move( m_zone ) );
        m_zone = nullptr;
    }

    m_parentView.SetVisible( &m_previewItem, false );
}

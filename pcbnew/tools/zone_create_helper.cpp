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

#include <tools/zone_create_helper.h>
#include <tool/tool_manager.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <class_edge_mod.h>
#include <board_commit.h>
#include <pcb_painter.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>
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


std::unique_ptr<ZONE_CONTAINER> ZONE_CREATE_HELPER::createNewZone( bool aKeepout )
{
    auto& frame = *m_tool.getEditFrame<PCB_BASE_EDIT_FRAME>();
    auto& board = *m_tool.getModel<BOARD>();
    BOARD_ITEM_CONTAINER* parent = m_tool.m_frame->GetModel();
    KIGFX::VIEW_CONTROLS* controls = m_tool.GetManager()->GetViewControls();

    // Get the current default settings for zones
    ZONE_SETTINGS zoneInfo = frame.GetZoneSettings();
    zoneInfo.m_CurrentZone_Layer = m_params.m_layer;
    zoneInfo.m_NetcodeSelection = board.GetHighLightNetCode();
    zoneInfo.SetIsKeepout( m_params.m_keepout );
    zoneInfo.m_Zone_45_Only = ( m_params.m_leaderMode == POLYGON_GEOM_MANAGER::LEADER_MODE::DEG45 );

    if( m_params.m_mode != ZONE_MODE::GRAPHIC_POLYGON )
    {
        // Get the current default settings for zones

        // Show options dialog
        int dialogResult;

        if( m_params.m_keepout )
            dialogResult = InvokeKeepoutAreaEditor( &frame, &zoneInfo );
        else
        {
            if( IsCopperLayer( zoneInfo.m_CurrentZone_Layer ) )
                dialogResult = InvokeCopperZonesEditor( &frame, &zoneInfo );
            else
                dialogResult = InvokeNonCopperZonesEditor( &frame, &zoneInfo );
        }

        if( dialogResult == wxID_CANCEL )
            return nullptr;

        controls->WarpCursor( controls->GetCursorPosition(), true );
    }

    // The new zone is a ZONE_CONTAINER if created in the board editor
    // and a MODULE_ZONE_CONTAINER if created in the footprint editor
    wxASSERT( !m_tool.m_editModules || ( parent->Type() == PCB_MODULE_T ) );

    auto newZone = m_tool.m_editModules ?
                        std::make_unique<MODULE_ZONE_CONTAINER>( parent ) :
                        std::make_unique<ZONE_CONTAINER>( parent );

    // Apply the selected settings
    zoneInfo.ExportSetting( *newZone );

    return newZone;
}


std::unique_ptr<ZONE_CONTAINER> ZONE_CREATE_HELPER::createZoneFromExisting(
        const ZONE_CONTAINER& aSrcZone )
{
    auto& board = *m_tool.getModel<BOARD>();

    auto newZone = std::make_unique<ZONE_CONTAINER>( &board );

    ZONE_SETTINGS zoneSettings;
    zoneSettings << aSrcZone;

    zoneSettings.ExportSetting( *newZone );

    return newZone;
}


void ZONE_CREATE_HELPER::performZoneCutout( ZONE_CONTAINER& aZone, ZONE_CONTAINER& aCutout )
{
    BOARD_COMMIT commit( &m_tool );
    BOARD* board = m_tool.getModel<BOARD>();
    std::vector<ZONE_CONTAINER*> newZones;

    // Clear the selection before removing the old zone
    auto toolMgr = m_tool.GetManager();
    toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    SHAPE_POLY_SET originalOutline( *aZone.Outline() );
    originalOutline.BooleanSubtract( *aCutout.Outline(), SHAPE_POLY_SET::PM_FAST );

    for( int i = 0; i < originalOutline.OutlineCount(); i++ )
    {
        auto newZoneOutline = new SHAPE_POLY_SET;
        newZoneOutline->AddOutline( originalOutline.Outline( i ) );

        for (int j = 0; j < originalOutline.HoleCount(i) ; j++ )
            newZoneOutline->AddHole( originalOutline.CHole(i, j), i );

        auto newZone = new ZONE_CONTAINER( aZone );
        newZone->SetOutline( newZoneOutline );
        newZone->SetLocalFlags( 1 );
        newZone->Hatch();
        newZones.push_back( newZone );
        commit.Add( newZone );
    }

    commit.Remove( &aZone );
    commit.Push( _( "Add a zone cutout" ) );

    ZONE_FILLER filler( board );
    filler.Fill( newZones );

    // Select the new zone and set it as the source for the next cutout
    toolMgr->RunAction( PCB_ACTIONS::selectItem, true, newZones[0] );
    m_params.m_sourceZone = newZones[0];

}


void ZONE_CREATE_HELPER::commitZone( std::unique_ptr<ZONE_CONTAINER> aZone )
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
            BOARD_COMMIT bCommit( &m_tool );

            aZone->Hatch();

            if( !m_params.m_keepout )
            {
                ZONE_FILLER filler( m_tool.getModel<BOARD>() );
                filler.Fill( { aZone.get() } );
            }

            bCommit.Add( aZone.get() );
            bCommit.Push( _( "Add a zone" ) );
            m_tool.GetManager()->RunAction( PCB_ACTIONS::selectItem, true, aZone.release() );
            break;
        }

        case ZONE_MODE::GRAPHIC_POLYGON:
        {
            BOARD_COMMIT bCommit( &m_tool );
            BOARD_ITEM_CONTAINER* parent = m_tool.m_frame->GetModel();

            if( m_tool.getDrawingLayer() != Edge_Cuts )
            {
                auto poly = m_tool.m_editModules ? new EDGE_MODULE( (MODULE *) parent )
                                                 : new DRAWSEGMENT();
                poly->SetShape ( S_POLYGON );
                poly->SetLayer( m_tool.getDrawingLayer() );
                poly->SetPolyShape ( *aZone->Outline() );
                bCommit.Add( poly );
                m_tool.GetManager()->RunAction( PCB_ACTIONS::selectItem, true, poly );
            }
            else
            {
                auto outline = aZone->Outline();

                for( auto seg = outline->CIterateSegments( 0 ); seg; seg++ )
                {
                    auto new_seg = m_tool.m_editModules ? new EDGE_MODULE( (MODULE *) parent )
                                                        : new DRAWSEGMENT();
                    new_seg->SetShape( S_SEGMENT );
                    new_seg->SetLayer( m_tool.getDrawingLayer() );
                    new_seg->SetStart( wxPoint( seg.Get().A.x, seg.Get().A.y ) );
                    new_seg->SetEnd( wxPoint( seg.Get().B.x, seg.Get().B.y ) );
                    bCommit.Add( new_seg );
                }
            }

            bCommit.Push( _( "Add a graphical polygon" ) );

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
        m_tool.GetManager()->RunAction( PCB_ACTIONS::selectionClear, true );

        if( m_params.m_sourceZone )
            m_zone = createZoneFromExisting( *m_params.m_sourceZone );
        else
            m_zone = createNewZone( m_params.m_keepout );

        if( m_zone )
        {
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

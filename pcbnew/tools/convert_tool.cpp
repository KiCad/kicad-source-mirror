/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <bitmaps.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <collectors.h>
#include <confirm.h>
#include <convert_basic_shapes_to_polygon.h>
#include <footprint_edit_frame.h>
#include <fp_shape.h>
#include <geometry/shape_compound.h>
#include <menus_helpers.h>
#include <pcb_edit_frame.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <tool/tool_manager.h>
#include <tools/edit_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <trigo.h>
#include <zone.h>

#include "convert_tool.h"


CONVERT_TOOL::CONVERT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Convert" ),
    m_selectionTool( nullptr ),
    m_menu( nullptr ),
    m_frame( nullptr )
{
}


CONVERT_TOOL::~CONVERT_TOOL()
{
    delete m_menu;
}


using S_C   = SELECTION_CONDITIONS;
using P_S_C = PCB_SELECTION_CONDITIONS;


bool CONVERT_TOOL::Init()
{
    m_selectionTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    m_frame         = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_menu = new CONDITIONAL_MENU( this );
    m_menu->SetIcon( BITMAPS::convert );
    m_menu->SetTitle( _( "Create from Selection" ) );

    static KICAD_T convertibleTracks[] = { PCB_TRACE_T, PCB_ARC_T, EOT };
    static KICAD_T zones[]  = { PCB_ZONE_T, PCB_FP_ZONE_T, EOT };

    auto graphicLines = P_S_C::OnlyGraphicShapeTypes( { SHAPE_T::SEGMENT,
                                                        SHAPE_T::RECT,
                                                        SHAPE_T::CIRCLE,
                                                        SHAPE_T::ARC } )
                                && P_S_C::SameLayer();

    auto graphicToTrack = P_S_C::OnlyGraphicShapeTypes( { SHAPE_T::SEGMENT, SHAPE_T::ARC } );

    auto trackLines   = S_C::MoreThan( 1 ) && S_C::OnlyTypes( convertibleTracks )
                                && P_S_C::SameLayer();

    auto anyLines     = graphicLines || trackLines;
    auto anyPolys     = S_C::OnlyTypes( zones )
                            || P_S_C::OnlyGraphicShapeTypes( { SHAPE_T::POLY, SHAPE_T::RECT } );

    auto lineToArc = S_C::Count( 1 )
                         && ( P_S_C::OnlyGraphicShapeTypes( { SHAPE_T::SEGMENT } )
                                || S_C::OnlyType( PCB_TRACE_T ) );

    auto showConvert       = anyPolys || anyLines || lineToArc;
    auto canCreatePolyType = anyLines || anyPolys;
    auto canCreateTracks   = anyPolys || graphicToTrack;

    m_menu->AddItem( PCB_ACTIONS::convertToPoly, canCreatePolyType );
    m_menu->AddItem( PCB_ACTIONS::convertToZone, canCreatePolyType );
    m_menu->AddItem( PCB_ACTIONS::convertToKeepout, canCreatePolyType );
    m_menu->AddItem( PCB_ACTIONS::convertToLines, anyPolys );

    // Currently the code exists, but tracks are not really existing in footprints
    // only segments on copper layers
    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        m_menu->AddItem( PCB_ACTIONS::convertToTracks, canCreateTracks );

    m_menu->AddItem( PCB_ACTIONS::convertToArc, lineToArc );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_menu, showConvert, 100 );

    return true;
}


int CONVERT_TOOL::CreatePolys( const TOOL_EVENT& aEvent )
{
    FOOTPRINT* parentFootprint = nullptr;

    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    switch( item->Type() )
                    {
                    case PCB_SHAPE_T:
                    case PCB_FP_SHAPE_T:
                        switch( static_cast<PCB_SHAPE*>( item )->GetShape() )
                        {
                        case SHAPE_T::SEGMENT:
                        case SHAPE_T::RECT:
                        case SHAPE_T::CIRCLE:
                        case SHAPE_T::ARC:
                        case SHAPE_T::POLY:
                            break;

                        default:
                            aCollector.Remove( item );
                        }

                        break;

                    case PCB_TRACE_T:
                    case PCB_ARC_T:
                        break;

                    case PCB_ZONE_T:
                    case PCB_FP_ZONE_T:
                        break;

                    default:
                        aCollector.Remove( item );
                    }
                }
            } );

    if( selection.Empty() )
        return 0;

    PCB_LAYER_ID   destLayer = m_frame->GetActiveLayer();
    SHAPE_POLY_SET polySet   = makePolysFromSegs( selection.GetItems() );

    polySet.Append( makePolysFromRects( selection.GetItems() ) );

    polySet.Append( makePolysFromCircles( selection.GetItems() ) );

    polySet.Append( extractPolygons( selection.GetItems() ) );

    if( polySet.IsEmpty() )
        return 0;

    bool isFootprint = m_frame->IsType( FRAME_FOOTPRINT_EDITOR );

    if( FP_SHAPE* graphic = dynamic_cast<FP_SHAPE*>( selection.Front() ) )
        parentFootprint = graphic->GetParentFootprint();

    BOARD_COMMIT commit( m_frame );

    // For now, we convert each outline in the returned shape to its own polygon
    std::vector<SHAPE_POLY_SET> polys;

    for( int i = 0; i < polySet.OutlineCount(); i++ )
        polys.emplace_back( SHAPE_POLY_SET( polySet.COutline( i ) ) );

    if( aEvent.IsAction( &PCB_ACTIONS::convertToPoly ) )
    {
        for( const SHAPE_POLY_SET& poly : polys )
        {
            PCB_SHAPE* graphic = isFootprint ? new FP_SHAPE( parentFootprint ) : new PCB_SHAPE;

            graphic->SetShape( SHAPE_T::POLY );
            graphic->SetFilled( false );
            graphic->SetWidth( poly.Outline( 0 ).Width() );
            graphic->SetLayer( destLayer );
            graphic->SetPolyShape( poly );

            commit.Add( graphic );
        }

        commit.Push( _( "Convert shapes to polygon" ) );
    }
    else
    {
        // Creating zone or keepout
        PCB_BASE_EDIT_FRAME*  frame    = getEditFrame<PCB_BASE_EDIT_FRAME>();
        BOARD_ITEM_CONTAINER* parent   = frame->GetModel();
        ZONE_SETTINGS         zoneInfo = frame->GetZoneSettings();

        bool nonCopper = IsNonCopperLayer( destLayer );
        zoneInfo.m_Layers.reset().set( destLayer );

        int ret;

        if( aEvent.IsAction( &PCB_ACTIONS::convertToKeepout ) )
            ret = InvokeRuleAreaEditor( frame, &zoneInfo );
        else if( nonCopper )
            ret = InvokeNonCopperZonesEditor( frame, &zoneInfo );
        else
            ret = InvokeCopperZonesEditor( frame, &zoneInfo );

        if( ret == wxID_CANCEL )
            return 0;

        for( const SHAPE_POLY_SET& poly : polys )
        {
            ZONE* zone = isFootprint ? new FP_ZONE( parent ) : new ZONE( parent );

            *zone->Outline() = poly;
            zone->HatchBorder();

            zoneInfo.ExportSetting( *zone );

            commit.Add( zone );
        }

        commit.Push( _( "Convert shapes to zone" ) );
    }

    return 0;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromSegs( const std::deque<EDA_ITEM*>& aItems )
{
    // TODO: This code has a somewhat-similar purpose to ConvertOutlineToPolygon but is slightly
    // different, so this remains a separate algorithm.  It might be nice to analyze the dfiferences
    // in requirements and refactor this.
    const int chainingEpsilon = Millimeter2iu( 0.02 );

    SHAPE_POLY_SET poly;

    // Stores pairs of (anchor, item) where anchor == 0 -> SEG.A, anchor == 1 -> SEG.B
    std::map<VECTOR2I, std::vector<std::pair<int, EDA_ITEM*>>> connections;
    std::set<EDA_ITEM*> used;
    std::deque<EDA_ITEM*> toCheck;

    auto closeEnough =
            []( VECTOR2I aLeft, VECTOR2I aRight, unsigned aLimit )
            {
                return ( aLeft - aRight ).SquaredEuclideanNorm() <= SEG::Square( aLimit );
            };

    auto findInsertionPoint =
            [&]( VECTOR2I aPoint ) -> VECTOR2I
            {
                for( const auto& candidatePair : connections )
                {
                    if( closeEnough( aPoint, candidatePair.first, chainingEpsilon ) )
                        return candidatePair.first;
                }

                return aPoint;
            };

    for( EDA_ITEM* item : aItems )
    {
        if( OPT<SEG> seg = getStartEndPoints( item, nullptr ) )
        {
            toCheck.push_back( item );
            connections[findInsertionPoint( seg->A )].emplace_back( std::make_pair( 0, item ) );
            connections[findInsertionPoint( seg->B )].emplace_back( std::make_pair( 1, item ) );
        }
    }

    while( !toCheck.empty() )
    {
        EDA_ITEM* candidate = toCheck.front();
        toCheck.pop_front();

        if( used.count( candidate ) )
            continue;

        int width = -1;
        SHAPE_LINE_CHAIN outline;

        auto insert =
                [&]( EDA_ITEM* aItem, VECTOR2I aAnchor, bool aDirection )
                {
                    if( aItem->Type() == PCB_ARC_T ||
                        ( aItem->Type() == PCB_SHAPE_T &&
                          static_cast<PCB_SHAPE*>( aItem )->GetShape() == SHAPE_T::ARC ) )
                    {
                        SHAPE_ARC arc;

                        if( aItem->Type() == PCB_ARC_T )
                        {
                            std::shared_ptr<SHAPE> es =
                                    static_cast<PCB_ARC*>( aItem )->GetEffectiveShape();
                            arc = *static_cast<SHAPE_ARC*>( es.get() );
                        }
                        else
                        {
                            PCB_SHAPE* ps = static_cast<PCB_SHAPE*>( aItem );
                            arc = SHAPE_ARC( ps->GetArcStart(), ps->GetArcMid(), ps->GetArcEnd(),
                                             ps->GetWidth() );
                        }

                        if( aDirection )
                            outline.Append( aAnchor == arc.GetP0() ? arc : arc.Reversed() );
                        else
                            outline.Insert( 0, aAnchor == arc.GetP0() ? arc : arc.Reversed() );
                    }
                    else
                    {
                        OPT<SEG> nextSeg = getStartEndPoints( aItem, &width );
                        wxASSERT( nextSeg );

                        VECTOR2I& point = ( aAnchor == nextSeg->A ) ? nextSeg->B : nextSeg->A;

                        if( aDirection )
                            outline.Append( point );
                        else
                            outline.Insert( 0, point );
                    }
                };

        // aDirection == true for walking "right" and appending to the end of points
        // false for walking "left" and prepending to the beginning
        std::function<void( EDA_ITEM*, VECTOR2I, bool )> process =
                [&]( EDA_ITEM* aItem, VECTOR2I aAnchor, bool aDirection )
                {
                    if( used.count( aItem ) )
                        return;

                    used.insert( aItem );

                    insert( aItem, aAnchor, aDirection );

                    OPT<SEG> anchors = getStartEndPoints( aItem, &width );
                    wxASSERT( anchors );

                    VECTOR2I nextAnchor = ( aAnchor == anchors->A ) ? anchors->B : anchors->A;

                    for( std::pair<int, EDA_ITEM*> pair : connections[nextAnchor] )
                    {
                        if( pair.second == aItem )
                            continue;

                        process( pair.second, nextAnchor, aDirection );
                    }
                };

        OPT<SEG> anchors = getStartEndPoints( candidate, &width );
        wxASSERT( anchors );

        // Start with the first object and walk "right"
        // Note if the first object is an arc, we don't need to insert its first point here, the
        // whole arc will be inserted at anchor B inside process()
        if( !( candidate->Type() == PCB_ARC_T ||
               ( candidate->Type() == PCB_SHAPE_T &&
                 static_cast<PCB_SHAPE*>( candidate )->GetShape() == SHAPE_T::ARC ) ) )
        {
            insert( candidate, anchors->A, true );
        }

        process( candidate, anchors->B, true );

        // check for any candidates on the "left"
        EDA_ITEM* left = nullptr;

        for( std::pair<int, EDA_ITEM*> possibleLeft : connections[anchors->A] )
        {
            if( possibleLeft.second != candidate )
            {
                left = possibleLeft.second;
                break;
            }
        }

        if( left )
            process( left, anchors->A, false );

        if( outline.PointCount() < 3 )
            continue;

        outline.SetClosed( true );
        outline.Simplify();

        if( width >= 0 )
            outline.SetWidth( width );

        poly.AddOutline( outline );
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromRects( const std::deque<EDA_ITEM*>& aItems )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() != PCB_SHAPE_T && item->Type() != PCB_FP_SHAPE_T )
            continue;

        PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( item );

        if( graphic->GetShape() != SHAPE_T::RECT )
            continue;

        SHAPE_LINE_CHAIN outline;
        VECTOR2I start( graphic->GetStart() );
        VECTOR2I end( graphic->GetEnd() );

        outline.Append( start );
        outline.Append( VECTOR2I( end.x, start.y ) );
        outline.Append( end );
        outline.Append( VECTOR2I( start.x, end.y ) );
        outline.SetClosed( true );

        outline.SetWidth( graphic->GetWidth() );

        poly.AddOutline( outline );
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromCircles( const std::deque<EDA_ITEM*>& aItems )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() != PCB_SHAPE_T && item->Type() != PCB_FP_SHAPE_T )
            continue;

        PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( item );

        if( graphic->GetShape() != SHAPE_T::CIRCLE )
            continue;

        BOARD_DESIGN_SETTINGS& bds = graphic->GetBoard()->GetDesignSettings();
        SHAPE_LINE_CHAIN outline;

        TransformCircleToPolygon( outline, graphic->GetPosition(), graphic->GetRadius(),
                                  bds.m_MaxError, ERROR_OUTSIDE );

        poly.AddOutline( outline );
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::extractPolygons( const std::deque<EDA_ITEM*>& aItems )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        switch( item->Type() )
        {
        case PCB_SHAPE_T:
            switch( static_cast<PCB_SHAPE*>( item )->GetShape() )
            {
            case SHAPE_T::POLY:
                poly.Append( static_cast<PCB_SHAPE*>( item )->GetPolyShape() );
                break;

            default:
                continue;
            }

            break;

        case PCB_ZONE_T:
        case PCB_FP_ZONE_T:
            poly.Append( *static_cast<ZONE*>( item )->Outline() );
            break;

        default:
            continue;
        }
    }

    return poly;
}


int CONVERT_TOOL::CreateLines( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    switch( item->Type() )
                    {
                    case PCB_SHAPE_T:
                    case PCB_FP_SHAPE_T:
                        switch( static_cast<PCB_SHAPE*>( item )->GetShape() )
                        {
                        case SHAPE_T::SEGMENT:
                        case SHAPE_T::ARC:
                        case SHAPE_T::POLY:
                        case SHAPE_T::RECT:
                            break;

                        default:
                            aCollector.Remove( item );
                        }

                        break;

                    case PCB_ZONE_T:
                    case PCB_FP_ZONE_T:
                        break;

                    default:
                        aCollector.Remove( item );
                    }
                }
            } );

    if( selection.Empty() )
        return 0;

    auto getPolySet =
            []( EDA_ITEM* aItem )
            {
                SHAPE_POLY_SET set;

                switch( aItem->Type() )
                {
                case PCB_ZONE_T:
                case PCB_FP_ZONE_T:
                    set = *static_cast<ZONE*>( aItem )->Outline();
                    break;

                case PCB_SHAPE_T:
                case PCB_FP_SHAPE_T:
                {
                    PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( aItem );

                    if( graphic->GetShape() == SHAPE_T::POLY )
                    {
                        set = graphic->GetPolyShape();
                    }
                    else if( graphic->GetShape() == SHAPE_T::RECT )
                    {
                        SHAPE_LINE_CHAIN outline;
                        VECTOR2I start( graphic->GetStart() );
                        VECTOR2I end( graphic->GetEnd() );

                        outline.Append( start );
                        outline.Append( VECTOR2I( end.x, start.y ) );
                        outline.Append( end );
                        outline.Append( VECTOR2I( start.x, end.y ) );
                        outline.SetClosed( true );

                        set.AddOutline( outline );
                    }
                    else
                    {
                        wxFAIL_MSG( "Unhandled graphic shape type in PolyToLines - getPolySet" );
                    }
                    break;
                }

                default:
                    wxFAIL_MSG( "Unhandled type in PolyToLines - getPolySet" );
                    break;
                }

                return set;
            };

    auto getSegList =
            []( SHAPE_POLY_SET& aPoly )
            {
                std::vector<SEG> segs;

                // Our input should be valid polys, so OK to assert here
                wxASSERT( aPoly.VertexCount() >= 2 );

                for( int i = 1; i < aPoly.VertexCount(); i++ )
                    segs.emplace_back( SEG( aPoly.CVertex( i - 1 ), aPoly.CVertex( i ) ) );

                segs.emplace_back( SEG( aPoly.CVertex( aPoly.VertexCount() - 1 ),
                                        aPoly.CVertex( 0 ) ) );

                return segs;
            };

    BOARD_COMMIT          commit( m_frame );
    PCB_BASE_EDIT_FRAME*  frame       = getEditFrame<PCB_BASE_EDIT_FRAME>();
    FOOTPRINT_EDIT_FRAME* fpEditor    = dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame );
    FOOTPRINT*            footprint   = nullptr;
    PCB_LAYER_ID          targetLayer = m_frame->GetActiveLayer();
    PCB_LAYER_ID          copperLayer = UNSELECTED_LAYER;
    BOARD_ITEM_CONTAINER* parent      = frame->GetModel();

    if( fpEditor )
        footprint = fpEditor->GetBoard()->GetFirstFootprint();

    auto handleGraphicSeg =
            [&]( EDA_ITEM* aItem )
            {
                if( aItem->Type() != PCB_SHAPE_T && aItem->Type() != PCB_FP_SHAPE_T )
                    return false;

                PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( aItem );

                if( graphic->GetShape() == SHAPE_T::SEGMENT )
                {
                    PCB_TRACK* track = new PCB_TRACK( parent );

                    track->SetLayer( targetLayer );
                    track->SetStart( graphic->GetStart() );
                    track->SetEnd( graphic->GetEnd() );
                    commit.Add( track );

                    return true;
                }
                else if( graphic->GetShape() == SHAPE_T::ARC )
                {
                    PCB_ARC* arc = new PCB_ARC( parent );

                    arc->SetLayer( targetLayer );
                    arc->SetStart( graphic->GetArcStart() );
                    arc->SetEnd( graphic->GetArcEnd() );
                    arc->SetMid( graphic->GetArcMid() );
                    commit.Add( arc );

                    return true;
                }

                return false;
            };

    for( EDA_ITEM* item : selection )
    {
        if( handleGraphicSeg( item ) )
            continue;

        SHAPE_POLY_SET   polySet = getPolySet( item );
        std::vector<SEG> segs    = getSegList( polySet );

        if( aEvent.IsAction( &PCB_ACTIONS::convertToLines ) )
        {
            for( SEG& seg : segs )
            {
                if( fpEditor )
                {
                    FP_SHAPE* graphic = new FP_SHAPE( footprint, SHAPE_T::SEGMENT );

                    graphic->SetLayer( targetLayer );
                    graphic->SetStart( wxPoint( seg.A ) );
                    graphic->SetStart0( wxPoint( seg.A ) );
                    graphic->SetEnd( wxPoint( seg.B ) );
                    graphic->SetEnd0( wxPoint( seg.B ) );
                    commit.Add( graphic );
                }
                else
                {
                    PCB_SHAPE* graphic = new PCB_SHAPE;

                    graphic->SetShape( SHAPE_T::SEGMENT );
                    graphic->SetLayer( targetLayer );
                    graphic->SetStart( wxPoint( seg.A ) );
                    graphic->SetEnd( wxPoint( seg.B ) );
                    commit.Add( graphic );
                }
            }
        }
        else
        {


            if( !IsCopperLayer( targetLayer ) )
            {
                if( copperLayer == UNSELECTED_LAYER )
                    copperLayer = frame->SelectOneLayer( F_Cu, LSET::AllNonCuMask() );

                if( copperLayer == UNDEFINED_LAYER )    // User canceled
                    continue;

                targetLayer = copperLayer;
            }

            // I am really unsure converting a polygon to "tracks" (i.e. segments on
            // copper layers) make sense for footprints, but anyway this code exists
            if( fpEditor )
            {
                // Creating segments on copper layer
                for( SEG& seg : segs )
                {
                    FP_SHAPE* graphic = new FP_SHAPE( footprint, SHAPE_T::SEGMENT );
                    graphic->SetLayer( targetLayer );
                    graphic->SetStart( wxPoint( seg.A ) );
                    graphic->SetStart0( wxPoint( seg.A ) );
                    graphic->SetEnd( wxPoint( seg.B ) );
                    graphic->SetEnd0( wxPoint( seg.B ) );
                    commit.Add( graphic );
                }
            }
            else
            {
                // Creating tracks
                for( SEG& seg : segs )
                {
                    PCB_TRACK* track = new PCB_TRACK( parent );

                    track->SetLayer( targetLayer );
                    track->SetStart( wxPoint( seg.A ) );
                    track->SetEnd( wxPoint( seg.B ) );
                    commit.Add( track );
                }
            }
        }
    }

    commit.Push( _( "Convert polygons to lines" ) );

    return 0;
}


int CONVERT_TOOL::SegmentToArc( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !( item->Type() == PCB_SHAPE_T ||
                           item->Type() == PCB_TRACE_T ||
                           item->Type() == PCB_FP_SHAPE_T ) )
                    {
                        aCollector.Remove( item );
                    }
                }
            } );

    EDA_ITEM* source = selection.Front();
    VECTOR2I start, end, mid;

    // Offset the midpoint along the normal a little bit so that it's more obviously an arc
    const double offsetRatio = 0.1;

    if( OPT<SEG> seg = getStartEndPoints( source, nullptr ) )
    {
        start = seg->A;
        end   = seg->B;

        VECTOR2I normal = ( seg->B - seg->A ).Perpendicular().Resize( offsetRatio * seg->Length() );
        mid = seg->Center() + normal;
    }
    else
    {
        return -1;
    }

    PCB_BASE_EDIT_FRAME*  frame  = getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD_ITEM_CONTAINER* parent = frame->GetModel();

    BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( source );

    // Don't continue processing if we don't actually have a board item
    if( !boardItem )
        return 0;

    PCB_LAYER_ID layer = boardItem->GetLayer();

    BOARD_COMMIT commit( m_frame );

    if( source->Type() == PCB_SHAPE_T || source->Type() == PCB_FP_SHAPE_T )
    {
        PCB_SHAPE* line = static_cast<PCB_SHAPE*>( source );
        PCB_SHAPE* arc  = new PCB_SHAPE( parent );

        VECTOR2I center = GetArcCenter( start, mid, end );

        arc->SetShape( SHAPE_T::ARC );
        arc->SetFilled( false );
        arc->SetLayer( layer );
        arc->SetWidth( line->GetWidth() );

        arc->SetCenter( wxPoint( center ) );
        arc->SetArcStart( wxPoint( start ) );
        arc->SetAngle( GetArcAngle( start, mid, end ) );

        arc->SetArcEnd( wxPoint( end ) );
        commit.Add( arc );
    }
    else
    {
        wxASSERT( source->Type() == PCB_TRACE_T );
        PCB_TRACK* line = static_cast<PCB_TRACK*>( source );
        PCB_ARC*   arc  = new PCB_ARC( parent );

        arc->SetLayer( layer );
        arc->SetWidth( line->GetWidth() );
        arc->SetStart( wxPoint( start ) );
        arc->SetMid( wxPoint( mid ) );
        arc->SetEnd( wxPoint( end ) );

        commit.Add( arc );
    }

    commit.Push( _( "Create arc from line segment" ) );

    return 0;
}


OPT<SEG> CONVERT_TOOL::getStartEndPoints( EDA_ITEM* aItem, int* aWidth )
{
    switch( aItem->Type() )
    {
    case PCB_SHAPE_T:
    case PCB_FP_SHAPE_T:
    {
        PCB_SHAPE* line = static_cast<PCB_SHAPE*>( aItem );

        if( aWidth )
            *aWidth = line->GetWidth();

        if( line->GetShape() == SHAPE_T::SEGMENT )
        {
            return boost::make_optional<SEG>( { VECTOR2I( line->GetStart() ),
                                                VECTOR2I( line->GetEnd() ) } );
        }
        else
        {
            return boost::make_optional<SEG>( { VECTOR2I( line->GetArcStart() ),
                                                VECTOR2I( line->GetArcEnd() ) } );
        }
    }

    case PCB_TRACE_T:
    {
        PCB_TRACK* line = static_cast<PCB_TRACK*>( aItem );

        if( aWidth )
            *aWidth = line->GetWidth();

        return boost::make_optional<SEG>( { VECTOR2I( line->GetStart() ),
                                            VECTOR2I( line->GetEnd() ) } );
    }

    case PCB_ARC_T:
    {
        PCB_ARC* arc = static_cast<PCB_ARC*>( aItem );

        if( aWidth )
            *aWidth = arc->GetWidth();

        return boost::make_optional<SEG>( { VECTOR2I( arc->GetStart() ),
                                            VECTOR2I( arc->GetEnd() ) } );
    }

    default:
        return NULLOPT;
    }
}


void CONVERT_TOOL::setTransitions()
{
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToPoly.MakeEvent() );
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToZone.MakeEvent() );
    Go( &CONVERT_TOOL::CreatePolys,    PCB_ACTIONS::convertToKeepout.MakeEvent() );
    Go( &CONVERT_TOOL::CreateLines,    PCB_ACTIONS::convertToLines.MakeEvent() );
    Go( &CONVERT_TOOL::CreateLines,    PCB_ACTIONS::convertToTracks.MakeEvent() );
    Go( &CONVERT_TOOL::SegmentToArc,   PCB_ACTIONS::convertToArc.MakeEvent() );
}

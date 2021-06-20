/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board_commit.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_shape.h>
#include <fp_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <collectors.h>
#include <confirm.h>
#include <menus_helpers.h>
#include <pcb_edit_frame.h>
#include <footprint_edit_frame.h>
#include <trigo.h>
#include <tool/tool_manager.h>
#include <tools/edit_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <convert_basic_shapes_to_polygon.h>

#include "convert_tool.h"


CONVERT_TOOL::CONVERT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Convert" ),
    m_selectionTool( NULL ),
    m_menu( NULL ),
    m_frame( NULL )
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
    m_menu->SetTitle( _( "Convert" ) );

    static KICAD_T convertableTracks[] = { PCB_TRACE_T, PCB_ARC_T, EOT };
    static KICAD_T zones[]  = { PCB_ZONE_T, PCB_FP_ZONE_T, EOT };

    auto graphicLines = P_S_C::OnlyGraphicShapeTypes( { PCB_SHAPE_TYPE::SEGMENT, PCB_SHAPE_TYPE::RECT,
                                            PCB_SHAPE_TYPE::CIRCLE } )
                                && P_S_C::SameLayer();

    auto trackLines   = S_C::MoreThan( 1 ) && S_C::OnlyTypes( convertableTracks )
                                && P_S_C::SameLayer();

    auto anyLines     = graphicLines || trackLines;

    auto anyPolys     = S_C::OnlyTypes( zones )
                    || P_S_C::OnlyGraphicShapeTypes(
                            { PCB_SHAPE_TYPE::POLYGON, PCB_SHAPE_TYPE::RECT } );

    auto lineToArc = S_C::Count( 1 )
                     && ( P_S_C::OnlyGraphicShapeTypes( { PCB_SHAPE_TYPE::SEGMENT } )
                                                    || S_C::OnlyType( PCB_TRACE_T ) );

    auto showConvert = anyPolys || anyLines || lineToArc;

    m_menu->AddItem( PCB_ACTIONS::convertToPoly, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToZone, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToKeepout, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToLines, anyPolys );

    // Currently the code exists, but tracks are not really existing in footprints
    // only segments on copper layers
    if( m_frame->IsType( FRAME_PCB_EDITOR ) )
        m_menu->AddItem( PCB_ACTIONS::convertToTracks, anyPolys );

    m_menu->AddItem( PCB_ACTIONS::convertToArc, lineToArc );

    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();
    selToolMenu.AddMenu( m_menu, showConvert, 100 );

    return true;
}


int CONVERT_TOOL::LinesToPoly( const TOOL_EVENT& aEvent )
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
                        case PCB_SHAPE_TYPE::SEGMENT:
                        case PCB_SHAPE_TYPE::RECT:
                        case PCB_SHAPE_TYPE::CIRCLE:
                        // case S_ARC: // Not yet
                            break;

                        default:
                            aCollector.Remove( item );
                        }

                        break;

                    case PCB_TRACE_T:
                    // case PCB_ARC_T: // Not yet
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

            graphic->SetShape( PCB_SHAPE_TYPE::POLYGON );
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
    SHAPE_POLY_SET poly;

    std::map<VECTOR2I, std::vector<EDA_ITEM*>> connections;
    std::set<EDA_ITEM*> used;
    std::deque<EDA_ITEM*> toCheck;

    for( EDA_ITEM* item : aItems )
    {
        if( OPT<SEG> seg = getStartEndPoints( item, nullptr ) )
        {
            toCheck.push_back( item );
            connections[seg->A].emplace_back( item );
            connections[seg->B].emplace_back( item );
        }
    }

    while( !toCheck.empty() )
    {
        EDA_ITEM* candidate = toCheck.front();
        toCheck.pop_front();

        if( used.count( candidate ) )
            continue;

        int      width = -1;
        OPT<SEG> seg = getStartEndPoints( candidate, &width );
        wxASSERT( seg );

        SHAPE_LINE_CHAIN     outline;
        std::deque<VECTOR2I> points;

        // aDirection == true for walking "right" and appending to the end of points
        // false for walking "left" and prepending to the beginning
        std::function<void( EDA_ITEM*, bool )> process =
                [&]( EDA_ITEM* aItem, bool aDirection )
                {
                    if( used.count( aItem ) )
                        return;

                    used.insert( aItem );

                    OPT<SEG> nextSeg = getStartEndPoints( aItem, &width );
                    wxASSERT( nextSeg );

                    // The reference point, i.e. last added point in the direction we're headed
                    VECTOR2I& ref = aDirection ? points.back() : points.front();

                    // The next point, i.e. the other point on this segment
                    VECTOR2I& next = ( ref == nextSeg->A ) ? nextSeg->B : nextSeg->A;

                    if( aDirection )
                        points.push_back( next );
                    else
                        points.push_front( next );

                    for( EDA_ITEM* neighbor : connections[next] )
                        process( neighbor, aDirection );
                };

        // Start with just one point and walk one direction
        points.push_back( seg->A );
        process( candidate, true );

        // check for any candidates on the "left"
        EDA_ITEM* left = nullptr;

        for( EDA_ITEM* possibleLeft : connections[seg->A] )
        {
            if( possibleLeft != candidate )
            {
                left = possibleLeft;
                break;
            }
        }

        if( left )
            process( left, false );

        if( points.size() < 3 )
            continue;

        for( const VECTOR2I& point : points )
            outline.Append( point );

        outline.SetClosed( true );

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

        if( graphic->GetShape() != PCB_SHAPE_TYPE::RECT )
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

        if( graphic->GetShape() != PCB_SHAPE_TYPE::CIRCLE )
            continue;

        BOARD_DESIGN_SETTINGS& bds = graphic->GetBoard()->GetDesignSettings();
        SHAPE_LINE_CHAIN outline;

        TransformCircleToPolygon( outline, graphic->GetPosition(), graphic->GetRadius(),
                                  bds.m_MaxError, ERROR_OUTSIDE );

        poly.AddOutline( outline );
    }

    return poly;
}


int CONVERT_TOOL::PolyToLines( const TOOL_EVENT& aEvent )
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
                        case PCB_SHAPE_TYPE::POLYGON:
                            break;

                        case PCB_SHAPE_TYPE::RECT:
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

                    if( graphic->GetShape() == PCB_SHAPE_TYPE::POLYGON )
                    {
                        set = graphic->GetPolyShape();
                    }
                    else if( graphic->GetShape() == PCB_SHAPE_TYPE::RECT )
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

    BOARD_COMMIT commit( m_frame );
    FOOTPRINT_EDIT_FRAME* fpEditor = dynamic_cast<FOOTPRINT_EDIT_FRAME*>( m_frame );

    FOOTPRINT* footprint = nullptr;

    if( fpEditor )
        footprint = fpEditor->GetBoard()->GetFirstFootprint();

    for( EDA_ITEM* item : selection )
    {
        PCB_LAYER_ID     layer   = static_cast<BOARD_ITEM*>( item )->GetLayer();
        SHAPE_POLY_SET   polySet = getPolySet( item );
        std::vector<SEG> segs    = getSegList( polySet );

        if( aEvent.IsAction( &PCB_ACTIONS::convertToLines ) )
        {
            for( SEG& seg : segs )
            {
                if( fpEditor )
                {
                    FP_SHAPE* graphic = new FP_SHAPE( footprint, PCB_SHAPE_TYPE::SEGMENT );

                    graphic->SetLayer( layer );
                    graphic->SetStart( wxPoint( seg.A ) );
                    graphic->SetStart0( wxPoint( seg.A ) );
                    graphic->SetEnd( wxPoint( seg.B ) );
                    graphic->SetEnd0( wxPoint( seg.B ) );
                    commit.Add( graphic );
                }
                else
                {
                    PCB_SHAPE* graphic = new PCB_SHAPE;

                    graphic->SetShape( PCB_SHAPE_TYPE::SEGMENT );
                    graphic->SetLayer( layer );
                    graphic->SetStart( wxPoint( seg.A ) );
                    graphic->SetEnd( wxPoint( seg.B ) );
                    commit.Add( graphic );
                }
            }
        }
        else
        {
            PCB_BASE_EDIT_FRAME*  frame  = getEditFrame<PCB_BASE_EDIT_FRAME>();
            BOARD_ITEM_CONTAINER* parent = frame->GetModel();

            if( !IsCopperLayer( layer ) )
                layer = frame->SelectOneLayer( F_Cu, LSET::AllNonCuMask() );

            // I am really unsure converting a polygon to "tracks" (i.e. segments on
            // copper layers) make sense for footprints, but anyway this code exists
            if( fpEditor )
            {
                // Creating segments on copper layer
                for( SEG& seg : segs )
                {
                    FP_SHAPE* graphic = new FP_SHAPE( footprint, PCB_SHAPE_TYPE::SEGMENT );
                    graphic->SetLayer( layer );
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

                    track->SetLayer( layer );
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

        arc->SetShape( PCB_SHAPE_TYPE::ARC );
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

        return boost::make_optional<SEG>( { VECTOR2I( line->GetStart() ),
                                            VECTOR2I( line->GetEnd() ) } );
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
    Go( &CONVERT_TOOL::LinesToPoly,    PCB_ACTIONS::convertToPoly.MakeEvent() );
    Go( &CONVERT_TOOL::LinesToPoly,    PCB_ACTIONS::convertToZone.MakeEvent() );
    Go( &CONVERT_TOOL::LinesToPoly,    PCB_ACTIONS::convertToKeepout.MakeEvent() );
    Go( &CONVERT_TOOL::PolyToLines,    PCB_ACTIONS::convertToLines.MakeEvent() );
    Go( &CONVERT_TOOL::PolyToLines,    PCB_ACTIONS::convertToTracks.MakeEvent() );
    Go( &CONVERT_TOOL::SegmentToArc,   PCB_ACTIONS::convertToArc.MakeEvent() );
}

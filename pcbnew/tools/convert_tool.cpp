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
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_track.h>
#include <class_zone.h>
#include <collectors.h>
#include <confirm.h>
#include <menus_helpers.h>
#include <pcb_edit_frame.h>
#include <trigo.h>
#include <tool/tool_manager.h>
#include <tools/edit_tool.h>
#include <tools/pcb_actions.h>
#include <tools/selection_tool.h>

#include "convert_tool.h"


TOOL_ACTION PCB_ACTIONS::convertToPoly( "pcbnew.Convert.convertToPoly",
        AS_GLOBAL, 0, "", _( "Convert to Polygon" ),
        _( "Creates a graphic polygon from the selection" ), add_graphical_polygon_xpm );

TOOL_ACTION PCB_ACTIONS::convertToZone( "pcbnew.Convert.convertToZone",
        AS_GLOBAL, 0, "", _( "Convert to Zone" ), _( "Creates a copper zone from the selection" ),
        add_zone_xpm );

TOOL_ACTION PCB_ACTIONS::convertToKeepout( "pcbnew.Convert.convertToKeepout",
        AS_GLOBAL, 0, "", _( "Convert to Keepout" ),
        _( "Creates a keepout zone from the selection" ), add_keepout_area_xpm );

TOOL_ACTION PCB_ACTIONS::convertToLines( "pcbnew.Convert.convertToLines",
        AS_GLOBAL, 0, "", _( "Convert to Lines" ), _( "Creates graphic lines from the selection" ),
        add_line_xpm );

TOOL_ACTION PCB_ACTIONS::convertToArc( "pcbnew.Convert.convertToArc",
        AS_GLOBAL, 0, "", _( "Convert to Arc" ), _( "Converts selected line segment to an arc" ),
        add_arc_xpm );

TOOL_ACTION PCB_ACTIONS::convertToTracks( "pcbnew.Convert.convertToTracks",
        AS_GLOBAL, 0, "", _( "Convert to Tracks" ),
        _( "Converts selected graphic lines to tracks" ), add_tracks_xpm );


CONVERT_TOOL::CONVERT_TOOL() :
    TOOL_INTERACTIVE( "pcbnew.Convert" ), m_selectionTool( NULL ),
    m_menu( NULL ), m_frame( NULL )
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
    m_selectionTool =
            static_cast<SELECTION_TOOL*>( m_toolMgr->FindTool( "pcbnew.InteractiveSelection" ) );

    if( !m_selectionTool )
    {
        DisplayError( NULL, wxT( "pcbnew.InteractiveSelection tool is not available" ) );
        return false;
    }

    m_frame = getEditFrame<PCB_BASE_FRAME>();

    // Create a context menu and make it available through selection tool
    m_menu = new CONDITIONAL_MENU( this );
    m_menu->SetIcon( refresh_xpm );
    m_menu->SetTitle( _( "Convert" ) );

    static KICAD_T convertableTracks[] = { PCB_TRACE_T, PCB_ARC_T, EOT };

    auto graphicLines = P_S_C::OnlyGraphicShapeTypes( { S_SEGMENT, S_RECT } ) && P_S_C::SameLayer();

    auto trackLines   = S_C::MoreThan( 1 ) &&
                        S_C::OnlyTypes( convertableTracks ) && P_S_C::SameLayer();

    auto anyLines     = graphicLines || trackLines;

    auto anyPolys     = ( S_C::OnlyType( PCB_ZONE_AREA_T ) ||
                          P_S_C::OnlyGraphicShapeTypes( { S_POLYGON } ) );

    auto showConvert = anyPolys || anyLines;

    m_menu->AddItem( PCB_ACTIONS::convertToPoly, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToZone, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToKeepout, anyLines );
    m_menu->AddItem( PCB_ACTIONS::convertToLines, anyPolys );
    m_menu->AddItem( PCB_ACTIONS::convertToTracks, anyPolys );
    // Not yet
    // m_menu->AddItem( PCB_ACTIONS::convertToArc, lineToArc );

    for( std::shared_ptr<ACTION_MENU>& subMenu : m_selectionTool->GetToolMenu().GetSubMenus() )
    {
        if( dynamic_cast<SPECIAL_TOOLS_CONTEXT_MENU*>( subMenu.get() ) )
            static_cast<CONDITIONAL_MENU*>( subMenu.get() )->AddMenu( m_menu, showConvert );
    }

    return true;
}


int CONVERT_TOOL::LinesToPoly( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, SELECTION_TOOL* sTool )
        {
            EditToolSelectionFilter( aCollector,
                                     EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS, sTool );

            for( int i = aCollector.GetCount() - 1; i >= 0; --i )
            {
                BOARD_ITEM* item = aCollector[i];

                switch( item->Type() )
                {
                case PCB_LINE_T:
                    switch( static_cast<DRAWSEGMENT*>( item )->GetShape() )
                    {
                    case S_SEGMENT:
                    case S_RECT:
                    //case S_ARC: // Not yet
                        break;

                    default:
                        aCollector.Remove( item );
                    }

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

    // TODO(JE) From a context menu, the selection condition enforces that the items are on
    // a single layer.  But, you can still trigger this with items on multiple layer selected.
    // Technically we should make this work if each contiguous poly shares a layer
    PCB_LAYER_ID destLayer = static_cast<BOARD_ITEM*>( selection.Front() )->GetLayer();

    SHAPE_POLY_SET polySet = makePolysFromSegs( selection.GetItems() );

    polySet.Append( makePolysFromRects( selection.GetItems() ) );

    if( polySet.IsEmpty() )
        return 0;

    BOARD_COMMIT commit( m_frame );

    // For now, we convert each outline in the returned shape to its own polygon
    std::vector<SHAPE_POLY_SET> polys;

    for( int i = 0; i < polySet.OutlineCount(); i++ )
        polys.emplace_back( SHAPE_POLY_SET( polySet.COutline( i ) ) );

    if( aEvent.IsAction( &PCB_ACTIONS::convertToPoly ) )
    {
        for( const SHAPE_POLY_SET& poly : polys )
        {
            DRAWSEGMENT* graphic = new DRAWSEGMENT;

            graphic->SetShape( S_POLYGON );
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

        int ret;

        if( aEvent.IsAction( &PCB_ACTIONS::convertToKeepout ) )
            ret = InvokeKeepoutAreaEditor( frame, &zoneInfo );
        else
            ret = InvokeCopperZonesEditor( frame, &zoneInfo );

        if( ret == wxID_CANCEL )
            return 0;

        for( const SHAPE_POLY_SET& poly : polys )
        {
            ZONE_CONTAINER* zone = new ZONE_CONTAINER( parent );

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
        if( OPT<SEG> seg = getStartEndPoints( item ) )
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

        OPT<SEG> seg = getStartEndPoints( candidate );
        wxASSERT( seg );

        SHAPE_LINE_CHAIN outline;
        std::deque<VECTOR2I> points;

        // aDirection == true for walking "right" and appending to the end of points
        // false for walking "left" and prepending to the beginning
        std::function<void( EDA_ITEM*, bool )> process =
                [&]( EDA_ITEM* aItem, bool aDirection )
                {
                    if( used.count( aItem ) )
                        return;

                    used.insert( aItem );

                    OPT<SEG> nextSeg = getStartEndPoints( aItem );
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

        poly.AddOutline( outline );
    }

    return poly;
}


SHAPE_POLY_SET CONVERT_TOOL::makePolysFromRects( const std::deque<EDA_ITEM*>& aItems )
{
    SHAPE_POLY_SET poly;

    for( EDA_ITEM* item : aItems )
    {
        if( item->Type() != PCB_LINE_T )
            continue;

        DRAWSEGMENT* graphic = static_cast<DRAWSEGMENT*>( item );

        if( graphic->GetShape() != S_RECT )
            continue;

        SHAPE_LINE_CHAIN outline;
        VECTOR2I start( graphic->GetStart() );
        VECTOR2I end( graphic->GetEnd() );

        outline.Append( start );
        outline.Append( VECTOR2I( end.x, start.y ) );
        outline.Append( end );
        outline.Append( VECTOR2I( start.x, end.y ) );
        outline.SetClosed( true );

        poly.AddOutline( outline );
    }

    return poly;
}


int CONVERT_TOOL::PolyToLines( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
        []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, SELECTION_TOOL* sTool )
        {
            EditToolSelectionFilter( aCollector,
                                     EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS, sTool );

            for( int i = aCollector.GetCount() - 1; i >= 0; --i )
            {
                BOARD_ITEM* item = aCollector[i];

                switch( item->Type() )
                {
                case PCB_LINE_T:
                    switch( static_cast<DRAWSEGMENT*>( item )->GetShape() )
                    {
                    case S_POLYGON:
                        break;

                    default:
                        aCollector.Remove( item );
                    }

                case PCB_ZONE_AREA_T:
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
                case PCB_ZONE_AREA_T:
                    set = *static_cast<ZONE_CONTAINER*>( aItem )->Outline();
                    break;

                case PCB_LINE_T:
                    wxASSERT( static_cast<DRAWSEGMENT*>( aItem )->GetShape() == S_POLYGON );
                    set = static_cast<DRAWSEGMENT*>( aItem )->GetPolyShape();
                    break;

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

    for( EDA_ITEM* item : selection )
    {
        PCB_LAYER_ID     layer   = static_cast<BOARD_ITEM*>( item )->GetLayer();
        SHAPE_POLY_SET   polySet = getPolySet( item );
        std::vector<SEG> segs    = getSegList( polySet );

        if( aEvent.IsAction( &PCB_ACTIONS::convertToLines ) )
        {
            for( SEG& seg : segs )
            {
                DRAWSEGMENT* graphic = new DRAWSEGMENT;

                graphic->SetShape( S_SEGMENT );
                graphic->SetLayer( layer );
                graphic->SetStart( wxPoint( seg.A ) );
                graphic->SetEnd( wxPoint( seg.B ) );

                commit.Add( graphic );
            }
        }
        else
        {
            PCB_BASE_EDIT_FRAME*  frame  = getEditFrame<PCB_BASE_EDIT_FRAME>();
            BOARD_ITEM_CONTAINER* parent = frame->GetModel();

            if( !IsCopperLayer( layer ) )
                layer = frame->SelectLayer( F_Cu, LSET::AllNonCuMask() );

            // Creating tracks
            for( SEG& seg : segs )
            {
                TRACK* track = new TRACK( parent );

                track->SetLayer( layer );
                track->SetStart( wxPoint( seg.A ) );
                track->SetEnd( wxPoint( seg.B ) );

                commit.Add( track );
            }
        }
    }

    commit.Push( _( "Convert polygons to lines" ) );

    return 0;
}


int CONVERT_TOOL::SegmentToArc( const TOOL_EVENT& aEvent )
{
    auto& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, SELECTION_TOOL* sTool )
            {
                EditToolSelectionFilter( aCollector,
                                         EXCLUDE_LOCKED | EXCLUDE_TRANSIENTS, sTool );

                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !( item->Type() == PCB_LINE_T || item->Type() == PCB_TRACE_T ) )
                        aCollector.Remove( item );
                }
            } );

    EDA_ITEM* source = selection.Front();
    wxPoint start, end, mid;

    if( OPT<SEG> optSeg = getStartEndPoints( source ) )
    {
        start = wxPoint( optSeg->A );
        end   = wxPoint( optSeg->B );
        mid   = wxPoint( optSeg->Center() );
    }
    else
        return -1;

    PCB_BASE_EDIT_FRAME*  frame  = getEditFrame<PCB_BASE_EDIT_FRAME>();
    BOARD_ITEM_CONTAINER* parent = frame->GetModel();

    BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( source );
    wxASSERT( boardItem );

    PCB_LAYER_ID layer = boardItem->GetLayer();

    BOARD_COMMIT commit( m_frame );

    if( source->Type() == PCB_LINE_T )
    {
        DRAWSEGMENT* line = static_cast<DRAWSEGMENT*>( source );
        DRAWSEGMENT* arc  = new DRAWSEGMENT( parent );

        wxPoint center = GetArcCenter( start, mid, end );

        arc->SetShape( S_ARC );
        arc->SetLayer( layer );
        arc->SetWidth( line->GetWidth() );
        arc->SetArcStart( start );
        arc->SetCenter( center );
        // TODO(JE): once !325 is merged
        //arc->SetArcEnd( end );

        commit.Add( arc );
    }
    else
    {
        wxASSERT( source->Type() == PCB_TRACE_T );
        TRACK* line = static_cast<TRACK*>( source );
        ARC*   arc  = new ARC( parent );

        arc->SetLayer( layer );
        arc->SetWidth( line->GetWidth() );
        arc->SetStart( start );
        arc->SetMid( mid );
        arc->SetEnd( end );

        commit.Add( arc );
    }

    commit.Push( _( "Create arc from line segment" ) );

    return 0;
}


OPT<SEG> CONVERT_TOOL::getStartEndPoints( EDA_ITEM* aItem )
{
    switch( aItem->Type() )
    {
    case PCB_LINE_T:
    {
        DRAWSEGMENT* line = static_cast<DRAWSEGMENT*>( aItem );
        return boost::make_optional<SEG>( { VECTOR2I( line->GetStart() ),
                                            VECTOR2I( line->GetEnd() ) } );
    }

    case PCB_TRACE_T:
    {
        TRACK* line = static_cast<TRACK*>( aItem );
        return boost::make_optional<SEG>( { VECTOR2I( line->GetStart() ),
                                            VECTOR2I( line->GetEnd() ) } );
    }

    case PCB_ARC_T:
    {
        ARC* arc = static_cast<ARC*>( aItem );
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

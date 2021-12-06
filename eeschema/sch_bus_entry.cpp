/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <schematic.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <sch_text.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/color_settings.h>
#include <netclass.h>
#include <trigo.h>
#include <board_item.h>
#include <advanced_config.h>
#include <connection_graph.h>
#include "sch_painter.h"


SCH_BUS_ENTRY_BASE::SCH_BUS_ENTRY_BASE( KICAD_T aType, const wxPoint& pos, bool aFlipY ) :
    SCH_ITEM( nullptr, aType )
{
    m_pos    = pos;
    m_size.x = Mils2iu( DEFAULT_SCH_ENTRY_SIZE );
    m_size.y = Mils2iu( DEFAULT_SCH_ENTRY_SIZE );

    m_stroke.SetWidth( 0 );
    m_stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    m_stroke.SetColor( COLOR4D::UNSPECIFIED );

    if( aFlipY )
        m_size.y *= -1;

    m_isDanglingStart = m_isDanglingEnd = true;

    m_lastResolvedWidth = Mils2iu( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_WIRE_ENTRY::SCH_BUS_WIRE_ENTRY( const wxPoint& pos, bool aFlipY ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_WIRE_ENTRY_T, pos, aFlipY )
{
    m_layer  = LAYER_WIRE;
    m_connected_bus_item = nullptr;

    m_lastResolvedWidth = Mils2iu( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_WIRE_ENTRY::SCH_BUS_WIRE_ENTRY( const wxPoint& pos, int aQuadrant ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_WIRE_ENTRY_T, pos, false )
{
    switch( aQuadrant )
    {
    case 1: m_size.x *=  1; m_size.y *= -1; break;
    case 2: m_size.x *=  1; m_size.y *=  1; break;
    case 3: m_size.x *= -1; m_size.y *=  1; break;
    case 4: m_size.x *= -1; m_size.y *= -1; break;
    default: wxFAIL_MSG( "SCH_BUS_WIRE_ENTRY ctor: unexpected quadrant" );
    }

    m_layer  = LAYER_WIRE;
    m_connected_bus_item = nullptr;

    m_lastResolvedWidth = Mils2iu( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_BUS_ENTRY::SCH_BUS_BUS_ENTRY( const wxPoint& pos, bool aFlipY ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_BUS_ENTRY_T, pos, aFlipY )
{
    m_layer = LAYER_BUS;
    m_connected_bus_items[0] = nullptr;
    m_connected_bus_items[1] = nullptr;

    m_lastResolvedWidth = Mils2iu( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


EDA_ITEM* SCH_BUS_WIRE_ENTRY::Clone() const
{
    return new SCH_BUS_WIRE_ENTRY( *this );
}


EDA_ITEM* SCH_BUS_BUS_ENTRY::Clone() const
{
    return new SCH_BUS_BUS_ENTRY( *this );
}


bool SCH_BUS_ENTRY_BASE::doIsConnected( const wxPoint& aPosition ) const
{
    return ( m_pos == aPosition || GetEnd() == aPosition );
}


wxPoint SCH_BUS_ENTRY_BASE::GetEnd() const
{
    return wxPoint( m_pos.x + m_size.x, m_pos.y + m_size.y );
}


void SCH_BUS_ENTRY_BASE::SwapData( SCH_ITEM* aItem )
{
    SCH_BUS_ENTRY_BASE* item = dynamic_cast<SCH_BUS_ENTRY_BASE*>( aItem );
    wxCHECK_RET( item, wxT( "Cannot swap bus entry data with invalid item." ) );

    std::swap( m_pos, item->m_pos );
    std::swap( m_size, item->m_size );
    std::swap( m_stroke, item->m_stroke );

    std::swap( m_lastResolvedWidth, item->m_lastResolvedWidth );
    std::swap( m_lastResolvedLineStyle, item->m_lastResolvedLineStyle );
    std::swap( m_lastResolvedColor, item->m_lastResolvedColor );
}


void SCH_BUS_ENTRY_BASE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 3;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = Type() == SCH_BUS_BUS_ENTRY_T ? LAYER_BUS : LAYER_WIRE;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT SCH_BUS_ENTRY_BASE::GetBoundingBox() const
{
    EDA_RECT box;

    box.SetOrigin( m_pos );
    box.SetEnd( GetEnd() );

    box.Normalize();
    box.Inflate( ( GetPenWidth() / 2 ) + 1 );

    return box;
}


COLOR4D SCH_BUS_ENTRY_BASE::GetStrokeColor() const
{
    if( m_stroke.GetColor() != COLOR4D::UNSPECIFIED )
    {
        m_lastResolvedColor = m_stroke.GetColor();
    }
    else if( IsConnectable() && !IsConnectivityDirty() )
    {
        NETCLASSPTR netclass = NetClass();

        if( netclass )
            m_lastResolvedColor = netclass->GetSchematicColor();
    }

    return m_lastResolvedColor;
}


PLOT_DASH_TYPE SCH_BUS_ENTRY_BASE::GetStrokeStyle() const
{
    if( m_stroke.GetPlotStyle() != PLOT_DASH_TYPE::DEFAULT )
    {
        m_lastResolvedLineStyle = m_stroke.GetPlotStyle();
    }
    else if( IsConnectable() && !IsConnectivityDirty() )
    {
        NETCLASSPTR netclass = NetClass();

        if( netclass )
            m_lastResolvedLineStyle = static_cast<PLOT_DASH_TYPE>( netclass->GetLineStyle() );
    }

    return m_lastResolvedLineStyle;
}


int SCH_BUS_WIRE_ENTRY::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
    {
        m_lastResolvedWidth = m_stroke.GetWidth();
    }
    else if( IsConnectable() && !IsConnectivityDirty() )
    {
        NETCLASSPTR netclass = NetClass();

        if( netclass )
            m_lastResolvedWidth = netclass->GetWireWidth();
    }

    return m_lastResolvedWidth;
}


int SCH_BUS_BUS_ENTRY::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
    {
        m_lastResolvedWidth = m_stroke.GetWidth();
    }
    else if( IsConnectable() && !IsConnectivityDirty() )
    {
        NETCLASSPTR netclass = NetClass();

        if( netclass )
            m_lastResolvedWidth = netclass->GetBusWidth();
    }

    return m_lastResolvedWidth;
}


void SCH_BUS_WIRE_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( WIRE_ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( WIRE_ENTRY_END, this, GetEnd() );
    aItemList.push_back( item1 );
}


void SCH_BUS_BUS_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( BUS_ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( BUS_ENTRY_END, this, GetEnd() );
    aItemList.push_back( item1 );
}


void SCH_BUS_ENTRY_BASE::Print( const RENDER_SETTINGS* aSettings, const wxPoint& aOffset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = ( GetStrokeColor() == COLOR4D::UNSPECIFIED ) ?
                    aSettings->GetLayerColor( m_layer ) : GetStrokeColor();
    wxPoint start = m_pos + aOffset;
    wxPoint end = GetEnd() + aOffset;
    int     penWidth = ( GetPenWidth() == 0 ) ? aSettings->GetDefaultPenWidth() : GetPenWidth();

    if( GetStrokeStyle() <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        GRLine( nullptr, DC, start.x, start.y, end.x, end.y, penWidth, color );
    }
    else
    {
        EDA_RECT clip( (wxPoint) start, wxSize( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );
        double strokes[] = { 1.0, dash_gap_len( penWidth ), 1.0, dash_gap_len( penWidth ) };

        switch( GetStrokeStyle() )
        {
        default:
        case PLOT_DASH_TYPE::DASH:
            strokes[0] = strokes[2] = dash_mark_len( penWidth );
            break;
        case PLOT_DASH_TYPE::DOT:
            strokes[0] = strokes[2] = dot_mark_len( penWidth );
            break;
        case PLOT_DASH_TYPE::DASHDOT:
            strokes[0] = dash_mark_len( penWidth );
            strokes[2] = dot_mark_len( penWidth );
            break;
        }

        for( size_t i = 0; i < 10000; ++i )
        {
            // Calculations MUST be done in doubles to keep from accumulating rounding
            // errors as we go.
            wxPoint next( start.x + strokes[ i % 4 ] * cos( theta ),
                          start.y + strokes[ i % 4 ] * sin( theta ) );

            // Drawing each segment can be done rounded to ints.
            wxPoint segStart( KiROUND( start.x ), KiROUND( start.y ) );
            wxPoint segEnd( KiROUND( next.x ), KiROUND( next.y ) );

            if( ClipLine( &clip, segStart.x, segStart.y, segEnd.x, segEnd.y ) )
                break;
            else if( i % 2 == 0 )
                GRLine( nullptr, DC, segStart.x, segStart.y, segEnd.x, segEnd.y, penWidth, color );

            start = next;
        }
    }
}


void SCH_BUS_ENTRY_BASE::MirrorVertically( int aCenter )
{
    MIRROR( m_pos.y, aCenter );
    m_size.y = -m_size.y;
}


void SCH_BUS_ENTRY_BASE::MirrorHorizontally( int aCenter )
{
    MIRROR( m_pos.x, aCenter );
    m_size.x = -m_size.x;
}


void SCH_BUS_ENTRY_BASE::Rotate( const wxPoint& aCenter )
{
    RotatePoint( &m_pos, aCenter, 900 );
    RotatePoint( &m_size.x, &m_size.y, 900 );
}


bool SCH_BUS_WIRE_ENTRY::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                              const SCH_SHEET_PATH* aPath )
{
    bool previousStateStart = m_isDanglingStart;
    bool previousStateEnd = m_isDanglingEnd;

    m_isDanglingStart = m_isDanglingEnd = true;

    // Store the connection type and state for the start (0) and end (1)
    bool has_wire[2] = { false };
    bool has_bus[2] = { false };

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case WIRE_END:
            if( m_pos == item.GetPosition() )
                has_wire[0] = true;
            else if( GetEnd() == item.GetPosition() )
                has_wire[1] = true;

            break;

        case BUS_END:
        {
            // The bus has created 2 DANGLING_END_ITEMs, one per end.
            DANGLING_END_ITEM& nextItem = aItemList[++ii];

            if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), m_pos ) )
                has_bus[0] = true;
            else if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), GetEnd() ) )
                has_bus[1] = true;
        }
            break;

        default:
            break;
        }
    }

    // A bus-wire entry is connected at both ends if it has a bus and a wire on its
    // ends.  Otherwise, we connect only one end (in the case of a wire-wire or bus-bus)
    if( ( has_wire[0] && has_bus[1] ) || ( has_wire[1] && has_bus[0] ) )
        m_isDanglingEnd = m_isDanglingStart = false;
    else if( has_wire[0] || has_bus[0] )
        m_isDanglingStart = false;
    else if( has_wire[1] || has_bus[1] )
        m_isDanglingEnd = false;

    return (previousStateStart != m_isDanglingStart) || (previousStateEnd != m_isDanglingEnd);
}


bool SCH_BUS_BUS_ENTRY::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                             const SCH_SHEET_PATH* aPath )
{
    bool previousStateStart = m_isDanglingStart;
    bool previousStateEnd = m_isDanglingEnd;

    m_isDanglingStart = m_isDanglingEnd = true;

    for( unsigned ii = 0; ii < aItemList.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemList[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case BUS_END:
        {
            // The bus has created 2 DANGLING_END_ITEMs, one per end.
            DANGLING_END_ITEM& nextItem = aItemList[++ii];

            if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), m_pos ) )
                m_isDanglingStart = false;
            if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), GetEnd() ) )
                m_isDanglingEnd = false;
        }
            break;

        default:
            break;
        }
    }

    return (previousStateStart != m_isDanglingStart) || (previousStateEnd != m_isDanglingEnd);
}


bool SCH_BUS_ENTRY_BASE::IsDangling() const
{
    return m_isDanglingStart || m_isDanglingEnd;
}


std::vector<wxPoint> SCH_BUS_ENTRY_BASE::GetConnectionPoints() const
{
    return { m_pos, GetEnd() };
}


wxString SCH_BUS_WIRE_ENTRY::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString( _( "Bus to Wire Entry" ) );
}


wxString SCH_BUS_BUS_ENTRY::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString( _( "Bus to Bus Entry" ) );
}


BITMAPS SCH_BUS_WIRE_ENTRY::GetMenuImage() const
{
    return BITMAPS::add_line2bus;
}


BITMAPS SCH_BUS_BUS_ENTRY::GetMenuImage() const
{
    return BITMAPS::add_bus2bus;
}


bool SCH_BUS_ENTRY_BASE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Insure minimum accuracy
    if( aAccuracy == 0 )
        aAccuracy = ( GetPenWidth() / 2 ) + 4;

    return TestSegmentHit( aPosition, m_pos, GetEnd(), aAccuracy );
}


bool SCH_BUS_ENTRY_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_BUS_ENTRY_BASE::Plot( PLOTTER* aPlotter ) const
{
    auto* settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );

    COLOR4D color = ( GetStrokeColor() == COLOR4D::UNSPECIFIED ) ?
                    settings->GetLayerColor( m_layer ) : GetStrokeColor();
    int     penWidth = ( GetPenWidth() == 0 ) ? settings->GetDefaultPenWidth() : GetPenWidth();

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetColor( color );
    aPlotter->SetDash( GetStrokeStyle() );
    aPlotter->MoveTo( m_pos );
    aPlotter->FinishTo( GetEnd() );
}


void SCH_BUS_ENTRY_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    switch( GetLayer() )
    {
    default:
    case LAYER_WIRE: msg = _( "Wire" ); break;
    case LAYER_BUS:  msg = _( "Bus" );  break;
    }

    aList.emplace_back( _( "Bus Entry Type" ), msg );

    SCH_CONNECTION* conn = nullptr;

    if( !IsConnectivityDirty() && dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
        conn = Connection();

    if( conn )
    {
        conn->AppendInfoToMsgPanel( aList );

        if( !conn->IsBus() )
        {
            NET_SETTINGS& netSettings = Schematic()->Prj().GetProjectFile().NetSettings();
            wxString netname = conn->Name();
            wxString netclassName = netSettings.m_NetClasses.GetDefaultPtr()->GetName();

            if( netSettings.m_NetClassAssignments.count( netname ) )
                netclassName = netSettings.m_NetClassAssignments[ netname ];

            aList.emplace_back( _( "Assigned Netclass" ), netclassName );
        }
    }
}


bool SCH_BUS_ENTRY_BASE::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    auto symbol = static_cast<const SCH_BUS_ENTRY_BASE*>( &aItem );

    if( GetLayer() != symbol->GetLayer() )
        return GetLayer() < symbol->GetLayer();

    if( GetPosition().x != symbol->GetPosition().x )
        return GetPosition().x < symbol->GetPosition().x;

    if( GetPosition().y != symbol->GetPosition().y )
        return GetPosition().y < symbol->GetPosition().y;

    if( GetEnd().x != symbol->GetEnd().x )
        return GetEnd().x < symbol->GetEnd().x;

    return GetEnd().y < symbol->GetEnd().y;
}


bool SCH_BUS_WIRE_ENTRY::ConnectionPropagatesTo( const EDA_ITEM* aItem ) const
{
    // Don't generate connections between bus entries and buses, since there is
    // a connectivity change at that point (e.g. A[7..0] to A7)
    if( ( aItem->Type() == SCH_LINE_T ) &&
        ( static_cast<const SCH_LINE*>( aItem )->GetLayer() == LAYER_BUS ) )
    {
        return false;
    }

    // Same for bus junctions
    if( ( aItem->Type() == SCH_JUNCTION_T ) &&
        ( static_cast<const SCH_JUNCTION*>( aItem )->GetLayer() == LAYER_BUS_JUNCTION ) )
    {
        return false;
    }

    // Don't generate connections between bus entries and bus labels that happen
    // to land at the same point on the bus wire as this bus entry
    if( ( aItem->Type() == SCH_LABEL_T ) &&
        SCH_CONNECTION::IsBusLabel( static_cast<const SCH_LABEL*>( aItem )->GetText() ) )
    {
        return false;
    }

    // Don't generate connections between two bus-wire entries
    if( aItem->Type() == SCH_BUS_WIRE_ENTRY_T )
        return false;

    return true;
}

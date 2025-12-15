/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <sch_draw_panel.h>
#include <bitmaps.h>
#include <core/mirror.h>
#include <schematic.h>
#include <geometry/shape_segment.h>
#include <geometry/geometry_utils.h>
#include <sch_bus_entry.h>
#include <sch_edit_frame.h>
#include <sch_junction.h>
#include <sch_line.h>
#include <project/net_settings.h>
#include <project/project_file.h>
#include <settings/color_settings.h>
#include <netclass.h>
#include <trigo.h>
#include <board_item.h>
#include <connection_graph.h>
#include "sch_painter.h"
#include "plotters/plotter.h"


SCH_BUS_ENTRY_BASE::SCH_BUS_ENTRY_BASE( KICAD_T aType, const VECTOR2I& pos, bool aFlipY ) :
    SCH_ITEM( nullptr, aType )
{
    m_pos    = pos;
    m_size.x = schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE );
    m_size.y = schIUScale.MilsToIU( DEFAULT_SCH_ENTRY_SIZE );

    m_stroke.SetWidth( 0 );
    m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
    m_stroke.SetColor( COLOR4D::UNSPECIFIED );

    if( aFlipY )
        m_size.y *= -1;

    m_isStartDangling = m_isEndDangling = true;

    m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = LINE_STYLE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_WIRE_ENTRY::SCH_BUS_WIRE_ENTRY( const VECTOR2I& pos, bool aFlipY ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_WIRE_ENTRY_T, pos, aFlipY )
{
    m_layer  = LAYER_WIRE;
    m_connected_bus_item = nullptr;

    m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = LINE_STYLE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_WIRE_ENTRY::SCH_BUS_WIRE_ENTRY( const VECTOR2I& pos, int aQuadrant ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_WIRE_ENTRY_T, pos, false )
{
    switch( aQuadrant )
    {
    case 1: m_size.x *=  1; m_size.y *= -1; break;
    case 2: m_size.x *=  1; m_size.y *=  1; break;
    case 3: m_size.x *= -1; m_size.y *=  1; break;
    case 4: m_size.x *= -1; m_size.y *= -1; break;
    default: wxFAIL_MSG( wxS( "SCH_BUS_WIRE_ENTRY ctor: unexpected quadrant" ) );
    }

    m_layer  = LAYER_WIRE;
    m_connected_bus_item = nullptr;

    m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = LINE_STYLE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_BUS_BUS_ENTRY::SCH_BUS_BUS_ENTRY( const VECTOR2I& pos, bool aFlipY ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_BUS_ENTRY_T, pos, aFlipY )
{
    m_layer = LAYER_BUS;
    m_connected_bus_items[0] = nullptr;
    m_connected_bus_items[1] = nullptr;

    m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS );
    m_lastResolvedLineStyle = LINE_STYLE::SOLID;
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


bool SCH_BUS_ENTRY_BASE::doIsConnected( const VECTOR2I& aPosition ) const
{
    return ( m_pos == aPosition || GetEnd() == aPosition );
}


VECTOR2I SCH_BUS_ENTRY_BASE::GetEnd() const
{
    return VECTOR2I( m_pos.x + m_size.x, m_pos.y + m_size.y );
}


void SCH_BUS_ENTRY_BASE::swapData( SCH_ITEM* aItem )
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


std::vector<int> SCH_BUS_ENTRY_BASE::ViewGetLayers() const
{
    if( Type() == SCH_BUS_BUS_ENTRY_T )
        return { LAYER_BUS, LAYER_NET_COLOR_HIGHLIGHT, LAYER_SELECTION_SHADOWS };

    return { LAYER_WIRE, LAYER_NET_COLOR_HIGHLIGHT, LAYER_SELECTION_SHADOWS };
}


const BOX2I SCH_BUS_ENTRY_BASE::GetBoundingBox() const
{
    BOX2I bbox( m_pos );
    bbox.SetEnd( GetEnd() );

    bbox.Normalize();
    bbox.Inflate( ( GetPenWidth() / 2 ) + 1 );

    return bbox;
}


COLOR4D SCH_BUS_ENTRY_BASE::GetBusEntryColor() const
{
    if( m_stroke.GetColor() != COLOR4D::UNSPECIFIED )
        m_lastResolvedColor = m_stroke.GetColor();
    else if( IsConnectable() && !IsConnectivityDirty() )
        m_lastResolvedColor = GetEffectiveNetClass()->GetSchematicColor();

    return m_lastResolvedColor;
}


void SCH_BUS_ENTRY_BASE::SetPenWidth( int aWidth )
{
    m_stroke.SetWidth( aWidth );
    m_lastResolvedWidth = aWidth;
}


int SCH_BUS_ENTRY_BASE::GetPenWidth() const
{
    return m_lastResolvedWidth;
}


void SCH_BUS_ENTRY_BASE::SetBusEntryColor( const COLOR4D& aColor )
{
    m_stroke.SetColor( aColor );
    m_lastResolvedColor = aColor;
}


LINE_STYLE SCH_BUS_ENTRY_BASE::GetEffectiveLineStyle() const
{
    if( m_stroke.GetLineStyle() != LINE_STYLE::DEFAULT )
        m_lastResolvedLineStyle = m_stroke.GetLineStyle();
    else if( IsConnectable() && !IsConnectivityDirty() )
        m_lastResolvedLineStyle = (LINE_STYLE) GetEffectiveNetClass()->GetLineStyle();

    return m_lastResolvedLineStyle;
}


void SCH_BUS_ENTRY_BASE::SetLineStyle( LINE_STYLE aStyle )
{
    m_stroke.SetLineStyle( aStyle );
    m_lastResolvedLineStyle = aStyle;
}


int SCH_BUS_WIRE_ENTRY::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
        m_lastResolvedWidth = m_stroke.GetWidth();
    else if( IsConnectable() && !IsConnectivityDirty() )
        m_lastResolvedWidth = GetEffectiveNetClass()->GetWireWidth();

    return m_lastResolvedWidth;
}


int SCH_BUS_BUS_ENTRY::GetPenWidth() const
{
    if( m_stroke.GetWidth() > 0 )
        m_lastResolvedWidth = m_stroke.GetWidth();
    else if( IsConnectable() && !IsConnectivityDirty() )
        m_lastResolvedWidth = GetEffectiveNetClass()->GetBusWidth();

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


void SCH_BUS_ENTRY_BASE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    RotatePoint( m_pos, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
    RotatePoint( &m_size.x, &m_size.y, aRotateCCW ? ANGLE_90 : ANGLE_270 );
}


bool SCH_BUS_WIRE_ENTRY::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                              std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                              const SCH_SHEET_PATH*           aPath )
{
    bool previousStateStart = m_isStartDangling;
    bool previousStateEnd = m_isEndDangling;

    m_isStartDangling = m_isEndDangling = true;

    // Store the connection type and state for the start (0) and end (1)
    bool has_wire[2] = { false };
    bool has_bus[2] = { false };

    for( unsigned ii = 0; ii < aItemListByType.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemListByType[ii];

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
            DANGLING_END_ITEM& nextItem = aItemListByType[++ii];

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
        m_isEndDangling = m_isStartDangling = false;
    else if( has_wire[0] || has_bus[0] )
        m_isStartDangling = false;
    else if( has_wire[1] || has_bus[1] )
        m_isEndDangling = false;

    return (previousStateStart != m_isStartDangling) || (previousStateEnd != m_isEndDangling);
}


bool SCH_BUS_BUS_ENTRY::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                             std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                             const SCH_SHEET_PATH*           aPath )
{
    bool previousStateStart = m_isStartDangling;
    bool previousStateEnd = m_isEndDangling;

    m_isStartDangling = m_isEndDangling = true;

    // TODO: filter using get_lower as we only use one item type
    for( unsigned ii = 0; ii < aItemListByType.size(); ii++ )
    {
        DANGLING_END_ITEM& item = aItemListByType[ii];

        if( item.GetItem() == this )
            continue;

        switch( item.GetType() )
        {
        case BUS_END:
        {
            // The bus has created 2 DANGLING_END_ITEMs, one per end.
            DANGLING_END_ITEM& nextItem = aItemListByType[++ii];

            if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), m_pos ) )
                m_isStartDangling = false;

            if( IsPointOnSegment( item.GetPosition(), nextItem.GetPosition(), GetEnd() ) )
                m_isEndDangling = false;
        }
            break;

        default:
            break;
        }
    }

    return (previousStateStart != m_isStartDangling) || (previousStateEnd != m_isEndDangling);
}


bool SCH_BUS_ENTRY_BASE::IsDangling() const
{
    return m_isStartDangling || m_isEndDangling;
}


std::vector<VECTOR2I> SCH_BUS_ENTRY_BASE::GetConnectionPoints() const
{
    return { m_pos, GetEnd() };
}


bool SCH_BUS_ENTRY_BASE::HasConnectivityChanges( const SCH_ITEM* aItem,
                                                 const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourselves.
    if( aItem == this )
        return false;

    const SCH_BUS_ENTRY_BASE* busEntry = dynamic_cast<const SCH_BUS_ENTRY_BASE*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( busEntry, false );

    if( GetPosition() != busEntry->GetPosition() )
        return true;

    return GetEnd() != busEntry->GetEnd();
}


wxString SCH_BUS_WIRE_ENTRY::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString( _( "Bus to wire entry" ) );
}


wxString SCH_BUS_BUS_ENTRY::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString( _( "Bus to bus entry" ) );
}


BITMAPS SCH_BUS_WIRE_ENTRY::GetMenuImage() const
{
    return BITMAPS::add_line2bus;
}


BITMAPS SCH_BUS_BUS_ENTRY::GetMenuImage() const
{
    return BITMAPS::add_bus2bus;
}


bool SCH_BUS_ENTRY_BASE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Insure minimum accuracy
    if( aAccuracy == 0 )
        aAccuracy = ( GetPenWidth() / 2 ) + 4;

    return TestSegmentHit( aPosition, m_pos, GetEnd(), aAccuracy );
}


bool SCH_BUS_ENTRY_BASE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_BUS_ENTRY_BASE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    SHAPE_SEGMENT line( m_pos, GetEnd(), GetPenWidth() );
    return KIGEOM::ShapeHitTest( aPoly, line, aContained );
}


void SCH_BUS_ENTRY_BASE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                               int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );

    COLOR4D color = ( GetBusEntryColor() == COLOR4D::UNSPECIFIED )
                            ? renderSettings->GetLayerColor( m_layer ) : GetBusEntryColor();

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    int penWidth = ( GetPenWidth() == 0 ) ? renderSettings->GetDefaultPenWidth() : GetPenWidth();

    penWidth = std::max( penWidth, renderSettings->GetMinPenWidth() );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetColor( color );
    aPlotter->SetDash( penWidth, GetEffectiveLineStyle() );
    aPlotter->MoveTo( m_pos );
    aPlotter->FinishTo( GetEnd() );

    aPlotter->SetDash( penWidth, LINE_STYLE::SOLID );
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
            aList.emplace_back( _( "Resolved Netclass" ),
                                GetEffectiveNetClass()->GetHumanReadableName() );
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

bool SCH_BUS_ENTRY_BASE::operator==( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return false;

    const SCH_BUS_ENTRY_BASE* symbol = static_cast<const SCH_BUS_ENTRY_BASE*>( &aItem );

    if( GetLayer() != symbol->GetLayer() )
        return false;

    if( GetPosition() != symbol->GetPosition() )
        return false;

    if( GetEnd() != symbol->GetEnd() )
        return false;

    return true;
}


double SCH_BUS_ENTRY_BASE::Similarity( const SCH_ITEM& aItem ) const
{
    if( aItem.Type() != Type() )
        return 0.0;

    if( m_Uuid == aItem.m_Uuid )
        return 1.0;

    const SCH_BUS_ENTRY_BASE& other = static_cast<const SCH_BUS_ENTRY_BASE&>( aItem );

    if( GetLayer() != other.GetLayer() )
        return 0.0;

    if( GetPosition() != other.GetPosition() )
        return 0.0;

    return 1.0;
}


static struct SCH_BUS_ENTRY_DESC
{
    SCH_BUS_ENTRY_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_BUS_WIRE_ENTRY );
        REGISTER_TYPE( SCH_BUS_BUS_ENTRY );
        REGISTER_TYPE( SCH_BUS_ENTRY_BASE );
        propMgr.InheritsAfter( TYPE_HASH( SCH_BUS_ENTRY_BASE ), TYPE_HASH( SCH_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_BUS_WIRE_ENTRY ), TYPE_HASH( SCH_BUS_ENTRY_BASE ) );
        propMgr.InheritsAfter( TYPE_HASH( SCH_BUS_BUS_ENTRY ), TYPE_HASH( SCH_BUS_ENTRY_BASE ) );

        ENUM_MAP<WIRE_STYLE>& wireLineStyleEnum = ENUM_MAP<WIRE_STYLE>::Instance();

        if( wireLineStyleEnum.Choices().GetCount() == 0 )
        {
            wireLineStyleEnum.Map( WIRE_STYLE::DEFAULT, _HKI( "Default" ) )
                             .Map( WIRE_STYLE::SOLID, _HKI( "Solid" ) )
                             .Map( WIRE_STYLE::DASH, _HKI( "Dashed" ) )
                             .Map( WIRE_STYLE::DOT, _HKI( "Dotted" ) )
                             .Map( WIRE_STYLE::DASHDOT, _HKI( "Dash-Dot" ) )
                             .Map( WIRE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_BUS_ENTRY_BASE, WIRE_STYLE>( _HKI( "Line Style" ),
                    &SCH_BUS_ENTRY_BASE::SetWireStyle, &SCH_BUS_ENTRY_BASE::GetWireStyle ) );

        propMgr.AddProperty( new PROPERTY<SCH_BUS_ENTRY_BASE, int>( _HKI( "Line Width" ),
                    &SCH_BUS_ENTRY_BASE::SetPenWidth, &SCH_BUS_ENTRY_BASE::GetPenWidth,
                    PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<SCH_BUS_ENTRY_BASE, COLOR4D>( _HKI( "Color" ),
                    &SCH_BUS_ENTRY_BASE::SetBusEntryColor, &SCH_BUS_ENTRY_BASE::GetBusEntryColor ) );
    }
} _SCH_BUS_ENTRY_DESC;

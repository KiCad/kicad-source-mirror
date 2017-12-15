/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file sch_bus_entry.cpp
 *
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <class_drawpanel.h>
#include <trigo.h>
#include <common.h>
#include <richio.h>
#include <class_plotter.h>
#include <bitmaps.h>

#include <eeschema_config.h>
#include <general.h>
#include <sch_bus_entry.h>


SCH_BUS_ENTRY_BASE::SCH_BUS_ENTRY_BASE( KICAD_T aType, const wxPoint& pos, char shape ) :
    SCH_ITEM( NULL, aType )
{
    m_pos    = pos;
    m_size.x = 100;
    m_size.y = 100;

    if( shape == '/' )
        m_size.y = -100;

    m_isDanglingStart = m_isDanglingEnd = true;
}

SCH_BUS_WIRE_ENTRY::SCH_BUS_WIRE_ENTRY( const wxPoint& pos, char shape ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_WIRE_ENTRY_T, pos, shape )
{
    m_Layer  = LAYER_WIRE;
}

SCH_BUS_BUS_ENTRY::SCH_BUS_BUS_ENTRY( const wxPoint& pos, char shape ) :
    SCH_BUS_ENTRY_BASE( SCH_BUS_BUS_ENTRY_T, pos, shape )
{
    m_Layer = LAYER_BUS;
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
    return ( m_pos == aPosition || m_End() == aPosition );
}


wxPoint SCH_BUS_ENTRY_BASE::m_End() const
{
    return wxPoint( m_pos.x + m_size.x, m_pos.y + m_size.y );
}


void SCH_BUS_ENTRY_BASE::SwapData( SCH_ITEM* aItem )
{
    SCH_BUS_ENTRY_BASE* item = dynamic_cast<SCH_BUS_ENTRY_BASE*>( aItem );
    wxCHECK_RET( item, wxT( "Cannot swap bus entry data with invalid item." ) );

    std::swap( m_pos, item->m_pos );
    std::swap( m_size, item->m_size );
}


const EDA_RECT SCH_BUS_ENTRY_BASE::GetBoundingBox() const
{
    EDA_RECT box;

    box.SetOrigin( m_pos );
    box.SetEnd( m_End() );

    box.Normalize();
    box.Inflate( GetPenSize() / 2 );

    return box;
}


int SCH_BUS_WIRE_ENTRY::GetPenSize() const
{
    return GetDefaultLineThickness();
}


int SCH_BUS_BUS_ENTRY::GetPenSize() const
{
    return GetDefaultBusThickness();
}


void SCH_BUS_WIRE_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( WIRE_ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( WIRE_ENTRY_END, this, m_End() );
    aItemList.push_back( item1 );
}


void SCH_BUS_BUS_ENTRY::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( BUS_ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( BUS_ENTRY_END, this, m_End() );
    aItemList.push_back( item1 );
}


void SCH_BUS_ENTRY_BASE::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                          GR_DRAWMODE aDrawMode, COLOR4D aColor )
{
    COLOR4D color;
    EDA_RECT* clipbox = aPanel->GetClipBox();

    if( aColor != COLOR4D::UNSPECIFIED )
        color = aColor;
    else
        color = GetLayerColor( GetState( BRIGHTENED ) ? LAYER_BRIGHTENED : m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( clipbox, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y,
            m_End().x + aOffset.x, m_End().y + aOffset.y, GetPenSize(), color );


    // Draw pin targets if part is being dragged
    bool dragging = aPanel->GetScreen()->GetCurItem() == this && aPanel->IsMouseCaptured();

    if( m_isDanglingStart || dragging )
    {
        GRCircle( clipbox, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y,
                TARGET_BUSENTRY_RADIUS, 0, color );
    }

    if( m_isDanglingEnd || dragging )
    {
        GRCircle( clipbox, aDC, m_End().x + aOffset.x, m_End().y + aOffset.y,
                TARGET_BUSENTRY_RADIUS, 0, color );
    }
}


void SCH_BUS_ENTRY_BASE::MirrorX( int aXaxis_position )
{
    MIRROR( m_pos.y, aXaxis_position );
    m_size.y = -m_size.y;
}


void SCH_BUS_ENTRY_BASE::MirrorY( int aYaxis_position )
{
    MIRROR( m_pos.x, aYaxis_position );
    m_size.x = -m_size.x;
}


void SCH_BUS_ENTRY_BASE::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_pos, aPosition, 900 );
    RotatePoint( &m_size.x, &m_size.y, 900 );
}


bool SCH_BUS_WIRE_ENTRY::IsDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList )
{
    bool previousStateStart = m_isDanglingStart;
    bool previousStateEnd = m_isDanglingEnd;

    m_isDanglingStart = m_isDanglingEnd = true;

    // Wires and buses are stored in the list as a pair, start and end. This
    // variable holds the start position from one iteration so it can be used
    // when the end position is found.
    wxPoint seg_start;

    // Store the connection type and state for the start (0) and end (1)
    bool has_wire[2] = { false };
    bool has_bus[2] = { false };

    for( DANGLING_END_ITEM& each_item : aItemList )
    {
        if( each_item.GetItem() == this )
            continue;

        switch( each_item.GetType() )
        {
        case WIRE_START_END:
        case BUS_START_END:
            seg_start = each_item.GetPosition();
            break;

        case WIRE_END_END:
            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_pos ) )
                has_wire[0] = true;

            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_End() ) )
                has_wire[1] = true;

            break;

        case BUS_END_END:
            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_pos ) )
                has_bus[0] = true;

            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_End() ) )
                has_bus[1] = true;

            break;

        default:
            break;
        }
    }

    /**
     * A bus-wire entry is connected at both ends if it has a bus and a wire on its
     * ends.  Otherwise, we connect only one end (in the case of a wire-wire or bus-bus)
     */
    if( ( has_wire[0] && has_bus[1] ) || ( has_wire[1] && has_bus[0] ) )
        m_isDanglingEnd = m_isDanglingStart = false;
    else if( has_wire[0] || has_bus[0] )
        m_isDanglingStart = false;
    else if( has_wire[1] || has_bus[1] )
        m_isDanglingEnd = false;

    return (previousStateStart != m_isDanglingStart) || (previousStateEnd != m_isDanglingEnd);
}


bool SCH_BUS_BUS_ENTRY::IsDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList )
{
    bool previousStateStart = m_isDanglingStart;
    bool previousStateEnd = m_isDanglingEnd;

    m_isDanglingStart = m_isDanglingEnd = true;

    // Wires and buses are stored in the list as a pair, start and end. This
    // variable holds the start position from one iteration so it can be used
    // when the end position is found.
    wxPoint seg_start;

    for( DANGLING_END_ITEM& each_item : aItemList )
    {
        if( each_item.GetItem() == this )
            continue;

        switch( each_item.GetType() )
        {
        case BUS_START_END:
            seg_start = each_item.GetPosition();
            break;
        case BUS_END_END:
            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_pos ) )
                m_isDanglingStart = false;
            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_End() ) )
                m_isDanglingEnd = false;
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


bool SCH_BUS_ENTRY_BASE::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    // If either end of the bus entry is inside the selection rectangle, the entire
    // bus entry is selected.  Bus entries have a fixed length and angle.
    if( aRect.Contains( m_pos ) || aRect.Contains( m_End() ) )
        SetFlags( SELECTED );
    else
        ClearFlags( SELECTED );

    return previousState != IsSelected();
}


void SCH_BUS_ENTRY_BASE::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_pos );
    aPoints.push_back( m_End() );
}


wxString SCH_BUS_WIRE_ENTRY::GetSelectMenuText() const
{
    return wxString( _( "Bus to Wire Entry" ) );
}


wxString SCH_BUS_BUS_ENTRY::GetSelectMenuText() const
{
    return wxString( _( "Bus to Bus Entry" ) );
}


BITMAP_DEF SCH_BUS_WIRE_ENTRY::GetMenuImage() const
{
    return add_line2bus_xpm;
}


BITMAP_DEF SCH_BUS_BUS_ENTRY::GetMenuImage() const
{
    return add_bus2bus_xpm;
}


bool SCH_BUS_ENTRY_BASE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    return TestSegmentHit( aPosition, m_pos, m_End(), aAccuracy );
}


bool SCH_BUS_ENTRY_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void SCH_BUS_ENTRY_BASE::Plot( PLOTTER* aPlotter )
{
    aPlotter->SetCurrentLineWidth( GetPenSize() );
    aPlotter->SetColor( GetLayerColor( GetLayer() ) );
    aPlotter->MoveTo( m_pos );
    aPlotter->FinishTo( m_End() );
}


void SCH_BUS_ENTRY_BASE::SetBusEntryShape( char aShape )
{
    switch( aShape )
    {
    case '\\':
        if( m_size.y < 0 )
            m_size.y = -m_size.y;
        break;

    case '/':
        if( m_size.y > 0 )
            m_size.y = -m_size.y;
        break;
    }
}


char SCH_BUS_ENTRY_BASE::GetBusEntryShape() const
{
    if( GetSize().y < 0 )
        return '/';
    else
        return '\\';
}

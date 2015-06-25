/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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
#include <plot_common.h>
#include <boost/foreach.hpp>

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


wxPoint SCH_BUS_ENTRY_BASE::m_End() const
{
    return wxPoint( m_pos.x + m_size.x, m_pos.y + m_size.y );
}


void SCH_BUS_ENTRY_BASE::SwapData( SCH_ITEM* aItem )
{
    SCH_BUS_ENTRY_BASE* item = dynamic_cast<SCH_BUS_ENTRY_BASE*>( aItem );
    wxCHECK_RET( item, wxT( "Cannot swap bus entry data with invalid item." ) );

    EXCHG( m_pos, item->m_pos );
    EXCHG( m_size, item->m_size );
}


bool SCH_BUS_WIRE_ENTRY::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "Entry Wire Line\n\t%-4d %-4d %-4d %-4d\n",
                 m_pos.x, m_pos.y, m_End().x, m_End().y ) == EOF )
        return false;
    return true;
}


bool SCH_BUS_BUS_ENTRY::Save( FILE* aFile ) const
{
    if( fprintf( aFile, "Entry Bus Bus\n\t%-4d %-4d %-4d %-4d\n",
                 m_pos.x, m_pos.y, m_End().x, m_End().y ) == EOF )
        return false;
    return true;
}


bool SCH_BUS_ENTRY_BASE::Load( LINE_READER& aLine, wxString& aErrorMsg,
                               SCH_ITEM **out )
{
    char Name1[256];
    char Name2[256];
    char* line = (char*) aLine;
    *out = NULL;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%255s %255s", Name1, Name2 ) != 2  )
    {
        aErrorMsg.Printf( wxT( "Eeschema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    SCH_BUS_ENTRY_BASE *this_new;
    if( Name1[0] == 'B' )
        this_new = new SCH_BUS_BUS_ENTRY;
    else
        this_new = new SCH_BUS_WIRE_ENTRY;
    *out = this_new;

    if( !aLine.ReadLine() || sscanf( (char*) aLine, "%d %d %d %d ",
                &this_new->m_pos.x, &this_new->m_pos.y,
                &this_new->m_size.x, &this_new->m_size.y ) != 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file bus entry load error at line %d" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    this_new->m_size.x -= this_new->m_pos.x;
    this_new->m_size.y -= this_new->m_pos.y;

    return true;
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


void SCH_BUS_ENTRY_BASE::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, const wxPoint& aOffset,
                          GR_DRAWMODE aDrawMode, EDA_COLOR_T aColor )
{
    EDA_COLOR_T color;
    EDA_RECT* clipbox = aPanel->GetClipBox();

    if( aColor >= 0 )
        color = aColor;
    else
        color = GetLayerColor( m_Layer );

    GRSetDrawMode( aDC, aDrawMode );

    GRLine( clipbox, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y,
            m_End().x + aOffset.x, m_End().y + aOffset.y, GetPenSize(), color );


    // Draw pin targets if part is being dragged
    bool dragging = ( aPanel->GetScreen()->GetCurItem() == this );

    if( m_isDanglingStart || dragging )
    {
        GRCircle( clipbox, aDC, m_pos.x + aOffset.x, m_pos.y + aOffset.y, TARGET_BUSENTRY_RADIUS, 0, color );
    }

    if( m_isDanglingEnd || dragging )
    {
        GRCircle( clipbox, aDC, m_End().x + aOffset.x, m_End().y + aOffset.y, TARGET_BUSENTRY_RADIUS, 0, color );
    }
}


void SCH_BUS_ENTRY_BASE::MirrorX( int aXaxis_position )
{
    m_pos.y -= aXaxis_position;
    NEGATE(  m_pos.y );
    m_pos.y += aXaxis_position;
    NEGATE(  m_size.y );
}


void SCH_BUS_ENTRY_BASE::MirrorY( int aYaxis_position )
{
    m_pos.x -= aYaxis_position;
    NEGATE(  m_pos.x );
    m_pos.x += aYaxis_position;
    NEGATE(  m_size.x );
}


void SCH_BUS_ENTRY_BASE::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_pos, aPosition, 900 );
    RotatePoint( &m_size.x, &m_size.y, 900 );
}


void SCH_BUS_ENTRY_BASE::GetEndPoints( std::vector< DANGLING_END_ITEM >& aItemList )
{
    DANGLING_END_ITEM item( ENTRY_END, this, m_pos );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( ENTRY_END, this, m_End() );
    aItemList.push_back( item1 );
}


bool SCH_BUS_ENTRY_BASE::IsDanglingStateChanged( std::vector<DANGLING_END_ITEM>& aItemList )
{
    bool previousStateStart = m_isDanglingStart;
    bool previousStateEnd = m_isDanglingEnd;

    m_isDanglingStart = m_isDanglingEnd = true;

    // Wires and buses are stored in the list as a pair, start and end. This
    // variable holds the start position from one iteration so it can be used
    // when the end position is found.
    wxPoint seg_start;

    // Special case: if both items are wires, show as dangling. This is because
    // a bus entry between two wires will look like a connection, but does NOT
    // actually represent one. We need to clarify this for the user.
    bool start_is_wire = false;
    bool end_is_wire = false;

    BOOST_FOREACH( DANGLING_END_ITEM& each_item, aItemList )
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
                start_is_wire = true;
            if( IsPointOnSegment( seg_start, each_item.GetPosition(), m_End() ) )
                end_is_wire = true;
            // Fall through

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

    // See above: show as dangling if joining two wires
    if( start_is_wire && end_is_wire )
        m_isDanglingStart = m_isDanglingEnd = true;

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

/* SetBusEntryShape:
 * Set the shape of the bus entry.
 * aShape = ascii code '/' or '\'
 */
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


/* GetBusEntryShape:
 * return the shape of the bus entry, as an ascii code '/' or '\'
 */
char SCH_BUS_ENTRY_BASE::GetBusEntryShape() const
{
    if( GetSize().y < 0 )
        return '/';
    else
        return '\\';
}


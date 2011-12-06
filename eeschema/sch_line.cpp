/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file sch_line.cpp
 * @brief Class SCH_LINE implementation
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "macros.h"
#include "class_drawpanel.h"
#include "trigo.h"
#include "richio.h"
#include "plot_common.h"

#include "general.h"
#include "protos.h"
#include "sch_line.h"
#include "class_netlist_object.h"

#include <boost/foreach.hpp>


SCH_LINE::SCH_LINE( const wxPoint& pos, int layer ) :
    SCH_ITEM( NULL, SCH_LINE_T )
{
    m_Start = pos;
    m_End   = pos;
    m_Width = 0;        // Default thickness used
    m_StartIsDangling = m_EndIsDangling = false;

    switch( layer )
    {
    default:
        m_Layer = LAYER_NOTES;
        break;

    case LAYER_WIRE:
        m_Layer = LAYER_WIRE;
        break;

    case LAYER_BUS:
        m_Layer = LAYER_BUS;
        break;
    }
}


SCH_LINE::SCH_LINE( const SCH_LINE& aLine ) :
    SCH_ITEM( aLine )
{
    m_Start = aLine.m_Start;
    m_End = aLine.m_End;
    m_Width = aLine.m_Width;
    m_StartIsDangling = m_EndIsDangling = false;
}


EDA_ITEM* SCH_LINE::doClone() const
{
    return new SCH_LINE( *this );
}


void SCH_LINE::Move( const wxPoint& aOffset )
{
    if( (m_Flags & STARTPOINT) == 0 && aOffset != wxPoint( 0, 0 ) )
    {
        m_Start += aOffset;
        SetModified();
    }

    if( (m_Flags & ENDPOINT) == 0 && aOffset != wxPoint( 0, 0 ) )
    {
        m_End += aOffset;
        SetModified();
    }
}


#if defined(DEBUG)

void SCH_LINE::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " width=\"" << m_Width << '"'
                                 << " startIsDangling=\"" << m_StartIsDangling
                                 << '"' << " endIsDangling=\""
                                 << m_EndIsDangling << '"' << ">"
                                 << " <start" << m_Start << "/>"
                                 << " <end" << m_End << "/>" << "</"
                                 << GetClass().Lower().mb_str() << ">\n";
}

#endif


EDA_RECT SCH_LINE::GetBoundingBox() const
{
    int      width = 25;

    int      xmin = MIN( m_Start.x, m_End.x ) - width;
    int      ymin = MIN( m_Start.y, m_End.y ) - width;

    int      xmax = MAX( m_Start.x, m_End.x ) + width;
    int      ymax = MAX( m_Start.y, m_End.y ) + width;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


double SCH_LINE::GetLength() const
{
    return GetLineLength( m_Start, m_End );
}


bool SCH_LINE::Save( FILE* aFile ) const
{
    bool        success = true;

    const char* layer = "Notes";
    const char* width = "Line";

    if( GetLayer() == LAYER_WIRE )
        layer = "Wire";

    if( GetLayer() == LAYER_BUS )
        layer = "Bus";

    if( fprintf( aFile, "Wire %s %s\n", layer, width ) == EOF )
    {
        success = false;
    }

    if( fprintf( aFile, "\t%-4d %-4d %-4d %-4d\n", m_Start.x, m_Start.y,
                 m_End.x, m_End.y ) == EOF )
    {
        success = false;
    }

    return success;
}


bool SCH_LINE::Load( LINE_READER& aLine, wxString& aErrorMsg )
{
    char  Name1[256];
    char  Name2[256];
    char* line = (char*) aLine;

    while( (*line != ' ' ) && *line )
        line++;

    if( sscanf( line, "%s %s", Name1, Name2 ) != 2  )
    {
        aErrorMsg.Printf( wxT( "Eeschema file segment error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    m_Layer = LAYER_NOTES;

    if( Name1[0] == 'W' )
        m_Layer = LAYER_WIRE;

    if( Name1[0] == 'B' )
        m_Layer = LAYER_BUS;

    if( !aLine.ReadLine() || sscanf( (char*) aLine, "%d %d %d %d ",
                                      &m_Start.x, &m_Start.y, &m_End.x, &m_End.y ) != 4 )
    {
        aErrorMsg.Printf( wxT( "Eeschema file Segment struct error at line %d, aborted" ),
                          aLine.LineNumber() );
        aErrorMsg << wxT( "\n" ) << FROM_UTF8( (char*) aLine );
        return false;
    }

    return true;
}


int SCH_LINE::GetPenSize() const
{
    int pensize = ( m_Width == 0 ) ? g_DrawDefaultLineThickness : m_Width;

    if( m_Layer == LAYER_BUS && m_Width == 0 )
    {
        pensize = wxRound( g_DrawDefaultLineThickness * BUS_WIDTH_EXPAND );
        pensize = MAX( pensize, 3 );
    }

    return pensize;
}


void SCH_LINE::Draw( EDA_DRAW_PANEL* panel, wxDC* DC, const wxPoint& offset,
                     int DrawMode, int Color )
{
    int color;
    int width = GetPenSize();

    if( Color >= 0 )
        color = Color;
    else
        color = ReturnLayerColor( m_Layer );

    GRSetDrawMode( DC, DrawMode );

    wxPoint start = m_Start;
    wxPoint end = m_End;

    if( ( m_Flags & STARTPOINT ) == 0 )
        start += offset;
    if( ( m_Flags & ENDPOINT ) == 0 )
        end += offset;

    if( m_Layer == LAYER_NOTES )
        GRDashedLine( &panel->m_ClipBox, DC, start.x, start.y, end.x, end.y, width, color );
    else
        GRLine( &panel->m_ClipBox, DC, start, end, width, color );

    if( m_StartIsDangling )
        DrawDanglingSymbol( panel, DC, start, color );

    if( m_EndIsDangling )
        DrawDanglingSymbol( panel, DC, end, color );
}


void SCH_LINE::Mirror_X( int aXaxis_position )
{
    m_Start.y -= aXaxis_position;
    NEGATE(  m_Start.y );
    m_Start.y += aXaxis_position;
    m_End.y   -= aXaxis_position;
    NEGATE(  m_End.y );
    m_End.y += aXaxis_position;
}


void SCH_LINE::Mirror_Y( int aYaxis_position )
{
    m_Start.x -= aYaxis_position;
    NEGATE(  m_Start.x );
    m_Start.x += aYaxis_position;
    m_End.x   -= aYaxis_position;
    NEGATE(  m_End.x );
    m_End.x += aYaxis_position;
}


void SCH_LINE::Rotate( wxPoint rotationPoint )
{
    RotatePoint( &m_Start, rotationPoint, 900 );
    RotatePoint( &m_End, rotationPoint, 900 );
}


bool SCH_LINE::MergeOverlap( SCH_LINE* aLine )
{
    wxCHECK_MSG( aLine != NULL && aLine->Type() == SCH_LINE_T, false,
                 wxT( "Cannot test line segment for overlap." ) );

    if( this == aLine || GetLayer() != aLine->GetLayer() )
        return false;

    // Search for a common end, and modify coordinates to ensure RefSegm->m_End
    // == TstSegm->m_Start
    if( m_Start == aLine->m_Start )
    {
        if( m_End == aLine->m_End )
            return true;

        EXCHG( m_Start, m_End );
    }
    else if( m_Start == aLine->m_End )
    {
        EXCHG( m_Start, m_End );
        EXCHG( aLine->m_Start, aLine->m_End );
    }
    else if( m_End == aLine->m_End )
    {
        EXCHG( aLine->m_Start, aLine->m_End );
    }
    else if( m_End != aLine->m_Start )
    {
        // No common end point, segments cannot be merged.
        return false;
    }

    /* Test alignment: */
    if( m_Start.y == m_End.y )       // Horizontal segment
    {
        if( aLine->m_Start.y == aLine->m_End.y )
        {
            m_End = aLine->m_End;
            return true;
        }
    }
    else if( m_Start.x == m_End.x )  // Vertical segment
    {
        if( aLine->m_Start.x == aLine->m_End.x )
        {
            m_End = aLine->m_End;
            return true;
        }
    }
    else
    {
        if( atan2( (double) ( m_Start.x - m_End.x ), (double) ( m_Start.y - m_End.y ) )
            == atan2( (double) ( aLine->m_Start.x - aLine->m_End.x ),
                      (double) ( aLine->m_Start.y - aLine->m_End.y ) ) )
        {
            m_End = aLine->m_End;
            return true;
        }
    }

    return false;
}


void SCH_LINE::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    if( GetLayer() == LAYER_NOTES )
        return;

    if( ( GetLayer() == LAYER_BUS ) || ( GetLayer() == LAYER_WIRE ) )
    {
        DANGLING_END_ITEM item( (GetLayer() == LAYER_BUS) ? BUS_START_END : WIRE_START_END, this,
                                m_Start );
        aItemList.push_back( item );

        DANGLING_END_ITEM item1( (GetLayer() == LAYER_BUS) ? BUS_END_END : WIRE_END_END, this,
                                 m_End );
        aItemList.push_back( item1 );
    }
}


bool SCH_LINE::IsDanglingStateChanged( std::vector< DANGLING_END_ITEM >& aItemList )
{
    bool previousStartState = m_StartIsDangling;
    bool previousEndState = m_EndIsDangling;

    m_StartIsDangling = m_EndIsDangling = true;

    if( GetLayer() == LAYER_WIRE )
    {
        BOOST_FOREACH( DANGLING_END_ITEM item, aItemList )
        {
            if( item.GetItem() == this )
                continue;

            if( m_Start == item.GetPosition() )
                m_StartIsDangling = false;

            if( m_End == item.GetPosition() )
                m_EndIsDangling = false;

            if( (m_StartIsDangling == false) && (m_EndIsDangling == false) )
                break;
        }
    }
    else if( GetLayer() == LAYER_BUS || GetLayer() == LAYER_NOTES )
    {
        // Lines on the notes layer and the bus layer cannot be tested for dangling ends.
        previousStartState = previousEndState = m_StartIsDangling = m_EndIsDangling = false;
    }

    return ( previousStartState != m_StartIsDangling ) || ( previousEndState != m_EndIsDangling );
}


bool SCH_LINE::IsSelectStateChanged( const wxRect& aRect )
{
    bool previousState = IsSelected();

    if( aRect.Contains( m_Start ) && aRect.Contains( m_End ) )
    {
        m_Flags |= SELECTED;
        m_Flags &= ~(STARTPOINT | ENDPOINT);
    }
    else if( aRect.Contains( m_Start ) )
    {
        m_Flags &= ~STARTPOINT;
        m_Flags |= ( SELECTED | ENDPOINT );
    }
    else if( aRect.Contains( m_End ) )
    {
        m_Flags &= ~ENDPOINT;
        m_Flags |= ( SELECTED | STARTPOINT );
    }
    else
    {
        m_Flags &= ~( SELECTED | STARTPOINT | ENDPOINT );
    }

    return previousState != IsSelected();
}


bool SCH_LINE::IsConnectable() const
{
    if( m_Layer == LAYER_WIRE || m_Layer == LAYER_BUS )
        return true;

    return false;
}


void SCH_LINE::GetConnectionPoints( vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_Start );
    aPoints.push_back( m_End );
}


wxString SCH_LINE::GetSelectMenuText() const
{
    wxString menuText, txtfmt, orient;

    if( m_Start.x == m_End.x )
        orient = _("Vert.");
    else if( m_Start.y == m_End.y )
        orient = _("Horiz.");

    switch( m_Layer )
    {
    case LAYER_NOTES:
        txtfmt = _( "%s Graphic Line from (%s,%s) to (%s,%s) " );
        break;

    case LAYER_WIRE:
        txtfmt = _( "%s Wire from (%s,%s) to (%s,%s)" );
        break;

    case LAYER_BUS:
        txtfmt = _( "%s Bus from (%s,%s) to (%s,%s)" );
        break;

    default:
        txtfmt += _( "%s Line on Unknown Layer from (%s,%s) to (%s,%s)" );
    }

    menuText.Printf( txtfmt, GetChars( orient ),
                    GetChars(CoordinateToString( m_Start.x, EESCHEMA_INTERNAL_UNIT )),
                    GetChars(CoordinateToString( m_Start.y, EESCHEMA_INTERNAL_UNIT )),
                    GetChars(CoordinateToString( m_End.x, EESCHEMA_INTERNAL_UNIT )),
                    GetChars(CoordinateToString( m_End.y, EESCHEMA_INTERNAL_UNIT )) );

    return menuText;
}


BITMAP_DEF SCH_LINE::GetMenuImage() const
{
    if( m_Layer == LAYER_NOTES )
        return add_dashed_line_xpm;
    else if( m_Layer == LAYER_WIRE )
        return add_line_xpm;

    return add_bus_xpm;
}


void SCH_LINE::GetNetListItem( vector<NETLIST_OBJECT*>& aNetListItems,
                               SCH_SHEET_PATH*          aSheetPath )
{
    // Net list item not required for graphic lines.
    if( (GetLayer() != LAYER_BUS) && (GetLayer() != LAYER_WIRE) )
        return;

    NETLIST_OBJECT* item = new NETLIST_OBJECT();
    item->m_SheetList = *aSheetPath;
    item->m_SheetListInclude = *aSheetPath;
    item->m_Comp = (SCH_ITEM*) this;
    item->m_Start = m_Start;
    item->m_End = m_End;

    if( GetLayer() == LAYER_BUS )
    {
        item->m_Type = NET_BUS;
    }
    else            /* WIRE */
    {
        item->m_Type = NET_SEGMENT;
    }

    aNetListItems.push_back( item );
}


bool SCH_LINE::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    SCH_LINE* line = (SCH_LINE*) &aItem;

    if( GetLength() != line->GetLength() )
        return GetLength() < line->GetLength();

    if( m_Start.x != line->m_Start.x )
        return m_Start.x < line->m_Start.x;

    if( m_Start.y != line->m_Start.y )
        return m_Start.y < line->m_Start.y;

    return false;
}


bool SCH_LINE::doHitTest( const wxPoint& aPoint, int aAccuracy ) const
{
    return TestSegmentHit( aPoint, m_Start, m_End, aAccuracy );
}


bool SCH_LINE::doHitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


bool SCH_LINE::doIsConnected( const wxPoint& aPosition ) const
{
    if( m_Layer != LAYER_WIRE && m_Layer != LAYER_BUS )
        return false;

    return IsEndPoint( aPosition );
}


void SCH_LINE::doPlot( PLOTTER* aPlotter )
{
    aPlotter->set_color( ReturnLayerColor( GetLayer() ) );
    aPlotter->set_current_line_width( GetPenSize() );

    if( m_Layer == LAYER_NOTES )
        aPlotter->set_dash( true );

    aPlotter->move_to( m_Start );
    aPlotter->finish_to( m_End );

    if( m_Layer == LAYER_NOTES )
        aPlotter->set_dash( false );
}


void SCH_LINE::doSetPosition( const wxPoint& aPosition )
{
    m_End = m_End - ( m_Start - aPosition );
    m_Start = aPosition;
}

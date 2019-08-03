/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <gr_basic.h>
#include <macros.h>
#include <sch_draw_panel.h>
#include <plotter.h>
#include <base_units.h>
#include <eeschema_config.h>
#include <general.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <netlist_object.h>
#include <sch_view.h>


static wxPenStyle getwxPenStyle( PlotDashType aType )
{
    switch( aType )
    {
    case PLOTDASHTYPE_SOLID:    return wxPENSTYLE_SOLID;
    case PLOTDASHTYPE_DASH:     return wxPENSTYLE_SHORT_DASH;
    case PLOTDASHTYPE_DOT:      return wxPENSTYLE_DOT;
    case PLOTDASHTYPE_DASHDOT:  return wxPENSTYLE_DOT_DASH;
    }

    wxFAIL_MSG( "Unhandled PlotDashType" );
    return wxPENSTYLE_SOLID;
}


SCH_LINE::SCH_LINE( const wxPoint& pos, int layer ) :
    SCH_ITEM( NULL, SCH_LINE_T )
{
    m_start = pos;
    m_end   = pos;
    m_startIsDangling = m_endIsDangling = false;
    m_size  = 0;
    m_style = -1;
    m_color = COLOR4D::UNSPECIFIED;

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
    m_start = aLine.m_start;
    m_end = aLine.m_end;
    m_size = aLine.m_size;
    m_style = aLine.m_style;
    m_color = aLine.m_color;
    m_startIsDangling = aLine.m_startIsDangling;
    m_endIsDangling = aLine.m_endIsDangling;
}


EDA_ITEM* SCH_LINE::Clone() const
{
    return new SCH_LINE( *this );
}

static const char* style_names[] =
{
    "solid", "dashed", "dotted", "dash_dot", nullptr
};

const char* SCH_LINE::GetLineStyleName( int aStyle )
{
    const char * styleName = style_names[1];

    switch( aStyle )
    {
        case PLOTDASHTYPE_SOLID:
            styleName = style_names[0];
            break;

        default:
        case PLOTDASHTYPE_DASH:
            styleName = style_names[1];
            break;

        case PLOTDASHTYPE_DOT:
            styleName = style_names[2];
            break;

        case PLOTDASHTYPE_DASHDOT:
            styleName = style_names[3];
            break;
    }

    return styleName;
}


int SCH_LINE::GetLineStyleInternalId( const wxString& aStyleName )
{
    int id = -1;    // Default style id

    for( int ii = 0; style_names[ii] != nullptr; ii++ )
    {
        if( aStyleName == style_names[ii] )
        {
            id = ii;
            break;
        }
    }

    return id;
}


void SCH_LINE::Move( const wxPoint& aOffset )
{
    if( aOffset != wxPoint( 0, 0 ) )
    {
        m_start += aOffset;
        m_end += aOffset;
        SetModified();
    }
}


void SCH_LINE::MoveStart( const wxPoint& aOffset )
{
    if( aOffset != wxPoint( 0, 0 ) )
    {
        m_start += aOffset;
        SetModified();
    }
}


void SCH_LINE::MoveEnd( const wxPoint& aOffset )
{
    if( aOffset != wxPoint( 0, 0 ) )
    {
        m_end += aOffset;
        SetModified();
    }
}


#if defined(DEBUG)

void SCH_LINE::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " layer=\"" << m_Layer << '"'
                                 << " startIsDangling=\"" << m_startIsDangling
                                 << '"' << " endIsDangling=\""
                                 << m_endIsDangling << '"' << ">"
                                 << " <start" << m_start << "/>"
                                 << " <end" << m_end << "/>" << "</"
                                 << GetClass().Lower().mb_str() << ">\n";
}

#endif


void SCH_LINE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount     = 2;
    aLayers[0] = m_Layer;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT SCH_LINE::GetBoundingBox() const
{
    int      width = 25;

    int      xmin = std::min( m_start.x, m_end.x ) - width;
    int      ymin = std::min( m_start.y, m_end.y ) - width;

    int      xmax = std::max( m_start.x, m_end.x ) + width;
    int      ymax = std::max( m_start.y, m_end.y ) + width;

    // return a rectangle which is [pos,dim) in nature.  therefore the +1
    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin + 1, ymax - ymin + 1 ) );

    return ret;
}


double SCH_LINE::GetLength() const
{
    return GetLineLength( m_start, m_end );
}


COLOR4D SCH_LINE::GetDefaultColor() const
{
    return GetLayerColor( m_Layer );
}


void SCH_LINE::SetLineColor( const COLOR4D aColor )
{
    if( aColor == GetDefaultColor() )
        m_color = COLOR4D::UNSPECIFIED;
    else
        m_color = aColor;
}


void SCH_LINE::SetLineColor( const double r, const double g, const double b, const double a )
{
    COLOR4D newColor(r, g, b, a);

    if( newColor == GetDefaultColor() || newColor == COLOR4D::UNSPECIFIED )
        m_color = COLOR4D::UNSPECIFIED;
    else
    {
        // Eeschema does not allow alpha channel in colors
        newColor.a = 1.0;
        m_color = newColor;
    }
}


COLOR4D SCH_LINE::GetLineColor() const
{
    if( m_color == COLOR4D::UNSPECIFIED )
        return GetLayerColor( m_Layer );

    return m_color;
}

int SCH_LINE::GetDefaultStyle() const
{
    if( m_Layer == LAYER_NOTES )
        return PLOTDASHTYPE_DASH;

    return PLOTDASHTYPE_SOLID;
}


void SCH_LINE::SetLineStyle( const int aStyle )
{
    if( aStyle == GetDefaultStyle() )
        m_style = -1;
    else
        m_style = aStyle;
}


int SCH_LINE::GetLineStyle() const
{
    if( m_style >= 0 )
        return m_style;

    return GetDefaultStyle();
}


int SCH_LINE::GetDefaultWidth() const
{
    if( m_Layer == LAYER_BUS )
        return GetDefaultBusThickness();
    else if( m_Layer == LAYER_WIRE )
        return GetDefaultWireThickness();

    return GetDefaultLineThickness();
}


void SCH_LINE::SetLineWidth( const int aSize )
{
    if( aSize == GetDefaultWidth() )
        m_size = 0;
    else
        m_size = aSize;
}


int SCH_LINE::GetPenSize() const
{
    if( m_size > 0 )
        return m_size;
    
    return GetDefaultWidth();
}


void SCH_LINE::Print( wxDC* DC, const wxPoint& offset )
{
    COLOR4D color = ( m_color != COLOR4D::UNSPECIFIED ) ? m_color : GetLayerColor( m_Layer );
    int     width = GetPenSize();
    wxPoint start = m_start;
    wxPoint end = m_end;

    GRLine( nullptr, DC, start.x, start.y, end.x, end.y, width, color,
            getwxPenStyle( (PlotDashType) GetLineStyle() ) );
}


void SCH_LINE::MirrorX( int aXaxis_position )
{
    MIRROR( m_start.y, aXaxis_position );
    MIRROR( m_end.y,   aXaxis_position );
}


void SCH_LINE::MirrorY( int aYaxis_position )
{
    MIRROR( m_start.x, aYaxis_position );
    MIRROR( m_end.x,   aYaxis_position );
}


void SCH_LINE::Rotate( wxPoint aPosition )
{
    RotatePoint( &m_start, aPosition, 900 );
    RotatePoint( &m_end, aPosition, 900 );
}


void SCH_LINE::RotateStart( wxPoint aPosition )
{
    RotatePoint( &m_start, aPosition, 900 );
}


void SCH_LINE::RotateEnd( wxPoint aPosition )
{
    RotatePoint( &m_end, aPosition, 900 );
}


bool SCH_LINE::IsSameQuadrant( SCH_LINE* aLine, const wxPoint& aPosition )
{
    wxPoint first;
    wxPoint second;

    if( m_start == aPosition )
        first = m_end - aPosition;
    else if( m_end == aPosition )
        first = m_start - aPosition;
    else
        return false;

    if( aLine->m_start == aPosition )
        second = aLine->m_end - aPosition;
    else if( aLine->m_end == aPosition )
        second = aLine->m_start - aPosition;
    else
        return false;

    return ( sign( first.x ) == sign( second.x ) && sign( first.y ) == sign( second.y ) );
}


bool SCH_LINE::IsParallel( SCH_LINE* aLine )
{
    wxCHECK_MSG( aLine != NULL && aLine->Type() == SCH_LINE_T, false,
                 wxT( "Cannot test line segment for overlap." ) );

    wxPoint firstSeg   = m_end - m_start;
    wxPoint secondSeg = aLine->m_end - aLine->m_start;

    // Use long long here to avoid overflow in calculations
    return !( (long long) firstSeg.x * secondSeg.y - (long long) firstSeg.y * secondSeg.x );
}


EDA_ITEM* SCH_LINE::MergeOverlap( SCH_LINE* aLine )
{
    auto less = []( const wxPoint& lhs, const wxPoint& rhs ) -> bool
    {
        if( lhs.x == rhs.x )
            return lhs.y < rhs.y;
        return lhs.x < rhs.x;
    };

    wxCHECK_MSG( aLine != NULL && aLine->Type() == SCH_LINE_T, NULL,
                 wxT( "Cannot test line segment for overlap." ) );

    if( this == aLine || GetLayer() != aLine->GetLayer() )
        return NULL;

    auto leftmost_start = aLine->m_start;
    auto leftmost_end = aLine->m_end;

    auto rightmost_start = m_start;
    auto rightmost_end = m_end;

    // We place the start to the left and below the end of both lines
    if( leftmost_start != std::min( { leftmost_start, leftmost_end }, less ) )
        std::swap( leftmost_start, leftmost_end );
    if( rightmost_start != std::min( { rightmost_start, rightmost_end }, less ) )
        std::swap( rightmost_start, rightmost_end );

    // -leftmost is the line that starts farthest to the left
    // -other is the line that is _not_ leftmost
    // -rightmost is the line that ends farthest to the right.  This may or
    //   may not be 'other' as the second line may be completely covered by
    //   the first.
    if( less( rightmost_start, leftmost_start ) )
    {
        std::swap( leftmost_start, rightmost_start );
        std::swap( leftmost_end, rightmost_end );
    }

    auto other_start = rightmost_start;
    auto other_end = rightmost_end;

    if( less( rightmost_end, leftmost_end ) )
    {
        rightmost_start = leftmost_start;
        rightmost_end = leftmost_end;
    }

    // If we end one before the beginning of the other, no overlap is possible
    if( less( leftmost_end, other_start ) )
    {
        return NULL;
    }

    // Search for a common end:
    if( ( leftmost_start == other_start ) &&
        ( leftmost_end == other_end ) )     // Trivial case
    {
        auto ret = new SCH_LINE( *aLine );
        ret->SetStartPoint( leftmost_start );
        ret->SetEndPoint( leftmost_end );
        return ret;
    }

    bool colinear = false;

    /* Test alignment: */
    if( ( leftmost_start.y == leftmost_end.y ) &&
        ( other_start.y == other_end.y ) )       // Horizontal segment
    {
        colinear = ( leftmost_start.y == other_start.y );
    }
    else if( ( leftmost_start.x == leftmost_end.x ) &&
             ( other_start.x == other_end.x ) )  // Vertical segment
    {
        colinear = ( leftmost_start.x == other_start.x );
    }
    else
    {
        // We use long long here to avoid overflow -- it enforces promotion
        // Don't use double as we need to make a direct comparison
        // The slope of the left-most line is dy/dx.  Then we check that the slope
        // from the left most start to the right most start is the same as well as
        // the slope from the left most start to right most end.
        long long dx = leftmost_end.x - leftmost_start.x;
        long long dy = leftmost_end.y - leftmost_start.y;
        colinear = ( ( ( other_start.y - leftmost_start.y ) * dx ==
                       ( other_start.x - leftmost_start.x ) * dy ) &&
                     ( ( other_end.y - leftmost_start.y ) * dx ==
                       ( other_end.x - leftmost_start.x ) * dy ) );
    }

    // Make a new segment that merges the 2 segments
    if( colinear )
    {
        leftmost_end = rightmost_end;

        auto ret = new SCH_LINE( *aLine );
        ret->SetStartPoint( leftmost_start );
        ret->SetEndPoint( leftmost_end );
        return ret;
    }

    return NULL;
}


void SCH_LINE::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    if( GetLayer() == LAYER_NOTES )
        return;

    if( ( GetLayer() == LAYER_BUS ) || ( GetLayer() == LAYER_WIRE ) )
    {
        DANGLING_END_ITEM item( (GetLayer() == LAYER_BUS) ? BUS_START_END : WIRE_START_END, this,
                                m_start );
        aItemList.push_back( item );

        DANGLING_END_ITEM item1( (GetLayer() == LAYER_BUS) ? BUS_END_END : WIRE_END_END, this,
                                 m_end );
        aItemList.push_back( item1 );
    }
}


bool SCH_LINE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList )
{
    bool previousStartState = m_startIsDangling;
    bool previousEndState = m_endIsDangling;

    m_startIsDangling = m_endIsDangling = true;

    if( GetLayer() == LAYER_WIRE )
    {
        for( DANGLING_END_ITEM item : aItemList )
        {
            if( item.GetItem() == this )
                continue;

            if(     item.GetType() == BUS_START_END ||
                    item.GetType() == BUS_END_END  ||
                    item.GetType() == BUS_ENTRY_END )
                continue;

            if( m_start == item.GetPosition() )
                m_startIsDangling = false;

            if( m_end == item.GetPosition() )
                m_endIsDangling = false;

            if( (m_startIsDangling == false) && (m_endIsDangling == false) )
                break;
        }
    }
    else if( GetLayer() == LAYER_BUS || GetLayer() == LAYER_NOTES )
    {
        // Lines on the notes layer and the bus layer cannot be tested for dangling ends.
        previousStartState = previousEndState = m_startIsDangling = m_endIsDangling = false;
    }

    return ( previousStartState != m_startIsDangling ) || ( previousEndState != m_endIsDangling );
}


bool SCH_LINE::IsConnectable() const
{
    if( m_Layer == LAYER_WIRE || m_Layer == LAYER_BUS )
        return true;

    return false;
}


bool SCH_LINE::CanConnect( const SCH_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case SCH_JUNCTION_T:
    case SCH_NO_CONNECT_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_COMPONENT_T:
    case SCH_SHEET_T:
    case SCH_SHEET_PIN_T:
        return true;

    default:
        return aItem->GetLayer() == m_Layer;
    }
}


void SCH_LINE::GetConnectionPoints( std::vector< wxPoint >& aPoints ) const
{
    aPoints.push_back( m_start );
    aPoints.push_back( m_end );
}


void SCH_LINE::GetSelectedPoints( std::vector< wxPoint >& aPoints ) const
{
    if( m_Flags & STARTPOINT )
        aPoints.push_back( m_start );

    if( m_Flags & ENDPOINT )
        aPoints.push_back( m_end );
}


wxString SCH_LINE::GetSelectMenuText( EDA_UNITS_T aUnits ) const
{
    wxString txtfmt, orient;

    if( m_start.x == m_end.x )
        orient = _( "Vertical" );
    else if( m_start.y == m_end.y )
        orient = _( "Horizontal" );

    switch( m_Layer )
    {
    case LAYER_NOTES: txtfmt = _( "%s Graphic Line from (%s, %s) to (%s, %s)" );          break;
    case LAYER_WIRE:  txtfmt = _( "%s Wire from (%s, %s) to (%s, %s)" );                  break;
    case LAYER_BUS:   txtfmt = _( "%s Bus from (%s, %s) to (%s, %s)" );                   break;
    default:          txtfmt = _( "%s Line on Unknown Layer from (%s, %s) to (%s, %s)" ); break;
    }

    return wxString::Format( txtfmt,
                             orient,
                             MessageTextFromValue( aUnits, m_start.x ),
                             MessageTextFromValue( aUnits, m_start.y ),
                             MessageTextFromValue( aUnits, m_end.x ),
                             MessageTextFromValue( aUnits, m_end.y ) );
}


BITMAP_DEF SCH_LINE::GetMenuImage() const
{
    if( m_Layer == LAYER_NOTES )
        return add_dashed_line_xpm;
    else if( m_Layer == LAYER_WIRE )
        return add_line_xpm;

    return add_bus_xpm;
}


void SCH_LINE::GetNetListItem( NETLIST_OBJECT_LIST& aNetListItems,
                               SCH_SHEET_PATH*      aSheetPath )
{
    // Net list item not required for graphic lines.
    if( (GetLayer() != LAYER_BUS) && (GetLayer() != LAYER_WIRE) )
        return;

    NETLIST_OBJECT* item = new NETLIST_OBJECT();
    item->m_SheetPath = *aSheetPath;
    item->m_SheetPathInclude = *aSheetPath;
    item->m_Comp = (SCH_ITEM*) this;
    item->m_Start = m_start;
    item->m_End = m_end;

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

    if( m_start.x != line->m_start.x )
        return m_start.x < line->m_start.x;

    if( m_start.y != line->m_start.y )
        return m_start.y < line->m_start.y;

    return false;
}


bool SCH_LINE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    // Insure minimum accuracy
    if( aAccuracy == 0 )
        aAccuracy = ( GetPenSize() / 2 ) + 4;

    return TestSegmentHit( aPosition, m_start, m_end, aAccuracy );
}


bool SCH_LINE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_Flags & ( STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    EDA_RECT rect = aRect;

    if ( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( m_start ) && rect.Contains( m_end );

    return rect.Intersects( m_start, m_end );
}


void SCH_LINE::SwapData( SCH_ITEM* aItem )
{
    SCH_LINE* item = (SCH_LINE*) aItem;

    std::swap( m_Layer, item->m_Layer );

    std::swap( m_start, item->m_start );
    std::swap( m_end, item->m_end );
    std::swap( m_startIsDangling, item->m_startIsDangling );
    std::swap( m_endIsDangling, item->m_endIsDangling );
    std::swap( m_style, item->m_style );
    std::swap( m_size, item->m_size );
    std::swap( m_color, item->m_color );
}


bool SCH_LINE::doIsConnected( const wxPoint& aPosition ) const
{
    if( m_Layer != LAYER_WIRE && m_Layer != LAYER_BUS )
        return false;

    return IsEndPoint( aPosition );
}


void SCH_LINE::Plot( PLOTTER* aPlotter )
{
    if( m_color != COLOR4D::UNSPECIFIED )
        aPlotter->SetColor( m_color );
    else
        aPlotter->SetColor( GetLayerColor( GetLayer() ) );

    aPlotter->SetCurrentLineWidth( GetPenSize() );

    aPlotter->SetDash( GetLineStyle() );

    aPlotter->MoveTo( m_start );
    aPlotter->FinishTo( m_end );

    aPlotter->SetDash( 0 );
}


void SCH_LINE::SetPosition( const wxPoint& aPosition )
{
    m_end = m_end - ( m_start - aPosition );
    m_start = aPosition;
}


void SCH_LINE::GetMsgPanelInfo( EDA_UNITS_T aUnits, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( GetLayer() )
    {
    case LAYER_WIRE: msg = _( "Net Wire" );  break;
    case LAYER_BUS:  msg = _( "Bus Wire" );  break;
    default:         msg = _( "Graphical" ); return;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Line Type" ), msg, DARKCYAN ) );

    if( auto conn = Connection( *g_CurrentSheet ) )
    {
#if defined(DEBUG)
        conn->AppendDebugInfoToMsgPanel( aList );

        msg.Printf( "%zu", m_connected_items.size() );
        aList.push_back( MSG_PANEL_ITEM( _( "Connections" ), msg, BROWN ) );
#else
        conn->AppendInfoToMsgPanel( aList );
#endif
    }
}


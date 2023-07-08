/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_units.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <core/mirror.h>
#include <sch_painter.h>
#include <plotters/plotter.h>
#include <geometry/shape_segment.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <settings/color_settings.h>
#include <schematic.h>
#include <connection_graph.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <trigo.h>
#include <board_item.h>


SCH_LINE::SCH_LINE( const VECTOR2I& pos, int layer ) :
    SCH_ITEM( nullptr, SCH_LINE_T )
{
    m_start           = pos;
    m_end             = pos;
    m_stroke.SetWidth( 0 );
    m_stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    m_stroke.SetColor( COLOR4D::UNSPECIFIED );

    switch( layer )
    {
    default:         m_layer = LAYER_NOTES; break;
    case LAYER_WIRE: m_layer = LAYER_WIRE;  break;
    case LAYER_BUS:  m_layer = LAYER_BUS;   break;
    }

    if( layer == LAYER_NOTES )
        m_startIsDangling = m_endIsDangling = true;
    else
        m_startIsDangling = m_endIsDangling = false;

    if( layer == LAYER_WIRE )
        m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_WIRE_WIDTH_MILS );
    else if( layer == LAYER_BUS )
        m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_BUS_WIDTH_MILS );
    else
        m_lastResolvedWidth = schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );

    m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    m_lastResolvedColor = COLOR4D::UNSPECIFIED;
}


SCH_LINE::SCH_LINE( const SCH_LINE& aLine ) :
    SCH_ITEM( aLine )
{
    m_start = aLine.m_start;
    m_end = aLine.m_end;
    m_stroke = aLine.m_stroke;
    m_startIsDangling = aLine.m_startIsDangling;
    m_endIsDangling = aLine.m_endIsDangling;

    m_lastResolvedLineStyle = aLine.m_lastResolvedLineStyle;
    m_lastResolvedWidth = aLine.m_lastResolvedWidth;
    m_lastResolvedColor = aLine.m_lastResolvedColor;

    m_operatingPoint = aLine.m_operatingPoint;
}


wxString SCH_LINE::GetFriendlyName() const
{
    switch( GetLayer() )
    {
    case LAYER_WIRE: return _( "Wire" );
    case LAYER_BUS:  return _( "Bus" );
    default:         return _( "Graphic Line" );
    }
}


wxString SCH_LINE::GetNetname( const SCH_SHEET_PATH& aSheet )
{
    std::list<const SCH_LINE *> checkedLines;
    checkedLines.push_back(this);
    return FindWireSegmentNetNameRecursive( this, checkedLines, aSheet );
}


wxString SCH_LINE::FindWireSegmentNetNameRecursive( SCH_LINE *line,
                                                    std::list<const SCH_LINE *> &checkedLines,
                                                    const SCH_SHEET_PATH& aSheet ) const
{
    for ( auto connected : line->ConnectedItems( aSheet ) )
    {
        if( connected->Type() == SCH_LINE_T )
        {
            if( std::find(checkedLines.begin(), checkedLines.end(), connected ) == checkedLines.end() )
            {
                SCH_LINE* connectedLine = static_cast<SCH_LINE*>( connected );
                checkedLines.push_back( connectedLine );

                wxString netName = FindWireSegmentNetNameRecursive( connectedLine, checkedLines,
                                                                    aSheet );

                if( !netName.IsEmpty() )
                    return netName;
            }
        }
        else if( connected->Type() == SCH_LABEL_T
                 || connected->Type() == SCH_GLOBAL_LABEL_T
                 || connected->Type() == SCH_DIRECTIVE_LABEL_T)
        {
            return static_cast<SCH_TEXT*>( connected )->GetText();
        }

    }
    return "";
}


EDA_ITEM* SCH_LINE::Clone() const
{
    return new SCH_LINE( *this );
}


void SCH_LINE::Move( const VECTOR2I& aOffset )
{
    m_start += aOffset;
    m_end += aOffset;
}


void SCH_LINE::MoveStart( const VECTOR2I& aOffset )
{
    m_start += aOffset;
}


void SCH_LINE::MoveEnd( const VECTOR2I& aOffset )
{
    m_end += aOffset;
}


#if defined(DEBUG)

void SCH_LINE::Show( int nestLevel, std::ostream& os ) const
{
    NestedSpace( nestLevel, os ) << '<' << GetClass().Lower().mb_str()
                                 << " layer=\"" << m_layer << '"'
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
    aCount     = 4;
    aLayers[0] = LAYER_DANGLING;
    aLayers[1] = m_layer;
    aLayers[2] = LAYER_SELECTION_SHADOWS;
    aLayers[3] = LAYER_OP_VOLTAGES;
}


double SCH_LINE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();
    constexpr double SHOW = 0.0;

    if( aLayer == LAYER_OP_VOLTAGES )
    {
        if( m_start == m_end )
            return HIDE;

        int height = std::abs( m_end.y - m_start.y );
        int width = std::abs( m_end.x - m_start.x );

        // Operating points will be shown only if zoom is appropriate
        if( height == 0 )
            return (double) schIUScale.mmToIU( 15 ) / width;
        else
            return (double) schIUScale.mmToIU( 5 ) / height;
    }

    // Other layers are always drawn.
    return SHOW;
}


const BOX2I SCH_LINE::GetBoundingBox() const
{
    int   width = GetPenWidth() / 2;

    int   xmin = std::min( m_start.x, m_end.x ) - width;
    int   ymin = std::min( m_start.y, m_end.y ) - width;

    int   xmax = std::max( m_start.x, m_end.x ) + width + 1;
    int   ymax = std::max( m_start.y, m_end.y ) + width + 1;

    BOX2I ret( VECTOR2I( xmin, ymin ), VECTOR2I( xmax - xmin, ymax - ymin ) );

    return ret;
}


double SCH_LINE::GetLength() const
{
    return GetLineLength( m_start, m_end );
}


void SCH_LINE::SetLineColor( const COLOR4D& aColor )
{
    m_stroke.SetColor( aColor );
    m_lastResolvedColor = GetLineColor();
}


void SCH_LINE::SetLineColor( const double r, const double g, const double b, const double a )
{
    COLOR4D newColor(r, g, b, a);

    if( newColor == COLOR4D::UNSPECIFIED )
    {
        m_stroke.SetColor( COLOR4D::UNSPECIFIED );
    }
    else
    {
        // Eeschema does not allow alpha channel in colors
        newColor.a = 1.0;
        m_stroke.SetColor( newColor );
    }
}


COLOR4D SCH_LINE::GetLineColor() const
{
    if( m_stroke.GetColor() != COLOR4D::UNSPECIFIED )
        m_lastResolvedColor = m_stroke.GetColor();
    else if( !IsConnectable() )
        m_lastResolvedColor = COLOR4D::UNSPECIFIED;
    else if( !IsConnectivityDirty() )
        m_lastResolvedColor = GetEffectiveNetClass()->GetSchematicColor();

    return m_lastResolvedColor;
}


void SCH_LINE::SetLineStyle( const int aStyleId )
{
    SetLineStyle( static_cast<PLOT_DASH_TYPE>( aStyleId ) );
}


void SCH_LINE::SetLineStyle( const PLOT_DASH_TYPE aStyle )
{
    m_stroke.SetPlotStyle( aStyle );
    m_lastResolvedLineStyle = GetLineStyle();
}


PLOT_DASH_TYPE SCH_LINE::GetLineStyle() const
{
    if( m_stroke.GetPlotStyle() != PLOT_DASH_TYPE::DEFAULT )
        return m_stroke.GetPlotStyle();

    return PLOT_DASH_TYPE::SOLID;
}


PLOT_DASH_TYPE SCH_LINE::GetEffectiveLineStyle() const
{
    if( m_stroke.GetPlotStyle() != PLOT_DASH_TYPE::DEFAULT )
        m_lastResolvedLineStyle = m_stroke.GetPlotStyle();
    else if( !IsConnectable() )
        m_lastResolvedLineStyle = PLOT_DASH_TYPE::SOLID;
    else if( !IsConnectivityDirty() )
        m_lastResolvedLineStyle = (PLOT_DASH_TYPE) GetEffectiveNetClass()->GetLineStyle();

    return m_lastResolvedLineStyle;
}


void SCH_LINE::SetLineWidth( const int aSize )
{
    m_stroke.SetWidth( aSize );
    m_lastResolvedWidth = GetPenWidth();
}


int SCH_LINE::GetPenWidth() const
{
    SCHEMATIC*  schematic = Schematic();

    switch ( m_layer )
    {
    default:
        if( m_stroke.GetWidth() > 0 )
            return m_stroke.GetWidth();

        if( schematic )
            return schematic->Settings().m_DefaultLineWidth;

        return schIUScale.MilsToIU( DEFAULT_LINE_WIDTH_MILS );

    case LAYER_WIRE:
        if( m_stroke.GetWidth() > 0 )
            m_lastResolvedWidth = m_stroke.GetWidth();
        else if( !IsConnectivityDirty() )
            m_lastResolvedWidth = GetEffectiveNetClass()->GetWireWidth();

        return m_lastResolvedWidth;

    case LAYER_BUS:
        if( m_stroke.GetWidth() > 0 )
            m_lastResolvedWidth = m_stroke.GetWidth();
        else if( !IsConnectivityDirty() )
            m_lastResolvedWidth = GetEffectiveNetClass()->GetBusWidth();

        return m_lastResolvedWidth;
    }
}


void SCH_LINE::Print( const RENDER_SETTINGS* aSettings, const VECTOR2I& offset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = GetLineColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( GetLayer() );

    VECTOR2I       start = m_start;
    VECTOR2I       end = m_end;
    PLOT_DASH_TYPE lineStyle = GetEffectiveLineStyle();
    int            penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        GRLine( DC, start.x, start.y, end.x, end.y, penWidth, color );
    }
    else
    {
        SHAPE_SEGMENT segment( start, end );

        STROKE_PARAMS::Stroke( &segment, lineStyle, penWidth, aSettings,
                               [&]( const VECTOR2I& a, const VECTOR2I& b )
                               {
                                   GRLine( DC, a.x, a.y, b.x, b.y, penWidth, color );
                               } );
    }
}


void SCH_LINE::MirrorVertically( int aCenter )
{
    if( m_flags & STARTPOINT )
        MIRROR( m_start.y, aCenter );

    if( m_flags & ENDPOINT )
        MIRROR( m_end.y,   aCenter );
}


void SCH_LINE::MirrorHorizontally( int aCenter )
{
    if( m_flags & STARTPOINT )
        MIRROR( m_start.x, aCenter );

    if( m_flags & ENDPOINT )
        MIRROR( m_end.x,   aCenter );
}


void SCH_LINE::Rotate( const VECTOR2I& aCenter )
{
    // When we allow off grid items, the
    // else if should become a plain if to allow
    // rotation around the center of the line
    if( m_flags & STARTPOINT )
        RotatePoint( m_start, aCenter, ANGLE_90 );

    else if( m_flags & ENDPOINT )
        RotatePoint( m_end, aCenter, ANGLE_90 );
}


void SCH_LINE::RotateStart( const VECTOR2I& aCenter )
{
    RotatePoint( m_start, aCenter, ANGLE_90 );
}


void SCH_LINE::RotateEnd( const VECTOR2I& aCenter )
{
    RotatePoint( m_end, aCenter, ANGLE_90 );
}


int SCH_LINE::GetAngleFrom( const VECTOR2I& aPoint ) const
{
    VECTOR2I vec;

    if( aPoint == m_start )
        vec = m_end - aPoint;
    else
        vec = m_start - aPoint;

    return KiROUND( EDA_ANGLE( vec ).AsDegrees() );
}


int SCH_LINE::GetReverseAngleFrom( const VECTOR2I& aPoint ) const
{
    VECTOR2I vec;

    if( aPoint == m_end )
        vec = m_start - aPoint;
    else
        vec = m_end - aPoint;

    return KiROUND( EDA_ANGLE( vec ).AsDegrees() );
}


bool SCH_LINE::IsParallel( const SCH_LINE* aLine ) const
{
    wxCHECK_MSG( aLine != nullptr && aLine->Type() == SCH_LINE_T, false,
                 wxT( "Cannot test line segment for overlap." ) );

    VECTOR2I firstSeg = m_end - m_start;
    VECTOR2I secondSeg = aLine->m_end - aLine->m_start;

    // Use long long here to avoid overflow in calculations
    return !( (long long) firstSeg.x * secondSeg.y - (long long) firstSeg.y * secondSeg.x );
}


SCH_LINE* SCH_LINE::MergeOverlap( SCH_SCREEN* aScreen, SCH_LINE* aLine, bool aCheckJunctions )
{
    auto less =
            []( const VECTOR2I& lhs, const VECTOR2I& rhs ) -> bool
            {
                if( lhs.x == rhs.x )
                    return lhs.y < rhs.y;

                return lhs.x < rhs.x;
            };

    wxCHECK_MSG( aLine != nullptr && aLine->Type() == SCH_LINE_T, nullptr,
                 wxT( "Cannot test line segment for overlap." ) );

    if( this == aLine || GetLayer() != aLine->GetLayer() )
        return nullptr;

    VECTOR2I leftmost_start = aLine->m_start;
    VECTOR2I leftmost_end = aLine->m_end;

    VECTOR2I rightmost_start = m_start;
    VECTOR2I rightmost_end = m_end;

    // We place the start to the left and below the end of both lines
    if( leftmost_start != std::min( { leftmost_start, leftmost_end }, less ) )
        std::swap( leftmost_start, leftmost_end );
    if( rightmost_start != std::min( { rightmost_start, rightmost_end }, less ) )
        std::swap( rightmost_start, rightmost_end );

    // - leftmost is the line that starts farthest to the left
    // - other is the line that is _not_ leftmost
    // - rightmost is the line that ends farthest to the right.  This may or may not be 'other'
    //      as the second line may be completely covered by the first.
    if( less( rightmost_start, leftmost_start ) )
    {
        std::swap( leftmost_start, rightmost_start );
        std::swap( leftmost_end, rightmost_end );
    }

    VECTOR2I other_start = rightmost_start;
    VECTOR2I other_end = rightmost_end;

    if( less( rightmost_end, leftmost_end ) )
    {
        rightmost_start = leftmost_start;
        rightmost_end = leftmost_end;
    }

    // If we end one before the beginning of the other, no overlap is possible
    if( less( leftmost_end, other_start ) )
    {
        return nullptr;
    }

    // Search for a common end:
    if( ( leftmost_start == other_start ) && ( leftmost_end == other_end ) )  // Trivial case
    {
        SCH_LINE* ret = new SCH_LINE( *aLine );
        ret->SetStartPoint( leftmost_start );
        ret->SetEndPoint( leftmost_end );
        ret->SetConnectivityDirty( true );

        if( IsSelected() || aLine->IsSelected() )
            ret->SetSelected();

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
        // The slope of the left-most line is dy/dx.  Then we check that the slope from the
        // left most start to the right most start is the same as well as the slope from the
        // left most start to right most end.
        long long dx = leftmost_end.x - leftmost_start.x;
        long long dy = leftmost_end.y - leftmost_start.y;
        colinear = ( ( ( other_start.y - leftmost_start.y ) * dx ==
                       ( other_start.x - leftmost_start.x ) * dy ) &&
                     ( ( other_end.y - leftmost_start.y ) * dx ==
                       ( other_end.x - leftmost_start.x ) * dy ) );
    }

    if( !colinear )
        return nullptr;

    // We either have a true overlap or colinear touching segments.  We always want to merge
    // the former, but the later only get merged if there no junction at the touch point.

    bool touching = leftmost_end == rightmost_start;

    if( touching && aCheckJunctions && aScreen->IsJunction( leftmost_end ) )
        return nullptr;

    // Make a new segment that merges the 2 segments
    leftmost_end = rightmost_end;

    SCH_LINE* ret = new SCH_LINE( *aLine );
    ret->SetStartPoint( leftmost_start );
    ret->SetEndPoint( leftmost_end );
    ret->SetConnectivityDirty( true );

    if( IsSelected() || aLine->IsSelected() )
        ret->SetSelected();

    return ret;
}


SCH_LINE* SCH_LINE::BreakAt( const VECTOR2I& aPoint )
{
    SCH_LINE* newSegment = static_cast<SCH_LINE*>( Duplicate() );

    newSegment->SetStartPoint( aPoint );
    newSegment->SetConnectivityDirty( true );
    SetEndPoint( aPoint );

    return newSegment;
}


void SCH_LINE::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    if( IsConnectable() )
    {
        aItemList.emplace_back( IsBus() ? BUS_END : WIRE_END, this, m_start );
        aItemList.emplace_back( IsBus() ? BUS_END : WIRE_END, this, m_end );
    }
}


bool SCH_LINE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                    const SCH_SHEET_PATH* aPath )
{
    if( IsConnectable() )
    {
        bool previousStartState = m_startIsDangling;
        bool previousEndState = m_endIsDangling;

        m_startIsDangling = m_endIsDangling = true;

        for( DANGLING_END_ITEM item : aItemList )
        {
            if( item.GetItem() == this )
                continue;

            if( ( IsWire() && item.GetType() != BUS_END && item.GetType() != BUS_ENTRY_END )
                || ( IsBus() && item.GetType() != WIRE_END && item.GetType() != PIN_END ) )
            {
                if( m_start == item.GetPosition() )
                    m_startIsDangling = false;

                if( m_end == item.GetPosition() )
                    m_endIsDangling = false;

                if( !m_startIsDangling && !m_endIsDangling )
                    break;
            }
        }

        // We only use the bus dangling state for automatic line starting, so we don't care if it
        // has changed or not (and returning true will result in extra work)
        if( IsBus() )
            return false;

        return previousStartState != m_startIsDangling || previousEndState != m_endIsDangling;
    }

    return false;
}


bool SCH_LINE::IsConnectable() const
{
    if( m_layer == LAYER_WIRE || m_layer == LAYER_BUS )
        return true;

    return false;
}


bool SCH_LINE::CanConnect( const SCH_ITEM* aItem ) const
{
    if( m_layer == LAYER_WIRE )
    {
        switch( aItem->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_NO_CONNECT_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_SYMBOL_T:
        case SCH_SHEET_T:
        case SCH_SHEET_PIN_T:
            return true;
        default:
            break;
        }
    }
    else if( m_layer == LAYER_BUS )
    {
        switch( aItem->Type() )
        {
        case SCH_JUNCTION_T:
        case SCH_LABEL_T:
        case SCH_GLOBAL_LABEL_T:
        case SCH_HIER_LABEL_T:
        case SCH_DIRECTIVE_LABEL_T:
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_SHEET_T:
        case SCH_SHEET_PIN_T:
            return true;
        default:
            break;
        }
    }

    return aItem->GetLayer() == m_layer;
}


std::vector<VECTOR2I> SCH_LINE::GetConnectionPoints() const
{
    return { m_start, m_end };
}


bool SCH_LINE::ConnectionPropagatesTo( const EDA_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case SCH_LINE_T:
        return IsBus() == static_cast<const SCH_LINE*>( aItem )->IsBus();

    default:
        return true;
    }
}


void SCH_LINE::GetSelectedPoints( std::vector<VECTOR2I>& aPoints ) const
{
    if( m_flags & STARTPOINT )
        aPoints.push_back( m_start );

    if( m_flags & ENDPOINT )
        aPoints.push_back( m_end );
}


wxString SCH_LINE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    wxString txtfmt;

    if( m_start.x == m_end.x )
    {
        switch( m_layer )
        {
        case LAYER_WIRE:  txtfmt = _( "Vertical Wire, length %s" );         break;
        case LAYER_BUS:   txtfmt = _( "Vertical Bus, length %s" );          break;
        default:          txtfmt = _( "Vertical Graphic Line, length %s" ); break;
        }
    }
    else if( m_start.y == m_end.y )
    {
        switch( m_layer )
        {
        case LAYER_WIRE:  txtfmt = _( "Horizontal Wire, length %s" );         break;
        case LAYER_BUS:   txtfmt = _( "Horizontal Bus, length %s" );          break;
        default:          txtfmt = _( "Horizontal Graphic Line, length %s" ); break;
        }
    }
    else
    {
        switch( m_layer )
        {
        case LAYER_WIRE:  txtfmt = _( "Wire, length %s" );         break;
        case LAYER_BUS:   txtfmt = _( "Bus, length %s" );          break;
        default:          txtfmt = _( "Graphic Line, length %s" ); break;
        }
    }

    return wxString::Format( txtfmt,
                             aUnitsProvider->MessageTextFromValue( EuclideanNorm( m_start - m_end ) ) );
}


BITMAPS SCH_LINE::GetMenuImage() const
{
    if( m_layer == LAYER_NOTES )
        return BITMAPS::add_dashed_line;
    else if( m_layer == LAYER_WIRE )
        return BITMAPS::add_line;

    return BITMAPS::add_bus;
}


bool SCH_LINE::operator <( const SCH_ITEM& aItem ) const
{
    if( Type() != aItem.Type() )
        return Type() < aItem.Type();

    const SCH_LINE* line = static_cast<const SCH_LINE*>( &aItem );

    if( GetLayer() != line->GetLayer() )
        return GetLayer() < line->GetLayer();

    if( GetStartPoint().x != line->GetStartPoint().x )
        return GetStartPoint().x < line->GetStartPoint().x;

    if( GetStartPoint().y != line->GetStartPoint().y )
        return GetStartPoint().y < line->GetStartPoint().y;

    if( GetEndPoint().x != line->GetEndPoint().x )
        return GetEndPoint().x < line->GetEndPoint().x;

    return GetEndPoint().y < line->GetEndPoint().y;
}


bool SCH_LINE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Performance enhancement for connection-building
    if( aPosition == m_start || aPosition == m_end )
        return true;

    if( aAccuracy >= 0 )
        aAccuracy += GetPenWidth() / 2;
    else
        aAccuracy = abs( aAccuracy );

    return TestSegmentHit( aPosition, m_start, m_end, aAccuracy );
}


bool SCH_LINE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    BOX2I rect = aRect;

    if ( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( m_start ) && rect.Contains( m_end );

    return rect.Intersects( m_start, m_end );
}


void SCH_LINE::SwapData( SCH_ITEM* aItem )
{
    SCH_ITEM::SwapFlags( aItem );

    SCH_LINE* item = (SCH_LINE*) aItem;

    std::swap( m_layer, item->m_layer );

    std::swap( m_start, item->m_start );
    std::swap( m_end, item->m_end );
    std::swap( m_startIsDangling, item->m_startIsDangling );
    std::swap( m_endIsDangling, item->m_endIsDangling );
    std::swap( m_stroke, item->m_stroke );
}


bool SCH_LINE::doIsConnected( const VECTOR2I& aPosition ) const
{
    if( m_layer != LAYER_WIRE && m_layer != LAYER_BUS )
        return false;

    return IsEndPoint( aPosition );
}


void SCH_LINE::Plot( PLOTTER* aPlotter, bool aBackground ) const
{
    if( aBackground )
        return;

    auto*   settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    int     penWidth = std::max( GetPenWidth(), settings->GetMinPenWidth() );
    COLOR4D color = GetLineColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( GetLayer() );

    aPlotter->SetColor( color );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetDash( penWidth, GetEffectiveLineStyle() );

    aPlotter->MoveTo( m_start );
    aPlotter->FinishTo( m_end );

    aPlotter->SetDash( penWidth, PLOT_DASH_TYPE::SOLID );

    // Plot attributes to a hypertext menu
    std::vector<wxString> properties;
    BOX2I                 bbox = GetBoundingBox();
    bbox.Inflate( GetPenWidth() * 3 );

    if( GetLayer() == LAYER_WIRE )
    {
        if( SCH_CONNECTION* connection = Connection() )
        {
            properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                       _( "Net" ),
                                                       connection->Name() ) );

            properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                       _( "Resolved netclass" ),
                                                       GetEffectiveNetClass()->GetName() ) );
        }
    }
    else if( GetLayer() == LAYER_BUS )
    {
        if( SCH_CONNECTION* connection = Connection() )
        {
            for( const std::shared_ptr<SCH_CONNECTION>& member : connection->Members() )
                properties.emplace_back( wxT( "!" ) + member->Name() );
        }

    }

    if( !properties.empty() )
        aPlotter->HyperlinkMenu( bbox, properties );
}


void SCH_LINE::SetPosition( const VECTOR2I& aPosition )
{
    m_end = m_end - ( m_start - aPosition );
    m_start = aPosition;
}


void SCH_LINE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    switch( GetLayer() )
    {
    case LAYER_WIRE: msg = _( "Wire" );      break;
    case LAYER_BUS:  msg = _( "Bus" );       break;
    default:         msg = _( "Graphical" ); break;
    }

    aList.emplace_back( _( "Line Type" ), msg );

    PLOT_DASH_TYPE lineStyle = GetLineStyle();

    if( GetEffectiveLineStyle() != lineStyle )
        aList.emplace_back( _( "Line Style" ), _( "from netclass" ) );
    else
        m_stroke.GetMsgPanelInfo( aFrame, aList, true, false );

    SCH_CONNECTION* conn = nullptr;

    if( !IsConnectivityDirty() && dynamic_cast<SCH_EDIT_FRAME*>( aFrame ) )
        conn = Connection();

    if( conn )
    {
        conn->AppendInfoToMsgPanel( aList );

        if( !conn->IsBus() )
        {
            aList.emplace_back( _( "Resolved Netclass" ),
                                UnescapeString( GetEffectiveNetClass()->GetName() ) );
        }
    }
}


bool SCH_LINE::IsGraphicLine() const
{
    return ( GetLayer() == LAYER_NOTES );
}


bool SCH_LINE::IsWire() const
{
    return ( GetLayer() == LAYER_WIRE );
}


bool SCH_LINE::IsBus() const
{
    return ( GetLayer() == LAYER_BUS );
}


static struct SCH_LINE_DESC
{
    SCH_LINE_DESC()
    {
        auto& plotDashTypeEnum = ENUM_MAP<PLOT_DASH_TYPE>::Instance();

        if( plotDashTypeEnum.Choices().GetCount() == 0 )
        {
            plotDashTypeEnum.Map( PLOT_DASH_TYPE::DEFAULT, _HKI( "Default" ) )
                            .Map( PLOT_DASH_TYPE::SOLID, _HKI( "Solid" ) )
                            .Map( PLOT_DASH_TYPE::DASH, _HKI( "Dashed" ) )
                            .Map( PLOT_DASH_TYPE::DOT, _HKI( "Dotted" ) )
                            .Map( PLOT_DASH_TYPE::DASHDOT, _HKI( "Dash-Dot" ) )
                            .Map( PLOT_DASH_TYPE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_LINE );
        propMgr.InheritsAfter( TYPE_HASH( SCH_LINE ), TYPE_HASH( SCH_ITEM ) );

        void ( SCH_LINE::*lineStyleSetter )( PLOT_DASH_TYPE ) = &SCH_LINE::SetLineStyle;

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_LINE, PLOT_DASH_TYPE>( _HKI( "Line Style" ),
                lineStyleSetter, &SCH_LINE::GetLineStyle ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "Line Width" ),
                &SCH_LINE::SetLineWidth, &SCH_LINE::GetLineWidth, PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, COLOR4D>( _HKI( "Color" ),
                &SCH_LINE::SetLineColor, &SCH_LINE::GetLineColor ) );
    }
} _SCH_LINE_DESC;

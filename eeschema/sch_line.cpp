/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/mirror.h>
#include <sch_painter.h>
#include <plotter.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <settings/color_settings.h>
#include <schematic.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <trigo.h>
#include <board_item.h>

SCH_LINE::SCH_LINE( const wxPoint& pos, int layer ) :
    SCH_ITEM( NULL, SCH_LINE_T )
{
    m_start           = pos;
    m_end             = pos;
    m_startIsDangling = m_endIsDangling = false;
    m_stroke.SetWidth( 0 );
    m_stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    m_stroke.SetColor( COLOR4D::UNSPECIFIED );

    switch( layer )
    {
    default: m_layer = LAYER_NOTES; break;
    case LAYER_WIRE: m_layer = LAYER_WIRE;  break;
    case LAYER_BUS: m_layer = LAYER_BUS;   break;
    }
}


SCH_LINE::SCH_LINE( const SCH_LINE& aLine ) :
    SCH_ITEM( aLine )
{
    m_start = aLine.m_start;
    m_end = aLine.m_end;
    m_stroke = aLine.m_stroke;
    m_startIsDangling = aLine.m_startIsDangling;
    m_endIsDangling = aLine.m_endIsDangling;
}


EDA_ITEM* SCH_LINE::Clone() const
{
    return new SCH_LINE( *this );
}


/*
 * Conversion between PLOT_DASH_TYPE values and style names displayed
 */
const std::map<PLOT_DASH_TYPE, const char*> lineStyleNames{
    { PLOT_DASH_TYPE::SOLID, "solid" },
    { PLOT_DASH_TYPE::DASH, "dashed" },
    { PLOT_DASH_TYPE::DASHDOT, "dash_dot" },
    { PLOT_DASH_TYPE::DOT, "dotted" },
};


const char* SCH_LINE::GetLineStyleName( PLOT_DASH_TYPE aStyle )
{
    auto resultIt = lineStyleNames.find( aStyle );

    //legacy behavior is to default to dash if there is no name
    return resultIt == lineStyleNames.end() ? lineStyleNames.find( PLOT_DASH_TYPE::DASH )->second :
                                              resultIt->second;
}


PLOT_DASH_TYPE SCH_LINE::GetLineStyleByName( const wxString& aStyleName )
{
    PLOT_DASH_TYPE id = PLOT_DASH_TYPE::DEFAULT; // Default style id

    //find the name by value
    auto resultIt = std::find_if( lineStyleNames.begin(), lineStyleNames.end(),
            [aStyleName]( const auto& it ) { return it.second == aStyleName; } );

    if( resultIt != lineStyleNames.end() )
        id = resultIt->first;

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
    aCount     = 2;
    aLayers[0] = m_layer;
    aLayers[1] = LAYER_SELECTION_SHADOWS;
}


const EDA_RECT SCH_LINE::GetBoundingBox() const
{
    int      width = m_stroke.GetWidth() / 2;
    int      extra = m_stroke.GetWidth() & 0x1;

    int      xmin = std::min( m_start.x, m_end.x ) - width;
    int      ymin = std::min( m_start.y, m_end.y ) - width;

    int      xmax = std::max( m_start.x, m_end.x ) + width + extra;
    int      ymax = std::max( m_start.y, m_end.y ) + width + extra;

    EDA_RECT ret( wxPoint( xmin, ymin ), wxSize( xmax - xmin, ymax - ymin ) );

    return ret;
}


double SCH_LINE::GetLength() const
{
    return GetLineLength( m_start, m_end );
}


void SCH_LINE::SetLineColor( const COLOR4D& aColor )
{
    m_stroke.SetColor( aColor );
}


void SCH_LINE::SetLineColor( const double r, const double g, const double b, const double a )
{
    COLOR4D newColor(r, g, b, a);

    if( newColor == COLOR4D::UNSPECIFIED )
        m_stroke.SetColor( COLOR4D::UNSPECIFIED );
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
        return m_stroke.GetColor();

    NETCLASSPTR netclass = NetClass();

    if( netclass )
        return netclass->GetSchematicColor();

    return m_stroke.GetColor();
}


PLOT_DASH_TYPE SCH_LINE::GetDefaultStyle() const
{
    if( IsGraphicLine() )
        return PLOT_DASH_TYPE::DASH;

    return PLOT_DASH_TYPE::SOLID;
}


void SCH_LINE::SetLineStyle( const int aStyleId )
{
    SetLineStyle( static_cast<PLOT_DASH_TYPE>( aStyleId ) );
}


void SCH_LINE::SetLineStyle( const PLOT_DASH_TYPE aStyle )
{
    if( aStyle == GetDefaultStyle() )
        m_stroke.SetPlotStyle( PLOT_DASH_TYPE::DEFAULT );
    else
        m_stroke.SetPlotStyle( aStyle );
}


PLOT_DASH_TYPE SCH_LINE::GetLineStyle() const
{
    if( m_stroke.GetPlotStyle() != PLOT_DASH_TYPE::DEFAULT )
        return m_stroke.GetPlotStyle();

    return GetDefaultStyle();
}


PLOT_DASH_TYPE SCH_LINE::GetEffectiveLineStyle() const
{
    if( m_stroke.GetPlotStyle() != PLOT_DASH_TYPE::DEFAULT )
        return m_stroke.GetPlotStyle();

    NETCLASSPTR netclass = NetClass();

    if( netclass )
        return (PLOT_DASH_TYPE) netclass->GetLineStyle();

    return GetLineStyle();
}


void SCH_LINE::SetLineWidth( const int aSize )
{
    m_stroke.SetWidth( aSize );
}


int SCH_LINE::GetPenWidth() const
{
    NETCLASSPTR netclass = NetClass();

    switch ( m_layer )
    {
    default:
        if( m_stroke.GetWidth() > 0 )
            return m_stroke.GetWidth();

        if( Schematic() )
            return Schematic()->Settings().m_DefaultLineWidth;

        return DEFAULT_LINE_THICKNESS;

    case LAYER_WIRE:
        if( m_stroke.GetWidth() > 0 )
            return m_stroke.GetWidth();

        if( netclass )
            return netclass->GetWireWidth();

        if( Schematic() )
            return Schematic()->Settings().m_DefaultWireThickness;

        return DEFAULT_WIRE_THICKNESS;

    case LAYER_BUS:
        if( m_stroke.GetWidth() > 0 )
            return m_stroke.GetWidth();

        if( netclass )
            return netclass->GetBusWidth();

        if( Schematic() )
            return Schematic()->Settings().m_DefaultBusThickness;

        return DEFAULT_BUS_THICKNESS;
    }
}


void SCH_LINE::Print( const RENDER_SETTINGS* aSettings, const wxPoint& offset )
{
    wxDC*   DC = aSettings->GetPrintDC();
    COLOR4D color = GetLineColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = aSettings->GetLayerColor( GetLayer() );

    wxPoint        start = m_start;
    wxPoint        end = m_end;
    PLOT_DASH_TYPE lineStyle = GetEffectiveLineStyle();
    int            penWidth = std::max( GetPenWidth(), aSettings->GetDefaultPenWidth() );

    if( lineStyle <= PLOT_DASH_TYPE::FIRST_TYPE )
    {
        GRLine( nullptr, DC, start.x, start.y, end.x, end.y, penWidth, color );
    }
    else
    {
        EDA_RECT clip( (wxPoint) start, wxSize( end.x - start.x, end.y - start.y ) );
        clip.Normalize();

        double theta = atan2( end.y - start.y, end.x - start.x );
        double strokes[] = { 1.0, DASH_GAP_LEN( penWidth ), 1.0, DASH_GAP_LEN( penWidth ) };

        switch( lineStyle )
        {
        default:
        case PLOT_DASH_TYPE::DASH:
            strokes[0] = strokes[2] = DASH_MARK_LEN( penWidth );
            break;
        case PLOT_DASH_TYPE::DOT:
            strokes[0] = strokes[2] = DOT_MARK_LEN( penWidth );
            break;
        case PLOT_DASH_TYPE::DASHDOT:
            strokes[0] = DASH_MARK_LEN( penWidth );
            strokes[2] = DOT_MARK_LEN( penWidth );
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


void SCH_LINE::MirrorVertically( int aCenter )
{
    MIRROR( m_start.y, aCenter );
    MIRROR( m_end.y,   aCenter );
}


void SCH_LINE::MirrorHorizontally( int aCenter )
{
    MIRROR( m_start.x, aCenter );
    MIRROR( m_end.x,   aCenter );
}


void SCH_LINE::Rotate( wxPoint aCenter )
{
    RotatePoint( &m_start, aCenter, 900 );
    RotatePoint( &m_end, aCenter, 900 );
}


void SCH_LINE::RotateStart( wxPoint aCenter )
{
    RotatePoint( &m_start, aCenter, 900 );
}


void SCH_LINE::RotateEnd( wxPoint aCenter )
{
    RotatePoint( &m_end, aCenter, 900 );
}


int SCH_LINE::GetAngleFrom( const wxPoint& aPoint ) const
{
    wxPoint vec;

    if( aPoint == m_start )
        vec = m_end - aPoint;
    else
        vec = m_start - aPoint;

    return KiROUND( ArcTangente( vec.y, vec.x ) );
}


int SCH_LINE::GetReverseAngleFrom( const wxPoint& aPoint ) const
{
    wxPoint vec;

    if( aPoint == m_end )
        vec = m_start - aPoint;
    else
        vec = m_end - aPoint;

    return KiROUND( ArcTangente( vec.y, vec.x ) );
}


bool SCH_LINE::IsParallel( const SCH_LINE* aLine ) const
{
    wxCHECK_MSG( aLine != NULL && aLine->Type() == SCH_LINE_T, false,
                 wxT( "Cannot test line segment for overlap." ) );

    wxPoint firstSeg   = m_end - m_start;
    wxPoint secondSeg = aLine->m_end - aLine->m_start;

    // Use long long here to avoid overflow in calculations
    return !( (long long) firstSeg.x * secondSeg.y - (long long) firstSeg.y * secondSeg.x );
}


SCH_LINE* SCH_LINE::MergeOverlap( SCH_SCREEN* aScreen, SCH_LINE* aLine, bool aCheckJunctions )
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
        return nullptr;

    auto leftmost_start = aLine->m_start;
    auto leftmost_end = aLine->m_end;

    auto rightmost_start = m_start;
    auto rightmost_end = m_end;

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

    wxPoint other_start = rightmost_start;
    wxPoint other_end = rightmost_end;

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
        auto ret = new SCH_LINE( *aLine );
        ret->SetStartPoint( leftmost_start );
        ret->SetEndPoint( leftmost_end );

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

    if( touching && aCheckJunctions && aScreen->IsJunctionNeeded( leftmost_end ) )
        return nullptr;

    // Make a new segment that merges the 2 segments
    leftmost_end = rightmost_end;

    auto ret = new SCH_LINE( *aLine );
    ret->SetStartPoint( leftmost_start );
    ret->SetEndPoint( leftmost_end );

    if( IsSelected() || aLine->IsSelected() )
        ret->SetSelected();

    return ret;
}


void SCH_LINE::GetEndPoints( std::vector <DANGLING_END_ITEM>& aItemList )
{
    DANGLING_END_T startType, endType;

    switch( GetLayer() )
    {
    case LAYER_WIRE:
        startType = WIRE_START_END;
        endType = WIRE_END_END;
        break;
    case LAYER_BUS:
        startType = BUS_START_END;
        endType = BUS_END_END;
        break;
    default:
        startType = GRAPHIC_START_END;
        endType = GRAPHIC_END_END;
        break;
    }

    DANGLING_END_ITEM item( startType, this, m_start );
    aItemList.push_back( item );

    DANGLING_END_ITEM item1( endType, this, m_end );
    aItemList.push_back( item1 );
}


bool SCH_LINE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemList,
                                    const SCH_SHEET_PATH* aPath )
{
    bool previousStartState = m_startIsDangling;
    bool previousEndState = m_endIsDangling;

    m_startIsDangling = m_endIsDangling = true;

    for( DANGLING_END_ITEM item : aItemList )
    {
        if( item.GetItem() == this )
            continue;

        if( ( IsWire()
              && ( item.GetType() == BUS_START_END || item.GetType() == BUS_END_END
                   || item.GetType() == BUS_ENTRY_END ) )
            || ( IsBus()
                 && ( item.GetType() == WIRE_START_END || item.GetType() == WIRE_END_END
                      || item.GetType() == PIN_END ) )
            || ( IsGraphicLine()
                 && ( item.GetType() != GRAPHIC_START_END && item.GetType() != GRAPHIC_END_END ) ) )
            continue;

        if( m_start == item.GetPosition() )
            m_startIsDangling = false;

        if( m_end == item.GetPosition() )
            m_endIsDangling = false;

        if( !m_startIsDangling && !m_endIsDangling )
            break;
    }

    if( IsBus() || IsGraphicLine() )
    {
        // Force unchanged return state for graphic lines and busses
        previousStartState = m_startIsDangling;
        previousEndState = m_endIsDangling;
    }

    return ( previousStartState != m_startIsDangling ) || ( previousEndState != m_endIsDangling );
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
        case SCH_BUS_WIRE_ENTRY_T:
        case SCH_COMPONENT_T:
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


std::vector<wxPoint> SCH_LINE::GetConnectionPoints() const
{
    return { m_start, m_end };
}


void SCH_LINE::GetSelectedPoints( std::vector< wxPoint >& aPoints ) const
{
    if( m_flags & STARTPOINT )
        aPoints.push_back( m_start );

    if( m_flags & ENDPOINT )
        aPoints.push_back( m_end );
}


wxString SCH_LINE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    wxString txtfmt, orient;

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
                             MessageTextFromValue( aUnits, EuclideanNorm( m_start - m_end ) ) );
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

    auto line = static_cast<const SCH_LINE*>( &aItem );

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


bool SCH_LINE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
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


bool SCH_LINE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
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

    std::swap( m_layer, item->m_layer );

    std::swap( m_start, item->m_start );
    std::swap( m_end, item->m_end );
    std::swap( m_startIsDangling, item->m_startIsDangling );
    std::swap( m_endIsDangling, item->m_endIsDangling );
    std::swap( m_stroke, item->m_stroke );
}


bool SCH_LINE::doIsConnected( const wxPoint& aPosition ) const
{
    if( m_layer != LAYER_WIRE && m_layer != LAYER_BUS )
        return false;

    return IsEndPoint( aPosition );
}


void SCH_LINE::Plot( PLOTTER* aPlotter ) const
{
    auto*   settings = static_cast<KIGFX::SCH_RENDER_SETTINGS*>( aPlotter->RenderSettings() );
    int     penWidth;
    COLOR4D color = GetLineColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = settings->GetLayerColor( GetLayer() );

    aPlotter->SetColor( color );

    switch( m_layer )
    {
    case LAYER_WIRE: penWidth = settings->m_DefaultWireThickness; break;
    case LAYER_BUS:  penWidth = settings->m_DefaultBusThickness;  break;
    default:         penWidth = GetPenWidth();                    break;
    }

    if( m_stroke.GetWidth() != 0 )
        penWidth = m_stroke.GetWidth();

    penWidth = std::max( penWidth, settings->GetMinPenWidth() );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetDash( GetEffectiveLineStyle() );

    aPlotter->MoveTo( m_start );
    aPlotter->FinishTo( m_end );

    aPlotter->SetDash( PLOT_DASH_TYPE::SOLID );
}


void SCH_LINE::SetPosition( const wxPoint& aPosition )
{
    m_end = m_end - ( m_start - aPosition );
    m_start = aPosition;
}


void SCH_LINE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, MSG_PANEL_ITEMS& aList )
{
    wxString msg;

    switch( GetLayer() )
    {
    case LAYER_WIRE: msg = _( "Wire" );      break;
    case LAYER_BUS:  msg = _( "Bus" );       break;
    default:         msg = _( "Graphical" ); break;
    }

    aList.push_back( MSG_PANEL_ITEM( _( "Line Type" ), msg ) );

    if( GetLineStyle() != GetEffectiveLineStyle() )
        msg = _( "from netclass" );
    else
        msg = GetLineStyleName( GetLineStyle() );

    aList.push_back( MSG_PANEL_ITEM( _( "Line Style" ), msg ) );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( aFrame );

    if( frame )
    {
        if( SCH_CONNECTION* conn = Connection() )
        {
            conn->AppendInfoToMsgPanel( aList );

            NET_SETTINGS& netSettings = Schematic()->Prj().GetProjectFile().NetSettings();
            wxString netname = conn->Name();
            wxString netclassName = netSettings.m_NetClasses.GetDefaultPtr()->GetName();

            if( netSettings.m_NetClassAssignments.count( netname ) )
                netclassName = netSettings.m_NetClassAssignments[ netname ];

            aList.push_back( MSG_PANEL_ITEM( _( "Assigned Netclass" ), netclassName ) );
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


bool SCH_LINE::UsesDefaultStroke() const
{
    return m_stroke.GetWidth() == 0 && m_stroke.GetColor() == COLOR4D::UNSPECIFIED
            && ( m_stroke.GetPlotStyle() == GetDefaultStyle()
            || m_stroke.GetPlotStyle() == PLOT_DASH_TYPE::DEFAULT );
}

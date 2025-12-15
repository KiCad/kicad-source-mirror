/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <base_units.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <core/mirror.h>
#include <sch_painter.h>
#include <sch_plotter.h>
#include <geometry/shape_segment.h>
#include <geometry/geometry_utils.h>
#include <sch_line.h>
#include <sch_edit_frame.h>
#include <settings/color_settings.h>
#include <connection_graph.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <trigo.h>
#include <board_item.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/schematic/schematic_types.pb.h>
#include <properties/property.h>
#include <origin_transforms.h>
#include <math/util.h>


SCH_LINE::SCH_LINE( const VECTOR2I& pos, int layer ) :
    SCH_ITEM( nullptr, SCH_LINE_T )
{
    m_start           = pos;
    m_end             = pos;
    m_stroke.SetWidth( 0 );
    m_stroke.SetLineStyle( LINE_STYLE::DEFAULT );
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

    m_lastResolvedLineStyle = LINE_STYLE::SOLID;
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

    // Don't apply groups to cloned lines. We have too many areas where we clone them
    // temporarily, then modify/split/join them in the line movement routines after the
    // segments are committed. Rely on the commit framework to add the lines to the
    // entered group as appropriate.
    m_group = nullptr;
}


void SCH_LINE::Serialize( google::protobuf::Any &aContainer ) const
{
    kiapi::schematic::types::Line line;

    line.mutable_id()->set_value( m_Uuid.AsStdString() );
    kiapi::common::PackVector2( *line.mutable_start(), GetStartPoint() );
    kiapi::common::PackVector2( *line.mutable_end(), GetEndPoint() );
    line.set_layer(
            ToProtoEnum<SCH_LAYER_ID, kiapi::schematic::types::SchematicLayer>( GetLayer() ) );

    aContainer.PackFrom( line );
}


bool SCH_LINE::Deserialize( const google::protobuf::Any &aContainer )
{
    kiapi::schematic::types::Line line;

    if( !aContainer.UnpackTo( &line ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( line.id().value() );
    SetStartPoint( kiapi::common::UnpackVector2( line.start() ) );
    SetEndPoint( kiapi::common::UnpackVector2( line.end() ) );
    SCH_LAYER_ID layer =
            FromProtoEnum<SCH_LAYER_ID, kiapi::schematic::types::SchematicLayer>( line.layer() );

    switch( layer )
    {
    case LAYER_WIRE:
    case LAYER_BUS:
    case LAYER_NOTES:
        SetLayer( layer );
        break;

    default:
        break;
    }

    return true;
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


std::vector<int> SCH_LINE::ViewGetLayers() const
{
    if( IsWire() || IsBus() )
        return { LAYER_DANGLING, m_layer, LAYER_SELECTION_SHADOWS, LAYER_NET_COLOR_HIGHLIGHT,
                 LAYER_OP_VOLTAGES };

    return { LAYER_DANGLING, m_layer, LAYER_SELECTION_SHADOWS, LAYER_OP_VOLTAGES };
}


double SCH_LINE::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    if( aLayer == LAYER_OP_VOLTAGES )
    {
        if( m_start == m_end )
            return LOD_HIDE;

        const int height = std::abs( m_end.y - m_start.y );

        // Operating points will be shown only if zoom is appropriate
        if( height > 0 )
            return lodScaleForThreshold( aView, height, schIUScale.mmToIU( 5 ) );

        const int width = std::abs( m_end.x - m_start.x );
        return lodScaleForThreshold( aView, width, schIUScale.mmToIU( 15 ) );
    }

    // Other layers are always drawn.
    return LOD_SHOW;
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
    return m_start.Distance( m_end );
}


void SCH_LINE::SetLength( double aLength )
{
    if( aLength < 0.0 )
        aLength = 0.0;

    double    currentLength = GetLength();
    VECTOR2I  start = GetStartPoint();
    VECTOR2I  end;

    if( currentLength <= 0.0 )
    {
        end = start + KiROUND( aLength, 0.0 );
    }
    else
    {
        VECTOR2I delta = GetEndPoint() - start;
        double   scale = aLength / currentLength;

        end = start + KiROUND( delta * scale );
    }

    SetEndPoint( end );
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


void SCH_LINE::SetLineStyle( const LINE_STYLE aStyle )
{
    m_stroke.SetLineStyle( aStyle );
    m_lastResolvedLineStyle = GetEffectiveLineStyle();
}


LINE_STYLE SCH_LINE::GetLineStyle() const
{
    if( IsGraphicLine() && m_stroke.GetLineStyle() == LINE_STYLE::DEFAULT )
        return LINE_STYLE::SOLID;
    else
        return m_stroke.GetLineStyle();
}


LINE_STYLE SCH_LINE::GetEffectiveLineStyle() const
{
    if( m_stroke.GetLineStyle() != LINE_STYLE::DEFAULT )
        m_lastResolvedLineStyle = m_stroke.GetLineStyle();
    else if( !IsConnectable() )
        m_lastResolvedLineStyle = LINE_STYLE::SOLID;
    else if( !IsConnectivityDirty() )
        m_lastResolvedLineStyle = (LINE_STYLE) GetEffectiveNetClass()->GetLineStyle();

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


void SCH_LINE::Rotate( const VECTOR2I& aCenter, bool aRotateCCW )
{
    if( m_flags & STARTPOINT )
        RotatePoint( m_start, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );

    if( m_flags & ENDPOINT )
        RotatePoint( m_end, aCenter, aRotateCCW ? ANGLE_90 : ANGLE_270 );
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


SCH_LINE* SCH_LINE::BreakAt( SCH_COMMIT* aCommit, const VECTOR2I& aPoint )
{
    SCH_LINE* newSegment = static_cast<SCH_LINE*>( Duplicate( true /* addToParentGroup */, aCommit ) );

    newSegment->SetStartPoint( aPoint );
    newSegment->SetConnectivityDirty( true );
    SetEndPoint( aPoint );

    return newSegment;
}


SCH_LINE* SCH_LINE::NonGroupAware_BreakAt( const VECTOR2I& aPoint )
{
    SCH_LINE* newSegment = static_cast<SCH_LINE*>( Duplicate( false /* addToParentGroup */, nullptr ) );

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


bool SCH_LINE::UpdateDanglingState( std::vector<DANGLING_END_ITEM>& aItemListByType,
                                    std::vector<DANGLING_END_ITEM>& aItemListByPos,
                                    const SCH_SHEET_PATH*           aPath )
{
    if( !IsConnectable() )
        return false;

    bool previousStartState = m_startIsDangling;
    bool previousEndState = m_endIsDangling;

    m_startIsDangling = m_endIsDangling = true;

    for( auto it = DANGLING_END_ITEM_HELPER::get_lower_pos( aItemListByPos, m_start );
         it < aItemListByPos.end() && it->GetPosition() == m_start; it++ )
    {
        DANGLING_END_ITEM& item = *it;

        if( item.GetItem() == this )
            continue;

        if( ( IsWire() && item.GetType() != BUS_END && item.GetType() != BUS_ENTRY_END )
            || ( IsBus() && item.GetType() != WIRE_END && item.GetType() != PIN_END ) )
        {
            m_startIsDangling = false;
            break;
        }
    }

    for( auto it = DANGLING_END_ITEM_HELPER::get_lower_pos( aItemListByPos, m_end );
         it < aItemListByPos.end() && it->GetPosition() == m_end; it++ )
    {
        DANGLING_END_ITEM& item = *it;

        if( item.GetItem() == this )
            continue;

        if( ( IsWire() && item.GetType() != BUS_END && item.GetType() != BUS_ENTRY_END )
            || ( IsBus() && item.GetType() != WIRE_END && item.GetType() != PIN_END ) )
        {
            m_endIsDangling = false;
            break;
        }
    }

    // We only use the bus dangling state for automatic line starting, so we don't care if it
    // has changed or not (and returning true will result in extra work)
    if( IsBus() )
        return false;

    return previousStartState != m_startIsDangling || previousEndState != m_endIsDangling;
}


bool SCH_LINE::IsConnectable() const
{
    if( m_layer == LAYER_WIRE || m_layer == LAYER_BUS )
        return true;

    return false;
}


bool SCH_LINE::CanConnect( const SCH_ITEM* aItem ) const
{
    switch( aItem->Type() )
    {
    case SCH_NO_CONNECT_T:
    case SCH_SYMBOL_T:
        return IsWire();

    case SCH_JUNCTION_T:
    case SCH_LABEL_T:
    case SCH_GLOBAL_LABEL_T:
    case SCH_HIER_LABEL_T:
    case SCH_DIRECTIVE_LABEL_T:
    case SCH_BUS_WIRE_ENTRY_T:
    case SCH_SHEET_T:
    case SCH_SHEET_PIN_T:
        return IsWire() || IsBus();

    default:
        return m_layer == aItem->GetLayer();
    }
}


bool SCH_LINE::HasConnectivityChanges( const SCH_ITEM* aItem,
                                       const SCH_SHEET_PATH* aInstance ) const
{
    // Do not compare to ourself.
    if( aItem == this || !IsConnectable() )
        return false;

    const SCH_LINE* line = dynamic_cast<const SCH_LINE*>( aItem );

    // Don't compare against a different SCH_ITEM.
    wxCHECK( line, false );

    if( GetStartPoint() != line->GetStartPoint() )
        return true;

    return GetEndPoint() != line->GetEndPoint();
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


wxString SCH_LINE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
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
                             aUnitsProvider->MessageTextFromValue( m_start.Distance( m_end ) ) );
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


bool SCH_LINE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    if( m_flags & (STRUCT_DELETED | SKIP_STRUCT ) )
        return false;

    SHAPE_SEGMENT line( m_start, m_end, GetPenWidth() );
    return KIGEOM::ShapeHitTest( aPoly, line, aContained );
}


void SCH_LINE::swapData( SCH_ITEM* aItem )
{
    SCH_LINE* item = (SCH_LINE*) aItem;

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


void SCH_LINE::Plot( PLOTTER* aPlotter, bool aBackground, const SCH_PLOT_OPTS& aPlotOpts,
                     int aUnit, int aBodyStyle, const VECTOR2I& aOffset, bool aDimmed )
{
    if( aBackground )
        return;

    SCH_RENDER_SETTINGS* renderSettings = getRenderSettings( aPlotter );
    int                  penWidth = GetEffectivePenWidth( renderSettings );
    COLOR4D              color = GetLineColor();

    if( color == COLOR4D::UNSPECIFIED )
        color = renderSettings->GetLayerColor( GetLayer() );

    if( color.m_text.has_value() && Schematic() )
        color = COLOR4D( ResolveText( color.m_text.value(), &Schematic()->CurrentSheet() ) );

    aPlotter->SetColor( color );

    aPlotter->SetCurrentLineWidth( penWidth );
    aPlotter->SetDash( penWidth, GetEffectiveLineStyle() );

    aPlotter->MoveTo( m_start );
    aPlotter->FinishTo( m_end );

    aPlotter->SetDash( penWidth, LINE_STYLE::SOLID );

    // Plot attributes to a hypertext menu
    std::vector<wxString> properties;
    BOX2I                 bbox = GetBoundingBox();
    bbox.Inflate( penWidth * 3 );

    if( aPlotOpts.m_PDFPropertyPopups )
    {
        if( GetLayer() == LAYER_WIRE )
        {
            if( SCH_CONNECTION* connection = Connection() )
            {
                properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                           _( "Net" ),
                                                           connection->Name() ) );

                properties.emplace_back( wxString::Format( wxT( "!%s = %s" ),
                                                           _( "Resolved netclass" ),
                                                           GetEffectiveNetClass()->GetHumanReadableName() ) );
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

    LINE_STYLE lineStyle = GetStroke().GetLineStyle();

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
                                UnescapeString( GetEffectiveNetClass()->GetHumanReadableName() ) );
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


bool SCH_LINE::operator==( const SCH_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const SCH_LINE& other = static_cast<const SCH_LINE&>( aOther );

    if( GetLayer() != other.GetLayer() )
        return false;

    if( m_start != other.m_start )
        return false;

    if( m_end != other.m_end )
        return false;

    if( m_stroke.GetWidth() != other.m_stroke.GetWidth() )
        return false;

    if( m_stroke.GetColor() != other.m_stroke.GetColor() )
        return false;

    if( m_stroke.GetLineStyle() != other.m_stroke.GetLineStyle() )
        return false;

    return true;
}


double SCH_LINE::Similarity( const SCH_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( Type() != aOther.Type() )
        return 0.0;

    const SCH_LINE& other = static_cast<const SCH_LINE&>( aOther );

    if( GetLayer() != other.GetLayer() )
        return 0.0;

    double similarity = 1.0;

    if( m_start != other.m_start )
        similarity *= 0.9;

    if( m_end != other.m_end )
        similarity *= 0.9;

    if( m_stroke.GetWidth() != other.m_stroke.GetWidth() )
        similarity *= 0.9;

    if( m_stroke.GetColor() != other.m_stroke.GetColor() )
        similarity *= 0.9;

    if( m_stroke.GetLineStyle() != other.m_stroke.GetLineStyle() )
        similarity *= 0.9;

    return similarity;
}


bool SCH_LINE::ShouldHopOver( const SCH_LINE* aLine ) const
{
    // try to find if this should hop over aLine. Horizontal wires have preference for hop.
    bool isMeVertical = ( m_end.x == m_start.x );
    bool isCandidateVertical = ( aLine->GetEndPoint().x == aLine->GetStartPoint().x );

    // Vertical vs. Horizontal: Horizontal should hop
    if( isMeVertical && !isCandidateVertical )
        return false;

    if( isCandidateVertical && !isMeVertical )
        return true;

    // Both this and aLine have a slope. Try to find the best candidate
    double slopeMe = ( m_end.y - m_start.y ) / (double) ( m_end.x - m_start.x );
    double slopeCandidate = ( aLine->GetEndPoint().y - aLine->GetStartPoint().y )
                        / (double) ( aLine->GetEndPoint().x - aLine->GetStartPoint().x );

    if( fabs( slopeMe ) == fabs( slopeCandidate ) )     // Can easily happen with 45 deg wires
        return slopeMe < slopeCandidate;                // signs are certainly different

    return fabs( slopeMe ) < fabs( slopeCandidate ); // The shallower line should hop
}


std::vector<VECTOR3I> SCH_LINE::BuildWireWithHopShape( const SCH_SCREEN* aScreen,
                                                       double aArcRadius ) const
{
    // Note: Points are VECTOR3D, with Z coord used as flag
    // for segments: start point and end point have the Z coord = 0
    // for arcs: start point middle point and end point have the Z coord = 1

    std::vector<VECTOR3I> wire_shape;       // List of coordinates:
                                            // 2 points for a segment, 3 points for an arc

    if( !IsWire() )
    {
        wire_shape.emplace_back( GetStartPoint().x,GetStartPoint().y, 0 );
        wire_shape.emplace_back( GetEndPoint().x, GetEndPoint().y, 0 );
        return wire_shape;
    }

    std::vector<SCH_LINE*> existingWires;   // wires to test (candidates)
    std::vector<VECTOR2I> intersections;

    for( SCH_ITEM* item : aScreen->Items().Overlapping( SCH_LINE_T, GetBoundingBox() ) )
    {
        SCH_LINE* line = static_cast<SCH_LINE*>( item );

        if( line->IsWire() )
            existingWires.push_back( line );
    }

    VECTOR2I currentLineStartPoint = GetStartPoint();
    VECTOR2I currentLineEndPoint = GetEndPoint();

    for( SCH_LINE* existingLine : existingWires )
    {
        VECTOR2I extLineStartPoint = existingLine->GetStartPoint();
        VECTOR2I extLineEndPoint = existingLine->GetEndPoint();

        if( extLineStartPoint == currentLineStartPoint && extLineEndPoint == currentLineEndPoint )
            continue;

        if( !ShouldHopOver( existingLine ) )
            continue;

        SEG currentSegment = SEG( currentLineStartPoint, currentLineEndPoint );
        SEG existingSegment = SEG( extLineStartPoint, extLineEndPoint );

        if( OPT_VECTOR2I intersect = currentSegment.Intersect( existingSegment, true, false ) )
        {
            if( IsEndPoint( *intersect ) || existingLine->IsEndPoint( *intersect ) )
                continue;

            // Ensure intersecting point is not yet entered. it can be already just entered
            // if more than two wires are intersecting at the same point,
            // creating bad hop over shapes for the current wire
            if( intersections.size() == 0 || intersections.back() != *intersect )
                intersections.push_back( *intersect );
        }
    }

    if( intersections.empty() )
    {
        wire_shape.emplace_back( currentLineStartPoint.x, currentLineStartPoint.y, 0 );
        wire_shape.emplace_back( currentLineEndPoint.x, currentLineEndPoint.y, 0 );
    }
    else
    {
        auto getDistance =
                []( const VECTOR2I& a, const VECTOR2I& b ) -> double
                {
                    return std::sqrt( std::pow( a.x - b.x, 2 ) + std::pow( a.y - b.y, 2 ) );
                };

        std::sort( intersections.begin(), intersections.end(),
                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       return getDistance( GetStartPoint(), a ) < getDistance( GetStartPoint(), b );
                   } );

        VECTOR2I currentStart = GetStartPoint();
        double   R = aArcRadius;

        for( const VECTOR2I& hopMid : intersections )
        {
            // Calculate the angle of the line from start point to end point in radians
            double lineAngle = std::atan2( GetEndPoint().y - GetStartPoint().y,
                                           GetEndPoint().x - GetStartPoint().x );

            // Convert the angle from radians to degrees
            double lineAngleDeg = lineAngle * ( 180.0f / M_PI );

            // Normalize the angle to be between 0 and 360 degrees
            if( lineAngleDeg < 0 )
                lineAngleDeg += 360;

            double startAngle = lineAngleDeg;
            double endAngle = startAngle + 180.0f;

            // Adjust the end angle if it exceeds 360 degrees
            if( endAngle >= 360.0 )
                endAngle -= 360.0;

            // Convert start and end angles from degrees to radians
            double startAngleRad = startAngle * ( M_PI / 180.0f );
            double endAngleRad = endAngle * ( M_PI / 180.0f );

            VECTOR2I arcMidPoint = {
                hopMid.x + static_cast<int>( R * cos( ( startAngleRad + endAngleRad ) / 2.0f ) ),
                hopMid.y - static_cast<int>( R * sin( ( startAngleRad + endAngleRad ) / 2.0f ) )
            };

            VECTOR2I beforeHop = hopMid - KiROUND( R * std::cos( lineAngle ), R * std::sin( lineAngle ) );
            VECTOR2I afterHop = hopMid + KiROUND( R * std::cos( lineAngle ), R * std::sin( lineAngle ) );

            // Draw the line from the current start point to the before-hop point
            wire_shape.emplace_back( currentStart.x, currentStart.y, 0 );
            wire_shape.emplace_back( beforeHop.x, beforeHop.y, 0 );

            // Create an arc object
            SHAPE_ARC arc( beforeHop, arcMidPoint, afterHop, 0 );
            wire_shape.emplace_back( beforeHop.x, beforeHop.y, 1 );
            wire_shape.emplace_back( arcMidPoint.x, arcMidPoint.y, 1 );
            wire_shape.emplace_back( afterHop.x, afterHop.y, 1 );

            currentStart = afterHop;
        }

        // Draw the final line from the current start point to the end point of the original line
        wire_shape.emplace_back( currentStart. x,currentStart.y, 0 );
        wire_shape.emplace_back( GetEndPoint().x, GetEndPoint().y, 0 );
    }

    return wire_shape;
}


static struct SCH_LINE_DESC
{
    SCH_LINE_DESC()
    {
        ENUM_MAP<LINE_STYLE>& lineStyleEnum = ENUM_MAP<LINE_STYLE>::Instance();

        if( lineStyleEnum.Choices().GetCount() == 0 )
        {
            lineStyleEnum.Map( LINE_STYLE::SOLID,      _HKI( "Solid" ) )
                         .Map( LINE_STYLE::DASH,       _HKI( "Dashed" ) )
                         .Map( LINE_STYLE::DOT,        _HKI( "Dotted" ) )
                         .Map( LINE_STYLE::DASHDOT,    _HKI( "Dash-Dot" ) )
                         .Map( LINE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        ENUM_MAP<WIRE_STYLE>& wireLineStyleEnum = ENUM_MAP<WIRE_STYLE>::Instance();

        if( wireLineStyleEnum.Choices().GetCount() == 0 )
        {
            wireLineStyleEnum.Map( WIRE_STYLE::DEFAULT,    _HKI( "Default" ) )
                             .Map( WIRE_STYLE::SOLID,      _HKI( "Solid" ) )
                             .Map( WIRE_STYLE::DASH,       _HKI( "Dashed" ) )
                             .Map( WIRE_STYLE::DOT,        _HKI( "Dotted" ) )
                             .Map( WIRE_STYLE::DASHDOT,    _HKI( "Dash-Dot" ) )
                             .Map( WIRE_STYLE::DASHDOTDOT, _HKI( "Dash-Dot-Dot" ) );
        }

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( SCH_LINE );
        propMgr.InheritsAfter( TYPE_HASH( SCH_LINE ), TYPE_HASH( SCH_ITEM ) );

        auto isGraphicLine =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_LINE* line = dynamic_cast<SCH_LINE*>( aItem ) )
                        return line->IsGraphicLine();

                    return false;
                };

        auto isWireOrBus =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( SCH_LINE* line = dynamic_cast<SCH_LINE*>( aItem ) )
                        return line->IsWire() || line->IsBus();

                    return false;
                };

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "Start X" ),
                    &SCH_LINE::SetStartX, &SCH_LINE::GetStartX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "Start Y" ),
                    &SCH_LINE::SetStartY, &SCH_LINE::GetStartY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "End X" ),
                    &SCH_LINE::SetEndX, &SCH_LINE::GetEndX, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_X_COORD ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "End Y" ),
                    &SCH_LINE::SetEndY, &SCH_LINE::GetEndY, PROPERTY_DISPLAY::PT_COORD,
                    ORIGIN_TRANSFORMS::ABS_Y_COORD ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, double>( _HKI( "Length" ),
                    &SCH_LINE::SetLength, &SCH_LINE::GetLength, PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_LINE, LINE_STYLE>( _HKI( "Line Style" ),
                    &SCH_LINE::SetLineStyle, &SCH_LINE::GetLineStyle ) )
                .SetAvailableFunc( isGraphicLine );

        propMgr.AddProperty( new PROPERTY_ENUM<SCH_LINE, WIRE_STYLE>( _HKI( "Line Style" ),
                    &SCH_LINE::SetWireStyle, &SCH_LINE::GetWireStyle ) )
                .SetAvailableFunc( isWireOrBus );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, int>( _HKI( "Line Width" ),
                    &SCH_LINE::SetLineWidth, &SCH_LINE::GetLineWidth, PROPERTY_DISPLAY::PT_SIZE ) );

        propMgr.AddProperty( new PROPERTY<SCH_LINE, COLOR4D>( _HKI( "Color" ),
                    &SCH_LINE::SetLineColor, &SCH_LINE::GetLineColor ) );
    }
} _SCH_LINE_DESC;

IMPLEMENT_ENUM_TO_WXANY( WIRE_STYLE )

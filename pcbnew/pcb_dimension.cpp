/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <pcb_dimension.h>
#include <pcb_text.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>


static const EDA_ANGLE s_arrowAngle( 27.5, DEGREES_T );


PCB_DIMENSION_BASE::PCB_DIMENSION_BASE( BOARD_ITEM* aParent, KICAD_T aType ) :
        BOARD_ITEM( aParent, aType ),
        m_overrideTextEnabled( false ),
        m_units( EDA_UNITS::INCHES ),
        m_autoUnits( false ),
        m_unitsFormat( DIM_UNITS_FORMAT::BARE_SUFFIX ),
        m_precision( 4 ),
        m_suppressZeroes( false ),
        m_lineThickness( Millimeter2iu( 0.2 ) ),
        m_arrowLength( Mils2iu( 50 ) ),
        m_extensionOffset( 0 ),
        m_textPosition( DIM_TEXT_POSITION::OUTSIDE ),
        m_keepTextAligned( true ),
        m_text( aParent ),
        m_measuredValue( 0 )
{
    m_layer = Dwgs_User;
}


void PCB_DIMENSION_BASE::SetParent( EDA_ITEM* aParent )
{
    BOARD_ITEM::SetParent( aParent );
    m_text.SetParent( aParent );
}


void PCB_DIMENSION_BASE::updateText()
{
    wxString text = m_overrideTextEnabled ? m_valueString : GetValueText();

    switch( m_unitsFormat )
    {
    case DIM_UNITS_FORMAT::NO_SUFFIX: // no units
        break;

    case DIM_UNITS_FORMAT::BARE_SUFFIX: // normal
        text += wxS( " " );
        text += GetAbbreviatedUnitsLabel( m_units );
        break;

    case DIM_UNITS_FORMAT::PAREN_SUFFIX: // parenthetical
        text += wxT( " (" );
        text += GetAbbreviatedUnitsLabel( m_units );
        text += wxT( ")" );
        break;
    }

    text.Prepend( m_prefix );
    text.Append( m_suffix );

    m_text.SetText( text );
}


template<typename ShapeType>
void PCB_DIMENSION_BASE::addShape( const ShapeType& aShape )
{
    m_shapes.push_back( std::make_shared<ShapeType>( aShape ) );
}


wxString PCB_DIMENSION_BASE::GetValueText() const
{
    struct lconv* lc = localeconv();
    wxChar sep = lc->decimal_point[0];

    int      val = GetMeasuredValue();
    wxString text;
    wxString format = wxT( "%." ) + wxString::Format( wxT( "%i" ), m_precision ) + wxT( "f" );

    text.Printf( format, To_User_Unit( m_units, val ) );

    if( m_suppressZeroes )
    {
        while( text.Last() == '0' )
        {
            text.RemoveLast();

            if( text.Last() == '.' || text.Last() == sep )
            {
                text.RemoveLast();
                break;
            }
        }
    }

    return text;
}


void PCB_DIMENSION_BASE::SetPrefix( const wxString& aPrefix )
{
    m_prefix = aPrefix;
}


void PCB_DIMENSION_BASE::SetSuffix( const wxString& aSuffix )
{
    m_suffix = aSuffix;
}


void PCB_DIMENSION_BASE::SetUnits( EDA_UNITS aUnits )
{
    m_units = aUnits;
}


DIM_UNITS_MODE PCB_DIMENSION_BASE::GetUnitsMode() const
{
    if( m_autoUnits )
    {
        return DIM_UNITS_MODE::AUTOMATIC;
    }
    else
    {
        switch( m_units )
        {
        default:
        case EDA_UNITS::INCHES:      return DIM_UNITS_MODE::INCHES;
        case EDA_UNITS::MILLIMETRES: return DIM_UNITS_MODE::MILLIMETRES;
        case EDA_UNITS::MILS:        return DIM_UNITS_MODE::MILS;
        }
    }
}


void PCB_DIMENSION_BASE::SetUnitsMode( DIM_UNITS_MODE aMode )
{
    m_autoUnits = false;

    switch( aMode )
    {
    case DIM_UNITS_MODE::INCHES:      m_units = EDA_UNITS::INCHES;      break;
    case DIM_UNITS_MODE::MILS:        m_units = EDA_UNITS::MILS;        break;
    case DIM_UNITS_MODE::MILLIMETRES: m_units = EDA_UNITS::MILLIMETRES; break;
    case DIM_UNITS_MODE::AUTOMATIC:   m_autoUnits = true;               break;
    }
}


void PCB_DIMENSION_BASE::SetText( const wxString& aNewText )
{
    m_valueString = aNewText;
    updateText();
}


const wxString PCB_DIMENSION_BASE::GetText() const
{
    return m_text.GetText();
}


void PCB_DIMENSION_BASE::SetLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
    m_text.SetLayer( aLayer );
}


void PCB_DIMENSION_BASE::Move( const VECTOR2I& offset )
{
    m_text.Offset( offset );

    m_start += offset;
    m_end   += offset;

    Update();
}


void PCB_DIMENSION_BASE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE newAngle = m_text.GetTextAngle() + aAngle;

    newAngle.Normalize();

    m_text.SetTextAngle( newAngle );

    VECTOR2I pt = m_text.GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    m_text.SetTextPos( pt );

    RotatePoint( m_start, aRotCentre, aAngle );
    RotatePoint( m_end, aRotCentre, aAngle );

    Update();
}


void PCB_DIMENSION_BASE::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    Mirror( aCentre );

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}


void PCB_DIMENSION_BASE::Mirror( const VECTOR2I& axis_pos, bool aMirrorLeftRight )
{
    int axis = aMirrorLeftRight ? axis_pos.x : axis_pos.y;
    VECTOR2I newPos = m_text.GetTextPos();

#define INVERT( pos ) ( ( pos ) = axis - ( ( pos ) - axis ) )
    if( aMirrorLeftRight )
        INVERT( newPos.x );
    else
        INVERT( newPos.y );

    m_text.SetTextPos( newPos );

    // invert angle
    m_text.SetTextAngle( -m_text.GetTextAngle() );

    if( aMirrorLeftRight )
    {
        INVERT( m_start.x );
        INVERT( m_end.x );
    }
    else
    {
        INVERT( m_start.y );
        INVERT( m_end.y );
    }

    m_text.SetMirrored( !m_text.IsMirrored() );

    Update();
}


void PCB_DIMENSION_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    // for now, display only the text within the DIMENSION using class PCB_TEXT.
    wxString    msg;

    wxCHECK_RET( m_parent != nullptr, wxT( "PCB_TEXT::GetMsgPanelInfo() m_Parent is NULL." ) );

    aList.emplace_back( _( "Dimension" ), m_text.GetShownText() );

    aList.emplace_back( _( "Prefix" ), GetPrefix() );

    if( GetOverrideTextEnabled() )
    {
        aList.emplace_back( _( "Override Text" ), GetOverrideText() );
    }
    else
    {
        aList.emplace_back( _( "Value" ), GetValueText() );

        msg = wxT( "%" ) + wxString::Format( wxT( "1.%df" ), GetPrecision() );
        aList.emplace_back( _( "Precision" ), wxString::Format( msg, 0.0 ) );
    }

    aList.emplace_back( _( "Suffix" ), GetSuffix() );

    EDA_UNITS units;

    GetUnits( units );
    aList.emplace_back( _( "Units" ), GetAbbreviatedUnitsLabel( units ).Trim( false ) );

    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    units = aFrame->GetUserUnits();

    if( Type() == PCB_DIM_CENTER_T || Type() == PCB_FP_DIM_CENTER_T )
    {
        VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                           MessageTextFromValue( units, startCoord.x ),
                                           MessageTextFromValue( units, startCoord.y ) );

        aList.emplace_back( start, wxEmptyString );
    }
    else
    {
        VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                           MessageTextFromValue( units, startCoord.x ),
                                           MessageTextFromValue( units, startCoord.y ) );
        VECTOR2I endCoord = originTransforms.ToDisplayAbs( GetEnd() );
        wxString end   = wxString::Format( wxT( "@(%s, %s)" ),
                                           MessageTextFromValue( units, endCoord.x ),
                                           MessageTextFromValue( units, endCoord.y ) );

        aList.emplace_back( start, end );
    }

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


std::shared_ptr<SHAPE> PCB_DIMENSION_BASE::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    std::shared_ptr<SHAPE_COMPOUND> effectiveShape = std::make_shared<SHAPE_COMPOUND>();

    effectiveShape->AddShape( Text().GetEffectiveTextShape()->Clone() );

    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
        effectiveShape->AddShape( shape->Clone() );

    return effectiveShape;
}


bool PCB_DIMENSION_BASE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( m_text.TextHitTest( aPosition ) )
        return true;

    int dist_max = aAccuracy + ( m_lineThickness / 2 );

    // Locate SEGMENTS

    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
    {
        if( shape->Collide( aPosition, dist_max ) )
            return true;
    }

    return false;
}


bool PCB_DIMENSION_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    EDA_RECT rect = GetBoundingBox();

    if( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( rect );

    return arect.Intersects( rect );
}


const EDA_RECT PCB_DIMENSION_BASE::GetBoundingBox() const
{
    EDA_RECT    bBox;
    int         xmin, xmax, ymin, ymax;

    bBox    = m_text.GetTextBox();
    xmin    = bBox.GetX();
    xmax    = bBox.GetRight();
    ymin    = bBox.GetY();
    ymax    = bBox.GetBottom();

    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
    {
        BOX2I shapeBox = shape->BBox();
        shapeBox.Inflate( m_lineThickness / 2 );

        xmin = std::min( xmin, shapeBox.GetOrigin().x );
        xmax = std::max( xmax, shapeBox.GetEnd().x );
        ymin = std::min( ymin, shapeBox.GetOrigin().y );
        ymax = std::max( ymax, shapeBox.GetEnd().y );
    }

    bBox.SetX( xmin );
    bBox.SetY( ymin );
    bBox.SetWidth( xmax - xmin + 1 );
    bBox.SetHeight( ymax - ymin + 1 );

    bBox.Normalize();

    return bBox;
}


wxString PCB_DIMENSION_BASE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Dimension '%s' on %s" ), GetText(), GetLayerName() );
}



const BOX2I PCB_DIMENSION_BASE::ViewBBox() const
{
    BOX2I dimBBox = BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                           VECTOR2I( GetBoundingBox().GetSize() ) );
    dimBBox.Merge( m_text.ViewBBox() );

    return dimBBox;
}


OPT_VECTOR2I PCB_DIMENSION_BASE::segPolyIntersection( const SHAPE_POLY_SET& aPoly, const SEG& aSeg,
                                                      bool aStart )
{
    VECTOR2I start( aStart ? aSeg.A : aSeg.B );
    VECTOR2I endpoint( aStart ? aSeg.B : aSeg.A );

    if( aPoly.Contains( start ) )
        return NULLOPT;

    for( SHAPE_POLY_SET::CONST_SEGMENT_ITERATOR seg = aPoly.CIterateSegments(); seg; ++seg )
    {
        if( OPT_VECTOR2I intersection = ( *seg ).Intersect( aSeg ) )
        {
            if( ( *intersection - start ).SquaredEuclideanNorm() <
                ( endpoint - start ).SquaredEuclideanNorm() )
                endpoint = *intersection;
        }
    }

    if( start == endpoint )
        return NULLOPT;

    return OPT_VECTOR2I( endpoint );
}


OPT_VECTOR2I PCB_DIMENSION_BASE::segCircleIntersection( CIRCLE& aCircle, SEG& aSeg, bool aStart )
{
    VECTOR2I start( aStart ? aSeg.A : aSeg.B );
    VECTOR2I endpoint( aStart ? aSeg.B : aSeg.A );

    if( aCircle.Contains( start ) )
        return NULLOPT;

    std::vector<VECTOR2I> intersections = aCircle.Intersect( aSeg );

    for( VECTOR2I& intersection : aCircle.Intersect( aSeg ) )
    {
        if( ( intersection - start ).SquaredEuclideanNorm() <
            ( endpoint - start ).SquaredEuclideanNorm() )
            endpoint = intersection;
    }

    if( start == endpoint )
        return NULLOPT;

    return OPT_VECTOR2I( endpoint );
}


void PCB_DIMENSION_BASE::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                               PCB_LAYER_ID aLayer, int aClearance,
                                                               int aError, ERROR_LOC aErrorLoc,
                                                               bool aIgnoreLineWidth ) const
{
    wxASSERT_MSG( !aIgnoreLineWidth, wxT( "IgnoreLineWidth has no meaning for dimensions." ) );

    for( const std::shared_ptr<SHAPE>& shape : m_shapes )
    {
        const SHAPE_CIRCLE*  circle = dynamic_cast<const SHAPE_CIRCLE*>( shape.get() );
        const SHAPE_SEGMENT* seg    = dynamic_cast<const SHAPE_SEGMENT*>( shape.get() );

        if( circle )
        {
            TransformCircleToPolygon( aCornerBuffer, circle->GetCenter(),
                                      circle->GetRadius() + m_lineThickness / 2 + aClearance,
                                      aError, aErrorLoc );
        }
        else if( seg )
        {
            TransformOvalToPolygon( aCornerBuffer, seg->GetSeg().A,
                                    seg->GetSeg().B, m_lineThickness + 2 * aClearance,
                                    aError, aErrorLoc );
        }
        else
        {
            wxFAIL_MSG( wxT( "PCB_DIMENSION_BASE::TransformShapeWithClearanceToPolygon unexpected "
                             "shape type." ) );
        }
    }
}


PCB_DIM_ALIGNED::PCB_DIM_ALIGNED( BOARD_ITEM* aParent, KICAD_T aType ) :
        PCB_DIMENSION_BASE( aParent, aType ),
        m_height( 0 )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = static_cast<int>( m_arrowLength * s_arrowAngle.Sin() );
}


EDA_ITEM* PCB_DIM_ALIGNED::Clone() const
{
    return new PCB_DIM_ALIGNED( *this );
}


void PCB_DIM_ALIGNED::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_ALIGNED*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_ALIGNED*>( this ), *static_cast<PCB_DIM_ALIGNED*>( aImage ) );

    Update();
}


BITMAPS PCB_DIM_ALIGNED::GetMenuImage() const
{
    return BITMAPS::add_aligned_dimension;
}


void PCB_DIM_ALIGNED::UpdateHeight( const VECTOR2I& aCrossbarStart, const VECTOR2I& aCrossbarEnd )
{
    VECTOR2D height( aCrossbarStart - GetStart() );
    VECTOR2D crossBar( aCrossbarEnd - aCrossbarStart );

    if( height.Cross( crossBar ) > 0 )
        m_height = -height.EuclideanNorm();
    else
        m_height = height.EuclideanNorm();

    Update();
}


void PCB_DIM_ALIGNED::updateGeometry()
{
    m_shapes.clear();

    VECTOR2I dimension( m_end - m_start );

    m_measuredValue = KiROUND( dimension.EuclideanNorm() );

    VECTOR2I extension;

    if( m_height > 0 )
        extension = VECTOR2I( -dimension.y, dimension.x );
    else
        extension = VECTOR2I( dimension.y, -dimension.x );

    // Add extension lines
    int extensionHeight = std::abs( m_height ) - m_extensionOffset + m_extensionHeight;

    VECTOR2I extStart( m_start );
    extStart += extension.Resize( m_extensionOffset );

    addShape( SHAPE_SEGMENT( extStart, extStart + extension.Resize( extensionHeight ) ) );

    extStart = VECTOR2I( m_end );
    extStart += extension.Resize( m_extensionOffset );

    addShape( SHAPE_SEGMENT( extStart, extStart + extension.Resize( extensionHeight ) ) );

    // Add crossbar
    VECTOR2I crossBarDistance = sign( m_height ) * extension.Resize( m_height );
    m_crossBarStart = m_start + crossBarDistance;
    m_crossBarEnd   = m_end + crossBarDistance;

    // Update text after calculating crossbar position but before adding crossbar lines
    updateText();

    // Now that we have the text updated, we can determine how to draw the crossbar.
    // First we need to create an appropriate bounding polygon to collide with
    EDA_RECT textBox = m_text.GetTextBox().Inflate( m_text.GetTextWidth() / 2,
                                                    m_text.GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( m_text.GetTextAngle(), textBox.GetCenter() );

    // The ideal crossbar, if the text doesn't collide
    SEG crossbar( m_crossBarStart, m_crossBarEnd );

    // Now we can draw 0, 1, or 2 crossbar lines depending on how the polygon collides
    bool containsA = polyBox.Contains( crossbar.A );
    bool containsB = polyBox.Contains( crossbar.B );

    OPT_VECTOR2I endpointA = segPolyIntersection( polyBox, crossbar );
    OPT_VECTOR2I endpointB = segPolyIntersection( polyBox, crossbar, false );

    if( endpointA )
        m_shapes.emplace_back( new SHAPE_SEGMENT( crossbar.A, *endpointA ) );

    if( endpointB )
        m_shapes.emplace_back( new SHAPE_SEGMENT( *endpointB, crossbar.B ) );

    if( !containsA && !containsB && !endpointA && !endpointB )
        m_shapes.emplace_back( new SHAPE_SEGMENT( crossbar ) );

    // Add arrows
    VECTOR2I arrowEndPos( m_arrowLength, 0 );
    VECTOR2I arrowEndNeg( m_arrowLength, 0 );
    RotatePoint( arrowEndPos, -EDA_ANGLE( dimension ) + s_arrowAngle );
    RotatePoint( arrowEndNeg, -EDA_ANGLE( dimension ) - s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart, m_crossBarStart + arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart, m_crossBarStart + arrowEndNeg ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd, m_crossBarEnd - arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd, m_crossBarEnd - arrowEndNeg ) );
}


void PCB_DIM_ALIGNED::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = m_text.GetEffectiveTextPenWidth() + m_text.GetTextHeight();

        EDA_ANGLE rotation;

        if( crossbarCenter.x == 0 )
            rotation = ANGLE_90 * sign( -crossbarCenter.y );
        else if( crossbarCenter.x < 0 )
            rotation = -ANGLE_90;
        else
            rotation = ANGLE_90;

        VECTOR2I textOffset = crossbarCenter;
        RotatePoint( crossbarCenter, rotation );
        textOffset += crossbarCenter.Resize( textOffsetDistance );

        m_text.SetTextPos( m_crossBarStart + textOffset );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        m_text.SetTextPos( m_crossBarStart + crossbarCenter );
    }

    if( m_keepTextAligned )
    {
        EDA_ANGLE textAngle = FULL_CIRCLE - EDA_ANGLE( crossbarCenter );
        textAngle.Normalize();

        if( textAngle > ANGLE_90 && textAngle <= ANGLE_270 )
            textAngle -= ANGLE_180;

        m_text.SetTextAngle( textAngle );
    }

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_ALIGNED::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    PCB_DIMENSION_BASE::GetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Height" ), MessageTextFromValue( aFrame->GetUserUnits(), m_height ) );
}


PCB_DIM_ORTHOGONAL::PCB_DIM_ORTHOGONAL( BOARD_ITEM* aParent, bool aInFP ) :
        PCB_DIM_ALIGNED( aParent, aInFP ? PCB_FP_DIM_ORTHOGONAL_T : PCB_DIM_ORTHOGONAL_T )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = static_cast<int>( m_arrowLength * s_arrowAngle.Sin() );
    m_orientation = DIR::HORIZONTAL;
}


EDA_ITEM* PCB_DIM_ORTHOGONAL::Clone() const
{
    return new PCB_DIM_ORTHOGONAL( *this );
}


void PCB_DIM_ORTHOGONAL::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_ORTHOGONAL*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_ORTHOGONAL*>( this ),
               *static_cast<PCB_DIM_ORTHOGONAL*>( aImage ) );

    Update();
}


BITMAPS PCB_DIM_ORTHOGONAL::GetMenuImage() const
{
    return BITMAPS::add_orthogonal_dimension;
}


void PCB_DIM_ORTHOGONAL::updateGeometry()
{
    m_shapes.clear();

    int measurement = ( m_orientation == DIR::HORIZONTAL ? m_end.x - m_start.x :
                                                           m_end.y - m_start.y );
    m_measuredValue = KiROUND( std::abs( measurement ) );

    VECTOR2I extension;

    if( m_orientation == DIR::HORIZONTAL )
        extension = VECTOR2I( 0, m_height );
    else
        extension = VECTOR2I( m_height, 0 );

    // Add first extension line
    int extensionHeight = std::abs( m_height ) - m_extensionOffset + m_extensionHeight;

    VECTOR2I extStart( m_start );
    extStart += extension.Resize( m_extensionOffset );

    addShape( SHAPE_SEGMENT( extStart, extStart + extension.Resize( extensionHeight ) ) );

    // Add crossbar
    VECTOR2I crossBarDistance = sign( m_height ) * extension.Resize( m_height );
    m_crossBarStart = m_start + crossBarDistance;

    if( m_orientation == DIR::HORIZONTAL )
        m_crossBarEnd = VECTOR2I( m_end.x, m_crossBarStart.y );
    else
        m_crossBarEnd = VECTOR2I( m_crossBarStart.x, m_end.y );

    // Add second extension line (m_end to crossbar end)
    if( m_orientation == DIR::HORIZONTAL )
        extension = VECTOR2I( 0, m_end.y - m_crossBarEnd.y );
    else
        extension = VECTOR2I( m_end.x - m_crossBarEnd.x, 0 );

    extensionHeight = extension.EuclideanNorm() - m_extensionOffset + m_extensionHeight;

    extStart = VECTOR2I( m_crossBarEnd );
    extStart -= extension.Resize( m_extensionHeight );

    addShape( SHAPE_SEGMENT( extStart, extStart + extension.Resize( extensionHeight ) ) );

    // Update text after calculating crossbar position but before adding crossbar lines
    updateText();

    // Now that we have the text updated, we can determine how to draw the crossbar.
    // First we need to create an appropriate bounding polygon to collide with
    EDA_RECT textBox = m_text.GetTextBox().Inflate( m_text.GetTextWidth() / 2,
                                                    m_text.GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( m_text.GetTextAngle(), textBox.GetCenter() );

    // The ideal crossbar, if the text doesn't collide
    SEG crossbar( m_crossBarStart, m_crossBarEnd );

    // Now we can draw 0, 1, or 2 crossbar lines depending on how the polygon collides
    bool containsA = polyBox.Contains( crossbar.A );
    bool containsB = polyBox.Contains( crossbar.B );

    OPT_VECTOR2I endpointA = segPolyIntersection( polyBox, crossbar );
    OPT_VECTOR2I endpointB = segPolyIntersection( polyBox, crossbar, false );

    if( endpointA )
        m_shapes.emplace_back( new SHAPE_SEGMENT( crossbar.A, *endpointA ) );

    if( endpointB )
        m_shapes.emplace_back( new SHAPE_SEGMENT( *endpointB, crossbar.B ) );

    if( !containsA && !containsB && !endpointA && !endpointB )
        m_shapes.emplace_back( new SHAPE_SEGMENT( crossbar ) );

    // Add arrows
    EDA_ANGLE crossBarAngle( m_crossBarEnd - m_crossBarStart );
    VECTOR2I  arrowEndPos( m_arrowLength, 0 );
    VECTOR2I  arrowEndNeg( m_arrowLength, 0 );
    RotatePoint( arrowEndPos, -crossBarAngle + s_arrowAngle );
    RotatePoint( arrowEndNeg, -crossBarAngle - s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart, m_crossBarStart + arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart, m_crossBarStart + arrowEndNeg ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd, m_crossBarEnd - arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd, m_crossBarEnd - arrowEndNeg ) );
}


void PCB_DIM_ORTHOGONAL::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = m_text.GetEffectiveTextPenWidth() + m_text.GetTextHeight();

        VECTOR2I textOffset;

        if( m_orientation == DIR::HORIZONTAL )
            textOffset.y = -textOffsetDistance;
        else
            textOffset.x = -textOffsetDistance;

        textOffset += crossbarCenter;

        m_text.SetTextPos( m_crossBarStart + textOffset );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        m_text.SetTextPos( m_crossBarStart + crossbarCenter );
    }

    if( m_keepTextAligned )
    {
        if( abs( crossbarCenter.x ) > abs( crossbarCenter.y ) )
            m_text.SetTextAngle( ANGLE_HORIZONTAL );
        else
            m_text.SetTextAngle( ANGLE_VERTICAL );
    }

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_ORTHOGONAL::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE angle( aAngle );

    // restrict angle to -179.9 to 180.0 degrees
    angle.Normalize180();

    // adjust orientation and height to new angle
    // we can only handle the cases of -90, 0, 90, 180 degrees exactly;
    // in the other cases we will use the nearest 90 degree angle to
    // choose at least an approximate axis for the target orientation
    // In case of exactly 45 or 135 degrees, we will round towards zero for consistency
    if( angle > ANGLE_45 && angle <= ANGLE_135 )
    {
        // about 90 degree
        if( m_orientation == DIR::HORIZONTAL )
        {
            m_orientation = DIR::VERTICAL;
        }
        else
        {
            m_orientation = DIR::HORIZONTAL;
            m_height = -m_height;
        }
    }
    else if( angle < -ANGLE_45 && angle >= -ANGLE_135 )
    {
        // about -90 degree
        if( m_orientation == DIR::HORIZONTAL )
        {
            m_orientation = DIR::VERTICAL;
            m_height = -m_height;
        }
        else
        {
            m_orientation = DIR::HORIZONTAL;
        }
    }
    else if( angle > ANGLE_135 || angle < -ANGLE_135 )
    {
        // about 180 degree
        m_height = -m_height;
    }

    // this will update m_crossBarStart and m_crossbarEnd
    PCB_DIMENSION_BASE::Rotate( aRotCentre, angle );
}


PCB_DIM_LEADER::PCB_DIM_LEADER( BOARD_ITEM* aParent, bool aInFP ) :
        PCB_DIMENSION_BASE( aParent, aInFP ? PCB_FP_DIM_LEADER_T : PCB_DIM_LEADER_T ),
        m_textBorder( DIM_TEXT_BORDER::NONE )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
    m_keepTextAligned     = false;

    SetText( _( "Leader" ) );
}


EDA_ITEM* PCB_DIM_LEADER::Clone() const
{
    return new PCB_DIM_LEADER( *this );
}


void PCB_DIM_LEADER::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_LEADER*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_LEADER*>( this ), *static_cast<PCB_DIM_LEADER*>( aImage ) );

    Update();
}


BITMAPS PCB_DIM_LEADER::GetMenuImage() const
{
    return BITMAPS::add_leader;
}


void PCB_DIM_LEADER::updateGeometry()
{
    m_shapes.clear();

    updateText();

    // Now that we have the text updated, we can determine how to draw the second line
    // First we need to create an appropriate bounding polygon to collide with
    EDA_RECT textBox = m_text.GetTextBox().Inflate( m_text.GetTextWidth() / 2,
                                                    m_text.GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( m_text.GetTextAngle(), textBox.GetCenter() );

    VECTOR2I firstLine( m_end - m_start );
    VECTOR2I start( m_start );
    start += firstLine.Resize( m_extensionOffset );

    SEG arrowSeg( m_start, m_end );
    SEG textSeg( m_end, m_text.GetPosition() );
    OPT_VECTOR2I arrowSegEnd = boost::make_optional( false, VECTOR2I() );;
    OPT_VECTOR2I textSegEnd = boost::make_optional( false, VECTOR2I() );

    if( m_textBorder == DIM_TEXT_BORDER::CIRCLE )
    {
        double penWidth = m_text.GetEffectiveTextPenWidth() / 2.0;
        double radius = ( textBox.GetWidth() / 2.0 ) - penWidth;
        CIRCLE circle( textBox.GetCenter(), radius );

        arrowSegEnd = segCircleIntersection( circle, arrowSeg );
        textSegEnd = segCircleIntersection( circle, textSeg );
    }
    else
    {
        arrowSegEnd = segPolyIntersection( polyBox, arrowSeg );
        textSegEnd = segPolyIntersection( polyBox, textSeg );
    }

    if( !arrowSegEnd )
        arrowSegEnd = m_end;

    m_shapes.emplace_back( new SHAPE_SEGMENT( start, *arrowSegEnd ) );

    // Add arrows
    VECTOR2I arrowEndPos( m_arrowLength, 0 );
    VECTOR2I arrowEndNeg( m_arrowLength, 0 );
    RotatePoint( arrowEndPos, -EDA_ANGLE( firstLine ) + s_arrowAngle );
    RotatePoint( arrowEndNeg, -EDA_ANGLE( firstLine ) - s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( start, start + arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( start, start + arrowEndNeg ) );


    if( !GetText().IsEmpty() )
    {
        switch( m_textBorder )
        {
        case DIM_TEXT_BORDER::RECTANGLE:
        {
            for( SHAPE_POLY_SET::SEGMENT_ITERATOR seg = polyBox.IterateSegments(); seg; seg++ )
                m_shapes.emplace_back( new SHAPE_SEGMENT( *seg ) );

            break;
        }

        case DIM_TEXT_BORDER::CIRCLE:
        {
            double penWidth = m_text.GetEffectiveTextPenWidth() / 2.0;
            double radius   = ( textBox.GetWidth() / 2.0 ) - penWidth;
            m_shapes.emplace_back( new SHAPE_CIRCLE( textBox.GetCenter(), radius ) );

            break;
        }

        default:
            break;
        }
    }

    if( textSegEnd && *arrowSegEnd == m_end )
        m_shapes.emplace_back( new SHAPE_SEGMENT( m_end, *textSegEnd ) );
}


void PCB_DIM_LEADER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString    msg;

    aList.emplace_back( _( "Leader" ), m_text.GetShownText() );

    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    EDA_UNITS         units = aFrame->GetUserUnits();

    VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
    wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                       MessageTextFromValue( units, startCoord.x ),
                                       MessageTextFromValue( units, startCoord.y ) );

    aList.emplace_back( start, wxEmptyString );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


PCB_DIM_RADIAL::PCB_DIM_RADIAL( BOARD_ITEM* aParent, bool aInFP ) :
        PCB_DIMENSION_BASE( aParent, aInFP ? PCB_FP_DIM_RADIAL_T : PCB_DIM_RADIAL_T )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = false;
    m_keepTextAligned     = true;
    m_isDiameter          = false;
    m_prefix              = "R ";
    m_leaderLength        = m_arrowLength * 3;
}


EDA_ITEM* PCB_DIM_RADIAL::Clone() const
{
    return new PCB_DIM_RADIAL( *this );
}


void PCB_DIM_RADIAL::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_RADIAL*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_RADIAL*>( this ), *static_cast<PCB_DIM_RADIAL*>( aImage ) );

    Update();
}


BITMAPS PCB_DIM_RADIAL::GetMenuImage() const
{
    return BITMAPS::add_radial_dimension;
}


VECTOR2I PCB_DIM_RADIAL::GetKnee() const
{
    VECTOR2I radial( m_end - m_start );

    return m_end + radial.Resize( m_leaderLength );
}


void PCB_DIM_RADIAL::updateText()
{
    if( m_keepTextAligned )
    {
        VECTOR2I  textLine( Text().GetPosition() - GetKnee() );
        EDA_ANGLE textAngle = FULL_CIRCLE - EDA_ANGLE( textLine );

        textAngle.Normalize();

        if( textAngle > ANGLE_90 && textAngle <= ANGLE_270 )
            textAngle -= ANGLE_180;

        // Round to nearest degree
        textAngle = EDA_ANGLE( KiROUND( textAngle.AsDegrees() ), DEGREES_T );

        m_text.SetTextAngle( textAngle );
    }

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_RADIAL::updateGeometry()
{
    m_shapes.clear();

    VECTOR2I center( m_start );
    VECTOR2I centerArm( 0, m_arrowLength );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - centerArm, center + centerArm ) );

    RotatePoint( centerArm, -ANGLE_90 );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - centerArm, center + centerArm ) );

    VECTOR2I radius( m_end - m_start );

    if( m_isDiameter )
        m_measuredValue = KiROUND( radius.EuclideanNorm() * 2 );
    else
        m_measuredValue = KiROUND( radius.EuclideanNorm() );

    updateText();

    // Now that we have the text updated, we can determine how to draw the second line
    // First we need to create an appropriate bounding polygon to collide with
    EDA_RECT textBox = m_text.GetTextBox().Inflate( m_text.GetTextWidth() / 2,
                                                    m_text.GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( m_text.GetTextAngle(), textBox.GetCenter() );

    VECTOR2I radial( m_end - m_start );
    radial = radial.Resize( m_leaderLength );

    SEG arrowSeg( m_end, m_end + radial );
    SEG textSeg( arrowSeg.B, m_text.GetPosition() );

    OPT_VECTOR2I arrowSegEnd = segPolyIntersection( polyBox, arrowSeg );
    OPT_VECTOR2I textSegEnd = segPolyIntersection( polyBox, textSeg );

    if( arrowSegEnd )
        arrowSeg.B = arrowSegEnd.get();

    if( textSegEnd )
        textSeg.B = textSegEnd.get();

    m_shapes.emplace_back( new SHAPE_SEGMENT( arrowSeg ) );

    // Add arrows
    VECTOR2I arrowEndPos( m_arrowLength, 0 );
    VECTOR2I arrowEndNeg( m_arrowLength, 0 );
    RotatePoint( arrowEndPos, -EDA_ANGLE( radial ) + s_arrowAngle );
    RotatePoint( arrowEndNeg, -EDA_ANGLE( radial ) - s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_end, m_end + arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( m_end, m_end + arrowEndNeg ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( textSeg ) );
}


PCB_DIM_CENTER::PCB_DIM_CENTER( BOARD_ITEM* aParent, bool aInFP ) :
        PCB_DIMENSION_BASE( aParent, aInFP ? PCB_FP_DIM_CENTER_T : PCB_DIM_CENTER_T )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
}


EDA_ITEM* PCB_DIM_CENTER::Clone() const
{
    return new PCB_DIM_CENTER( *this );
}


void PCB_DIM_CENTER::SwapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    std::swap( *static_cast<PCB_DIM_CENTER*>( this ), *static_cast<PCB_DIM_CENTER*>( aImage ) );
}


BITMAPS PCB_DIM_CENTER::GetMenuImage() const
{
    return BITMAPS::add_center_dimension;
}


const EDA_RECT PCB_DIM_CENTER::GetBoundingBox() const
{
    int halfWidth = VECTOR2I( m_end - m_start ).x + ( m_lineThickness / 2.0 );

    EDA_RECT bBox;

    bBox.SetX( m_start.x - halfWidth );
    bBox.SetY( m_start.y - halfWidth );
    bBox.SetWidth( halfWidth * 2 );
    bBox.SetHeight( halfWidth * 2 );

    bBox.Normalize();

    return bBox;
}


const BOX2I PCB_DIM_CENTER::ViewBBox() const
{
    return BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                  VECTOR2I( GetBoundingBox().GetSize() ) );
}


void PCB_DIM_CENTER::updateGeometry()
{
    m_shapes.clear();

    VECTOR2I center( m_start );
    VECTOR2I arm( m_end - m_start );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );

    RotatePoint( arm, -ANGLE_90 );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );
}


static struct DIMENSION_DESC
{
    DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIMENSION_BASE );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIMENSION_BASE ), TYPE_HASH( BOARD_ITEM ) );
        // TODO: add dimension properties:
        //propMgr.AddProperty( new PROPERTY<DIMENSION, int>( _HKI( "Height" ),
                    //&DIMENSION::SetHeight, &DIMENSION::GetHeight, PROPERTY_DISPLAY::DISTANCE ) );
    }
} _DIMENSION_DESC;



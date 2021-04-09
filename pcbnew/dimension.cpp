/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <board.h>
#include <dimension.h>
#include <pcb_text.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <i18n_utility.h>


DIMENSION_BASE::DIMENSION_BASE( BOARD_ITEM* aParent, KICAD_T aType ) :
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


void DIMENSION_BASE::SetParent( EDA_ITEM* aParent )
{
    BOARD_ITEM::SetParent( aParent );
    m_text.SetParent( aParent );
}


void DIMENSION_BASE::updateText()
{
    wxString text = m_overrideTextEnabled ? m_valueString : GetValueText();

    switch( m_unitsFormat )
    {
    case DIM_UNITS_FORMAT::NO_SUFFIX: // no units
        break;

    case DIM_UNITS_FORMAT::BARE_SUFFIX: // normal
        text += " ";
        text += GetAbbreviatedUnitsLabel( m_units );
        break;

    case DIM_UNITS_FORMAT::PAREN_SUFFIX: // parenthetical
        text += " (";
        text += GetAbbreviatedUnitsLabel( m_units );
        text += ")";
        break;
    }

    text.Prepend( m_prefix );
    text.Append( m_suffix );

    m_text.SetText( text );
}


template<typename ShapeType>
void DIMENSION_BASE::addShape( const ShapeType& aShape )
{
    m_shapes.push_back( std::make_shared<ShapeType>( aShape ) );
}


wxString DIMENSION_BASE::GetValueText() const
{
    struct lconv* lc = localeconv();
    wxChar sep = lc->decimal_point[0];

    int      val = GetMeasuredValue();
    wxString text;
    wxString format = wxT( "%." ) + wxString::Format( "%i", m_precision ) + wxT( "f" );

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


void DIMENSION_BASE::SetPrefix( const wxString& aPrefix )
{
    m_prefix = aPrefix;
}


void DIMENSION_BASE::SetSuffix( const wxString& aSuffix )
{
    m_suffix = aSuffix;
}


void DIMENSION_BASE::SetUnits( EDA_UNITS aUnits )
{
    m_units = aUnits;
}


DIM_UNITS_MODE DIMENSION_BASE::GetUnitsMode() const
{
    if( m_autoUnits )
    {
        return DIM_UNITS_MODE::AUTOMATIC;
    }
    else
    {
        switch( m_units )
        {
        case EDA_UNITS::MILLIMETRES:
            return DIM_UNITS_MODE::MILLIMETRES;

        case EDA_UNITS::MILS:
            return DIM_UNITS_MODE::MILS;

        default:
        case EDA_UNITS::INCHES:
            return DIM_UNITS_MODE::INCHES;
        }
    }
}


void DIMENSION_BASE::SetUnitsMode( DIM_UNITS_MODE aMode )
{
    m_autoUnits = false;

    switch( aMode )
    {
    case DIM_UNITS_MODE::INCHES:
        m_units = EDA_UNITS::INCHES;
        break;

    case DIM_UNITS_MODE::MILS:
        m_units = EDA_UNITS::MILS;
        break;

    case DIM_UNITS_MODE::MILLIMETRES:
        m_units = EDA_UNITS::MILLIMETRES;
        break;

    case DIM_UNITS_MODE::AUTOMATIC:
        m_autoUnits = true;
        break;
    }
}


void DIMENSION_BASE::SetText( const wxString& aNewText )
{
    m_valueString = aNewText;
    updateText();
}


const wxString DIMENSION_BASE::GetText() const
{
    return m_text.GetText();
}


void DIMENSION_BASE::SetLayer( PCB_LAYER_ID aLayer )
{
    m_layer = aLayer;
    m_text.SetLayer( aLayer );
}


void DIMENSION_BASE::Move( const wxPoint& offset )
{
    m_text.Offset( offset );

    m_start += offset;
    m_end   += offset;

    Update();
}


void DIMENSION_BASE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    double newAngle = m_text.GetTextAngle() + aAngle;

    if( newAngle >= 3600 )
        newAngle -= 3600;

    m_text.SetTextAngle( newAngle );

    wxPoint pt = m_text.GetTextPos();
    RotatePoint( &pt, aRotCentre, aAngle );
    m_text.SetTextPos( pt );

    RotatePoint( &m_start, aRotCentre, aAngle );
    RotatePoint( &m_end, aRotCentre, aAngle );

    Update();
}


void DIMENSION_BASE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    Mirror( aCentre );

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}


void DIMENSION_BASE::Mirror( const wxPoint& axis_pos, bool aMirrorLeftRight )
{
    int axis = aMirrorLeftRight ? axis_pos.x : axis_pos.y;
    wxPoint newPos = m_text.GetTextPos();

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


void DIMENSION_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // for now, display only the text within the DIMENSION using class PCB_TEXT.
    wxString    msg;

    wxCHECK_RET( m_parent != NULL, wxT( "PCB_TEXT::GetMsgPanelInfo() m_Parent is NULL." ) );

    aList.emplace_back( _( "Dimension" ), m_text.GetShownText() );

    aList.emplace_back( _( "Prefix" ), GetPrefix() );

    if( GetOverrideTextEnabled() )
    {
        aList.emplace_back( _( "Override Text" ), GetOverrideText() );
    }
    else
    {
        aList.emplace_back( _( "Value" ), GetValueText() );

        msg = "%" + wxString::Format( "1.%df", GetPrecision() );
        aList.emplace_back( _( "Precision" ), wxString::Format( msg, 0.0 ) );
    }

    aList.emplace_back( _( "Suffix" ), GetSuffix() );

    EDA_UNITS units;

    GetUnits( units );
    aList.emplace_back( _( "Units" ), GetAbbreviatedUnitsLabel( units ) );

    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    units = aFrame->GetUserUnits();

    if( Type() == PCB_DIM_CENTER_T )
    {
        wxPoint startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( "@(%s, %s)",
                                           MessageTextFromValue( units, startCoord.x ),
                                           MessageTextFromValue( units, startCoord.y ) );

        aList.emplace_back( start, wxEmptyString );
    }
    else
    {
        wxPoint startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( "@(%s, %s)",
                                           MessageTextFromValue( units, startCoord.x ),
                                           MessageTextFromValue( units, startCoord.y ) );
        wxPoint endCoord = originTransforms.ToDisplayAbs( GetEnd() );
        wxString end   = wxString::Format( "@(%s, %s)",
                                           MessageTextFromValue( units, endCoord.x ),
                                           MessageTextFromValue( units, endCoord.y ) );

        aList.emplace_back( start, end );
    }

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


std::shared_ptr<SHAPE> DIMENSION_BASE::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    std::shared_ptr<SHAPE_COMPOUND> effectiveShape = std::make_shared<SHAPE_COMPOUND>();

    effectiveShape->AddShape( Text().GetEffectiveTextShape()->Clone() );

    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
        effectiveShape->AddShape( shape->Clone() );

    return effectiveShape;
}


bool DIMENSION_BASE::HitTest( const wxPoint& aPosition, int aAccuracy ) const
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


bool DIMENSION_BASE::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
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


const EDA_RECT DIMENSION_BASE::GetBoundingBox() const
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


wxString DIMENSION_BASE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "Dimension '%s' on %s" ),
                             GetText(),
                             GetLayerName() );
}



const BOX2I DIMENSION_BASE::ViewBBox() const
{
    BOX2I dimBBox = BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                           VECTOR2I( GetBoundingBox().GetSize() ) );
    dimBBox.Merge( m_text.ViewBBox() );

    return dimBBox;
}


OPT_VECTOR2I DIMENSION_BASE::segPolyIntersection( const SHAPE_POLY_SET& aPoly, const SEG& aSeg, bool aStart )
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


OPT_VECTOR2I DIMENSION_BASE::segCircleIntersection( CIRCLE& aCircle, SEG& aSeg, bool aStart )
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


ALIGNED_DIMENSION::ALIGNED_DIMENSION( BOARD_ITEM* aParent, KICAD_T aType ) :
        DIMENSION_BASE( aParent, aType ),
        m_height( 0 )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = static_cast<int>( m_arrowLength * std::sin( DEG2RAD( s_arrowAngle ) ) );
}


EDA_ITEM* ALIGNED_DIMENSION::Clone() const
{
    return new ALIGNED_DIMENSION( *this );
}


void ALIGNED_DIMENSION::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIM_ALIGNED_T );

    m_shapes.clear();
    static_cast<ALIGNED_DIMENSION*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<ALIGNED_DIMENSION*>( this ),
               *static_cast<ALIGNED_DIMENSION*>( aImage ) );

    Update();
}

BITMAPS ALIGNED_DIMENSION::GetMenuImage() const
{
    return BITMAPS::add_aligned_dimension;
}


void ALIGNED_DIMENSION::UpdateHeight( const wxPoint& aCrossbarStart, const wxPoint& aCrossbarEnd )
{
    VECTOR2D height( aCrossbarStart - GetStart() );
    VECTOR2D crossBar( aCrossbarEnd - aCrossbarStart );

    if( height.Cross( crossBar ) > 0 )
        m_height = -height.EuclideanNorm();
    else
        m_height = height.EuclideanNorm();

    Update();
}


void ALIGNED_DIMENSION::updateGeometry()
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
    m_crossBarStart = m_start + wxPoint( crossBarDistance );
    m_crossBarEnd   = m_end + wxPoint( crossBarDistance );

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
    polyBox.Rotate( -m_text.GetTextAngleRadians(), textBox.GetCenter() );

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
    VECTOR2I arrowEnd( m_arrowLength, 0 );

    double arrowRotPos = dimension.Angle() + DEG2RAD( s_arrowAngle );
    double arrowRotNeg = dimension.Angle() - DEG2RAD( s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart,
                           m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart,
                           m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd,
                           m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd,
                           m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );
}


void ALIGNED_DIMENSION::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = m_text.GetEffectiveTextPenWidth() + m_text.GetTextHeight();

        double rotation;
        if( crossbarCenter.x == 0 )
            rotation = sign( crossbarCenter.y ) * DEG2RAD( 90 );
        else
            rotation = -std::copysign( DEG2RAD( 90 ), crossbarCenter.x );

        VECTOR2I textOffset = crossbarCenter.Rotate( rotation ).Resize( textOffsetDistance );
        textOffset += crossbarCenter;

        m_text.SetTextPos( m_crossBarStart + wxPoint( textOffset ) );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        m_text.SetTextPos( m_crossBarStart + wxPoint( crossbarCenter ) );
    }

    if( m_keepTextAligned )
    {
        double textAngle = 3600 - RAD2DECIDEG( crossbarCenter.Angle() );

        NORMALIZE_ANGLE_POS( textAngle );

        if( textAngle > 900 && textAngle <= 2700 )
            textAngle -= 1800;

        m_text.SetTextAngle( textAngle );
    }

    DIMENSION_BASE::updateText();
}


void ALIGNED_DIMENSION::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                         std::vector<MSG_PANEL_ITEM>& aList )
{
    DIMENSION_BASE::GetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Height" ), MessageTextFromValue( aFrame->GetUserUnits(), m_height ) );
}


ORTHOGONAL_DIMENSION::ORTHOGONAL_DIMENSION( BOARD_ITEM* aParent ) :
        ALIGNED_DIMENSION( aParent, PCB_DIM_ORTHOGONAL_T )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = static_cast<int>( m_arrowLength * std::sin( DEG2RAD( s_arrowAngle ) ) );
    m_orientation = DIR::HORIZONTAL;
}


EDA_ITEM* ORTHOGONAL_DIMENSION::Clone() const
{
    return new ORTHOGONAL_DIMENSION( *this );
}


void ORTHOGONAL_DIMENSION::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIM_ORTHOGONAL_T );

    m_shapes.clear();
    static_cast<ORTHOGONAL_DIMENSION*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<ORTHOGONAL_DIMENSION*>( this ),
               *static_cast<ORTHOGONAL_DIMENSION*>( aImage ) );

    Update();
}


BITMAPS ORTHOGONAL_DIMENSION::GetMenuImage() const
{
    return BITMAPS::add_orthogonal_dimension;
}


void ORTHOGONAL_DIMENSION::updateGeometry()
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
    m_crossBarStart = m_start + wxPoint( crossBarDistance );

    if( m_orientation == DIR::HORIZONTAL )
        m_crossBarEnd = wxPoint( m_end.x, m_crossBarStart.y );
    else
        m_crossBarEnd = wxPoint( m_crossBarStart.x, m_end.y );

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
    polyBox.Rotate( -m_text.GetTextAngleRadians(), textBox.GetCenter() );

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
    VECTOR2I crossBarAngle( m_crossBarEnd - m_crossBarStart );
    VECTOR2I arrowEnd( m_arrowLength, 0 );

    double arrowRotPos = crossBarAngle.Angle() + DEG2RAD( s_arrowAngle );
    double arrowRotNeg = crossBarAngle.Angle() - DEG2RAD( s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart,
                                              m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarStart,
                                              m_crossBarStart + wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd,
                                              m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( m_crossBarEnd,
                                              m_crossBarEnd - wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );
}


void ORTHOGONAL_DIMENSION::updateText()
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

        m_text.SetTextPos( m_crossBarStart + wxPoint( textOffset ) );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        m_text.SetTextPos( m_crossBarStart + wxPoint( crossbarCenter ) );
    }

    if( m_keepTextAligned )
    {
        double textAngle;
        if( abs( crossbarCenter.x ) > abs( crossbarCenter.y ) )
            textAngle = 0;
        else
            textAngle = 900;

        m_text.SetTextAngle( textAngle );
    }

    DIMENSION_BASE::updateText();
}


void ORTHOGONAL_DIMENSION::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    // restrict angle to -179.9 to 180.0 degrees
    if( aAngle > 1800 )
    {
        aAngle -= 3600;
    }
    else if( aAngle <= -1800 )
    {
        aAngle += 3600;
    }

    // adjust orientation and height to new angle
    // we can only handle the cases of -90, 0, 90, 180 degrees exactly;
    // in the other cases we will use the nearest 90 degree angle to
    // choose at least an approximate axis for the target orientation
    // In case of exactly 45 or 135 degrees, we will round towards zero for consistency
    if( aAngle > 450 && aAngle <= 1350 )
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
    else if( aAngle < -450 && aAngle >= -1350 )
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
    else if( aAngle > 1350 || aAngle < -1350 )
    {
        // about 180 degree
        m_height = -m_height;
    }

    // this will update m_crossBarStart and m_crossbarEnd
    DIMENSION_BASE::Rotate( aRotCentre, aAngle );
}


LEADER::LEADER( BOARD_ITEM* aParent ) :
        DIMENSION_BASE( aParent, PCB_DIM_LEADER_T ),
        m_textFrame( DIM_TEXT_FRAME::NONE )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
    m_keepTextAligned     = false;
}


EDA_ITEM* LEADER::Clone() const
{
    return new LEADER( *this );
}


void LEADER::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIM_LEADER_T );

    std::swap( *static_cast<LEADER*>( this ), *static_cast<LEADER*>( aImage ) );
}


BITMAPS LEADER::GetMenuImage() const
{
    return BITMAPS::add_leader;
}


void LEADER::updateGeometry()
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
    polyBox.Rotate( -m_text.GetTextAngleRadians(), textBox.GetCenter() );

    VECTOR2I firstLine( m_end - m_start );
    VECTOR2I start( m_start );
    start += firstLine.Resize( m_extensionOffset );

    SEG arrowSeg( m_start, m_end );
    SEG textSeg( m_end, m_text.GetPosition() );
    OPT_VECTOR2I arrowSegEnd, textSegEnd;

    if( m_textFrame == DIM_TEXT_FRAME::CIRCLE )
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
    VECTOR2I arrowEnd( m_arrowLength, 0 );

    double arrowRotPos = firstLine.Angle() + DEG2RAD( s_arrowAngle );
    double arrowRotNeg = firstLine.Angle() - DEG2RAD( s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( start,
                                              start + wxPoint( arrowEnd.Rotate( arrowRotPos ) ) ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( start,
                                              start + wxPoint( arrowEnd.Rotate( arrowRotNeg ) ) ) );


    if( !GetText().IsEmpty() )
    {
        switch( m_textFrame )
        {
        case DIM_TEXT_FRAME::RECTANGLE:
        {
            for( SHAPE_POLY_SET::SEGMENT_ITERATOR seg = polyBox.IterateSegments(); seg; seg++ )
                m_shapes.emplace_back( new SHAPE_SEGMENT( *seg ) );

            break;
        }

        case DIM_TEXT_FRAME::CIRCLE:
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


void LEADER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString    msg;

    aList.emplace_back( _( "Leader" ), m_text.GetShownText() );

    ORIGIN_TRANSFORMS originTransforms = aFrame->GetOriginTransforms();
    EDA_UNITS         units = aFrame->GetUserUnits();

    wxPoint startCoord = originTransforms.ToDisplayAbs( GetStart() );
    wxString start = wxString::Format( "@(%s, %s)",
                                       MessageTextFromValue( units, startCoord.x ),
                                       MessageTextFromValue( units, startCoord.y ) );

    aList.emplace_back( start, wxEmptyString );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


CENTER_DIMENSION::CENTER_DIMENSION( BOARD_ITEM* aParent ) :
        DIMENSION_BASE( aParent, PCB_DIM_CENTER_T )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
}


EDA_ITEM* CENTER_DIMENSION::Clone() const
{
    return new CENTER_DIMENSION( *this );
}


void CENTER_DIMENSION::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_DIM_CENTER_T );

    std::swap( *static_cast<CENTER_DIMENSION*>( this ), *static_cast<CENTER_DIMENSION*>( aImage ) );
}


BITMAPS CENTER_DIMENSION::GetMenuImage() const
{
    return BITMAPS::add_center_dimension;
}


const EDA_RECT CENTER_DIMENSION::GetBoundingBox() const
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


const BOX2I CENTER_DIMENSION::ViewBBox() const
{
    return BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                  VECTOR2I( GetBoundingBox().GetSize() ) );
}


void CENTER_DIMENSION::updateGeometry()
{
    m_shapes.clear();

    VECTOR2I center( m_start );
    VECTOR2I arm( m_end - m_start );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );

    arm = arm.Rotate( DEG2RAD( 90 ) );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );
}


static struct DIMENSION_DESC
{
    DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( DIMENSION_BASE );
        propMgr.InheritsAfter( TYPE_HASH( DIMENSION_BASE ), TYPE_HASH( BOARD_ITEM ) );
        // TODO: add dimension properties:
        //propMgr.AddProperty( new PROPERTY<DIMENSION, int>( _HKI( "Height" ),
                    //&DIMENSION::SetHeight, &DIMENSION::GetHeight, PROPERTY_DISPLAY::DISTANCE ) );
    }
} _DIMENSION_DESC;



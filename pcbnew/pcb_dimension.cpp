/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2023 CERN
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

#include <bitmaps.h>
#include <pcb_edit_frame.h>
#include <base_units.h>
#include <convert_basic_shapes_to_polygon.h>
#include <font/font.h>
#include <board.h>
#include <pcb_dimension.h>
#include <pcb_text.h>
#include <board_design_settings.h>
#include <geometry/shape_compound.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_segment.h>
#include <geometry/shape_rect.h>
#include <geometry/geometry_utils.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>


static const int INWARD_ARROW_LENGTH_TO_HEAD_RATIO = 2;

static const EDA_ANGLE s_arrowAngle( 27.5, DEGREES_T );


/**
 * Find the intersection between a given segment and polygon outline.
 *
 * @param aPoly is the polygon to collide.
 * @param aSeg is the segment to collide.
 * @param aStart if true will start from aSeg.A, otherwise aSeg.B.
 * @return a point on aSeg that collides with aPoly closest to the start, if one exists.
 */
static OPT_VECTOR2I segPolyIntersection( const SHAPE_POLY_SET& aPoly, const SEG& aSeg,
                                         bool aStart = true )
{
    VECTOR2I start( aStart ? aSeg.A : aSeg.B );
    VECTOR2I endpoint( aStart ? aSeg.B : aSeg.A );

    if( aPoly.Contains( start ) )
        return std::nullopt;

    for( SHAPE_POLY_SET::CONST_SEGMENT_ITERATOR seg = aPoly.CIterateSegments(); seg; ++seg )
    {
        if( OPT_VECTOR2I intersection = ( *seg ).Intersect( aSeg ) )
        {
            if( ( *intersection - start ).SquaredEuclideanNorm()
                < ( endpoint - start ).SquaredEuclideanNorm() )
                endpoint = *intersection;
        }
    }

    if( start == endpoint )
        return std::nullopt;

    return OPT_VECTOR2I( endpoint );
}


static OPT_VECTOR2I segCircleIntersection( CIRCLE& aCircle, SEG& aSeg, bool aStart = true )
{
    VECTOR2I start( aStart ? aSeg.A : aSeg.B );
    VECTOR2I endpoint( aStart ? aSeg.B : aSeg.A );

    if( aCircle.Contains( start ) )
        return std::nullopt;

    std::vector<VECTOR2I> intersections = aCircle.Intersect( aSeg );

    for( VECTOR2I& intersection : aCircle.Intersect( aSeg ) )
    {
        if( ( intersection - start ).SquaredEuclideanNorm()
            < ( endpoint - start ).SquaredEuclideanNorm() )
            endpoint = intersection;
    }

    if( start == endpoint )
        return std::nullopt;

    return OPT_VECTOR2I( endpoint );
}


/**
 * Knockout a polygon from a segment. This function will add 0, 1 or 2 segments to the
 * vector, depending on how the polygon intersects the segment.
 */
static void CollectKnockedOutSegments( const SHAPE_POLY_SET& aPoly, const SEG& aSeg,
                                       std::vector<std::shared_ptr<SHAPE>>& aSegmentsAfterKnockout )
{
    // Now we can draw 0, 1, or 2 crossbar lines depending on how the polygon collides
    const bool containsA = aPoly.Contains( aSeg.A );
    const bool containsB = aPoly.Contains( aSeg.B );

    const OPT_VECTOR2I endpointA = segPolyIntersection( aPoly, aSeg );
    const OPT_VECTOR2I endpointB = segPolyIntersection( aPoly, aSeg, false );

    if( endpointA )
        aSegmentsAfterKnockout.emplace_back( new SHAPE_SEGMENT( aSeg.A, *endpointA ) );

    if( endpointB )
    {
        bool can_add = true;

        if( endpointA )
        {
            if( ( *endpointB == aSeg.A && *endpointA == aSeg.B )
                || ( *endpointA == *endpointB && aSeg.A == aSeg.B ) )
                can_add = false;
        }

        if( can_add )
            aSegmentsAfterKnockout.emplace_back( new SHAPE_SEGMENT( *endpointB, aSeg.B ) );
    }

    if( !containsA && !containsB && !endpointA && !endpointB )
        aSegmentsAfterKnockout.emplace_back( new SHAPE_SEGMENT( aSeg ) );
}


PCB_DIMENSION_BASE::PCB_DIMENSION_BASE( BOARD_ITEM* aParent, KICAD_T aType ) :
        PCB_TEXT( aParent, aType ),
        m_overrideTextEnabled( false ),
        m_units( EDA_UNITS::INCH ),
        m_autoUnits( false ),
        m_unitsFormat( DIM_UNITS_FORMAT::BARE_SUFFIX ),
        m_arrowDirection( DIM_ARROW_DIRECTION::OUTWARD ),
        m_precision( DIM_PRECISION::X_XXXX ),
        m_suppressZeroes( false ),
        m_lineThickness( pcbIUScale.mmToIU( 0.2 ) ),
        m_arrowLength( pcbIUScale.MilsToIU( 50 ) ),
        m_extensionOffset( 0 ),
        m_textPosition( DIM_TEXT_POSITION::OUTSIDE ),
        m_keepTextAligned( true ),
        m_measuredValue( 0 ),
        m_inClearRenderCache( false )
{
    m_layer = Dwgs_User;
    m_busy = false;
}


bool PCB_DIMENSION_BASE::operator==( const BOARD_ITEM& aOther ) const
{
    if( Type() != aOther.Type() )
        return false;

    const PCB_DIMENSION_BASE& other = static_cast<const PCB_DIMENSION_BASE&>( aOther );

    return *this == other;
}


bool PCB_DIMENSION_BASE::operator==( const PCB_DIMENSION_BASE& aOther ) const
{
    if( m_textPosition != aOther.m_textPosition )
        return false;

    if( m_keepTextAligned != aOther.m_keepTextAligned )
        return false;

    if( m_units != aOther.m_units )
        return false;

    if( m_autoUnits != aOther.m_autoUnits )
        return false;

    if( m_unitsFormat != aOther.m_unitsFormat )
        return false;

    if( m_precision != aOther.m_precision )
        return false;

    if( m_suppressZeroes != aOther.m_suppressZeroes )
        return false;

    if( m_lineThickness != aOther.m_lineThickness )
        return false;

    if( m_arrowLength != aOther.m_arrowLength )
        return false;

    if( m_extensionOffset != aOther.m_extensionOffset )
        return false;

    if( m_measuredValue != aOther.m_measuredValue )
        return false;

    return EDA_TEXT::operator==( aOther );
}


double PCB_DIMENSION_BASE::Similarity( const BOARD_ITEM& aOther ) const
{
    if( m_Uuid == aOther.m_Uuid )
        return 1.0;

    if( Type() != aOther.Type() )
        return 0.0;

    const PCB_DIMENSION_BASE& other = static_cast<const PCB_DIMENSION_BASE&>( aOther );

    double similarity = 1.0;

    if( m_textPosition != other.m_textPosition )
        similarity *= 0.9;

    if( m_keepTextAligned != other.m_keepTextAligned )
        similarity *= 0.9;

    if( m_units != other.m_units )
        similarity *= 0.9;

    if( m_autoUnits != other.m_autoUnits )
        similarity *= 0.9;

    if( m_unitsFormat != other.m_unitsFormat )
        similarity *= 0.9;

    if( m_precision != other.m_precision )
        similarity *= 0.9;

    if( m_suppressZeroes != other.m_suppressZeroes )
        similarity *= 0.9;

    if( m_lineThickness != other.m_lineThickness )
        similarity *= 0.9;

    if( m_arrowLength != other.m_arrowLength )
        similarity *= 0.9;

    if( m_extensionOffset != other.m_extensionOffset )
        similarity *= 0.9;

    if( m_measuredValue != other.m_measuredValue )
        similarity *= 0.9;

    similarity *= EDA_TEXT::Similarity( other );

    return similarity;
}


void PCB_DIMENSION_BASE::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    using namespace kiapi::board::types;
    Dimension dimension;

    dimension.mutable_id()->set_value( m_Uuid.AsStdString() );
    dimension.set_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( GetLayer() ) );
    dimension.set_locked( IsLocked() ? types::LockedState::LS_LOCKED
                                     : types::LockedState::LS_UNLOCKED );

    google::protobuf::Any any;
    EDA_TEXT::Serialize( any );
    any.UnpackTo( dimension.mutable_text() );

    types::Text* text = dimension.mutable_text();
    text->set_text( GetValueText() );

    dimension.set_override_text_enabled( m_overrideTextEnabled );
    dimension.set_override_text( m_valueString.ToUTF8() );
    dimension.set_prefix( m_prefix.ToUTF8() );
    dimension.set_suffix( m_suffix.ToUTF8() );

    dimension.set_unit( ToProtoEnum<DIM_UNITS_MODE, DimensionUnit>( GetUnitsMode() ) );
    dimension.set_unit_format(
            ToProtoEnum<DIM_UNITS_FORMAT, DimensionUnitFormat>( m_unitsFormat ) );
    dimension.set_arrow_direction(
            ToProtoEnum<DIM_ARROW_DIRECTION, DimensionArrowDirection>( m_arrowDirection ) );
    dimension.set_precision( ToProtoEnum<DIM_PRECISION, DimensionPrecision>( m_precision ) );
    dimension.set_suppress_trailing_zeroes( m_suppressZeroes );

    dimension.mutable_line_thickness()->set_value_nm( m_lineThickness );
    dimension.mutable_arrow_length()->set_value_nm( m_arrowLength );
    dimension.mutable_extension_offset()->set_value_nm( m_extensionOffset );
    dimension.set_text_position(
            ToProtoEnum<DIM_TEXT_POSITION, DimensionTextPosition>( m_textPosition ) );
    dimension.set_keep_text_aligned( m_keepTextAligned );

    aContainer.PackFrom( dimension );
}


bool PCB_DIMENSION_BASE::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    if( !aContainer.UnpackTo( &dimension ) )
        return false;

    SetLayer( FromProtoEnum<PCB_LAYER_ID, kiapi::board::types::BoardLayer>( dimension.layer() ) );
    const_cast<KIID&>( m_Uuid ) = KIID( dimension.id().value() );
    SetLocked( dimension.locked() == types::LockedState::LS_LOCKED );

    google::protobuf::Any any;
    any.PackFrom( dimension.text() );
    EDA_TEXT::Deserialize( any );

    SetOverrideTextEnabled( dimension.override_text_enabled() );
    SetOverrideText( wxString::FromUTF8( dimension.override_text() ) );
    SetPrefix( wxString::FromUTF8( dimension.prefix() ) );
    SetSuffix( wxString::FromUTF8( dimension.suffix() ) );

    SetUnitsMode( FromProtoEnum<DIM_UNITS_MODE>( dimension.unit() ) );
    SetUnitsFormat( FromProtoEnum<DIM_UNITS_FORMAT>( dimension.unit_format() ) );
    SetArrowDirection( FromProtoEnum<DIM_ARROW_DIRECTION>( dimension.arrow_direction() ) );
    SetPrecision( FromProtoEnum<DIM_PRECISION>( dimension.precision() ) );
    SetSuppressZeroes( dimension.suppress_trailing_zeroes() );

    SetLineThickness( dimension.line_thickness().value_nm() );
    SetArrowLength( dimension.arrow_length().value_nm() );
    SetExtensionOffset( dimension.extension_offset().value_nm() );
    SetTextPositionMode( FromProtoEnum<DIM_TEXT_POSITION>( dimension.text_position() ) );
    SetKeepTextAligned( dimension.keep_text_aligned() );

    Update();

    return true;
}


void PCB_DIMENSION_BASE::drawAnArrow( VECTOR2I startPoint, EDA_ANGLE anAngle, int aLength )
{
    if( aLength )
    {
        VECTOR2I tailEnd( aLength, 0 );
        RotatePoint( tailEnd, -anAngle );
        m_shapes.emplace_back( new SHAPE_SEGMENT( startPoint, startPoint + tailEnd ) );
    }

    VECTOR2I arrowEndPos( m_arrowLength, 0 );
    VECTOR2I arrowEndNeg( m_arrowLength, 0 );

    RotatePoint( arrowEndPos, -anAngle + s_arrowAngle );
    RotatePoint( arrowEndNeg, -anAngle - s_arrowAngle );

    m_shapes.emplace_back( new SHAPE_SEGMENT( startPoint, startPoint + arrowEndPos ) );
    m_shapes.emplace_back( new SHAPE_SEGMENT( startPoint, startPoint + arrowEndNeg ) );
}


void PCB_DIMENSION_BASE::updateText()
{
    wxString text = m_overrideTextEnabled ? m_valueString : GetValueText();

    switch( m_unitsFormat )
    {
    case DIM_UNITS_FORMAT::NO_SUFFIX: // no units
        break;

    case DIM_UNITS_FORMAT::BARE_SUFFIX: // normal
        text += EDA_UNIT_UTILS::GetText( m_units );
        break;

    case DIM_UNITS_FORMAT::PAREN_SUFFIX: // parenthetical
        text += wxT( " (" ) + EDA_UNIT_UTILS::GetText( m_units ).Trim( false ) + wxT( ")" );
        break;
    }

    text.Prepend( m_prefix );
    text.Append( m_suffix );

    SetText( text );
}


void PCB_DIMENSION_BASE::ClearRenderCache()
{
    PCB_TEXT::ClearRenderCache();

    // We use EDA_TEXT::ClearRenderCache() as a signal that the properties of the EDA_TEXT
    // have changed and we may need to update the dimension text

    if( !m_inClearRenderCache )
    {
        m_inClearRenderCache = true;
        Update();
        m_inClearRenderCache = false;
    }
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
    int      precision = static_cast<int>( m_precision );
    wxString text;

    if( precision >= 6 )
    {
        switch( m_units )
        {
        case EDA_UNITS::INCH: precision = precision - 4;                break;
        case EDA_UNITS::MILS: precision = std::max( 0, precision - 7 ); break;
        case EDA_UNITS::MM:   precision = precision - 5;                break;
        default:              precision = precision - 4;                break;
        }
    }

    wxString format = wxT( "%." ) + wxString::Format( wxT( "%i" ), precision ) + wxT( "f" );

    text.Printf( format, EDA_UNIT_UTILS::UI::ToUserUnit( pcbIUScale, m_units, val ) );

    if( m_suppressZeroes )
    {
        while( text.EndsWith( '0' ) )
        {
            text.RemoveLast();

            if( text.EndsWith( '.' ) || text.EndsWith( sep ) )
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
        case EDA_UNITS::INCH: return DIM_UNITS_MODE::INCH;
        case EDA_UNITS::MM:   return DIM_UNITS_MODE::MM;
        case EDA_UNITS::MILS: return DIM_UNITS_MODE::MILS;
        }
    }
}


void PCB_DIMENSION_BASE::SetUnitsMode( DIM_UNITS_MODE aMode )
{
    switch( aMode )
    {
    case DIM_UNITS_MODE::INCH:
        m_autoUnits = false;
        m_units = EDA_UNITS::INCH;
        break;

    case DIM_UNITS_MODE::MILS:
        m_autoUnits = false;
        m_units = EDA_UNITS::MILS;
        break;

    case DIM_UNITS_MODE::MM:
        m_autoUnits = false;
        m_units = EDA_UNITS::MM;
        break;

    case DIM_UNITS_MODE::AUTOMATIC:
        m_autoUnits = true;
        m_units = GetBoard() ? GetBoard()->GetUserUnits() : EDA_UNITS::MM;
        break;
    }
}


void PCB_DIMENSION_BASE::ChangeTextAngleDegrees( double aDegrees )
{
    SetTextAngleDegrees( aDegrees );
    // Create or repair any knockouts
    Update();
}


void PCB_DIMENSION_BASE::ChangeKeepTextAligned( bool aKeepAligned )
{
    SetKeepTextAligned( aKeepAligned );
    // Re-align the text and repair any knockouts
    Update();
}


void PCB_DIMENSION_BASE::Move( const VECTOR2I& offset )
{
    PCB_TEXT::Offset( offset );

    m_start += offset;
    m_end   += offset;

    Update();
}


void PCB_DIMENSION_BASE::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE newAngle = GetTextAngle() + aAngle;

    newAngle.Normalize();

    SetTextAngle( newAngle );

    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    SetTextPos( pt );

    RotatePoint( m_start, aRotCentre, aAngle );
    RotatePoint( m_end, aRotCentre, aAngle );

    Update();
}


void PCB_DIMENSION_BASE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    Mirror( aCentre, aFlipDirection );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
}


void PCB_DIMENSION_BASE::Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection )
{
    VECTOR2I newPos = GetTextPos();

    MIRROR( newPos, axis_pos, aFlipDirection );

    SetTextPos( newPos );

    // invert angle
    SetTextAngle( -GetTextAngle() );

    MIRROR( m_start, axis_pos, aFlipDirection );
    MIRROR( m_end, axis_pos, aFlipDirection );

    if( IsSideSpecific() )
        SetMirrored( !IsMirrored() );

    Update();
}


void PCB_DIMENSION_BASE::StyleFromSettings( const BOARD_DESIGN_SETTINGS& settings, bool aCheckSide )
{
    PCB_TEXT::StyleFromSettings( settings, aCheckSide );

    SetLineThickness( settings.GetLineThickness( m_layer ) );
    SetUnitsMode( settings.m_DimensionUnitsMode );
    SetUnitsFormat( settings.m_DimensionUnitsFormat );
    SetPrecision( settings.m_DimensionPrecision );
    SetSuppressZeroes( settings.m_DimensionSuppressZeroes );
    SetTextPositionMode( settings.m_DimensionTextPosition );
    SetKeepTextAligned( settings.m_DimensionKeepTextAligned );

    Update();    // refresh text & geometry

}


void PCB_DIMENSION_BASE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                          std::vector<MSG_PANEL_ITEM>& aList )
{
    // for now, display only the text within the DIMENSION using class PCB_TEXT.
    wxString    msg;

    wxCHECK_RET( m_parent != nullptr, wxT( "PCB_TEXT::GetMsgPanelInfo() m_Parent is NULL." ) );

    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( _( "Dimension" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    aList.emplace_back( _( "Prefix" ), GetPrefix() );

    if( GetOverrideTextEnabled() )
    {
        aList.emplace_back( _( "Override Text" ), GetOverrideText() );
    }
    else
    {
        aList.emplace_back( _( "Value" ), GetValueText() );

        switch( GetPrecision() )
        {
        case DIM_PRECISION::V_VV:    msg = wxT( "0.00 in / 0 mils / 0.0 mm" );          break;
        case DIM_PRECISION::V_VVV:   msg = wxT( "0.000 in / 0 mils / 0.00 mm" );        break;
        case DIM_PRECISION::V_VVVV:  msg = wxT( "0.0000 in / 0.0 mils / 0.000 mm" );    break;
        case DIM_PRECISION::V_VVVVV: msg = wxT( "0.00000 in / 0.00 mils / 0.0000 mm" ); break;
        default:  msg = wxT( "%" ) + wxString::Format( wxT( "1.%df" ), GetPrecision() );
        }

        aList.emplace_back( _( "Precision" ), wxString::Format( msg, 0.0 ) );
    }

    aList.emplace_back( _( "Suffix" ), GetSuffix() );

    // Use our own UNITS_PROVIDER to report dimension info in dimension's units rather than
    // in frame's units.
    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );
    unitsProvider.SetUserUnits( GetUnits() );

    aList.emplace_back( _( "Units" ), EDA_UNIT_UTILS::GetLabel( GetUnits() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Text Thickness" ), unitsProvider.MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), unitsProvider.MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), unitsProvider.MessageTextFromValue( GetTextHeight() ) );

    ORIGIN_TRANSFORMS& originTransforms = aFrame->GetOriginTransforms();

    if( Type() == PCB_DIM_CENTER_T )
    {
        VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                           aFrame->MessageTextFromValue( startCoord.x ),
                                           aFrame->MessageTextFromValue( startCoord.y ) );

        aList.emplace_back( start, wxEmptyString );
    }
    else
    {
        VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
        wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                           aFrame->MessageTextFromValue( startCoord.x ),
                                           aFrame->MessageTextFromValue( startCoord.y ) );
        VECTOR2I endCoord = originTransforms.ToDisplayAbs( GetEnd() );
        wxString end   = wxString::Format( wxT( "@(%s, %s)" ),
                                           aFrame->MessageTextFromValue( endCoord.x ),
                                           aFrame->MessageTextFromValue( endCoord.y ) );

        aList.emplace_back( start, end );
    }

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


std::shared_ptr<SHAPE> PCB_DIMENSION_BASE::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> effectiveShape = std::make_shared<SHAPE_COMPOUND>();

    effectiveShape->AddShape( GetEffectiveTextShape()->Clone() );

    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
        effectiveShape->AddShape( shape->Clone() );

    return effectiveShape;
}


bool PCB_DIMENSION_BASE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    if( TextHitTest( aPosition ) )
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


bool PCB_DIMENSION_BASE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    BOX2I rect = GetBoundingBox();

    if( aAccuracy )
        rect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( rect );

    return arect.Intersects( rect );
}


bool PCB_DIMENSION_BASE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    // Note: Can't use GetEffectiveShape() because we want text as BoundingBox, not as graphics.
    SHAPE_COMPOUND effShape;

    // Add shapes
    for( const std::shared_ptr<SHAPE>& shape : GetShapes() )
        effShape.AddShape( shape );

    if( aContained )
        return TextHitTest( aPoly, aContained ) && KIGEOM::ShapeHitTest( aPoly, effShape, aContained );
    else
        return TextHitTest( aPoly, aContained ) || KIGEOM::ShapeHitTest( aPoly, effShape, aContained );
}


const BOX2I PCB_DIMENSION_BASE::GetBoundingBox() const
{
    BOX2I bBox;
    int   xmin, xmax, ymin, ymax;

    bBox    = GetTextBox( nullptr );
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


wxString PCB_DIMENSION_BASE::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return wxString::Format( _( "Dimension '%s' on %s" ),
                             aFull ? GetShownText( false ) : KIUI::EllipsizeMenuText( GetText() ),
                             GetLayerName() );
}



const BOX2I PCB_DIMENSION_BASE::ViewBBox() const
{
    BOX2I dimBBox = BOX2I( VECTOR2I( GetBoundingBox().GetPosition() ),
                           VECTOR2I( GetBoundingBox().GetSize() ) );
    dimBBox.Merge( PCB_TEXT::ViewBBox() );

    return dimBBox;
}


void PCB_DIMENSION_BASE::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                                  int aClearance, int aError, ERROR_LOC aErrorLoc,
                                                  bool aIgnoreLineWidth ) const
{
    wxASSERT_MSG( !aIgnoreLineWidth, wxT( "IgnoreLineWidth has no meaning for dimensions." ) );

    for( const std::shared_ptr<SHAPE>& shape : m_shapes )
    {
        const SHAPE_CIRCLE*  circle = dynamic_cast<const SHAPE_CIRCLE*>( shape.get() );
        const SHAPE_SEGMENT* seg    = dynamic_cast<const SHAPE_SEGMENT*>( shape.get() );

        if( circle )
        {
            TransformCircleToPolygon( aBuffer, circle->GetCenter(),
                                      circle->GetRadius() + m_lineThickness / 2 + aClearance,
                                      aError, aErrorLoc );
        }
        else if( seg )
        {
            TransformOvalToPolygon( aBuffer, seg->GetSeg().A, seg->GetSeg().B,
                                    m_lineThickness + 2 * aClearance, aError, aErrorLoc );
        }
        else
        {
            wxFAIL_MSG( wxT( "PCB_DIMENSION_BASE::TransformShapeToPolygon unknown shape type." ) );
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


void PCB_DIM_ALIGNED::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    PCB_DIMENSION_BASE::Serialize( aContainer );
    aContainer.UnpackTo( &dimension );

    PackVector2( *dimension.mutable_aligned()->mutable_start(), m_start );
    PackVector2( *dimension.mutable_aligned()->mutable_end(), m_end );
    dimension.mutable_aligned()->mutable_height()->set_value_nm( m_height );
    dimension.mutable_aligned()->mutable_extension_height()->set_value_nm( m_extensionHeight );

    aContainer.PackFrom( dimension );
}


bool PCB_DIM_ALIGNED::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    if( !PCB_DIMENSION_BASE::Deserialize( aContainer ) )
        return false;

    kiapi::board::types::Dimension dimension;
    aContainer.UnpackTo( &dimension );

    if( !dimension.has_aligned() )
        return false;

    SetStart( UnpackVector2( dimension.aligned().start() ) );
    SetEnd( UnpackVector2( dimension.aligned().end() ) );
    SetHeight( dimension.aligned().height().value_nm());
    SetExtensionHeight( dimension.aligned().extension_height().value_nm() );

    Update();

    return true;
}


void PCB_DIM_ALIGNED::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_ALIGNED*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_ALIGNED*>( this ), *static_cast<PCB_DIM_ALIGNED*>( aImage ) );

    Update();
}


void PCB_DIM_ALIGNED::Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection )
{
    m_height = -m_height;
    // Call this last for the Update()
    PCB_DIMENSION_BASE::Mirror( axis_pos, aFlipDirection );
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
    if( m_busy )    // Skeep reentrance that happens sometimes after calling updateText()
        return;

    m_busy = true;

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
    BOX2I textBox = GetTextBox( nullptr ).Inflate( GetTextWidth() / 2, - GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( GetTextAngle(), textBox.GetCenter() );

    // The ideal crossbar, if the text doesn't collide
    SEG crossbar( m_crossBarStart, m_crossBarEnd );

    CollectKnockedOutSegments( polyBox, crossbar, m_shapes );

    if( m_arrowDirection == DIM_ARROW_DIRECTION::INWARD )
    {
        drawAnArrow( m_crossBarStart, EDA_ANGLE( dimension ) + EDA_ANGLE( 180 ),
                     m_arrowLength * INWARD_ARROW_LENGTH_TO_HEAD_RATIO );
        drawAnArrow( m_crossBarEnd, EDA_ANGLE( dimension ),
                     m_arrowLength * INWARD_ARROW_LENGTH_TO_HEAD_RATIO );
    }
    else
    {
        drawAnArrow( m_crossBarStart, EDA_ANGLE( dimension ), 0 );
        drawAnArrow( m_crossBarEnd, EDA_ANGLE( dimension ) + EDA_ANGLE( 180 ), 0 );
    }

    m_busy = false;
}


void PCB_DIM_ALIGNED::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = GetEffectiveTextPenWidth() + GetTextHeight();
        EDA_ANGLE rotation;

        if( crossbarCenter.x == 0 )
            rotation = ANGLE_90 * sign( -crossbarCenter.y );
        else if( crossbarCenter.x < 0 )
            rotation = -ANGLE_90;
        else
            rotation = ANGLE_90;

        VECTOR2I textOffset = crossbarCenter;
        RotatePoint( textOffset, rotation );
        textOffset = crossbarCenter + textOffset.Resize( textOffsetDistance );

        SetTextPos( m_crossBarStart + textOffset );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        SetTextPos( m_crossBarStart + crossbarCenter );
    }

    if( m_keepTextAligned )
    {
        EDA_ANGLE textAngle = FULL_CIRCLE - EDA_ANGLE( crossbarCenter );
        textAngle.Normalize();

        if( textAngle > ANGLE_90 && textAngle <= ANGLE_270 )
            textAngle -= ANGLE_180;

        SetTextAngle( textAngle );
    }

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_ALIGNED::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    PCB_DIMENSION_BASE::GetMsgPanelInfo( aFrame, aList );

    // Use our own UNITS_PROVIDER to report dimension info in dimension's units rather than
    // in frame's units.
    UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );
    unitsProvider.SetUserUnits( GetUnits() );

    aList.emplace_back( _( "Height" ), unitsProvider.MessageTextFromValue( m_height ) );
}


PCB_DIM_ORTHOGONAL::PCB_DIM_ORTHOGONAL( BOARD_ITEM* aParent ) :
        PCB_DIM_ALIGNED( aParent, PCB_DIM_ORTHOGONAL_T )
{
    // To preserve look of old dimensions, initialize extension height based on default arrow length
    m_extensionHeight = static_cast<int>( m_arrowLength * s_arrowAngle.Sin() );
    m_orientation = DIR::HORIZONTAL;
}


EDA_ITEM* PCB_DIM_ORTHOGONAL::Clone() const
{
    return new PCB_DIM_ORTHOGONAL( *this );
}


void PCB_DIM_ORTHOGONAL::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    PCB_DIMENSION_BASE::Serialize( aContainer );
    aContainer.UnpackTo( &dimension );

    PackVector2( *dimension.mutable_orthogonal()->mutable_start(), m_start );
    PackVector2( *dimension.mutable_orthogonal()->mutable_end(), m_end );
    dimension.mutable_orthogonal()->mutable_height()->set_value_nm( m_height );
    dimension.mutable_orthogonal()->mutable_extension_height()->set_value_nm( m_extensionHeight );

    dimension.mutable_orthogonal()->set_alignment( m_orientation == DIR::VERTICAL
                                                           ? types::AxisAlignment::AA_Y_AXIS
                                                           : types::AxisAlignment::AA_X_AXIS );
    aContainer.PackFrom( dimension );
}


bool PCB_DIM_ORTHOGONAL::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    if( !PCB_DIMENSION_BASE::Deserialize( aContainer ) )
        return false;

    kiapi::board::types::Dimension dimension;
    aContainer.UnpackTo( &dimension );

    if( !dimension.has_orthogonal() )
        return false;

    SetStart( UnpackVector2( dimension.orthogonal().start() ) );
    SetEnd( UnpackVector2( dimension.orthogonal().end() ) );
    SetHeight( dimension.orthogonal().height().value_nm());
    SetExtensionHeight( dimension.orthogonal().extension_height().value_nm() );
    SetOrientation( dimension.orthogonal().alignment() == types::AxisAlignment::AA_Y_AXIS
                            ? DIR::VERTICAL
                            : DIR::HORIZONTAL );

    Update();

    return true;
}


void PCB_DIM_ORTHOGONAL::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    m_shapes.clear();
    static_cast<PCB_DIM_ORTHOGONAL*>( aImage )->m_shapes.clear();

    std::swap( *static_cast<PCB_DIM_ORTHOGONAL*>( this ),
               *static_cast<PCB_DIM_ORTHOGONAL*>( aImage ) );

    Update();
}


void PCB_DIM_ORTHOGONAL::Mirror( const VECTOR2I& axis_pos, FLIP_DIRECTION aFlipDirection )
{
    // Only reverse the height if the height is aligned with the flip
    if( m_orientation == DIR::HORIZONTAL && aFlipDirection == FLIP_DIRECTION::TOP_BOTTOM )
        m_height = -m_height;
    else if( m_orientation == DIR::VERTICAL && aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        m_height = -m_height;

    // Call this last, as we need the Update()
    PCB_DIMENSION_BASE::Mirror( axis_pos, aFlipDirection );
}


BITMAPS PCB_DIM_ORTHOGONAL::GetMenuImage() const
{
    return BITMAPS::add_orthogonal_dimension;
}


void PCB_DIM_ORTHOGONAL::updateGeometry()
{
    if( m_busy )    // Skeep reentrance that happens sometimes after calling updateText()
        return;

    m_busy = true;
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
    BOX2I textBox = GetTextBox( nullptr ).Inflate( GetTextWidth() / 2, GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( GetTextAngle(), textBox.GetCenter() );

    // The ideal crossbar, if the text doesn't collide
    SEG crossbar( m_crossBarStart, m_crossBarEnd );

    CollectKnockedOutSegments( polyBox, crossbar, m_shapes );

    EDA_ANGLE crossBarAngle( m_crossBarEnd - m_crossBarStart );

    if( m_arrowDirection == DIM_ARROW_DIRECTION::INWARD )
    {
        // Arrows with fixed length.
        drawAnArrow( m_crossBarStart, crossBarAngle + EDA_ANGLE( 180 ),
                     m_arrowLength * INWARD_ARROW_LENGTH_TO_HEAD_RATIO );
        drawAnArrow( m_crossBarEnd, crossBarAngle, m_arrowLength * INWARD_ARROW_LENGTH_TO_HEAD_RATIO );
    }
    else
    {
        drawAnArrow( m_crossBarStart, crossBarAngle, 0 );
        drawAnArrow( m_crossBarEnd, crossBarAngle + EDA_ANGLE( 180 ), 0 );
    }

    m_busy = false;
}


void PCB_DIM_ORTHOGONAL::updateText()
{
    VECTOR2I crossbarCenter( ( m_crossBarEnd - m_crossBarStart ) / 2 );

    if( m_textPosition == DIM_TEXT_POSITION::OUTSIDE )
    {
        int textOffsetDistance = GetEffectiveTextPenWidth() + GetTextHeight();

        VECTOR2I textOffset;

        if( m_orientation == DIR::HORIZONTAL )
            textOffset.y = -textOffsetDistance;
        else
            textOffset.x = -textOffsetDistance;

        textOffset += crossbarCenter;

        SetTextPos( m_crossBarStart + textOffset );
    }
    else if( m_textPosition == DIM_TEXT_POSITION::INLINE )
    {
        SetTextPos( m_crossBarStart + crossbarCenter );
    }

    if( m_keepTextAligned )
    {
        if( abs( crossbarCenter.x ) > abs( crossbarCenter.y ) )
            SetTextAngle( ANGLE_HORIZONTAL );
        else
            SetTextAngle( ANGLE_VERTICAL );
    }

    PCB_DIM_ALIGNED::updateText();
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


PCB_DIM_LEADER::PCB_DIM_LEADER( BOARD_ITEM* aParent ) :
        PCB_DIMENSION_BASE( aParent, PCB_DIM_LEADER_T ),
        m_textBorder( DIM_TEXT_BORDER::NONE )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
    m_keepTextAligned     = false;

    SetOverrideText( _( "Leader" ) );
}


void PCB_DIM_LEADER::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    PCB_DIMENSION_BASE::Serialize( aContainer );
    aContainer.UnpackTo( &dimension );

    PackVector2( *dimension.mutable_leader()->mutable_start(), m_start );
    PackVector2( *dimension.mutable_leader()->mutable_end(), m_end );
    dimension.mutable_leader()->set_border_style(
            ToProtoEnum<DIM_TEXT_BORDER, kiapi::board::types::DimensionTextBorderStyle>(
                    m_textBorder ) );

    aContainer.PackFrom( dimension );
}


bool PCB_DIM_LEADER::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    if( !PCB_DIMENSION_BASE::Deserialize( aContainer ) )
        return false;

    kiapi::board::types::Dimension dimension;
    aContainer.UnpackTo( &dimension );

    if( !dimension.has_leader() )
        return false;

    SetStart( UnpackVector2( dimension.leader().start() ) );
    SetEnd( UnpackVector2( dimension.leader().end() ) );
    SetTextBorder( FromProtoEnum<DIM_TEXT_BORDER>( dimension.leader().border_style() ) );

    Update();

    return true;
}


EDA_ITEM* PCB_DIM_LEADER::Clone() const
{
    return new PCB_DIM_LEADER( *this );
}


void PCB_DIM_LEADER::swapData( BOARD_ITEM* aImage )
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


void PCB_DIM_LEADER::updateText()
{
    // Our geometry is dependent on the size of the text, so just update the whole shebang
    updateGeometry();
}


void PCB_DIM_LEADER::updateGeometry()
{
    if( m_busy )    // Skeep reentrance that happens sometimes after calling updateText()
        return;

    m_busy = true;

    m_shapes.clear();

    PCB_DIMENSION_BASE::updateText();

    // Now that we have the text updated, we can determine how to draw the second line
    // First we need to create an appropriate bounding polygon to collide with
    BOX2I textBox = GetTextBox( nullptr ).Inflate( GetTextWidth() / 2, GetEffectiveTextPenWidth() * 2 );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( GetTextAngle(), textBox.GetCenter() );

    VECTOR2I firstLine( m_end - m_start );
    VECTOR2I start( m_start );
    start += firstLine.Resize( m_extensionOffset );

    SEG arrowSeg( m_start, m_end );
    SEG textSeg( m_end, GetTextPos() );
    OPT_VECTOR2I arrowSegEnd;
    OPT_VECTOR2I textSegEnd;

    if( m_textBorder == DIM_TEXT_BORDER::CIRCLE )
    {
        double penWidth = GetEffectiveTextPenWidth() / 2.0;
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

    drawAnArrow( start, EDA_ANGLE( firstLine ), 0 );

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
            double penWidth = GetEffectiveTextPenWidth() / 2.0;
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

    m_busy = false;
}


void PCB_DIM_LEADER::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText(); we want to see the variable references here
    aList.emplace_back( _( "Leader" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    ORIGIN_TRANSFORMS& originTransforms = aFrame->GetOriginTransforms();

    VECTOR2I startCoord = originTransforms.ToDisplayAbs( GetStart() );
    wxString start = wxString::Format( wxT( "@(%s, %s)" ),
                                       aFrame->MessageTextFromValue( startCoord.x ),
                                       aFrame->MessageTextFromValue( startCoord.y ) );

    aList.emplace_back( start, wxEmptyString );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


PCB_DIM_RADIAL::PCB_DIM_RADIAL( BOARD_ITEM* aParent ) :
        PCB_DIMENSION_BASE( aParent, PCB_DIM_RADIAL_T )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = false;
    m_keepTextAligned     = true;
    m_prefix              = "R ";
    m_leaderLength        = m_arrowLength * 3;
}


void PCB_DIM_RADIAL::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    PCB_DIMENSION_BASE::Serialize( aContainer );
    aContainer.UnpackTo( &dimension );

    PackVector2( *dimension.mutable_radial()->mutable_center(), m_start );
    PackVector2( *dimension.mutable_radial()->mutable_radius_point(), m_end );
    dimension.mutable_radial()->mutable_leader_length()->set_value_nm( m_leaderLength );

    aContainer.PackFrom( dimension );
}


bool PCB_DIM_RADIAL::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    if( !PCB_DIMENSION_BASE::Deserialize( aContainer ) )
        return false;

    kiapi::board::types::Dimension dimension;
    aContainer.UnpackTo( &dimension );

    if( !dimension.has_radial() )
        return false;

    SetStart( UnpackVector2( dimension.radial().center() ) );
    SetEnd( UnpackVector2( dimension.radial().radius_point() ) );
    SetLeaderLength( dimension.radial().leader_length().value_nm() );

    Update();

    return true;
}


EDA_ITEM* PCB_DIM_RADIAL::Clone() const
{
    return new PCB_DIM_RADIAL( *this );
}


void PCB_DIM_RADIAL::swapData( BOARD_ITEM* aImage )
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
        VECTOR2I  textLine( GetTextPos() - GetKnee() );
        EDA_ANGLE textAngle = FULL_CIRCLE - EDA_ANGLE( textLine );

        textAngle.Normalize();

        if( textAngle > ANGLE_90 && textAngle <= ANGLE_270 )
            textAngle -= ANGLE_180;

        // Round to nearest degree
        textAngle = EDA_ANGLE( KiROUND( textAngle.AsDegrees() ), DEGREES_T );

        SetTextAngle( textAngle );
    }

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_RADIAL::updateGeometry()
{
    if( m_busy )    // Skeep reentrance that happens sometimes after calling updateText()
        return;

    m_busy = true;

    m_shapes.clear();

    VECTOR2I center( m_start );
    VECTOR2I centerArm( 0, m_arrowLength );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - centerArm, center + centerArm ) );

    RotatePoint( centerArm, -ANGLE_90 );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - centerArm, center + centerArm ) );

    VECTOR2I radius( m_end - m_start );

    m_measuredValue = KiROUND( radius.EuclideanNorm() );

    updateText();

    // Now that we have the text updated, we can determine how to draw the second line
    // First we need to create an appropriate bounding polygon to collide with
    BOX2I textBox = GetTextBox( nullptr ).Inflate( GetTextWidth() / 2, GetEffectiveTextPenWidth() );

    SHAPE_POLY_SET polyBox;
    polyBox.NewOutline();
    polyBox.Append( textBox.GetOrigin() );
    polyBox.Append( textBox.GetOrigin().x, textBox.GetEnd().y );
    polyBox.Append( textBox.GetEnd() );
    polyBox.Append( textBox.GetEnd().x, textBox.GetOrigin().y );
    polyBox.Rotate( GetTextAngle(), textBox.GetCenter() );

    VECTOR2I radial( m_end - m_start );
    radial = radial.Resize( m_leaderLength );

    SEG arrowSeg( m_end, m_end + radial );
    SEG textSeg( arrowSeg.B, GetTextPos() );

    CollectKnockedOutSegments( polyBox, arrowSeg, m_shapes );
    CollectKnockedOutSegments( polyBox, textSeg, m_shapes );

    drawAnArrow( m_end, EDA_ANGLE( radial ), 0 );

    m_busy = false;
}


PCB_DIM_CENTER::PCB_DIM_CENTER( BOARD_ITEM* aParent ) :
        PCB_DIMENSION_BASE( aParent, PCB_DIM_CENTER_T )
{
    m_unitsFormat         = DIM_UNITS_FORMAT::NO_SUFFIX;
    m_overrideTextEnabled = true;
}


void PCB_DIM_CENTER::Serialize( google::protobuf::Any &aContainer ) const
{
    using namespace kiapi::common;
    kiapi::board::types::Dimension dimension;

    PCB_DIMENSION_BASE::Serialize( aContainer );
    aContainer.UnpackTo( &dimension );

    PackVector2( *dimension.mutable_center()->mutable_center(), m_start );
    PackVector2( *dimension.mutable_center()->mutable_end(), m_end );

    aContainer.PackFrom( dimension );
}


bool PCB_DIM_CENTER::Deserialize( const google::protobuf::Any &aContainer )
{
    using namespace kiapi::common;

    if( !PCB_DIMENSION_BASE::Deserialize( aContainer ) )
        return false;

    kiapi::board::types::Dimension dimension;
    aContainer.UnpackTo( &dimension );

    if( !dimension.has_center() )
        return false;

    SetStart( UnpackVector2( dimension.center().center() ) );
    SetEnd( UnpackVector2( dimension.center().end() ) );

    Update();

    return true;
}


EDA_ITEM* PCB_DIM_CENTER::Clone() const
{
    return new PCB_DIM_CENTER( *this );
}


void PCB_DIM_CENTER::swapData( BOARD_ITEM* aImage )
{
    wxASSERT( aImage->Type() == Type() );

    std::swap( *static_cast<PCB_DIM_CENTER*>( this ), *static_cast<PCB_DIM_CENTER*>( aImage ) );
}


BITMAPS PCB_DIM_CENTER::GetMenuImage() const
{
    return BITMAPS::add_center_dimension;
}


const BOX2I PCB_DIM_CENTER::GetBoundingBox() const
{
    BOX2I bBox;
    int   xmin, xmax, ymin, ymax;

    xmin    = m_start.x;
    xmax    = m_start.x;
    ymin    = m_start.y;
    ymax    = m_start.y;

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


const BOX2I PCB_DIM_CENTER::ViewBBox() const
{
    return GetBoundingBox();
}


void PCB_DIM_CENTER::updateText()
{
    // Even if PCB_DIM_CENTER has no text, we still need to update its text position
    // so GetTextPos() users get a valid value. Required at least for lasso hit-testing.
    SetTextPos( m_start );

    PCB_DIMENSION_BASE::updateText();
}


void PCB_DIM_CENTER::updateGeometry()
{
    if( m_busy )    // Skeep reentrance that happens sometimes after calling updateText()
        return;

    m_busy = true;

    m_shapes.clear();

    VECTOR2I center( m_start );
    VECTOR2I arm( m_end - m_start );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );

    RotatePoint( arm, -ANGLE_90 );

    m_shapes.emplace_back( new SHAPE_SEGMENT( center - arm, center + arm ) );

    updateText();

    m_busy = false;
}


static struct DIMENSION_DESC
{
    DIMENSION_DESC()
    {
        ENUM_MAP<DIM_PRECISION>::Instance()
                    .Map( DIM_PRECISION::X,       _HKI( "0" ) )
                    .Map( DIM_PRECISION::X_X,     _HKI( "0.0" ) )
                    .Map( DIM_PRECISION::X_XX,    _HKI( "0.00" ) )
                    .Map( DIM_PRECISION::X_XXX,   _HKI( "0.000" ) )
                    .Map( DIM_PRECISION::X_XXXX,  _HKI( "0.0000" ) )
                    .Map( DIM_PRECISION::X_XXXXX, _HKI( "0.00000" ) )
                    .Map( DIM_PRECISION::V_VV,    _HKI( "0.00 in / 0 mils / 0.0 mm" ) )
                    .Map( DIM_PRECISION::V_VVV,   _HKI( "0.000 / 0 / 0.00" ) )
                    .Map( DIM_PRECISION::V_VVVV,  _HKI( "0.0000 / 0.0 / 0.000" ) )
                    .Map( DIM_PRECISION::V_VVVVV, _HKI( "0.00000 / 0.00 / 0.0000" ) );

        ENUM_MAP<DIM_UNITS_FORMAT>::Instance()
                    .Map( DIM_UNITS_FORMAT::NO_SUFFIX,    _HKI( "1234.0" ) )
                    .Map( DIM_UNITS_FORMAT::BARE_SUFFIX,  _HKI( "1234.0 mm" ) )
                    .Map( DIM_UNITS_FORMAT::PAREN_SUFFIX, _HKI( "1234.0 (mm)" ) );

        ENUM_MAP<DIM_UNITS_MODE>::Instance()
                    .Map( DIM_UNITS_MODE::INCH,      _HKI( "Inches" ) )
                    .Map( DIM_UNITS_MODE::MILS,      _HKI( "Mils" ) )
                    .Map( DIM_UNITS_MODE::MM,        _HKI( "Millimeters" ) )
                    .Map( DIM_UNITS_MODE::AUTOMATIC, _HKI( "Automatic" ) );

        ENUM_MAP<DIM_ARROW_DIRECTION>::Instance()
                    .Map( DIM_ARROW_DIRECTION::INWARD,  _HKI( "Inward" ) )
                    .Map( DIM_ARROW_DIRECTION::OUTWARD, _HKI( "Outward" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIMENSION_BASE );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIMENSION_BASE, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIMENSION_BASE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIMENSION_BASE, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIMENSION_BASE ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIMENSION_BASE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIMENSION_BASE ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( PCB_DIMENSION_BASE ), TYPE_HASH( EDA_TEXT ), _HKI( "Orientation" ) );

        const wxString groupDimension = _HKI( "Dimension Properties" );

        auto isLeader =
                []( INSPECTABLE* aItem ) -> bool
                {
                    return dynamic_cast<PCB_DIM_LEADER*>( aItem ) != nullptr;
                };

        auto isNotLeader =
                []( INSPECTABLE* aItem ) -> bool
                {
                    return dynamic_cast<PCB_DIM_LEADER*>( aItem ) == nullptr;
                };

        auto isMultiArrowDirection =
                []( INSPECTABLE* aItem ) -> bool
                {
                    return dynamic_cast<PCB_DIM_ALIGNED*>( aItem ) != nullptr;
                };

        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, wxString>( _HKI( "Prefix" ),
                &PCB_DIMENSION_BASE::ChangePrefix, &PCB_DIMENSION_BASE::GetPrefix ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );
        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, wxString>( _HKI( "Suffix" ),
                &PCB_DIMENSION_BASE::ChangeSuffix, &PCB_DIMENSION_BASE::GetSuffix ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );
        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, wxString>( _HKI( "Override Text" ),
                &PCB_DIMENSION_BASE::ChangeOverrideText, &PCB_DIMENSION_BASE::GetOverrideText ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );

        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, wxString>( _HKI( "Text" ),
                &PCB_DIMENSION_BASE::ChangeOverrideText, &PCB_DIMENSION_BASE::GetOverrideText ),
                groupDimension )
                .SetAvailableFunc( isLeader );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DIMENSION_BASE, DIM_UNITS_MODE>( _HKI( "Units" ),
                &PCB_DIMENSION_BASE::ChangeUnitsMode, &PCB_DIMENSION_BASE::GetUnitsMode ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DIMENSION_BASE, DIM_UNITS_FORMAT>( _HKI( "Units Format" ),
                &PCB_DIMENSION_BASE::ChangeUnitsFormat, &PCB_DIMENSION_BASE::GetUnitsFormat ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );
        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DIMENSION_BASE, DIM_PRECISION>( _HKI( "Precision" ),
                &PCB_DIMENSION_BASE::ChangePrecision, &PCB_DIMENSION_BASE::GetPrecision ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );
        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, bool>( _HKI( "Suppress Trailing Zeroes" ),
                &PCB_DIMENSION_BASE::ChangeSuppressZeroes, &PCB_DIMENSION_BASE::GetSuppressZeroes ),
                groupDimension )
                .SetAvailableFunc( isNotLeader );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DIMENSION_BASE, DIM_ARROW_DIRECTION>( _HKI( "Arrow Direction"),
                &PCB_DIMENSION_BASE::ChangeArrowDirection, &PCB_DIMENSION_BASE::GetArrowDirection ),
                groupDimension )
                .SetAvailableFunc( isMultiArrowDirection );

        const wxString groupText = _HKI( "Text Properties" );

        const auto isTextOrientationWriteable =
                []( INSPECTABLE* aItem ) -> bool
                {
                    return !static_cast<PCB_DIMENSION_BASE*>( aItem )->GetKeepTextAligned();
                };

        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, bool>( _HKI( "Keep Aligned with Dimension" ),
                &PCB_DIMENSION_BASE::ChangeKeepTextAligned,
                &PCB_DIMENSION_BASE::GetKeepTextAligned ),
                groupText );

        propMgr.AddProperty( new PROPERTY<PCB_DIMENSION_BASE, double>( _HKI( "Orientation" ),
                &PCB_DIMENSION_BASE::ChangeTextAngleDegrees,
                &PCB_DIMENSION_BASE::GetTextAngleDegreesProp,
                PROPERTY_DISPLAY::PT_DEGREE ),
                groupText )
                .SetWriteableFunc( isTextOrientationWriteable );
    }
} _DIMENSION_DESC;

ENUM_TO_WXANY( DIM_PRECISION )
ENUM_TO_WXANY( DIM_UNITS_FORMAT )
ENUM_TO_WXANY( DIM_UNITS_MODE )
ENUM_TO_WXANY( DIM_ARROW_DIRECTION )


static struct ALIGNED_DIMENSION_DESC
{
    ALIGNED_DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIM_ALIGNED );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ALIGNED, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ALIGNED, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ALIGNED, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ALIGNED, PCB_DIMENSION_BASE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( EDA_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( PCB_DIMENSION_BASE ) );

        const wxString groupDimension = _HKI( "Dimension Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_DIM_ALIGNED, int>( _HKI( "Crossbar Height" ),
                &PCB_DIM_ALIGNED::ChangeHeight, &PCB_DIM_ALIGNED::GetHeight,
                PROPERTY_DISPLAY::PT_SIZE ),
                groupDimension );
        propMgr.AddProperty( new PROPERTY<PCB_DIM_ALIGNED, int>( _HKI( "Extension Line Overshoot" ),
                &PCB_DIM_ALIGNED::ChangeExtensionHeight, &PCB_DIM_ALIGNED::GetExtensionHeight,
                PROPERTY_DISPLAY::PT_SIZE ),
                groupDimension );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Text" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Vertical Justification" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ALIGNED ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Knockout" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} ALIGNED_DIMENSION_DESC;


static struct ORTHOGONAL_DIMENSION_DESC
{
    ORTHOGONAL_DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIM_ORTHOGONAL );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ORTHOGONAL, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ORTHOGONAL, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ORTHOGONAL, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ORTHOGONAL, PCB_DIMENSION_BASE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_ORTHOGONAL, PCB_DIM_ALIGNED> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( EDA_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( PCB_DIMENSION_BASE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( PCB_DIM_ALIGNED ) );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Text" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Vertical Justification" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_ORTHOGONAL ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Knockout" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} ORTHOGONAL_DIMENSION_DESC;


static struct RADIAL_DIMENSION_DESC
{
    RADIAL_DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIM_RADIAL );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_RADIAL, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_RADIAL, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_RADIAL, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_RADIAL, PCB_DIMENSION_BASE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( EDA_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( PCB_DIMENSION_BASE ) );

        const wxString groupDimension = _HKI( "Dimension Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_DIM_RADIAL, int>( _HKI( "Leader Length" ),
                &PCB_DIM_RADIAL::ChangeLeaderLength, &PCB_DIM_RADIAL::GetLeaderLength,
                PROPERTY_DISPLAY::PT_SIZE ),
                groupDimension );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Text" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Vertical Justification" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_RADIAL ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Knockout" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} RADIAL_DIMENSION_DESC;


static struct LEADER_DIMENSION_DESC
{
    LEADER_DIMENSION_DESC()
    {
        ENUM_MAP<DIM_TEXT_BORDER>::Instance()
                    .Map( DIM_TEXT_BORDER::NONE,      _HKI( "None" ) )
                    .Map( DIM_TEXT_BORDER::RECTANGLE, _HKI( "Rectangle" ) )
                    .Map( DIM_TEXT_BORDER::CIRCLE,    _HKI( "Circle" ) );

        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIM_LEADER );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_LEADER, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_LEADER, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_LEADER, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_LEADER, PCB_DIMENSION_BASE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( EDA_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( PCB_DIMENSION_BASE ) );

        const wxString groupDimension = _HKI( "Dimension Properties" );

        propMgr.AddProperty( new PROPERTY_ENUM<PCB_DIM_LEADER, DIM_TEXT_BORDER>( _HKI( "Text Frame" ),
                &PCB_DIM_LEADER::ChangeTextBorder, &PCB_DIM_LEADER::GetTextBorder ),
                groupDimension );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Text" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Vertical Justification" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_LEADER ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Knockout" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} LEADER_DIMENSION_DESC;

ENUM_TO_WXANY( DIM_TEXT_BORDER )


static struct CENTER_DIMENSION_DESC
{
    CENTER_DIMENSION_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_DIM_CENTER );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_CENTER, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_CENTER, EDA_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_CENTER, PCB_TEXT> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_DIM_CENTER, PCB_DIMENSION_BASE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( EDA_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( PCB_TEXT ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( PCB_DIMENSION_BASE ) );


        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Text" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Vertical Justification" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
        propMgr.OverrideAvailability( TYPE_HASH( PCB_DIM_CENTER ), TYPE_HASH( BOARD_ITEM ),
                                      _HKI( "Knockout" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} CENTER_DIMENSION_DESC;

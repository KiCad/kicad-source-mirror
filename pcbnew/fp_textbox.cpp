/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <board.h>
#include <board_design_settings.h>
#include <core/mirror.h>
#include <footprint.h>
#include <fp_textbox.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <painter.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>


FP_TEXTBOX::FP_TEXTBOX( FOOTPRINT* aParentFootprint ) :
        FP_SHAPE( aParentFootprint, SHAPE_T::RECT, PCB_FP_TEXTBOX_T ),
        EDA_TEXT( pcbIUScale )
{
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );

    SetDrawCoord();
}


FP_TEXTBOX::~FP_TEXTBOX()
{
}


int FP_TEXTBOX::GetTextMargin() const
{
    return KiROUND( GetStroke().GetWidth() / 2.0 ) + KiROUND( GetTextSize().y * 0.75 );
}


VECTOR2I FP_TEXTBOX::GetTopLeft() const
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 )
        return VECTOR2I( GetStartX(), GetEndY() );
    else if( rotation == ANGLE_180 )
        return GetEnd();
    else if( rotation == ANGLE_270 )
        return VECTOR2I( GetEndX(), GetStartY() );
    else
        return GetStart();
}


VECTOR2I FP_TEXTBOX::GetBotRight() const
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 )
        return VECTOR2I( GetEndX(), GetStartY() );
    else if( rotation == ANGLE_180 )
        return GetStart();
    else if( rotation == ANGLE_270 )
        return VECTOR2I( GetStartX(), GetEndY() );
    else
        return GetEnd();
}


void FP_TEXTBOX::SetTop( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetEndY( aVal );
    else
        SetStartY( aVal );
}


void FP_TEXTBOX::SetBottom( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetStartY( aVal );
    else
        SetEndY( aVal );
}


void FP_TEXTBOX::SetLeft( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetEndX( aVal );
    else
        SetStartX( aVal );
}


void FP_TEXTBOX::SetRight( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetStartX( aVal );
    else
        SetEndX( aVal );
}


std::vector<VECTOR2I> FP_TEXTBOX::GetCorners() const
{
    std::vector<VECTOR2I> pts = FP_SHAPE::GetCorners();

    // SHAPE_T::POLY doesn't use the DrawCoord/LocalCoord architecture and instead stores fully
    // resolved points (ie: relative to the board, not parent footprint).
    if( GetShape() == SHAPE_T::POLY )
    {
        if( FOOTPRINT* parentFootprint = PCB_SHAPE::GetParentFootprint() )
        {
            for( VECTOR2I& pt : pts )
                RotatePoint( pt, parentFootprint->GetPosition(), parentFootprint->GetOrientation() );
        }
    }

    return pts;
}


EDA_ANGLE FP_TEXTBOX::GetDrawRotation() const
{
    EDA_ANGLE rotation = GetTextAngle();

    if( FOOTPRINT* parentFootprint = PCB_SHAPE::GetParentFootprint() )
        rotation += parentFootprint->GetOrientation();

    rotation.Normalize();

    return rotation;
}


std::vector<VECTOR2I> FP_TEXTBOX::GetNormalizedCorners() const
{
    std::vector<VECTOR2I> corners = GetCorners();
    EDA_ANGLE             textAngle( GetDrawRotation() );

    if( FOOTPRINT* parentFootprint = PCB_SHAPE::GetParentFootprint() )
    {
        if( parentFootprint->IsFlipped() )
            std::swap( corners[1], corners[3] );
    }

    textAngle.Normalize();

    if( textAngle < ANGLE_90 )
    {
        if( corners[1].y > corners[0].y )
            std::swap( corners[1], corners[3] );
    }
    else if( textAngle < ANGLE_180 )
    {
        if( corners[1].x > corners[0].x )
            std::swap( corners[1], corners[3] );
    }
    else if( textAngle < ANGLE_270 )
    {
        if( corners[1].y < corners[0].y )
            std::swap( corners[1], corners[3] );
    }
    else
    {
        if( corners[1].x < corners[0].x )
            std::swap( corners[1], corners[3] );
    }

    return corners;
}


VECTOR2I FP_TEXTBOX::GetDrawPos() const
{
    std::vector<VECTOR2I> corners = GetNormalizedCorners();
    GR_TEXT_H_ALIGN_T     effectiveAlignment = GetHorizJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              vMargin;
    VECTOR2I              hMargin;
    bool                  isFlipped = false;

    if( FOOTPRINT* parentFootprint = PCB_SHAPE::GetParentFootprint() )
        isFlipped = parentFootprint->IsFlipped();

    if( IsMirrored() != isFlipped )
    {
        switch( GetHorizJustify() )
        {
        case GR_TEXT_H_ALIGN_LEFT:   effectiveAlignment = GR_TEXT_H_ALIGN_RIGHT;  break;
        case GR_TEXT_H_ALIGN_CENTER: effectiveAlignment = GR_TEXT_H_ALIGN_CENTER; break;
        case GR_TEXT_H_ALIGN_RIGHT:  effectiveAlignment = GR_TEXT_H_ALIGN_LEFT;   break;
        }
    }

    switch( effectiveAlignment )
    {
    case GR_TEXT_H_ALIGN_LEFT:
        textAnchor = corners[0];
        vMargin = ( corners[2] - corners[1] ).Resize( GetTextMargin() );
        hMargin = ( corners[1] - corners[0] ).Resize( GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_CENTER:
        textAnchor = ( corners[0] + corners[1] ) / 2;
        vMargin = ( corners[2] - corners[1] ).Resize( GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_RIGHT:
        textAnchor = corners[1];
        vMargin = ( corners[2] - corners[1] ).Resize( GetTextMargin() );
        hMargin = ( corners[0] - corners[1] ).Resize( GetTextMargin() );
        break;
    }

    return textAnchor + hMargin + vMargin;
}


bool FP_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool FP_TEXTBOX::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void FP_TEXTBOX::Move( const VECTOR2I& aMoveVector )
{
    FP_SHAPE::Move( aMoveVector );
    EDA_TEXT::Offset( aMoveVector );
}


void FP_TEXTBOX::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    FP_SHAPE::Rotate( aRotCentre, aAngle );
    SetTextAngle( GetTextAngle() + aAngle );
}


void FP_TEXTBOX::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    FP_SHAPE::Flip( aCentre, aFlipLeftRight );

    // flipping the footprint is relative to the X axis
    if( aFlipLeftRight )
    {
        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
        SetTextAngle( -GetTextAngle() );
    }
    else
    {
        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
        SetTextAngle( ANGLE_180 - GetTextAngle() );
    }

    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        SetMirrored( !IsMirrored() );

    SetLocalCoord();
}


void FP_TEXTBOX::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position is mirrored, but not the text (or its justification)
    FP_SHAPE::Mirror( aCentre, aMirrorAroundXAxis );

    BOX2I rect( m_start0, m_end0 - m_start0 );
    rect.Normalize();
    m_start0 = VECTOR2I( rect.GetLeft(), rect.GetTop() );
    m_end0 = VECTOR2I( rect.GetRight(), rect.GetBottom() );

    SetDrawCoord();
}


void FP_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), UnescapeString( GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );

    wxString msg = aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) );
    aList.emplace_back( _( "Box Width" ), msg );

    msg = aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) );
    aList.emplace_back( _( "Box Height" ), msg );

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


wxString FP_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "Footprint Text Box of %s" ),
                             static_cast<FOOTPRINT*>( GetParent() )->GetReference() );
}


BITMAPS FP_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


EDA_ITEM* FP_TEXTBOX::Clone() const
{
    return new FP_TEXTBOX( *this );
}


void FP_TEXTBOX::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_MOD_TEXT_INVISIBLE;

    aCount = 1;
}


double FP_TEXTBOX::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = (double)std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    // Hidden text gets put on the LAYER_MOD_TEXT_INVISIBLE for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    RENDER_SETTINGS* renderSettings = aView->GetPainter()->GetSettings();
    COLOR4D          backgroundColor = renderSettings->GetLayerColor( LAYER_PCB_BACKGROUND );

    // Handle Render tab switches
    if( renderSettings->GetLayerColor( LAYER_MOD_TEXT ) == backgroundColor )
        return HIDE;

    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( !aView->IsLayerVisible( LAYER_MOD_TEXT ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0.0;
}


wxString FP_TEXTBOX::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( GetParent() );

    std::function<bool( wxString* )> footprintResolver =
            [&]( wxString* token ) -> bool
            {
                return parentFootprint && parentFootprint->ResolveTextVar( token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
    {
        if( aDepth < 10 )
            text = ExpandTextVars( text, &footprintResolver );
    }

    KIFONT::FONT*         font = getDrawFont();
    std::vector<VECTOR2I> corners = GetNormalizedCorners();
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    colWidth -= GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


std::shared_ptr<SHAPE> FP_TEXTBOX::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = GetEffectiveTextShape();

    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
        shape->AddShape( PCB_SHAPE::GetEffectiveShape( aLayer, aFlash ) );

    return shape;
}


void FP_TEXTBOX::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer,  int aClearance, int aError,
                                         ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    // Note: this function is mainly used in 3D viewer.
    // the polygonal shape of a text can have many basic shapes,
    // so combining these shapes can be very useful to create a final shape
    // swith a lot less vertices to speedup calculations using this final shape
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET buffer;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( buffer, aPt1, aPt2, penWidth, aError, aErrorLoc );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                buffer.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    buffer.Append( point.x, point.y );
            } );

    font->Draw( &callback_gal, GetShownText( true ), GetDrawPos(), GetAttributes() );

    buffer.Simplify( SHAPE_POLY_SET::PM_FAST );
    aBuffer.Append( buffer );
}


void FP_TEXTBOX::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                          int aClearance, int aError, ERROR_LOC aErrorLoc,
                                          bool aIgnoreLineWidth ) const
{
    // Don't use FP_SHAPE::TransformShapeToPolygon.  We want to treat the textbox as filled even
    // if there's no background colour.

    int width = GetWidth() + ( 2 * aClearance );

    switch( m_shape )
    {
    case SHAPE_T::RECT:
    {
        std::vector<VECTOR2I> pts = GetRectCorners();

        aBuffer.NewOutline();

        for( const VECTOR2I& pt : pts )
            aBuffer.Append( pt );

        if( width > 0 )
        {
            // Add in segments
            TransformOvalToPolygon( aBuffer, pts[0], pts[1], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[1], pts[2], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[2], pts[3], width, aError, aErrorLoc );
            TransformOvalToPolygon( aBuffer, pts[3], pts[0], width, aError, aErrorLoc );
        }

        break;
    }

    case SHAPE_T::POLY:
    {
        if( !IsPolyShapeValid() )
            break;

        // The polygon is expected to be a simple polygon; not self intersecting, no hole.
        EDA_ANGLE orientation = getParentOrientation();
        VECTOR2I  offset = getParentPosition();

        // Build the polygon with the actual position and orientation:
        std::vector<VECTOR2I> poly;
        DupPolyPointsList( poly );

        for( VECTOR2I& point : poly )
        {
            RotatePoint( point, orientation );
            point += offset;
        }

        aBuffer.NewOutline();

        for( const VECTOR2I& point : poly )
            aBuffer.Append( point.x, point.y );

        if( width > 0 )
        {
            VECTOR2I pt1( poly[poly.size() - 1] );

            for( const VECTOR2I& pt2 : poly )
            {
                if( pt2 != pt1 )
                    TransformOvalToPolygon( aBuffer, pt1, pt2, width, aError, aErrorLoc );

                pt1 = pt2;
            }
        }

        break;
    }

    default:
        UNIMPLEMENTED_FOR( SHAPE_T_asString() );
        break;
    }
}


wxString FP_TEXTBOX::GetParentAsString() const
{
    if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( m_parent ) )
        return fp->GetReference();

    return m_parent->m_Uuid.AsString();
}


bool FP_TEXTBOX::IsBorderEnabled() const
{
    return m_stroke.GetWidth() != -1;
}


void FP_TEXTBOX::DisableBorder()
{
    m_stroke.SetWidth( -1 );
}


static struct FP_TEXTBOX_DESC
{
    FP_TEXTBOX_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( FP_TEXTBOX );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXTBOX, FP_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXTBOX ), TYPE_HASH( FP_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<FP_TEXTBOX, wxString>( _HKI( "Parent" ),
                    NO_SETTER( FP_TEXTBOX, wxString ), &FP_TEXTBOX::GetParentAsString ) );
    }
} _FP_TEXTBOX_DESC;

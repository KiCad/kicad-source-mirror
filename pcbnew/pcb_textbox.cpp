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
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_textbox.h>
#include <pcb_painter.h>
#include <trigo.h>
#include <string_utils.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>
#include <macros.h>
#include <core/ignore.h>


PCB_TEXTBOX::PCB_TEXTBOX( BOARD_ITEM* parent ) :
    PCB_SHAPE( parent, PCB_TEXTBOX_T, SHAPE_T::RECT ),
    EDA_TEXT( pcbIUScale )
{
    SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    SetMultilineAllowed( true );
}


PCB_TEXTBOX::~PCB_TEXTBOX()
{
}


int PCB_TEXTBOX::GetTextMargin() const
{
    return KiROUND( GetStroke().GetWidth() / 2.0 ) + KiROUND( GetTextSize().y * 0.75 );
}


VECTOR2I PCB_TEXTBOX::GetTopLeft() const
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


VECTOR2I PCB_TEXTBOX::GetBotRight() const
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


void PCB_TEXTBOX::SetTop( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetEndY( aVal );
    else
        SetStartY( aVal );
}


void PCB_TEXTBOX::SetBottom( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_90 || rotation == ANGLE_180 )
        SetStartY( aVal );
    else
        SetEndY( aVal );
}


void PCB_TEXTBOX::SetLeft( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetEndX( aVal );
    else
        SetStartX( aVal );
}


void PCB_TEXTBOX::SetRight( int aVal )
{
    EDA_ANGLE rotation = GetDrawRotation();

    if( rotation == ANGLE_180 || rotation == ANGLE_270 )
        SetStartX( aVal );
    else
        SetEndX( aVal );
}


void PCB_TEXTBOX::SetTextAngle( const EDA_ANGLE& aAngle )
{
    EDA_ANGLE delta = aAngle.Normalized() - GetTextAngle();
    Rotate( GetPosition(), delta );
}


std::vector<VECTOR2I> PCB_TEXTBOX::GetAnchorAndOppositeCorner() const
{
    std::vector<VECTOR2I> pts;
    std::vector<VECTOR2I> corners = GetCorners();
    EDA_ANGLE             textAngle( GetDrawRotation() );

    textAngle.Normalize();

    pts.emplace_back( corners[0] );

    if( textAngle < ANGLE_90 )
    {
        if( corners[1].y <= corners[0].y )
            pts.emplace_back( corners[1] );
        else
            pts.emplace_back( corners[3] );
    }
    else if( textAngle < ANGLE_180 )
    {
        if( corners[1].x <= corners[0].x )
            pts.emplace_back( corners[1] );
        else
            pts.emplace_back( corners[3] );
    }
    else if( textAngle < ANGLE_270 )
    {
        if( corners[1].y >= corners[0].y )
            pts.emplace_back( corners[1] );
        else
            pts.emplace_back( corners[3] );
    }
    else
    {
        if( corners[1].x >= corners[0].x )
            pts.emplace_back( corners[1] );
        else
            pts.emplace_back( corners[3] );
    }

    return pts;
}


VECTOR2I PCB_TEXTBOX::GetDrawPos() const
{
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    GR_TEXT_H_ALIGN_T     effectiveAlignment = GetHorizJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              offset;

    if( IsMirrored() )
    {
        std::swap( corners[0], corners[1] );

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
        offset = VECTOR2I( GetTextMargin(), GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_CENTER:
        textAnchor = ( corners[0] + corners[1] ) / 2;
        offset = VECTOR2I( 0, GetTextMargin() );
        break;
    case GR_TEXT_H_ALIGN_RIGHT:
        textAnchor = corners[1];
        offset = VECTOR2I( -GetTextMargin(), GetTextMargin() );
        break;
    }

    RotatePoint( offset, GetDrawRotation() );
    return textAnchor + offset;
}


double PCB_TEXTBOX::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow if the main layer is not shown
        if( !aView->IsLayerVisible( m_layer ) )
            return HIDE;

        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    return 0.0;
}


wxString PCB_TEXTBOX::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    BOARD* board = dynamic_cast<BOARD*>( GetParent() );

    std::function<bool( wxString* )> pcbTextResolver =
            [&]( wxString* token ) -> bool
            {
                if( token->IsSameAs( wxT( "LAYER" ) ) )
                {
                    *token = GetLayerName();
                    return true;
                }

                if( board->ResolveTextVar( token, aDepth + 1 ) )
                {
                    return true;
                }

                return false;
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( board && HasTextVars() && aDepth < 10 )
        text = ExpandTextVars( text, &pcbTextResolver );

    KIFONT::FONT*         font = getDrawFont();
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    colWidth -= GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


bool PCB_TEXTBOX::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


void PCB_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Text Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );

    aList.emplace_back( _( "Box Width" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().x - GetStart().x ) ) );

    aList.emplace_back( _( "Box Height" ),
                        aFrame->MessageTextFromValue( std::abs( GetEnd().y - GetStart().y ) ));

    m_stroke.GetMsgPanelInfo( aFrame, aList );
}


void PCB_TEXTBOX::Move( const VECTOR2I& aMoveVector )
{
    PCB_SHAPE::Move( aMoveVector );
    EDA_TEXT::Offset( aMoveVector );
}


void PCB_TEXTBOX::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    PCB_SHAPE::Rotate( aRotCentre, aAngle );
    EDA_TEXT::SetTextAngle( ( GetTextAngle() + aAngle ).Normalized() );

    if( GetTextAngle().IsCardinal() && GetShape() != SHAPE_T::RECT )
    {
        std::vector<VECTOR2I> corners = GetCorners();
        VECTOR2I              diag = corners[2] - corners[0];
        EDA_ANGLE             angle = GetTextAngle();

        SetShape( SHAPE_T::RECT );
        SetStart( corners[0] );

        angle.Normalize();

        if( angle == ANGLE_90 )
            SetEnd( VECTOR2I( corners[0].x + abs( diag.x ), corners[0].y - abs( diag.y ) ) );
        else if( angle == ANGLE_180 )
            SetEnd( VECTOR2I( corners[0].x - abs( diag.x ), corners[0].y - abs( diag.y ) ) );
        else if( angle == ANGLE_270 )
            SetEnd( VECTOR2I( corners[0].x - abs( diag.x ), corners[0].y + abs( diag.y ) ) );
        else
            SetEnd( VECTOR2I( corners[0].x + abs( diag.x ), corners[0].y + abs( diag.y ) ) );
    }
}


void PCB_TEXTBOX::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position is mirrored, but not the text (or its justification)
    PCB_SHAPE::Mirror( aCentre, aMirrorAroundXAxis );

    BOX2I rect( m_start, m_end - m_start );
    rect.Normalize();
    m_start = VECTOR2I( rect.GetLeft(), rect.GetTop() );
    m_end = VECTOR2I( rect.GetRight(), rect.GetBottom() );
}


void PCB_TEXTBOX::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        m_start.x = aCentre.x - ( m_start.x - aCentre.x );
        m_end.x   = aCentre.x - ( m_end.x - aCentre.x );
        EDA_TEXT::SetTextAngle( -GetTextAngle() );
    }
    else
    {
        m_start.y = aCentre.y - ( m_start.y - aCentre.y );
        m_end.y   = aCentre.y - ( m_end.y - aCentre.y );
        EDA_TEXT::SetTextAngle( ANGLE_180 - GetTextAngle() );
    }

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );

    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        SetMirrored( !IsMirrored() );
}


bool PCB_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_TEXTBOX::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


wxString PCB_TEXTBOX::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    return wxString::Format( _( "PCB Text Box on %s" ), GetLayerName() );
}


BITMAPS PCB_TEXTBOX::GetMenuImage() const
{
    return BITMAPS::add_textbox;
}


EDA_ITEM* PCB_TEXTBOX::Clone() const
{
    return new PCB_TEXTBOX( *this );
}


void PCB_TEXTBOX::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TEXTBOX_T );

    std::swap( *((PCB_TEXTBOX*) this), *((PCB_TEXTBOX*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXTBOX::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    std::shared_ptr<SHAPE_COMPOUND> shape = GetEffectiveTextShape();

    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
        shape->AddShape( PCB_SHAPE::GetEffectiveShape( aLayer, aFlash ) );

    return shape;
}


void PCB_TEXTBOX::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                          int aClearance, int aError, ERROR_LOC aErrorLoc ) const
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
                TransformOvalToPolygon( buffer, aPt1, aPt2, penWidth + ( 2 * aClearance ), aError,
                                        ERROR_INSIDE );
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


void PCB_TEXTBOX::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                           int aClearance, int aError, ERROR_LOC aErrorLoc,
                                           bool aIgnoreLineWidth ) const
{
    // Don't use PCB_SHAPE::TransformShapeToPolygon.  We want to treat the textbox as filled even
    // if there's no background colour.

    int width = GetWidth() + ( 2 * aClearance );

    if( GetShape() == SHAPE_T::RECT )
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
    }
    else if( GetShape() == SHAPE_T::POLY )  // Non-cardinally-rotated rect
    {
        aBuffer.NewOutline();

        const SHAPE_LINE_CHAIN& poly = m_poly.Outline( 0 );

        for( int ii = 0; ii < poly.PointCount(); ++ii )
            aBuffer.Append( poly.GetPoint( ii ) );

        if( width > 0 )
        {
            for( int ii = 0; ii < poly.SegmentCount(); ++ii )
            {
                const SEG& seg = poly.GetSegment( ii );
                TransformOvalToPolygon( aBuffer, seg.A, seg.B, width, aError, aErrorLoc );
            }
        }
    }
}


static struct PCB_TEXTBOX_DESC
{
    PCB_TEXTBOX_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXTBOX );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, PCB_SHAPE> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXTBOX, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( PCB_SHAPE ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_TEXT ) );

        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Shape" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Start Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "End X" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "End Y" ) );
        propMgr.Mask( TYPE_HASH( PCB_TEXTBOX ), TYPE_HASH( EDA_SHAPE ), _HKI( "Line Width" ) );
    }
} _PCB_TEXTBOX_DESC;

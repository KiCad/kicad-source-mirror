/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/mirror.h>
#include <footprint.h>
#include <fp_textbox.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <string_utils.h>
#include <painter.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>

FP_TEXTBOX::FP_TEXTBOX( FOOTPRINT* aParentFootprint ) :
        FP_SHAPE( aParentFootprint, SHAPE_T::RECT, PCB_FP_TEXTBOX_T ),
        EDA_TEXT()
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
    return KiROUND( GetTextSize().y * 0.8 );
}


EDA_ANGLE FP_TEXTBOX::GetDrawRotation() const
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );
    EDA_ANGLE  rotation = GetTextAngle();

    if( parentFootprint )
        rotation += parentFootprint->GetOrientation();

    rotation.Normalize();

    return rotation;
}


std::vector<VECTOR2I> FP_TEXTBOX::GetAnchorAndOppositeCorner() const
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


VECTOR2I FP_TEXTBOX::GetDrawPos() const
{
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    GR_TEXT_H_ALIGN_T     effectiveAlignment = GetHorizJustify();
    VECTOR2I              textAnchor;
    VECTOR2I              offset;

    if( IsMirrored() )
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


bool FP_TEXTBOX::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    EDA_RECT rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool FP_TEXTBOX::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT rect = aRect;

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

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
    SetMirrored( IsBackLayer( GetLayer() ) );
    SetLocalCoord();
}


void FP_TEXTBOX::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position is mirrored, but the text itself is not mirrored

    if( aMirrorAroundXAxis )
        SetTextY( ::MIRRORVAL( GetTextPos().y, aCentre.y ) );
    else
        SetTextX( ::MIRRORVAL( GetTextPos().x, aCentre.x ) );

    SetLocalCoord();
}


void FP_TEXTBOX::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text Box" ), UnescapeString( GetText() ) );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );
    aList.emplace_back( _( "Angle" ), wxString::Format( "%g", GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Thickness" ), MessageTextFromValue( units, GetTextThickness() ) );
    aList.emplace_back( _( "Text Width" ), MessageTextFromValue( units, GetTextWidth() ) );
    aList.emplace_back( _( "Text Height" ), MessageTextFromValue( units, GetTextHeight() ) );

    wxString msg = MessageTextFromValue( units, std::abs( GetEnd().x - GetStart().x ) );
    aList.emplace_back( _( "Box Width" ), msg );

    msg = MessageTextFromValue( units, std::abs( GetEnd().y - GetStart().y ) );
    aList.emplace_back( _( "Box Height" ), msg );

    m_stroke.GetMsgPanelInfo( units, aList );
}


wxString FP_TEXTBOX::GetSelectMenuText( EDA_UNITS aUnits ) const
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


wxString FP_TEXTBOX::GetShownText( int aDepth ) const
{
    const FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( GetParent() );
    wxASSERT( parentFootprint );
    const BOARD*  board = parentFootprint->GetBoard();

    std::function<bool( wxString* )> footprintResolver =
            [&]( wxString* token ) -> bool
            {
                return parentFootprint && parentFootprint->ResolveTextVar( token, aDepth );
            };

    std::function<bool( wxString* )> boardTextResolver =
            [&]( wxString* token ) -> bool
            {
                return board->ResolveTextVar( token, aDepth + 1 );
            };

    wxString text = EDA_TEXT::GetShownText();

    if( HasTextVars() )
    {
        PROJECT* project = nullptr;

        if( parentFootprint && parentFootprint->GetParent() )
            project = static_cast<BOARD*>( parentFootprint->GetParent() )->GetProject();

        if( aDepth < 10 )
            text = ExpandTextVars( text, &footprintResolver, &boardTextResolver, project );
    }

    KIFONT::FONT*         font = GetDrawFont();
    std::vector<VECTOR2I> corners = GetAnchorAndOppositeCorner();
    int                   colWidth = ( corners[1] - corners[0] ).EuclideanNorm();

    colWidth -= GetTextMargin() * 2;
    font->LinebreakText( text, colWidth, GetTextSize(), GetTextThickness(), IsBold(), IsItalic() );

    return text;
}


std::shared_ptr<SHAPE> FP_TEXTBOX::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return GetEffectiveTextShape();
}


void FP_TEXTBOX::TransformTextShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                        PCB_LAYER_ID aLayer, int aClearance,
                                                        int aError, ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = GetDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( aCornerBuffer, aPt1, aPt2, penWidth + ( 2 * aClearance ),
                                        aError, ERROR_INSIDE );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                aCornerBuffer.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    aCornerBuffer.Append( point.x, point.y );
            } );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_Angle = GetDrawRotation();

    font->Draw( &callback_gal, GetShownText(), GetDrawPos(), attrs );
}


void FP_TEXTBOX::TransformShapeWithClearanceToPolygon( SHAPE_POLY_SET& aCornerBuffer,
                                                    PCB_LAYER_ID aLayer, int aClearance,
                                                    int aError, ERROR_LOC aErrorLoc,
                                                    bool aIgnoreLineWidth ) const
{
    if( PCB_SHAPE::GetStroke().GetWidth() >= 0 )
    {
        FP_SHAPE::TransformShapeWithClearanceToPolygon( aCornerBuffer, aLayer, aClearance,
                                                        aError, aErrorLoc );
    }
    else
    {
        EDA_TEXT::TransformBoundingBoxWithClearanceToPolygon( &aCornerBuffer, aClearance );
    }
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

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <pcb_edit_frame.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <core/mirror.h>
#include <footprint.h>
#include <pcb_text.h>
#include <pcb_painter.h>
#include <trigo.h>
#include <string_utils.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>


PCB_TEXT::PCB_TEXT( BOARD_ITEM* parent, KICAD_T idtype ) :
        BOARD_ITEM( parent, idtype ),
        EDA_TEXT( pcbIUScale ),
        m_type( TEXT_is_DIVERS )
{
    SetMultilineAllowed( true );
}


PCB_TEXT::PCB_TEXT( FOOTPRINT* aParent, TEXT_TYPE text_type ) :
        BOARD_ITEM( aParent, PCB_TEXT_T ),
        EDA_TEXT( pcbIUScale ),
        m_type( text_type )
{
    SetKeepUpright( true );

    // Set text thickness to a default value
    SetTextThickness( pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ) );
    SetLayer( F_SilkS );

    if( aParent )
    {
        SetTextPos( aParent->GetPosition() );

        if( IsBackLayer( aParent->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            SetMirrored( true );
        }
    }
}


PCB_TEXT::~PCB_TEXT()
{
}


wxString PCB_TEXT::GetShownText( bool aAllowExtraText, int aDepth ) const
{
    const FOOTPRINT* parentFootprint = GetParentFootprint();
    const BOARD*     board = GetBoard();

    std::function<bool( wxString* )> resolver =
            [&]( wxString* token ) -> bool
            {
                if( parentFootprint && parentFootprint->ResolveTextVar( token, aDepth + 1 ) )
                    return true;

                if( token->IsSameAs( wxT( "LAYER" ) ) )
                {
                    *token = GetLayerName();
                    return true;
                }

                if( board->ResolveTextVar( token, aDepth + 1 ) )
                    return true;

                return false;
            };

    wxString text = EDA_TEXT::GetShownText( aAllowExtraText, aDepth );

    if( HasTextVars() )
    {
        if( aDepth < 10 )
            text = ExpandTextVars( text, &resolver );
    }

    return text;
}


bool PCB_TEXT::Matches( const EDA_SEARCH_DATA& aSearchData, void* aAuxData ) const
{
    return BOARD_ITEM::Matches( UnescapeString( GetText() ), aSearchData );
}


EDA_ANGLE PCB_TEXT::GetDrawRotation() const
{
    EDA_ANGLE rotation = GetTextAngle();

    if( GetParentFootprint() && IsKeepUpright() )
    {
        // Keep angle between ]-90..90] deg. Otherwise the text is not easy to read
        while( rotation > ANGLE_90 )
            rotation -= ANGLE_180;

        while( rotation <= -ANGLE_90 )
            rotation += ANGLE_180;
    }
    else
    {
        rotation.Normalize();
    }

    return rotation;
}


const BOX2I PCB_TEXT::ViewBBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     text_area = GetTextBox();

    if( !angle.IsZero() )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return BOX2I( text_area.GetPosition(), text_area.GetSize() );
}


void PCB_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( GetParentFootprint() == nullptr || IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_MOD_TEXT_INVISIBLE;

    aCount = 1;
}


double PCB_TEXT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    KIGFX::PCB_PAINTER*  painter = static_cast<KIGFX::PCB_PAINTER*>( aView->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    // Hidden text gets put on the LAYER_MOD_TEXT_INVISIBLE for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    if( aLayer == LAYER_LOCKED_ITEM_SHADOW )
    {
        // Hide shadow on dimmed tracks
        if( renderSettings->GetHighContrast() )
        {
            if( m_layer != renderSettings->GetPrimaryHighContrastLayer() )
                return HIDE;
        }
    }

    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        // Handle Render tab switches
        if( m_type == TEXT_is_VALUE || GetText() == wxT( "${VALUE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_MOD_VALUES ) )
                return HIDE;
        }

        if( m_type == TEXT_is_REFERENCE || GetText() == wxT( "${REFERENCE}" ) )
        {
            if( !aView->IsLayerVisible( LAYER_MOD_REFERENCES ) )
                return HIDE;
        }

        if( parentFP->GetLayer() == F_Cu && !aView->IsLayerVisible( LAYER_MOD_FR ) )
            return HIDE;

        if( parentFP->GetLayer() == B_Cu && !aView->IsLayerVisible( LAYER_MOD_BK ) )
            return HIDE;

        if( !aView->IsLayerVisible( LAYER_MOD_TEXT ) )
            return HIDE;
    }

    return 0.0;
}


void PCB_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    FOOTPRINT* parentFP = GetParentFootprint();

    if( parentFP && aFrame->GetName() == PCB_EDIT_FRAME_NAME )
        aList.emplace_back( _( "Footprint" ), parentFP->GetReference() );

    // Don't use GetShownText() here; we want to show the user the variable references
    if( parentFP )
        aList.emplace_back( _( "Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );
    else
        aList.emplace_back( _( "PCB Text" ), KIUI::EllipsizeStatusText( aFrame, GetText() ) );

    if( parentFP )
    {
        switch( m_type )
        {
        case TEXT_is_REFERENCE: aList.emplace_back( _( "Type" ), _( "Reference" ) ); break;
        case TEXT_is_VALUE:     aList.emplace_back( _( "Type" ), _( "Value" ) );     break;
        case TEXT_is_DIVERS:    aList.emplace_back( _( "Type" ), _( "Text" ) );      break;
        }
    }

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    if( parentFP )
        aList.emplace_back( _( "Display" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    aList.emplace_back( _( "Angle" ), wxString::Format( wxT( "%g" ), GetTextAngle().AsDegrees() ) );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );
}


int PCB_TEXT::getKnockoutMargin() const
{
    VECTOR2I textSize( GetTextWidth(), GetTextHeight() );
    int      thickness = GetTextThickness();

    return thickness * 1.5 + GetKnockoutTextMargin( textSize, thickness );
}


void PCB_TEXT::KeepUpright( const EDA_ANGLE& aOldOrientation, const EDA_ANGLE& aNewOrientation )
{
    if( !IsKeepUpright() )
        return;

    EDA_ANGLE newAngle = GetTextAngle() + aNewOrientation;
    newAngle.Normalize();

    bool needsFlipped = newAngle >= ANGLE_180;

    if( needsFlipped )
    {
        SetHorizJustify( static_cast<GR_TEXT_H_ALIGN_T>( -GetHorizJustify() ) );
        SetTextAngle( GetTextAngle() + ANGLE_180 );
    }
}


const BOX2I PCB_TEXT::GetBoundingBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     rect = GetTextBox();

    if( IsKnockout() )
        rect.Inflate( getKnockoutMargin() );

    if( !angle.IsZero() )
        rect = rect.GetBoundingBoxRotated( GetTextPos(), GetTextAngle() );

    return rect;
}


bool PCB_TEXT::TextHitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    if( IsKnockout() )
    {
        SHAPE_POLY_SET poly;

        TransformBoundingBoxToPolygon( &poly, getKnockoutMargin());

        return poly.Collide( aPoint, aAccuracy );
    }
    else
    {
        return EDA_TEXT::TextHitTest( aPoint, aAccuracy );
    }
}


bool PCB_TEXT::TextHitTest( const BOX2I& aRect, bool aContains, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


void PCB_TEXT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    SetTextPos( pt );

    EDA_ANGLE new_angle = GetTextAngle() + aAngle;
    new_angle.Normalize180();
    SetTextAngle( new_angle );
}


void PCB_TEXT::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position and justification are mirrored, but not the text itself

    if( aMirrorAroundXAxis )
    {
        if( GetTextAngle() == ANGLE_VERTICAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextY( MIRRORVAL( GetTextPos().y, aCentre.y ) );
    }
    else
    {
        if( GetTextAngle() == ANGLE_HORIZONTAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextX( MIRRORVAL( GetTextPos().x, aCentre.x ) );
    }
}


void PCB_TEXT::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
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

    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        SetMirrored( !IsMirrored() );
}


wxString PCB_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    if( FOOTPRINT* parentFP = GetParentFootprint() )
    {
        switch( m_type )
        {
        case TEXT_is_REFERENCE:
            return wxString::Format( _( "Reference '%s'" ),
                                     parentFP->GetReference() );

        case TEXT_is_VALUE:
            return wxString::Format( _( "Value '%s' of %s" ),
                                     KIUI::EllipsizeMenuText( GetText() ),
                                     parentFP->GetReference() );

        case TEXT_is_DIVERS:
            return wxString::Format( _( "Footprint Text '%s' of %s" ),
                                     KIUI::EllipsizeMenuText( GetText() ),
                                     parentFP->GetReference() );
        }
    }
    else
    {
        return wxString::Format( _( "PCB Text '%s' on %s"),
                                 KIUI::EllipsizeMenuText( GetText() ),
                                 GetLayerName() );
    }

    // Can't get here, but gcc doesn't seem to know that....
    return wxEmptyString;
}


BITMAPS PCB_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


EDA_ITEM* PCB_TEXT::Clone() const
{
    return new PCB_TEXT( *this );
}


void PCB_TEXT::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TEXT_T );

    std::swap( *((PCB_TEXT*) this), *((PCB_TEXT*) aImage) );
}


std::shared_ptr<SHAPE> PCB_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( IsKnockout() )
    {
        SHAPE_POLY_SET knockouts;

        TransformTextToPolySet( knockouts, aLayer, 0, GetBoard()->GetDesignSettings().m_MaxError,
                                ERROR_INSIDE );

        SHAPE_POLY_SET finalPoly;
        int            strokeWidth = GetEffectiveTextPenWidth();
        VECTOR2I       fontSize = GetTextSize();
        int            margin = strokeWidth * 1.5 + GetKnockoutTextMargin( fontSize, strokeWidth );

        TransformBoundingBoxToPolygon( &finalPoly, margin );
        finalPoly.BooleanSubtract( knockouts, SHAPE_POLY_SET::PM_FAST );
        finalPoly.Fracture( SHAPE_POLY_SET::PM_FAST );

        return std::make_shared<SHAPE_POLY_SET>( finalPoly );
    }

    return GetEffectiveTextShape();
}


void PCB_TEXT::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                       int aClearance, int aError, ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();
    TEXT_ATTRIBUTES            attrs = GetAttributes();

    attrs.m_Angle = GetDrawRotation();

    // The polygonal shape of a text can have many basic shapes, so combining these shapes can
    // be very useful to create a final shape with a lot less vertices to speedup calculations.
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET buffer;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( buffer, aPt1, aPt2, penWidth + ( 2 * aClearance ),
                                        aError, ERROR_INSIDE );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                buffer.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    buffer.Append( point.x, point.y );
            } );

    font->Draw( &callback_gal, GetShownText( true ), GetTextPos(), attrs );
    buffer.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;
        int            margin = attrs.m_StrokeWidth * 1.5
                                    + GetKnockoutTextMargin( attrs.m_Size, attrs.m_StrokeWidth );

        TransformBoundingBoxToPolygon( &finalPoly, margin );
        finalPoly.BooleanSubtract( buffer, SHAPE_POLY_SET::PM_FAST );

        aBuffer.Append( finalPoly );
    }
    else
    {
        aBuffer.Append( buffer );
    }
}


void PCB_TEXT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                        int aClearance, int aError, ERROR_LOC aErrorLoc,
                                        bool aIgnoreLineWidth ) const
{
    EDA_TEXT::TransformBoundingBoxToPolygon( &aBuffer, aClearance );
}


static struct PCB_TEXT_DESC
{
    PCB_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, BOARD_ITEM>( _HKI( "Knockout" ),
                &BOARD_ITEM::SetIsKnockout, &BOARD_ITEM::IsKnockout ),
                _HKI( "Text Properties" ) );

        propMgr.AddProperty( new PROPERTY<PCB_TEXT, bool, EDA_TEXT>( _HKI( "Keep Upright" ),
                &PCB_TEXT::SetKeepUpright, &PCB_TEXT::IsKeepUpright ),
                _HKI( "Text Properties" ) );

        auto isFootprintText =
                []( INSPECTABLE* aItem ) -> bool
                {
                    if( PCB_TEXT* text = dynamic_cast<PCB_TEXT*>( aItem ) )
                        return text->GetParentFootprint();

                    return false;
                };

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Visible" ), isFootprintText );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Keep Upright" ), isFootprintText );

        propMgr.OverrideAvailability( TYPE_HASH( PCB_TEXT ), TYPE_HASH( EDA_TEXT ),
                                      _HKI( "Hyperlink" ),
                                      []( INSPECTABLE* aItem ) { return false; } );
    }
} _PCB_TEXT_DESC;

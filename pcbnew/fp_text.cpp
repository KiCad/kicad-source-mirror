/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
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
#include <settings/settings_manager.h>
#include <trigo.h>
#include <string_utils.h>
#include <painter.h>
#include <geometry/shape_compound.h>
#include <callback_gal.h>
#include <convert_basic_shapes_to_polygon.h>

FP_TEXT::FP_TEXT( FOOTPRINT* aParentFootprint, TEXT_TYPE text_type ) :
    BOARD_ITEM( aParentFootprint, PCB_FP_TEXT_T ),
    EDA_TEXT( pcbIUScale )
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );

    m_Type = text_type;
    SetKeepUpright( true );

    // Set text thickness to a default value
    SetTextThickness( pcbIUScale.mmToIU( DEFAULT_TEXT_WIDTH ) );
    SetLayer( F_SilkS );

    // Set position and give a default layer if a valid parent footprint exists
    if( parentFootprint && parentFootprint->Type() == PCB_FOOTPRINT_T )
    {
        SetTextPos( parentFootprint->GetPosition() );

        if( IsBackLayer( parentFootprint->GetLayer() ) )
        {
            SetLayer( B_SilkS );
            SetMirrored( true );
        }
    }

    SetDrawCoord();
}


FP_TEXT::~FP_TEXT()
{
}


bool FP_TEXT::TextHitTest( const VECTOR2I& aPoint, int aAccuracy ) const
{
    BOX2I    rect = GetTextBox();
    VECTOR2I location = aPoint;

    rect.Inflate( aAccuracy );

    RotatePoint( location, GetTextPos(), -GetDrawRotation() );

    return rect.Contains( location );
}


bool FP_TEXT::TextHitTest( const BOX2I& aRect, bool aContains, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContains )
        return rect.Contains( GetBoundingBox() );
    else
        return rect.Intersects( GetTextBox(), GetDrawRotation() );
}


void FP_TEXT::KeepUpright( const EDA_ANGLE& aOldOrientation, const EDA_ANGLE& aNewOrientation )
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
        SetDrawCoord();
    }
}


void FP_TEXT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    // Used in footprint editing
    // Note also in footprint editor, m_Pos0 = m_Pos

    VECTOR2I pt = GetTextPos();
    RotatePoint( pt, aRotCentre, aAngle );
    SetTextPos( pt );

    EDA_ANGLE new_angle = GetTextAngle() + aAngle;
    new_angle.Normalize180();
    SetTextAngle( new_angle );

    SetLocalCoord();
}


void FP_TEXT::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
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

    if( ( GetLayerSet() & LSET::SideSpecificMask() ).any() )
        SetMirrored( !IsMirrored() );

    SetLocalCoord();
}

bool FP_TEXT::IsParentFlipped() const
{
    if( GetParent() &&  GetParent()->GetLayer() == B_Cu )
        return true;
    return false;
}


void FP_TEXT::Mirror( const VECTOR2I& aCentre, bool aMirrorAroundXAxis )
{
    // the position and justification are mirrored, but not the text itself

    if( aMirrorAroundXAxis )
    {
        if( GetTextAngle() == ANGLE_VERTICAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextY( ::MIRRORVAL( GetTextPos().y, aCentre.y ) );
    }
    else
    {
        if( GetTextAngle() == ANGLE_HORIZONTAL )
            SetHorizJustify( (GR_TEXT_H_ALIGN_T) -GetHorizJustify() );

        SetTextX( ::MIRRORVAL( GetTextPos().x, aCentre.x ) );
    }

    SetLocalCoord();
}


void FP_TEXT::Move( const VECTOR2I& aMoveVector )
{
    Offset( aMoveVector );
    SetLocalCoord();
}


int FP_TEXT::GetLength() const
{
    return GetText().Len();
}


void FP_TEXT::SetDrawCoord()
{
    const FOOTPRINT* parentFootprint = static_cast<const FOOTPRINT*>( m_parent );

    SetTextPos( m_Pos0 );

    if( parentFootprint  )
    {
        VECTOR2I pt = GetTextPos();
        RotatePoint( pt, parentFootprint->GetOrientation() );
        SetTextPos( pt );

        Offset( parentFootprint->GetPosition() );
    }
}


void FP_TEXT::SetLocalCoord()
{
    const FOOTPRINT* parentFootprint = static_cast<const FOOTPRINT*>( m_parent );

    if( parentFootprint )
    {
        m_Pos0 = GetTextPos() - parentFootprint->GetPosition();
        RotatePoint( &m_Pos0.x, &m_Pos0.y, - parentFootprint->GetOrientation() );
    }
    else
    {
        m_Pos0 = GetTextPos();
    }
}

const BOX2I FP_TEXT::GetBoundingBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     bbox = GetTextBox();

    if( !angle.IsZero() )
        bbox = bbox.GetBoundingBoxRotated( GetTextPos(), angle );

    return bbox;
}


EDA_ANGLE FP_TEXT::GetDrawRotation() const
{
    FOOTPRINT* parentFootprint = static_cast<FOOTPRINT*>( m_parent );
    EDA_ANGLE  rotation = GetTextAngle();

    if( parentFootprint )
        rotation += parentFootprint->GetOrientation();

    if( IsKeepUpright() )
    {
        // Keep angle between ]-90 .. 90 deg]. Otherwise the text is not easy to read
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


void FP_TEXT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    wxString msg;

    static const wxString text_type_msg[3] =
    {
        _( "Reference" ), _( "Value" ), _( "Text" )
    };

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME )
    {
        FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent );

        if( fp )
            aList.emplace_back( _( "Footprint" ), fp->GetReference() );
    }

    // Don't use GetShownText() here; we want to show the user the variable references
    aList.emplace_back( _( "Text" ), UnescapeString( GetText() ) );

    wxASSERT( m_Type >= TEXT_is_REFERENCE && m_Type <= TEXT_is_DIVERS );
    aList.emplace_back( _( "Type" ), text_type_msg[m_Type] );

    if( aFrame->GetName() == PCB_EDIT_FRAME_NAME && IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    aList.emplace_back( _( "Display" ), IsVisible() ? _( "Yes" ) : _( "No" ) );

    // Display text layer
    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Mirror" ), IsMirrored() ? _( "Yes" ) : _( "No" ) );

    msg.Printf( wxT( "%g" ), GetTextAngle().AsDegrees() );
    aList.emplace_back( _( "Angle" ), msg );

    aList.emplace_back( _( "Font" ), GetFont() ? GetFont()->GetName() : _( "Default" ) );
    aList.emplace_back( _( "Thickness" ), aFrame->MessageTextFromValue( GetTextThickness() ) );
    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetTextWidth() ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetTextHeight() ) );
}


wxString FP_TEXT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider ) const
{
    switch( m_Type )
    {
    case TEXT_is_REFERENCE:
        return wxString::Format( _( "Reference '%s'" ),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );

    case TEXT_is_VALUE:
        return wxString::Format( _( "Value '%s' of %s" ),
                                 GetShownText( false ),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );

    default:
        return wxString::Format( _( "Footprint Text '%s' of %s" ),
                                 KIUI::EllipsizeMenuText( GetShownText( false ) ),
                                 static_cast<FOOTPRINT*>( GetParent() )->GetReference() );
    }
}


BITMAPS FP_TEXT::GetMenuImage() const
{
    return BITMAPS::text;
}


EDA_ITEM* FP_TEXT::Clone() const
{
    return new FP_TEXT( *this );
}


const BOX2I FP_TEXT::ViewBBox() const
{
    EDA_ANGLE angle = GetDrawRotation();
    BOX2I     text_area = GetTextBox();

    if( !angle.IsZero() )
        text_area = text_area.GetBoundingBoxRotated( GetTextPos(), angle );

    return BOX2I( text_area.GetPosition(), text_area.GetSize() );
}


void FP_TEXT::ViewGetLayers( int aLayers[], int& aCount ) const
{
    if( IsVisible() )
        aLayers[0] = GetLayer();
    else
        aLayers[0] = LAYER_MOD_TEXT_INVISIBLE;

    aCount = 1;
}


double FP_TEXT::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = (double)std::numeric_limits<double>::max();

    if( !aView )
        return 0.0;

    // Hidden text gets put on the LAYER_MOD_TEXT_INVISIBLE for rendering, but
    // should only render if its native layer is visible.
    if( !aView->IsLayerVisible( GetLayer() ) )
        return HIDE;

    // Handle Render tab switches
    if( m_Type == TEXT_is_VALUE || GetText() == wxT( "${VALUE}" ) )
    {
        if( !aView->IsLayerVisible( LAYER_MOD_VALUES ) )
        {
            return HIDE;
        }
    }

    if( m_Type == TEXT_is_REFERENCE || GetText() == wxT( "${REFERENCE}" ) )
    {
        if( !aView->IsLayerVisible( LAYER_MOD_REFERENCES ) )
        {
            return HIDE;
        }
    }

    if( !IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_FR ) )
        return HIDE;

    if( IsParentFlipped() && !aView->IsLayerVisible( LAYER_MOD_BK ) )
        return HIDE;

    if( !aView->IsLayerVisible( LAYER_MOD_TEXT ) )
        return HIDE;

    // Other layers are shown without any conditions
    return 0.0;
}


wxString FP_TEXT::GetShownText( bool aAllowExtraText, int aDepth ) const
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

    return text;
}


std::shared_ptr<SHAPE> FP_TEXT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    if( IsKnockout() )
    {
        SHAPE_POLY_SET poly;

        TransformTextToPolySet( poly, 0, GetBoard()->GetDesignSettings().m_MaxError, ERROR_INSIDE );

        return std::make_shared<SHAPE_POLY_SET>( poly );
    }

    return GetEffectiveTextShape();
}


void FP_TEXT::buildBoundingHull( SHAPE_POLY_SET* aBuffer, const SHAPE_POLY_SET& aRenderedText,
                                 int aClearance ) const
{
    SHAPE_POLY_SET poly( aRenderedText );

    poly.Rotate( -GetDrawRotation(), GetDrawPos() );

    BOX2I    rect = poly.BBox( aClearance );
    VECTOR2I corners[4];

    corners[0].x = rect.GetOrigin().x;
    corners[0].y = rect.GetOrigin().y;
    corners[1].y = corners[0].y;
    corners[1].x = rect.GetRight();
    corners[2].x = corners[1].x;
    corners[2].y = rect.GetBottom();
    corners[3].y = corners[2].y;
    corners[3].x = corners[0].x;

    aBuffer->NewOutline();

    for( VECTOR2I& corner : corners )
    {
        RotatePoint( corner, GetDrawPos(), GetDrawRotation() );
        aBuffer->Append( corner.x, corner.y );
    }
}


void FP_TEXT::TransformTextToPolySet( SHAPE_POLY_SET& aBuffer, int aClearance, int aError,
                                      ERROR_LOC aErrorLoc ) const
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    KIFONT::FONT*              font = getDrawFont();
    int                        penWidth = GetEffectiveTextPenWidth();

    // The polygonal shape of a text can have many basic shapes, so combining these shapes can
    // be very useful to create a final shape with a lot less vertices to speedup calculations.
    // Simplify shapes is not usually always efficient, but in this case it is.
    SHAPE_POLY_SET textShape;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                TransformOvalToPolygon( textShape, aPt1, aPt2, penWidth, aError, aErrorLoc );
            },
            // Triangulation callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2, const VECTOR2I& aPt3 )
            {
                textShape.NewOutline();

                for( const VECTOR2I& point : { aPt1, aPt2, aPt3 } )
                    textShape.Append( point.x, point.y );
            } );

    TEXT_ATTRIBUTES attrs = GetAttributes();
    attrs.m_Angle = GetDrawRotation();

    font->Draw( &callback_gal, GetShownText( true ), GetTextPos(), attrs );

    textShape.Simplify( SHAPE_POLY_SET::PM_FAST );

    if( IsKnockout() )
    {
        SHAPE_POLY_SET finalPoly;
        int            margin = GetKnockoutTextMargin( attrs.m_Size, penWidth );

        buildBoundingHull( &finalPoly, textShape, margin + aClearance );
        finalPoly.BooleanSubtract( textShape, SHAPE_POLY_SET::PM_FAST );

        aBuffer.Append( finalPoly );
    }
    else
    {
        aBuffer.Append( textShape );
    }
}


void FP_TEXT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer, int aClearance,
                                       int aError, ERROR_LOC aErrorLoc, bool aIgnoreLineWidth ) const
{
    SHAPE_POLY_SET poly;

    TransformTextToPolySet( poly, 0, GetBoard()->GetDesignSettings().m_MaxError, ERROR_INSIDE );

    buildBoundingHull( &aBuffer, poly, aClearance );
}


wxString FP_TEXT::GetParentAsString() const
{
    if( FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( m_parent ) )
        return fp->GetReference();

    return m_parent->m_Uuid.AsString();
}


static struct FP_TEXT_DESC
{
    FP_TEXT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( FP_TEXT );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXT, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<FP_TEXT, EDA_TEXT> );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXT ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( FP_TEXT ), TYPE_HASH( EDA_TEXT ) );

        propMgr.AddProperty( new PROPERTY<FP_TEXT, wxString>( _HKI( "Parent" ),
                    NO_SETTER( FP_TEXT, wxString ), &FP_TEXT::GetParentAsString ) );
    }
} _FP_TEXT_DESC;

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2022 Mike Williams
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

#include "pcb_reference_image.h"

#include <api/api_enums.h>
#include <api/api_utils.h>
#include <api/board/board_types.pb.h>
#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <common.h>
#include <core/mirror.h>
#include <eda_draw_frame.h>
#include <footprint.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_painter.h>
#include <plotters/plotter.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_rect.h>
#include <settings/color_settings.h>
#include <trigo.h>

#include <string>
#include <wx/mstream.h>
#include <google/protobuf/any.pb.h>
#include <properties/property.h>
#include <properties/property_mgr.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;


PCB_REFERENCE_IMAGE::PCB_REFERENCE_IMAGE( BOARD_ITEM* aParent, const VECTOR2I& aPos,
                                          PCB_LAYER_ID aLayer ) :
        BOARD_ITEM( aParent, PCB_REFERENCE_IMAGE_T, aLayer ), m_referenceImage( pcbIUScale )
{
    m_referenceImage.SetPosition( aPos );
}


PCB_REFERENCE_IMAGE::PCB_REFERENCE_IMAGE( const PCB_REFERENCE_IMAGE& aPCBBitmap ) :
        BOARD_ITEM( aPCBBitmap ), m_referenceImage( aPCBBitmap.m_referenceImage )
{
}


PCB_REFERENCE_IMAGE::~PCB_REFERENCE_IMAGE()
{
}


PCB_REFERENCE_IMAGE& PCB_REFERENCE_IMAGE::operator=( const BOARD_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " )
                         + GetClass() );

    if( &aItem != this )
    {
        BOARD_ITEM::operator=( aItem );
        const PCB_REFERENCE_IMAGE& refImg = static_cast<const PCB_REFERENCE_IMAGE&>( aItem );
        m_referenceImage = refImg.m_referenceImage;
    }

    return *this;
}


void PCB_REFERENCE_IMAGE::CopyFrom( const BOARD_ITEM* aOther )
{
    wxCHECK( aOther && aOther->Type() == PCB_REFERENCE_IMAGE_T, /* void */ );
    *this = *static_cast<const PCB_REFERENCE_IMAGE*>( aOther );
}


EDA_ITEM* PCB_REFERENCE_IMAGE::Clone() const
{
    return new PCB_REFERENCE_IMAGE( *this );
}


void PCB_REFERENCE_IMAGE::swapData( BOARD_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == PCB_REFERENCE_IMAGE_T,
                 wxString::Format( wxT( "% object cannot swap data with %s object." ),
                                   GetClass(), aItem->GetClass() ) );

    PCB_REFERENCE_IMAGE* item = (PCB_REFERENCE_IMAGE*) aItem;
    std::swap( m_layer, item->m_layer );
    std::swap( m_isKnockout, item->m_isKnockout );
    std::swap( m_isLocked, item->m_isLocked );
    std::swap( m_flags, item->m_flags );
    std::swap( m_parent, item->m_parent );
    std::swap( m_forceVisible, item->m_forceVisible );
    m_referenceImage.SwapData( item->m_referenceImage );
}


double PCB_REFERENCE_IMAGE::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    // All bitmaps are drawn on LAYER_DRAW_BITMAPS, but their
    // associated board layer controls their visibility.
    if( !GetBoard()->IsLayerVisible( m_layer ) )
        return LOD_HIDE;

    if( renderSettings->GetHighContrast()
        && renderSettings->m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN
        && !renderSettings->GetLayerIsHighContrast( m_layer ) )
    {
        return LOD_HIDE;
    }

    if( aView->IsLayerVisible( LAYER_DRAW_BITMAPS ) )
        return LOD_SHOW;

    return LOD_HIDE;
}


const BOX2I PCB_REFERENCE_IMAGE::GetBoundingBox() const
{
    return m_referenceImage.GetBoundingBox();
}


std::shared_ptr<SHAPE> PCB_REFERENCE_IMAGE::GetEffectiveShape( PCB_LAYER_ID aLayer,
                                                               FLASHING aFlash ) const
{
    const BOX2I box = GetBoundingBox();
    return std::make_shared<SHAPE_RECT>( box.GetPosition(), box.GetWidth(), box.GetHeight() );
}


VECTOR2I PCB_REFERENCE_IMAGE::GetPosition() const
{
    return m_referenceImage.GetPosition();
}


void PCB_REFERENCE_IMAGE::SetPosition( const VECTOR2I& aPos )
{
    m_referenceImage.SetPosition( aPos );
}


void PCB_REFERENCE_IMAGE::Move( const VECTOR2I& aMoveVector )
{
    // Defer to SetPosition to check the new position overflow
    SetPosition( GetPosition() + aMoveVector );
}


void PCB_REFERENCE_IMAGE::Serialize( google::protobuf::Any& aContainer ) const
{
    using namespace kiapi::board::types;

    ReferenceImage refImage;

    refImage.mutable_id()->set_value( m_Uuid.AsStdString() );
    refImage.set_layer( ToProtoEnum<PCB_LAYER_ID, BoardLayer>( m_layer ) );
    kiapi::common::PackVector2( *refImage.mutable_position(), m_referenceImage.GetPosition() );
    kiapi::common::PackVector2( *refImage.mutable_transform_origin_offset(),
                                m_referenceImage.GetTransformOriginOffset() );

    refImage.mutable_image_scale()->set_value( m_referenceImage.GetImageScale() );
    refImage.set_locked( IsLocked() ? kiapi::common::types::LockedState::LS_LOCKED
                                    : kiapi::common::types::LockedState::LS_UNLOCKED );

    wxMemoryOutputStream imageStream;

    if( m_referenceImage.GetImage().GetImageData()
        && m_referenceImage.GetImage().SaveImageData( imageStream ) )
    {
        size_t size = imageStream.GetSize();

        if( size > 0 )
        {
            std::string encoded;
            encoded.resize( size );
            imageStream.CopyTo( encoded.data(), size );
            refImage.set_image_data( encoded );
        }
    }

    aContainer.PackFrom( refImage );
}


bool PCB_REFERENCE_IMAGE::Deserialize( const google::protobuf::Any& aContainer )
{
    using namespace kiapi::board::types;

    ReferenceImage refImage;

    if( !aContainer.UnpackTo( &refImage ) )
        return false;

    const_cast<KIID&>( m_Uuid ) = KIID( refImage.id().value() );
    SetLayer( FromProtoEnum<PCB_LAYER_ID, BoardLayer>( refImage.layer() ) );
    SetPosition( kiapi::common::UnpackVector2( refImage.position() ) );
    m_referenceImage.SetTransformOriginOffset( kiapi::common::UnpackVector2( refImage.transform_origin_offset() ) );

    if( !refImage.image_data().empty() )
    {
        wxMemoryBuffer imageBuffer;
        imageBuffer.AppendData( refImage.image_data().data(), refImage.image_data().size() );

        if( !m_referenceImage.ReadImageFile( imageBuffer ) )
            return false;
    }

    if( refImage.has_image_scale() )
        m_referenceImage.SetImageScale( refImage.image_scale().value() );

    SetLocked( refImage.locked() == kiapi::common::types::LockedState::LS_LOCKED );
    return true;
}


void PCB_REFERENCE_IMAGE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    m_referenceImage.Flip( aCentre, aFlipDirection );

    if( GetBoard() )
        SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
    else
        SetLayer( FlipLayer( GetLayer() ) );
}


void PCB_REFERENCE_IMAGE::Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle )
{
    m_referenceImage.Rotate( aCenter, aAngle );
}


#if defined( DEBUG )
void PCB_REFERENCE_IMAGE::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_referenceImage.GetPosition()
                                 << "/>\n";
}
#endif


bool PCB_REFERENCE_IMAGE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    return KIGEOM::BoxHitTest( aPosition, GetBoundingBox(), aAccuracy );
}


bool PCB_REFERENCE_IMAGE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    return KIGEOM::BoxHitTest( aRect, GetBoundingBox(), aContained, aAccuracy );
}


bool PCB_REFERENCE_IMAGE::HitTest( const SHAPE_LINE_CHAIN& aPoly, bool aContained ) const
{
    return KIGEOM::BoxHitTest( aPoly, GetBoundingBox(), aContained );
}


BITMAPS PCB_REFERENCE_IMAGE::GetMenuImage() const
{
    return BITMAPS::image;
}


void PCB_REFERENCE_IMAGE::GetMsgPanelInfo( EDA_DRAW_FRAME*              aFrame,
                                           std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Reference Image" ), wxEmptyString );

    aList.emplace_back( _( "PPI" ),
                        wxString::Format( wxT( "%d " ), m_referenceImage.GetImage().GetPPI() ) );
    aList.emplace_back( _( "Scale" ),
                        wxString::Format( wxT( "%f " ), m_referenceImage.GetImageScale() ) );

    aList.emplace_back( _( "Width" ),
                        aFrame->MessageTextFromValue( m_referenceImage.GetSize().x ) );
    aList.emplace_back( _( "Height" ),
                        aFrame->MessageTextFromValue( m_referenceImage.GetSize().y ) );
    aList.emplace_back( _( "Layer" ), LayerName( m_layer ) );
}


std::vector<int> PCB_REFERENCE_IMAGE::ViewGetLayers() const
{
    return { BITMAP_LAYER_FOR( m_layer ) };
}


bool PCB_REFERENCE_IMAGE::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_REFERENCE_IMAGE& other = static_cast<const PCB_REFERENCE_IMAGE&>( aBoardItem );

    return *this == other;
}


bool PCB_REFERENCE_IMAGE::operator==( const PCB_REFERENCE_IMAGE& aOther ) const
{
    if( m_layer != aOther.m_layer )
        return false;

    if( m_referenceImage != aOther.m_referenceImage )
        return false;

    return true;
}


double PCB_REFERENCE_IMAGE::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_REFERENCE_IMAGE& other = static_cast<const PCB_REFERENCE_IMAGE&>( aOther );

    double similarity = 1.0;

    if( m_layer != other.m_layer )
        similarity *= 0.9;

    similarity *= m_referenceImage.Similarity( other.m_referenceImage );

    return similarity;
}


int PCB_REFERENCE_IMAGE::GetTransformOriginOffsetX() const
{
    return m_referenceImage.GetTransformOriginOffset().x;
}


void PCB_REFERENCE_IMAGE::SetTransformOriginOffsetX( int aX )
{
    VECTOR2I offset = m_referenceImage.GetTransformOriginOffset();
    offset.x = aX;
    m_referenceImage.SetTransformOriginOffset( offset );
}


int PCB_REFERENCE_IMAGE::GetTransformOriginOffsetY() const
{
    return m_referenceImage.GetTransformOriginOffset().y;
}


void PCB_REFERENCE_IMAGE::SetTransformOriginOffsetY( int aY )
{
    VECTOR2I offset = m_referenceImage.GetTransformOriginOffset();
    offset.y = aY;
    m_referenceImage.SetTransformOriginOffset( offset );
}


double PCB_REFERENCE_IMAGE::GetImageScale() const
{
    return m_referenceImage.GetImageScale();
}


void PCB_REFERENCE_IMAGE::SetImageScale( double aScale )
{
    m_referenceImage.SetImageScale( aScale );
}


int PCB_REFERENCE_IMAGE::GetWidth() const
{
    return m_referenceImage.GetImage().GetSize().x;
}


void PCB_REFERENCE_IMAGE::SetWidth( int aWidth )
{
    m_referenceImage.SetWidth( aWidth );
}


int PCB_REFERENCE_IMAGE::GetHeight() const
{
    return m_referenceImage.GetImage().GetSize().y;
}


void PCB_REFERENCE_IMAGE::SetHeight( int aHeight )
{
    m_referenceImage.SetHeight( aHeight );
}


static struct PCB_REFERENCE_IMAGE_DESC
{
    PCB_REFERENCE_IMAGE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_REFERENCE_IMAGE );
        propMgr.InheritsAfter( TYPE_HASH( PCB_REFERENCE_IMAGE ), TYPE_HASH( BOARD_ITEM ) );

        propMgr.ReplaceProperty( TYPE_HASH( BOARD_ITEM ), _HKI( "Layer" ),
            new PROPERTY_ENUM<PCB_REFERENCE_IMAGE, PCB_LAYER_ID, BOARD_ITEM>( _HKI( "Associated Layer" ),
            &PCB_REFERENCE_IMAGE::SetLayer, &PCB_REFERENCE_IMAGE::GetLayer ) );

        const wxString groupImage = _HKI( "Image Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, double>( _HKI( "Scale" ),
                             &PCB_REFERENCE_IMAGE::SetImageScale,
                             &PCB_REFERENCE_IMAGE::GetImageScale ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>( _HKI( "Transform Offset X" ),
                                     &PCB_REFERENCE_IMAGE::SetTransformOriginOffsetX,
                                     &PCB_REFERENCE_IMAGE::GetTransformOriginOffsetX,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>( _HKI( "Transform Offset Y" ),
                                     &PCB_REFERENCE_IMAGE::SetTransformOriginOffsetY,
                                     &PCB_REFERENCE_IMAGE::GetTransformOriginOffsetY,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>( _HKI( "Width" ),
                                     &PCB_REFERENCE_IMAGE::SetWidth, &PCB_REFERENCE_IMAGE::GetWidth,
                                     PROPERTY_DISPLAY::PT_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>( _HKI( "Height" ),
                                     &PCB_REFERENCE_IMAGE::SetHeight, &PCB_REFERENCE_IMAGE::GetHeight,
                                     PROPERTY_DISPLAY::PT_COORD ),
                             groupImage );

        // For future use
        const wxString greyscale = _HKI( "Greyscale" );
    }
} _PCB_REFERENCE_IMAGE_DESC;

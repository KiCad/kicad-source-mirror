/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2022 Mike Williams
 * Copyright (C) 2011-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_draw_panel_gal.h>
#include <plotters/plotter.h>
#include <settings/color_settings.h>
#include <pcb_painter.h>
#include <bitmaps.h>
#include <base_units.h>
#include <common.h>
#include <eda_draw_frame.h>
#include <core/mirror.h>
#include <board.h>
#include <trigo.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_rect.h>

#include <wx/mstream.h>

using KIGFX::PCB_PAINTER;
using KIGFX::PCB_RENDER_SETTINGS;


PCB_REFERENCE_IMAGE::PCB_REFERENCE_IMAGE( BOARD_ITEM* aParent, const VECTOR2I& aPos,
                                          PCB_LAYER_ID aLayer ) :
        BOARD_ITEM( aParent, PCB_REFERENCE_IMAGE_T, aLayer ), m_pos( aPos ),
        m_transformOriginOffset( 0, 0 )
{
    m_bitmapBase = new BITMAP_BASE();
    m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


PCB_REFERENCE_IMAGE::PCB_REFERENCE_IMAGE( const PCB_REFERENCE_IMAGE& aPCBBitmap ) :
        BOARD_ITEM( aPCBBitmap ), m_pos( aPCBBitmap.m_pos ),
        m_transformOriginOffset( aPCBBitmap.m_transformOriginOffset )
{
    m_bitmapBase = new BITMAP_BASE( *aPCBBitmap.m_bitmapBase );
    m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


PCB_REFERENCE_IMAGE::~PCB_REFERENCE_IMAGE()
{
    delete m_bitmapBase;
}


PCB_REFERENCE_IMAGE& PCB_REFERENCE_IMAGE::operator=( const BOARD_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " )
                         + GetClass() );

    if( &aItem != this )
    {
        BOARD_ITEM::operator=( aItem );

        PCB_REFERENCE_IMAGE* bitmap = (PCB_REFERENCE_IMAGE*) &aItem;

        delete m_bitmapBase;
        m_bitmapBase = new BITMAP_BASE( *bitmap->m_bitmapBase );
        m_pos = bitmap->m_pos;
        m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
    }

    return *this;
}


bool PCB_REFERENCE_IMAGE::ReadImageFile( const wxString& aFullFilename )
{
    if( m_bitmapBase->ReadImageFile( aFullFilename ) )
    {
        m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
        return true;
    }

    return false;
}


bool PCB_REFERENCE_IMAGE::ReadImageFile( wxMemoryBuffer& aBuffer )
{
    if( m_bitmapBase->ReadImageFile( aBuffer ) )
    {
        m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
        return true;
    }

    return false;
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
    std::swap( m_pos, item->m_pos );
    std::swap( m_bitmapBase, item->m_bitmapBase );
}


double PCB_REFERENCE_IMAGE::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    PCB_PAINTER*         painter = static_cast<PCB_PAINTER*>( aView->GetPainter() );
    PCB_RENDER_SETTINGS* renderSettings = painter->GetSettings();

    // All bitmaps are drawn on LAYER_DRAW_BITMAPS, but their
    // associated board layer controls their visibility.
    if( !GetBoard()->IsLayerVisible( m_layer ) )
        return HIDE;

    if( renderSettings->GetHighContrast()
        && renderSettings->m_ContrastModeDisplay == HIGH_CONTRAST_MODE::HIDDEN
        && !renderSettings->GetLayerIsHighContrast( m_layer ) )
    {
        return HIDE;
    }

    return aView->IsLayerVisible( LAYER_DRAW_BITMAPS ) ? 0.0 : HIDE;
}


const BOX2I PCB_REFERENCE_IMAGE::GetBoundingBox() const
{
    // Bitmaps are center origin, BOX2Is need top-left origin
    const VECTOR2I size = m_bitmapBase->GetSize();
    const VECTOR2I topLeft{ m_pos.x - size.x / 2, m_pos.y - size.y / 2 };

    return BOX2I{ topLeft, size };
}


std::shared_ptr<SHAPE> PCB_REFERENCE_IMAGE::GetEffectiveShape( PCB_LAYER_ID aLayer,
                                                               FLASHING aFlash ) const
{
    const BOX2I box = GetBoundingBox();
    return std::make_shared<SHAPE_RECT>( box.GetPosition(), box.GetWidth(), box.GetHeight() );
}


void PCB_REFERENCE_IMAGE::SetPosition( const VECTOR2I& aPos )
{
    const BOX2D newBox = BOX2D::ByCenter( aPos, m_bitmapBase->GetSize() );

    if( !IsBOX2Safe( newBox ) )
        return;

    m_pos = aPos;
}


void PCB_REFERENCE_IMAGE::Move( const VECTOR2I& aMoveVector )
{
    // Defer to SetPosition to check the new position overflow
    SetPosition( m_pos + aMoveVector );
}


void PCB_REFERENCE_IMAGE::SetImageScale( double aScale )
{
    if( aScale < 0 )
        return;

    const double ratio = aScale / m_bitmapBase->GetScale();

    const VECTOR2D currentOrigin = m_pos + m_transformOriginOffset;
    const VECTOR2D newOffset = m_transformOriginOffset * ratio;
    const VECTOR2D newCenter = currentOrigin - newOffset;
    const VECTOR2D newSize = m_bitmapBase->GetSize() * ratio;

    // The span of the image is limited to the size of the coordinate system
    if( !IsVec2SafeXY( newSize ) )
        return;

    const BOX2D newBox = BOX2D::ByCenter( newCenter, newSize );

    // Any overflow, just reject the call
    if( !IsBOX2Safe( newBox ) )
        return;

    m_bitmapBase->SetScale( aScale );
    SetTransformOriginOffset( KiROUND( newOffset ) );
    // Don't need to recheck the box, we just did that
    m_pos = KiROUND( newCenter );
}


const VECTOR2I PCB_REFERENCE_IMAGE::GetSize() const
{
    return m_bitmapBase->GetSize();
}


void PCB_REFERENCE_IMAGE::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    VECTOR2I newPos = m_pos;
    MIRROR( newPos, aCentre, aFlipDirection );

    const BOX2D newBox = BOX2D::ByCenter( newPos, m_bitmapBase->GetSize() );

    if( !IsBOX2Safe( newBox ) )
        return;

    m_pos = newPos;
    m_bitmapBase->Mirror( aFlipDirection );
}

void PCB_REFERENCE_IMAGE::Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE norm( aAngle.AsDegrees(), DEGREES_T );

    RotatePoint( m_pos, aCenter, aAngle );

    norm.Normalize();

    // each call to m_bitmapBase->Rotate() rotates 90 degrees CCW
    for( double ang = 45.0; ang < norm.AsDegrees(); ang += 90.0 )
        m_bitmapBase->Rotate( false );
}


#if defined( DEBUG )
void PCB_REFERENCE_IMAGE::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_pos << "/>\n";
}
#endif


bool PCB_REFERENCE_IMAGE::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_REFERENCE_IMAGE::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


BITMAPS PCB_REFERENCE_IMAGE::GetMenuImage() const
{
    return BITMAPS::image;
}


void PCB_REFERENCE_IMAGE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame,
                                           std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Reference Image" ), wxEmptyString );

    aList.emplace_back( _( "PPI" ), wxString::Format( wxT( "%d "), GetImage()->GetPPI() ) );
    aList.emplace_back( _( "Scale" ), wxString::Format( wxT( "%f "), GetImageScale() ) );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetSize().x ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetSize().y ) );
    aList.emplace_back( _( "Layer" ), LayerName( m_layer ) );
}


void PCB_REFERENCE_IMAGE::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = BITMAP_LAYER_FOR( m_layer );
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

    if( m_pos != aOther.m_pos )
        return false;

    if( m_bitmapBase->GetSize() != aOther.m_bitmapBase->GetSize() )
        return false;

    if( m_bitmapBase->GetPPI() != aOther.m_bitmapBase->GetPPI() )
        return false;

    if( m_bitmapBase->GetScale() != aOther.m_bitmapBase->GetScale() )
        return false;

    if( m_bitmapBase->GetImageID() != aOther.m_bitmapBase->GetImageID() )
        return false;

    if( m_bitmapBase->GetImageData() != aOther.m_bitmapBase->GetImageData() )
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

    if( m_pos != other.m_pos )
        similarity *= 0.9;

    if( m_bitmapBase->GetSize() != other.m_bitmapBase->GetSize() )
        similarity *= 0.9;

    if( m_bitmapBase->GetPPI() != other.m_bitmapBase->GetPPI() )
        similarity *= 0.9;

    if( m_bitmapBase->GetScale() != other.m_bitmapBase->GetScale() )
        similarity *= 0.9;

    if( m_bitmapBase->GetImageID() != other.m_bitmapBase->GetImageID() )
        similarity *= 0.9;

    if( m_bitmapBase->GetImageData() != other.m_bitmapBase->GetImageData() )
        similarity *= 0.9;

    return similarity;
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

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>(
                                     _HKI( "Transform Offset X" ),
                                     &PCB_REFERENCE_IMAGE::SetTransformOriginOffsetX,
                                     &PCB_REFERENCE_IMAGE::GetTransformOriginOffsetX,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_X_COORD ),
                             groupImage );

        propMgr.AddProperty( new PROPERTY<PCB_REFERENCE_IMAGE, int>(
                                     _HKI( "Transform Offset Y" ),
                                     &PCB_REFERENCE_IMAGE::SetTransformOriginOffsetY,
                                     &PCB_REFERENCE_IMAGE::GetTransformOriginOffsetY,
                                     PROPERTY_DISPLAY::PT_COORD, ORIGIN_TRANSFORMS::ABS_Y_COORD ),
                             groupImage );

        // For future use
        const wxString greyscale = _HKI( "Greyscale" );
    }
} _PCB_REFERENCE_IMAGE_DESC;

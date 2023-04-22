/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2011 jean-pierre.charras
 * Copyright (C) 2022 Mike Williams
 * Copyright (C) 2011-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file pcb_bitmap.cpp
 */

#include <pcb_draw_panel_gal.h>
#include <plotters/plotter.h>
#include <settings/color_settings.h>
#include <bitmaps.h>
#include <base_units.h>
#include <common.h>
#include <eda_draw_frame.h>
#include <core/mirror.h>
#include <board.h>
#include <pcb_bitmap.h>
#include <trigo.h>
#include <geometry/shape_rect.h>

#include <wx/mstream.h>


PCB_BITMAP::PCB_BITMAP( BOARD_ITEM* aParent, const VECTOR2I& pos, PCB_LAYER_ID aLayer ) :
        BOARD_ITEM( aParent, PCB_BITMAP_T, aLayer )
{
    m_pos = pos;
    m_bitmapBase = new BITMAP_BASE();
    m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


PCB_BITMAP::PCB_BITMAP( const PCB_BITMAP& aPCBBitmap ) : BOARD_ITEM( aPCBBitmap )
{
    m_pos = aPCBBitmap.m_pos;
    m_bitmapBase = new BITMAP_BASE( *aPCBBitmap.m_bitmapBase );
    m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


PCB_BITMAP& PCB_BITMAP::operator=( const BOARD_ITEM& aItem )
{
    wxCHECK_MSG( Type() == aItem.Type(), *this,
                 wxT( "Cannot assign object type " ) + aItem.GetClass() + wxT( " to type " )
                         + GetClass() );

    if( &aItem != this )
    {
        BOARD_ITEM::operator=( aItem );

        PCB_BITMAP* bitmap = (PCB_BITMAP*) &aItem;

        delete m_bitmapBase;
        m_bitmapBase = new BITMAP_BASE( *bitmap->m_bitmapBase );
        m_pos = bitmap->m_pos;
        m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
    }

    return *this;
}


void PCB_BITMAP::SetImage( wxImage* aImage )
{
    m_bitmapBase->SetImage( aImage );
    m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
}


bool PCB_BITMAP::ReadImageFile( const wxString& aFullFilename )
{
    if( m_bitmapBase->ReadImageFile( aFullFilename ) )
    {
        m_bitmapBase->SetPixelSizeIu( (float) pcbIUScale.MilsToIU( 1000 ) / m_bitmapBase->GetPPI() );
        return true;
    }

    return false;
}


EDA_ITEM* PCB_BITMAP::Clone() const
{
    return new PCB_BITMAP( *this );
}


void PCB_BITMAP::swapData( BOARD_ITEM* aItem )
{
    wxCHECK_RET( aItem->Type() == PCB_BITMAP_T,
                 wxString::Format( wxT( "PCB_BITMAP object cannot swap data with %s object." ),
                                   aItem->GetClass() ) );

    PCB_BITMAP* item = (PCB_BITMAP*) aItem;
    std::swap( m_layer, item->m_layer );
    std::swap( m_isKnockout, item->m_isKnockout );
    std::swap( m_isLocked, item->m_isLocked );
    std::swap( m_flags, item->m_flags );
    std::swap( m_parent, item->m_parent );
    std::swap( m_forceVisible, item->m_forceVisible );
    std::swap( m_pos, item->m_pos );
    std::swap( m_bitmapBase, item->m_bitmapBase );
}


double PCB_BITMAP::ViewGetLOD( int aLayer, KIGFX::VIEW* aView ) const
{
    constexpr double HIDE = std::numeric_limits<double>::max();

    // All bitmaps are drawn on LAYER_DRAW_BITMAPS, but their
    // associated board layer controls their visibility.
    if( !GetBoard()->IsLayerVisible( m_layer ) )
        return HIDE;

    return aView->IsLayerVisible( LAYER_DRAW_BITMAPS ) ? 0.0 : HIDE;
}


const BOX2I PCB_BITMAP::GetBoundingBox() const
{
    // Bitmaps are center origin, BOX2Is need top-left origin
    VECTOR2I size = m_bitmapBase->GetSize();
    VECTOR2I topLeft = { m_pos.x - size.x / 2, m_pos.y - size.y / 2 };

    return BOX2I( topLeft, size );
}


std::shared_ptr<SHAPE> PCB_BITMAP::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    BOX2I box = GetBoundingBox();
    return std::shared_ptr<SHAPE_RECT>( new SHAPE_RECT( box.GetPosition(), box.GetWidth(),
                                                        box.GetHeight() ) );
}


const VECTOR2I PCB_BITMAP::GetSize() const
{
    return m_bitmapBase->GetSize();
}


void PCB_BITMAP::Flip( const VECTOR2I& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
    {
        MIRROR( m_pos.x, aCentre.x );
        m_bitmapBase->Mirror( false );
    }
    else
    {
        MIRROR( m_pos.y, aCentre.y );
        m_bitmapBase->Mirror( true );
    }
}

void PCB_BITMAP::Rotate( const VECTOR2I& aCenter, const EDA_ANGLE& aAngle )
{
    EDA_ANGLE norm( aAngle.AsDegrees(), DEGREES_T );

    RotatePoint( m_pos, aCenter, aAngle );

    norm.Normalize();

    // each call to m_bitmapBase->Rotate() rotates 90 degrees CCW
    for( double ang = 45.0; ang < norm.AsDegrees(); ang += 90.0 )
        m_bitmapBase->Rotate( false );
}


#if defined( DEBUG )
void PCB_BITMAP::Show( int nestLevel, std::ostream& os ) const
{
    // XML output:
    wxString s = GetClass();

    NestedSpace( nestLevel, os ) << '<' << s.Lower().mb_str() << m_pos << "/>\n";
}
#endif


bool PCB_BITMAP::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    BOX2I rect = GetBoundingBox();

    rect.Inflate( aAccuracy );

    return rect.Contains( aPosition );
}


bool PCB_BITMAP::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I rect = aRect;

    rect.Inflate( aAccuracy );

    if( aContained )
        return rect.Contains( GetBoundingBox() );

    return rect.Intersects( GetBoundingBox() );
}


BITMAPS PCB_BITMAP::GetMenuImage() const
{
    return BITMAPS::image;
}


void PCB_BITMAP::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Bitmap" ), wxEmptyString );

    aList.emplace_back( _( "PPI" ), wxString::Format( wxT( "%d "), GetImage()->GetPPI() ) );
    aList.emplace_back( _( "Scale" ), wxString::Format( wxT( "%f "), GetImageScale() ) );

    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetSize().x ) );
    aList.emplace_back( _( "Height" ), aFrame->MessageTextFromValue( GetSize().y ) );
    aList.emplace_back( _( "Layer" ), LayerName( m_layer ) );
}


void PCB_BITMAP::ViewGetLayers( int aLayers[], int& aCount ) const
{
    aCount = 1;
    aLayers[0] = BITMAP_LAYER_FOR( m_layer );
}


static struct PCB_BITMAP_DESC
{
    PCB_BITMAP_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_BITMAP );
        propMgr.InheritsAfter( TYPE_HASH( PCB_BITMAP ), TYPE_HASH( BOARD_ITEM ) );

        const wxString groupBitmap = _HKI( "Bitmap Properties" );

        propMgr.AddProperty( new PROPERTY<PCB_BITMAP, double>( _HKI( "Scale" ),
                             &PCB_BITMAP::SetImageScale, &PCB_BITMAP::GetImageScale ),
                             groupBitmap );

        // For future use
        const wxString greyscale = _HKI( "Greyscale" );
    }
} _PCB_BITMAP_DESC;

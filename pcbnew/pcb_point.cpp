/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
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
#include "pcb_point.h"

#include <base_units.h>
#include <bitmaps.h>
#include <board.h>
#include <eda_draw_frame.h>
#include <geometry/geometry_utils.h>
#include <geometry/shape_circle.h>
#include <geometry/shape_rect.h>
#include <pcb_shape.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <view/view.h>


static const double DEFAULT_PT_SIZE_MM = 1.0;


PCB_POINT::PCB_POINT( BOARD_ITEM* aParent ) :
        BOARD_ITEM( aParent, PCB_POINT_T ),
        m_pos( 0, 0 ),
        m_size( pcbIUScale.mmToIU( DEFAULT_PT_SIZE_MM ) )
{
}


PCB_POINT::PCB_POINT( BOARD_ITEM* aParent, const VECTOR2I& aPos, int aSize ) :
    BOARD_ITEM( aParent, PCB_POINT_T ),
    m_pos( aPos ),
    m_size( aSize )
{
}


bool PCB_POINT::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    // Compute the hit on the bars of the X
    const int size = GetSize() / 2;
    const SEG loc = SEG( aPosition, aPosition );

    const SEG          seg1 = SEG( m_pos - VECTOR2I{ size, size }, m_pos + VECTOR2I{ size, size } );
    const SEG          seg2 = SEG( m_pos - VECTOR2I{ size, -size }, m_pos + VECTOR2I{ size, -size } );
    const SHAPE_CIRCLE circle( m_pos, size / 2 );

    bool hit = seg1.Collide( loc, aAccuracy ) || seg2.Collide( loc, aAccuracy ) || circle.Collide( loc, aAccuracy );

    return hit;
}


bool PCB_POINT::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    return KIGEOM::BoxHitTest( aRect, GetBoundingBox(), aContained, aAccuracy );
}


std::vector<int> PCB_POINT::ViewGetLayers() const
{
    const PCB_LAYER_ID layer = GetLayer();

    std::vector<int> layers = {
        POINT_LAYER_FOR( layer ),
    };

    if( IsLocked() )
        layers.push_back( LAYER_LOCKED_ITEM_SHADOW );

    return layers;
}


double PCB_POINT::ViewGetLOD( int aLayer, const KIGFX::VIEW* aView ) const
{
    // All points hidden
    if( !aView->IsLayerVisible( LAYER_POINTS ) )
        return LOD_HIDE;

    // Hide if the "main" layer is not shown
    if( !aView->IsLayerVisible( m_layer ) )
        return LOD_HIDE;

    return LOD_SHOW;
}


void PCB_POINT::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );
}


void PCB_POINT::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    MIRROR(m_pos, aCentre, aFlipDirection);

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
}


const BOX2I PCB_POINT::GetBoundingBox() const
{
    BOX2I bbox = BOX2I::ByCenter( m_pos, VECTOR2I{ m_size, m_size } );
    return bbox;
}


std::shared_ptr<SHAPE> PCB_POINT::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_RECT>( GetBoundingBox() );
}


wxString PCB_POINT::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    return _( "Point" );
}


BITMAPS PCB_POINT::GetMenuImage() const
{
    return BITMAPS::add_point;
}


EDA_ITEM* PCB_POINT::Clone() const
{
    return new PCB_POINT( *this );
}


void PCB_POINT::swapData( BOARD_ITEM* aOther )
{
    assert( aOther->Type() == PCB_POINT_T );

    std::swap( *((PCB_POINT*) this), *((PCB_POINT*) aOther) );
}


void PCB_POINT::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "PCB Point" ), wxEmptyString );

    aList.emplace_back( _( "Position X" ), aFrame->MessageTextFromValue( GetPosition().x ) );
    aList.emplace_back( _( "Position Y" ), aFrame->MessageTextFromValue( GetPosition().y ) );
    aList.emplace_back( _( "Size" ), aFrame->MessageTextFromValue( GetSize() ) );
    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


void PCB_POINT::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                          int aClearance, int aError, ERROR_LOC aErrorLoc,
                                          bool ignoreLineWidth ) const
{
}


bool PCB_POINT::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_POINT& other = static_cast<const PCB_POINT&>( aBoardItem );

    return *this == other;
}


bool PCB_POINT::operator==( const PCB_POINT& aOther ) const
{
    return m_pos == aOther.m_pos && m_size == aOther.m_size;
}


double PCB_POINT::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_POINT& other = static_cast<const PCB_POINT&>( aOther );

    double similarity = 1.0;

    if( m_pos == other.m_pos )
        similarity *= 0.9;

    if( m_size != other.m_size )
        similarity *= 0.9;

    if( GetLayer() != other.GetLayer() )
        similarity *= 0.9;

    return similarity;
}

static struct PCB_POINT_DESC
{
    // clang-format off
    PCB_POINT_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        // PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_POINT );
        propMgr.InheritsAfter( TYPE_HASH( PCB_POINT ), TYPE_HASH( BOARD_ITEM ) );

        propMgr.AddProperty( new PROPERTY<PCB_POINT, int>( _HKI( "Size" ),
                &PCB_POINT::SetSize, &PCB_POINT::GetSize,
                PROPERTY_DISPLAY::PT_SIZE ) );
    }
    // clang-format on
} _PCB_POINT_DESC;

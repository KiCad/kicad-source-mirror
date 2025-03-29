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
#include "pcb_target.h"

#include <bitmaps.h>
#include <board.h>
#include <board_design_settings.h>
#include <base_units.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <i18n_utility.h>
#include <geometry/shape_circle.h>
#include <eda_draw_frame.h>
#include <pcb_shape.h>


PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_shape     = 0;
    m_size      = pcbIUScale.mmToIU( 5 );          // Gives a decent size
    m_lineWidth = pcbIUScale.mmToIU( DEFAULT_COPPER_LINE_WIDTH );
    m_layer     = Edge_Cuts;                   // a target is on all layers
}

PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent, int aShape, PCB_LAYER_ID aLayer, const VECTOR2I& aPos,
                        int aSize, int aWidth ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_shape     = aShape;
    m_layer     = aLayer;
    m_pos       = aPos;
    m_size      = aSize;
    m_lineWidth = aWidth;
}


PCB_TARGET::~PCB_TARGET()
{
}


bool PCB_TARGET::HitTest( const VECTOR2I& aPosition, int aAccuracy ) const
{
    int dX = aPosition.x - m_pos.x;
    int dY = aPosition.y - m_pos.y;
    int radius = aAccuracy + ( m_size / 2 );
    return abs( dX ) <= radius && abs( dY ) <= radius;
}


bool PCB_TARGET::HitTest( const BOX2I& aRect, bool aContained, int aAccuracy ) const
{
    BOX2I arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( GetBoundingBox() );
    else
        return GetBoundingBox().Intersects( arect );
}


void PCB_TARGET::Rotate( const VECTOR2I& aRotCentre, const EDA_ANGLE& aAngle )
{
    RotatePoint( m_pos, aRotCentre, aAngle );
}


void PCB_TARGET::Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection )
{
    if( aFlipDirection == FLIP_DIRECTION::LEFT_RIGHT )
        m_pos.x = aCentre.x - ( m_pos.x - aCentre.x );
    else
        m_pos.y = aCentre.y - ( m_pos.y - aCentre.y );

    SetLayer( GetBoard()->FlipLayer( GetLayer() ) );
}


const BOX2I PCB_TARGET::GetBoundingBox() const
{
    BOX2I bBox;
    bBox.SetX( m_pos.x - m_size / 2 );
    bBox.SetY( m_pos.y - m_size / 2 );
    bBox.SetWidth( m_size );
    bBox.SetHeight( m_size );

    return bBox;
}


std::shared_ptr<SHAPE> PCB_TARGET::GetEffectiveShape( PCB_LAYER_ID aLayer, FLASHING aFlash ) const
{
    return std::make_shared<SHAPE_CIRCLE>( m_pos, m_size / 2 );
}


wxString PCB_TARGET::GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const
{
    // Targets are on *every* layer by definition
    return _( "Target" );
}


BITMAPS PCB_TARGET::GetMenuImage() const
{
    return  BITMAPS::add_pcb_target;
}


EDA_ITEM* PCB_TARGET::Clone() const
{
    return new PCB_TARGET( *this );
}


void PCB_TARGET::swapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TARGET_T );

    std::swap( *((PCB_TARGET*) this), *((PCB_TARGET*) aImage) );
}


void PCB_TARGET::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "PCB Target" ), wxEmptyString );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Size" ), aFrame->MessageTextFromValue( GetSize() ) );
    aList.emplace_back( _( "Width" ), aFrame->MessageTextFromValue( GetWidth() ) );
    aList.emplace_back( _( "Shape" ), GetShape() == 0 ? wxT( "+" ) : wxT( "X" ) );
}


void PCB_TARGET::TransformShapeToPolygon( SHAPE_POLY_SET& aBuffer, PCB_LAYER_ID aLayer,
                                          int aClearance, int aError, ERROR_LOC aErrorLoc,
                                          bool ignoreLineWidth ) const
{
    int size = GetShape() ? GetSize() / 1.5 : GetSize() / 2.0;
    int radius = GetShape() ? GetSize() / 2.0 : GetSize() / 3.0;

    PCB_SHAPE line1, line2;
    PCB_SHAPE circle( nullptr, SHAPE_T::CIRCLE );
    line1.SetStart( VECTOR2I( -size, 0.0 ) );
    line1.SetEnd( VECTOR2I( size, 0.0 ) );
    line2.SetStart( VECTOR2I( 0.0, -size ) );
    line2.SetEnd( VECTOR2I( 0.0, size ) );
    circle.SetEndX( radius );

    if( GetShape() )    // shape x
    {
        line1.Rotate( VECTOR2I(0,0), EDA_ANGLE( 45.0, DEGREES_T ) );
        line2.Rotate( VECTOR2I(0,0), EDA_ANGLE( 45.0, DEGREES_T ) );
    }

    for( PCB_SHAPE* item: { &line1, &line2, &circle } )
    {
        item->SetWidth( GetWidth() );
        item->Move( GetPosition() );
        item->TransformShapeToPolygon( aBuffer, aLayer, aClearance, aError, aErrorLoc,
                                       ignoreLineWidth );
    }
}


bool PCB_TARGET::operator==( const BOARD_ITEM& aBoardItem ) const
{
    if( aBoardItem.Type() != Type() )
        return false;

    const PCB_TARGET& other = static_cast<const PCB_TARGET&>( aBoardItem );

    return *this == other;
}


bool PCB_TARGET::operator==( const PCB_TARGET& aOther ) const
{
    return m_shape == aOther.m_shape
           && m_size == aOther.m_size
           && m_lineWidth == aOther.m_lineWidth
           && m_layer == aOther.m_layer
           && m_pos == aOther.m_pos;
}


double PCB_TARGET::Similarity( const BOARD_ITEM& aOther ) const
{
    if( aOther.Type() != Type() )
        return 0.0;

    const PCB_TARGET& other = static_cast<const PCB_TARGET&>( aOther );

    double similarity = 1.0;

    if( GetShape() != other.GetShape() )
        similarity *= 0.9;

    if( GetSize() != other.GetSize() )
        similarity *= 0.9;

    if( GetWidth() != other.GetWidth() )
        similarity *= 0.9;

    if( GetLayer() != other.GetLayer() )
        similarity *= 0.9;

    if( GetPosition() != other.GetPosition() )
        similarity *= 0.9;

    return similarity;
}

static struct PCB_TARGET_DESC
{
    PCB_TARGET_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TARGET );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TARGET ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.AddProperty( new PROPERTY<PCB_TARGET, int>( _HKI( "Size" ),
                    &PCB_TARGET::SetSize, &PCB_TARGET::GetSize, PROPERTY_DISPLAY::PT_SIZE ) );
        propMgr.AddProperty( new PROPERTY<PCB_TARGET, int>( _HKI( "Width" ),
                    &PCB_TARGET::SetWidth, &PCB_TARGET::GetWidth, PROPERTY_DISPLAY::PT_SIZE ) );

        auto shape = new PROPERTY<PCB_TARGET, int>( _HKI( "Shape" ),
                &PCB_TARGET::SetShape, &PCB_TARGET::GetShape );
        // TODO change the integer to an enum?
        //shape->SetValues( { { 0, _HKI( "Cross" ) }, { 1, ( "Plus" ) } } );
        propMgr.AddProperty( shape );
    }
} _PCB_TARGET_DESC;

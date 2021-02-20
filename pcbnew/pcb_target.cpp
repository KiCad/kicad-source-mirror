/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <board.h>
#include <pcb_target.h>
#include <base_units.h>
#include <settings/color_settings.h>
#include <settings/settings_manager.h>
#include <trigo.h>
#include <i18n_utility.h>
#include <geometry/shape_circle.h>
#include <eda_draw_frame.h>

PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent ) :
    BOARD_ITEM( aParent, PCB_TARGET_T )
{
    m_shape     = 0;
    m_size      = Millimeter2iu( 5 );          // Gives a decent size
    m_lineWidth = Millimeter2iu( DEFAULT_COPPER_LINE_WIDTH );
    m_layer     = Edge_Cuts;                   // a target is on all layers
}

PCB_TARGET::PCB_TARGET( BOARD_ITEM* aParent, int aShape, PCB_LAYER_ID aLayer,
                        const wxPoint& aPos, int aSize, int aWidth ) :
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


bool PCB_TARGET::HitTest( const wxPoint& aPosition, int aAccuracy ) const
{
    int dX = aPosition.x - m_pos.x;
    int dY = aPosition.y - m_pos.y;
    int radius = aAccuracy + ( m_size / 2 );
    return abs( dX ) <= radius && abs( dY ) <= radius;
}


bool PCB_TARGET::HitTest( const EDA_RECT& aRect, bool aContained, int aAccuracy ) const
{
    EDA_RECT arect = aRect;
    arect.Inflate( aAccuracy );

    if( aContained )
        return arect.Contains( GetBoundingBox() );
    else
        return GetBoundingBox().Intersects( arect );
}


void PCB_TARGET::Rotate(const wxPoint& aRotCentre, double aAngle)
{
    RotatePoint( &m_pos, aRotCentre, aAngle );
}


void PCB_TARGET::Flip(const wxPoint& aCentre, bool aFlipLeftRight )
{
    if( aFlipLeftRight )
        m_pos.x = aCentre.x - ( m_pos.x - aCentre.x );
    else
        m_pos.y = aCentre.y - ( m_pos.y - aCentre.y );

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}


const EDA_RECT PCB_TARGET::GetBoundingBox() const
{
    EDA_RECT bBox;
    bBox.SetX( m_pos.x - m_size / 2 );
    bBox.SetY( m_pos.y - m_size / 2 );
    bBox.SetWidth( m_size );
    bBox.SetHeight( m_size );

    return bBox;
}


std::shared_ptr<SHAPE> PCB_TARGET::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return std::make_shared<SHAPE_CIRCLE>( m_pos, m_size / 2 );
}


wxString PCB_TARGET::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    // Targets are on *every* layer by definition
    return _( "Target" );
}


BITMAP_DEF PCB_TARGET::GetMenuImage() const
{
    return  add_pcb_target_xpm;
}


EDA_ITEM* PCB_TARGET::Clone() const
{
    return new PCB_TARGET( *this );
}


void PCB_TARGET::SwapData( BOARD_ITEM* aImage )
{
    assert( aImage->Type() == PCB_TARGET_T );

    std::swap( *((PCB_TARGET*) this), *((PCB_TARGET*) aImage) );
}


void PCB_TARGET::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    EDA_UNITS units = aFrame->GetUserUnits();

    aList.emplace_back( _( "PCB Target" ), wxEmptyString );

    aList.emplace_back( _( "Layer" ), GetLayerName() );

    aList.emplace_back( _( "Size" ), MessageTextFromValue( units, GetSize() ) );
    aList.emplace_back( _( "Width" ), MessageTextFromValue( units, GetWidth() ) );
    aList.emplace_back( _( "Shape" ), GetShape() == 0 ? "+" : "X" );
}


static struct PCB_TARGET_DESC
{
    PCB_TARGET_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_TARGET );
        propMgr.InheritsAfter( TYPE_HASH( PCB_TARGET ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.AddProperty( new PROPERTY<PCB_TARGET, int>( _HKI( "Size" ),
                    &PCB_TARGET::SetSize, &PCB_TARGET::GetSize, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<PCB_TARGET, int>( _HKI( "Width" ),
                    &PCB_TARGET::SetWidth, &PCB_TARGET::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );

        auto shape = new PROPERTY<PCB_TARGET, int>( _HKI( "Shape" ),
                &PCB_TARGET::SetShape, &PCB_TARGET::GetShape );
        // TODO change the integer to an enum?
        //shape->SetValues( { { 0, _HKI( "Cross" ) }, { 1, ( "Plus" ) } } );
        propMgr.AddProperty( shape );
    }
} _PCB_TARGET_DESC;

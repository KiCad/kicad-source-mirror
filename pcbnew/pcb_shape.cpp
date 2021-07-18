/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <base_units.h>
#include <geometry/shape_compound.h>
#include <pcb_shape.h>


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, KICAD_T idtype, SHAPE_T shapetype ) :
    BOARD_ITEM( aParent, idtype ),
    EDA_SHAPE( shapetype, Millimeter2iu( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
{
}


PCB_SHAPE::PCB_SHAPE( BOARD_ITEM* aParent, SHAPE_T shapetype ) :
    BOARD_ITEM( aParent, PCB_SHAPE_T ),
    EDA_SHAPE( shapetype, Millimeter2iu( DEFAULT_LINE_WIDTH ), FILL_T::NO_FILL )
{
}


PCB_SHAPE::~PCB_SHAPE()
{
}


const wxPoint PCB_SHAPE::GetFocusPosition() const
{
    // For some shapes return the visual center, but for not filled polygonal shapes,
    // the center is usually far from the shape: a point on the outline is better

    switch( m_shape )
    {
    case SHAPE_T::CIRCLE:
        if( !IsFilled() )
            return wxPoint( GetCenter().x + GetRadius(), GetCenter().y );
        else
            return GetCenter();

    case SHAPE_T::RECT:
        if( !IsFilled() )
            return GetStart();
        else
            return GetCenter();

    case SHAPE_T::POLY:
        if( !IsFilled() )
        {
            VECTOR2I pos = GetPolyShape().Outline(0).CPoint(0);
            return wxPoint( pos.x, pos.y );
        }
        else
        {
            return GetCenter();
        }

    case SHAPE_T::ARC:
        return GetArcMid();

    case SHAPE_T::BEZIER:
        return GetStart();

    default:
        return GetCenter();
    }
}


void PCB_SHAPE::Move( const wxPoint& aMoveVector )
{
    move( aMoveVector );
}


void PCB_SHAPE::Scale( double aScale )
{
    scale( aScale );
}


void PCB_SHAPE::Rotate( const wxPoint& aRotCentre, double aAngle )
{
    rotate( aRotCentre, aAngle );
}


void PCB_SHAPE::Flip( const wxPoint& aCentre, bool aFlipLeftRight )
{
    flip( aCentre, aFlipLeftRight );

    SetLayer( FlipLayer( GetLayer(), GetBoard()->GetCopperLayerCount() ) );
}


FOOTPRINT* PCB_SHAPE::GetParentFootprint() const
{
    if( !m_parent || m_parent->Type() != PCB_FOOTPRINT_T )
        return nullptr;

    return (FOOTPRINT*) m_parent;
}


double PCB_SHAPE::getParentOrientation() const
{
    if( GetParentFootprint() )
        return GetParentFootprint()->GetOrientation();
    else
        return 0.0;
}


wxPoint PCB_SHAPE::getParentPosition() const
{
    if( GetParentFootprint() )
        return GetParentFootprint()->GetPosition();
    else
        return wxPoint( 0, 0 );
}


void PCB_SHAPE::GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList )
{
    aList.emplace_back( _( "Type" ), _( "Drawing" ) );

    if( IsLocked() )
        aList.emplace_back( _( "Status" ), _( "Locked" ) );

    ShapeGetMsgPanelInfo( aFrame, aList );

    aList.emplace_back( _( "Layer" ), GetLayerName() );
}


wxString PCB_SHAPE::GetSelectMenuText( EDA_UNITS aUnits ) const
{
    return wxString::Format( _( "%s on %s" ), ShowShape(), GetLayerName() );
}


BITMAPS PCB_SHAPE::GetMenuImage() const
{
    return BITMAPS::add_dashed_line;
}


EDA_ITEM* PCB_SHAPE::Clone() const
{
    return new PCB_SHAPE( *this );
}


const BOX2I PCB_SHAPE::ViewBBox() const
{
    // For arcs - do not include the center point in the bounding box,
    // it is redundant for displaying an arc
    if( GetShape() == SHAPE_T::ARC )
    {
        EDA_RECT bbox;
        computeArcBBox( bbox );
        return BOX2I( bbox.GetOrigin(), bbox.GetSize() );
    }

    BOX2I return_box = EDA_ITEM::ViewBBox();
    return_box.Inflate( GetWidth() );    // Technically GetWidth() / 2, but it never hurts to be
                                         // a bit large to account for selection shadows, etc.

    return return_box;
}


std::shared_ptr<SHAPE> PCB_SHAPE::GetEffectiveShape( PCB_LAYER_ID aLayer ) const
{
    return std::make_shared<SHAPE_COMPOUND>( MakeEffectiveShapes() );
}


void PCB_SHAPE::SwapData( BOARD_ITEM* aImage )
{
    PCB_SHAPE* image = dynamic_cast<PCB_SHAPE*>( aImage );
    assert( image );

    SwapShape( image );

    std::swap( m_layer, image->m_layer );
    std::swap( m_flags, image->m_flags );
    std::swap( m_status, image->m_status );
    std::swap( m_parent, image->m_parent );
    std::swap( m_forceVisible, image->m_forceVisible );
}


bool PCB_SHAPE::cmp_drawings::operator()( const BOARD_ITEM* aFirst,
                                          const BOARD_ITEM* aSecond ) const
{
    if( aFirst->Type() != aSecond->Type() )
        return aFirst->Type() < aSecond->Type();

    if( aFirst->GetLayer() != aSecond->GetLayer() )
        return aFirst->GetLayer() < aSecond->GetLayer();

    if( aFirst->Type() == PCB_SHAPE_T )
    {
        const PCB_SHAPE* dwgA = static_cast<const PCB_SHAPE*>( aFirst );
        const PCB_SHAPE* dwgB = static_cast<const PCB_SHAPE*>( aSecond );

        if( dwgA->GetShape() != dwgB->GetShape() )
            return dwgA->GetShape() < dwgB->GetShape();
    }

    return aFirst->m_Uuid < aSecond->m_Uuid;
}


static struct PCB_SHAPE_DESC
{
    PCB_SHAPE_DESC()
    {
        PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
        REGISTER_TYPE( PCB_SHAPE );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, BOARD_ITEM> );
        propMgr.AddTypeCast( new TYPE_CAST<PCB_SHAPE, EDA_SHAPE> );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( BOARD_ITEM ) );
        propMgr.InheritsAfter( TYPE_HASH( PCB_SHAPE ), TYPE_HASH( EDA_SHAPE ) );

        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "Thickness" ),
                    &EDA_SHAPE::SetWidth, &EDA_SHAPE::GetWidth, PROPERTY_DISPLAY::DISTANCE ) );
        // TODO show certain properties depending on the shape
        //propMgr.AddProperty( new PROPERTY<PCB_SHAPE, double>( _HKI( "Angle" ),
        //            &PCB_SHAPE::SetArcAngle, &PCB_SHAPE::GetAngle, PROPERTY_DISPLAY::DECIDEGREE ) );
        // TODO or may have different names (arcs)
        // TODO type?
        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "End X" ),
                    &EDA_SHAPE::SetEndX, &EDA_SHAPE::GetEndX, PROPERTY_DISPLAY::DISTANCE ) );
        propMgr.AddProperty( new PROPERTY<EDA_SHAPE, int>( _HKI( "End Y" ),
                    &EDA_SHAPE::SetEndY, &EDA_SHAPE::GetEndY, PROPERTY_DISPLAY::DISTANCE ) );
    }
} _PCB_SHAPE_DESC;

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <tools/graphic_edit.h>

#include <board_item.h>
#include <pcb_shape.h>

#include <cmath>


const PCB_SHAPE* GraphicEditShape( const EDA_ITEM* aItem )
{
    if( !aItem || aItem->Type() != PCB_SHAPE_T )
        return nullptr;

    const PCB_SHAPE* shape = static_cast<const PCB_SHAPE*>( aItem );

    if( shape->GetShape() != SHAPE_T::SEGMENT && shape->GetShape() != SHAPE_T::ARC )
        return nullptr;

    return shape;
}


const PCB_SHAPE* GraphicEditSource( const BOARD_ITEM& aSource, GRAPHIC_EDIT_RESULT& aResult )
{
    const PCB_SHAPE* source = GraphicEditShape( &aSource );

    if( !source )
    {
        aResult.m_Refusal = GRAPHIC_EDIT_REFUSAL::UNSUPPORTED_SOURCE;
        return nullptr;
    }

    if( source->IsLocked() )
    {
        aResult.m_Refusal = GRAPHIC_EDIT_REFUSAL::LOCKED_SOURCE;
        return nullptr;
    }

    return source;
}


const PCB_SHAPE* GraphicEditBoundary( const BOARD_ITEM* aBoundary, const PCB_SHAPE& aSource )
{
    const PCB_SHAPE* boundary = GraphicEditShape( aBoundary );

    if( !boundary || boundary == &aSource || boundary->GetLayer() != aSource.GetLayer() )
        return nullptr;

    return boundary;
}


SHAPE_ARC GraphicEditArc( const PCB_SHAPE& aShape )
{
    return SHAPE_ARC( aShape.GetStart(), aShape.GetArcMid(), aShape.GetEnd(), 0 );
}


bool IsGraphicEditArcUsable( const SHAPE_ARC& aArc )
{
    double sweep = std::abs( aArc.GetCentralAngle().AsDegrees() );

    return aArc.GetRadius() > 0.0 && aArc.GetRadius() <= MAX_GRAPHIC_EDIT_ARC_RADIUS
           && sweep >= GRAPHIC_EDIT_ANGLE_EPSILON && sweep < 360.0 - GRAPHIC_EDIT_ANGLE_EPSILON;
}


bool GraphicEditCoincident( const VECTOR2I& aA, const VECTOR2I& aB )
{
    // 2 is the furthest apart a rounding step either way can put them.
    return aA.SquaredDistance( aB ) <= 2;
}

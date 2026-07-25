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

#include <tools/board_item_geometry.h>

#include <pcb_reference_image.h>
#include <pcb_shape.h>
#include <pcb_track.h>


std::optional<INTERSECTABLE_GEOM> BoardItemIntersectable( const BOARD_ITEM& aItem )
{
    switch( aItem.Type() )
    {
    case PCB_SHAPE_T:
    {
        const PCB_SHAPE& shape = static_cast<const PCB_SHAPE&>( aItem );

        switch( shape.GetShape() )
        {
        case SHAPE_T::SEGMENT:   return SEG{ shape.GetStart(), shape.GetEnd() };
        case SHAPE_T::CIRCLE:    return CIRCLE{ shape.GetCenter(), shape.GetRadius() };
        case SHAPE_T::ARC:       return SHAPE_ARC{ shape.GetStart(), shape.GetArcMid(), shape.GetEnd(), 0 };
        case SHAPE_T::RECTANGLE: return BOX2I::ByCorners( shape.GetStart(), shape.GetEnd() );

        case SHAPE_T::ELLIPSE:
            return SHAPE_ELLIPSE{ shape.GetEllipseCenter(), shape.GetEllipseMajorRadius(),
                                  shape.GetEllipseMinorRadius(), shape.GetEllipseRotation() };

        case SHAPE_T::ELLIPSE_ARC:
            return SHAPE_ELLIPSE{ shape.GetEllipseCenter(),      shape.GetEllipseMajorRadius(),
                                  shape.GetEllipseMinorRadius(), shape.GetEllipseRotation(),
                                  shape.GetEllipseStartAngle(),  shape.GetEllipseEndAngle() };

        default:                 break;
        }

        break;
    }

    case PCB_TRACE_T:
    {
        const PCB_TRACK& track = static_cast<const PCB_TRACK&>( aItem );
        return SEG{ track.GetStart(), track.GetEnd() };
    }

    case PCB_ARC_T:
    {
        const PCB_ARC& arc = static_cast<const PCB_ARC&>( aItem );
        return SHAPE_ARC{ arc.GetStart(), arc.GetMid(), arc.GetEnd(), 0 };
    }

    case PCB_REFERENCE_IMAGE_T:
    {
        const PCB_REFERENCE_IMAGE& refImage = static_cast<const PCB_REFERENCE_IMAGE&>( aItem );
        return refImage.GetBoundingBox();
    }

    default:
        break;
    }

    return std::nullopt;
}

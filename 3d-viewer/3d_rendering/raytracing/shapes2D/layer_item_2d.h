
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#pragma once

#include "object_2d.h"
#include <vector>

struct RAYSEG2D;

#define CSGITEM_EMPTY 0
#define CSGITEM_FULL (OBJECT_2D*) ( ( size_t )( -1 ) )

/**
 *  Make solid geometry for objects on layers.
 *
 *  The operation is in the form (A - B) /\ C
 *  For almost all of the layers it translate something like:
 *  A (a via, a track, pad, polygon), B (a via hole, a THT hole, .. ),
 *  C the board (epoxy)
 *  For layers like Solder mask that are negative:
 *  A ( epoxy ), B( pads, polygons, ..), C=1
 *
 *  Some notes:
 *  BODY = PCB_outlines - PCB_holes - (Modules_THT_holes + VIA_THT_holes)
 *
 *  Plated_vias_and_holes = Tracks.Vias + Modules.PlatedHoles
 *
 *  Layer.cu = ( Tracks.cu + Modules_Pads.cu + Modules_Graphics.cu +
 *               Layer_zones.cu + PCB_drawings.cu - Layer_VIA_holes ) & BODY
 *
 *  Layer.Mask =    BODY -
 *                  (PCB_drawing.Mask + Modules_Graphics.Mask +
 *                   Modules_Pads.Mask + Layer_zones.Mask )
 *  Layer.Paste =     (PCB_drawing.Paste + Modules_Graphics.Paste +
 *                     Modules_Pads.Paste + Layer_zones.Paste) & BODY
 *  Layer.Silk  =     (PCB_drawing.Silk  + Modules_Graphics.Silk +
 *                     Modules_Pads.Silk  + Layer_zones.Paste) & BODY
 *
 *  BODY         = A - B /\ 1
 *  Layer.cu     = A - B /\ C
 *  Layer.mask   = A - B /\ 1
 *  Layers.Paste = A - 0 /\ C
 *  Layers.Silk  = A - 0 /\ C
 *
 *  BODY         =    P - T /\ 1
 *  Layer.cu     =    T - H /\ BODY
 *  Layer.mask   = BODY - M /\ 1
 *  Layers.Paste =    P - 0 /\ BODY
 *  Layers.Silk  =    S - 0 /\ BODY
 */
class LAYER_ITEM_2D : public OBJECT_2D
{
public:
    LAYER_ITEM_2D( const OBJECT_2D* aObjectA, std::vector<const OBJECT_2D*>* aObjectB,
                   const OBJECT_2D* aObjectC, const BOARD_ITEM& aBoardItem );

    ~LAYER_ITEM_2D();

    // We own at least one list of raw pointers.  Don't let the compiler fill in copy c'tors that
    // will only land us in trouble.
    LAYER_ITEM_2D( const LAYER_ITEM_2D& ) = delete;
    LAYER_ITEM_2D& operator=( const LAYER_ITEM_2D& ) = delete;

    // Imported from OBJECT_2D
    bool Overlaps( const BBOX_2D& aBBox ) const override;
    bool Intersects( const BBOX_2D& aBBox ) const override;
    bool Intersect( const RAYSEG2D& aSegRay, float* aOutT, SFVEC2F* aNormalOut ) const override;
    INTERSECTION_RESULT IsBBoxInside( const BBOX_2D& aBBox ) const override;
    bool IsPointInside( const SFVEC2F& aPoint ) const override;

private:
    const OBJECT_2D*                m_objectA;
    std::vector<const OBJECT_2D*>*  m_objectB;
    const OBJECT_2D*                m_objectC;
};


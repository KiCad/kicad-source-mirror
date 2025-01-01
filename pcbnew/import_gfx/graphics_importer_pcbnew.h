/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef GRAPHICS_IMPORTER_PCBNEW_H
#define GRAPHICS_IMPORTER_PCBNEW_H

#include <import_gfx/graphics_importer.h>
#include <layer_ids.h>

class BOARD_ITEM_CONTAINER;

class GRAPHICS_IMPORTER_PCBNEW : public GRAPHICS_IMPORTER
{
public:
    GRAPHICS_IMPORTER_PCBNEW( BOARD_ITEM_CONTAINER* aParent );

    void SetLayer( PCB_LAYER_ID aLayer ) { m_layer = aLayer; }
    PCB_LAYER_ID GetLayer() const { return m_layer; }

    void AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                  const IMPORTED_STROKE& aStroke ) override;

    void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                    bool aFilled, const COLOR4D& aFillColor = COLOR4D::UNSPECIFIED ) override;

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 const IMPORTED_STROKE& aStroke ) override;

    void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                     bool aFilled, const COLOR4D& aFillColor = COLOR4D::UNSPECIFIED ) override;

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth,
                  double aThickness, double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                  GR_TEXT_V_ALIGN_T aVJustify,
                  const COLOR4D&    aColor = COLOR4D::UNSPECIFIED ) override;

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                    const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                    const IMPORTED_STROKE& aStroke ) override;

    /**
     * Convert an imported coordinate to a board coordinate, according to the internal units,
     * user scale and offset
     *
     * @param aCoordinate is the imported coordinate in mm.
     */
    VECTOR2I MapCoordinate( const VECTOR2D& aCoordinate );

    /**
     * If aLineWidth < 0, the default line thickness value is returned.
     *
     * @param aLineWidth is the line thickness in mm to convert.
     * @return a line thickness in a board Iu value, according to the internal units.
     */
    int MapLineWidth( double aLineWidth );

    STROKE_PARAMS MapStrokeParams( const IMPORTED_STROKE& aStroke );

protected:
    ///< Target layer for the imported shapes.
    PCB_LAYER_ID          m_layer;
    BOARD_ITEM_CONTAINER* m_parent;
};



#endif /* GRAPHICS_IMPORTER_PCBNEW_H */

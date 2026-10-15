/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef GRAPHICS_IMPORTER_PL_EDITOR_H
#define GRAPHICS_IMPORTER_PL_EDITOR_H

#include <memory>
#include <vector>

#include <drawing_sheet/ds_data_item.h>
#include <import_gfx/graphics_importer.h>


/**
 * Turn imported geometry into drawing sheet items.
 *
 * A drawing sheet holds only text, segments, rectangles and filled polygons, so curves
 * arrive as the segments they are flattened into. Everything is kept in millimetres,
 * which is what a drawing sheet item stores.
 */
class GRAPHICS_IMPORTER_PL_EDITOR : public GRAPHICS_IMPORTER
{
public:
    GRAPHICS_IMPORTER_PL_EDITOR();

    /**
     * The imported items, which the caller takes over.
     *
     * The base class list stays empty, because a drawing sheet item is not an EDA_ITEM.
     */
    std::vector<std::unique_ptr<DS_DATA_ITEM>>& GetDataItems() { return m_dataItems; }

    void AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd, const IMPORTED_STROKE& aStroke ) override;

    void AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke, bool aFilled,
                    const COLOR4D& aFillColor ) override;

    void AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                 const IMPORTED_STROKE& aStroke ) override;

    void AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke, bool aFilled,
                     const COLOR4D& aFillColor ) override;

    void AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight, double aWidth, double aThickness,
                  double aOrientation, GR_TEXT_H_ALIGN_T aHJustify, GR_TEXT_V_ALIGN_T aVJustify,
                  const COLOR4D& aColor ) override;

    void AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1, const VECTOR2D& aBezierControl2,
                    const VECTOR2D& aEnd, const IMPORTED_STROKE& aStroke ) override;

    void AddEllipse( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                     const IMPORTED_STROKE& aStroke, bool aFilled,
                     const COLOR4D& aFillColor = COLOR4D::UNSPECIFIED ) override;

    void AddEllipseArc( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius, const EDA_ANGLE& aRotation,
                        const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aEndAngle,
                        const IMPORTED_STROKE& aStroke ) override;

private:
    /// Scale and offset an imported point into the drawing sheet's millimetres.
    VECTOR2D mapPoint( const VECTOR2D& aPoint ) const;

    /// The line width to draw a stroke with, in millimetres.
    double mapLineWidth( const IMPORTED_STROKE& aStroke ) const;

    void addSegment( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth );

    /// Add a run of points as segments, closing back to the first point when asked.
    void addPolyline( const std::vector<VECTOR2D>& aPoints, double aWidth, bool aClose );

    /// Add a run of points as one contour of the filled polygon item.
    void addFilledPolygon( const std::vector<VECTOR2D>& aPoints );

    /// Walk an elliptical arc, which covers circles and arcs as well.
    ///
    /// The sweep is signed, because an arc runs the way its angle says, not the short way.
    std::vector<VECTOR2D> flattenEllipseArc( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius,
                                             const EDA_ANGLE& aRotation, double aStartRadians,
                                             double aSweepRadians ) const;

    std::vector<std::unique_ptr<DS_DATA_ITEM>> m_dataItems;

    /// All filled areas go into one item, the way a traced logo is one object.
    DS_DATA_ITEM_POLYGONS* m_polygons;
};

#endif // GRAPHICS_IMPORTER_PL_EDITOR_H

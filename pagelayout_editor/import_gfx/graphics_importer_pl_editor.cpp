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

#include "graphics_importer_pl_editor.h"

#include <algorithm>
#include <cmath>

#include <bezier_curves.h>

/// How far a flattened curve may sit from the real one, in mm.
static constexpr double FLATTEN_ERROR_MM = 0.01;


GRAPHICS_IMPORTER_PL_EDITOR::GRAPHICS_IMPORTER_PL_EDITOR() :
        m_polygons( nullptr )
{
    // A drawing sheet item holds millimetres, so there is no internal unit to convert to.
    m_millimeterToIu = 1.0;
}


VECTOR2D GRAPHICS_IMPORTER_PL_EDITOR::mapPoint( const VECTOR2D& aPoint ) const
{
    const VECTOR2D scale = GetScale();
    const VECTOR2D offset = GetImportOffsetMM();

    return VECTOR2D( aPoint.x * scale.x + offset.x, aPoint.y * scale.y + offset.y );
}


double GRAPHICS_IMPORTER_PL_EDITOR::mapLineWidth( const IMPORTED_STROKE& aStroke ) const
{
    const VECTOR2D scale = GetScale();
    const double   factor = ( std::abs( scale.x ) + std::abs( scale.y ) ) * 0.5;

    // A width the file did not give falls back to the one the dialog offers, and a zero
    // width leaves the drawing sheet to use its own default.
    if( aStroke.GetWidth() <= 0.0 )
        return GetLineWidthMM() * factor;

    return aStroke.GetWidth() * factor;
}


void GRAPHICS_IMPORTER_PL_EDITOR::addSegment( const VECTOR2D& aStart, const VECTOR2D& aEnd, double aWidth )
{
    auto item = std::make_unique<DS_DATA_ITEM>( DS_DATA_ITEM::DS_SEGMENT );

    // Imported geometry runs right and down, which is what the left top corner anchor means.
    item->SetStart( aStart.x, aStart.y, LT_CORNER );
    item->SetEnd( aEnd.x, aEnd.y, LT_CORNER );
    item->m_LineWidth = aWidth;

    m_dataItems.push_back( std::move( item ) );
}


void GRAPHICS_IMPORTER_PL_EDITOR::addPolyline( const std::vector<VECTOR2D>& aPoints, double aWidth, bool aClose )
{
    for( size_t ii = 1; ii < aPoints.size(); ii++ )
        addSegment( aPoints[ii - 1], aPoints[ii], aWidth );

    if( aClose && aPoints.size() > 2 )
        addSegment( aPoints.back(), aPoints.front(), aWidth );
}


void GRAPHICS_IMPORTER_PL_EDITOR::addFilledPolygon( const std::vector<VECTOR2D>& aPoints )
{
    if( aPoints.size() < 3 )
        return;

    // A traced logo is hundreds of contours but one thing to the user, so they all go into
    // a single item, which is also what the standalone converter writes.
    if( !m_polygons )
    {
        auto item = std::make_unique<DS_DATA_ITEM_POLYGONS>();

        item->SetStart( 0.0, 0.0, LT_CORNER );

        // Drawing sheet polygons are filled; this is what their outline is drawn with.
        item->m_LineWidth = 0.01;

        m_polygons = item.get();
        m_dataItems.push_back( std::move( item ) );
    }

    // A polygon's corners are kept relative to its position, which sits at the origin.
    for( const VECTOR2D& point : aPoints )
        m_polygons->AppendCorner( point );

    // The contour has to come back to where it started.
    m_polygons->AppendCorner( aPoints.front() );
    m_polygons->CloseContour();
}


std::vector<VECTOR2D> GRAPHICS_IMPORTER_PL_EDITOR::flattenEllipseArc( const VECTOR2D& aCenter, double aMajorRadius,
                                                                      double aMinorRadius, const EDA_ANGLE& aRotation,
                                                                      double aStartRadians, double aSweepRadians ) const
{
    std::vector<VECTOR2D> points;

    const double major = std::abs( aMajorRadius );
    const double minor = std::abs( aMinorRadius );
    const double largest = std::max( major, minor );

    if( largest <= 0.0 || aSweepRadians == 0.0 )
        return points;

    // Short enough that the chord never sits further from the curve than the error allows.
    const double ratio = std::clamp( 1.0 - FLATTEN_ERROR_MM / largest, -1.0, 1.0 );
    const double step = std::max( 2.0 * std::acos( ratio ), 1e-3 );
    const int    count = std::max( 2, (int) std::ceil( std::abs( aSweepRadians ) / step ) );

    const double rotCos = std::cos( aRotation.AsRadians() );
    const double rotSin = std::sin( aRotation.AsRadians() );

    for( int ii = 0; ii <= count; ii++ )
    {
        const double angle = aStartRadians + aSweepRadians * ii / count;
        const double x = major * std::cos( angle );
        const double y = minor * std::sin( angle );

        points.emplace_back( aCenter.x + x * rotCos - y * rotSin, aCenter.y + x * rotSin + y * rotCos );
    }

    return points;
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddLine( const VECTOR2D& aOrigin, const VECTOR2D& aEnd,
                                           const IMPORTED_STROKE& aStroke )
{
    addSegment( mapPoint( aOrigin ), mapPoint( aEnd ), mapLineWidth( aStroke ) );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddCircle( const VECTOR2D& aCenter, double aRadius, const IMPORTED_STROKE& aStroke,
                                             bool aFilled, const COLOR4D& aFillColor )
{
    AddEllipse( aCenter, aRadius, aRadius, ANGLE_0, aStroke, aFilled, aFillColor );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart, const EDA_ANGLE& aAngle,
                                          const IMPORTED_STROKE& aStroke )
{
    const VECTOR2D startVec = aStart - aCenter;
    const double   radius = startVec.EuclideanNorm();
    const double   startAngle = std::atan2( startVec.y, startVec.x );

    // The arc runs from its start by the angle given, the way the other importers turn it.
    std::vector<VECTOR2D> points =
            flattenEllipseArc( aCenter, radius, radius, ANGLE_0, startAngle, -aAngle.AsRadians() );

    for( VECTOR2D& point : points )
        point = mapPoint( point );

    addPolyline( points, mapLineWidth( aStroke ), false );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddPolygon( const std::vector<VECTOR2D>& aVertices, const IMPORTED_STROKE& aStroke,
                                              bool aFilled, const COLOR4D& aFillColor )
{
    std::vector<VECTOR2D> points;

    points.reserve( aVertices.size() );

    for( const VECTOR2D& vertex : aVertices )
        points.push_back( mapPoint( vertex ) );

    if( aFilled )
        addFilledPolygon( points );
    else
        addPolyline( points, mapLineWidth( aStroke ), true );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddText( const VECTOR2D& aOrigin, const wxString& aText, double aHeight,
                                           double aWidth, double aThickness, double aOrientation,
                                           GR_TEXT_H_ALIGN_T aHJustify, GR_TEXT_V_ALIGN_T aVJustify,
                                           const COLOR4D& aColor )
{
    auto item = std::make_unique<DS_DATA_ITEM_TEXT>( aText );

    const VECTOR2D position = mapPoint( aOrigin );
    const VECTOR2D scale = GetScale();

    item->SetStart( position.x, position.y, LT_CORNER );

    // A size of zero is what the drawing sheet spells "use the default", which beats the
    // invisible text a file that gives no size would otherwise produce.
    if( aWidth > 0.0 && aHeight > 0.0 )
        item->m_TextSize = VECTOR2D( aWidth * std::abs( scale.x ), aHeight * std::abs( scale.y ) );

    item->m_LineWidth = aThickness * std::abs( scale.x );
    item->m_Orient = aOrientation;
    item->m_Hjustify = aHJustify;
    item->m_Vjustify = aVJustify;

    m_dataItems.push_back( std::move( item ) );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                                             const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                                             const IMPORTED_STROKE& aStroke )
{
    std::vector<VECTOR2D> controls = { mapPoint( aStart ), mapPoint( aBezierControl1 ), mapPoint( aBezierControl2 ),
                                       mapPoint( aEnd ) };
    std::vector<VECTOR2D> points;

    BEZIER_POLY( controls ).GetPoly( points, FLATTEN_ERROR_MM );

    addPolyline( points, mapLineWidth( aStroke ), false );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddEllipse( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius,
                                              const EDA_ANGLE& aRotation, const IMPORTED_STROKE& aStroke, bool aFilled,
                                              const COLOR4D& aFillColor )
{
    std::vector<VECTOR2D> points = flattenEllipseArc( aCenter, aMajorRadius, aMinorRadius, aRotation, 0.0, 2.0 * M_PI );

    if( points.empty() )
        return;

    // The walk closes on its own start, which a closed shape does not want repeated.
    points.pop_back();

    for( VECTOR2D& point : points )
        point = mapPoint( point );

    if( aFilled )
        addFilledPolygon( points );
    else
        addPolyline( points, mapLineWidth( aStroke ), true );
}


void GRAPHICS_IMPORTER_PL_EDITOR::AddEllipseArc( const VECTOR2D& aCenter, double aMajorRadius, double aMinorRadius,
                                                 const EDA_ANGLE& aRotation, const EDA_ANGLE& aStartAngle,
                                                 const EDA_ANGLE& aEndAngle, const IMPORTED_STROKE& aStroke )
{
    double sweep = ( aEndAngle - aStartAngle ).AsRadians();

    // An ellipse arc is given by its two ends, and runs between them the positive way.
    while( sweep <= 0.0 )
        sweep += 2.0 * M_PI;

    std::vector<VECTOR2D> points =
            flattenEllipseArc( aCenter, aMajorRadius, aMinorRadius, aRotation, aStartAngle.AsRadians(), sweep );

    for( VECTOR2D& point : points )
        point = mapPoint( point );

    addPolyline( points, mapLineWidth( aStroke ), false );
}

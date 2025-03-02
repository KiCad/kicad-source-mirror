/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Janito V. Ferreira Filho
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

#include "svg_import_plugin.h"

#include <nanosvg.h>
#include <algorithm>
#include <cmath>
#include <locale_io.h>

#include <eda_item.h>
#include "graphics_importer.h"

static const int SVG_DPI = 96;

static VECTOR2D calculateBezierBoundingBoxExtremity( const float* aCurvePoints,
        std::function< const float&( const float&, const float& ) > comparator );
static float calculateBezierSegmentationThreshold( const float* aCurvePoints );
static void segmentBezierCurve( const VECTOR2D& aStart, const VECTOR2D& aEnd, float aOffset,
        float aStep, const float* aCurvePoints, float aSegmentationThreshold,
        std::vector< VECTOR2D >& aGeneratedPoints );
static void createNewBezierCurveSegments( const VECTOR2D& aStart, const VECTOR2D& aMiddle,
        const VECTOR2D& aEnd, float aOffset, float aStep, const float* aCurvePoints,
        float aSegmentationThreshold, std::vector< VECTOR2D >& aGeneratedPoints );
static VECTOR2D getBezierPoint( const float* aCurvePoints, float aStep );
static VECTOR2D getPoint( const float* aPointCoordinates );
static VECTOR2D getPointInLine( const VECTOR2D& aLineStart, const VECTOR2D& aLineEnd,
        float aDistance );
static float distanceFromPointToLine( const VECTOR2D& aPoint, const VECTOR2D& aLineStart,
        const VECTOR2D& aLineEnd );


bool SVG_IMPORT_PLUGIN::Load( const wxString& aFileName )
{
    wxCHECK( m_importer, false );

    LOCALE_IO toggle; // switch on/off the locale "C" notation

    // 1- wxFopen takes care of unicode filenames across platforms
    // 2 - nanosvg (exactly nsvgParseFromFile) expects a binary file (exactly the CRLF eof must
    // not be replaced by LF and changes the byte count) in one validity test,
    // so open it in binary mode.
    FILE* fp = wxFopen( aFileName, wxT( "rb" ) );

    if( fp == nullptr )
        return false;

    // nsvgParseFromFile will close the file after reading
    m_parsedImage = nsvgParseFromFile( fp, "mm", SVG_DPI );

    wxCHECK( m_parsedImage, false );

    return true;
}


bool SVG_IMPORT_PLUGIN::LoadFromMemory( const wxMemoryBuffer& aMemBuffer )
{
    wxCHECK( m_importer, false );

    LOCALE_IO toggle; // switch on/off the locale "C" notation

    std::string str( reinterpret_cast<char*>( aMemBuffer.GetData() ), aMemBuffer.GetDataLen() );
    wxCHECK( str.data()[aMemBuffer.GetDataLen()] == '\0', false );

    // nsvgParse will modify the string data
    m_parsedImage = nsvgParse( str.data(), "mm", SVG_DPI );

    wxCHECK( m_parsedImage, false );

    return true;
}


bool SVG_IMPORT_PLUGIN::Import()
{
    auto alpha =
            []( unsigned int color )
            {
                return color >> 24;
            };

    for( NSVGshape* shape = m_parsedImage->shapes; shape != nullptr; shape = shape->next )
    {
        if( !( shape->flags & NSVG_FLAGS_VISIBLE ) )
            continue;

        if( shape->stroke.type == NSVG_PAINT_NONE && shape->fill.type == NSVG_PAINT_NONE )
            continue;

        double lineWidth = shape->stroke.type != NSVG_PAINT_NONE ? shape->strokeWidth : -1;
        bool   filled = shape->fill.type != NSVG_PAINT_NONE && alpha( shape->fill.color ) > 0;

        COLOR4D fillColor = COLOR4D::UNSPECIFIED;

        if( shape->fill.type == NSVG_PAINT_COLOR )
        {
            unsigned int icolor = shape->fill.color;

            fillColor.r = std::clamp( ( icolor >> 0 ) & 0xFF, 0u, 255u ) / 255.0;
            fillColor.g = std::clamp( ( icolor >> 8 ) & 0xFF, 0u, 255u ) / 255.0;
            fillColor.b = std::clamp( ( icolor >> 16 ) & 0xFF, 0u, 255u ) / 255.0;
            fillColor.a = std::clamp( ( icolor >> 24 ) & 0xFF, 0u, 255u ) / 255.0;

            // nanosvg probably didn't read it properly, use default
            if( fillColor == COLOR4D::BLACK )
                fillColor = COLOR4D::UNSPECIFIED;
        }

        COLOR4D strokeColor = COLOR4D::UNSPECIFIED;

        if( shape->stroke.type == NSVG_PAINT_COLOR )
        {
            unsigned int icolor = shape->stroke.color;

            strokeColor.r = std::clamp( ( icolor >> 0 ) & 0xFF, 0u, 255u ) / 255.0;
            strokeColor.g = std::clamp( ( icolor >> 8 ) & 0xFF, 0u, 255u ) / 255.0;
            strokeColor.b = std::clamp( ( icolor >> 16 ) & 0xFF, 0u, 255u ) / 255.0;
            strokeColor.a = std::clamp( ( icolor >> 24 ) & 0xFF, 0u, 255u ) / 255.0;

            // nanosvg probably didn't read it properly, use default
            if( strokeColor == COLOR4D::BLACK )
                strokeColor = COLOR4D::UNSPECIFIED;
        }

        LINE_STYLE dashType = LINE_STYLE::SOLID;

        if( shape->strokeDashCount > 0 )
        {
            float* dashArray = shape->strokeDashArray;

            int dotCount = 0;
            int dashCount = 0;

            const float dashThreshold = shape->strokeWidth * 1.9f;

            for( int i = 0; i < shape->strokeDashCount; i += 2 )
            {
                if( dashArray[i] < dashThreshold )
                    dotCount++;
                else
                    dashCount++;
            }

            if( dotCount > 0 && dashCount == 0 )
                dashType = LINE_STYLE::DOT;
            else if( dotCount == 0 && dashCount > 0 )
                dashType = LINE_STYLE::DASH;
            else if( dotCount == 1 && dashCount == 1 )
                dashType = LINE_STYLE::DASHDOT;
            else if( dotCount == 2 && dashCount == 1 )
                dashType = LINE_STYLE::DASHDOTDOT;
        }

        IMPORTED_STROKE stroke( lineWidth, dashType, strokeColor );

        GRAPHICS_IMPORTER::POLY_FILL_RULE rule = GRAPHICS_IMPORTER::PF_NONZERO;

        switch( shape->fillRule )
        {
            case NSVG_FILLRULE_NONZERO: rule = GRAPHICS_IMPORTER::PF_NONZERO; break;
            case NSVG_FILLRULE_EVENODD: rule = GRAPHICS_IMPORTER::PF_EVEN_ODD; break;
            default: break;
        }

        m_internalImporter.NewShape( rule );

        for( NSVGpath* path = shape->paths; path != nullptr; path = path->next )
        {
            if( filled && !path->closed )
            {
                // KiCad doesn't support a single object representing a filled shape that is
                // *not* closed so create a filled, closed shape for the fill, and an unfilled,
                // open shape for the outline
                static IMPORTED_STROKE noStroke( -1, LINE_STYLE::SOLID, COLOR4D::UNSPECIFIED );
                const bool             closed = true;

                DrawPath( path->pts, path->npts, closed, noStroke, true, fillColor );

                if( stroke.GetWidth() > 0 )
                    DrawPath( path->pts, path->npts, !closed, stroke, false, COLOR4D::UNSPECIFIED );
            }
            else
            {
                // Either the shape has fill and no stroke, so we implicitly close it (for no
                // difference), or it's really closed.
                // We could choose to import a not-filled, closed outline as splines to keep the
                // original editability and control points, but currently we don't.
                const bool closed = path->closed || filled;

                DrawPath( path->pts, path->npts, closed, stroke, filled, fillColor );
            }
        }
    }

    m_internalImporter.PostprocessNestedPolygons();
    wxCHECK( m_importer, false );
    m_internalImporter.ImportTo( *m_importer );

    return true;
}


double SVG_IMPORT_PLUGIN::GetImageHeight() const
{
    if( !m_parsedImage )
    {
        wxASSERT_MSG( false, wxT( "Image must have been loaded before checking height" ) );
        return 0.0;
    }

    return m_parsedImage->height / SVG_DPI * inches2mm;
}


double SVG_IMPORT_PLUGIN::GetImageWidth() const
{
    if( !m_parsedImage )
    {
        wxASSERT_MSG( false, wxT( "Image must have been loaded before checking width" ) );
        return 0.0;
    }

    return m_parsedImage->width / SVG_DPI * inches2mm;
}


BOX2D SVG_IMPORT_PLUGIN::GetImageBBox() const
{
    BOX2D bbox;

    if( !m_parsedImage || !m_parsedImage->shapes )
    {
        wxASSERT_MSG( false, wxT( "Image must have been loaded before getting bbox" ) );
        return bbox;
    }

    for( NSVGshape* shape = m_parsedImage->shapes; shape != nullptr; shape = shape->next )
    {
        BOX2D shapeBbox;
        float( &bounds )[4] = shape->bounds;

        shapeBbox.SetOrigin( bounds[0], bounds[1] );
        shapeBbox.SetEnd( bounds[2], bounds[3] );

        bbox.Merge( shapeBbox );
    }

    return bbox;
}


static void GatherInterpolatedCubicBezierCurve( const float*           aPoints,
                                                std::vector<VECTOR2D>& aGeneratedPoints )
{
    auto start = getBezierPoint( aPoints, 0.0f );
    auto end = getBezierPoint( aPoints, 1.0f );
    auto segmentationThreshold = calculateBezierSegmentationThreshold( aPoints );

    if( aGeneratedPoints.size() == 0 || aGeneratedPoints.back() != start )
        aGeneratedPoints.push_back( start );

    segmentBezierCurve( start, end, 0.0f, 0.5f, aPoints, segmentationThreshold, aGeneratedPoints );
    aGeneratedPoints.push_back( end );
}


static void GatherInterpolatedCubicBezierPath( const float* aPoints, int aNumPoints,
                                               std::vector<VECTOR2D>& aGeneratedPoints )
{
    const int    pointsPerSegment = 4;
    const int    curveSpecificPointsPerSegment = 3;
    const int    curveSpecificCoordinatesPerSegment = 2 * curveSpecificPointsPerSegment;
    const float* currentPoints = aPoints;
    int          remainingPoints = aNumPoints;

    while( remainingPoints >= pointsPerSegment )
    {
        GatherInterpolatedCubicBezierCurve( currentPoints, aGeneratedPoints );
        currentPoints += curveSpecificCoordinatesPerSegment;
        remainingPoints -= curveSpecificPointsPerSegment;
    }
}


void SVG_IMPORT_PLUGIN::DrawPath( const float* aPoints, int aNumPoints, bool aClosedPath,
                                  const IMPORTED_STROKE& aStroke, bool aFilled,
                                  const COLOR4D& aFillColor )
{
    bool drewPolygon = false;

    if( aClosedPath )
    {
        // Closed paths are always polygons, which mean they need to be interpolated
        std::vector<VECTOR2D> collectedPathPoints;

        if( aNumPoints > 0 )
            GatherInterpolatedCubicBezierPath( aPoints, aNumPoints, collectedPathPoints );

        if( collectedPathPoints.size() > 2 )
        {
            DrawPolygon( collectedPathPoints, aStroke, aFilled, aFillColor );
            drewPolygon = true;
        }
    }

    if( !drewPolygon )
    {
        DrawSplinePath( aPoints, aNumPoints, aStroke );
    }
}


void SVG_IMPORT_PLUGIN::DrawSplinePath( const float* aCoords, int aNumPoints,
                                        const IMPORTED_STROKE& aStroke )
{
    // NanoSVG just gives us the points of the Bezier curves, so we have to
    // decide whether to draw lines or splines based on the points we have.

    const int    pointsPerSegment = 4;
    const int    curveSpecificPointsPerSegment = 3;
    const int    curveSpecificCoordinatesPerSegment = 2 * curveSpecificPointsPerSegment;
    const float* currentCoords = aCoords;
    int          remainingPoints = aNumPoints;

    while( remainingPoints >= pointsPerSegment )
    {
        VECTOR2D start = getPoint( currentCoords );
        VECTOR2D c1 = getPoint( currentCoords + 2 );
        VECTOR2D c2 = getPoint( currentCoords + 4 );
        VECTOR2D end = getPoint( currentCoords + 6 );

        // Add as a spline and the importer will decide whether to draw it as a spline or as lines
        m_internalImporter.AddSpline( start, c1, c2, end, aStroke );

        currentCoords += curveSpecificCoordinatesPerSegment;
        remainingPoints -= curveSpecificPointsPerSegment;
    }
}


void SVG_IMPORT_PLUGIN::DrawPolygon( const std::vector<VECTOR2D>& aPoints,
                                     const IMPORTED_STROKE& aStroke, bool aFilled,
                                     const COLOR4D& aFillColor )
{
    m_internalImporter.AddPolygon( aPoints, aStroke, aFilled, aFillColor );
}


void SVG_IMPORT_PLUGIN::DrawLineSegments( const std::vector<VECTOR2D>& aPoints,
                                          const IMPORTED_STROKE&       aStroke )
{
    unsigned int numLineStartPoints = aPoints.size() - 1;

    for( unsigned int pointIndex = 0; pointIndex < numLineStartPoints; ++pointIndex )
        m_internalImporter.AddLine( aPoints[pointIndex], aPoints[pointIndex + 1], aStroke );
}


static VECTOR2D getPoint( const float* aPointCoordinates )
{
    return VECTOR2D( aPointCoordinates[0], aPointCoordinates[1] );
}


static VECTOR2D getBezierPoint( const float* aPoints, float aStep )
{
    const int coordinatesPerPoint = 2;

    auto firstCubicPoint = getPoint( aPoints );
    auto secondCubicPoint = getPoint( aPoints + 1 * coordinatesPerPoint );
    auto thirdCubicPoint = getPoint( aPoints + 2 * coordinatesPerPoint );
    auto fourthCubicPoint = getPoint( aPoints + 3 * coordinatesPerPoint );

    auto firstQuadraticPoint = getPointInLine( firstCubicPoint, secondCubicPoint, aStep );
    auto secondQuadraticPoint = getPointInLine( secondCubicPoint, thirdCubicPoint, aStep );
    auto thirdQuadraticPoint = getPointInLine( thirdCubicPoint, fourthCubicPoint, aStep );

    auto firstLinearPoint = getPointInLine( firstQuadraticPoint, secondQuadraticPoint, aStep );
    auto secondLinearPoint = getPointInLine( secondQuadraticPoint, thirdQuadraticPoint, aStep );

    return getPointInLine( firstLinearPoint, secondLinearPoint, aStep );
}


static VECTOR2D getPointInLine( const VECTOR2D& aLineStart, const VECTOR2D& aLineEnd,
                                float aDistance )
{
    return aLineStart + ( aLineEnd - aLineStart ) * aDistance;
}


static float calculateBezierSegmentationThreshold( const float* aCurvePoints )
{
    using comparatorFunction = const float&(*)( const float&, const float& );

    auto minimumComparator = static_cast< comparatorFunction >( &std::min );
    auto maximumComparator = static_cast< comparatorFunction >( &std::max );

    VECTOR2D minimum = calculateBezierBoundingBoxExtremity( aCurvePoints, minimumComparator );
    VECTOR2D maximum = calculateBezierBoundingBoxExtremity( aCurvePoints, maximumComparator );
    VECTOR2D boundingBoxDimensions = maximum - minimum;

    return 0.001 * std::max( boundingBoxDimensions.x, boundingBoxDimensions.y );
}


static VECTOR2D calculateBezierBoundingBoxExtremity( const float* aCurvePoints,
        std::function< const float&( const float&, const float& ) > comparator )
{
    float x = aCurvePoints[0];
    float y = aCurvePoints[1];

    for( int pointIndex = 1; pointIndex < 3; ++pointIndex )
    {
        x = comparator( x, aCurvePoints[ 2 * pointIndex ] );
        y = comparator( y, aCurvePoints[ 2 * pointIndex + 1 ] );
    }

    return VECTOR2D( x, y );
}


static void segmentBezierCurve( const VECTOR2D& aStart, const VECTOR2D& aEnd, float aOffset,
                                float aStep, const float* aCurvePoints,
                                float aSegmentationThreshold,
                                std::vector< VECTOR2D >& aGeneratedPoints )
{
    VECTOR2D middle = getBezierPoint( aCurvePoints, aOffset + aStep );
    float distanceToPreviousSegment = distanceFromPointToLine( middle, aStart, aEnd );

    if( distanceToPreviousSegment > aSegmentationThreshold )
    {
        createNewBezierCurveSegments( aStart, middle, aEnd, aOffset, aStep, aCurvePoints,
                                      aSegmentationThreshold, aGeneratedPoints );
    }
}


static void createNewBezierCurveSegments( const VECTOR2D& aStart, const VECTOR2D& aMiddle,
                                          const VECTOR2D& aEnd, float aOffset, float aStep,
                                          const float* aCurvePoints, float aSegmentationThreshold,
                                          std::vector< VECTOR2D >& aGeneratedPoints )
{
    float newStep = aStep / 2.f;
    float offsetAfterMiddle = aOffset + aStep;

    segmentBezierCurve( aStart, aMiddle, aOffset, newStep, aCurvePoints, aSegmentationThreshold,
                        aGeneratedPoints );

    aGeneratedPoints.push_back( aMiddle );

    segmentBezierCurve( aMiddle, aEnd, offsetAfterMiddle, newStep, aCurvePoints,
                        aSegmentationThreshold, aGeneratedPoints );
}


static float distanceFromPointToLine( const VECTOR2D& aPoint, const VECTOR2D& aLineStart,
                                      const VECTOR2D& aLineEnd )
{
    auto lineDirection = aLineEnd - aLineStart;
    auto lineNormal = lineDirection.Perpendicular().Resize( 1.f );
    auto lineStartToPoint = aPoint - aLineStart;

    auto distance = lineNormal.Dot( lineStartToPoint );

    return fabs( distance );
}


void SVG_IMPORT_PLUGIN::ReportMsg( const wxString& aMessage )
{
    // Add message to keep trace of not handled svg entities
    m_messages += aMessage;
    m_messages += '\n';
}

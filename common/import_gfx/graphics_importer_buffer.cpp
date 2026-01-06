/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Janito Vaqueiro Ferreira Filho <janito.vff@gmail.com>
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

#include <eda_item.h>

#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>

#include "graphics_importer_buffer.h"

using namespace std;

template <typename T, typename... Args>
static std::unique_ptr<T> make_shape( const Args&... aArguments )
{
    return std::make_unique<T>( aArguments... );
}

void GRAPHICS_IMPORTER_BUFFER::AddLine( const VECTOR2D& aStart, const VECTOR2D& aEnd,
                                        const IMPORTED_STROKE& aStroke )
{
    m_shapes.push_back( make_shape<IMPORTED_LINE>( aStart, aEnd, aStroke ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddCircle( const VECTOR2D& aCenter, double aRadius,
                                          const IMPORTED_STROKE& aStroke, bool aFilled,
                                          const COLOR4D& aFillColor )
{
    m_shapes.push_back(
            make_shape<IMPORTED_CIRCLE>( aCenter, aRadius, aStroke, aFilled, aFillColor ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddArc( const VECTOR2D& aCenter, const VECTOR2D& aStart,
                                       const EDA_ANGLE& aAngle, const IMPORTED_STROKE& aStroke )
{
    m_shapes.push_back( make_shape<IMPORTED_ARC>( aCenter, aStart, aAngle, aStroke ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddPolygon( const std::vector<VECTOR2D>& aVertices,
                                           const IMPORTED_STROKE& aStroke, bool aFilled,
                                           const COLOR4D& aFillColor )
{
    m_shapes.push_back( make_shape<IMPORTED_POLYGON>( aVertices, aStroke, aFilled, aFillColor ) );

    m_shapes.back()->SetParentShapeIndex( m_shapeFillRules.size() - 1 );
}


void GRAPHICS_IMPORTER_BUFFER::AddText( const VECTOR2D& aOrigin, const wxString& aText,
                                        double aHeight, double aWidth, double aThickness,
                                        double aOrientation, GR_TEXT_H_ALIGN_T aHJustify,
                                        GR_TEXT_V_ALIGN_T aVJustify, const COLOR4D& aColor )
{
    m_shapes.push_back( make_shape<IMPORTED_TEXT>( aOrigin, aText, aHeight, aWidth, aThickness,
                                                   aOrientation, aHJustify, aVJustify, aColor ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddSpline( const VECTOR2D& aStart, const VECTOR2D& aBezierControl1,
                                          const VECTOR2D& aBezierControl2, const VECTOR2D& aEnd,
                                          const IMPORTED_STROKE& aStroke )
{
    m_shapes.push_back( make_shape<IMPORTED_SPLINE>( aStart, aBezierControl1, aBezierControl2, aEnd,
                                                     aStroke ) );
}


void GRAPHICS_IMPORTER_BUFFER::AddShape( std::unique_ptr<IMPORTED_SHAPE>& aShape )
{
    m_shapes.push_back( std::move( aShape ) );
}


void GRAPHICS_IMPORTER_BUFFER::ImportTo( GRAPHICS_IMPORTER& aImporter )
{
    BOX2D boundingBox;

    for( std::unique_ptr<IMPORTED_SHAPE>& shape : m_shapes )
    {
        BOX2D box = shape->GetBoundingBox();

        if( box.IsValid() )
            boundingBox.Merge( box );
    }

    boundingBox.SetOrigin( boundingBox.GetPosition().x * aImporter.GetScale().x,
                           boundingBox.GetPosition().y * aImporter.GetScale().y );
    boundingBox.SetSize( boundingBox.GetSize().x * aImporter.GetScale().x,
                         boundingBox.GetSize().y * aImporter.GetScale().y );

    // Check that the scaled graphics fit in the KiCad numeric limits
    if( boundingBox.GetSize().x * aImporter.GetMillimeterToIuFactor()
                > std::numeric_limits<int>::max()
      || boundingBox.GetSize().y * aImporter.GetMillimeterToIuFactor()
                > std::numeric_limits<int>::max() )
    {
        double scale_factor = std::numeric_limits<int>::max() /
                              ( aImporter.GetMillimeterToIuFactor() + 100 );
        double max_scale = std::max( scale_factor / boundingBox.GetSize().x,
                                     scale_factor / boundingBox.GetSize().y );
        aImporter.ReportMsg( wxString::Format( _( "Imported graphic is too large. Maximum scale is %f" ),
                                               max_scale ) );
        return;
    }
    // They haven't set the import offset, so we set it to the bounding box origin to keep
    // the graphics in the KiCad drawing area.
    else if( aImporter.GetImportOffsetMM() == VECTOR2D( 0, 0 ) )
    {
        double iuFactor = aImporter.GetMillimeterToIuFactor();

        if( boundingBox.GetRight() * iuFactor > std::numeric_limits<int>::max()
            || boundingBox.GetBottom() * iuFactor > std::numeric_limits<int>::max()
            || boundingBox.GetLeft() * iuFactor < std::numeric_limits<int>::min()
            || boundingBox.GetTop() * iuFactor < std::numeric_limits<int>::min() )
        {
            VECTOR2D offset = boundingBox.GetOrigin();
            aImporter.SetImportOffsetMM( -offset );
        }
    }
    else
    {
        double   total_scale_x = aImporter.GetScale().x * aImporter.GetMillimeterToIuFactor();
        double   total_scale_y = aImporter.GetScale().y * aImporter.GetMillimeterToIuFactor();

        double max_offset_x = ( aImporter.GetImportOffsetMM().x + boundingBox.GetRight() ) * total_scale_x;
        double max_offset_y = ( aImporter.GetImportOffsetMM().y + boundingBox.GetBottom() ) * total_scale_y;
        double min_offset_x = ( aImporter.GetImportOffsetMM().x + boundingBox.GetLeft() ) * total_scale_x;
        double min_offset_y = ( aImporter.GetImportOffsetMM().y + boundingBox.GetTop() ) * total_scale_y;

        VECTOR2D newOffset = aImporter.GetImportOffsetMM();
        bool needsAdjustment = false;

        if( max_offset_x >= std::numeric_limits<int>::max() )
        {
            newOffset.x -= ( max_offset_x - std::numeric_limits<int>::max() + 100.0 ) / total_scale_x;
            needsAdjustment = true;
        }
        else if( min_offset_x <= std::numeric_limits<int>::min() )
        {
            newOffset.x -= ( min_offset_x - std::numeric_limits<int>::min() - 100 ) / total_scale_x;
            needsAdjustment = true;
        }

        if( max_offset_y >= std::numeric_limits<int>::max() )
        {
            newOffset.y -= ( max_offset_y - std::numeric_limits<int>::max() + 100 ) / total_scale_y;
            needsAdjustment = true;
        }
        else if( min_offset_y <= std::numeric_limits<int>::min() )
        {
            newOffset.y -= ( min_offset_y - std::numeric_limits<int>::min() - 100 ) / total_scale_y;
            needsAdjustment = true;
        }

        if( needsAdjustment )
        {
            aImporter.ReportMsg( wxString::Format( _( "Import offset adjusted to (%f, %f) to fit "
                                                      "within numeric limits" ),
                                                   newOffset.x, newOffset.y ) );
            aImporter.SetImportOffsetMM( newOffset );
        }
    }

    for( std::unique_ptr<IMPORTED_SHAPE>& shape : m_shapes )
        shape->ImportTo( aImporter );
}

// converts a single SVG-style polygon (multiple outlines, hole detection based on orientation,
// custom fill rule) to a format that can be digested by KiCad (single outline, fractured).
static void convertPolygon( std::list<std::unique_ptr<IMPORTED_SHAPE>>& aShapes,
                            std::vector<IMPORTED_POLYGON*>&             aPaths,
                            GRAPHICS_IMPORTER::POLY_FILL_RULE           aFillRule,
                            const IMPORTED_STROKE& aStroke, bool aFilled,
                            const COLOR4D& aFillColor )
{
    double minX = std::numeric_limits<double>::max();
    double minY = minX;
    double maxX = std::numeric_limits<double>::min();
    double maxY = maxX;

    // as Clipper/SHAPE_POLY_SET uses ints we first need to upscale to a reasonably large size
    // (in integer coordinates) to avoid losing accuracy.
    const double convert_scale = 1000000000.0;

    for( IMPORTED_POLYGON* path : aPaths )
    {
        for( VECTOR2D& v : path->Vertices() )
        {
            minX = std::min( minX, v.x );
            minY = std::min( minY, v.y );
            maxX = std::max( maxX, v.x );
            maxY = std::max( maxY, v.y );
        }
    }

    double origW = ( maxX - minX );
    double origH = ( maxY - minY );
    double upscaledW, upscaledH;

    wxCHECK( origH && origW, /* void */ );

    if( origW > origH )
    {
        upscaledW = convert_scale;
        upscaledH = ( origH == 0.0f ? 0.0 : origH * convert_scale / origW );
    }
    else
    {
        upscaledH = convert_scale;
        upscaledW = ( origW == 0.0f ? 0.0 : origW * convert_scale / origH );
    }

    std::vector<IMPORTED_POLYGON*> openPaths;
    std::vector<SHAPE_LINE_CHAIN>  upscaledPaths;

    for( IMPORTED_POLYGON* path : aPaths )
    {
        if( path->Vertices().size() < 3 )
        {
            openPaths.push_back( path );
            continue;
        }

        SHAPE_LINE_CHAIN lc;

        for( VECTOR2D& v : path->Vertices() )
        {
            int xp = KiROUND( ( v.x - minX ) * ( upscaledW / origW ) );
            int yp = KiROUND( ( v.y - minY ) * ( upscaledH / origH ) );
            lc.Append( xp, yp );
        }

        lc.SetClosed( true );
        upscaledPaths.push_back( lc );
    }

    SHAPE_POLY_SET result;
    result.BuildPolysetFromOrientedPaths( upscaledPaths,
                                          aFillRule == GRAPHICS_IMPORTER::PF_EVEN_ODD );
    result.Fracture();

    for( int outl = 0; outl < result.OutlineCount(); outl++ )
    {
        const SHAPE_LINE_CHAIN& ro = result.COutline( outl );
        std::vector<VECTOR2D>   pts;

        for( int i = 0; i < ro.PointCount(); i++ )
        {
            double xp = (double) ro.CPoint( i ).x * ( origW / upscaledW ) + minX;
            double yp = (double) ro.CPoint( i ).y * ( origH / upscaledH ) + minY;
            pts.emplace_back( VECTOR2D( xp, yp ) );
        }

        aShapes.push_back( std::make_unique<IMPORTED_POLYGON>( pts, aStroke, aFilled, aFillColor ) );
    }

    for( IMPORTED_POLYGON* openPath : openPaths )
        aShapes.push_back( std::make_unique<IMPORTED_POLYGON>( *openPath ) );
}


void GRAPHICS_IMPORTER_BUFFER::PostprocessNestedPolygons()
{
    int             curShapeIdx = -1;
    IMPORTED_STROKE lastStroke;
    bool            lastFilled = false;
    COLOR4D         lastFillColor = COLOR4D::UNSPECIFIED;

    std::list<std::unique_ptr<IMPORTED_SHAPE>> newShapes;
    std::vector<IMPORTED_POLYGON*>             polypaths;

    for( std::unique_ptr<IMPORTED_SHAPE>& shape : m_shapes )
    {
        IMPORTED_POLYGON* poly = dynamic_cast<IMPORTED_POLYGON*>( shape.get() );

        if( !poly || poly->GetParentShapeIndex() < 0 )
        {
            newShapes.push_back( shape->clone() );
            continue;
        }

        int index = poly->GetParentShapeIndex();

        if( index != curShapeIdx && curShapeIdx >= 0 )
        {
            convertPolygon( newShapes, polypaths, m_shapeFillRules[curShapeIdx], lastStroke,
                            lastFilled, lastFillColor );

            polypaths.clear();
        }

        curShapeIdx = index;
        lastStroke = poly->GetStroke();
        lastFilled = poly->IsFilled();
        lastFillColor = poly->GetFillColor();
        polypaths.push_back( poly );
    }

    if( curShapeIdx >= 0 )
    {
        convertPolygon( newShapes, polypaths, m_shapeFillRules[curShapeIdx], lastStroke, lastFilled,
                        lastFillColor );
    }

    m_shapes.swap( newShapes );
}

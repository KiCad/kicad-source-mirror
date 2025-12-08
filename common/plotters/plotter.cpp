/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

/**
 * @file plotter.cpp
 * @brief KiCad: Base of all the specialized plotters
 * the class PLOTTER handle basic functions to plot schematic and boards
 * with different plot formats.
 *
 * There are currently engines for:
 * POSTSCRIPT
 * GERBER
 * DXF
 * an SVG 'plot' is also provided along with the 'print' function by wx, but
 * is not handled here.
 */

#include <trigo.h>
#include <plotters/plotter.h>
#include <text_eval/text_eval_wrapper.h>
#include <geometry/shape_line_chain.h>
#include <bezier_curves.h>
#include <callback_gal.h>
#include <math/util.h>      // for KiROUND
#include <convert_basic_shapes_to_polygon.h>

PLOTTER::PLOTTER( const PROJECT* aProject ) :
    m_project( aProject )
{
    m_plotScale = 1;
    m_currentPenWidth = -1;       // To-be-set marker
    m_penState = 'Z';             // End-of-path idle
    m_plotMirror = false;       // Plot mirror option flag
    m_mirrorIsHorizontal = true;
    m_yaxisReversed = false;
    m_outputFile = nullptr;
    m_colorMode = false;          // Starts as a BW plot
    m_negativeMode = false;

    // Temporary init to avoid not initialized vars, will be set later
    m_IUsPerDecimil = 1;        // will be set later to the actual value
    m_iuPerDeviceUnit = 1;        // will be set later to the actual value
    m_renderSettings = nullptr;
    m_layer = PCB_LAYER_ID::UNDEFINED_LAYER;
}


PLOTTER::~PLOTTER()
{
    // Emergency cleanup, but closing the file is usually made in EndPlot().
    if( m_outputFile )
        fclose( m_outputFile );
}


bool PLOTTER::OpenFile( const wxString& aFullFilename )
{
    m_filename = aFullFilename;

    wxASSERT( !m_outputFile );

    // Open the file in text mode (not suitable for all plotters but only for most of them.
    m_outputFile = wxFopen( m_filename, wxT( "wt" ) );

    if( m_outputFile == nullptr )
        return false ;

    return true;
}


VECTOR2D PLOTTER::userToDeviceCoordinates( const VECTOR2I& aCoordinate )
{
    VECTOR2I pos = aCoordinate - m_plotOffset;

    double x = pos.x * m_plotScale;
    double y = ( m_paperSize.y - pos.y * m_plotScale );

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
            x = ( m_paperSize.x - pos.x * m_plotScale );
        else
            y = pos.y * m_plotScale;
    }

    if( m_yaxisReversed )
        y = m_paperSize.y - y;

    x *= m_iuPerDeviceUnit;
    y *= m_iuPerDeviceUnit;

    return VECTOR2D( x, y );
}


VECTOR2D PLOTTER::userToDeviceSize( const VECTOR2I& size )
{
    return VECTOR2D( size.x * m_plotScale * m_iuPerDeviceUnit,
                     size.y * m_plotScale * m_iuPerDeviceUnit );
}


double PLOTTER::userToDeviceSize( double size ) const
{
    return size * m_plotScale * m_iuPerDeviceUnit;
}


#define IU_PER_MILS ( m_IUsPerDecimil * 10 )


double PLOTTER::GetDotMarkLenIU( int aLineWidth ) const
{
    return userToDeviceSize( m_renderSettings->GetDotLength( aLineWidth ) );
}


double PLOTTER::GetDashMarkLenIU( int aLineWidth ) const
{
    return userToDeviceSize( m_renderSettings->GetDashLength( aLineWidth ) );
}


double PLOTTER::GetDashGapLenIU( int aLineWidth ) const
{
    return userToDeviceSize( m_renderSettings->GetGapLength( aLineWidth ) );
}


void PLOTTER::Arc( const VECTOR2D& aStart, const VECTOR2D& aMid, const VECTOR2D& aEnd, FILL_T aFill,
                   int aWidth )
{
    VECTOR2D aCenter = CalcArcCenter( aStart, aMid, aEnd );

    EDA_ANGLE startAngle( aStart - aCenter );
    EDA_ANGLE endAngle( aEnd - aCenter );

    // < 0: left, 0 : on the line, > 0 : right
    double det = ( aEnd - aStart ).Cross( aMid - aStart );

    int       cw = det <= 0;
    EDA_ANGLE angle = endAngle - startAngle;

    if( cw )
        angle.Normalize();
    else
        angle.NormalizeNegative();

    double radius = ( aStart - aCenter ).EuclideanNorm();
    Arc( aCenter, startAngle, angle, radius, aFill, aWidth );
}


void PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle, const EDA_ANGLE& aAngle,
                   double aRadius, FILL_T aFill, int aWidth )
{
    polyArc( aCenter, aStartAngle, aAngle, aRadius, aFill, aWidth );
}


void PLOTTER::polyArc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    EDA_ANGLE       startAngle = aStartAngle;
    EDA_ANGLE       endAngle = startAngle + aAngle;
    const EDA_ANGLE delta( 5.0, DEGREES_T ); // increment to draw arc
    VECTOR2I        start, end;
    const int       sign = 1;

    if( aAngle < ANGLE_0 )
        std::swap( startAngle, endAngle );

    SetCurrentLineWidth( aWidth );

    start.x = KiROUND( aCenter.x + aRadius * startAngle.Cos() );
    start.y = KiROUND( aCenter.y + sign * aRadius * startAngle.Sin() );

    if( aFill != FILL_T::NO_FILL )
    {
        MoveTo( aCenter );
        LineTo( start );
    }
    else
    {
        MoveTo( start );
    }

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = KiROUND( aCenter.x + aRadius * ii.Cos() );
        end.y = KiROUND( aCenter.y + sign * aRadius * ii.Sin() );
        LineTo( end );
    }

    end.x = KiROUND( aCenter.x + aRadius * endAngle.Cos() );
    end.y = KiROUND( aCenter.y + sign * aRadius * endAngle.Sin() );

    if( aFill != FILL_T::NO_FILL )
    {
        LineTo( end );
        FinishTo( aCenter );
    }
    else
    {
        FinishTo( end );
    }
}


void PLOTTER::BezierCurve( const VECTOR2I& aStart, const VECTOR2I& aControl1,
                           const VECTOR2I& aControl2, const VECTOR2I& aEnd,
                           int aTolerance, int aLineThickness )
{
    // Generic fallback: Quadratic Bezier curve plotted as a polyline
    std::vector<VECTOR2I> ctrlPoints;
    ctrlPoints.reserve( 4 );

    ctrlPoints.push_back( aStart );
    ctrlPoints.push_back( aControl1 );
    ctrlPoints.push_back( aControl2 );
    ctrlPoints.push_back( aEnd );

    BEZIER_POLY bezier_converter( ctrlPoints );

    std::vector<VECTOR2I> approxPoints;
    bezier_converter.GetPoly( approxPoints, aTolerance );

    SetCurrentLineWidth( aLineThickness );
    MoveTo( aStart );

    for( unsigned ii = 1; ii < approxPoints.size()-1; ii++ )
        LineTo( approxPoints[ii] );

    FinishTo( aEnd );
}


void PLOTTER::PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor )
{
    VECTOR2I size( aImage.GetWidth() * aScaleFactor, aImage.GetHeight() * aScaleFactor );

    VECTOR2I start = aPos;
    start.x -= size.x / 2;
    start.y -= size.y / 2;

    VECTOR2I end = start;
    end.x += size.x;
    end.y += size.y;

    Rect( start, end, FILL_T::NO_FILL, USE_DEFAULT_LINE_WIDTH, 0 );
}


void PLOTTER::markerSquare( const VECTOR2I& position, int radius )
{
    double                r = KiROUND( radius / 1.4142 );
    std::vector<VECTOR2I> corner_list;
    VECTOR2I              corner;

    corner_list.reserve( 4 );

    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y - r;
    corner_list.push_back( corner );
    corner.x = position.x - r;
    corner.y = position.y + r;
    corner_list.push_back( corner );
    corner.x = position.x + r;
    corner.y = position.y + r;
    corner_list.push_back( corner );

    PlotPoly( corner_list, FILL_T::NO_FILL, GetCurrentLineWidth(), nullptr );
}


void PLOTTER::markerCircle( const VECTOR2I& position, int radius )
{
    Circle( position, radius * 2, FILL_T::NO_FILL, GetCurrentLineWidth() );
}


void PLOTTER::markerLozenge( const VECTOR2I& position, int radius )
{
    std::vector<VECTOR2I> corner_list;
    VECTOR2I              corner;

    corner_list.reserve( 4 );

    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );
    corner.x = position.x + radius;
    corner.y = position.y,
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y - radius;
    corner_list.push_back( corner );
    corner.x = position.x - radius;
    corner.y = position.y;
    corner_list.push_back( corner );
    corner.x = position.x;
    corner.y = position.y + radius;
    corner_list.push_back( corner );

    PlotPoly( corner_list, FILL_T::NO_FILL, GetCurrentLineWidth(), nullptr );
}


void PLOTTER::markerHBar( const VECTOR2I& pos, int radius )
{
    MoveTo( VECTOR2I( pos.x - radius, pos.y ) );
    FinishTo( VECTOR2I( pos.x + radius, pos.y ) );
}


void PLOTTER::markerSlash( const VECTOR2I& pos, int radius )
{
    MoveTo( VECTOR2I( pos.x - radius, pos.y - radius ) );
    FinishTo( VECTOR2I( pos.x + radius, pos.y + radius ) );
}


void PLOTTER::markerBackSlash( const VECTOR2I& pos, int radius )
{
    MoveTo( VECTOR2I( pos.x + radius, pos.y - radius ) );
    FinishTo( VECTOR2I( pos.x - radius, pos.y + radius ) );
}


void PLOTTER::markerVBar( const VECTOR2I& pos, int radius )
{
    MoveTo( VECTOR2I( pos.x, pos.y - radius ) );
    FinishTo( VECTOR2I( pos.x, pos.y + radius ) );
}


void PLOTTER::Marker( const VECTOR2I& position, int diametre, unsigned aShapeId )
{
    int radius = diametre / 2;

    /* Marker are composed by a series of 'parts' superimposed; not every
       combination make sense, obviously. Since they are used in order I
       tried to keep the uglier/more complex constructions at the end.
       Also I avoided the |/ |\ -/ -\ construction because they're *very*
       ugly... if needed they could be added anyway... I'd like to see
       a board with more than 58 drilling/slotting tools!
       If Visual C++ supported the 0b literals they would be optimally
       and easily encoded as an integer array. We have to do with octal */
    static const unsigned char marker_patterns[MARKER_COUNT] = {

        // Bit order:  O Square Lozenge - | \ /
        // First choice: simple shapes
        0003,  // X
        0100,  // O
        0014,  // +
        0040,  // Sq
        0020,  // Lz

        // Two simple shapes
        0103,  // X O
        0017,  // X +
        0043,  // X Sq
        0023,  // X Lz
        0114,  // O +
        0140,  // O Sq
        0120,  // O Lz
        0054,  // + Sq
        0034,  // + Lz
        0060,  // Sq Lz

        // Three simple shapes
        0117,  // X O +
        0143,  // X O Sq
        0123,  // X O Lz
        0057,  // X + Sq
        0037,  // X + Lz
        0063,  // X Sq Lz
        0154,  // O + Sq
        0134,  // O + Lz
        0074,  // + Sq Lz

        // Four simple shapes
        0174,  // O Sq Lz +
        0163,  // X O Sq Lz
        0157,  // X O Sq +
        0137,  // X O Lz +
        0077,  // X Sq Lz +

        // This draws *everything *
        0177,  // X O Sq Lz +

        // Here we use the single bars... so the cross is forbidden
        0110,  // O -
        0104,  // O |
        0101,  // O /
        0050,  // Sq -
        0044,  // Sq |
        0041,  // Sq /
        0030,  // Lz -
        0024,  // Lz |
        0021,  // Lz /
        0150,  // O Sq -
        0144,  // O Sq |
        0141,  // O Sq /
        0130,  // O Lz -
        0124,  // O Lz |
        0121,  // O Lz /
        0070,  // Sq Lz -
        0064,  // Sq Lz |
        0061,  // Sq Lz /
        0170,  // O Sq Lz -
        0164,  // O Sq Lz |
        0161,  // O Sq Lz /

        // Last resort: the backlash component (easy to confound)
        0102,  // \ O
        0042,  // \ Sq
        0022,  // \ Lz
        0142,  // \ O Sq
        0122,  // \ O Lz
        0062,  // \ Sq Lz
        0162   // \ O Sq Lz
    };

    if( aShapeId >= MARKER_COUNT )
    {
        // Fallback shape
        markerCircle( position, radius );
    }
    else
    {
        // Decode the pattern and draw the corresponding parts
        unsigned char pat = marker_patterns[aShapeId];

        if( pat & 0001 )
            markerSlash( position, radius );

        if( pat & 0002 )
            markerBackSlash( position, radius );

        if( pat & 0004 )
            markerVBar( position, radius );

        if( pat & 0010 )
            markerHBar( position, radius );

        if( pat & 0020 )
            markerLozenge( position, radius );

        if( pat & 0040 )
            markerSquare( position, radius );

        if( pat & 0100 )
            markerCircle( position, radius );
    }
}


void PLOTTER::ThickOval( const VECTOR2I& aPos, const VECTOR2I& aSize, const EDA_ANGLE& aOrient,
                         int aWidth, void* aData )
{
    SetCurrentLineWidth( aWidth, aData );

    EDA_ANGLE orient( aOrient );
    VECTOR2I  size( aSize );

    if( size.x > size.y )
    {
        std::swap( size.x, size.y );
        orient += ANGLE_90;
    }

    int deltaxy = size.y - size.x;       /* distance between centers of the oval */
    int radius  = size.x / 2;

    // Build a vertical oval shape giving the start and end points of arcs and edges,
    // and the middle point of arcs
    std::vector<VECTOR2I> corners;
    corners.reserve( 6 );
    // Shape is (x = corner and arc ends, c = arc centre)
    //  xcx
    //
    //  xcx
    int half_height = deltaxy / 2;
    corners.emplace_back( -radius, -half_height );
    corners.emplace_back( -radius, half_height );
    corners.emplace_back( 0, half_height );
    corners.emplace_back( radius, half_height );
    corners.emplace_back( radius, -half_height );
    corners.emplace_back( 0, -half_height );

    // Rotate and move to the actual position
    for( VECTOR2I& corner : corners )
    {
        RotatePoint( corner, orient );
        corner += aPos;
    }

    // Gen shape (2 lines and 2 180 deg arcs):
    MoveTo( corners[0] );
    FinishTo( corners[1] );

    Arc( corners[2], -orient, ANGLE_180, radius, FILL_T::NO_FILL, aWidth );

    MoveTo( corners[3] );
    FinishTo( corners[4] );

    Arc( corners[5], -orient, -ANGLE_180, radius, FILL_T::NO_FILL, aWidth );
}


void PLOTTER::ThickSegment( const VECTOR2I& start, const VECTOR2I& end, int width, void* aData )
{
    if( start == end )
    {
        Circle( start, width, FILL_T::FILLED_SHAPE, 0 );
    }
    else
    {
        SetCurrentLineWidth( width );
        MoveTo( start );
        FinishTo( end );
    }
}


void PLOTTER::ThickArc( const VECTOR2D& centre, const EDA_ANGLE& aStartAngle,
                        const EDA_ANGLE& aAngle, double aRadius, int aWidth, void* aData )
{
    Arc( centre, aStartAngle, aAngle, aRadius, FILL_T::NO_FILL, aWidth );
}


void PLOTTER::ThickArc( const EDA_SHAPE& aArcShape, void* aData, int aWidth )
{
    VECTOR2D center = aArcShape.getCenter();
    VECTOR2D mid = aArcShape.GetArcMid();
    VECTOR2D start = aArcShape.GetStart();
    VECTOR2D end = aArcShape.GetEnd();

    EDA_ANGLE startAngle( start - center );
    EDA_ANGLE endAngle( end - center );
    EDA_ANGLE angle = endAngle - startAngle;

    // < 0: left, 0 : on the line, > 0 : right
    double det = ( end - start ).Cross( mid - start );

    if( det <= 0 ) // cw
        angle.Normalize();
    else
        angle.NormalizeNegative();

    double radius = ( start - center ).EuclideanNorm();

    ThickArc( center, startAngle, angle, radius, aWidth, aData );
}


void PLOTTER::ThickRect( const VECTOR2I& p1, const VECTOR2I& p2, int width, void* aData )
{
    Rect( p1, p2, FILL_T::NO_FILL, width, 0 );
}


void PLOTTER::ThickCircle( const VECTOR2I& pos, int diametre, int width, void* aData )
{
    Circle( pos, diametre, FILL_T::NO_FILL, width );
}


void PLOTTER::FilledCircle( const VECTOR2I& pos, int diametre, void* aData )
{
    Circle( pos, diametre, FILL_T::FILLED_SHAPE, 0 );
}


void PLOTTER::ThickPoly( const SHAPE_POLY_SET& aPoly, int aWidth, void* aData )
{
    PlotPoly( aPoly.COutline( 0 ), FILL_T::NO_FILL, aWidth, aData );
}


void PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth, void* aData )
{
    std::vector<VECTOR2I> cornerList;
    cornerList.reserve( aLineChain.PointCount() );

    for( int ii = 0; ii < aLineChain.PointCount(); ii++ )
        cornerList.emplace_back( aLineChain.CPoint( ii ) );

    if( aLineChain.IsClosed() && cornerList.front() != cornerList.back() )
        cornerList.emplace_back( aLineChain.CPoint( 0 ) );

    PlotPoly( cornerList, aFill, aWidth, aData );
}


void PLOTTER::Text( const VECTOR2I&        aPos,
                    const COLOR4D&         aColor,
                    const wxString&        aText,
                    const EDA_ANGLE&       aOrient,
                    const VECTOR2I&        aSize,
                    enum GR_TEXT_H_ALIGN_T aH_justify,
                    enum GR_TEXT_V_ALIGN_T aV_justify,
                    int                    aPenWidth,
                    bool                   aItalic,
                    bool                   aBold,
                    bool                   aMultilineAllowed,
                    KIFONT::FONT*          aFont,
                    const KIFONT::METRICS& aFontMetrics,
                    void*                  aData )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    wxString                   text( aText );

    if( text.Contains( wxS( "@{" ) ) )
    {
        EXPRESSION_EVALUATOR evaluator;
        text = evaluator.Evaluate( text );
    }

    SetColor( aColor );

    if( aPenWidth == 0 && aBold ) // Use default values if aPenWidth == 0
        aPenWidth = GetPenSizeForBold( std::min( aSize.x, aSize.y ) );

    if( aPenWidth < 0 )
        aPenWidth = -aPenWidth;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                SetCurrentLineWidth( aPenWidth );
                MoveTo( aPt1 );
                LineTo( aPt2 );
                PenFinish();
            },
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                PlotPoly( aPoly, FILL_T::FILLED_SHAPE, 0, aData );
            } );

    TEXT_ATTRIBUTES attributes;
    attributes.m_Angle = aOrient;
    attributes.m_StrokeWidth = aPenWidth;
    attributes.m_Italic = aItalic;
    attributes.m_Bold = aBold;
    attributes.m_Halign = aH_justify;
    attributes.m_Valign = aV_justify;
    attributes.m_Size = aSize;

    // if Size.x is < 0, the text is mirrored (we have no other param to know a text is mirrored)
    if( attributes.m_Size.x < 0 )
    {
        attributes.m_Size.x = -attributes.m_Size.x;
        attributes.m_Mirrored = true;
    }

    if( !aFont )
        aFont = KIFONT::FONT::GetFont( m_renderSettings->GetDefaultFont() );

    aFont->Draw( &callback_gal, text, aPos, attributes, aFontMetrics );
}


void PLOTTER::PlotText( const VECTOR2I&        aPos,
                        const COLOR4D&         aColor,
                        const wxString&        aText,
                        const TEXT_ATTRIBUTES& aAttributes,
                        KIFONT::FONT*          aFont,
                        const KIFONT::METRICS& aFontMetrics,
                        void*                  aData )
{
    KIGFX::GAL_DISPLAY_OPTIONS empty_opts;
    wxString                   text( aText );

    if( text.Contains( wxS( "@{" ) ) )
    {
        EXPRESSION_EVALUATOR evaluator;
        text = evaluator.Evaluate( text );
    }

    TEXT_ATTRIBUTES attributes = aAttributes;
    int penWidth = attributes.m_StrokeWidth;

    SetColor( aColor );
    SetCurrentLineWidth( penWidth, aData );

    if( penWidth == 0 && attributes.m_Bold ) // Use default values if aPenWidth == 0
        penWidth = GetPenSizeForBold( std::min( attributes.m_Size.x, attributes.m_Size.y ) );

    if( penWidth < 0 )
        penWidth = -penWidth;

    attributes.m_StrokeWidth = penWidth;

    CALLBACK_GAL callback_gal( empty_opts,
            // Stroke callback
            [&]( const VECTOR2I& aPt1, const VECTOR2I& aPt2 )
            {
                MoveTo( aPt1 );
                LineTo( aPt2 );
                PenFinish();
            },
            // Polygon callback
            [&]( const SHAPE_LINE_CHAIN& aPoly )
            {
                PlotPoly( aPoly, FILL_T::FILLED_SHAPE, 0, aData );
            } );

    if( !aFont )
        aFont = KIFONT::FONT::GetFont( m_renderSettings->GetDefaultFont() );

    aFont->Draw( &callback_gal, text, aPos, attributes, aFontMetrics );
}

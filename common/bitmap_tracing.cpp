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

#include <bitmap_tracing.h>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstring>
#include <vector>

#include <bitmap.h>
#include <math/util.h>
#include <potracelib.h>
#include <reporter.h>
#include <wx/translation.h>


static double square( double x )
{
    return x * x;
}


static double cube( double x )
{
    return x * x * x;
}


/* render a Bezier curve. */
static void BezierToPolyline( std::vector<potrace_dpoint_t>& aCornersBuffer, potrace_dpoint_t p1, potrace_dpoint_t p2,
                              potrace_dpoint_t p3, potrace_dpoint_t p4 )
{
    double dd0, dd1, dd, delta, e2, epsilon, t;

    // p1 = starting point

    /* we approximate the curve by small line segments. The interval
     *  size, epsilon, is determined on the fly so that the distance
     *  between the true curve and its approximation does not exceed the
     *  desired accuracy delta. */

    delta = 0.25; /* desired accuracy, in pixels */

    /* let dd = maximal value of 2nd derivative over curve - this must
     *  occur at an endpoint. */
    dd0 = square( p1.x - 2 * p2.x + p3.x ) + square( p1.y - 2 * p2.y + p3.y );
    dd1 = square( p2.x - 2 * p3.x + p4.x ) + square( p2.y - 2 * p3.y + p4.y );
    dd = 6 * sqrt( std::max( dd0, dd1 ) );
    e2 = 8 * delta <= dd ? 8 * delta / dd : 1;
    epsilon = sqrt( e2 ); /* necessary interval size */

    for( t = epsilon; t < 1; t += epsilon )
    {
        potrace_dpoint_t intermediate_point;
        intermediate_point.x = p1.x * cube( 1 - t ) + 3 * p2.x * square( 1 - t ) * t
                               + 3 * p3.x * ( 1 - t ) * square( t ) + p4.x * cube( t );

        intermediate_point.y = p1.y * cube( 1 - t ) + 3 * p2.y * square( 1 - t ) * t
                               + 3 * p3.y * ( 1 - t ) * square( t ) + p4.y * cube( t );

        aCornersBuffer.push_back( intermediate_point );
    }

    aCornersBuffer.push_back( p4 );
}


wxImage ConvertImageToBilevel( const wxImage& aImage, double aThreshold, bool aInvert )
{
    wxImage bilevel = aImage.ConvertToGreyscale();

    if( !bilevel.IsOk() )
        return bilevel;

    bilevel.UnShare();

    const int            pixelCount = bilevel.GetWidth() * bilevel.GetHeight();
    const unsigned char* source = aImage.GetData();
    const unsigned char* alpha = bilevel.HasAlpha() ? bilevel.GetAlpha() : nullptr;
    unsigned char*       grey = bilevel.GetData();

    const bool          hasMask = aImage.HasMask();
    const unsigned char maskRed = hasMask ? aImage.GetMaskRed() : 0;
    const unsigned char maskGreen = hasMask ? aImage.GetMaskGreen() : 0;
    const unsigned char maskBlue = hasMask ? aImage.GetMaskBlue() : 0;

    const unsigned char threshold = aThreshold * 255;
    const unsigned char alphaThreshold = 0.7 * threshold;

    // The whole image is walked again on every move of a threshold slider, so this reads and
    // writes the raw pixels rather than going through the per pixel accessors.
    for( int ii = 0; ii < pixelCount; ii++ )
    {
        unsigned char level = grey[ii * 3];

        // A masked pixel is see-through, so it stays white whatever colour it carries.
        if( hasMask && source[ii * 3] == maskRed && source[ii * 3 + 1] == maskGreen && source[ii * 3 + 2] == maskBlue )
        {
            level = 255;
        }

        if( aInvert )
            level = ~level;

        level = ( level < threshold && ( !alpha || alpha[ii] > alphaThreshold ) ) ? 0 : 255;

        grey[ii * 3] = level;
        grey[ii * 3 + 1] = level;
        grey[ii * 3 + 2] = level;
    }

    return bilevel;
}


wxImage MakeBilevelPreview( const wxImage& aImage, double aThreshold, bool aInvert, const wxSize& aRoom )
{
    wxImage preview = ConvertImageToBilevel( aImage, aThreshold, aInvert );

    if( !preview.IsOk() )
        return preview;

    double fit = std::min( (double) aRoom.x / preview.GetWidth(), (double) aRoom.y / preview.GetHeight() );

    if( fit < 1.0 )
    {
        preview = preview.Scale( KiROUND( preview.GetWidth() * fit ), KiROUND( preview.GetHeight() * fit ),
                                 wxIMAGE_QUALITY_NORMAL );
    }

    return preview;
}


SHAPE_POLY_SET TraceBilevelImage( const wxImage& aBilevelImage, double aScaleX, double aScaleY, REPORTER& aReporter )
{
    SHAPE_POLY_SET result;

    potrace_bitmap_t* potraceBitmap = bm_new( aBilevelImage.GetWidth(), aBilevelImage.GetHeight() );

    if( !potraceBitmap )
    {
        aReporter.Report( _( "Error allocating memory for potrace bitmap" ), RPT_SEVERITY_ERROR );
        return result;
    }

    for( int y = 0; y < aBilevelImage.GetHeight(); y++ )
    {
        for( int x = 0; x < aBilevelImage.GetWidth(); x++ )
        {
            unsigned char pixel = aBilevelImage.GetGreen( x, y );
            BM_PUT( potraceBitmap, x, y, pixel ? 0 : 1 );
        }
    }

    potrace_param_t* param = potrace_param_default();

    if( !param )
    {
        bm_free( potraceBitmap );
        aReporter.Report(
                wxString::Format( _( "Error allocating parameters: %s" ), wxString::FromUTF8( strerror( errno ) ) ),
                RPT_SEVERITY_ERROR );
        return result;
    }

    // For parameters: see http://potrace.sourceforge.net/potracelib.pdf
    param->turdsize = 0;       // area (in pixels) of largest path to be ignored.
                               // Potrace default is 2
    param->opttolerance = 0.2; // curve optimization tolerance. Potrace default is 0.2

    potrace_state_t* st = potrace_trace( param, potraceBitmap );

    if( !st || st->status != POTRACE_STATUS_OK )
    {
        if( st )
            potrace_state_free( st );

        potrace_param_free( param );
        bm_free( potraceBitmap );

        aReporter.Report( wxString::Format( _( "Error tracing bitmap: %s" ), wxString::FromUTF8( strerror( errno ) ) ),
                          RPT_SEVERITY_ERROR );
        return result;
    }

    std::vector<potrace_dpoint_t> cornersBuffer;
    SHAPE_POLY_SET                areas;
    SHAPE_POLY_SET                holes;
    bool                          mainOutline = true;
    int                           droppedGroups = 0;

    for( potrace_path_t* path = st->plist; path != nullptr; path = path->next )
    {
        int  cnt = path->curve.n;
        int* tag = path->curve.tag;
        potrace_dpoint_t( *c )[3] = path->curve.c;
        potrace_dpoint_t startpoint = c[cnt - 1][2];

        for( int ii = 0; ii < cnt; ii++ )
        {
            switch( tag[ii] )
            {
            case POTRACE_CORNER:
                cornersBuffer.push_back( c[ii][1] );
                cornersBuffer.push_back( c[ii][2] );
                startpoint = c[ii][2];
                break;

            case POTRACE_CURVETO:
                BezierToPolyline( cornersBuffer, startpoint, c[ii][0], c[ii][1], c[ii][2] );
                startpoint = c[ii][2];
                break;
            }
        }

        SHAPE_POLY_SET& target = mainOutline ? areas : holes;

        mainOutline = false;
        target.NewOutline();

        for( const potrace_dpoint_s& pt : cornersBuffer )
            target.Append( KiROUND( pt.x * aScaleX ), KiROUND( pt.y * aScaleY ) );

        cornersBuffer.clear();

        // at the end of a group of a positive path and its negative children, fill.
        if( path->next == nullptr || path->next->sign == '+' )
        {
            areas.Simplify();
            holes.Simplify();
            areas.BooleanSubtract( holes );

            // Ensure there are no self intersecting polygons
            if( areas.NormalizeAreaOutlines() )
            {
                // Convert polygons with holes to polygons with slits
                areas.Fracture();

                for( int ii = 0; ii < areas.OutlineCount(); ii++ )
                    result.AddOutline( areas.Outline( ii ) );
            }
            else
            {
                droppedGroups++;
            }

            // Start the next group even after a failure, so one bad group loses only itself.
            areas.RemoveAllContours();
            holes.RemoveAllContours();
            mainOutline = true;
        }
    }

    potrace_state_free( st );
    potrace_param_free( param );
    bm_free( potraceBitmap );

    // A dropped group leaves a hole in the result, so say so rather than let the user hunt
    // for what went missing.
    if( droppedGroups > 0 )
    {
        aReporter.Report( wxString::Format( wxPLURAL( "%d region of the image could not be traced.",
                                                      "%d regions of the image could not be traced.", droppedGroups ),
                                            droppedGroups ),
                          RPT_SEVERITY_WARNING );
    }

    return result;
}

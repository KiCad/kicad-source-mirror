/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 1992-2010 jean-pierre.charras
 * Copyright (C) 1992-2010 Kicad Developers, see change_log.txt for contributors.
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

#include "kbool/booleng.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <math.h>
#include <vector>

#include "potracelib.h"
#include "auxiliary.h"

#ifndef max
    #define max( a, b ) ( ( (a) > (b) ) ? (a) : (b) )
  #endif
  #ifndef min
    #define min( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
  #endif


enum output_format {
    POSTSCRIPT_FMT = 1,
    PCBNEW_FMT,
    EESCHEMA_FMT
};
/* free a potrace bitmap */
static void bm_free(potrace_bitmap_t *bm) {
  if (bm != NULL) {
    free(bm->map);
  }
  free(bm);
}


/* Helper class th handle useful info to convert a bitmpa to
 *  a polygonal object description
 */
class BITMAPCONV_INFO
{
public:
    enum output_format m_Format;
    int m_PixmapWidth;
    int m_PixmapHeight;             // the bitmap size in pixels
    double             m_ScaleX;
    double             m_ScaleY;    // the conversion scale
    potrace_path_t*    m_Paths;     // the list of paths, from potrace (list of lines and bezier curves)
    FILE* m_Outfile;
public:
    BITMAPCONV_INFO();
};

static void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                              potrace_dpoint_t                p1,
                              potrace_dpoint_t                p2,
                              potrace_dpoint_t                p3,
                              potrace_dpoint_t                p4 );

static void CreateOutputFile( BITMAPCONV_INFO& aInfo );

static const char* CmpName = "LOGO";


BITMAPCONV_INFO::BITMAPCONV_INFO()
{
    m_Format = POSTSCRIPT_FMT;
    m_PixmapWidth  = 0;
    m_PixmapHeight = 0;
    m_ScaleX  = 1.0;
    m_ScaleY  = 1.0;
    m_Paths   = NULL;
    m_Outfile = NULL;
}


/** Function ArmBoolEng
 * Initialise parameters used in kbool
 * @param aBooleng = pointer to the Bool_Engine to initialise
 * @param aConvertHoles = mode for holes when a boolean operation is made
 *   true: in resulting polygon, holes are linked into outer contours by double overlapping segments
 *   false: in resulting polygons, holes are not linked: they are separate polygons
 */
void ArmBoolEng( Bool_Engine* aBooleng, bool aConvertHoles )
{
    // set some global vals to arm the boolean engine

    // input points are scaled up with GetDGrid() * GetGrid()

    // DGRID is only meant to make fractional parts of input data which

    /*
     *  The input data scaled up with DGrid is related to the accuracy the user has in his input data.
     *  User data with a minimum accuracy of 0.001, means set the DGrid to 1000.
     *  The input data may contain data with a minimum accuracy much smaller, but by setting the DGrid
     *  everything smaller than 1/DGrid is rounded.
     *
     *  DGRID is only meant to make fractional parts of input data which can be
     *  doubles, part of the integers used in vertexes within the boolean algorithm.
     *  And therefore DGRID bigger than 1 is not usefull, you would only loose accuracy.
     *  Within the algorithm all input data is multiplied with DGRID, and the result
     *  is rounded to an integer.
     */
    double DGRID = 1000.0;      // round coordinate X or Y value in calculations to this (initial value = 1000.0 in kbool example)
                                // kbool uses DGRID to convert float user units to integer
                                // kbool unit = (int)(user unit * DGRID)
                                // Note: in kicad, coordinates are already integer so DGRID could be set to 1
                                // we can choose 1.0,
                                // but choose DGRID = 1000.0 solves some filling problems
//                             (perhaps because this allows a better precision in kbool internal calculations

    double MARGE = 1.0 / DGRID;         // snap with in this range points to lines in the intersection routines
                                        // should always be >= 1/DGRID  a  MARGE >= 10/DGRID is ok
                                        // this is also used to remove small segments and to decide when
                                        // two segments are in line. ( initial value = 0.001 )
                                        // For kicad we choose MARGE = 1/DGRID

    double CORRECTIONFACTOR = 0.0;      // correct the polygons by this number: used in BOOL_CORRECTION operation
                                        // this operation shrinks a polygon if CORRECTIONFACTOR < 0
                                        // or stretch it if CORRECTIONFACTOR > 0
                                        // the size change is CORRECTIONFACTOR (holes are correctly handled)
    double CORRECTIONABER = 1.0;        // the accuracy for the rounded shapes used in correction
    double ROUNDFACTOR    = 1.5;        // when will we round the correction shape to a circle
    double SMOOTHABER     = 10.0;       // accuracy when smoothing a polygon
    double MAXLINEMERGE   = 1000.0;     // leave as is, segments of this length in smoothen


    /*
     *    Grid makes sure that the integer data used within the algorithm has room for extra intersections
     *    smaller than the smallest number within the input data.
     *    The input data scaled up with DGrid is related to the accuracy the user has in his input data.
     *    Another scaling with Grid is applied on top of it to create space in the integer number for
     *    even smaller numbers.
     */
    int GRID = (int) ( 10000 / DGRID );    // initial value = 10000 in kbool example

    // But we use 10000/DGRID because the scalling is made
    // by DGRID on integer pcbnew units and
    // the global scalling ( GRID*DGRID) must be < 30000 to avoid
    // overflow in calculations (made in long long in kbool)
    if( GRID <= 1 )     // Cannot be null!
        GRID = 1;

    aBooleng->SetMarge( MARGE );
    aBooleng->SetGrid( GRID );
    aBooleng->SetDGrid( DGRID );
    aBooleng->SetCorrectionFactor( CORRECTIONFACTOR );
    aBooleng->SetCorrectionAber( CORRECTIONABER );
    aBooleng->SetSmoothAber( SMOOTHABER );
    aBooleng->SetMaxlinemerge( MAXLINEMERGE );
    aBooleng->SetRoundfactor( ROUNDFACTOR );
    aBooleng->SetWindingRule( true );           // This is the default kbool value

    if( aConvertHoles )
    {
#if 1                                                       // Can be set to 1 for kbool version >= 2.1, must be set to 0 for previous versions
        // SetAllowNonTopHoleLinking() exists only in kbool >= 2.1
        aBooleng->SetAllowNonTopHoleLinking( false );       // Default = , but i have problems (filling errors) when true
#endif
        aBooleng->SetLinkHoles( true );                     // holes will be connected by double overlapping segments
        aBooleng->SetOrientationEntryMode( false );         // all polygons are contours, not holes
    }
    else
    {
        aBooleng->SetLinkHoles( false );                // holes will not be connected by double overlapping segments
        aBooleng->SetOrientationEntryMode( true );      // holes are entered counter clockwise
    }
}


int bitmap2component( potrace_bitmap_t* aPotrace_bitmap, FILE * aOutfile, int aFormat )
{
    potrace_param_t*  param;
    potrace_state_t*  st;


    /* set tracing parameters, starting from defaults */
    param = potrace_param_default();
    if( !param )
    {
        fprintf( stderr, "Error allocating parameters: %s\n", strerror( errno ) );
        return 1;
    }
    param->turdsize = 0;

    /* convert the bitmap to curves */
    st = potrace_trace( param, aPotrace_bitmap );
    if( !st || st->status != POTRACE_STATUS_OK )
    {
        fprintf( stderr, "Error tracing bitmap: %s\n", strerror( errno ) );
        return 1;
    }

    BITMAPCONV_INFO info;
    info.m_PixmapWidth  = aPotrace_bitmap->w;
    info.m_PixmapHeight = aPotrace_bitmap->h;     // the bitmap size in pixels
    info.m_Paths   = st->plist;
    info.m_Outfile = aOutfile;

    switch( aFormat )
    {
    case 2:
        info.m_Format = POSTSCRIPT_FMT;
        info.m_ScaleX = info.m_ScaleY = 1.0;        // the conversion scale
        /* output vector data, e.g. as a rudimentary EPS file */
        CreateOutputFile( info );
        break;

    case 1:
        info.m_Format = EESCHEMA_FMT;
        info.m_ScaleX = 1000.0 / 300;       // the conversion scale
        info.m_ScaleY = -  info.m_ScaleX;   // Y axis is bottom to Top for components in libs
        CreateOutputFile( info );
        break;

    case 0:
        info.m_Format = PCBNEW_FMT;
        info.m_ScaleX = 10000.0 / 300;       // the conversion scale
        info.m_ScaleY = info.m_ScaleX;       // Y axis is top to bottom in modedit
        CreateOutputFile( info );
        break;

    default:
       break;
    }


    bm_free( aPotrace_bitmap );
    potrace_state_free( st );
    potrace_param_free( param );

    return 0;
}


static void OuputHeader( BITMAPCONV_INFO& aInfo )
{
    int Ypos = (int) ( aInfo.m_PixmapHeight / 2 * aInfo.m_ScaleY );
    int fieldSize;             // fields text size = 60 mils

    switch( aInfo.m_Format )
    {
    case POSTSCRIPT_FMT:
        /* output vector data, e.g. as a rudimentary EPS file */
        fprintf( aInfo.m_Outfile, "%%!PS-Adobe-3.0 EPSF-3.0\n" );
        fprintf( aInfo.m_Outfile, "%%%%BoundingBox: 0 0 %d %d\n",
                 aInfo.m_PixmapWidth, aInfo.m_PixmapHeight );
        fprintf( aInfo.m_Outfile, "gsave\n" );
        break;

    case PCBNEW_FMT:
        #define FIELD_LAYER 21
        fieldSize = 600;             // fields text size = 60 mils
        Ypos += fieldSize / 2;
        fprintf( aInfo.m_Outfile, "PCBNEW-LibModule-V1\n" );
        fprintf( aInfo.m_Outfile, "$INDEX\n%s\n$EndINDEX\n", CmpName );

        fprintf( aInfo.m_Outfile, "#\n# %s\n", CmpName );
        fprintf( aInfo.m_Outfile, "# pixmap w = %d, h = %d\n#\n",
                 aInfo.m_PixmapWidth, aInfo.m_PixmapHeight );
        fprintf( aInfo.m_Outfile, "$MODULE %s\n", CmpName );
        fprintf( aInfo.m_Outfile, "Po 0 0 0 15 00000000 00000000 ~~\n");
        fprintf( aInfo.m_Outfile, "T0 0 %d %d %d 0 %d N I %d \"G***\"\n",
                Ypos, fieldSize, fieldSize, fieldSize/5, FIELD_LAYER );
        fprintf( aInfo.m_Outfile, "T1 0 %d %d %d 0 %d N I %d \"%s\"\n",
                -Ypos, fieldSize, fieldSize, fieldSize/5, FIELD_LAYER, CmpName );
        break;

    case EESCHEMA_FMT:
        fprintf( aInfo.m_Outfile, "EESchema-LIBRARY Version 2.3\n" );
        fprintf( aInfo.m_Outfile, "#\n# %s\n", CmpName );
        fprintf( aInfo.m_Outfile, "# pixmap size w = %d, h = %d\n#\n",
                 aInfo.m_PixmapWidth, aInfo.m_PixmapHeight );

        // print reference and value
        fieldSize = 60;             // fields text size = 60 mils
        Ypos += fieldSize / 2;
        fprintf( aInfo.m_Outfile, "DEF %s G 0 40 Y Y 1 F N\n", CmpName );
        fprintf( aInfo.m_Outfile, "F0 \"#G\" 0 %d %d H I C CNN\n", Ypos, fieldSize );
        fprintf( aInfo.m_Outfile, "F1 \"%s\" 0 %d %d H I C CNN\n", CmpName, -Ypos, fieldSize );
        fprintf( aInfo.m_Outfile, "DRAW\n" );
        break;
    }
}


static void OuputEnd( BITMAPCONV_INFO& aInfo )
{
    switch( aInfo.m_Format )
    {
    case POSTSCRIPT_FMT:
        fprintf( aInfo.m_Outfile, "grestore\n" );
        fprintf( aInfo.m_Outfile, "%%EOF\n" );
        break;

    case PCBNEW_FMT:
        fprintf( aInfo.m_Outfile, "$EndMODULE %s\n", CmpName );
        fprintf( aInfo.m_Outfile, "$EndLIBRARY\n" );
        break;

    case EESCHEMA_FMT:
        fprintf( aInfo.m_Outfile, "ENDDRAW\n" );
        fprintf( aInfo.m_Outfile, "ENDDEF\n" );
        break;
    }
}


static void OuputOnePolygon( BITMAPCONV_INFO&                aInfo,
                             std::vector <potrace_dpoint_t>& aPolygonBuffer )
{
    unsigned ii;

    double   offsetX = aInfo.m_PixmapWidth / 2 * aInfo.m_ScaleX;
    double   offsetY = aInfo.m_PixmapHeight / 2 * aInfo.m_ScaleY;

    switch( aInfo.m_Format )
    {
    case POSTSCRIPT_FMT:
        fprintf( aInfo.m_Outfile, "%f %f moveto\n",
                 aPolygonBuffer[0].x * aInfo.m_ScaleX,
                 aPolygonBuffer[0].y * aInfo.m_ScaleY );
        for( ii = 1; ii < aPolygonBuffer.size(); ii++ )
            fprintf( aInfo.m_Outfile, "%f %f lineto\n",
                     aPolygonBuffer[ii].x * aInfo.m_ScaleX,
                     aPolygonBuffer[ii].y * aInfo.m_ScaleY );

        fprintf( aInfo.m_Outfile, "0 setgray fill\n" );
        break;

    case PCBNEW_FMT:
    {
        #define SILKSCREEN_N_FRONT 21
        int layer = SILKSCREEN_N_FRONT;
        int width = 1;
        fprintf( aInfo.m_Outfile, "DP %d %d %d %d %d %d %d\n",
                       0, 0, 0, 0,
                       int(aPolygonBuffer.size()+1),
                       width, layer );

        for( ii = 0; ii < aPolygonBuffer.size();  ii++ )
            fprintf( aInfo.m_Outfile, "Dl %d %d\n",
                    (int) ( aPolygonBuffer[ii].x * aInfo.m_ScaleX - offsetX ),
                    (int) ( aPolygonBuffer[ii].y * aInfo.m_ScaleY - offsetY ) );
        // Close polygon
        fprintf( aInfo.m_Outfile, "Dl %d %d\n",
                (int) ( aPolygonBuffer[0].x * aInfo.m_ScaleX - offsetX ),
                (int) ( aPolygonBuffer[0].y * aInfo.m_ScaleY - offsetY ) );
    }
        break;

    case EESCHEMA_FMT:
        fprintf( aInfo.m_Outfile, "P %d 0 0 1", int(aPolygonBuffer.size()+1) );
        for( ii = 0; ii < aPolygonBuffer.size(); ii++ )
            fprintf( aInfo.m_Outfile, " %d %d",
                    (int) ( aPolygonBuffer[ii].x * aInfo.m_ScaleX - offsetX ),
                    (int) ( aPolygonBuffer[ii].y * aInfo.m_ScaleY - offsetY ) );
       // Close polygon
        fprintf( aInfo.m_Outfile, " %d %d",
                (int) ( aPolygonBuffer[0].x * aInfo.m_ScaleX - offsetX ),
                (int) ( aPolygonBuffer[0].y * aInfo.m_ScaleY - offsetY ) );

        fprintf( aInfo.m_Outfile, " F\n" );
        break;
    }
}


static void CreateOutputFile( BITMAPCONV_INFO& aInfo )
{
    unsigned int i, n;
    int*         tag;

    std::vector <potrace_dpoint_t> cornersBuffer;

    potrace_dpoint_t( *c )[3];
    OuputHeader( aInfo );

    bool         main_outline = true;
    Bool_Engine* booleng = NULL;

    /* draw each as a polygon with no hole.
     * Bezier curves are approximated by a polyline
     */
    potrace_path_t* paths = aInfo.m_Paths;    // the list of paths
    while( paths != NULL )
    {
        n   = paths->curve.n;
        tag = paths->curve.tag;
        c   = paths->curve.c;
        potrace_dpoint_t startpoint = c[n - 1][2];
        cornersBuffer.push_back( startpoint );
        if( booleng == NULL )
        {
            booleng = new Bool_Engine();
            ArmBoolEng( booleng, true );
        }
        for( i = 0; i < n; i++ )
        {
            switch( tag[i] )
            {
            case POTRACE_CORNER:
                cornersBuffer.push_back( c[i][1] );
                cornersBuffer.push_back( c[i][2] );
                startpoint = c[i][2];
                break;

            case POTRACE_CURVETO:
                BezierToPolyline( cornersBuffer, startpoint, c[i][0], c[i][1], c[i][2] );
                startpoint = c[i][2];
                break;
            }
        }

        // Store current path
        if( main_outline )
        {
            main_outline = false;
            booleng->StartPolygonAdd( GROUP_A );
            for( i = 1; i < cornersBuffer.size(); i++ )
                booleng->AddPoint( cornersBuffer[i].x, cornersBuffer[i].y );

            booleng->EndPolygonAdd();
        }
        else
        {
            booleng->StartPolygonAdd( GROUP_B );
            for( i = 1; i < cornersBuffer.size(); i++ )
                booleng->AddPoint( cornersBuffer[i].x, cornersBuffer[i].y );

            booleng->EndPolygonAdd();
        }
        cornersBuffer.clear();

        /* at the end of a group of a positive path and its negative
         *  children, fill. */
        if( paths->next == NULL || paths->next->sign == '+' )
        {
            booleng->Do_Operation( BOOL_A_SUB_B );
            std::vector <potrace_dpoint_t> PolygonBuffer;
            while( booleng->StartPolygonGet() )
            {
                potrace_dpoint_t corner;
                PolygonBuffer.clear();
                while( booleng->PolygonHasMorePoints() )
                {
                    corner.x = booleng->GetPolygonXPoint();
                    corner.y = booleng->GetPolygonYPoint();
                    PolygonBuffer.push_back( corner );
                }

                booleng->EndPolygonGet();
                OuputOnePolygon( aInfo, PolygonBuffer );
                PolygonBuffer.clear();
            }

            delete booleng;
            booleng = NULL;
            main_outline = true;
        }
        paths = paths->next;
    }

    OuputEnd( aInfo );
}


/* render a Bezier curve. */
void BezierToPolyline( std::vector <potrace_dpoint_t>& aCornersBuffer,
                       potrace_dpoint_t                p1,
                       potrace_dpoint_t                p2,
                       potrace_dpoint_t                p3,
                       potrace_dpoint_t                p4 )
{
    double dd0, dd1, dd, delta, e2, epsilon, t;

    // p1 = starting point

    /* we approximate the curve by small line segments. The interval
     *  size, epsilon, is determined on the fly so that the distance
     *  between the true curve and its approximation does not exceed the
     *  desired accuracy delta. */

    delta = 0.5; /* desired accuracy, in pixels */

    /* let dd = maximal value of 2nd derivative over curve - this must
     *  occur at an endpoint. */
    dd0     = sq( p1.x - 2 * p2.x + p3.x ) + sq( p1.y - 2 * p2.y + p3.y );
    dd1     = sq( p2.x - 2 * p3.x + p4.x ) + sq( p2.y - 2 * p3.y + p4.y );
    dd      = 6 * sqrt( max( dd0, dd1 ) );
    e2      = 8 * delta <= dd ? 8 * delta / dd : 1;
    epsilon = sqrt( e2 ); /* necessary interval size */

    for( t = epsilon; t<1; t += epsilon )
    {
        potrace_dpoint_t intermediate_point;
        intermediate_point.x = p1.x * cu( 1 - t ) +
                               3* p2.x* sq( 1 - t ) * t +
                               3 * p3.x * (1 - t) * sq( t ) +
                               p4.x*    cu( t );

        intermediate_point.y = p1.y * cu( 1 - t ) +
                               3* p2.y* sq( 1 - t ) * t +
                               3 * p3.y * (1 - t) * sq( t ) + p4.y* cu( t );

        aCornersBuffer.push_back( intermediate_point );
    }

    aCornersBuffer.push_back( p4 );
}

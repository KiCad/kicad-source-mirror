/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2017 KiCad Developers, see AUTHORS.txt for contributors.
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


/*  This source is a complement to specctra.cpp and implements the export to
    specctra dsn file format.  The specification for the grammar of the specctra
    dsn file used to develop this code is given here:
    http://tech.groups.yahoo.com/group/kicad-users/files/  then file "specctra.pdf"

    Also see the comments at the top of the specctra.cpp file itself.
*/

#include <pcb_edit_frame.h>
#include <confirm.h>            // DisplayError()
#include <gestfich.h>           // EDA_FileSelector()
#include <trigo.h>              // RotatePoint()
#include <macros.h>

#include <set>                  // std::set
#include <map>                  // std::map

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_track.h>
#include <class_zone.h>
#include <base_units.h>
#include <wildcards_and_files_ext.h>
#include <collectors.h>
#include <geometry/shape_poly_set.h>
#include <geometry/convex_hull.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>

#include "specctra.h"

using namespace DSN;

// comment the line #define EXPORT_CUSTOM_PADS_CONVEX_HULL to export CUSTOM pads exact shapes.
// Keep in mind shapes can be non convex polygons with holes (linked to outline)
// that can create issues.
// Especially Freerouter does not handle them very well:
// - too complex shapes are not accepted, especially shapes with holes (dsn files are not loaded).
// - and Freerouter actually uses something like a convex hull of the shape (that works not very well).
// I am guessing non convex polygons with holes linked could create issues with any Router.
#define EXPORT_CUSTOM_PADS_CONVEX_HULL

// Add .1 mil to the requested clearances as a safety margin.
// There has been disagreement about interpretation of clearance in the past
// between KiCad and Freerouter, so keep this safetyMargin until the
// disagreement is resolved and stable.  Freerouter seems to be moving
// (protected) traces upon loading the DSN file, and even though it seems to sometimes
// add its own 0.1 to the clearances, I believe this is happening after
// the load process (and moving traces) so I am of the opinion this is
// still needed.
static const double safetyMargin = 0.1;


bool PCB_EDIT_FRAME::ExportSpecctraFile( const wxString& aFullFilename )
{
    SPECCTRA_DB     db;
    bool            ok = true;
    wxString        errorText;

    BASE_SCREEN*    screen = GetScreen();
    bool            wasModified = screen->IsModify();

    db.SetPCB( SPECCTRA_DB::MakePCB() );

    LOCALE_IO       toggle;     // Switch the locale to standard C

    // DSN Images (=KiCad MODULES and pads) must be presented from the
    // top view.  So we temporarily flip any modules which are on the back
    // side of the board to the front, and record this in the MODULE's flag field.
    db.FlipMODULEs( GetBoard() );

    try
    {
        GetBoard()->SynchronizeNetsAndNetClasses();
        db.FromBOARD( GetBoard() );
        db.ExportPCB( aFullFilename, true );

        // if an exception is thrown by FromBOARD or ExportPCB(), then
        // ~SPECCTRA_DB() will close the file.
    }
    catch( const IO_ERROR& ioe )
    {
        ok = false;

        // copy the error string to safe place, ioe is in this scope only.
        errorText = ioe.What();
    }

    // done assuredly, even if an exception was thrown and caught.
    db.RevertMODULEs( GetBoard() );

    // The two calls below to MODULE::Flip(), both set the
    // modified flag, yet their actions cancel each other out, so it should
    // be ok to clear the modify flag.
    if( !wasModified )
        screen->ClrModify();

    if( ok )
    {
        SetStatusText( wxString( _( "BOARD exported OK." ) ) );
    }
    else
    {
        DisplayErrorMessage( this,
                             _( "Unable to export, please fix and try again" ),
                             errorText );
    }

    return ok;
}


namespace DSN {

const KICAD_T SPECCTRA_DB::scanPADs[] = { PCB_PAD_T, EOT };

// "specctra reported units" are what we tell the external router that our
// exported lengths are in.


/**
 * Function scale
 * converts a distance from PCBNEW internal units to the reported specctra dsn units
 * in floating point format.
 */
static inline double scale( int kicadDist )
{
    // nanometers to um
    return kicadDist / ( IU_PER_MM / 1000.0 );
}


// / Convert integer internal units to float um
static inline double IU2um( int kicadDist )
{
    return kicadDist * (1000.0 / IU_PER_MM);
}


static inline double mapX( int x )
{
    return scale( x );
}


static inline double mapY( int y )
{
    return -scale( y );      // make y negative, since it is increasing going down.
}


/**
 * Function mapPt
 * converts a KiCad point into a DSN file point.  Kicad's BOARD coordinates
 * are in nanometers (called Internal Units or IU)and we are exporting in units
 * of mils, so we have to scale them.
 */
static POINT mapPt( const wxPoint& pt )
{
    POINT ret;

    ret.x   = mapX( pt.x );
    ret.y   = mapY( pt.y );
    ret.FixNegativeZero();
    return ret;
}


/**
 * Function isRoundKeepout
 * decides if the pad is a copper-less through hole which needs to be made into
 * a round keepout.
 */
static bool isRoundKeepout( D_PAD* aPad )
{
    if( aPad->GetShape()==PAD_SHAPE_CIRCLE )
    {
        if( aPad->GetDrillSize().x >= aPad->GetSize().x )
            return true;

        if( !( aPad->GetLayerSet() & LSET::AllCuMask() ).any() )
            return true;
    }

    return false;
}


/**
 * Function makePath
 * creates a PATH element with a single straight line, a pair of vertices.
 */
static PATH* makePath( const POINT& aStart, const POINT& aEnd, const std::string& aLayerName )
{
    PATH* path = new PATH( 0, T_path );

    path->AppendPoint( aStart );
    path->AppendPoint( aEnd );
    path->SetLayerId( aLayerName.c_str() );
    return path;
}


PADSTACK* SPECCTRA_DB::makePADSTACK( BOARD* aBoard, D_PAD* aPad )
{
    char        name[256];                  // padstack name builder
    std::string uniqifier;

    // caller must do these checks before calling here.
    wxASSERT( !isRoundKeepout( aPad ) );

    PADSTACK*   padstack = new PADSTACK();

    int         reportedLayers = 0;         // how many in reported padstack
    const char* layerName[MAX_CU_LAYERS];

    uniqifier = '[';

    static const LSET all_cu = LSET::AllCuMask();

    bool onAllCopperLayers = ( (aPad->GetLayerSet() & all_cu) == all_cu );

    if( onAllCopperLayers )
        uniqifier += 'A'; // A for all layers

    const int copperCount = aBoard->GetCopperLayerCount();

    for( int layer=0; layer<copperCount; ++layer )
    {
        PCB_LAYER_ID kilayer = pcbLayer2kicad[layer];

        if( onAllCopperLayers || aPad->IsOnLayer( kilayer ) )
        {
            layerName[reportedLayers++] = layerIds[layer].c_str();

            if( !onAllCopperLayers )
            {
                if( layer == 0 )
                    uniqifier += 'T';
                else if( layer == copperCount - 1 )
                    uniqifier += 'B';
                else
                    uniqifier += char('0' + layer); // layer index char
            }
        }
    }

    uniqifier += ']';

    POINT   dsnOffset;

    if( aPad->GetOffset().x || aPad->GetOffset().y )
    {
        char offsetTxt[64];

        wxPoint offset( aPad->GetOffset().x, aPad->GetOffset().y );

        dsnOffset = mapPt( offset );

        // using '(' or ')' would cause padstack name to be quote wrapped,
        // so use other brackets, and {} locks freerouter.
        sprintf( offsetTxt, "[%.6g,%.6g]", dsnOffset.x, dsnOffset.y );

        uniqifier += offsetTxt;
    }

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE_CIRCLE:
        {
            double diameter = scale( aPad->GetSize().x );

            for( int ndx=0; ndx<reportedLayers; ++ndx )
            {
                SHAPE* shape = new SHAPE( padstack );

                padstack->Append( shape );

                CIRCLE* circle = new CIRCLE( shape );

                shape->SetShape( circle );

                circle->SetLayerId( layerName[ndx] );
                circle->SetDiameter( diameter );
                circle->SetVertex( dsnOffset );
            }

            snprintf( name, sizeof(name), "Round%sPad_%.6g_um",
                      uniqifier.c_str(), IU2um( aPad->GetSize().x ) );

            name[ sizeof(name) - 1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;

    case PAD_SHAPE_RECT:
        {
            double  dx  = scale( aPad->GetSize().x ) / 2.0;
            double  dy  = scale( aPad->GetSize().y ) / 2.0;

            POINT   lowerLeft( -dx, -dy );
            POINT   upperRight( dx, dy );

            lowerLeft   += dsnOffset;
            upperRight  += dsnOffset;

            for( int ndx=0;  ndx<reportedLayers;  ++ndx )
            {
                SHAPE* shape = new SHAPE( padstack );

                padstack->Append( shape );

                RECTANGLE* rect = new RECTANGLE( shape );

                shape->SetShape( rect );

                rect->SetLayerId( layerName[ndx] );
                rect->SetCorners( lowerLeft, upperRight );
            }

            snprintf( name, sizeof(name), "Rect%sPad_%.6gx%.6g_um",
                      uniqifier.c_str(),
                      IU2um( aPad->GetSize().x ),
                      IU2um( aPad->GetSize().y ) );

            name[ sizeof(name) - 1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;

    case PAD_SHAPE_OVAL:
        {
            double  dx  = scale( aPad->GetSize().x ) / 2.0;
            double  dy  = scale( aPad->GetSize().y ) / 2.0;
            double  dr  = dx - dy;
            double  radius;
            POINT   pstart;
            POINT   pstop;

            if( dr >= 0 )       // oval is horizontal
            {
                radius = dy;

                pstart   = POINT( -dr, 0.0 );
                pstop    = POINT(  dr, 0.0 );
            }
            else        // oval is vertical
            {
                radius  = dx;
                dr      = -dr;

                pstart   = POINT( 0.0, -dr );
                pstop    = POINT( 0.0, dr );
            }

            pstart   += dsnOffset;
            pstop    += dsnOffset;

            for( int ndx=0; ndx<reportedLayers; ++ndx )
            {
                SHAPE*  shape;
                PATH*   path;
                // see http://www.freerouting.net/usren/viewtopic.php?f=3&t=317#p408
                shape = new SHAPE( padstack );

                padstack->Append( shape );
                path = makePath( pstart, pstop, layerName[ndx] );
                shape->SetShape( path );
                path->aperture_width = 2.0 * radius;
            }

            snprintf( name, sizeof(name), "Oval%sPad_%.6gx%.6g_um",
                      uniqifier.c_str(),
                      IU2um( aPad->GetSize().x ),
                      IU2um( aPad->GetSize().y ) );
            name[ sizeof(name) - 1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;

    case PAD_SHAPE_TRAPEZOID:
        {
            double  dx  = scale( aPad->GetSize().x ) / 2.0;
            double  dy  = scale( aPad->GetSize().y ) / 2.0;

            double  ddx = scale( aPad->GetDelta().x ) / 2.0;
            double  ddy = scale( aPad->GetDelta().y ) / 2.0;

            // see class_pad_draw_functions.cpp which draws the trapezoid pad
            POINT   lowerLeft(  -dx - ddy, -dy - ddx );
            POINT   upperLeft(  -dx + ddy, +dy + ddx );
            POINT   upperRight( +dx - ddy, +dy - ddx );
            POINT   lowerRight( +dx + ddy, -dy + ddx );

            lowerLeft   += dsnOffset;
            upperLeft   += dsnOffset;
            upperRight  += dsnOffset;
            lowerRight  += dsnOffset;

            for( int ndx=0; ndx<reportedLayers; ++ndx )
            {
                SHAPE* shape = new SHAPE( padstack );

                padstack->Append( shape );

                // a T_polygon exists as a PATH
                PATH* polygon = new PATH( shape, T_polygon );

                shape->SetShape( polygon );

                polygon->SetLayerId( layerName[ndx] );

                polygon->AppendPoint( lowerLeft );
                polygon->AppendPoint( upperLeft );
                polygon->AppendPoint( upperRight );
                polygon->AppendPoint( lowerRight );
            }

            // this string _must_ be unique for a given physical shape
            snprintf( name, sizeof(name), "Trapz%sPad_%.6gx%.6g_%c%.6gx%c%.6g_um",
                     uniqifier.c_str(), IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ),
                     aPad->GetDelta().x < 0 ? 'n' : 'p',
                     std::abs( IU2um( aPad->GetDelta().x )),
                     aPad->GetDelta().y < 0 ? 'n' : 'p',
                     std::abs( IU2um( aPad->GetDelta().y ) )
                     );
            name[ sizeof(name)-1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;

    case PAD_SHAPE_CHAMFERED_RECT:
    case PAD_SHAPE_ROUNDRECT:
        {
            // Export the shape as as polygon, round rect does not exist as primitive
            const int circleToSegmentsCount = 36;
            int rradius = aPad->GetRoundRectCornerRadius();
            SHAPE_POLY_SET cornerBuffer;
            // Use a slightly bigger shape because the round corners are approximated by
            // segments, giving to the polygon a slightly smaller shape than the actual shape

            /* calculates the coeff to compensate radius reduction of holes clearance
             * due to the segment approx.
             * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
             * correctionFactor is cos( PI/s_CircleToSegmentsCount  )
             */
            double correctionFactor = cos( M_PI / (double) circleToSegmentsCount );
            int extra_clearance = KiROUND( rradius * (1.0 - correctionFactor ) );
            wxSize psize = aPad->GetSize();
            psize.x += extra_clearance*2;
            psize.y += extra_clearance*2;
            rradius += extra_clearance;
            bool doChamfer = aPad->GetShape() == PAD_SHAPE_CHAMFERED_RECT;

            TransformRoundChamferedRectToPolygon( cornerBuffer, wxPoint(0,0), psize,
                                         0, rradius,
                                         aPad->GetChamferRectRatio(),
                                         doChamfer ? aPad->GetChamferPositions() : 0,
                                         aBoard->GetDesignSettings().m_MaxError );
            SHAPE_LINE_CHAIN& polygonal_shape = cornerBuffer.Outline( 0 );

            for( int ndx=0; ndx < reportedLayers; ++ndx )
            {
                SHAPE* shape = new SHAPE( padstack );

                padstack->Append( shape );

                // a T_polygon exists as a PATH
                PATH* polygon = new PATH( shape, T_polygon );

                shape->SetShape( polygon );

                polygon->SetLayerId( layerName[ndx] );
                // append a closed polygon
                POINT first_corner;

                for( int idx = 0; idx < polygonal_shape.PointCount(); idx++ )
                {
                    POINT corner( scale( polygonal_shape.Point( idx ).x ),
                                  scale( -polygonal_shape.Point( idx ).y ) );
                    corner += dsnOffset;
                    polygon->AppendPoint( corner );

                    if( idx == 0 )
                        first_corner = corner;
                }
                polygon->AppendPoint( first_corner );   // Close polygon
            }

            // this string _must_ be unique for a given physical shape
            snprintf( name, sizeof(name), "RoundRect%sPad_%.6gx%.6g_%.6g_um",
                      uniqifier.c_str(),
                      IU2um( aPad->GetSize().x ),
                      IU2um( aPad->GetSize().y ), IU2um( rradius ) );

            name[ sizeof(name) - 1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;

    case PAD_SHAPE_CUSTOM:
        {
            std::vector<wxPoint> polygonal_shape;
            const SHAPE_POLY_SET& pad_shape = aPad->GetCustomShapeAsPolygon();

            #ifdef EXPORT_CUSTOM_PADS_CONVEX_HULL
            BuildConvexHull( polygonal_shape, pad_shape );
            #else
            const SHAPE_LINE_CHAIN& p_outline = pad_shape.COutline( 0 );
            for( int ii = 0; ii < p_outline.PointCount(); ++ii )
                polygonal_shape.push_back( wxPoint( p_outline.CPoint( ii ) ) );
            #endif

            // The polygon must be closed
            if( polygonal_shape.front() != polygonal_shape.back() )
                polygonal_shape.push_back( polygonal_shape.front() );

            for( int ndx=0; ndx < reportedLayers; ++ndx )
            {
                SHAPE* shape = new SHAPE( padstack );

                padstack->Append( shape );

                // a T_polygon exists as a PATH
                PATH* polygon = new PATH( shape, T_polygon );

                shape->SetShape( polygon );

                polygon->SetLayerId( layerName[ndx] );

                for( unsigned idx = 0; idx < polygonal_shape.size(); idx++ )
                {
                    POINT corner( scale( polygonal_shape[idx].x ), scale( -polygonal_shape[idx].y ) );
                    corner += dsnOffset;
                    polygon->AppendPoint( corner );
                }
            }

            // this string _must_ be unique for a given physical shape, so try to make it unique
            EDA_RECT rect = aPad->GetBoundingBox();
            snprintf( name, sizeof(name), "Cust%sPad_%.6gx%.6g_%.6gx_%.6g_%d_um",
                     uniqifier.c_str(), IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ),
                     IU2um( rect.GetWidth() ), IU2um( rect.GetHeight() ),
                     (int)polygonal_shape.size() );
            name[ sizeof(name)-1 ] = 0;

            padstack->SetPadstackId( name );
        }
        break;
    }

    return padstack;
}


/// data type used to ensure unique-ness of pin names, holding (wxString and int)
typedef std::map<wxString, int> PINMAP;


IMAGE* SPECCTRA_DB::makeIMAGE( BOARD* aBoard, MODULE* aModule )
{
    PINMAP      pinmap;
    wxString    padName;

    PCB_TYPE_COLLECTOR  moduleItems;

    // get all the MODULE's pads.
    moduleItems.Collect( aModule, scanPADs );

    IMAGE*  image = new IMAGE(0);

    image->image_id = aModule->GetFPID().Format().c_str();

    // from the pads, and make an IMAGE using collated padstacks.
    for( int p=0; p < moduleItems.GetCount(); ++p )
    {
        D_PAD* pad = (D_PAD*) moduleItems[p];

        // see if this pad is a through hole with no copper on its perimeter
        if( isRoundKeepout( pad ) )
        {
            double  diameter = scale( pad->GetDrillSize().x );
            POINT   vertex   = mapPt( pad->GetPos0() );

            int layerCount = aBoard->GetCopperLayerCount();

            for( int layer=0;  layer<layerCount;  ++layer )
            {
                KEEPOUT* keepout = new KEEPOUT( image, T_keepout );

                image->keepouts.push_back( keepout );

                CIRCLE* circle = new CIRCLE( keepout );

                keepout->SetShape( circle );

                circle->SetDiameter( diameter );
                circle->SetVertex( vertex );
                circle->SetLayerId( layerIds[layer].c_str() );
            }
        }
        // else if() could there be a square keepout here?

        else
        {
            // Pads not on copper layers (i.e. only on tech layers) are ignored
            // because they create invalid pads in .dsn file for freeroute
            LSET mask_copper_layers = pad->GetLayerSet() & LSET::AllCuMask();

            if( !mask_copper_layers.any() )
                continue;

            PADSTACK*               padstack = makePADSTACK( aBoard, pad );
            PADSTACKSET::iterator   iter = padstackset.find( *padstack );

            if( iter != padstackset.end() )
            {
                // padstack is a duplicate, delete it and use the original
                delete padstack;
                padstack = (PADSTACK*) *iter.base();    // folklore, be careful here
            }
            else
            {
                padstackset.insert( padstack );
            }

            PIN* pin = new PIN( image );

            padName     = pad->GetName();
            pin->pin_id = TO_UTF8( padName );

            if( padName!=wxEmptyString && pinmap.find( padName )==pinmap.end() )
            {
                pinmap[ padName ] = 0;
            }
            else    // pad name is a duplicate within this module
            {
                char    buf[32];

                int     duplicates = ++pinmap[ padName ];

                sprintf( buf, "@%d", duplicates );

                pin->pin_id += buf;      // append "@1" or "@2", etc. to pin name
            }

            pin->kiNetCode = pad->GetNetCode();

            image->pins.push_back( pin );

            pin->padstack_id = padstack->padstack_id;

            double angle = pad->GetOrientationDegrees() - aModule->GetOrientationDegrees();
            NORMALIZE_ANGLE_DEGREES_POS( angle );
            pin->SetRotation( angle );

            wxPoint pos( pad->GetPos0() );

            pin->SetVertex( mapPt( pos ) );
        }
    }

#if 1    // enable image (outline) scopes.
    static const KICAD_T scanEDGEs[] = { PCB_MODULE_EDGE_T, EOT };

    // get all the MODULE's EDGE_MODULEs and convert those to DSN outlines.
    moduleItems.Collect( aModule, scanEDGEs );

    for( int i = 0; i<moduleItems.GetCount(); ++i )
    {
        EDGE_MODULE*    graphic = (EDGE_MODULE*) moduleItems[i];
        SHAPE*          outline;
        PATH*           path;

        switch( graphic->GetShape() )
        {
        case S_SEGMENT:
            outline = new SHAPE( image, T_outline );

            image->Append( outline );
            path = new PATH( outline );

            outline->SetShape( path );
            path->SetAperture( scale( graphic->GetWidth() ) );
            path->SetLayerId( "signal" );
            path->AppendPoint( mapPt( graphic->GetStart0() ) );
            path->AppendPoint( mapPt( graphic->GetEnd0() ) );
            break;

        case S_CIRCLE:
            {
                // this is best done by 4 QARC's but freerouter does not yet support QARCs.
                // for now, support by using line segments.

                outline = new SHAPE( image, T_outline );

                image->Append( outline );
                path = new PATH( outline );

                outline->SetShape( path );
                path->SetAperture( scale( graphic->GetWidth() ) );
                path->SetLayerId( "signal" );

                // Do the math using KiCad units, that way we stay out of the
                // scientific notation range of floating point numbers in the
                // DSN file.   We do not parse scientific notation in our own
                // lexer/beautifier, and the spec is not clear that this is
                // required.  Fixed point floats are all that should be needed.

                double radius = GetLineLength( graphic->GetStart(), graphic->GetEnd() );

                // seg count to approximate circle by line segments
                int seg_per_circle = GetArcToSegmentCount( radius, ARC_LOW_DEF, 360.0 );

                for( int ii = 0; ii < seg_per_circle; ++ii )
                {
                    double radians =  2*M_PI / seg_per_circle * ii;
                    wxPoint point( KiROUND( radius * cos( radians ) ),
                                   KiROUND( radius * sin( radians ) ) );

                    point += graphic->m_Start0;     // an offset

                    path->AppendPoint( mapPt( point ) );
                }
                // The shape must be closed
                wxPoint point( radius , 0 );
                point += graphic->m_Start0;
                path->AppendPoint( mapPt( point ) );
            }
            break;

        case S_RECT:
        case S_ARC:
        default:
            DBG( printf( "makeIMAGE(): unsupported shape %s\n",
                       TO_UTF8( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) ); )
            continue;
        }
    }

#endif

    return image;
}


PADSTACK* SPECCTRA_DB::makeVia( int aCopperDiameter, int aDrillDiameter,
                               int aTopLayer, int aBotLayer )
{
    char        name[48];
    PADSTACK*   padstack = new PADSTACK();
    double      dsnDiameter = scale( aCopperDiameter );

    for( int layer=aTopLayer; layer<=aBotLayer; ++layer )
    {
        SHAPE* shape = new SHAPE( padstack );

        padstack->Append( shape );

        CIRCLE* circle = new CIRCLE( shape );

        shape->SetShape( circle );

        circle->SetDiameter( dsnDiameter );
        circle->SetLayerId( layerIds[layer].c_str() );
    }

    snprintf( name, sizeof(name), "Via[%d-%d]_%.6g:%.6g_um",
              aTopLayer, aBotLayer, dsnDiameter,
              // encode the drill value into the name for later import
              IU2um( aDrillDiameter )
              );

    name[ sizeof(name) - 1 ] = 0;
    padstack->SetPadstackId( name );

    return padstack;
}


PADSTACK* SPECCTRA_DB::makeVia( const ::VIA* aVia )
{
    PCB_LAYER_ID    topLayerNum;
    PCB_LAYER_ID    botLayerNum;

    aVia->LayerPair( &topLayerNum, &botLayerNum );

    int topLayer = kicadLayer2pcb[topLayerNum];
    int botLayer = kicadLayer2pcb[botLayerNum];

    if( topLayer > botLayer )
        std::swap( topLayer, botLayer );

    return makeVia( aVia->GetWidth(), aVia->GetDrillValue(), topLayer, botLayer );
}


void SPECCTRA_DB::fillBOUNDARY( BOARD* aBoard, BOUNDARY* boundary )
{
    wxString errMessage;
    SHAPE_POLY_SET outlines;

    aBoard->GetBoardPolygonOutlines( outlines, &errMessage );

    for( int cnt = 0; cnt < outlines.OutlineCount(); cnt++ )   // Should be one outline
    {
        PATH*  path = new PATH( boundary );
        boundary->paths.push_back( path );
        path->layer_id = "pcb";

        SHAPE_LINE_CHAIN& outline = outlines.Outline( cnt );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
        {
            wxPoint pos( outline.Point( ii ).x, outline.Point( ii ).y );
            path->AppendPoint( mapPt( pos ) );
        }

        // Close polygon:
        wxPoint pos0( outline.Point( 0 ).x, outline.Point( 0 ).y );
        path->AppendPoint( mapPt( pos0 ) );

        // Generate holes as keepout:
        for( int ii = 0; ii < outlines.HoleCount( cnt ); ii++ )
        {
            // emit a signal layers keepout for every interior polygon left...
            KEEPOUT*    keepout = new KEEPOUT( NULL, T_keepout );
            PATH*       poly_ko = new PATH( NULL, T_polygon );

            keepout->SetShape( poly_ko );
            poly_ko->SetLayerId( "signal" );
            pcb->structure->keepouts.push_back( keepout );

            SHAPE_LINE_CHAIN& hole = outlines.Hole( cnt, ii );

            for( int jj = 0; jj < hole.PointCount(); jj++ )
            {
                wxPoint pos( hole.Point( jj ).x, hole.Point( jj ).y );
                poly_ko->AppendPoint( mapPt( pos ) );
            }

            // Close polygon:
            wxPoint pos( hole.Point( 0 ).x, hole.Point( 0 ).y );
            poly_ko->AppendPoint( mapPt( pos ) );
        }
    }

    if( !errMessage.IsEmpty() )
        wxLogMessage( errMessage );
}


typedef std::set<std::string>                   STRINGSET;
typedef std::pair<STRINGSET::iterator, bool>    STRINGSET_PAIR;


void SPECCTRA_DB::FromBOARD( BOARD* aBoard )
{
    PCB_TYPE_COLLECTOR     items;

    static const KICAD_T    scanMODULEs[] = { PCB_MODULE_T, EOT };

    // Not all boards are exportable.  Check that all reference Ids are unique.
    // Unless they are unique, we cannot import the session file which comes
    // back to us later from the router.
    {
        items.Collect( aBoard, scanMODULEs );

        STRINGSET       refs;       // holds module reference designators

        for( int i=0;  i<items.GetCount();  ++i )
        {
            MODULE* module = (MODULE*) items[i];

            if( module->GetReference() == wxEmptyString )
            {
                THROW_IO_ERROR( wxString::Format( _( "Symbol with value of \"%s\" has empty reference id." ),
                                                  GetChars( module->GetValue() ) ) );
            }

            // if we cannot insert OK, that means the reference has been seen before.
            STRINGSET_PAIR refpair = refs.insert( TO_UTF8( module->GetReference() ) );
            if( !refpair.second )      // insert failed
            {
                THROW_IO_ERROR( wxString::Format( _( "Multiple symbols have identical reference IDs of \"%s\"." ),
                                                  GetChars( module->GetReference() ) ) );
            }
        }
    }

    if( !pcb )
        pcb = SPECCTRA_DB::MakePCB();

    //-----<layer_descriptor>-----------------------------------------------
    {
        // specctra wants top physical layer first, then going down to the
        // bottom most physical layer in physical sequence.
        // @question : why does KiCad not display layers in that order?

        buildLayerMaps( aBoard );

        int layerCount = aBoard->GetCopperLayerCount();

        for( int pcbNdx=0; pcbNdx<layerCount; ++pcbNdx )
        {
            LAYER* layer = new LAYER( pcb->structure );

            pcb->structure->layers.push_back( layer );

            layer->name = layerIds[pcbNdx];

            DSN_T layerType;

            switch( aBoard->GetLayerType( pcbLayer2kicad[pcbNdx] ) )
            {
            default:
            case LT_SIGNAL: layerType = T_signal;       break;
            case LT_POWER:  layerType = T_power;        break;

#if 1       // Freerouter does not support type "mixed", only signal and power.
            // Remap "mixed" to "signal".
            case LT_MIXED:  layerType = T_signal;       break;
#else
            case LT_MIXED:  layerType = T_mixed;        break;
#endif
            case LT_JUMPER: layerType = T_jumper;       break;
            }

            layer->layer_type = layerType;

            layer->properties.push_back( PROPERTY() );
            PROPERTY*   property = &layer->properties.back();
            property->name = "index";
            char        temp[32];
            sprintf( temp, "%d", pcbNdx );
            property->value = temp;
        }
    }

    // a space in a quoted token is NOT a terminator, true establishes this.
    pcb->parser->space_in_quoted_tokens = true;

    //-----<unit_descriptor> & <resolution_descriptor>--------------------
    {
        // tell freerouter to use "tenths of micrometers",
        // which is 100 nm resolution.  Possibly more resolution is possible
        // in freerouter, but it would need testing.

        pcb->unit->units = T_um;
        pcb->resolution->units  = T_um;
        pcb->resolution->value  = 10;       // tenths of a um
        // pcb->resolution->value = 1000;   // "thousandths of a um" (i.e. "nm")
    }

    //-----<boundary_descriptor>------------------------------------------
    {
        // Because fillBOUNDARY() can throw an exception, we link in an
        // empty boundary so the BOUNDARY does not get lost in the event of
        // of an exception.
        BOUNDARY* boundary = new BOUNDARY( 0 );

        pcb->structure->SetBOUNDARY( boundary );
        fillBOUNDARY( aBoard, boundary );
    }


    //-----<rules>--------------------------------------------------------
    {
        char        rule[80];
        NETCLASSPTR defaultClass = aBoard->GetDesignSettings().GetDefault();

        int         defaultTrackWidth   = defaultClass->GetTrackWidth();
        int         defaultClearance    = defaultClass->GetClearance();

        double      clearance = scale( defaultClearance );

        STRINGS&    rules = pcb->structure->rules->rules;

        sprintf( rule, "(width %.6g)", scale( defaultTrackWidth ) );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g)", clearance + safetyMargin );
        rules.push_back( rule );

        // On a high density board (a board with 4 mil tracks, 4 mil spacing)
        // a typical solder mask clearance will be 2-3 mils.
        // This exposes 2 to 3 mils of bare board around each pad, and would
        // leave only 1 to 2 mils of solder mask between the solder mask's boundary
        // to the edge of any trace within "clearance" of the pad.  So we need at least
        // 2 mils *extra* clearance for traces which would come near a pad on
        // a different net.  So if the baseline trace to trace clearance was say 4 mils, then
        // the SMD to trace clearance should be at least 6 mils.
        double default_smd = clearance + safetyMargin;

        if( default_smd <= 6.0 )
            default_smd = 6.0;

        sprintf( rule, "(clearance %.6g (type default_smd))", default_smd );

        rules.push_back( rule );

        /* see: http://www.freerouting.net/usren/viewtopic.php?f=5&t=339#p474
        sprintf( rule, "(clearance %.6g (type pad_to_turn_gap))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type smd_to_turn_gap))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_via))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_smd))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_pin))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type pin_pin))", clearance + safetyMargin );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type smd_pin))", clearance + safetyMargin );
        rules.push_back( rule );
        */

        // Pad to pad spacing on a single SMT part can be closer than our
        // clearance, we don't want freerouter complaining about that, so
        // output a significantly smaller pad to pad clearance to freerouter.
        clearance = scale( defaultClearance ) / 4;

        sprintf( rule, "(clearance %.6g (type smd_smd))", clearance );
        rules.push_back( rule );
    }


    //-----<zone containers (not keepout areas) become planes>--------------------------------
    // Note: only zones are output here, keepout areas be be created later
    {
        int netlessZones = 0;

        static const KICAD_T scanZONEs[] = { PCB_ZONE_AREA_T, EOT };
        items.Collect( aBoard, scanZONEs );

        for( int i = 0; i<items.GetCount(); ++i )
        {
            ZONE_CONTAINER* item = (ZONE_CONTAINER*) items[i];

            if( item->GetIsKeepout() )
                continue;

            // Currently, we export only copper layers
            if( ! IsCopperLayer( item->GetLayer() ) )
                continue;

            COPPER_PLANE*   plane = new COPPER_PLANE( pcb->structure );

            pcb->structure->planes.push_back( plane );

            PATH* mainPolygon = new     PATH( plane, T_polygon );

            plane->SetShape( mainPolygon );

            plane->name = TO_UTF8( item->GetNetname() );

            if( plane->name.size() == 0 )
            {
                char name[32];

                // This is one of those no connection zones, netcode=0, and it has no name.
                // Create a unique, bogus netname.
                NET* no_net = new NET( pcb->network );

                sprintf( name, "@:no_net_%d", netlessZones++ );
                no_net->net_id = name;

                // add the bogus net name to network->nets.
                pcb->network->nets.push_back( no_net );

                // use the bogus net name in the netless zone.
                plane->name = no_net->net_id;
            }

            mainPolygon->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];

            // Handle the main outlines
            SHAPE_POLY_SET::ITERATOR iterator;
            wxPoint startpoint;
            bool is_first_point = true;

            for( iterator = item->IterateWithHoles(); iterator; iterator++ )
            {
                wxPoint point( iterator->x, iterator->y );

                if( is_first_point )
                {
                    startpoint = point;
                    is_first_point = false;
                }

                mainPolygon->AppendPoint( mapPt( point ) );

                // this was the end of the main polygon
                if( iterator.IsEndContour() )
                {
                    // Close polygon
                    mainPolygon->AppendPoint( mapPt( startpoint ) );
                    break;
                }
            }

            WINDOW* window  = 0;
            PATH*   cutout  = 0;

            bool isStartContour = true;

            // handle the cutouts
            for( iterator++; iterator; iterator++ )
            {
                if( isStartContour )
                {
                    is_first_point = true;
                    window = new WINDOW( plane );

                    plane->AddWindow( window );

                    cutout = new PATH( window, T_polygon );

                    window->SetShape( cutout );

                    cutout->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];
                }

                // If the point in this iteration is the last of the contour, the next iteration
                // will start with a new contour.
                isStartContour = iterator.IsEndContour();

                wxASSERT( window );
                wxASSERT( cutout );

                wxPoint point(iterator->x, iterator->y );

                if( is_first_point )
                {
                    startpoint = point;
                    is_first_point = false;
                }

                cutout->AppendPoint( mapPt(point) );

                // Close the polygon
                if( iterator.IsEndContour() )
                    cutout->AppendPoint( mapPt( startpoint ) );
            }
        }
    }

    //-----<zone containers flagged keepout areas become keepout>--------------------------------
    {
        static const KICAD_T  scanZONEs[] = { PCB_ZONE_AREA_T, EOT };
        items.Collect( aBoard, scanZONEs );

        for( int i=0;  i<items.GetCount();  ++i )
        {
            ZONE_CONTAINER* item = (ZONE_CONTAINER*) items[i];

            if( ! item->GetIsKeepout() )
                continue;

            // keepout areas have a type. types are
            // T_place_keepout, T_via_keepout, T_wire_keepout,
            // T_bend_keepout, T_elongate_keepout, T_keepout.
            // Pcbnew knows only T_keepout, T_via_keepout and T_wire_keepout
            DSN_T keepout_type;

            if( item->GetDoNotAllowVias() && item->GetDoNotAllowTracks() )
                keepout_type = T_keepout;
            else if( item->GetDoNotAllowVias() )
                keepout_type = T_via_keepout;
            else if( item->GetDoNotAllowTracks() )
                keepout_type = T_wire_keepout;
            else
                keepout_type = T_keepout;

            // Now, build keepout polygon on each copper layer where the item
            // keepout is living (keepout zones can live on many copper layers)
            const int copperCount = aBoard->GetCopperLayerCount();

            for( int layer = 0; layer < copperCount; layer++ )
            {
                if( layer == copperCount-1)
                    layer = B_Cu;

                if( !item->IsOnLayer( PCB_LAYER_ID( layer ) ) )
                    continue;

                KEEPOUT*   keepout = new KEEPOUT( pcb->structure, keepout_type );
                pcb->structure->keepouts.push_back( keepout );

                PATH* mainPolygon = new PATH( keepout, T_polygon );
                keepout->SetShape( mainPolygon );

                mainPolygon->layer_id = layerIds[ kicadLayer2pcb[ layer ] ];

                // Handle the main outlines
                SHAPE_POLY_SET::ITERATOR iterator;
                bool is_first_point = true;
                wxPoint startpoint;

                for( iterator = item->IterateWithHoles(); iterator; iterator++ )
                {
                    wxPoint point( iterator->x, iterator->y );

                    if( is_first_point )
                    {
                        startpoint = point;
                        is_first_point = false;
                    }

                    mainPolygon->AppendPoint( mapPt(point) );

                    // this was the end of the main polygon
                    if( iterator.IsEndContour() )
                    {
                        mainPolygon->AppendPoint( mapPt( startpoint ) );
                        break;
                    }
                }

                WINDOW* window = nullptr;
                PATH*   cutout = nullptr;

                bool isStartContour = true;

                // handle the cutouts
                for( iterator++; iterator; iterator++ )
                {
                    if( isStartContour )
                    {
                        is_first_point = true;
                        window = new WINDOW( keepout );
                        keepout->AddWindow( window );

                        cutout = new PATH( window, T_polygon );

                        window->SetShape( cutout );

                        cutout->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];
                    }

                    isStartContour = iterator.IsEndContour();

                    wxASSERT( window );
                    wxASSERT( cutout );

                    wxPoint point(iterator->x, iterator->y );

                    if( is_first_point )
                    {
                        startpoint = point;
                        is_first_point = false;
                    }

                    cutout->AppendPoint( mapPt(point) );

                    // Close the polygon
                    if( iterator.IsEndContour() )
                        cutout->AppendPoint( mapPt( startpoint ) );
                }
            }
        }
    }

    //-----<build the images, components, and netlist>-----------------------
    {
        PIN_REF empty( pcb->network );

        std::string componentId;

        // find the highest numbered netCode within the board.
        int highestNetCode = aBoard->GetNetCount() - 1;

        deleteNETs();

        // expand the net vector to highestNetCode+1, setting empty to NULL
        nets.resize( highestNetCode + 1, NULL );

        for( unsigned i = 1 /* skip "No Net" at [0] */; i < nets.size(); ++i )
            nets[i] = new NET( pcb->network );

        for( unsigned ii = 0; ii < aBoard->GetNetCount(); ii++ )
        {
            NETINFO_ITEM*   net     = aBoard->FindNet( ii );
            int             netcode = net->GetNet();

            if( netcode > 0 )
                nets[ netcode ]->net_id = TO_UTF8( net->GetNetname() );
        }

        items.Collect( aBoard, scanMODULEs );

        padstackset.clear();

        for( int m = 0; m<items.GetCount(); ++m )
        {
            MODULE* module = (MODULE*) items[m];

            IMAGE*  image = makeIMAGE( aBoard, module );

            componentId = TO_UTF8( module->GetReference() );

            // create a net list entry for all the actual pins in the image
            // for the current module.  location of this code is critical
            // because we fabricated some pin names to ensure unique-ness
            // of pin names within a module, do not move this code because
            // the life of this 'IMAGE* image' is not necessarily long.  The
            // exported netlist will have some fabricated pin names in it.
            // If you don't like fabricated pin names, then make sure all pads
            // within your MODULEs are uniquely named!
            for( unsigned p = 0; p<image->pins.size(); ++p )
            {
                PIN*    pin = &image->pins[p];

                int     netcode = pin->kiNetCode;

                if( netcode > 0 )
                {
                    NET* net = nets[netcode];

                    net->pins.push_back( empty );

                    PIN_REF& pin_ref = net->pins.back();

                    pin_ref.component_id = componentId;
                    pin_ref.pin_id = pin->pin_id;
                }
            }


            IMAGE* registered = pcb->library->LookupIMAGE( image );

            if( registered != image )
            {
                // If our new 'image' is not a unique IMAGE, delete it.
                // and use the registered one, known as 'image' after this.
                delete image;
                image = registered;
            }

            COMPONENT*  comp = pcb->placement->LookupCOMPONENT( image->GetImageId() );

            PLACE*      place = new PLACE( comp );

            comp->places.push_back( place );

            place->SetRotation( module->GetOrientationDegrees() );
            place->SetVertex( mapPt( module->GetPosition() ) );
            place->component_id = componentId;
            place->part_number  = TO_UTF8( module->GetValue() );

            // module is flipped from bottom side, set side to T_back
            if( module->GetFlag() )
            {
                double angle = 180.0 - module->GetOrientationDegrees();
                NORMALIZE_ANGLE_DEGREES_POS( angle );
                place->SetRotation( angle );

                place->side = T_back;
            }
        }

        // copy the SPECCTRA_DB::padstackset to the LIBRARY.  Since we are
        // removing, do not increment the iterator
        for( PADSTACKSET::iterator i = padstackset.begin(); i!=padstackset.end();
             i = padstackset.begin() )
        {
            PADSTACKSET::auto_type ps = padstackset.release( i );
            PADSTACK* padstack = ps.release();

            pcb->library->AddPadstack( padstack );
        }

        // copy our SPECCTRA_DB::nets to the pcb->network
        for( unsigned n = 1; n<nets.size(); ++n )
        {
            NET* net = nets[n];

            if( net->pins.size() )
            {
                // give ownership to pcb->network
                pcb->network->nets.push_back( net );
                nets[n] = 0;
            }
        }
    }


    //-----< output vias used in netclasses >-----------------------------------
    {
        NETCLASSES& nclasses = aBoard->GetDesignSettings().m_NetClasses;

        // Assume the netclass vias are all the same kind of thru, blind, or buried vias.
        // This is in lieu of either having each netclass via have its own layer pair in
        // the netclass dialog, or such control in the specctra export dialog.


        // if( aBoard->GetDesignSettings().m_CurrentViaType == VIA_THROUGH )
        {
            m_top_via_layer = 0;       // first specctra cu layer is number zero.
            m_bot_via_layer = aBoard->GetCopperLayerCount()-1;
        }
        /*
        else
        {
            // again, should be in the BOARD:
            topLayer = kicadLayer2pcb[ GetScreen()->m_Route_Layer_TOP ];
            botLayer = kicadLayer2pcb[ GetScreen()->m_Route_Layer_BOTTOM ];
        }
        */

        // Add the via from the Default netclass first.  The via container
        // in pcb->library preserves the sequence of addition.

        NETCLASSPTR netclass = nclasses.GetDefault();

        PADSTACK*   via = makeVia( netclass->GetViaDiameter(), netclass->GetViaDrill(),
                                   m_top_via_layer, m_bot_via_layer );

        // we AppendVia() this first one, there is no way it can be a duplicate,
        // the pcb->library via container is empty at this point.  After this,
        // we'll have to use LookupVia().
        wxASSERT( pcb->library->vias.size() == 0 );
        pcb->library->AppendVia( via );

#if 0
        // I've seen no way to make stock vias useable by freerouter.  Also the
        // zero based diameter was leading to duplicates in the LookupVia() function.
        // User should use netclass based vias when going to freerouter.

        // Output the stock vias, but preserve uniqueness in the via container by
        // using LookupVia().
        for( unsigned i = 0; i < aBoard->m_ViasDimensionsList.size(); ++i )
        {
            int viaSize     = aBoard->m_ViasDimensionsList[i].m_Diameter;
            int viaDrill    = aBoard->m_ViasDimensionsList[i].m_Drill;

            via = makeVia( viaSize, viaDrill,
                           m_top_via_layer, m_bot_via_layer );

            // maybe add 'via' to the library, but only if unique.
            PADSTACK* registered = pcb->library->LookupVia( via );

            if( registered != via )
                delete via;
        }
#endif

        // set the "spare via" index at the start of the
        // pcb->library->spareViaIndex = pcb->library->vias.size();

        // output the non-Default netclass vias
        for( NETCLASSES::iterator nc = nclasses.begin(); nc != nclasses.end(); ++nc )
        {
            netclass = nc->second;

            via = makeVia( netclass->GetViaDiameter(), netclass->GetViaDrill(),
                           m_top_via_layer, m_bot_via_layer );

            // maybe add 'via' to the library, but only if unique.
            PADSTACK* registered = pcb->library->LookupVia( via );

            if( registered != via )
                delete via;
        }
    }


#if 1    // do existing wires and vias

    //-----<create the wires from tracks>-----------------------------------
    {
        // export all of them for now, later we'll decide what controls we need
        // on this.
        static const KICAD_T scanTRACKs[] = { PCB_TRACE_T, EOT };

        items.Collect( aBoard, scanTRACKs );

        std::string netname;
        WIRING*     wiring = pcb->wiring;
        PATH*       path = 0;

        int old_netcode = -1;
        int old_width = -1;
        LAYER_NUM old_layer = UNDEFINED_LAYER;

        for( int i=0;  i<items.GetCount();  ++i )
        {
            TRACK*  track = (TRACK*) items[i];

            int     netcode = track->GetNetCode();

            if( netcode == 0 )
                continue;

            if( old_netcode != netcode ||
                old_width   != track->GetWidth() ||
                old_layer   != track->GetLayer() ||
                (path && path->points.back() != mapPt(track->GetStart()) )
              )
            {
                old_width   = track->GetWidth();
                old_layer   = track->GetLayer();

                if( old_netcode != netcode )
                {
                    old_netcode = netcode;
                    NETINFO_ITEM* net = aBoard->FindNet( netcode );
                    wxASSERT( net );
                    netname = TO_UTF8( net->GetNetname() );
                }

                WIRE* wire = new WIRE( wiring );

                wiring->wires.push_back( wire );
                wire->net_id = netname;

                wire->wire_type = T_protect;    // @todo, this should be configurable

                LAYER_NUM kiLayer  = track->GetLayer();
                int pcbLayer = kicadLayer2pcb[kiLayer];

                path = new PATH( wire );

                wire->SetShape( path );

                path->layer_id = layerIds[pcbLayer];
                path->aperture_width = scale( old_width );

                path->AppendPoint( mapPt( track->GetStart() ) );
            }

            if( path )  // Should not occur
                path->AppendPoint( mapPt( track->GetEnd() ) );
        }
    }


    //-----<export the existing real BOARD instantiated vias>-----------------
    {
        // Export all vias, once per unique size and drill diameter combo.
        static const KICAD_T scanVIAs[] = { PCB_VIA_T, EOT };

        items.Collect( aBoard, scanVIAs );

        for( int i = 0; i<items.GetCount(); ++i )
        {
            ::VIA* via = (::VIA*) items[i];
            wxASSERT( via->Type() == PCB_VIA_T );

            int     netcode = via->GetNetCode();

            if( netcode == 0 )
                continue;

            PADSTACK*   padstack    = makeVia( via );
            PADSTACK*   registered  = pcb->library->LookupVia( padstack );

            // if the one looked up is not our padstack, then delete our padstack
            // since it was a duplicate of one already registered.
            if( padstack != registered )
            {
                delete padstack;
            }

            WIRE_VIA* dsnVia = new WIRE_VIA( pcb->wiring );

            pcb->wiring->wire_vias.push_back( dsnVia );

            dsnVia->padstack_id = registered->padstack_id;
            dsnVia->vertexes.push_back( mapPt( via->GetPosition() ) );

            NETINFO_ITEM* net = aBoard->FindNet( netcode );
            wxASSERT( net );

            dsnVia->net_id = TO_UTF8( net->GetNetname() );

            dsnVia->via_type = T_protect;     // @todo, this should be configurable
        }
    }

#endif    // do existing wires and vias

    //-----<via_descriptor>-------------------------------------------------
    {
        // The pcb->library will output <padstack_descriptors> which is a combined
        // list of part padstacks and via padstacks.  specctra dsn uses the
        // <via_descriptors> to say which of those padstacks are vias.

        // Output the vias in the padstack list here, by name only.  This must
        // be done after exporting existing vias as WIRE_VIAs.
        VIA* vias = pcb->structure->via;

        for(  unsigned viaNdx = 0; viaNdx < pcb->library->vias.size(); ++viaNdx )
        {
            vias->AppendVia( pcb->library->vias[viaNdx].padstack_id.c_str() );
        }
    }


    //-----<output NETCLASSs>----------------------------------------------------
    NETCLASSES& nclasses = aBoard->GetDesignSettings().m_NetClasses;

    exportNETCLASS( nclasses.GetDefault(), aBoard );

    for( NETCLASSES::iterator nc = nclasses.begin(); nc != nclasses.end(); ++nc )
    {
        NETCLASSPTR netclass = nc->second;
        exportNETCLASS( netclass, aBoard );
    }
}


void SPECCTRA_DB::exportNETCLASS( const NETCLASSPTR& aNetClass, BOARD* aBoard )
{
    /*  From page 11 of specctra spec:
     *
     *   Routing and Placement Rule Hierarchies
     *
     *   Routing and placement rules can be defined at multiple levels of design
     *   specification. When a routing or placement rule is defined for an object at
     *   multiple levels, a predefined routing or placement precedence order
     *   automatically determines which rule to apply to the object. The routing rule
     *   precedence order is
     *
     *       pcb < layer < class < class layer < group_set < group_set layer < net <
     *       net layer < group < group layer < fromto < fromto layer < class_class <
     *       class_class layer < padstack < region < class region < net region <
     *       class_class region
     *
     *   A pcb rule (global rule for the PCB design) has the lowest precedence in the
     *   hierarchy. A class-to-class region rule has the highest precedence. Rules
     *   set at one level of the hierarchy override conflicting rules set at lower
     *   levels. The placement rule precedence order is
     *
     *       pcb < image_set < image < component < super cluster < room <
     *       room_image_set < family_family < image_image
     *
     *   A pcb rule (global rule for the PCB design) has the lowest precedence in the
     *   hierarchy. An image-to-image rule has the highest precedence. Rules set at
     *   one level of the hierarchy override conflicting rules set at lower levels.
     */

    char    text[256];

    CLASS*  clazz = new CLASS( pcb->network );

    pcb->network->classes.push_back( clazz );

    // freerouter creates a class named 'default' anyway, and if we
    // try and use that, we end up with two 'default' via rules so use
    // something else as the name of our default class.
    clazz->class_id = TO_UTF8( aNetClass->GetName() );

    for( NETCLASS::iterator net = aNetClass->begin(); net != aNetClass->end(); ++net )
        clazz->net_ids.push_back( TO_UTF8( *net ) );

    clazz->rules = new RULE( clazz, T_rule );

    // output the track width.
    int trackWidth = aNetClass->GetTrackWidth();
    sprintf( text, "(width %.6g)", scale( trackWidth ) );
    clazz->rules->rules.push_back( text );

    // output the clearance.
    int clearance = aNetClass->GetClearance();
    sprintf( text, "(clearance %.6g)", scale( clearance ) + safetyMargin );
    clazz->rules->rules.push_back( text );

    if( aNetClass->GetName() == NETCLASS::Default )
    {
        clazz->class_id = "kicad_default";
    }

    // the easiest way to get the via name is to create a via (which generates
    // the name internal to the PADSTACK), and then grab the name and then
    // delete the via.  There are not that many netclasses so
    // this should never become a performance issue.

    PADSTACK* via = makeVia( aNetClass->GetViaDiameter(), aNetClass->GetViaDrill(),
                             m_top_via_layer, m_bot_via_layer );

    snprintf( text, sizeof(text), "(use_via %s)", via->GetPadstackId().c_str() );
    clazz->circuit.push_back( text );

    delete via;
}


void SPECCTRA_DB::FlipMODULEs( BOARD* aBoard )
{
    for( auto module : aBoard->Modules() )
    {
        module->SetFlag( 0 );
        if( module->GetLayer() == B_Cu )
        {
            module->Flip( module->GetPosition(), aBoard->GeneralSettings().m_FlipLeftRight );
            module->SetFlag( 1 );
        }
    }

    modulesAreFlipped = true;
}


void SPECCTRA_DB::RevertMODULEs( BOARD* aBoard )
{
    if( !modulesAreFlipped )
        return;

    // DSN Images (=KiCad MODULES and pads) must be presented from the
    // top view.  Restore those that were flipped.
    for( auto module : aBoard->Modules() )
    {
        if( module->GetFlag() )
        {
            module->Flip( module->GetPosition(), aBoard->GeneralSettings().m_FlipLeftRight );
            module->SetFlag( 0 );
        }
    }

    modulesAreFlipped = false;
}

}       // namespace DSN

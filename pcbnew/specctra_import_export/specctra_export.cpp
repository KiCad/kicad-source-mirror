/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <locale_io.h>
#include <macros.h>
#include <math/util.h>          // for KiROUND

#include <set>                  // std::set
#include <map>                  // std::map

#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <pad.h>
#include <zone.h>
#include <base_units.h>
#include <collectors.h>
#include <geometry/shape_poly_set.h>
#include <geometry/convex_hull.h>
#include <convert_basic_shapes_to_polygon.h>
#include <geometry/geometry_utils.h>
#include <pcbnew_settings.h>
#include <wx/log.h>

#include "specctra.h"

using namespace DSN;

// comment the line #define EXPORT_CUSTOM_PADS_CONVEX_HULL to export CUSTOM pads exact shapes.
// Shapes can be non convex polygons with holes (linked to outline) that can create issues.
// Especially Freerouter does not handle them very well:
// - too complex shapes are not accepted, especially shapes with holes (dsn files are not loaded).
// - and Freerouter actually uses something like a convex hull of the shape (that works poorly).
// I am guessing non convex polygons with holes linked could create issues with any Router.
#define EXPORT_CUSTOM_PADS_CONVEX_HULL


bool PCB_EDIT_FRAME::ExportSpecctraFile( const wxString& aFullFilename )
{
    BASE_SCREEN* screen = GetScreen();
    bool         wasModified = screen->IsContentModified();
    wxString     errorText;
    bool         ok = true;

    try
    {
        ExportBoardToSpecctraFile( GetBoard(), aFullFilename );
    }
    catch( const IO_ERROR& ioe )
    {
        ok = false;

        // copy the error string to safe place, ioe is in this scope only.
        errorText = ioe.What();
    }

    // The two calls to FOOTPRINT::Flip() in ExportBoardToSpecctraFile both set the modified flag,
    // yet their actions cancel each other out, so it should be ok to clear the flag.
    if( !wasModified )
        screen->SetContentModified( false );

    if( ok )
        SetStatusText( wxString( _( "BOARD exported OK." ) ) );
    else
        DisplayErrorMessage( this, _( "Unable to export, please fix and try again" ), errorText );

    return ok;
}


void ExportBoardToSpecctraFile( BOARD* aBoard, const wxString& aFullFilename )
{
    SPECCTRA_DB db;

    db.SetPCB( SPECCTRA_DB::MakePCB() );

    LOCALE_IO toggle; // Switch the locale to standard C

    // Build the board outlines *before* flipping footprints
    if( !db.BuiltBoardOutlines( aBoard ) )
        wxLogWarning( _( "Board outline is malformed. Run DRC for a full analysis." ) );

    // DSN Images (=KiCad FOOTPRINTs and PADs) must be presented from the top view.  So we
    // temporarily flip any footprints which are on the back side of the board to the front,
    // and record this in the FOOTPRINT's flag field.
    db.FlipFOOTPRINTs( aBoard );

    try
    {
        aBoard->SynchronizeNetsAndNetClasses( false );
        db.FromBOARD( aBoard );
        db.ExportPCB( aFullFilename, true );
        db.RevertFOOTPRINTs( aBoard );

        // if an exception is thrown by FromBOARD() or ExportPCB(), then ~SPECCTRA_DB() will
        // close the file.
    }
    catch( ... )
    {
        db.RevertFOOTPRINTs( aBoard );
        throw;
    }
}


namespace DSN {

// "specctra reported units" are what we tell the external router that our exported lengths are in

/**
 * Convert a distance from Pcbnew internal units to the reported Specctra DSN units in floating
 * point format.
 */
static inline double scale( int kicadDist )
{
    // nanometers to um
    return kicadDist / ( pcbIUScale.IU_PER_MM / 1000.0 );
}


///< Convert integer internal units to float um
static inline double IU2um( int kicadDist )
{
    return kicadDist * ( 1000.0 / pcbIUScale.IU_PER_MM );
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
 * Convert a KiCad point into a DSN file point.
 *
 * Kicad's #BOARD coordinates are in nanometers (called Internal Units or IU) and we are
 * exporting in units of mils, so we have to scale them.
 */
static POINT mapPt( const VECTOR2I& pt )
{
    POINT ret;

    ret.x   = mapX( pt.x );
    ret.y   = mapY( pt.y );
    ret.FixNegativeZero();
    return ret;
}


static POINT mapPt( const VECTOR2I& pt, FOOTPRINT* aFootprint )
{
    VECTOR2I fpRelative = pt - aFootprint->GetPosition();
    RotatePoint( fpRelative, -aFootprint->GetOrientation() );
    return mapPt( fpRelative );
}


/**
 * Decide if the pad is a copper-less through hole which needs to be made into a round keepout.
 */
static bool isRoundKeepout( PAD* aPad )
{
    if( aPad->GetShape() == PAD_SHAPE::CIRCLE )
    {
        if( aPad->GetDrillSize().x >= aPad->GetSize().x )
            return true;

        if( !( aPad->GetLayerSet() & LSET::AllCuMask() ).any() )
            return true;
    }

    return false;
}


/**
 * Create a PATH element with a single straight line, a pair of vertices.
 */
static PATH* makePath( const POINT& aStart, const POINT& aEnd, const std::string& aLayerName )
{
    PATH* path = new PATH( nullptr, T_path );

    path->AppendPoint( aStart );
    path->AppendPoint( aEnd );
    path->SetLayerId( aLayerName.c_str() );
    return path;
}


bool SPECCTRA_DB::BuiltBoardOutlines( BOARD* aBoard  )
{
    return aBoard->GetBoardPolygonOutlines( m_brd_outlines );
}


PADSTACK* SPECCTRA_DB::makePADSTACK( BOARD* aBoard, PAD* aPad )
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
        PCB_LAYER_ID kilayer = m_pcbLayer2kicad[layer];

        if( onAllCopperLayers || aPad->IsOnLayer( kilayer ) )
        {
            layerName[reportedLayers++] = m_layerIds[layer].c_str();

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

        VECTOR2I offset( aPad->GetOffset().x, aPad->GetOffset().y );

        dsnOffset = mapPt( offset );

        // using () would cause padstack name to be quoted, and {} locks freerouter, so use [].
        std::snprintf( offsetTxt, sizeof( offsetTxt ), "[%.6g,%.6g]", dsnOffset.x, dsnOffset.y );

        uniqifier += offsetTxt;
    }

    switch( aPad->GetShape() )
    {
    case PAD_SHAPE::CIRCLE:
    {
        double diameter = scale( aPad->GetSize().x );

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
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
        break;
    }

    case PAD_SHAPE::RECT:
    {
        double dx = scale( aPad->GetSize().x ) / 2.0;
        double dy = scale( aPad->GetSize().y ) / 2.0;

        POINT lowerLeft( -dx, -dy );
        POINT upperRight( dx, dy );

        lowerLeft += dsnOffset;
        upperRight += dsnOffset;

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
        {
            SHAPE* shape = new SHAPE( padstack );

            padstack->Append( shape );

            RECTANGLE* rect = new RECTANGLE( shape );

            shape->SetShape( rect );

            rect->SetLayerId( layerName[ndx] );
            rect->SetCorners( lowerLeft, upperRight );
        }

        snprintf( name, sizeof( name ), "Rect%sPad_%.6gx%.6g_um", uniqifier.c_str(),
                  IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ) );

        name[sizeof( name ) - 1] = 0;

        padstack->SetPadstackId( name );
        break;
    }

    case PAD_SHAPE::OVAL:
    {
        double dx = scale( aPad->GetSize().x ) / 2.0;
        double dy = scale( aPad->GetSize().y ) / 2.0;
        double dr = dx - dy;
        double radius;
        POINT  pstart;
        POINT  pstop;

        if( dr >= 0 ) // oval is horizontal
        {
            radius = dy;

            pstart = POINT( -dr, 0.0 );
            pstop = POINT( dr, 0.0 );
        }
        else // oval is vertical
        {
            radius = dx;
            dr = -dr;

            pstart = POINT( 0.0, -dr );
            pstop = POINT( 0.0, dr );
        }

        pstart += dsnOffset;
        pstop += dsnOffset;

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
        {
            SHAPE* shape;
            PATH*  path;

            // see http://www.freerouting.net/usren/viewtopic.php?f=3&t=317#p408
            shape = new SHAPE( padstack );

            padstack->Append( shape );
            path = makePath( pstart, pstop, layerName[ndx] );
            shape->SetShape( path );
            path->aperture_width = 2.0 * radius;
        }

        snprintf( name, sizeof( name ), "Oval%sPad_%.6gx%.6g_um", uniqifier.c_str(),
                  IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ) );
        name[sizeof( name ) - 1] = 0;

        padstack->SetPadstackId( name );
        break;
    }

    case PAD_SHAPE::TRAPEZOID:
    {
        double dx = scale( aPad->GetSize().x ) / 2.0;
        double dy = scale( aPad->GetSize().y ) / 2.0;

        double ddx = scale( aPad->GetDelta().x ) / 2.0;
        double ddy = scale( aPad->GetDelta().y ) / 2.0;

        // see class_pad_draw_functions.cpp which draws the trapezoid pad
        POINT lowerLeft( -dx - ddy, -dy - ddx );
        POINT upperLeft( -dx + ddy, +dy + ddx );
        POINT upperRight( +dx - ddy, +dy - ddx );
        POINT lowerRight( +dx + ddy, -dy + ddx );

        lowerLeft += dsnOffset;
        upperLeft += dsnOffset;
        upperRight += dsnOffset;
        lowerRight += dsnOffset;

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
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
        snprintf( name, sizeof( name ), "Trapz%sPad_%.6gx%.6g_%c%.6gx%c%.6g_um", uniqifier.c_str(),
                  IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ),
                  aPad->GetDelta().x < 0 ? 'n' : 'p', std::abs( IU2um( aPad->GetDelta().x ) ),
                  aPad->GetDelta().y < 0 ? 'n' : 'p', std::abs( IU2um( aPad->GetDelta().y ) ) );
        name[sizeof( name ) - 1] = 0;

        padstack->SetPadstackId( name );
        break;
    }

    case PAD_SHAPE::CHAMFERED_RECT:
    case PAD_SHAPE::ROUNDRECT:
    {
        // Export the shape as as polygon, round rect does not exist as primitive
        const int      circleToSegmentsCount = 36;
        int            rradius = aPad->GetRoundRectCornerRadius();
        SHAPE_POLY_SET cornerBuffer;

        // Use a slightly bigger shape because the round corners are approximated by
        // segments, giving to the polygon a slightly smaller shape than the actual shape

        /* calculates the coeff to compensate radius reduction of holes clearance
         * due to the segment approx.
         * For a circle the min radius is radius * cos( 2PI / s_CircleToSegmentsCount / 2)
         * correctionFactor is cos( PI/s_CircleToSegmentsCount  )
         */
        double correctionFactor = cos( M_PI / (double) circleToSegmentsCount );
        int    extra_clearance = KiROUND( rradius * ( 1.0 - correctionFactor ) );
        VECTOR2I psize = aPad->GetSize();
        psize.x += extra_clearance * 2;
        psize.y += extra_clearance * 2;
        rradius += extra_clearance;
        bool doChamfer = aPad->GetShape() == PAD_SHAPE::CHAMFERED_RECT;

        TransformRoundChamferedRectToPolygon( cornerBuffer, VECTOR2I( 0, 0 ), psize, ANGLE_0,
                                              rradius, aPad->GetChamferRectRatio(),
                                              doChamfer ? aPad->GetChamferPositions() : 0,
                                              0, aBoard->GetDesignSettings().m_MaxError,
                                              ERROR_INSIDE );

        SHAPE_LINE_CHAIN& polygonal_shape = cornerBuffer.Outline( 0 );

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
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
                POINT corner( scale( polygonal_shape.CPoint( idx ).x ),
                              scale( -polygonal_shape.CPoint( idx ).y ) );
                corner += dsnOffset;
                polygon->AppendPoint( corner );

                if( idx == 0 )
                    first_corner = corner;
            }

            polygon->AppendPoint( first_corner ); // Close polygon
        }

        // this string _must_ be unique for a given physical shape
        snprintf( name, sizeof( name ), "RoundRect%sPad_%.6gx%.6g_%.6g_um_%f_%X", uniqifier.c_str(),
                  IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ), IU2um( rradius ),
                  doChamfer ? aPad->GetChamferRectRatio() : 0.0,
                  doChamfer ? aPad->GetChamferPositions() : 0 );

        name[sizeof( name ) - 1] = 0;

        padstack->SetPadstackId( name );
        break;
    }

    case PAD_SHAPE::CUSTOM:
    {
        std::vector<VECTOR2I> polygonal_shape;
        SHAPE_POLY_SET       pad_shape;
        aPad->MergePrimitivesAsPolygon( &pad_shape );

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

        for( int ndx = 0; ndx < reportedLayers; ++ndx )
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
        MD5_HASH hash = pad_shape.GetHash();
        BOX2I    rect = aPad->GetBoundingBox();
        snprintf( name, sizeof( name ), "Cust%sPad_%.6gx%.6g_%.6gx_%.6g_%d_um_%s",
                  uniqifier.c_str(), IU2um( aPad->GetSize().x ), IU2um( aPad->GetSize().y ),
                  IU2um( rect.GetWidth() ), IU2um( rect.GetHeight() ), (int) polygonal_shape.size(),
                  hash.Format( true ).c_str() );
        name[sizeof( name ) - 1] = 0;

        padstack->SetPadstackId( name );
        break;
    }
    }

    return padstack;
}


/// data type used to ensure unique-ness of pin names, holding (wxString and int)
typedef std::map<wxString, int> PINMAP;


IMAGE* SPECCTRA_DB::makeIMAGE( BOARD* aBoard, FOOTPRINT* aFootprint )
{
    PINMAP      pinmap;
    wxString    padNumber;

    PCB_TYPE_COLLECTOR  fpItems;

    // get all the FOOTPRINT's pads.
    fpItems.Collect( aFootprint, { PCB_PAD_T } );

    IMAGE*  image = new IMAGE( nullptr );

    image->m_image_id = aFootprint->GetFPID().Format().c_str();

    // from the pads, and make an IMAGE using collated padstacks.
    for( int p = 0; p < fpItems.GetCount(); ++p )
    {
        PAD* pad = (PAD*) fpItems[p];

        // see if this pad is a through hole with no copper on its perimeter
        if( isRoundKeepout( pad ) )
        {
            double  diameter = scale( pad->GetDrillSize().x );
            POINT   vertex   = mapPt( pad->GetFPRelativePosition() );

            diameter += scale( aBoard->GetDesignSettings().m_HoleClearance * 2 );

            int layerCount = aBoard->GetCopperLayerCount();

            for( int layer=0; layer<layerCount; ++layer )
            {
                KEEPOUT* keepout = new KEEPOUT( image, T_keepout );

                image->m_keepouts.push_back( keepout );

                CIRCLE* circle = new CIRCLE( keepout );

                keepout->SetShape( circle );

                circle->SetDiameter( diameter );
                circle->SetVertex( vertex );
                circle->SetLayerId( m_layerIds[layer].c_str() );
            }
        }
        else        // else if() could there be a square keepout here?
        {
            // Pads not on copper layers (i.e. only on tech layers) are ignored
            // because they create invalid pads in .dsn file for freeroute
            LSET mask_copper_layers = pad->GetLayerSet() & LSET::AllCuMask();

            if( !mask_copper_layers.any() )
                continue;

            PADSTACK*               padstack = makePADSTACK( aBoard, pad );
            PADSTACKSET::iterator   iter = m_padstackset.find( *padstack );

            if( iter != m_padstackset.end() )
            {
                // padstack is a duplicate, delete it and use the original
                delete padstack;
                padstack = (PADSTACK*) *iter.base();    // folklore, be careful here
            }
            else
            {
                m_padstackset.insert( padstack );
            }

            PIN* pin = new PIN( image );

            padNumber   = pad->GetNumber();
            pin->m_pin_id = TO_UTF8( padNumber );

            if( padNumber != wxEmptyString && pinmap.find( padNumber ) == pinmap.end() )
            {
                pinmap[ padNumber ] = 0;
            }
            else    // pad name is a duplicate within this footprint
            {
                int     duplicates = ++pinmap[ padNumber ];

                pin->m_pin_id +=
                        "@" + std::to_string( duplicates ); // append "@1" or "@2", etc. to pin name
            }

            pin->m_kiNetCode = pad->GetNetCode();

            image->m_pins.push_back( pin );

            pin->m_padstack_id = padstack->m_padstack_id;

            EDA_ANGLE angle = pad->GetOrientation() - aFootprint->GetOrientation();
            pin->SetRotation( angle.Normalize().AsDegrees() );

            VECTOR2I pos( pad->GetFPRelativePosition() );

            pin->SetVertex( mapPt( pos ) );
        }
    }

    // get all the FOOTPRINT's SHAPEs and convert those to DSN outlines.
    fpItems.Collect( aFootprint, { PCB_SHAPE_T } );

    for( int i = 0; i < fpItems.GetCount(); ++i )
    {
        PCB_SHAPE* graphic = static_cast<PCB_SHAPE*>( fpItems[i] );
        SHAPE*     outline;
        PATH*      path;

        switch( graphic->GetShape() )
        {
        case SHAPE_T::SEGMENT:
            outline = new SHAPE( image, T_outline );

            image->Append( outline );
            path = new PATH( outline );

            outline->SetShape( path );
            path->SetAperture( scale( graphic->GetWidth() ) );
            path->SetLayerId( "signal" );
            path->AppendPoint( mapPt( graphic->GetStart(), aFootprint ) );
            path->AppendPoint( mapPt( graphic->GetEnd(), aFootprint ) );
            break;

        case SHAPE_T::CIRCLE:
        {
            // this is best done by 4 QARC's but freerouter does not yet support QARCs.
            // for now, support by using line segments.
            outline = new SHAPE( image, T_outline );
            image->Append( outline );

            path = new PATH( outline );

            outline->SetShape( path );
            path->SetAperture( scale( graphic->GetWidth() ) );
            path->SetLayerId( "signal" );

            double radius = graphic->GetRadius();
            VECTOR2I circle_centre = graphic->GetStart();

            SHAPE_LINE_CHAIN polyline;
            ConvertArcToPolyline( polyline, VECTOR2I( circle_centre ), radius, ANGLE_0, ANGLE_360,
                                  ARC_HIGH_DEF, ERROR_INSIDE );

            for( int ii = 0; ii < polyline.PointCount(); ++ii )
            {
                VECTOR2I corner( polyline.CPoint( ii ).x, polyline.CPoint( ii ).y );
                path->AppendPoint( mapPt( corner, aFootprint ) );
            }

            break;
        }

        case SHAPE_T::RECT:
        {
            outline = new SHAPE( image, T_outline );

            image->Append( outline );
            path = new PATH( outline );

            outline->SetShape( path );
            path->SetAperture( scale( graphic->GetWidth() ) );
            path->SetLayerId( "signal" );
            VECTOR2I corner = graphic->GetStart();
            path->AppendPoint( mapPt( corner, aFootprint ) );

            corner.x = graphic->GetEnd().x;
            path->AppendPoint( mapPt( corner, aFootprint ) );

            corner.y = graphic->GetEnd().y;
            path->AppendPoint( mapPt( corner, aFootprint ) );

            corner.x = graphic->GetStart().x;
            path->AppendPoint( mapPt( corner, aFootprint ) );
            break;
        }

        case SHAPE_T::ARC:
        {
            // this is best done by QARC's but freerouter does not yet support QARCs.
            // for now, support by using line segments.
            // So we use a polygon (PATH) to create a approximate arc shape
            outline = new SHAPE( image, T_outline );

            image->Append( outline );
            path = new PATH( outline );

            outline->SetShape( path );
            path->SetAperture( 0 );//scale( graphic->GetWidth() ) );
            path->SetLayerId( "signal" );

            VECTOR2I  arc_centre = graphic->GetCenter();
            double    radius = graphic->GetRadius() + graphic->GetWidth()/2;
            EDA_ANGLE arcAngle = graphic->GetArcAngle();

            VECTOR2I  startRadial = graphic->GetStart() - graphic->GetCenter();
            EDA_ANGLE arcStart( startRadial );

            arcStart.Normalize();

            // For some obscure reason, FreeRouter does not show the same polygonal
            // shape for polygons CW and CCW. So used only the order of corners
            // giving the best look.
            if( arcAngle < ANGLE_0 )
            {
                VECTOR2I endRadial = graphic->GetEnd() - graphic->GetCenter();
                arcStart = EDA_ANGLE( endRadial );
                arcStart.Normalize();

                arcAngle = -arcAngle;
            }

            SHAPE_LINE_CHAIN polyline;
            ConvertArcToPolyline( polyline, VECTOR2I( arc_centre ), radius, arcStart, arcAngle,
                                  ARC_HIGH_DEF, ERROR_INSIDE );

            SHAPE_POLY_SET polyBuffer;
            polyBuffer.AddOutline( polyline );

            radius -= graphic->GetWidth();

            if( radius > 0 )
            {
                polyline.Clear();
                ConvertArcToPolyline( polyline, VECTOR2I( arc_centre ), radius, arcStart, arcAngle,
                                      ARC_HIGH_DEF, ERROR_INSIDE );

                // Add points in reverse order, to create a closed polygon
                for( int ii = polyline.PointCount() - 1; ii >= 0; --ii )
                    polyBuffer.Append( polyline.CPoint( ii ) );
            }

            // ensure the polygon is closed
            polyBuffer.Append( polyBuffer.Outline( 0 ).CPoint( 0 ) );

            VECTOR2I move = graphic->GetCenter() - arc_centre;

            TransformCircleToPolygon( polyBuffer, graphic->GetStart() - move,
                                      graphic->GetWidth() / 2, ARC_HIGH_DEF, ERROR_INSIDE );

            TransformCircleToPolygon( polyBuffer, graphic->GetEnd() - move,
                                      graphic->GetWidth() / 2, ARC_HIGH_DEF, ERROR_INSIDE );

            polyBuffer.Simplify( SHAPE_POLY_SET::PM_FAST );
            SHAPE_LINE_CHAIN& poly = polyBuffer.Outline( 0 );

            for( int ii = 0; ii < poly.PointCount(); ++ii )
            {
                VECTOR2I corner( poly.CPoint( ii ).x, poly.CPoint( ii ).y );
                path->AppendPoint( mapPt( corner, aFootprint ) );
            }

            break;
        }

        default:
            continue;
        }
    }

    for( ZONE* zone : aFootprint->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            continue;

        // IMAGE object coordinates are relative to the IMAGE not absolute board coordinates.
        ZONE untransformedZone( *zone );

        EDA_ANGLE angle = -aFootprint->GetOrientation();
        angle.Normalize();
        untransformedZone.Rotate( aFootprint->GetPosition(), angle );

        // keepout areas have a type. types are
        // T_place_keepout, T_via_keepout, T_wire_keepout,
        // T_bend_keepout, T_elongate_keepout, T_keepout.
        // Pcbnew knows only T_keepout, T_via_keepout and T_wire_keepout
        DSN_T keepout_type;

        if( zone->GetDoNotAllowVias() && zone->GetDoNotAllowTracks() )
            keepout_type = T_keepout;
        else if( zone->GetDoNotAllowVias() )
            keepout_type = T_via_keepout;
        else if( zone->GetDoNotAllowTracks() )
            keepout_type = T_wire_keepout;
        else
            keepout_type = T_keepout;

        // Now, build keepout polygon on each copper layer where the zone
        // keepout is living (keepout zones can live on many copper layers)
        const int copperCount = aBoard->GetCopperLayerCount();

        for( int layer = 0; layer < copperCount; layer++ )
        {
            if( layer == copperCount-1 )
                layer = B_Cu;

            if( !zone->IsOnLayer( PCB_LAYER_ID( layer ) ) )
                continue;

            KEEPOUT* keepout = new KEEPOUT( m_pcb->m_structure, keepout_type );
            image->m_keepouts.push_back( keepout );

            PATH* mainPolygon = new PATH( keepout, T_polygon );
            keepout->SetShape( mainPolygon );

            mainPolygon->layer_id = m_layerIds[ m_kicadLayer2pcb[ layer ] ];

            // Handle the main outlines
            SHAPE_POLY_SET::ITERATOR iterator;
            bool is_first_point = true;
            VECTOR2I startpoint;

            for( iterator = untransformedZone.IterateWithHoles(); iterator; iterator++ )
            {
                VECTOR2I point( iterator->x, iterator->y );

                point -= aFootprint->GetPosition();

                if( is_first_point )
                {
                    startpoint = point;
                    is_first_point = false;
                }

                mainPolygon->AppendPoint( mapPt( point ) );

                // this was the end of the main polygon
                if( iterator.IsEndContour() )
                {
                    mainPolygon->AppendPoint( mapPt( startpoint ) );
                    break;
                }
            }

            WINDOW* window = nullptr;
            PATH*   cutout = nullptr;
            bool    isStartContour = true;

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

                    cutout->layer_id = m_layerIds[ m_kicadLayer2pcb[ zone->GetLayer() ] ];
                }

                isStartContour = iterator.IsEndContour();

                wxASSERT( window );
                wxASSERT( cutout );

                VECTOR2I point( iterator->x, iterator->y );

                point -= aFootprint->GetPosition();

                if( is_first_point )
                {
                    startpoint = point;
                    is_first_point = false;
                }

                cutout->AppendPoint( mapPt( point ) );

                // Close the polygon
                if( iterator.IsEndContour() )
                    cutout->AppendPoint( mapPt( startpoint ) );
            }
        }
    }

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
        circle->SetLayerId( m_layerIds[layer].c_str() );
    }

    snprintf( name, sizeof( name ), "Via[%d-%d]_%.6g:%.6g_um",
              aTopLayer, aBotLayer, dsnDiameter,
              // encode the drill value into the name for later import
              IU2um( aDrillDiameter ) );

    name[ sizeof(name) - 1 ] = 0;
    padstack->SetPadstackId( name );

    return padstack;
}


PADSTACK* SPECCTRA_DB::makeVia( const PCB_VIA* aVia )
{
    PCB_LAYER_ID    topLayerNum;
    PCB_LAYER_ID    botLayerNum;

    aVia->LayerPair( &topLayerNum, &botLayerNum );

    int topLayer = m_kicadLayer2pcb[topLayerNum];
    int botLayer = m_kicadLayer2pcb[botLayerNum];

    if( topLayer > botLayer )
        std::swap( topLayer, botLayer );

    return makeVia( aVia->GetWidth(), aVia->GetDrillValue(), topLayer, botLayer );
}


void SPECCTRA_DB::fillBOUNDARY( BOARD* aBoard, BOUNDARY* boundary )
{
    for( int cnt = 0; cnt < m_brd_outlines.OutlineCount(); cnt++ )   // Should be one outline
    {
        PATH*  path = new PATH( boundary );
        boundary->paths.push_back( path );
        path->layer_id = "pcb";

        SHAPE_LINE_CHAIN& outline = m_brd_outlines.Outline( cnt );

        for( int ii = 0; ii < outline.PointCount(); ii++ )
        {
            VECTOR2I pos( outline.CPoint( ii ).x, outline.CPoint( ii ).y );
            path->AppendPoint( mapPt( pos ) );
        }

        // Close polygon:
        VECTOR2I pos0( outline.CPoint( 0 ).x, outline.CPoint( 0 ).y );
        path->AppendPoint( mapPt( pos0 ) );

        // Generate holes as keepout:
        for( int ii = 0; ii < m_brd_outlines.HoleCount( cnt ); ii++ )
        {
            // emit a signal layers keepout for every interior polygon left...
            KEEPOUT*    keepout = new KEEPOUT( nullptr, T_keepout );
            PATH*       poly_ko = new PATH( nullptr, T_polygon );

            keepout->SetShape( poly_ko );
            poly_ko->SetLayerId( "signal" );
            m_pcb->m_structure->m_keepouts.push_back( keepout );

            SHAPE_LINE_CHAIN& hole = m_brd_outlines.Hole( cnt, ii );

            for( int jj = 0; jj < hole.PointCount(); jj++ )
            {
                VECTOR2I pos( hole.CPoint( jj ).x, hole.CPoint( jj ).y );
                poly_ko->AppendPoint( mapPt( pos ) );
            }

            // Close polygon:
            VECTOR2I pos( hole.CPoint( 0 ).x, hole.CPoint( 0 ).y );
            poly_ko->AppendPoint( mapPt( pos ) );
        }
    }
}


typedef std::set<std::string>                   STRINGSET;
typedef std::pair<STRINGSET::iterator, bool>    STRINGSET_PAIR;


void SPECCTRA_DB::FromBOARD( BOARD* aBoard )
{
    std::shared_ptr<NET_SETTINGS>& netSettings = aBoard->GetDesignSettings().m_NetSettings;

    // Not all boards are exportable.  Check that all reference Ids are unique, or we won't be
    // able to import the session file which comes back to us later from the router.
    {
        STRINGSET refs;       // holds footprint reference designators

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            if( footprint->GetReference() == wxEmptyString )
            {
                THROW_IO_ERROR( wxString::Format( _( "Footprint with value of '%s' has an empty "
                                                     "reference designator." ),
                                                  footprint->GetValue() ) );
            }

            // if we cannot insert OK, that means the reference has been seen before.
            STRINGSET_PAIR refpair = refs.insert( TO_UTF8( footprint->GetReference() ) );

            if( !refpair.second )      // insert failed
            {
                THROW_IO_ERROR( wxString::Format( _( "Multiple footprints have the reference "
                                                     "designator '%s'." ),
                                                  footprint->GetReference() ) );
            }
        }
    }

    if( !m_pcb )
        m_pcb = SPECCTRA_DB::MakePCB();

    //-----<layer_descriptor>-----------------------------------------------
    {
        // Specctra wants top physical layer first, then going down to the bottom most physical
        // layer in physical sequence.

        buildLayerMaps( aBoard );

        int layerCount = aBoard->GetCopperLayerCount();

        for( int pcbNdx=0; pcbNdx<layerCount; ++pcbNdx )
        {
            LAYER* layer = new LAYER( m_pcb->m_structure );

            m_pcb->m_structure->m_layers.push_back( layer );

            layer->name = m_layerIds[pcbNdx];

            DSN_T layerType;

            switch( aBoard->GetLayerType( m_pcbLayer2kicad[pcbNdx] ) )
            {
            default:
            case LT_SIGNAL: layerType = T_signal;       break;
            case LT_POWER:  layerType = T_power;        break;

            // Freerouter does not support type "mixed", only signal and power.
            // Remap "mixed" to "signal".
            case LT_MIXED:  layerType = T_signal;       break;
            case LT_JUMPER: layerType = T_jumper;       break;
            }

            layer->layer_type = layerType;

            layer->properties.push_back( PROPERTY() );
            PROPERTY*   property = &layer->properties.back();
            property->name = "index";
            property->value = std::to_string( pcbNdx );
        }
    }

    // a space in a quoted token is NOT a terminator, true establishes this.
    m_pcb->m_parser->space_in_quoted_tokens = true;

    //-----<unit_descriptor> & <resolution_descriptor>--------------------
    {
        // Tell freerouter to use "tenths of micrometers", which is 100 nm resolution.  Possibly
        // more resolution is possible in freerouter, but it would need testing.

        m_pcb->m_unit->units = T_um;
        m_pcb->m_resolution->units  = T_um;
        m_pcb->m_resolution->value  = 10;       // tenths of a um
    }

    //-----<boundary_descriptor>------------------------------------------
    {
        // Because fillBOUNDARY() can throw an exception, we link in an empty boundary so the
        // BOUNDARY does not get lost in the event of of an exception.
        BOUNDARY* boundary = new BOUNDARY( nullptr );

        m_pcb->m_structure->SetBOUNDARY( boundary );
        fillBOUNDARY( aBoard, boundary );
    }

    //-----<rules>--------------------------------------------------------
    {
        char      rule[80];
        int       defaultTrackWidth = netSettings->m_DefaultNetClass->GetTrackWidth();
        int       defaultClearance  = netSettings->m_DefaultNetClass->GetClearance();
        double    clearance         = scale( defaultClearance );

        STRINGS&  rules = m_pcb->m_structure->m_rules->m_rules;

        std::snprintf( rule, sizeof( rule ), "(width %.6g)", scale( defaultTrackWidth ) );
        rules.push_back( rule );

        std::snprintf( rule, sizeof( rule ), "(clearance %.6g)", clearance );
        rules.push_back( rule );

        // On a high density board (4 mil tracks, 4 mil spacing) a typical solder mask clearance
        // will be 2-3 mils.  This exposes 2 to 3 mils of bare board around each pad, and would
        // leave only 1 to 2 mils of solder mask between the solder mask's boundary and the edge of
        // any trace within "clearance" of the pad.  So we need at least 2 mils *extra* clearance
        // for traces which would come near a pad on a different net.  So if the baseline trace to
        // trace clearance was 4 mils, then the SMD to trace clearance should be at least 6 mils.
        double default_smd = clearance;

        if( default_smd <= 6.0 )
            default_smd = 6.0;

        std::snprintf( rule, sizeof( rule ), "(clearance %.6g (type default_smd))", default_smd );

        rules.push_back( rule );

        // Pad to pad spacing on a single SMT part can be closer than our clearance. We don't want
        // freerouter complaining about that, so output a significantly smaller pad to pad
        // clearance to freerouter.
        clearance = scale( defaultClearance ) / 4;

        std::snprintf( rule, sizeof( rule ), "(clearance %.6g (type smd_smd))", clearance );
        rules.push_back( rule );
    }

    //-----<zones (not keepout areas) become planes>--------------------------------
    // Note: only zones are output here, keepout areas are created later.
    {
        int netlessZones = 0;

        for( ZONE* zone : aBoard->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            // Currently, we export only copper layers
            if( ! zone->IsOnCopperLayer() )
                continue;

            // Now, build zone polygon on each copper layer where the zone
            // is living (zones can live on many copper layers)
            const int copperCount = aBoard->GetCopperLayerCount();

            for( int layer = 0; layer < copperCount; layer++ )
            {
                if( layer == copperCount-1 )
                    layer = B_Cu;

                if( !zone->IsOnLayer( PCB_LAYER_ID( layer ) ) )
                    continue;

                COPPER_PLANE*   plane = new COPPER_PLANE( m_pcb->m_structure );

                m_pcb->m_structure->m_planes.push_back( plane );

                PATH* mainPolygon = new PATH( plane, T_polygon );

                plane->SetShape( mainPolygon );
                plane->m_name = TO_UTF8( zone->GetNetname() );

                if( plane->m_name.size() == 0 )
                {
                    // This is one of those no connection zones, netcode=0, and it has no name.
                    // Create a unique, bogus netname.
                    NET* no_net = new NET( m_pcb->m_network );


                    no_net->m_net_id = "@:no_net_" + std::to_string( netlessZones++ );

                    // add the bogus net name to network->nets.
                    m_pcb->m_network->m_nets.push_back( no_net );

                    // use the bogus net name in the netless zone.
                    plane->m_name = no_net->m_net_id;
                }

                mainPolygon->layer_id = m_layerIds[ m_kicadLayer2pcb[ layer ] ];

                // Handle the main outlines
                SHAPE_POLY_SET::ITERATOR iterator;
                VECTOR2I                 startpoint;
                bool is_first_point = true;

                for( iterator = zone->IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I point( iterator->x, iterator->y );

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

                WINDOW* window  = nullptr;
                PATH*   cutout  = nullptr;

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
                        cutout->layer_id = m_layerIds[ m_kicadLayer2pcb[ layer ] ];
                    }

                    // If the point in this iteration is the last of the contour, the next iteration
                    // will start with a new contour.
                    isStartContour = iterator.IsEndContour();

                    wxASSERT( window );
                    wxASSERT( cutout );

                    VECTOR2I point( iterator->x, iterator->y );

                    if( is_first_point )
                    {
                        startpoint = point;
                        is_first_point = false;
                    }

                    cutout->AppendPoint( mapPt( point ) );

                    // Close the polygon
                    if( iterator.IsEndContour() )
                        cutout->AppendPoint( mapPt( startpoint ) );
                }
            }   // end build zones by layer
        }
    }

    //-----<zones flagged keepout areas become keepout>--------------------------------
    {
        for( ZONE* zone : aBoard->Zones() )
        {
            if( !zone->GetIsRuleArea() )
                continue;

            // Keepout areas have a type: T_place_keepout, T_via_keepout, T_wire_keepout,
            // T_bend_keepout, T_elongate_keepout, T_keepout.
            // Pcbnew knows only T_keepout, T_via_keepout and T_wire_keepout
            DSN_T keepout_type;

            if( zone->GetDoNotAllowVias() && zone->GetDoNotAllowTracks() )
                keepout_type = T_keepout;
            else if( zone->GetDoNotAllowVias() )
                keepout_type = T_via_keepout;
            else if( zone->GetDoNotAllowTracks() )
                keepout_type = T_wire_keepout;
            else
                keepout_type = T_keepout;

            // Now, build keepout polygon on each copper layer where the zone
            // keepout is living (keepout zones can live on many copper layers)
            const int copperCount = aBoard->GetCopperLayerCount();

            for( int layer = 0; layer < copperCount; layer++ )
            {
                if( layer == copperCount - 1 )
                    layer = B_Cu;

                if( !zone->IsOnLayer( PCB_LAYER_ID( layer ) ) )
                    continue;

                KEEPOUT*   keepout = new KEEPOUT( m_pcb->m_structure, keepout_type );
                m_pcb->m_structure->m_keepouts.push_back( keepout );

                PATH* mainPolygon = new PATH( keepout, T_polygon );
                keepout->SetShape( mainPolygon );

                mainPolygon->layer_id = m_layerIds[ m_kicadLayer2pcb[ layer ] ];

                // Handle the main outlines
                SHAPE_POLY_SET::ITERATOR iterator;
                bool                     is_first_point = true;
                VECTOR2I                 startpoint;

                for( iterator = zone->IterateWithHoles(); iterator; iterator++ )
                {
                    VECTOR2I point( iterator->x, iterator->y );

                    if( is_first_point )
                    {
                        startpoint = point;
                        is_first_point = false;
                    }

                    mainPolygon->AppendPoint( mapPt( point ) );

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
                        cutout->layer_id = m_layerIds[ m_kicadLayer2pcb[ layer ] ];
                    }

                    isStartContour = iterator.IsEndContour();

                    wxASSERT( window );
                    wxASSERT( cutout );

                    VECTOR2I point( iterator->x, iterator->y );

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
        PIN_REF       empty( m_pcb->m_network );
        std::string   componentId;
        int           highestNetCode = 0;
        NETINFO_LIST& netInfo = aBoard->GetNetInfo();

        // find the highest numbered netCode within the board.
        for( NETINFO_LIST::iterator i = netInfo.begin(); i != netInfo.end(); ++i )
            highestNetCode = std::max( highestNetCode, i->GetNetCode() );

        deleteNETs();

        // expand the net vector to highestNetCode+1, setting empty to NULL
        m_nets.resize( highestNetCode + 1, nullptr );

        for( unsigned i = 1 /* skip "No Net" at [0] */; i < m_nets.size(); ++i )
            m_nets[i] = new NET( m_pcb->m_network );

        for( NETINFO_LIST::iterator i = netInfo.begin(); i != netInfo.end(); ++i )
        {
            if( i->GetNetCode() > 0 )
                m_nets[i->GetNetCode()]->m_net_id = TO_UTF8( i->GetNetname() );
        }

        m_padstackset.clear();

        for( FOOTPRINT* footprint : aBoard->Footprints() )
        {
            IMAGE* image = makeIMAGE( aBoard, footprint );

            componentId = TO_UTF8( footprint->GetReference() );

            // Create a net list entry for all the actual pins in the current footprint.
            // Location of this code is critical because we fabricated some pin names to ensure
            // unique-ness within a footprint, and the life of this 'IMAGE* image' is not
            // necessarily long.  The exported netlist will have some fabricated pin names in it.
            // If you don't like fabricated pin names, then make sure all pads within your
            // FOOTPRINTs are uniquely named!
            for( unsigned p = 0; p < image->m_pins.size(); ++p )
            {
                PIN* pin = &image->m_pins[p];
                int  netcode = pin->m_kiNetCode;

                if( netcode > 0 )
                {
                    NET* net = m_nets[netcode];

                    net->m_pins.push_back( empty );

                    PIN_REF& pin_ref = net->m_pins.back();

                    pin_ref.component_id = componentId;
                    pin_ref.pin_id = pin->m_pin_id;
                }
            }

            IMAGE* registered = m_pcb->m_library->LookupIMAGE( image );

            if( registered != image )
            {
                // If our new 'image' is not a unique IMAGE, delete it.
                // and use the registered one, known as 'image' after this.
                delete image;
                image = registered;
            }

            COMPONENT*  comp = m_pcb->m_placement->LookupCOMPONENT( image->GetImageId() );
            PLACE*      place = new PLACE( comp );

            comp->m_places.push_back( place );

            place->SetRotation( footprint->GetOrientationDegrees() );
            place->SetVertex( mapPt( footprint->GetPosition() ) );
            place->m_component_id = componentId;
            place->m_part_number  = TO_UTF8( footprint->GetValue() );

            // footprint is flipped from bottom side, set side to T_back
            if( footprint->GetFlag() )
            {
                EDA_ANGLE angle = ANGLE_180 - footprint->GetOrientation();
                place->SetRotation( angle.Normalize().AsDegrees() );

                place->m_side = T_back;
            }
        }

        // copy the SPECCTRA_DB::padstackset to the LIBRARY.  Since we are
        // removing, do not increment the iterator
        for( PADSTACKSET::iterator i = m_padstackset.begin(); i != m_padstackset.end();
             i = m_padstackset.begin() )
        {
            PADSTACKSET::auto_type ps = m_padstackset.release( i );
            PADSTACK* padstack = ps.release();

            m_pcb->m_library->AddPadstack( padstack );
        }

        // copy our SPECCTRA_DB::nets to the pcb->network
        for( unsigned n = 1; n < m_nets.size(); ++n )
        {
            NET* net = m_nets[n];

            if( net->m_pins.size() )
            {
                // give ownership to pcb->network
                m_pcb->m_network->m_nets.push_back( net );
                m_nets[n] = nullptr;
            }
        }
    }

    //-----< output vias used in netclasses >-----------------------------------
    {
        // Assume the netclass vias are all the same kind of thru, blind, or buried vias.
        // This is in lieu of either having each netclass via have its own layer pair in
        // the netclass dialog, or such control in the specctra export dialog.

        m_top_via_layer = 0;       // first specctra cu layer is number zero.
        m_bot_via_layer = aBoard->GetCopperLayerCount()-1;

        // Add the via from the Default netclass first.  The via container
        // in pcb->library preserves the sequence of addition.

        PADSTACK*   via = makeVia( netSettings->m_DefaultNetClass->GetViaDiameter(),
                                   netSettings->m_DefaultNetClass->GetViaDrill(),
                                   m_top_via_layer, m_bot_via_layer );

        // we AppendVia() this first one, there is no way it can be a duplicate,
        // the pcb->library via container is empty at this point.  After this,
        // we'll have to use LookupVia().
        wxASSERT( m_pcb->m_library->m_vias.size() == 0 );
        m_pcb->m_library->AppendVia( via );

        // set the "spare via" index at the start of the
        // pcb->library->spareViaIndex = pcb->library->vias.size();

        // output the non-Default netclass vias
        for( const auto& [ name, netclass ] : netSettings->m_NetClasses )
        {
            via = makeVia( netclass->GetViaDiameter(), netclass->GetViaDrill(),
                           m_top_via_layer, m_bot_via_layer );

            // maybe add 'via' to the library, but only if unique.
            PADSTACK* registered = m_pcb->m_library->LookupVia( via );

            if( registered != via )
                delete via;
        }
    }

    //-----<create the wires from tracks>-----------------------------------
    {
        // export all of them for now, later we'll decide what controls we need on this.
        std::string netname;
        WIRING*     wiring = m_pcb->m_wiring;
        PATH*       path = nullptr;

        int old_netcode = -1;
        int old_width = -1;
        int old_layer = UNDEFINED_LAYER;

        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( !track->IsType( { PCB_TRACE_T, PCB_ARC_T } ) )
                continue;

            int netcode = track->GetNetCode();

            if( netcode == 0 )
                continue;

            if( old_netcode != netcode
                    || old_width != track->GetWidth()
                    || old_layer != track->GetLayer()
                    || ( path && path->points.back() != mapPt( track->GetStart() ) ) )
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
                wire->m_net_id = netname;

                if( track->IsLocked() )
                    wire->m_wire_type = T_fix;    // tracks with fix property are not returned in .ses files
                else
                    wire->m_wire_type = T_route;  // could be T_protect

                int kiLayer  = track->GetLayer();
                int pcbLayer = m_kicadLayer2pcb[kiLayer];

                path = new PATH( wire );
                wire->SetShape( path );
                path->layer_id = m_layerIds[pcbLayer];
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
        for( PCB_TRACK* track : aBoard->Tracks() )
        {
            if( track->Type() != PCB_VIA_T )
                continue;

            PCB_VIA* via = static_cast<PCB_VIA*>( track );
            int      netcode = via->GetNetCode();

            if( netcode == 0 )
                continue;

            PADSTACK*   padstack    = makeVia( via );
            PADSTACK*   registered  = m_pcb->m_library->LookupVia( padstack );

            // if the one looked up is not our padstack, then delete our padstack
            // since it was a duplicate of one already registered.
            if( padstack != registered )
                delete padstack;

            WIRE_VIA* dsnVia = new WIRE_VIA( m_pcb->m_wiring );

            m_pcb->m_wiring->wire_vias.push_back( dsnVia );

            dsnVia->m_padstack_id = registered->m_padstack_id;
            dsnVia->m_vertexes.push_back( mapPt( via->GetPosition() ) );

            NETINFO_ITEM* net = aBoard->FindNet( netcode );
            wxASSERT( net );

            dsnVia->m_net_id = TO_UTF8( net->GetNetname() );

            if( via->IsLocked() )
                dsnVia->m_via_type = T_fix;    // vias with fix property are not returned in .ses files
            else
                dsnVia->m_via_type = T_route;  // could be T_protect
        }
    }

    //-----<via_descriptor>-------------------------------------------------
    {
        // The pcb->library will output <padstack_descriptors> which is a combined list of part
        // padstacks and via padstacks.  specctra dsn uses the <via_descriptors> to say which of
        // those padstacks are vias.

        // Output the vias in the padstack list here, by name only.  This must be done after
        // exporting existing vias as WIRE_VIAs.
        VIA* vias = m_pcb->m_structure->m_via;

        for( unsigned viaNdx = 0; viaNdx < m_pcb->m_library->m_vias.size(); ++viaNdx )
            vias->AppendVia( m_pcb->m_library->m_vias[viaNdx].m_padstack_id.c_str() );
    }

    //-----<output NETCLASSs>----------------------------------------------------

    exportNETCLASS( netSettings->m_DefaultNetClass, aBoard );

    for( const auto& [ name, netclass ] : netSettings->m_NetClasses )
        exportNETCLASS( netclass, aBoard );
}


void SPECCTRA_DB::exportNETCLASS( const std::shared_ptr<NETCLASS>& aNetClass, BOARD* aBoard )
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

    CLASS*  clazz = new CLASS( m_pcb->m_network );

    m_pcb->m_network->m_classes.push_back( clazz );

    // Freerouter creates a class named 'default' anyway, and if we try to use that we end up
    // with two 'default' via rules so use something else as the name of our default class.
    clazz->m_class_id = TO_UTF8( aNetClass->GetName() );

    for( NETINFO_ITEM* net : aBoard->GetNetInfo() )
    {
        if( net->GetNetClass()->GetName() == clazz->m_class_id )
            clazz->m_net_ids.push_back( TO_UTF8( net->GetNetname() ) );
    }

    clazz->m_rules = new RULE( clazz, T_rule );

    // output the track width.
    int trackWidth = aNetClass->GetTrackWidth();
    std::snprintf( text, sizeof( text ), "(width %.6g)", scale( trackWidth ) );
    clazz->m_rules->m_rules.push_back( text );

    // output the clearance.
    int clearance = aNetClass->GetClearance();
    std::snprintf( text, sizeof( text ), "(clearance %.6g)", scale( clearance ) );
    clazz->m_rules->m_rules.push_back( text );

    if( aNetClass->GetName() == NETCLASS::Default )
        clazz->m_class_id = "kicad_default";

    // The easiest way to get the via name is to create a temporary via (which generates the
    // name internal to the PADSTACK), and then grab the name and delete the via.  There are not
    // that many netclasses so this should never become a performance issue.

    PADSTACK* via = makeVia( aNetClass->GetViaDiameter(), aNetClass->GetViaDrill(),
                             m_top_via_layer, m_bot_via_layer );

    snprintf( text, sizeof(text), "(use_via %s)", via->GetPadstackId().c_str() );
    clazz->m_circuit.push_back( text );

    delete via;
}


void SPECCTRA_DB::FlipFOOTPRINTs( BOARD* aBoard )
{
    // DSN Images (=KiCad FOOTPRINTs and PADs) must be presented from the top view.
    // Note: to export footprints, the footprints must be flipped around the X axis, otherwise
    // the rotation angle is not good.
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        footprint->SetFlag( 0 );

        if( footprint->GetLayer() == B_Cu )
        {
            footprint->Flip( footprint->GetPosition(), false );
            footprint->SetFlag( 1 );
        }
    }

    m_footprintsAreFlipped = true;
}


void SPECCTRA_DB::RevertFOOTPRINTs( BOARD* aBoard )
{
    if( !m_footprintsAreFlipped )
        return;

    // DSN Images (=KiCad FOOTPRINTs and PADs) must be presented from the
    // top view.  Restore those that were flipped.
    // Note: to export footprints, the footprints were flipped around the X axis,
    for( FOOTPRINT* footprint : aBoard->Footprints() )
    {
        if( footprint->GetFlag() )
        {
            footprint->Flip( footprint->GetPosition(), false );
            footprint->SetFlag( 0 );
        }
    }

    m_footprintsAreFlipped = false;
}

}       // namespace DSN

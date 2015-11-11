/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 KiCad Developers, see change_log.txt for contributors.
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

#include <wxPcbStruct.h>
#include <pcbstruct.h>          // HISTORY_NUMBER
#include <confirm.h>            // DisplayError()
#include <gestfich.h>           // EDA_FileSelector()
#include <trigo.h>              // RotatePoint()
#include <macros.h>

#include <set>                  // std::set
#include <map>                  // std::map

#include <boost/utility.hpp>    // boost::addressof()

#include <class_board.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_drawsegment.h>
#include <base_units.h>

#include <collectors.h>

#include <geometry/shape_poly_set.h>

#include <specctra.h>

using namespace DSN;


// Add .1 mil to the requested clearances as a safety margin.
// There has been disagreement about interpretation of clearance in the past
// between KiCad and Freerouter, so keep this safetyMargin until the
// disagreement is resolved and stable.  Freerouter seems to be moving
// (protected) traces upon loading the DSN file, and even though it seems to sometimes
// add its own 0.1 to the clearances, I believe this is happening after
// the load process (and moving traces) so I am of the opinion this is
// still needed.
static const double safetyMargin = 0.1;


/**
 * Function close_ness
 * is a non-exact distance calculator used to approximate the distance between
 * two points.  The distance is very in-exact, but can be helpful when used
 * to pick between alternative neighboring points.
 * @param aLeft is the first point
 * @param aRight is the second point
 * @return unsigned - a measure of proximity that the caller knows about, in BIU,
 *  but remember it is only an approximation.
 */
static unsigned close_ness(  const wxPoint& aLeft, const wxPoint& aRight )
{
    // Don't need an accurate distance calculation, just something
    // approximating it, for relative ordering.
    return unsigned( abs( aLeft.x - aRight.x ) + abs( aLeft.y - aRight.y ) );
}


/**
 * Function close_enough
 * is a local and tunable method of qualifying the proximity of two points.
 *
 * @param aLeft is the first point
 * @param aRight is the second point
 * @param aLimit is a measure of proximity that the caller knows about.
 * @return bool - true if the two points are close enough, else false.
 */
inline bool close_enough( const wxPoint& aLeft, const wxPoint& aRight, unsigned aLimit )
{
    // We don't use an accurate distance calculation, just something
    // approximating it, since aLimit is non-exact anyway except when zero.
    return close_ness( aLeft, aRight ) <= aLimit;
}


/**
 * Function close_st
 * is a local method of qualifying if either the start of end point of a segment is closest to a point.
 *
 * @param aReference is the reference point
 * @param aFirst is the first point
 * @param aSecond is the second point
 * @return bool - true if the the first point is closest to the reference, otherwise false.
 */
inline bool close_st( const wxPoint& aReference, const wxPoint& aFirst, const wxPoint& aSecond )
{
    // We don't use an accurate distance calculation, just something
    // approximating to find the closest to the reference.
    return close_ness( aReference, aFirst ) <= close_ness( aReference, aSecond );
}


// see wxPcbStruct.h
void PCB_EDIT_FRAME::ExportToSpecctra( wxCommandEvent& event )
{
    wxString    fullFileName = GetBoard()->GetFileName();
    wxString    path;
    wxString    name;
    wxString    ext;
    wxString    dsn_ext = wxT( ".dsn" );
    wxString    mask    = wxT( "*" ) + dsn_ext;

    wxFileName::SplitPath( fullFileName, &path, &name, &ext );

    name += dsn_ext;

    fullFileName = EDA_FILE_SELECTOR( _( "Specctra DSN file:" ),
                                      path,
                                      name,      // name.ext without path!
                                      dsn_ext,
                                      mask,
                                      this,
                                      wxFD_SAVE,
                                      false );

    if( fullFileName == wxEmptyString )
        return;

    ExportSpecctraFile( fullFileName );
}


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
        db.ExportPCB(  aFullFilename, true );

        // if an exception is thrown by FromBOARD or ExportPCB(), then
        // ~SPECCTRA_DB() will close the file.
    }
    catch( const IO_ERROR& ioe )
    {
        ok = false;

        // copy the error string to safe place, ioe is in this scope only.
        errorText = ioe.errorText;
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
        errorText   += '\n';
        errorText   += _( "Unable to export, please fix and try again." );
        DisplayError( this, errorText );
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
 * Function findPoint
 * searches for a DRAWSEGMENT with an end point or start point of aPoint, and
 * if found, removes it from the TYPE_COLLECTOR and returns it, else returns NULL.
 * @param aPoint The starting or ending point to search for.
 * @param items The list to remove from.
 * @param aLimit is the distance from \a aPoint that still constitutes a valid find.
 * @return DRAWSEGMENT* - The first DRAWSEGMENT that has a start or end point matching
 *   aPoint, otherwise NULL if none.
 */
static DRAWSEGMENT* findPoint( const wxPoint& aPoint, ::PCB_TYPE_COLLECTOR* items, unsigned aLimit )
{
    unsigned min_d = INT_MAX;
    int      ndx_min = 0;

    // find the point closest to aPoint and perhaps exactly matching aPoint.
    for( int i = 0; i < items->GetCount(); ++i )
    {
        DRAWSEGMENT*    graphic = (DRAWSEGMENT*) (*items)[i];
        unsigned        d;

        wxASSERT( graphic->Type() == PCB_LINE_T );

        switch( graphic->GetShape() )
        {
        case S_ARC:
            if( aPoint == graphic->GetArcStart() || aPoint == graphic->GetArcEnd() )
            {
                items->Remove( i );
                return graphic;
            }

            d = close_ness( aPoint, graphic->GetArcStart() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }

            d = close_ness( aPoint, graphic->GetArcEnd() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }
            break;

        default:
            if( aPoint == graphic->GetStart() || aPoint == graphic->GetEnd() )
            {
                items->Remove( i );
                return graphic;
            }

            d = close_ness( aPoint, graphic->GetStart() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }

            d = close_ness( aPoint, graphic->GetEnd() );
            if( d < min_d )
            {
                min_d = d;
                ndx_min = i;
            }
        }
    }

    if( min_d <= aLimit )
    {
        DRAWSEGMENT* graphic = (DRAWSEGMENT*) (*items)[ndx_min];
        items->Remove( ndx_min );
        return graphic;
    }

#if defined(DEBUG)
    if( items->GetCount() )
    {
        printf( "Unable to find segment matching point (%.6g,%.6g) (seg count %d)\n",
                IU2um( aPoint.x )/1000, IU2um( aPoint.y )/1000,
                items->GetCount());

        for( int i = 0; i< items->GetCount(); ++i )
        {
            DRAWSEGMENT* graphic = (DRAWSEGMENT*) (*items)[i];

            printf( "item %d, type=%s, start=%.6g %.6g  end=%.6g,%.6g\n",
                    i + 1,
                    TO_UTF8( BOARD_ITEM::ShowShape( graphic->GetShape() ) ),
                    IU2um( graphic->GetStart().x )/1000,
                    IU2um( graphic->GetStart().y )/1000,
                    IU2um( graphic->GetEnd().x )/1000,
                    IU2um( graphic->GetEnd().y )/1000 );
        }
    }
#endif

    return NULL;
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
        LAYER_ID kilayer = pcbLayer2kicad[layer];

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
    default:
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
            POINT   start;
            POINT   stop;

            if( dr >= 0 )       // oval is horizontal
            {
                radius = dy;

                start   = POINT( -dr, 0.0 );
                stop    = POINT(  dr, 0.0 );
            }
            else        // oval is vertical
            {
                radius  = dx;
                dr      = -dr;

                start   = POINT( 0.0, -dr );
                stop    = POINT( 0.0, dr );
            }

            start   += dsnOffset;
            stop    += dsnOffset;

            for( int ndx=0; ndx<reportedLayers; ++ndx )
            {
                SHAPE*  shape;
                PATH*   path;
                // see http://www.freerouting.net/usren/viewtopic.php?f=3&t=317#p408
                shape = new SHAPE( padstack );

                padstack->Append( shape );
                path = makePath( start, stop, layerName[ndx] );
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
    for( int p=0;  p<moduleItems.GetCount();  ++p )
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

            padName     = pad->GetPadName();
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

            int angle = pad->GetOrientation() - aModule->GetOrientation();    // tenths of degrees

            if( angle )
            {
                NORMALIZE_ANGLE_POS( angle );
                pin->SetRotation( angle / 10.0 );
            }

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

                // better if evenly divisible into 360
                const int DEGREE_INTERVAL = 18;         // 18 means 20 line segments

                for( double radians = 0.0;
                     radians < 2 * M_PI;
                     radians += DEGREE_INTERVAL * M_PI / 180.0 )
                {
                    wxPoint point( KiROUND( radius * cos( radians ) ),
                                   KiROUND( radius * sin( radians ) ) );

                    point += graphic->m_Start0;     // an offset

                    path->AppendPoint( mapPt( point ) );
                }
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
    LAYER_ID    topLayerNum;
    LAYER_ID    botLayerNum;

    aVia->LayerPair( &topLayerNum, &botLayerNum );

    int topLayer = kicadLayer2pcb[topLayerNum];
    int botLayer = kicadLayer2pcb[botLayerNum];

    if( topLayer > botLayer )
        std::swap( topLayer, botLayer );

    return makeVia( aVia->GetWidth(), aVia->GetDrillValue(), topLayer, botLayer );
}


/**
 * Function makeCircle
 * does a line segmented circle into aPath.
 */
static void makeCircle( PATH* aPath, DRAWSEGMENT* aGraphic )
{
    // do a circle segmentation
    const int   STEPS = 2 * 36;

    int         radius  = aGraphic->GetRadius();

    if( radius <= 0 )   // Should not occur, but ...
        return;

    wxPoint     center  = aGraphic->GetCenter();
    double      angle   = 3600.0;
    wxPoint     start = center;
    start.x += radius;

    wxPoint nextPt;

    for( int step = 0; step<STEPS; ++step )
    {
        double rotation = ( angle * step ) / STEPS;

        nextPt = start;

        RotatePoint( &nextPt.x, &nextPt.y, center.x, center.y, rotation );

        aPath->AppendPoint( mapPt( nextPt ) );
    }
}


void SPECCTRA_DB::fillBOUNDARY( BOARD* aBoard, BOUNDARY* boundary )
    throw( IO_ERROR, boost::bad_pointer )
{
    PCB_TYPE_COLLECTOR  items;

    unsigned    prox;           // a proximity BIU metric, not an accurate distance
    const int   STEPS = 36;     // for a segmentation of an arc of 360 degrees

    // Get all the DRAWSEGMENTS and module graphics into 'items',
    // then keep only those on layer == Edge_Cuts.

    static const KICAD_T  scan_graphics[] = { PCB_LINE_T, PCB_MODULE_EDGE_T, EOT };

    items.Collect( aBoard, scan_graphics );

    for( int i = 0; i<items.GetCount(); )
    {
        if( items[i]->GetLayer() != Edge_Cuts )
            items.Remove( i );
        else    // remove graphics not on Edge_Cuts layer
            ++i;
    }

    if( items.GetCount() )
    {
        PATH*  path = new PATH( boundary );
        boundary->paths.push_back( path );
        path->layer_id = "pcb";

        wxPoint         prevPt;

        DRAWSEGMENT*    graphic;

        // Find edge point with minimum x, this should be in the outer polygon
        // which will define the perimeter Edge.Cuts polygon.
        wxPoint xmin    = wxPoint( INT_MAX, 0 );
        int     xmini   = 0;

        for( int i = 0; i < items.GetCount(); i++ )
        {
            graphic = (DRAWSEGMENT*) items[i];

            switch( graphic->GetShape() )
            {
            case S_SEGMENT:
                {
                    if( graphic->GetStart().x < xmin.x )
                    {
                        xmin    = graphic->GetStart();
                        xmini   = i;
                    }

                    if( graphic->GetEnd().x < xmin.x )
                    {
                        xmin    = graphic->GetEnd();
                        xmini   = i;
                    }
                }
                break;

            case S_ARC:
                // Freerouter does not yet understand arcs, so approximate
                // an arc with a series of short lines and put those
                // line segments into the !same! PATH.
                {
                    wxPoint     start   = graphic->GetArcStart();
                    wxPoint     center  = graphic->GetCenter();
                    double      angle   = -graphic->GetAngle();
                    int         steps   = STEPS * fabs(angle) /3600.0;

                    if( steps == 0 )
                        steps = 1;

                    wxPoint     pt;

                    for( int step = 1; step<=steps; ++step )
                    {
                        double rotation = ( angle * step ) / steps;

                        pt = start;

                        RotatePoint( &pt.x, &pt.y, center.x, center.y, rotation );

                        if( pt.x < xmin.x )
                        {
                            xmin  = pt;
                            xmini = i;
                        }
                    }
                }
                break;

            case S_CIRCLE:
                {
                    wxPoint pt = graphic->GetCenter();

                    // pt has minimum x point
                    pt.x -= graphic->GetRadius();

                    // when the radius <= 0, this is a mal-formed circle. Skip it
                    if( graphic->GetRadius() > 0 && pt.x < xmin.x )
                    {
                        xmin  = pt;
                        xmini = i;
                    }
                }
                break;

            default:
                {
                    wxString error = wxString::Format( _( "Unsupported DRAWSEGMENT type %s" ),
                        GetChars( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) );

                    ThrowIOError( error );
                }
                break;
            }
        }

        // Grab the left most point, assume its on the board's perimeter, and see if we
        // can put enough graphics together by matching endpoints to formulate a cohesive
        // polygon.

        graphic = (DRAWSEGMENT*) items[xmini];

        // The first DRAWSEGMENT is in 'graphic', ok to remove it from 'items'
        items.Remove( xmini );

        // Set maximum proximity threshold for point to point nearness metric for
        // board perimeter only, not interior keepouts yet.
        prox = Millimeter2iu( 0.01 );  // should be enough to fix rounding issues
                                        // is arc start and end point calculations

        // Output the Edge.Cuts perimeter as circle or polygon.
        if( graphic->GetShape() == S_CIRCLE )
        {
            makeCircle( path, graphic );
        }
        else
        {
            // Polygon start point. Arbitrarily chosen end of the
            // segment and build the poly from here.

            wxPoint startPt = wxPoint( graphic->GetEnd() );
            prevPt = graphic->GetEnd();
            path->AppendPoint( mapPt( prevPt ) );

            // Do not append the other end point yet of this 'graphic', this first
            // 'graphic' might be an arc.

            for(;;)
            {
                switch( graphic->GetShape() )
                {
                case S_SEGMENT:
                    {
                        wxPoint  nextPt;

                        // Use the line segment end point furthest away from
                        // prevPt as we assume the other end to be ON prevPt or
                        // very close to it.

                        if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                        {
                            nextPt = graphic->GetEnd();
                        }
                        else
                        {
                            nextPt = graphic->GetStart();
                        }

                        path->AppendPoint( mapPt( nextPt ) );
                        prevPt = nextPt;
                    }
                    break;

                case S_ARC:
                    // Freerouter does not yet understand arcs, so approximate
                    // an arc with a series of short lines and put those
                    // line segments into the !same! PATH.
                    {
                        wxPoint start  = graphic->GetArcStart();
                        wxPoint end    = graphic->GetArcEnd();
                        wxPoint center = graphic->GetCenter();
                        double  angle  = -graphic->GetAngle();
                        int     steps  = STEPS * fabs(angle) /3600.0;

                        if( steps == 0 )
                            steps = 1;

                        if( !close_enough( prevPt, start, prox ) )
                        {
                            wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), prox ) );

                            angle = -angle;
                            std::swap( start, end );
                        }

                        wxPoint nextPt;

                        for( int step = 1; step<=steps; ++step )
                        {
                            double rotation = ( angle * step ) / steps;

                            nextPt = start;

                            RotatePoint( &nextPt.x, &nextPt.y, center.x, center.y, rotation );

                            path->AppendPoint( mapPt( nextPt ) );
                        }

                        prevPt = nextPt;
                    }
                    break;

                default:
                    {
                        wxString error = wxString::Format( _( "Unsupported DRAWSEGMENT type %s" ),
                            GetChars( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) );

                        ThrowIOError( error );
                    }
                    break;
                }

                // Get next closest segment.

                graphic = findPoint( prevPt, &items, prox );

                // If there are no more close segments, check if the board
                // outline polygon can be closed.

                if( !graphic )
                {
                    if( close_enough( startPt, prevPt, prox ) )
                    {
                        // Close the polygon back to start point
                        path->AppendPoint( mapPt( startPt ) );
                    }
                    else
                    {
                        wxString error = wxString::Format(
                            _( "Unable to find the next boundary segment with an endpoint of (%s mm, %s mm).\n"
                                  "Edit Edge.Cuts perimeter graphics, making them contiguous polygons each." ),
                            GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.x ).c_str() ) ),
                            GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.y ).c_str() ) )
                        );
                        ThrowIOError( error );
                    }
                    break;
                }
            }
        }

        // Output the interior Edge.Cuts graphics as keepouts, using same nearness
        // metric as the board edge as otherwise we have trouble completing complex
        // polygons.
        prox = Millimeter2iu( 0.05 );

        while( items.GetCount() )
        {
            // emit a signal layers keepout for every interior polygon left...
            KEEPOUT*    keepout = new KEEPOUT( NULL, T_keepout );
            PATH*       poly_ko = new PATH( NULL, T_polygon );

            keepout->SetShape( poly_ko );
            poly_ko->SetLayerId( "signal" );
            pcb->structure->keepouts.push_back( keepout );
            graphic = (DRAWSEGMENT*) items[0];
            items.Remove( 0 );

            if( graphic->GetShape() == S_CIRCLE )
            {
                makeCircle( poly_ko, graphic );
            }
            else
            {
                // Polygon start point. Arbitrarily chosen end of the
                // segment and build the poly from here.

                wxPoint startPt( graphic->GetEnd() );
                prevPt = graphic->GetEnd();
                poly_ko->AppendPoint( mapPt( prevPt ) );

                // do not append the other end point yet, this first 'graphic' might be an arc
                for(;;)
                {
                    switch( graphic->GetShape() )
                    {
                    case S_SEGMENT:
                        {
                            wxPoint nextPt;

                            // Use the line segment end point furthest away from
                            // prevPt as we assume the other end to be ON prevPt or
                            // very close to it.

                            if( close_st( prevPt, graphic->GetStart(), graphic->GetEnd() ) )
                            {
                                nextPt = graphic->GetEnd();
                            }
                            else
                            {
                                nextPt = graphic->GetStart();
                            }

                            prevPt = nextPt;
                            poly_ko->AppendPoint( mapPt( prevPt ) );
                        }
                        break;

                    case S_ARC:
                        // Freerouter does not yet understand arcs, so approximate
                        // an arc with a series of short lines and put those
                        // line segments into the !same! PATH.
                        {
                            wxPoint     start   = graphic->GetArcStart();
                            wxPoint     end     = graphic->GetArcEnd();
                            wxPoint     center  = graphic->GetCenter();
                            double      angle   = -graphic->GetAngle();
                            int         steps   = STEPS * fabs(angle) /3600.0;

                            if( steps == 0 )
                                steps = 1;

                            if( !close_enough( prevPt, start, prox ) )
                            {
                                wxASSERT( close_enough( prevPt, graphic->GetArcEnd(), prox ) );

                                angle = -angle;
                                std::swap( start, end );
                            }

                            wxPoint nextPt;

                            for( int step = 1; step<=steps; ++step )
                            {
                                double rotation = ( angle * step ) / steps;

                                nextPt = start;

                                RotatePoint( &nextPt.x, &nextPt.y, center.x, center.y, rotation );

                                poly_ko->AppendPoint( mapPt( nextPt ) );
                            }

                            prevPt = nextPt;
                        }
                        break;

                    default:
                        {
                            wxString error = wxString::Format(
                                _( "Unsupported DRAWSEGMENT type %s" ),
                                GetChars( BOARD_ITEM::ShowShape( graphic->GetShape() ) ) );

                            ThrowIOError( error );
                        }
                        break;
                    }

                    // Get next closest segment.

                    graphic = findPoint( prevPt, &items, prox );

                    // If there are no more close segments, check if polygon
                    // can be closed.

                    if( !graphic )
                    {
                        if( close_enough( startPt, prevPt, prox ) )
                        {
                            // Close the polygon back to start point
                            poly_ko->AppendPoint( mapPt( startPt ) );
                        }
                        else
                        {
                            wxString error = wxString::Format(
                                _( "Unable to find the next keepout segment with an endpoint of (%s mm, %s mm).\n"
                                   "Edit Edge.Cuts interior graphics, making them contiguous polygons each." ),
                                GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.x ).c_str() ) ),
                                GetChars( FROM_UTF8( BOARD_ITEM::FormatInternalUnits( prevPt.y ).c_str() ) )
                            );

                            ThrowIOError( error );
                        }
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // User has not defined a board perimeter yet...

        EDA_RECT    bbbox = aBoard->ComputeBoundingBox();
        RECTANGLE*  rect = new RECTANGLE( boundary );

        boundary->rectangle = rect;

        rect->layer_id = "pcb";

        // opposite corners
        wxPoint bottomRight( bbbox.GetRight(), bbbox.GetBottom() );

        rect->SetCorners( mapPt( bbbox.GetOrigin() ),
                          mapPt( bottomRight ) );
    }
}

/* This function is not used in SPECCTRA export,
 * but uses a lot of functions from it
 * and is used to extract a board outlines (3D view, automatic zones build ...)
 * makes the board perimeter for the DSN file by filling the BOUNDARY element.
 * Any closed outline inside the main outline is a hole
 * All contours should be closed, i.e. valid closed polygon vertices
 */
bool SPECCTRA_DB::GetBoardPolygonOutlines( BOARD* aBoard,
                                           SHAPE_POLY_SET& aOutlines,
                                           SHAPE_POLY_SET& aHoles,
                                           wxString* aErrorText )
{
    bool success = true;
    double specctra2UIfactor = IU_PER_MM / 1000.0;  // Specctra unite = micron

    if( ! pcb )
    {
        pcb = new PCB();
        pcb->structure = new STRUCTURE( pcb );
    }

    CPolyPt corner;
    BOUNDARY* boundary = new BOUNDARY( 0 );
    pcb->structure->SetBOUNDARY( boundary );

    aOutlines.NewOutline();

    try
    {
        fillBOUNDARY( aBoard, boundary );
        std::vector<double> buffer;
        boundary->GetCorners( buffer );

        for( unsigned ii = 0; ii < buffer.size(); ii+=2 )
        {
            corner.x = buffer[ii] * specctra2UIfactor;
            corner.y =  - buffer[ii+1] * specctra2UIfactor;
            aOutlines.Append( corner.x, corner.y );
        }

        // Export holes, stored as keepouts polygonal shapes.
        // by fillBOUNDARY()
        KEEPOUTS& holes = pcb->structure->keepouts;

        for( KEEPOUTS::iterator i=holes.begin();  i!=holes.end();  ++i )
        {
            KEEPOUT& keepout = *i;
            PATH* poly_hole = (PATH*)keepout.shape;
            POINTS& plist = poly_hole->GetPoints();

            aHoles.NewOutline();

            for( unsigned ii = 0; ii < plist.size(); ii++ )
            {
                corner.x = plist[ii].x * specctra2UIfactor;
                corner.y =  - plist[ii].y * specctra2UIfactor;
                aHoles.Append( corner.x, corner.y );
            }
        }
    }
    catch( const IO_ERROR& ioe )
    {
        // Creates a valid polygon outline is not possible.
        // So uses the board edge cuts bounding box to create a
        // rectangular outline
        // (when no edge cuts items, fillBOUNDARY build a contour
        // from global bounding box
        success = false;
        if( aErrorText )
            *aErrorText = ioe.errorText;

        EDA_RECT bbbox = aBoard->ComputeBoundingBox( true );

        // Ensure non null area. If happen, gives a minimal size.
        if( ( bbbox.GetWidth() ) == 0 || ( bbbox.GetHeight() == 0 ) )
            bbbox.Inflate( Millimeter2iu( 1.0 ) );

        aOutlines.RemoveAllContours();
        aOutlines.NewOutline();

        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetOrigin().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetEnd().y;
        aOutlines.Append( corner.x, corner.y );

        corner.x = bbbox.GetEnd().x;
        corner.y = bbbox.GetOrigin().y;
        aOutlines.Append( corner.x, corner.y );
    }

    return success;
}


typedef std::set<std::string>                   STRINGSET;
typedef std::pair<STRINGSET::iterator, bool>    STRINGSET_PAIR;


void SPECCTRA_DB::FromBOARD( BOARD* aBoard )
    throw( IO_ERROR, boost::bad_ptr_container_operation )
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
                ThrowIOError( _( "Component with value of '%s' has empty reference id." ),
                                GetChars( module->GetValue() ) );
            }

            // if we cannot insert OK, that means the reference has been seen before.
            STRINGSET_PAIR refpair = refs.insert( TO_UTF8( module->GetReference() ) );
            if( !refpair.second )      // insert failed
            {
                ThrowIOError( _( "Multiple components have identical reference IDs of '%s'." ),
                      GetChars( module->GetReference() ) );
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

            int count = item->Outline()->m_CornersList.GetCornersCount();
            int ndx = 0;  // used in 2 for() loops below
            for( ; ndx<count; ++ndx )
            {
                wxPoint   point( item->Outline()->m_CornersList[ndx].x,
                                 item->Outline()->m_CornersList[ndx].y );
                mainPolygon->AppendPoint( mapPt(point) );

                // this was the end of the main polygon
                if( item->Outline()->m_CornersList[ndx].end_contour )
                    break;
            }

            WINDOW* window  = 0;
            PATH*   cutout  = 0;

            // handle the cutouts
            for( ++ndx; ndx<count; ++ndx )
            {
                if( item->Outline()->m_CornersList[ndx-1].end_contour )
                {
                    window = new WINDOW( plane );

                    plane->AddWindow( window );

                    cutout = new PATH( window, T_polygon );

                    window->SetShape( cutout );

                    cutout->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];
                }

                wxASSERT( window );
                wxASSERT( cutout );

                wxPoint point(item->Outline()->m_CornersList[ndx].x,
                              item->Outline()->m_CornersList[ndx].y );
                cutout->AppendPoint( mapPt(point) );
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

            KEEPOUT*   keepout = new KEEPOUT( pcb->structure, keepout_type );
            pcb->structure->keepouts.push_back( keepout );

            PATH* mainPolygon = new PATH( keepout, T_polygon );
            keepout->SetShape( mainPolygon );

            mainPolygon->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];

            int count = item->Outline()->m_CornersList.GetCornersCount();
            int ndx = 0;  // used in 2 for() loops below
            for( ; ndx<count; ++ndx )
            {
                wxPoint   point( item->Outline()->m_CornersList[ndx].x,
                                 item->Outline()->m_CornersList[ndx].y );
                mainPolygon->AppendPoint( mapPt(point) );

                // this was the end of the main polygon
                if( item->Outline()->m_CornersList[ndx].end_contour )
                    break;
            }

            WINDOW* window = 0;
            PATH*   cutout = 0;

            // handle the cutouts
            for( ++ndx; ndx<count; ++ndx )
            {
                if( item->Outline()->m_CornersList[ndx-1].end_contour )
                {
                    window = new WINDOW( keepout );
                    keepout->AddWindow( window );

                    cutout = new PATH( window, T_polygon );
                    window->SetShape( cutout );

                    cutout->layer_id = layerIds[ kicadLayer2pcb[ item->GetLayer() ] ];
                }

                wxASSERT( window );
                wxASSERT( cutout );

                wxPoint point(item->Outline()->m_CornersList[ndx].x,
                              item->Outline()->m_CornersList[ndx].y );
                cutout->AppendPoint( mapPt(point) );
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

        // skip netcode = 0
        for( unsigned i = 1; i<nets.size(); ++i )
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

            place->SetRotation( module->GetOrientation()/10.0 );
            place->SetVertex( mapPt( module->GetPosition() ) );
            place->component_id = componentId;
            place->part_number  = TO_UTF8( module->GetValue() );

            // module is flipped from bottom side, set side to T_back
            if( module->GetFlag() )
            {
                int angle = 1800 - module->GetOrientation();
                NORMALIZE_ANGLE_POS( angle );
                place->SetRotation( angle / 10.0 );

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


void SPECCTRA_DB::exportNETCLASS( NETCLASSPTR aNetClass, BOARD* aBoard )
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
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        module->SetFlag( 0 );
        if( module->GetLayer() == B_Cu )
        {
            module->Flip( module->GetPosition() );
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
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( module->GetFlag() )
        {
            module->Flip( module->GetPosition() );
            module->SetFlag( 0 );
        }
    }

    modulesAreFlipped = false;
}

}       // namespace DSN


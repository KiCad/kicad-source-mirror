/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2007 Kicad Developers, see change_log.txt for contributors.
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
    http://www.autotraxeda.com/docs/SPECCTRA/SPECCTRA.pdf

    Also see the comments at the top of the specctra.cpp file itself.
*/


#include "specctra.h"
#include "collectors.h"
#include "wxPcbStruct.h"        // Change_Side_Module()
#include "pcbstruct.h"          // HISTORY_NUMBER
#include "autorout.h"           // NET_CODES_OK


using namespace DSN;


// see wxPcbStruct.h
void WinEDA_PcbFrame::ExportToSpecctra( wxCommandEvent& event )
{
    wxString        fullFileName = GetScreen()->m_FileName;
    wxString        std_ext = wxT( ".dsn" );
    wxString        mask    = wxT( "*" ) + std_ext;

    ChangeFileNameExt( fullFileName, std_ext );

    fullFileName = EDA_FileSelector( _( "Specctra DSN file:" ),
                                     wxEmptyString,     /* Chemin par defaut */
                                     fullFileName,      /* nom fichier par defaut */
                                     std_ext,           /* extension par defaut */
                                     mask,              /* Masque d'affichage */
                                     this,
                                     wxFD_SAVE,
                                     FALSE
                                     );
    if( fullFileName == wxEmptyString )
        return;

    SPECCTRA_DB     db;
    bool            ok = true;
    wxString        errorText;

    BASE_SCREEN*    screen = GetScreen();
    bool            wasModified = screen->IsModify() && !screen->IsSave();

    db.SetPCB( SPECCTRA_DB::MakePCB() );

    setlocale( LC_NUMERIC, "C" );    // Switch the locale to standard C

    try
    {
        db.FromBOARD( m_Pcb );
        db.ExportPCB(  fullFileName, true );

        // if an exception is thrown by FromBOARD or ExportPCB(), then
        // ~SPECCTRA_DB() will close the file.
    }
    catch( IOError ioe )
    {
        ok = false;

        // copy the error string to safe place, ioe is in this scope only.
        errorText = ioe.errorText;
    }

    setlocale( LC_NUMERIC, "" );      // revert to the current locale

    // The two calls below to BOARD::Change_Side_Module(), both set the
    // modified flag, yet their actions cancel each other out, so it should
    // be ok to clear the modify flag.
    if( !wasModified )
        screen->ClrModify();

    if( ok )
    {
        Affiche_Message( wxString( _("BOARD exported OK.")) );
    }
    else
        DisplayError( this, errorText );
}


namespace DSN {

struct POINT_PAIR
{
    POINT       start;
    POINT       end;
    BOARD_ITEM* item;       ///< the item which has these points, TRACK or DRAWSEGMENT
};
typedef std::vector<POINT_PAIR>     POINT_PAIRS;


const KICAD_T SPECCTRA_DB::scanPADs[] = { TYPEPAD, EOT };


static inline void swap( POINT_PAIR& pair )
{
    POINT temp = pair.start;
    pair.start = pair.end;
    pair.end   = temp;
}


/**
 * Function scale
 * converts a distance from kicad units to our reported specctra dsn units:
 * 1/10000 inches (deci-mils) to mils.  So the factor of 10 comes in.
 */
static inline double scale( int kicadDist )
{
    return kicadDist/10.0;
}

static inline double mapX( int x )
{
    return scale(x);
}

static inline double mapY( int y )
{
    return -scale(y);      // make y negative, since it is increasing going down.
}


/**
 * Function mapPt
 * converts a Kicad point into a DSN file point.  Kicad's BOARD coordinates
 * are in deci-mils  (i.e. 1/10,000th of an inch) and we are exporting in units
 * of mils, so we have to divide by 10.
 */
static POINT mapPt( const wxPoint& pt )
{
    POINT ret;
    ret.x = mapX( pt.x );
    ret.y = mapY( pt.y );
    ret.FixNegativeZero();
    return ret;
}


/**
 * Function findPOINT
 * searches the list of POINT_PAIRS for a matching end to the given POINT.
 * @return int - 0 if no match, or positive one based index of a POINT_PAIR with a matching ".start",
 *              or a negated one based index of a POINT_PAIR with a matching ".end".
 */
static int findPOINT( const POINT& pt, const POINT_PAIR source[], int count )
{
    for( int i=0;  i<count;  ++i )
    {
        if( pt == source[i].start )
        {
            return +( i + 1 );
        }

        if( pt == source[i].end )
        {
            return -( i + 1 );
        }
    }

    return 0;
}


/**
 * Function swapEnds
 * will swap ends of any POINT_PAIR in the POINT_PAIRS list in order to
 * make the consecutive POINT_PAIRs be "connected" at their ends.
 */
static void swapEnds( POINT_PAIRS& aList )
{
    if( !aList.size() )
        return;

    // do an extraction sort based on matching ends here.
    POINT_PAIRS sorted;
    POINT_PAIRS source( aList );

    // try and start the search using a POINT which has at least one match elsewhere.
    if( findPOINT( source.begin()->start, &source[1], source.size()-1 ) != 0 )
        swap( *source.begin() );        // swap start and end of first PAIR

    while( source.size() )
    {
        sorted.push_back( *source.begin() );
        source.erase( source.begin() );

        // keep looping through the source list looking for a match to the end of the last sorted.
        int result;
        while( (result = findPOINT( sorted.back().end, &source[0], source.size() ) ) != 0 )
        {
            int ndx = ABS(result)-1;
            sorted.push_back( source[ ndx ] );
            source.erase( source.begin()+ndx );

            if( result < 0 )
                swap( sorted.back() );
        }
    }

#if 0 && defined(DEBUG)
    printf( "swapEnds():\n" );
    for( unsigned i=0;  i<sorted.size();  ++i )
    {
        printf( "(%.6g,%.6g)  (%.6g,%.6g)\n",
               sorted[i].start.x, sorted[i].start.y,
               sorted[i].end.x,   sorted[i].end.y );
    }
#endif

    aList = sorted;
}


/**
 * Function isRectangle
 * tests to see if the POINT_PAIRS list makes up a vertically/horizontally
 * oriented rectangle.
 * @return bool - true if there are 4 point pairs making a rectangle.
 */
static bool isRectangle( POINT_PAIRS& aList )
{
    if( aList.size() == 4 )
    {
        for( unsigned i=0;  i<aList.size();  ++i )
        {
            if( i < aList.size()-1 )
                if( aList[i].end != aList[i+1].start )
                    return false;

            if( aList[i].start.x != aList[i].end.x
            &&  aList[i].start.y != aList[i].end.y )
                return false;
        }

        return ( aList[0].start == aList[3].end );
    }
    return false;
}


/**************************************************************************/
static int Pad_list_Sort_by_Shapes( const void* refptr, const void* objptr )
/**************************************************************************/
{
    const D_PAD* padref = *(D_PAD**)refptr;
    const D_PAD* padcmp = *(D_PAD**)objptr;

    return D_PAD::Compare( padref, padcmp );
}


/**
 * Function makePath
 * creates a PATH element with a single straight line, a pair of vertices.
 */
static PATH* makePath( const POINT& aStart, const POINT& aEnd, const std::string& aLayerName )
{
    PATH*   path = new PATH( 0, T_path );

    path->AppendPoint( aStart );
    path->AppendPoint( aEnd );
    path->SetLayerId( aLayerName.c_str() );
    return path;
}


IMAGE* SPECCTRA_DB::makeIMAGE( MODULE* aModule )
{
    PADSTACKS&  padstacks = pcb->library->padstacks;

    TYPE_COLLECTOR  pads;

    // get all the MODULE's pads.
    pads.Collect( aModule, scanPADs );

    IMAGE*  image = new IMAGE(0);

    image->image_id = CONV_TO_UTF8( aModule->m_LibRef );

    // from the pads, and make an IMAGE using collated padstacks.
    for( int p=0;  p<pads.GetCount();  ++p )
    {
        D_PAD* pad = (D_PAD*) pads[p];

        // see if this pad is a through hole with no copper on its perimeter
        if( pad->m_PadShape==PAD_CIRCLE &&  pad->m_Drill.x >= pad->m_Size.x )
        {
            KEEPOUT* keepout = new KEEPOUT(image, T_keepout);
            image->keepouts.push_back( keepout );

            CIRCLE* circle = new CIRCLE(keepout);
            keepout->SetShape( circle );

            circle->SetDiameter( scale(pad->m_Drill.x) );
            circle->SetVertex( mapPt( pad->m_Pos0 ) );
            circle->layer_id = "signal";
        }
        else
        {
            PADSTACK*   padstack = &padstacks[pad->m_logical_connexion];

            PIN*    pin = new PIN(image);
            image->pins.push_back( pin );

            pin->padstack_id = padstack->padstack_id;
            pin->pin_id      = CONV_TO_UTF8( pad->ReturnStringPadName() );

            // copper shape's position is hole position + offset
            wxPoint pos = pad->m_Pos0 + pad->m_Offset;

            pin->SetVertex( mapPt( pos )  );
        }
    }

    return image;
}


PADSTACK* SPECCTRA_DB::makeVia( const SEGVIA* aVia )
{
    CIRCLE*     circle;
    SHAPE*      shape;
    double      dsnDiameter;
    char        name[48];

    PADSTACK*   padstack = new PADSTACK( pcb->library );

    switch( aVia->Shape() )
    {
    case VIA_THROUGH:
        shape = new SHAPE( padstack );
        padstack->Append( shape );

        circle = new CIRCLE( shape );
        shape->SetShape( circle );

        dsnDiameter = scale( aVia->m_Width );
        circle->SetDiameter( dsnDiameter );

        circle->SetLayerId( "signal" );

        snprintf( name, sizeof(name),  "Via_%.6g_mil", dsnDiameter );
        name[ sizeof(name)-1 ] = 0;
        padstack->SetPadstackId( name );
        break;

    case VIA_BLIND_BURIED:
    case VIA_MICROVIA:
        int topLayer;
        int botLayer;
        aVia->ReturnLayerPair( &topLayer, &botLayer );
        topLayer = kicadLayer2pcb[topLayer];
        botLayer = kicadLayer2pcb[botLayer];
        if( topLayer > botLayer )
            EXCHG( topLayer, botLayer );

        dsnDiameter = scale( aVia->m_Width );

        for( int layer=topLayer;  layer<=botLayer;  ++layer )
        {
            shape = new SHAPE( padstack );
            padstack->Append( shape );

            circle = new CIRCLE( shape );
            shape->SetShape( circle );

            circle->SetDiameter( dsnDiameter );
            circle->SetLayerId( layerIds[layer].c_str() );
        }

        snprintf( name, sizeof(name),  "Via[%d-%d]_%.6g_mil", topLayer, botLayer, dsnDiameter );
        name[ sizeof(name)-1 ] = 0;
        padstack->SetPadstackId( name );
        break;
    }

    return padstack;
}


PADSTACK* SPECCTRA_DB::makeVia( int aCopperDiameter )
{
    char        name[48];
    PADSTACK*   padstack = new PADSTACK( pcb->library );

    SHAPE*      shape = new SHAPE( padstack );
    padstack->Append( shape );

    CIRCLE*     circle = new CIRCLE( shape );
    shape->SetShape( circle );

    double      dsnDiameter = scale(aCopperDiameter);
    circle->SetDiameter( dsnDiameter );

    circle->SetLayerId( "signal" );

    snprintf( name, sizeof(name),  "Via_%.6g_mil", dsnDiameter );
    name[ sizeof(name)-1 ] = 0;
    padstack->SetPadstackId( name );

    return padstack;
}


void SPECCTRA_DB::makePADSTACKs( BOARD* aBoard, TYPE_COLLECTOR& aPads )
{
    char    name[80];       // padstack name builder

    if( aPads.GetCount() )
    {
        qsort( (void*) aPads.BasePtr(), aPads.GetCount(), sizeof(D_PAD*), Pad_list_Sort_by_Shapes );
    }

    D_PAD*  old_pad = NULL;

    for( int i=0;  i<aPads.GetCount();  ++i )
    {
        D_PAD*  pad = (D_PAD*) aPads[i];

        bool doLayer[2] =  {                    // top and bottom layers only
            pad->IsOnLayer( LAYER_CMP_N ),
            pad->IsOnLayer( COPPER_LAYER_N )
        };

        if( old_pad && 0==D_PAD::Compare( old_pad, pad ) )
        {
            // padstacks.size()-1 is the index of the matching padstack in LIBRARY::padstacks
            pad->m_logical_connexion = pcb->library->padstacks.size()-1;

            // this is the same as the last pad, so do not add it to the padstack list.
            continue;
        }

        // if pad has no copper presence, then it will be made into
        // an "image->keepout" later.  No copper pad here, it is probably a hole.
        if( (!doLayer[0] && !doLayer[1])
           ||  (pad->m_PadShape==PAD_CIRCLE &&  pad->m_Drill.x >= pad->m_Size.x) )
        {
            // padstacks.size()-1 is the index of the matching padstack in LIBRARY::padstacks
            pad->m_logical_connexion = pcb->library->padstacks.size()-1;

            continue;
        }

        old_pad = pad;

        PADSTACK*   padstack = new PADSTACK( pcb->library );
        pcb->library->AddPadstack( padstack );

        // padstacks.size()-1 is the index of the matching padstack in LIBRARY::padstacks
        pad->m_logical_connexion = pcb->library->padstacks.size()-1;

        // For now, we will report only one layer for the pads.  SMD pads are reported on the
        // top layer, and through hole are reported on <reserved_layer_name> "signal".
        // We could do better if there was actually a "layer type" field within
        // Kicad which would hold one of:  T_signal, T_power, T_mixed, T_jumper
        // See bottom of page 74 of the SECCTRA Design Language Reference, May 2000.
        int         reportedLayers = 1;     // how many layers are reported.

        doLayer[0] = true;

        const char* layerName = ( pad->m_Attribut == PAD_SMD ) ?
                                    layerIds[0].c_str() : "signal";

        int         coppers = 0;        // will always be one for now

        switch( pad->m_PadShape )
        {
        default:
        case PAD_CIRCLE:
            {
                double  diameter = scale(pad->m_Size.x);

                for( int layer=0;  layer<reportedLayers;  ++layer )
                {
                    if( doLayer[layer] )
                    {
                        SHAPE*      shape = new SHAPE( padstack );
                        padstack->Append( shape );

                        CIRCLE*     circle = new CIRCLE( shape );
                        shape->SetShape( circle );

                        circle->SetLayerId( layerName );
                        circle->SetDiameter( diameter );
                        ++coppers;
                    }
                }

                snprintf( name, sizeof(name), "Round%dPad_%.6g_mil", coppers, scale(pad->m_Size.x) );
                name[ sizeof(name)-1 ] = 0;

                // @todo verify that all pad names are unique, there is a chance that
                // D_PAD::Compare() could say two pads are different, yet the get the same
                // name here. If so, blend in the padNdx into the name.

                padstack->SetPadstackId( name );
            }
            break;

        case PAD_RECT:
            {
                double dx = scale( pad->m_Size.x ) / 2.0;
                double dy = scale( pad->m_Size.y ) / 2.0;

                POINT   lowerLeft( -dx, -dy );
                POINT   upperRight( dx, dy );

                for( int layer=0;  layer<reportedLayers;  ++layer )
                {
                    if( doLayer[layer] )
                    {
                        SHAPE*      shape = new SHAPE( padstack );
                        padstack->Append( shape );

                        RECTANGLE*  rect = new RECTANGLE( shape );
                        shape->SetShape( rect );

                        rect->SetLayerId( layerName );
                        rect->SetCorners( lowerLeft, upperRight );
                        ++coppers;
                    }
                }

                snprintf( name, sizeof(name),  "Rect%dPad_%.6gx%.6g_mil",
                         coppers, scale(pad->m_Size.x), scale(pad->m_Size.y)  );
                name[ sizeof(name)-1 ] = 0;

                // @todo verify that all pad names are unique, there is a chance that
                // D_PAD::Compare() could say two pads are different, yet they get the same
                // name here. If so, blend in the padNdx into the name.

                padstack->SetPadstackId( name );
            }
            break;

        case PAD_OVAL:
            {
                double dx = scale( pad->m_Size.x ) / 2.0;
                double dy = scale( pad->m_Size.y ) / 2.0;
                double dr = dx - dy;

                if( dr >= 0 )       // oval is horizontal
                {
                    double  radius = dy;

                    for( int layer=0;  layer<reportedLayers;  ++layer )
                    {
                        if( doLayer[layer] )
                        {
                            SHAPE*  shape;
                            PATH*   path;
                            // see http://www.freerouting.net/usren/viewtopic.php?f=3&t=317#p408
                            shape = new SHAPE( padstack );
                            padstack->Append( shape );
                            path = makePath( POINT(-dr, 0.0), POINT(dr, 0.0), layerName );
                            shape->SetShape( path );
                            path->aperture_width = 2.0 * radius;
                            ++coppers;
                        }
                    }
                }
                else        // oval is vertical
                {
                    double  radius = dx;

                    dr = -dr;

                    for( int layer=0;  layer<reportedLayers;  ++layer )
                    {
                        if( doLayer[layer] )
                        {
                            SHAPE*  shape;
                            PATH*   path;
                            // see http://www.freerouting.net/usren/viewtopic.php?f=3&t=317#p408
                            shape = new SHAPE( padstack );
                            padstack->Append( shape );
                            path = makePath( POINT(0.0, -dr), POINT(0.0, dr), layerName );
                            shape->SetShape( path );
                            path->aperture_width = 2.0 * radius;
                            ++coppers;
                        }
                    }
                }

                snprintf( name, sizeof(name),  "Oval%dPad_%.6gx%.6g_mil",
                         coppers, scale(pad->m_Size.x), scale(pad->m_Size.y)  );
                name[ sizeof(name)-1 ] = 0;

                // @todo verify that all pad names are unique, there is a chance that
                // D_PAD::Compare() could say two pads are different, yet they get the same
                // name here. If so, blend in the padNdx into the name.

                padstack->SetPadstackId( name );
            }
            break;

/*
        case PAD_TRAPEZOID:
            break;
*/
        }
    }

    // unique pads are now in the padstack list.
    // next we add the via's which may be used.

    int defaultViaSize = aBoard->m_BoardSettings->m_CurrentViaSize;
    if( defaultViaSize )
    {
        PADSTACK*   padstack = makeVia( defaultViaSize );
        pcb->library->AddPadstack( padstack );

        // remember this index, it is the default via and also the start of the
        // vias within the padstack list.  Before this index are the pads.
        // At this index and later are the vias.
        pcb->library->SetViaStartIndex( pcb->library->padstacks.size()-1 );

        // padstack->SetPadstackId( "Via_Default" );  I like the padstack_id with the size in it.
    }

    for( int i=0;  i<HISTORY_NUMBER;  ++i )
    {
        int viaSize = aBoard->m_BoardSettings->m_ViaSizeHistory[i];
        if( !viaSize )
            break;

        if( viaSize == defaultViaSize )
            continue;

        PADSTACK*   padstack = makeVia( viaSize );
        pcb->library->AddPadstack( padstack );
    }
}


void SPECCTRA_DB::FromBOARD( BOARD* aBoard )
{
    TYPE_COLLECTOR          items;
    POINT_PAIRS             ppairs;
    POINT_PAIR              pair;

    static const KICAD_T    scanMODULEs[] = { TYPEMODULE, EOT };


    if( !pcb )
        pcb = SPECCTRA_DB::MakePCB();

    //  DSN Images (=Kicad MODULES and pads) must be presented from the
    //  top view.  So we temporarily flip any modules which are on the back
    //  side of the board to the front, and record this in the MODULE's flag field.
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        module->flag = 0;
        if( module->GetLayer() == COPPER_LAYER_N )
        {
            aBoard->Change_Side_Module( module, NULL );
            module->flag = 1;
        }
    }

    //pcb->placement->flip_style = T_rotate_first;

    // Since none of these statements cause any immediate output, the order
    // of them is somewhat flexible.  The outputting to disk is done at the
    // end.  We start by gathering all the layer information from the board.


    //-----<layer_descriptor>-----------------------------------------------
    {
        // specctra wants top physical layer first, then going down to the
        // bottom most physical layer in physical sequence.
        // @question : why does Kicad not display layers in that order?

        buildLayerMaps( aBoard );

        int layerCount = aBoard->GetCopperLayerCount();

        for( int pcbNdx=0;  pcbNdx<layerCount; ++pcbNdx )
        {
            LAYER*      layer = new LAYER( pcb->structure );
            pcb->structure->layers.push_back( layer );

            layer->name = layerIds[pcbNdx];

            layer->properties.push_back( PROPERTY() );
            PROPERTY* property = &layer->properties.back();
            property->name = "index";
            char temp[32];
            sprintf( temp, "%d", pcbNdx );
            property->value = temp;

            // layer->type =  @todo need this, the export would be better.
        }
    }


    // for now, report on only the top and bottom layers with respect to the copper
    // within a pad's padstack.  this is usually correct, but not rigorous.

    // a space in a quoted token is NOT a terminator, true establishes this.
    pcb->parser->space_in_quoted_tokens = true;

    //-----<unit_descriptor> & <resolution_descriptor>--------------------
    {
        pcb->unit->units = T_mil;
        pcb->resolution->units = T_mil;
        pcb->resolution->value = 100;
    }


    //-----<boundary_descriptor>------------------------------------------
    {
        // get all the DRAWSEGMENTS into 'items', then look for layer == EDGE_N,
        // and those segments comprise the board's perimeter.
        static const KICAD_T  scanDRAWSEGMENTS[] = { TYPEDRAWSEGMENT, EOT };
        items.Collect( aBoard, scanDRAWSEGMENTS );

        bool haveEdges = false;
        ppairs.clear();
        for( int i=0;  i<items.GetCount();  ++i )
        {
            DRAWSEGMENT* item = (DRAWSEGMENT*) items[i];

            wxASSERT( item->Type() == TYPEDRAWSEGMENT );

            if( item->GetLayer() == EDGE_N )
            {
                pair.start = mapPt( item->m_Start );
                pair.end = mapPt( item->m_End );
                pair.item = item;
                ppairs.push_back( pair );
                haveEdges = true;
            }
        }

        if( haveEdges )
        {
            swapEnds( ppairs );

#if 0 && defined(DEBUG)
            for( unsigned i=0;  i<ppairs.size();  ++i )
            {
                POINT_PAIR* p = &ppairs[i];
                p->item->Show( 0, std::cout );
            }
#endif

            BOUNDARY*   boundary = new BOUNDARY(0);

            if( isRectangle( ppairs ) )
            {
                RECTANGLE*  rect = new RECTANGLE( boundary );
                rect->layer_id = "pcb";

                // opposite corners
                rect->SetCorners( ppairs[0].start, ppairs[2].start );

                boundary->rectangle = rect;
            }
            else
            {
                PATH*  path = new PATH( boundary );
                boundary->paths.push_back( path );

                path->layer_id = "pcb";
                for( unsigned i=0; i<ppairs.size();  ++i )
                {
                    // unless its a closed polygon, this probably won't work,
                    // otherwise it will.
                    path->points.push_back( ppairs[i].start );
                }
            }

            pcb->structure->SetBOUNDARY( boundary );
        }
        else
        {
            aBoard->ComputeBoundaryBox();

            BOUNDARY*   boundary = new BOUNDARY(0);
            RECTANGLE*  rect = new RECTANGLE( boundary );

            rect->layer_id = "pcb";

            // opposite corners
            wxPoint bottomRight;
            bottomRight.x = aBoard->m_BoundaryBox.GetRight();
            bottomRight.y = aBoard->m_BoundaryBox.GetBottom();

            rect->SetCorners( mapPt( aBoard->m_BoundaryBox.GetOrigin() ),
                              mapPt( bottomRight ) );

            boundary->rectangle = rect;

            pcb->structure->SetBOUNDARY( boundary );
        }
    }


    //-----<rules>--------------------------------------------------------
    {
        // put out these rules, the user can then edit them with a text editor
        char    rule[80];       // padstack name builder

        int     curTrackWidth = aBoard->m_BoardSettings->m_CurrentTrackWidth;
        int     curTrackClear = aBoard->m_BoardSettings->m_TrackClearence;
        double  clearance = scale(curTrackClear);
        STRINGS& rules = pcb->structure->rules->rules;

        sprintf( rule, "(width %.6g)", scale( curTrackWidth ) );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g)", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type pad_to_turn_gap))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type smd_to_turn_gap))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_via))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_smd))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type via_pin))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type pin_pin))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type smd_pin))", clearance );
        rules.push_back( rule );

        sprintf( rule, "(clearance %.6g (type smd_smd))", clearance );
        rules.push_back( rule );
    }


    //-----<zone containers become planes>--------------------------------
    {
        static const KICAD_T  scanZONEs[] = { TYPEZONE_CONTAINER, EOT };
        items.Collect( aBoard, scanZONEs );

        for( int i=0;  i<items.GetCount();  ++i )
        {
            ZONE_CONTAINER* item = (ZONE_CONTAINER*) items[i];

            COPPER_PLANE*   plane = new COPPER_PLANE( pcb->structure );
            PATH*           polygon = new PATH( plane, T_polygon );
            plane->SetShape( polygon );

            plane->name = CONV_TO_UTF8( item->m_Netname );

            wxString        layerName = aBoard->GetLayerName( item->GetLayer() );
            polygon->layer_id = CONV_TO_UTF8( layerName );

            int count = item->m_Poly->corner.size();
            for( int j=0; j<count; ++j )
            {
                wxPoint   point( item->m_Poly->corner[j].x, item->m_Poly->corner[j].y );

                polygon->points.push_back( mapPt(point) );
            }

            pcb->structure->planes.push_back( plane );
        }
    }

    // keepouts could go here, there are none in Kicad at this time.
    // although COPPER_PLANEs probably will need them for the thru holes, etc.
    // but in that case they are WINDOWs within the COPPER_PLANEs.


    //-----<build the initial padstack list>--------------------------------
    {
        TYPE_COLLECTOR  pads;

        // get all the D_PADs into 'pads'.
        pads.Collect( aBoard, scanPADs );

        makePADSTACKs( aBoard, pads );

#if 0 && defined(DEBUG)
        for( int p=0;  p<pads.GetCount();  ++p )
            pads[p]->Show( 0, std::cout );
#endif
    }


    //-----<build the images and components>---------------------------------
    {
        items.Collect( aBoard, scanMODULEs );

        for( int m=0;  m<items.GetCount();  ++m )
        {
            MODULE* module = (MODULE*) items[m];

            IMAGE*  image  = makeIMAGE( module );

            IMAGE*  registered = pcb->library->LookupIMAGE( image );
            if( registered != image )
            {
                // If our new 'image' is not a unique IMAGE, delete it.
                // In either case, 'registered' is the one we'll work with henceforth.
                delete image;
            }

            // @todo: this only works if the user has not modified the MODULE within the PCB
            // and made it different from what is in the PCBNEW library.  Need to test
            // each image for uniqueness, not just based on name as is done here:

            COMPONENT* comp = pcb->placement->LookupCOMPONENT( registered->GetImageId() );

            PLACE* place = new PLACE( comp );
            comp->places.push_back( place );

            place->SetRotation( module->m_Orient/10.0 );
            place->SetVertex( mapPt( module->m_Pos ) );
            place->component_id = CONV_TO_UTF8( module->GetReference() );
            place->part_number  = CONV_TO_UTF8( module->GetValue() );

            // module is flipped from bottom side, set side to T_back
            if( module->flag )
            {
                int angle = 1800 - module->m_Orient;

                NORMALIZE_ANGLE_POS(angle);

                place->SetRotation( angle/10.0 );

                place->side = T_back;
            }
        }
    }


    //-----<create the nets>------------------------------------------------
    {
        NETWORK*        network = pcb->network;
        TYPE_COLLECTOR  nets;
        TYPE_COLLECTOR  pads;

        static const KICAD_T scanNETs[] = { PCB_EQUIPOT_STRUCT_TYPE, EOT };

        nets.Collect( aBoard, scanNETs );

        items.Collect( aBoard, scanMODULEs );

        PIN_REF emptypin(0);

        for( int n=0;  n<nets.GetCount();  ++n )
        {
            EQUIPOT*   kinet = (EQUIPOT*) nets[n];

            if( kinet->GetNet() == 0 )
                continue;

            NET* net = new NET( network );
            network->nets.push_back( net );

            net->net_id = CONV_TO_UTF8( kinet->m_Netname );
            net->net_number = kinet->GetNet();

            for( int m=0;  m<items.GetCount();  ++m )
            {
                MODULE* module = (MODULE*) items[m];

                pads.Collect( module, scanPADs );

                for( int p=0;  p<pads.GetCount();  ++p )
                {
                    D_PAD* pad = (D_PAD*) pads[p];

                    if( pad->GetNet() == kinet->GetNet() )
                    {
                        // push on an empty one, then fill it via 'pin_ref'
                        net->pins.push_back( emptypin );
                        PIN_REF* pin_ref = &net->pins.back();

                        pin_ref->SetParent( net );
                        pin_ref->component_id = CONV_TO_UTF8( module->GetReference() );
                        pin_ref->pin_id = CONV_TO_UTF8( pad->ReturnStringPadName() );
                    }
                }
            }
        }
    }

#if 1  // do existing wires and vias

    //-----<create the wires from tracks>-----------------------------------
    {
        // export all of them for now, later we'll decide what controls we need
        // on this.
        static const KICAD_T scanTRACKs[] = { TYPETRACK, EOT };

        items.Collect( aBoard, scanTRACKs );

/*
        if( items.GetCount() )
            qsort( (void*) items.BasePtr(), items.GetCount(),
                  sizeof(TRACK*), Track_list_Sort_by_Netcode );
*/

        std::string netname;
        WIRING*     wiring = pcb->wiring;
        PATH*       path = 0;

        int old_netcode = -1;
        int old_width = -1;
        int old_layer = -1;

        for( int i=0;  i<items.GetCount();  ++i )
        {
            TRACK*  track = (TRACK*) items[i];

            if( track->GetNet() == 0 )
                continue;

            if( old_netcode != track->GetNet()
            ||  old_width   != track->m_Width
            ||  old_layer   != track->GetLayer()
            ||  (path && path->points.back() != mapPt(track->m_Start) )
              )
            {
                old_width   = track->m_Width;
                old_layer   = track->GetLayer();

                if( old_netcode != track->GetNet() )
                {
                    old_netcode = track->GetNet();
                    EQUIPOT* equipot = aBoard->FindNet( track->GetNet() );
                    wxASSERT( equipot );
                    netname = CONV_TO_UTF8( equipot->m_Netname );
                }

                WIRE* wire = new WIRE( wiring );
                wiring->wires.push_back( wire );
                wire->net_id = netname;

                wire->wire_type = T_normal;  // @todo, this should be configurable

                int kiLayer  = track->GetLayer();
                int pcbLayer = kicadLayer2pcb[kiLayer];

                path = new PATH( wire );
                wire->SetShape( path );

                path->layer_id = layerIds[pcbLayer];
                path->aperture_width = scale( old_width );

                path->AppendPoint( mapPt( track->m_Start ) );
            }

            path->AppendPoint( mapPt( track->m_End ) );
        }
    }


    //-----<export the existing real instantiated vias>---------------------
    {
        // export all of them for now, later we'll decide what controls we need
        // on this.
        static const KICAD_T scanVIAs[] = { TYPEVIA, EOT };

        items.Collect( aBoard, scanVIAs );

        for( int i=0;  i<items.GetCount();  ++i )
        {
            SEGVIA* via = (SEGVIA*) items[i];
            wxASSERT( via->Type() == TYPEVIA );

            PADSTACK* padstack = makeVia( via );
            PADSTACK* registered = pcb->library->LookupVia( padstack );
            if( padstack != registered )
            {
                delete padstack;
            }

            WIRE_VIA* dsnVia = new WIRE_VIA( pcb->wiring );
            pcb->wiring->wire_vias.push_back( dsnVia );

            dsnVia->padstack_id = registered->padstack_id;
            dsnVia->vertexes.push_back( mapPt( via->GetPosition() ) );

            int netcode = via->GetNet();
            EQUIPOT* equipot = aBoard->FindNet( netcode );
            wxASSERT( equipot );

            dsnVia->net_id = CONV_TO_UTF8( equipot->m_Netname );

            dsnVia->via_type = T_normal;     // @todo, this should be configurable
        }
    }

#endif  // do existing wires and vias

    //-----<via_descriptor>-------------------------------------------------
    {
        // Output the vias in the padstack list here, by name.  This must
        // be done after exporting existing vias as WIRE_VIAs.
        VIA*        vias = pcb->structure->via;
        PADSTACKS&  padstacks = pcb->library->padstacks;
        int         viaNdx = pcb->library->via_start_index;

        if( viaNdx != -1 )
        {
            for(  ; viaNdx < (int)padstacks.size();  ++viaNdx )
            {
                vias->AppendVia( padstacks[viaNdx].padstack_id.c_str() );
            }
        }
    }


    //-----<restore MODULEs>------------------------------------------------

    //  DSN Images (=Kicad MODULES and pads) must be presented from the
    //  top view.  Restore those that were flipped.
    for( MODULE* module = aBoard->m_Modules;  module;  module = module->Next() )
    {
        if( module->flag )
        {
            aBoard->Change_Side_Module( module, NULL );
            module->flag = 0;
        }
    }
}


}       // namespace DSN

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

using namespace DSN;


// see wxPcbStruct.h
void WinEDA_PcbFrame::ExportToSPECCTRA( wxCommandEvent& event )
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
    
    db.SetPCB( SPECCTRA_DB::MakePCB() );


    //  DSN Images (=Kicad MODULES and pads) must be presented from the
    //  top view.  So we temporarily flip any modules which are on the back
    //  side of the board to the front, and record this in the MODULE's flag field.
    for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        module->flag = 0;
        if( module->GetLayer() == COPPER_LAYER_N )
        {
            m_Pcb->Change_Side_Module( module, NULL );
            module->flag = 1;
        }
    }
    
    try 
    {    
        db.FromBOARD( m_Pcb );
        db.ExportPCB(  fullFileName, true );
    
        // if an exception is thrown by FromBOARD or ExportPCB(), then 
        // ~SPECCTRA_DB() will close the file.
    } 
    catch ( IOError ioe )
    {
        ok = false;
        
        // display no messages until we flip back the modules below.
        errorText = ioe.errorText;
    }

    //  DSN Images (=Kicad MODULES and pads) must be presented from the
    //  top view.  Restore those that were flipped.
    for( MODULE* module = m_Pcb->m_Modules;  module;  module = module->Next() )
    {
        if( module->flag )
        {
            m_Pcb->Change_Side_Module( module, NULL );
            module->flag = 0;
        }
    }
    
    if( ok )
    {
        // @todo display a message saying the export is complete.
    }
    else
        DisplayError( this, errorText );
}


namespace DSN {

struct POINT_PAIR
{
    POINT       p1;         ///< start
    POINT       p2;         ///< end
    BOARD_ITEM* item;       ///< the item which has these points, TRACK or DRAWSEGMENT
};
typedef std::vector<POINT_PAIR>     POINT_PAIRS; 


static inline void swap( POINT_PAIR& pair )
{
    POINT temp = pair.p1;
    pair.p1 = pair.p2;
    pair.p2 = temp;
}


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
    return ret;
}


/**
 * Function swapEnds
 * will swap ends of any POINT_PAIR in the POINT_PAIRS list in order to
 * make the consecutive POINT_PAIRs be "connected" at their ends.
 */
static void swapEnds( POINT_PAIRS& aList )
{
    POINT   temp;
    
    if( aList.size() <= 1 )
        return;
    
    for( unsigned i=0;  i<aList.size();  ++i )
    {
        if( aList[i].p1 == aList[i+1].p1 )
            swap( aList[i] ); 

        else if( aList[i].p1 == aList[i+1].p2 )
        {
            swap( aList[i] );
            swap( aList[i+1] );
            ++i;    // skip next one, we swapped i+1 here
        }
    }
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
                if( aList[i].p2 != aList[i+1].p1 )
                    return false;
            
            if( aList[i].p1.x != aList[i].p2.x 
            &&  aList[i].p1.y != aList[i].p2.y )
                return false;
        }
        
        return ( aList[0].p1 == aList[3].p2 );
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
 * Function makePADSTACKs
 * makes all the PADSTACKs, and marks each D_PAD with the index into the 
 * LIBRARY::padstacks list that it matches.
 */
static void makePADSTACKs( BOARD* aBoard, TYPE_COLLECTOR& aPads, 
                          LIBRARY* aLibrary, PADSTACKS& aPadstacks )
{
    if( aPads.GetCount() )
    {
#warning "uncomment next line asap"
//JPC        qsort( (void*) aPads.BasePtr(), aPads.GetCount(), sizeof(D_PAD*), Pad_list_Sort_by_Shapes );
    }

    D_PAD*  old_pad = NULL;
    int     padstackNdx = 0;
    
    for( int i=0;  i<aPads.GetCount();  ++i )
    {
        D_PAD*  pad = (D_PAD*) aPads[i];

        pad->m_logical_connexion = padstackNdx;

        if( old_pad  && 0==D_PAD::Compare( old_pad, pad ) )
        {
            continue;
        }

        old_pad = pad;

        // this is the index into the library->padstacks, be careful.
        pad->m_logical_connexion = padstackNdx++;

        PADSTACK*   padstack = new PADSTACK( aLibrary );
        SHAPE*      shape = new SHAPE( padstack );
        padstack->Append( shape );
        
        switch( pad->m_PadShape )
        {
        default:
        case PAD_CIRCLE:
            {
                CIRCLE* circle;
                double  diameter = scale(pad->m_Size.x);
                int coppers = 0;

                if( pad->IsOnLayer( COPPER_LAYER_N ) )
                {
                    circle = new CIRCLE( shape );
                    circle->SetLayerId( CONV_TO_UTF8(aBoard->GetLayerName( COPPER_LAYER_N )) );
                    circle->SetDiameter( diameter );
                    shape->Append( circle );
                    ++coppers;
                }
                
                if( pad->IsOnLayer( LAYER_CMP_N ) )
                {
                    circle = new CIRCLE( shape );
                    circle->SetLayerId( CONV_TO_UTF8(aBoard->GetLayerName( LAYER_CMP_N )) );
                    circle->SetDiameter( diameter );
                    shape->Append( circle );
                    ++coppers;
                }

                char    name[50];
                
                snprintf( name, sizeof(name),  "Round%dPad_%.6g_mil", coppers, scale(pad->m_Size.x) );
                
                name[ sizeof(name)-1 ] = 0;

                // @todo verify that all pad names are unique, there is a chance that 
                // D_PAD::Compare() could say two pads are different, yet the get the same
                // name here. If so, blend in the padNdx into the name.
                
                padstack->SetPadstackId( name );
                
                aLibrary->AddPadstack( padstack );
            }
            break;

        case PAD_RECT:
            {
                double dx = scale( pad->m_Size.x ) / 2.0;
                double dy = scale( pad->m_Size.y ) / 2.0;

                RECTANGLE* rect;
                int coppers = 0;

                if( pad->IsOnLayer( COPPER_LAYER_N ) )
                {
                    rect = new RECTANGLE( shape );
                    rect->SetLayerId( CONV_TO_UTF8(aBoard->GetLayerName( COPPER_LAYER_N )) );
                    rect->SetCorners( POINT(-dx,-dy), POINT(dx,dy) );
                    shape->Append( rect );
                    ++coppers;
                }
                
                if( pad->IsOnLayer( LAYER_CMP_N ) )
                {
                    rect = new RECTANGLE( shape );
                    rect->SetLayerId( CONV_TO_UTF8(aBoard->GetLayerName( LAYER_CMP_N )) );
                    rect->SetCorners( POINT(-dx,-dy), POINT(dx,dy) );
                    shape->Append( rect );
                    ++coppers;
                }

                char    name[50];
                
                snprintf( name, sizeof(name),  "Rect%dPad_%.6gx%.6g_mil", 
                         coppers, scale(pad->m_Size.x), scale(pad->m_Size.y)  );
                
                name[ sizeof(name)-1 ] = 0;

                // @todo verify that all pad names are unique, there is a chance that 
                // D_PAD::Compare() could say two pads are different, yet the get the same
                // name here. If so, blend in the padNdx into the name.
                
                padstack->SetPadstackId( name );
                
                aLibrary->AddPadstack( padstack );
            }
            break;
#if 0            
            pad_type = "RECTANGULAR";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            fprintf( file, "RECTANGLE %d %d %d %d\n",
                     -dx + pad->m_Offset.x, -dy - pad->m_Offset.y,
                     dx + pad->m_Offset.x, -pad->m_Offset.y + dy );
            break;
    
        case PAD_OVAL:     /* description du contour par 2 linges et 2 arcs */
        {
            pad_type = "FINGER";
            fprintf( file, " %s %d\n", pad_type, pad->m_Drill.x );
            int dr = dx - dy;
            if( dr >= 0 )       // ovale horizontal
            {
                int rayon = dy;
                fprintf( file, "LINE %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y - rayon );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         dr + pad->m_Offset.x, -pad->m_Offset.y );
    
                fprintf( file, "LINE %d %d %d %d\n",
                         dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + rayon );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -dr + pad->m_Offset.x, -pad->m_Offset.y + rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y - rayon,
                         -dr + pad->m_Offset.x, -pad->m_Offset.y );
            }
            else        // ovale vertical
            {
                dr = -dr;
                int rayon = dx;
                fprintf( file, "LINE %d %d %d %d\n",
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y + dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         pad->m_Offset.x, -pad->m_Offset.y + dr );
    
                fprintf( file, "LINE %d %d %d %d\n",
                         rayon + pad->m_Offset.x, -pad->m_Offset.y + dr,
                         rayon + pad->m_Offset.x, -pad->m_Offset.y - dr );
                fprintf( file, "ARC %d %d %d %d %d %d\n",
                         rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         -rayon + pad->m_Offset.x, -pad->m_Offset.y - dr,
                         pad->m_Offset.x, -pad->m_Offset.y - dr );
            }
            break;
        }
    
        case PAD_TRAPEZOID:
            pad_type = "POLYGON";
            break;
#endif
        }
    }
}


void SPECCTRA_DB::FromBOARD( BOARD* aBoard ) throw( IOError )
{
    TYPE_COLLECTOR          items;
    POINT_PAIRS             ppairs;
    POINT_PAIR              pair;

    if( !pcb )
        pcb = SPECCTRA_DB::MakePCB();

    //-----<unit_descriptor> & <resolution_descriptor>--------------------
    {    
        pcb->unit->units = T_mil;
        pcb->resolution->units = T_mil;
        pcb->resolution->value = 100;
    }

    
    //-----<boundary_descriptor>------------------------------------------
    {
        // get all the DRAWSEGMENTS into 'items', then look for layer == EDGE_N,
        // and those segments comprize the board's perimeter.
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
                pair.p1 = mapPt( item->m_Start );
                pair.p2 = mapPt( item->m_End );
                pair.item = item;
                ppairs.push_back( pair );
                haveEdges = true;
            }
        }
    
        if( haveEdges )
        {
            swapEnds( ppairs );
    
#if defined(DEBUG)        
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
                rect->point0 = ppairs[0].p1;
                rect->point1 = ppairs[2].p1;
                
                boundary->rectangle = rect;
            }
            else
            {
                PATH*  path = new PATH( boundary );
                
                path->layer_id = "pcb";
                for( unsigned i=0; i<ppairs.size();  ++i )
                {
                    // unless its a closed polygon, this probably won't work,
                    // otherwise it will.
                    path->points.push_back( ppairs[i].p1 );
                }
                
                boundary->paths.push_back( path );
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
            
            rect->point0 = mapPt( aBoard->m_BoundaryBox.GetOrigin() );
            rect->point1 = mapPt( bottomRight );
            
            boundary->rectangle = rect;
            
            pcb->structure->SetBOUNDARY( boundary );
        }
    }

    
    //-----<layer_descriptor>-----------------------------------------------
    {
        // specctra wants top physical layer first, then going down to the 
        // bottom most physical layer in physical sequence.
        // @question : why does Kicad not display layers in that order?
        int layerCount = aBoard->GetCopperLayerCount();
        
        for( int ndx=layerCount-1;  ndx >= 0;  --ndx )
        {
            wxString    layerName = aBoard->GetLayerName( ndx>0 && ndx==layerCount-1 ? LAYER_CMP_N : ndx );
            LAYER*      layer = new LAYER( pcb->structure );
            
            layer->name = CONV_TO_UTF8( layerName );
            
            // layer->type =
            pcb->structure->layers.push_back( layer );
        }
    }

    
    //-----<zone containers become planes>--------------------------------------------
    {
        static const KICAD_T  scanZONEs[] = { TYPEZONE_CONTAINER, EOT };
        items.Collect( aBoard, scanZONEs );

        for( int i=0;  i<items.GetCount();  ++i )
        {
            ZONE_CONTAINER* item = (ZONE_CONTAINER*) items[i];

            wxString        layerName = aBoard->GetLayerName( item->GetLayer() );
            COPPER_PLANE*   plane = new COPPER_PLANE( pcb->structure );
            PATH*           polygon = new PATH( plane, T_polygon );

            plane->path = polygon;
            plane->name = CONV_TO_UTF8( item->m_Netname );
            
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

    
    //-----<build the padstack list here, no output>------------------------
    {
        static const KICAD_T scanPADs[] = { TYPEPAD, EOT };

        TYPE_COLLECTOR  pads;

        // get all the D_PADs into pads.        
        pads.Collect( aBoard, scanPADs );

        makePADSTACKs( aBoard, pads, pcb->library, pcb->library->padstacks );

#if defined(DEBUG)
        for( int p=0;  p<pads.GetCount();  ++p )
            pads[p]->Show( 0, std::cout );
#endif    
        
/*        
        static const KICAD_T scanMODULEs[] = { TYPEMODULE, EOT };
        
        items.Collect( aBoard, scanMODULEs );

        for( int m=0;  m<items.GetCount();  ++m )
        {
            MODULE* module = (MODULE*) items[m];
            

            // collate all the pads, and make a component.
            for( int p=0;  p<pads.GetCount();  ++p )
            {
                D_PAD* pad = (D_PAD*) pads[p];
                
                D(pad->Show( 0, std::cout );)
                
                // lookup and maybe add this pad to the padstack.
                wxString padName = lookupPad( pcb->library->padstacks, pad ); 
            }
        }
*/        
    }
    
    //-----<via_descriptor>-------------------------------------------------
    {
        // Output the vias in the padstack list here, by name
    }

    
}

    
}       // namespace DSN


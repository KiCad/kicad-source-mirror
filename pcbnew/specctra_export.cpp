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
    
    db.SetPCB( SPECCTRA_DB::MakePCB() );
    
    try 
    {    
        db.FromBOARD( m_Pcb );
        db.ExportPCB(  fullFileName, true );
    } 
    catch ( IOError ioe )
    {
        DisplayError( this, ioe.errorText );
    }
    
    // if an exception is thrown by FromBOARD or Export(), then 
    // ~SPECCTRA_DB() will close the file.
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


static POINT mapPt( const wxPoint& pt )
{
    POINT ret;
    ret.x = pt.x;
    ret.y = -pt.y;      // make y negative, since it is increasing going down.
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
 * tests to see if the POINT_PAIRS list make up a vertically/horizontally 
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


void SPECCTRA_DB::exportEdges( BOARD* aBoard ) throw( IOError )
{
}


void SPECCTRA_DB::FromBOARD( BOARD* aBoard ) throw( IOError )
{
    TYPE_COLLECTOR          items;
    POINT_PAIRS             ppairs;
    POINT_PAIR              pair;

    if( !pcb )
        pcb = SPECCTRA_DB::MakePCB();

    //-----<header stuff>-------------------------------------------------    
    pcb->unit->units = T_mil;
    pcb->resolution->units = T_mil;
    pcb->resolution->value = 10;

    //-----<board edges>--------------------------------------------------
 
    // get all the DRAWSEGMENTS into 'items', then look for layer == EDGE_N,
    // and those segments comprize the board's perimeter.
    const KICAD_T  scanDRAWSEGMENTS[] = { TYPEDRAWSEGMENT, EOT };
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
    
    //-----<layers>-------------------------------------------------------
    
    
    
}

    
}       // namespace DSN


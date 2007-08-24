/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2007 Dick Hollenbeck, dick@softplc.com
 * Copyright (C) 2004-2007 Kicad Developers, see change_log.txt for contributors.
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

#if defined(DEBUG)

#include "collectors.h"  
#include "pcbnew.h"             // class BOARD


/*  This module contains out of line member functions for classes given in 
    collectors.h.  Those classes augment the functionality of class WinEDA_PcbFrame.
*/


// see collectors.h
const KICAD_T GENERALCOLLECTOR::AllBoardItems[] = {
    TYPETEXTE, 
    TYPEDRAWSEGMENT, 
    TYPECOTATION,
    TYPEVIA,
    TYPETRACK,
    TYPEPAD,
    TYPETEXTEMODULE,
    TYPEMODULE,
    EOT
};    


/**
 * Function Inspect
 * is the examining function within the INSPECTOR which is passed to the 
 * Iterate function.  Searches and collects all the objects that the old 
 * function PcbGeneralLocateAndDisplay() would find, except that it keeps all
 * that it finds and does not do any displaying. 
 *
 * @param testItem An EDA_BaseStruct to examine.
 * @param notUsed The const void* testData.
 * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
 *   else SCAN_CONTINUE;
 */ 
SEARCH_RESULT GENERALCOLLECTOR::Inspect( EDA_BaseStruct* testItem, const void* notUsed )
{
    BOARD_ITEM* item = (BOARD_ITEM*) testItem;

#if 1   // debugging
    static int breakhere = 0;
    switch( item->m_StructType )
    {
    case TYPEPAD:
            breakhere++;
            break;
    case TYPEVIA:
            breakhere++;
            break;
    case TYPETRACK:
            breakhere++;
            break;
    case TYPETEXTE: 
            breakhere++;
            break;
    case TYPEDRAWSEGMENT: 
            breakhere++;
            break;
    case TYPECOTATION:
            breakhere++;
            break;
    case TYPETEXTEMODULE:
            TEXTE_MODULE* tm;
            tm = (TEXTE_MODULE*) item;
            if( tm->m_Text == wxT("U5") )
            {
                breakhere++;
            }
            break;
    case TYPEMODULE:
            breakhere++;
            break;
    default:
            breakhere++;
            break;
    }
#endif
    
    switch( item->m_StructType )
    {
    case TYPEPAD:
    case TYPEVIA:
    case TYPETRACK:
    case TYPETEXTE: 
    case TYPEDRAWSEGMENT: 
    case TYPECOTATION:
    case TYPETEXTEMODULE:
    case TYPEMODULE:
        
        // The primary search criteria:
        if( item->IsOnLayer( m_PreferredLayer ) )
        {
            if( item->HitTest( m_RefPos ) )
            {
                if( !item->IsLocked() )
                    Append( item );
                else
                    Append2nd( item );      // 2nd if locked.
            }
        }
        
        // The secondary search criteria
        else if( item->IsOnOneOfTheseLayers( m_LayerMask ) )
        {
            if( item->HitTest( m_RefPos ) )
                Append2nd( item );
        }
        break;
        
    default:
        printf("OOPS, not expecting class type %d\n", item->m_StructType );
    }

    return SEARCH_CONTINUE;
}


// see collectors.h 
void GENERALCOLLECTOR::Scan( BOARD* board, const wxPoint& refPos, 
                          int aPreferredLayer, int aLayerMask )
{
    Empty();        // empty the collection, primary criteria list
    Empty2nd();     // empty the collection, secondary criteria list
    
    SetPreferredLayer( aPreferredLayer );
    SetLayerMask( aLayerMask );
    
    /*  remember where the snapshot was taken from and pass refPos to
        the Inspect() function.
    */        
    SetRefPos( refPos );

#if defined(DEBUG)
    std::cout << '\n';
#endif
    
    // visit the board with the INSPECTOR (me).
    board->Visit(   this,       // INSPECTOR* inspector
                    NULL,       // const void* testData, not used here 
                    m_ScanTypes);
    
    SetTimeNow();               // when snapshot was taken
    
    // append 2nd list onto end of the first "list" 
    for( unsigned i=0;  i<list2nd.size();  ++i )
        Append( list2nd[i] );
    
    Empty2nd();
}


#endif  // DEBUG

//EOF

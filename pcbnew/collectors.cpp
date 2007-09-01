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
const KICAD_T GENERAL_COLLECTOR::AllBoardItems[] = {
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
SEARCH_RESULT GENERAL_COLLECTOR::Inspect( EDA_BaseStruct* testItem, const void* notUsed )
{
    BOARD_ITEM* item = (BOARD_ITEM*) testItem;

#if 1   // debugging
    static int breakhere = 0;
    switch( item->Type() )
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
            if( tm->m_Text == wxT("10uH") )
            {
                breakhere++;
            }
            break;
    case TYPEMODULE:
            MODULE* m;
            m = (MODULE*) item;
            if( m->GetReference() == wxT("L1") )
            {
                breakhere++;
            }
            break;
    default:
            breakhere++;
            break;
    }
#endif
    

#if 1

/*
    int     m_PreferredLayer;             x
    bool    m_IgnorePreferredLayer    
    int     m_LayerVisible;               x
    bool    m_IgnoreNonVisibleLayers;
    
    int     m_LayerLocked;                x
    bool    m_IgnoreLockedLayers;
    
    bool    m_IgnoreLockedItems;          x
    
    bool    m_IncludeSecondary;
*/    
   
    if( item->IsOnLayer( m_Guide->GetPreferredLayer() ) || m_Guide->IgnorePreferredLayer() )
    {
        int layer = item->GetLayer();
        
        if(  m_Guide->IsLayerVisible( layer ) || !m_Guide->IgnoreNonVisibleLayers() )
        {
            if( !m_Guide->IsLayerLocked(layer) || !m_Guide->IgnoreLockedLayers() )
            {
                if( !item->IsLocked() || !m_Guide->IgnoreLockedItems() )
                {
                    if( item->HitTest( m_RefPos ) )
                    {
                        Append( item );
                        goto exit;
                    }
                }
            }
        }
    }

    if( m_Guide->IncludeSecondary() )
    {
        // for now, "secondary" means "tolerate any layer".  It has
        // no effect on other criteria, since there is a separate "ignore" control for 
        // those in the COLLECTORS_GUIDE
        
        int layer = item->GetLayer();
        
        if(  m_Guide->IsLayerVisible( layer ) || !m_Guide->IgnoreNonVisibleLayers() )
        {
            if( !m_Guide->IsLayerLocked(layer) || !m_Guide->IgnoreLockedLayers() )
            {
                if( !item->IsLocked() || !m_Guide->IgnoreLockedItems() )
                {
                    if( item->HitTest( m_RefPos ) )
                    {
                        Append2nd( item );
                        goto exit;
                    }
                }
            }
        }
    }
    
#else
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
#endif        

exit:
    return SEARCH_CONTINUE;     // always when collecting
}


// see collectors.h
/*
void GENERAL_COLLECTOR::Collect( BOARD* board, const wxPoint& refPos, 
                          int aPreferredLayer, int aLayerMask )
{
    Empty();        // empty the collection, primary criteria list
    Empty2nd();     // empty the collection, secondary criteria list
    
    SetPreferredLayer( aPreferredLayer );
    SetLayerMask( aLayerMask );
    
    //  remember refPos, pass to Inspect()
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
*/


// see collectors.h 
void GENERAL_COLLECTOR::Collect( BOARD* board, const wxPoint& refPos, 
                            const COLLECTORS_GUIDE* guide )
{
    Empty();        // empty the collection, primary criteria list
    Empty2nd();     // empty the collection, secondary criteria list

    // remember guide, pass it to Inspect()
    SetGuide( guide );
    
    // remember where the snapshot was taken from and pass refPos to
    // the Inspect() function.
    SetRefPos( refPos );

    // visit the board with the INSPECTOR (me).
    board->Visit(   this,       // INSPECTOR* inspector
                    NULL,       // const void* testData, not used here 
                    m_ScanTypes);
    
    SetTimeNow();               // when snapshot was taken
    
    // append 2nd list onto end of the first list 
    for( unsigned i=0;  i<m_List2nd.size();  ++i )
        Append( m_List2nd[i] );
    
    Empty2nd();
}


/** is still inline
 * Constructor GENERAL_COLLECTORS_GUIDE
 * grabs stuff from global preferences and uses reasonable defaults.
 * Add more constructors as needed.
GENERAL_COLLECTORS_GUIDE::GENERAL_COLLECTORS_GUIDE()
{
    
    m_LayerLocked;    
    m_LayerVisible;       
    m_IgnoreLockedLayers;
    m_IgnoreNonVisibleLayers;
    m_PreferredLayer;    
    m_IgnoreLockedItems;    
    m_IncludeSecondary;
}
 */


#endif  // DEBUG

//EOF

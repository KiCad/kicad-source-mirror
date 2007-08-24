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

#ifndef COLLECTORS_H
#define COLLECTORS_H


/*  This module contains a number of COLLECTOR implementations which are used
    to augment the functionality of class WinEDA_PcbFrame.
*/


#include "class_collector.h"

/**
 * Class GENERALCOLLECTOR
 * is intended for use when the right click button is pressed, or when the 
 * plain "arrow" tool is in effect.  This class can be used by window classes
 * such as WinEDA_PcbFrame.
 *
 * Philosophy: this class knows nothing of the context in which a BOARD is used
 * and that means it knows nothing about which layers are visible or current, 
 * but can handle those concerns by the SetPreferredLayer() function and the
 * SetLayerMask() fuction.
 */
class GENERALCOLLECTOR : public COLLECTOR
{
    /**
     * A place to hold collected objects which don't match precisely the search 
     * criteria, but would be acceptable if nothing else is found. 
     * "2nd" choice, which will be appended to the end of COLLECTOR's prime 
     * "list" at the end of the search.
     */
    std::vector<BOARD_ITEM*>    list2nd;
    

    /**
     * A bit-mapped layer mask that defines any layers which are acceptable 
     * on a secondary search criterion basis.
     */
    int             m_LayerMask;
    
    
public:

    /// A scan list for all editable board items, like PcbGeneralLocateAndDisplay()
    static const KICAD_T AllBoardItems[];


    /**
     * Constructor GENERALCOLLECTOR
     */ 
    GENERALCOLLECTOR()
    {
        m_LayerMask = 0;
        SetScanTypes( AllBoardItems );
    }

    void Empty2nd()
    {
        list2nd.clear();
    }

    void Append2nd( BOARD_ITEM* item )
    {
        list2nd.push_back( item );
    }

    
    /**
     * Function SetLayerMask
     * takes a bit-mapped layer mask and records it.  During the scan/search,
     * this is used as a secondary search criterion.  That is, if there is no direct 
     * layer match with COLLECTOR::m_PreferredLayer (the primary criterion), 
     * then an object on any layer given in this bit-map is recorded as a 
     * second choice object if it also HitTest()s true.
     *
     * @param aLayerMask A layer mask which has bits in it indicating which
     *  layers are acceptable.  Caller must pay attention to which layers are
     *  visible, selected, etc.  All those concerns are handled outside this
     *  class, as stated in the class Philosophy above.
     */
    void SetLayerMask( int aLayerMask )
    {
        m_LayerMask = aLayerMask;
    }

    
    /**
     * Function operator[int]
     * overloads COLLECTOR::operator[](int) to return a BOARD_ITEM* instead of
     * an EDA_BaseStruct* type.
     * @param ndx The index into the list.
     * @return BOARD_ITEM* - or something derived from it, or NULL.
     */
    BOARD_ITEM* operator[]( int ndx ) const
    {
        if( (unsigned)ndx < (unsigned)GetCount() )
            return (BOARD_ITEM*) list[ ndx ];
        return NULL;
    }

    
    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the 
     * Iterate function.
     *
     * @param testItem An EDA_BaseStruct to examine.
     * @param testData is not used in this class.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     */ 
    SEARCH_RESULT Inspect( EDA_BaseStruct* testItem, const void* testData );
    
    
    /**
     * Function Scan
     * scans a BOARD using this class's Inspector method, which does the collection.
     * @param board A BOARD to scan.
     * @param refPos A wxPoint to use in hit-testing.
     * @param aPreferredLayer The layer meeting the primary search criterion.
     * @param aLayerMask The layers, in bit-mapped form, meeting the secondary search criterion.
     */
    void Scan( BOARD* board, const wxPoint& refPos, int aPreferredLayer, int aLayerMask );
};


#endif // COLLECTORS_H

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

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <vector>
//#include <cstdlib>          // abs()

#include "fctsys.h"
#include "base_struct.h"    // SEARCH_RESULT
#include "common.h"         // GetTimeStamp()


class EDA_BaseStruct;


/**
 * Class COLLECTOR
 * is an abstract class that will find and hold all the objects according to 
 * an inspection done by the Inspect() function which must be implemented by
 * any derived class.  When Inspect() finds an object that it wants to collect,
 * i.e. one that it "likes", then it only has to do an Append( testItem ) 
 * on it to add it to its collection, but in all cases for the scan to continue,
 * Inspect() must return SEARCH_CONTINUE.
 *
 * Later, after collection, the user can iterate through all the objects 
 * in the remembered collection using GetCount() and the [int] operator.
 */
class COLLECTOR : public INSPECTOR
{
protected:
    /// Which object types to scan
    const KICAD_T*  m_ScanTypes;

    /// The layer that is desired as a primary search criterion
    int             m_PreferredLayer;
    
    /// A place to hold collected objects without taking ownership of their memory.
    std::vector<EDA_BaseStruct*>    list;

    /// The point at which the snapshot was taken.
    wxPoint         m_RefPos;

    /// The time at which the collection was made.
    int             m_TimeAtCollection;
    
    
public:

    COLLECTOR()
    {
        m_PreferredLayer = 0;
        m_ScanTypes      = 0;
    }

    virtual ~COLLECTOR()
    {
    }

    
    void SetPreferredLayer( int aPreferredLayer )
    {
        m_PreferredLayer = aPreferredLayer;
    }
    
    
    /**
     * Function GetCount
     * returns the number of objects in the list
     */
    unsigned GetCount() const
    {
        return list.size();
    }

    
    /**
     * Function Empty
     * sets the list to empty
     */
    void Empty()
    {
        list.clear();
    }

    
    /**
     * Function Append
     * adds an item to the end of the list.
     * @param item An EDA_BaseStruct* to add.
     */
    void Append( EDA_BaseStruct* item )
    {
        list.push_back( item );
    }


    /**
     * Function operator[int]
     * is used for read only access and returns the object at index ndx.
     * @param ndx The index into the list.
     * @return EDA_BaseStruct* - or something derived from it, or NULL.
     */
    EDA_BaseStruct* operator[]( int ndx ) const
    {
        if( (unsigned)ndx < GetCount() )
            return list[ ndx ];
        return NULL;
    }

    void SetScanTypes( const KICAD_T* scanTypes )
    {
        m_ScanTypes = scanTypes;
    }
    
    wxPoint GetRefPos() const  {  return m_RefPos; }
    
    void SetRefPos( const wxPoint& arefPos )
    {
        m_RefPos = arefPos;
    }

    void SetTimeNow()
    {
        m_TimeAtCollection = GetTimeStamp();
    }
    int GetTime()
    {
        return m_TimeAtCollection;
    }

    
    /**
     * Function IsSimilarPointAndTime
     * returns true if the given reference point is "similar" (defined here) 
     * to the internal reference point and the current time is within a few 
     * seconds of the internal m_TimeAtCollection.
     *
     * @param aRefPos A wxPoint to compare to.
     * @return bool - true if the point and time are similar, else false.
     */
    bool IsSimilarPointAndTime( const wxPoint& aRefPos )
    {
        const int distMax = 2;      // adjust these here
        const int timeMax = 3;      // seconds, I think
        
        int dx = abs( aRefPos.x - m_RefPos.x );
        int dy = abs( aRefPos.y - m_RefPos.y );
        
        if( dx <= distMax && dy <= distMax 
                && GetTimeStamp()-m_TimeAtCollection <= timeMax )
            return true;
        else
            return false;
    }

    
    /**
     * Function Inspect
     * is the examining function within the INSPECTOR which is passed to the 
     * Iterate function.  It is used primarily for searching, but not limited to
     * that.  It can also collect or modify the scanned objects.
     *
     * @param testItem An EDA_BaseStruct to examine.
     * @param testData is arbitrary data needed by the inspector to determine
     *   if the EDA_BaseStruct under test meets its match criteria.
     * @return SEARCH_RESULT - SEARCH_QUIT if the Iterator is to stop the scan,
     *   else SCAN_CONTINUE;
     *
     * implement in derived class:
    SEARCH_RESULT virtual Inspect( EDA_BaseStruct* testItem, 
        const void* testData ) = 0;
     */ 
    

    /**
     * Function Scan
     * scans a BOARD using this class's Inspector method, which does the collection.
     * @param board A BOARD to scan.
     * @param refPos A wxPoint to use in hit-testing.
     *
     * example implementation, in derived class:
     *
    virtual void Scan( BOARD* board, const wxPoint& refPos )
    {
        example implementation:
        
        SetRefPos( refPos );        // remember where the snapshot was taken from
        
        Empty();        // empty the collection
        
        // visit the board with the INSPECTOR (me).
        board->Visit(   this,       // INSPECTOR* inspector
                        NULL,       // const void* testData, 
                        m_ScanTypes);
        SetTimeNow();               // when it was taken
    }
    */
};

#endif  // COLLECTOR_H


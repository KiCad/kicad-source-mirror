/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2004-2007 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file class_collector.h
 * @brief COLLECTOR class definition.
 */

#ifndef COLLECTOR_H
#define COLLECTOR_H

#include <vector>

#include <fctsys.h>
#include <base_struct.h>    // SEARCH_RESULT
#include <common.h>         // GetNewTimeStamp()


class EDA_ITEM;


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
    const KICAD_T* m_ScanTypes;

    /// A place to hold collected objects without taking ownership of their memory.
    std::vector<EDA_ITEM*> m_List;

    /// A point to test against, and that was used to make the collection.
    wxPoint m_RefPos;

    /// A bounding box to test against, and that was used to make the collection.
    EDA_RECT m_RefBox;

    /// The time at which the collection was made.
    time_t m_TimeAtCollection;

public:
    COLLECTOR()
    {
        m_ScanTypes = 0;
    }

    virtual ~COLLECTOR() {}

    /**
     * Function IsValidIndex
     * tests if \a aIndex is with the limits of the list of collected items.
     *
     * @param aIndex The index to test.
     * @return True if \a aIndex is with the limits of the list of collected items,
     *         otherwise false.
     */
    bool IsValidIndex( int aIndex )
    {
        return ( (unsigned) aIndex < m_List.size() );
    }

    /**
     * Function GetCount
     * returns the number of objects in the list
     */
    int GetCount() const
    {
        return (int) m_List.size();
    }

    /**
     * Function Empty
     * sets the list to empty
     */
    void Empty()
    {
        m_List.clear();
    }

    /**
     * Function Append
     * adds an item to the end of the list.
     * @param item An EDA_ITEM* to add.
     */
    void Append( EDA_ITEM* item )
    {
        m_List.push_back( item );
    }

    /**
     * Function Remove
     * removes the item at \a aIndex (first position is 0);
     * @param aIndex The index into the list.
     */
    void Remove( int aIndex )
    {
        m_List.erase( m_List.begin() + aIndex );
    }

    /**
     * Function operator[int]
     * is used for read only access and returns the object at \a aIndex.
     * @param aIndex The index into the list.
     * @return EDA_ITEM* - or something derived from it, or NULL.
     */
    EDA_ITEM* operator[]( int aIndex ) const
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )  // (unsigned) excludes aIndex<0 also
            return m_List[ aIndex ];

        return NULL;
    }

    /**
     * Function BasePtr
     * returns the address of the first element in the array.  Only call this
     * if there is at least one element in the vector m_List, otherwise a
     * C++ exception should get thrown.
     */
    EDA_ITEM* const* BasePtr() const
    {
        return &m_List[0];
    }

    /**
     * Function HasItem
     * tests if \a aItem has already been collected.
     *
     * @param aItem The EDA_ITEM* to be tested.
     * @return True if \a aItem is already collected.
     */
    bool HasItem( const EDA_ITEM* aItem ) const
    {
        for( size_t i = 0;  i < m_List.size();  i++ )
        {
            if( m_List[i] == aItem )
                return true;
        }

        return false;
    }

    /**
     * Function SetScanTypes
     * records the list of KICAD_T types to consider for collection by
     * the Inspect() function.
     * @param scanTypes An array of KICAD_T, terminated by EOT.  No copy is
     *                  is made of this array (so cannot come from caller's stack).
     */
    void SetScanTypes( const KICAD_T* scanTypes )
    {
        m_ScanTypes = scanTypes;
    }

    void SetTimeNow()
    {
        m_TimeAtCollection = GetNewTimeStamp();
    }

    time_t GetTime()
    {
        return m_TimeAtCollection;
    }

    void SetRefPos( const wxPoint& aRefPos )  {  m_RefPos = aRefPos; }
    const wxPoint& GetRefPos() const  {  return m_RefPos; }

    void SetBoundingBox( const EDA_RECT& aRefBox ) { m_RefBox = aRefBox;  }
    const EDA_RECT& GetBoundingBox() const {  return m_RefBox; }

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
        const time_t timeMax = 3;   // seconds

        int dx = abs( aRefPos.x - m_RefPos.x );
        int dy = abs( aRefPos.y - m_RefPos.y );

        if( dx <= distMax && dy <= distMax &&
            (int)GetNewTimeStamp() - m_TimeAtCollection <= timeMax )
            return true;
        else
            return false;
    }

    /**
     * Function Collect
     * scans an EDA_ITEM using this class's Inspector method, which does
     * the collection.
     * @param container An EDA_ITEM to scan, including those items it contains.
     * @param aRefPos A wxPoint to use in hit-testing.
     *
     * example implementation, in derived class:
     *
    void Collect( EDA_ITEM* container, const wxPoint& aRefPos )
    {
        example implementation:

        SetRefPos( aRefPos );    // remember where the snapshot was taken from

        Empty();        // empty the collection

        // visit the board with the INSPECTOR (me).
        container->Visit(   this,       // INSPECTOR* inspector
                            NULL,       // const void* testData,
                            m_ScanTypes);
        SetTimeNow();                   // when it was taken
    }
    */
};

#endif  // COLLECTOR_H

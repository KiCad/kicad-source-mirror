/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2008 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <core/kicad_algo.h>
#include <algorithm>
#include <eda_item.h>    // SEARCH_RESULT


class EDA_ITEM;


/**
 * An abstract class that will find and hold all the objects according to
 * an inspection done by the Inspect() function which must be implemented by
 * any derived class.
 *
 * When Inspect() finds an object that it wants to collect, i.e. one that it "likes", then
 * it only has to do an Append( testItem )on it to add it to its collection, but in all cases
 * for the scan to continue, Inspect() must return SEARCH_CONTINUE.  Later, after collection,
 * the user can iterate through all the objects in the remembered collection using GetCount()
 * and the [int] operator.
 */
class COLLECTOR
{
public:
    COLLECTOR() :
            m_Threshold( 0 ),
            m_MenuCancelled( false ),
            m_scanTypes( {} )
    {
        // Inspect() is virtual so calling it from a class common inspector preserves
        // polymorphism.
        m_inspector =
                [this]( EDA_ITEM* aItem, void* aTestData )
                {
                    return this->Inspect( aItem, aTestData );
                };
    }

    virtual ~COLLECTOR() {}

    virtual INSPECT_RESULT Inspect( EDA_ITEM* aTestItem, void* aTestData )
    {
        return INSPECT_RESULT::QUIT;
    };

    using ITER = std::vector<EDA_ITEM*>::iterator;
    using CITER = std::vector<EDA_ITEM*>::const_iterator;

    ITER begin() { return m_list.begin(); }
    ITER end() { return m_list.end(); }
    CITER begin() const { return m_list.cbegin(); }
    CITER end() const { return m_list.cend(); }

    /**
     * Return the number of objects in the list.
     */
    int GetCount() const
    {
        return (int) m_list.size();
    }

    /**
     * Clear the list.
     */
    void Empty()
    {
        m_list.clear();
    }

    /**
     * Add an item to the end of the list.
     *
     * @param item An EDA_ITEM* to add.
     */
    void Append( EDA_ITEM* item )
    {
        m_list.push_back( item );
    }

    /**
     * Remove the item at \a aIndex (first position is 0).
     *
     * @param aIndex The index into the list.
     */
    void Remove( int aIndex )
    {
        m_list.erase( m_list.begin() + aIndex );
    }

    /**
     * Remove the item aItem (if exists in the collector).
     *
     * @param aItem the item to be removed.
     */
    void Remove( const EDA_ITEM* aItem )
    {
        std::erase_if( m_list, [&aItem]( const EDA_ITEM* aCandidate )
                                {
                                    return aCandidate == aItem;
                                } );
    }

    /**
     * Test if the collector has heuristic backup items.
     *
     * @return true if Combine() can run to bring secondary items into the list.
     */
    bool HasAdditionalItems()
    {
        return !m_backupList.empty();
    }

    /**
     * Re-combine the backup list into the main list of the collector.
     */
    void Combine()
    {
        std::copy( m_backupList.begin(), m_backupList.end(), std::back_inserter( m_list ) );
        m_backupList.clear();
    }

    /**
     * Move the item at \a aIndex (first position is 0) to the backup list.
     *
     * @param aIndex The index into the list.
     */
    void Transfer( int aIndex )
    {
        m_backupList.push_back( m_list[aIndex] );
        m_list.erase( m_list.begin() + aIndex );
    }

    /**
     * Move \a aItem (if exists in the collector) to the backup list.
     *
     * @param aItem the item to be moved.
     */
    void Transfer( EDA_ITEM* aItem )
    {
        for( size_t i = 0; i < m_list.size(); i++ )
        {
            if( m_list[i] == aItem )
            {
                m_list.erase( m_list.begin() + i );
                m_backupList.push_back( aItem );
                return;
            }
        }
    }

    /**
     * Used for read only access and returns the object at \a aIndex.
     *
     * @param aIndex The index into the list.
     * @return the object at \a aIndex something derived from it or NULL.
     */
    virtual EDA_ITEM* operator[]( int aIndex ) const
    {
        if( (unsigned)aIndex < (unsigned)GetCount() )  // (unsigned) excludes aIndex<0 also
            return m_list[ aIndex ];

        return nullptr;
    }

    /**
     * Tests if \a aItem has already been collected.
     *
     * @param aItem The EDA_ITEM* to be tested.
     * @return True if \a aItem is already collected.
     */
    bool HasItem( const EDA_ITEM* aItem ) const
    {
        for( size_t i = 0; i < m_list.size(); i++ )
        {
            if( m_list[i] == aItem )
                return true;
        }

        return false;
    }

    /**
     * Record the list of #KICAD_T types to consider for collection by the Inspect() function.
     *
     * @param aScanTypes A list of KICAD_Ts.
     */
    void SetScanTypes( const std::vector<KICAD_T>& aTypes ) { m_scanTypes = aTypes; }

    void SetRefPos( const VECTOR2I& aRefPos ) { m_refPos = aRefPos; }

    /**
     * Count the number of items matching \a aType.
     *
     * @param aType type we are interested in.
     * @return number of occurrences.
     */
    int CountType( KICAD_T aType )
    {
        int cnt = 0;

        for( size_t i = 0; i < m_list.size(); i++ )
        {
            if( m_list[i]->Type() == aType )
                cnt++;
        }

        return cnt;
    }

    int            m_Threshold;          // Hit-test threshold in internal units.

    wxString       m_MenuTitle;          // The title of selection disambiguation menu (if needed)
    bool           m_MenuCancelled;      // Indicates selection disambiguation menu was canceled

protected:
    std::vector<EDA_ITEM*> m_list;       // Primary list of most likely items
    std::vector<EDA_ITEM*> m_backupList; // Secondary list with items removed by heuristics

    std::vector<KICAD_T>   m_scanTypes;
    INSPECTOR_FUNC         m_inspector;

    VECTOR2I               m_refPos;     // Reference pos used to generate the collection.
};

#endif  // COLLECTOR_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2016-2017 CERN
 * Copyright (C) 2020-2021 KiCad Developers, see change_log.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __COMMIT_H
#define __COMMIT_H

#include <set>
#include <vector>
#include <wx/string.h>
#include <undo_redo_container.h>
#include <kiid.h>

class EDA_ITEM;
class BASE_SCREEN;

///< Types of changes
enum CHANGE_TYPE {
    CHT_ADD     = 1,
    CHT_REMOVE  = 2,
    CHT_MODIFY  = 4,
    CHT_TYPE    = CHT_ADD | CHT_REMOVE | CHT_MODIFY,

    CHT_DONE    = 8,             ///< Flag to indicate the change is already applied
    CHT_FLAGS   = CHT_DONE
};

template<typename T>
CHANGE_TYPE operator|( CHANGE_TYPE aTypeA, T aTypeB )
{
    return CHANGE_TYPE( (int) aTypeA | (int) aTypeB );
}

template<typename T>
CHANGE_TYPE operator&( CHANGE_TYPE aTypeA, T aTypeB )
{
    return CHANGE_TYPE( (int) aTypeA & (int) aTypeB );
}


/**
 * Represent a set of changes (additions, deletions or modifications) of a data model
 * (e.g. the BOARD) class.
 *
 * The class can be used to propagate changes to subscribed objects (e.g. views, ratsnest),
 * and automatically create undo/redo points.
 */
class COMMIT
{
public:
    COMMIT();
    virtual ~COMMIT();

    ///< Add a new item to the model
    COMMIT& Add( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr )
    {
        return Stage( aItem, CHT_ADD, aScreen );
    }

    ///< Notify observers that aItem has been added
    COMMIT& Added( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr )
    {
        return Stage( aItem, CHT_ADD | CHT_DONE, aScreen );
    }

    ///< Remove a new item from the model
    COMMIT& Remove( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr )
    {
        return Stage( aItem, CHT_REMOVE, aScreen );
    }

    ///< Notify observers that aItem has been removed
    COMMIT& Removed( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr )
    {
        return Stage( aItem, CHT_REMOVE | CHT_DONE, aScreen );
    }

    ///< Modify a given item in the model.
    ///< Must be called before modification is performed.
    COMMIT& Modify( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr )
    {
        return Stage( aItem, CHT_MODIFY, aScreen );
    }

    ///< Create an undo entry for an item that has been already modified. Requires a copy done
    ///< before the modification.
    COMMIT& Modified( EDA_ITEM* aItem, EDA_ITEM* aCopy, BASE_SCREEN *aScreen = nullptr )
    {
        return createModified( aItem, aCopy );
    }

    template<class Range>

    COMMIT& StageItems( const Range& aRange, CHANGE_TYPE aChangeType )
    {
        for( const auto& item : aRange )
            Stage( item, aChangeType );

        return *this;
    }

    ///< Add a change of the item aItem of type aChangeType to the change list.
    virtual COMMIT& Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType,
                           BASE_SCREEN *aScreen = nullptr );

    virtual COMMIT& Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType,
                           BASE_SCREEN *aScreen = nullptr );

    virtual COMMIT& Stage( const PICKED_ITEMS_LIST& aItems,
                           UNDO_REDO aModFlag = UNDO_REDO::UNSPECIFIED,
                           BASE_SCREEN *aScreen = nullptr );

    ///< Execute the changes.
    virtual void Push( const wxString& aMessage = wxT( "A commit" ), int aFlags = 0 ) = 0;

    ///< Revert the commit by restoring the modified items state.
    virtual void Revert() = 0;

    bool Empty() const
    {
        return m_changes.empty();
    }

    ///< Returns status of an item.
    int GetStatus( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr );

protected:
    struct COMMIT_LINE
    {
        EDA_ITEM*    m_item;                ///< Main item that is added/deleted/modified
        EDA_ITEM*    m_copy;                ///< Optional copy of the item
        CHANGE_TYPE  m_type;                ///< Modification type
        KIID         m_parent = NilUuid();  ///< Parent item (primarily for undo of deleted items)
        BASE_SCREEN* m_screen;
    };

    // Should be called in Push() & Revert() methods
    void clear()
    {
        m_changedItems.clear();
        m_changes.clear();
    }

    COMMIT& createModified( EDA_ITEM* aItem, EDA_ITEM* aCopy, int aExtraFlags = 0,
                            BASE_SCREEN *aScreen = nullptr );

    virtual void makeEntry( EDA_ITEM* aItem, CHANGE_TYPE aType, EDA_ITEM* aCopy = nullptr,
                            BASE_SCREEN *aScreen = nullptr );

    /**
     * Search for an entry describing change for a particular item.
     *
     * @return null if there is no related entry.
     */
    COMMIT_LINE* findEntry( EDA_ITEM* aItem, BASE_SCREEN *aScreen = nullptr );

    virtual EDA_ITEM* parentObject( EDA_ITEM* aItem ) const = 0;

    virtual EDA_ITEM* makeImage( EDA_ITEM* aItem ) const = 0;

    CHANGE_TYPE convert( UNDO_REDO aType ) const;
    UNDO_REDO convert( CHANGE_TYPE aType ) const;

protected:
    std::set<EDA_ITEM*>      m_changedItems;
    std::vector<COMMIT_LINE> m_changes;
};

#endif

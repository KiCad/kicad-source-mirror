/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#include <class_undoredo_container.h>

class EDA_ITEM;

///> Types of changes
enum CHANGE_TYPE {
    CHT_ADD     = 1,
    CHT_REMOVE  = 2,
    CHT_MODIFY  = 4,
    CHT_TYPE    = CHT_ADD | CHT_REMOVE | CHT_MODIFY,

    ///> Flag to indicate the change is already applied,
    ///> just notify observers (not compatible with CHT_MODIFY)
    CHT_DONE    = 8,
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
 * Class COMMIT
 *
 * Represents a set of changes (additions, deletions or modifications)
 * of a data model (e.g. the BOARD) class.
 *
 * The class can be used to propagate changes to subscribed objects (e.g. views, ratsnest),
 * and automatically create undo/redo points.
 */
class COMMIT
{
public:
    COMMIT();
    virtual ~COMMIT();

    ///> Adds a new item to the model
    COMMIT& Add( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_ADD );
    }

    ///> Notifies observers that aItem has been added
    COMMIT& Added( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_ADD | CHT_DONE );
    }

    ///> Removes a new item from the model
    COMMIT& Remove( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_REMOVE );
    }

    ///> Notifies observers that aItem has been removed
    COMMIT& Removed( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_REMOVE | CHT_DONE );
    }

    ///> Modifies a given item in the model.
    ///> Must be called before modification is performed.
    COMMIT& Modify( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_MODIFY );
    }

    ///> Creates an undo entry for an item that has been already modified. Requires a copy done
    ///> before the modification.
    COMMIT& Modified( EDA_ITEM* aItem, EDA_ITEM* aCopy );

    ///> Adds a change of the item aItem of type aChangeType to the change list.
    COMMIT& Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType );

    COMMIT& Stage( std::vector<EDA_ITEM*>& container, CHANGE_TYPE aChangeType );

    COMMIT& Stage( const PICKED_ITEMS_LIST& aItems, UNDO_REDO_T aModFlag = UR_UNSPECIFIED );

    ///> Executes the changes.
    virtual void Push( const wxString& aMessage ) = 0;

    ///> Revertes the commit by restoring the modifed items state.
    virtual void Revert() = 0;

    bool Empty() const
    {
        return m_changes.empty();
    }

protected:
    struct COMMIT_LINE
    {
        ///> Main item that is added/deleted/modified
        EDA_ITEM* m_item;

        ///> Optional copy of the item
        EDA_ITEM* m_copy;

        ///> Modification type
        CHANGE_TYPE m_type;
    };

    // Should be called in Push() & Revert() methods
    void clear()
    {
        m_changedItems.clear();
        m_changes.clear();
    }

    virtual void makeEntry( EDA_ITEM* aItem, CHANGE_TYPE aType, EDA_ITEM* aCopy = NULL );

    virtual EDA_ITEM* parentObject( EDA_ITEM* aItem ) const = 0;

    CHANGE_TYPE convert( UNDO_REDO_T aType ) const;

    std::set<EDA_ITEM*> m_changedItems;
    std::vector<COMMIT_LINE> m_changes;
};

#endif

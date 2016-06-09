/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
#include <base_struct.h>


class PICKED_ITEMS_LIST;

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
    ///> types of changes.
    enum CHANGE_TYPE {
        CHT_ADD = 0,
        CHT_REMOVE = 1,
        CHT_MODIFY = 2
    };

    COMMIT();
    virtual ~COMMIT();

    ///> Adds a new item to the model
    COMMIT& Add( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_ADD );
    }

    ///> Removes a new item from the model
    COMMIT& Remove( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_REMOVE );
    }

    ///> Modifies a given item in the model.
    ///> Must be called before modification is performed.
    COMMIT& Modify( EDA_ITEM* aItem )
    {
        return Stage( aItem, CHT_MODIFY );
    }

    ///> Adds a change of the item aItem of type aChangeType to the change list.
    virtual COMMIT& Stage( EDA_ITEM* aItem, CHANGE_TYPE aChangeType );


    void Stage( const PICKED_ITEMS_LIST& aItems, CHANGE_TYPE aChangeType );

    ///> Executes the changes.
    virtual void Push( const wxString& aMessage ) = 0;

    ///> Revertes the commit by restoring the modifed items state.
    virtual void Revert() = 0;

protected:
    struct COMMIT_LINE
    {
        EDA_ITEM *m_item;
        EDA_ITEM *m_copy;
        CHANGE_TYPE m_type;
    };

    virtual void makeEntry( EDA_ITEM* aItem, CHANGE_TYPE type, bool saveCopy );
    virtual EDA_ITEM* parentObject( EDA_ITEM* aItem ) const = 0;

    bool m_committed;

    std::set<EDA_ITEM*> m_changedItems;
    std::vector<COMMIT_LINE> m_changes;
};

#endif

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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

#ifndef __SELECTION_H
#define __SELECTION_H

#include <set>

#include <base_struct.h>
#include <view/view_group.h>


class SELECTION : public KIGFX::VIEW_GROUP
{
public:
    using ITER = std::set<EDA_ITEM*>::iterator;
    using CITER = std::set<EDA_ITEM*>::const_iterator;

    ITER begin() { return m_items.begin(); }
    ITER end() { return m_items.end(); }
    CITER begin() const { return m_items.cbegin(); }
    CITER end() const { return m_items.cend(); }

    virtual void Add( EDA_ITEM* aItem )
    {
        m_items.insert( aItem );
    }

    virtual void Remove( EDA_ITEM *aItem )
    {
        m_items.erase( aItem );
    }

    virtual void Clear() override
    {
        m_items.clear();
    }

    virtual unsigned int GetSize() const override
    {
        return m_items.size();
    }

    virtual KIGFX::VIEW_ITEM* GetItem( unsigned int idx ) const override
    {
        auto iter = m_items.begin();

        std::advance( iter, idx );

        return *iter;
    }

    bool Contains( EDA_ITEM* aItem ) const
    {
        return m_items.find( aItem ) != m_items.end();
    }

    /// Checks if there is anything selected
    bool Empty() const
    {
        return ( m_items.size() == 0 );
    }

    /// Returns the number of selected parts
    int Size() const
    {
        return m_items.size();
    }

    const std::set<EDA_ITEM*> GetItems() const
    {
        return m_items;
    }

    /// Returns the center point of the selection area bounding box.
    VECTOR2I GetCenter() const;

    const BOX2I ViewBBox() const override;

    EDA_ITEM* operator[]( const int index ) const
    {
        if( index < 0 || (unsigned int) index >= m_items.size() )
            return nullptr;

        auto iter = m_items.begin();
        std::advance( iter, index );
        return *iter;
    }

    EDA_ITEM* Front() const
    {
        if ( !m_items.size() )
            return nullptr;

        return *m_items.begin();
    }

    std::set<EDA_ITEM*>& Items()
    {
        return m_items;
    }

    virtual const VIEW_GROUP::ITEMS updateDrawList() const override;

private:
    /// Set of selected items
    std::set<EDA_ITEM*> m_items;

    // mute hidden overloaded virtual function warnings
    using VIEW_GROUP::Add;
    using VIEW_GROUP::Remove;
};

enum SELECTION_LOCK_FLAGS
{
    SELECTION_UNLOCKED = 0,
    SELECTION_LOCK_OVERRIDE = 1,
    SELECTION_LOCKED = 2
};

#endif

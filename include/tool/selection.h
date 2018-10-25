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

#include <deque>

#include <base_struct.h>
#include <view/view_group.h>


class SELECTION : public KIGFX::VIEW_GROUP
{
public:
    SELECTION()
    {
        m_isHover = false;
    }

    SELECTION( const SELECTION& aOther )
    {
        m_items = aOther.m_items;
        m_isHover = aOther.m_isHover;
    }

    const SELECTION& operator= ( const SELECTION& aOther )
    {
        m_items = aOther.m_items;
        m_isHover = aOther.m_isHover;
        return *this;
    }

    using ITER = std::deque<EDA_ITEM*>::iterator;
    using CITER = std::deque<EDA_ITEM*>::const_iterator;

    ITER begin() { return m_items.begin(); }
    ITER end() { return m_items.end(); }
    CITER begin() const { return m_items.cbegin(); }
    CITER end() const { return m_items.cend(); }

    void SetIsHover( bool aIsHover )
    {
        m_isHover = aIsHover;
    }

    bool IsHover() const
    {
        return m_isHover;
    }

    virtual void Add( EDA_ITEM* aItem )
    {
        ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        if( i == m_items.end() || *i > aItem )
            m_items.insert( i, aItem );
    }

    virtual void Remove( EDA_ITEM *aItem )
    {
        ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        if( !( i == m_items.end() || *i > aItem  ) )
            m_items.erase( i );
    }

    virtual void Clear() override
    {
        m_items.clear();
    }

    virtual unsigned int GetSize() const override
    {
        return m_items.size();
    }

    virtual KIGFX::VIEW_ITEM* GetItem( unsigned int aIdx ) const override
    {
        if( aIdx < m_items.size() )
            return m_items[ aIdx ];

        return nullptr;
    }

    bool Contains( EDA_ITEM* aItem ) const
    {
        CITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

        return !( i == m_items.end() || *i > aItem  );
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

    const std::deque<EDA_ITEM*> GetItems() const
    {
        return m_items;
    }

    /// Returns the center point of the selection area bounding box.
    VECTOR2I GetCenter() const;

    const BOX2I ViewBBox() const override;

    /// Returns the top left point of the selection area bounding box.
    VECTOR2I GetPosition() const;

    EDA_RECT    GetBoundingBox() const;
    EDA_ITEM*   GetTopLeftItem( bool onlyModules = false ) const;
    EDA_ITEM*   GetTopLeftModule() const;

    EDA_ITEM* operator[]( const size_t aIdx ) const
    {
        if( aIdx < m_items.size() )
            return m_items[ aIdx ];

        return nullptr;
    }

    EDA_ITEM* Front() const
    {
        return m_items.front();
    }

    std::deque<EDA_ITEM*>& Items()
    {
        return m_items;
    }

    template<class T>
    T* FirstOfKind() const
    {
        auto refType = T( nullptr ).Type();

        for( auto item : m_items )
        {
            if( item->Type() == refType )
                return static_cast<T*> ( item );
        }

        return nullptr;
    }

    /**
     * Checks if there is at least one item of requested kind.
     *
     * @param aType is the type to check for.
     * @return True if there is at least one item of such kind.
     */
    bool HasType( KICAD_T aType ) const
    {
        for( auto item : m_items )
        {
            if( item->Type() == aType )
                return true;
        }

        return false;
    }

    virtual const VIEW_GROUP::ITEMS updateDrawList() const override;

    bool HasReferencePoint() const
    {
        return m_referencePoint != NULLOPT;
    }

    VECTOR2I GetReferencePoint() const
    {
        return *m_referencePoint;
    }

    void SetReferencePoint( const VECTOR2I& aP )
    {
        m_referencePoint = aP;
    }

    void ClearReferencePoint()
    {
        m_referencePoint = NULLOPT;
    }

private:

    OPT<VECTOR2I> m_referencePoint;

    /// Set of selected items
    std::deque<EDA_ITEM*> m_items;

    bool m_isHover;

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

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <algorithm>
#include <eda_item.h>
#include <tool/selection.h>


bool SELECTION::operator==( const SELECTION& aOther ) const
{
    return ( m_items == aOther.m_items
             && m_itemsOrders == aOther.m_itemsOrders
             && m_isHover == aOther.m_isHover
             && m_lastAddedItem == aOther.m_lastAddedItem
             && m_orderCounter == aOther.m_orderCounter );
}


void SELECTION::Add( EDA_ITEM* aItem )
{
    // We're not sorting here; this is just a time-optimized way to do an
    // inclusion check.  std::lower_bound will return the first i >= aItem
    // and the second i > aItem check rules out i == aItem.
    ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

    if( i == m_items.end() || *i > aItem )
    {
        m_itemsOrders.insert( m_itemsOrders.begin() + std::distance( m_items.begin(), i ),
                              m_orderCounter );
        m_items.insert( i, aItem );
        m_orderCounter++;
        m_lastAddedItem = aItem;
    }
}


void SELECTION::Remove( EDA_ITEM* aItem )
{
    ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

    if( !( i == m_items.end() || *i > aItem ) )
    {
        m_itemsOrders.erase( m_itemsOrders.begin() + std::distance( m_items.begin(), i ) );
        m_items.erase( i );

        if( aItem == m_lastAddedItem )
            m_lastAddedItem = nullptr;
    }
}


KIGFX::VIEW_ITEM* SELECTION::GetItem( unsigned int aIdx ) const
{
    if( aIdx < m_items.size() )
        return m_items[aIdx];

    return nullptr;
}


bool SELECTION::Contains( EDA_ITEM* aItem ) const
{
    CITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

    return !( i == m_items.end() || *i > aItem );
}


VECTOR2I SELECTION::GetCenter() const
{
    static const std::vector<KICAD_T> textTypes = { SCH_TEXT_T, SCH_LABEL_LOCATE_ANY_T };
    bool                              hasOnlyText = true;

    // If the selection contains only texts calculate the center as the mean of all positions
    // instead of using the center of the total bounding box. Otherwise rotating the selection will
    // also translate it.
    for( EDA_ITEM* item : m_items )
    {
        if( !item->IsType( textTypes ) )
        {
            hasOnlyText = false;
            break;
        }
    }

    BOX2I bbox;

    if( hasOnlyText )
    {
        VECTOR2I center( 0, 0 );

        for( EDA_ITEM* item : m_items )
            center += item->GetPosition();

        center = center / static_cast<int>( m_items.size() );
        return static_cast<VECTOR2I>( center );
    }

    for( EDA_ITEM* item : m_items )
    {
        if( !item->IsType( { SCH_TEXT_T, SCH_LABEL_LOCATE_ANY_T } ) )
            bbox.Merge( item->GetBoundingBox() );
    }

    return static_cast<VECTOR2I>( bbox.GetCenter() );
}


BOX2I SELECTION::GetBoundingBox() const
{
    BOX2I bbox;

    for( EDA_ITEM* item : m_items )
        bbox.Merge( item->GetBoundingBox() );

    return bbox;
}


bool SELECTION::HasType( KICAD_T aType ) const
{
    for( const EDA_ITEM* item : m_items )
    {
        if( item->IsType( { aType } ) )
            return true;
    }

    return false;
}


size_t SELECTION::CountType( KICAD_T aType ) const
{
    size_t count = 0;

    for( const EDA_ITEM* item : m_items )
    {
        if( item->IsType( { aType } ) )
            count++;
    }

    return count;
}


VECTOR2I SELECTION::GetReferencePoint() const
{
    if( m_referencePoint )
        return *m_referencePoint;
    else
        return GetBoundingBox().Centre();
}


void SELECTION::SetReferencePoint( const VECTOR2I& aP )
{
    m_referencePoint = aP;
}


void SELECTION::ClearReferencePoint()
{
    m_referencePoint = std::nullopt;
}


const std::vector<KIGFX::VIEW_ITEM*> SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    for( EDA_ITEM* item : m_items )
        items.push_back( item );

    return items;
}


bool SELECTION::AreAllItemsIdentical() const
{
    return std::all_of( m_items.begin() + 1, m_items.end(),
            [&]( const EDA_ITEM* r )
            {
                return r->Type() == m_items.front()->Type();
            } );
}


bool SELECTION::OnlyContains( std::vector<KICAD_T> aList ) const
{
    return std::all_of( m_items.begin(), m_items.end(),
            [&]( const EDA_ITEM* r )
            {
                return r->IsType( aList );
            } );
}


std::vector<EDA_ITEM*> SELECTION::GetItemsSortedByTypeAndXY( bool leftBeforeRight,
                                                             bool topBeforeBottom ) const
{
    std::vector<EDA_ITEM*> sorted_items = std::vector<EDA_ITEM*>( m_items.begin(), m_items.end() );

    std::sort( sorted_items.begin(), sorted_items.end(),
               [&]( EDA_ITEM* a, EDA_ITEM* b )
               {
                   if( a->Type() == b->Type() )
                   {
                       const VECTOR2I aPos = a->GetSortPosition();
                       const VECTOR2I bPos = b->GetSortPosition();

                       if( aPos.x == bPos.x )
                       {
                           // Ensure deterministic sort
                           if( aPos.y == bPos.y )
                               return a->m_Uuid < b->m_Uuid;

                           if( topBeforeBottom )
                               return aPos.y < bPos.y;
                           else
                               return aPos.y > bPos.y;
                       }
                       else if( leftBeforeRight )
                       {
                           return aPos.x < bPos.x;
                       }
                       else
                       {
                           return aPos.x > bPos.x;
                       }
                   }
                   else
                   {
                       return a->Type() < b->Type();
                   }
               } );
    return sorted_items;
}


std::vector<EDA_ITEM*> SELECTION::GetItemsSortedBySelectionOrder() const
{
    using pairedIterators = std::pair<decltype( m_items.begin() ),
                                      decltype( m_itemsOrders.begin() )>;

    // Create a vector of all {selection item, selection order} iterator pairs
    std::vector<pairedIterators> pairs;
    auto                         item = m_items.begin();
    auto                         order = m_itemsOrders.begin();

    for( ; item != m_items.end(); ++item, ++order )
        pairs.emplace_back( make_pair( item, order ) );

    // Sort the pairs by the selection order
    std::sort( pairs.begin(), pairs.end(),
               []( pairedIterators const& a, pairedIterators const& b )
               {
                   return *a.second < *b.second;
               } );

    // Make a vector of just the sortedItems
    std::vector<EDA_ITEM*> sortedItems;

    for( pairedIterators sortedItem : pairs )
        sortedItems.emplace_back( *sortedItem.first );

    return sortedItems;
}

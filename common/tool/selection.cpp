/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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


void SELECTION::Add( EDA_ITEM* aItem )
{
    // We're not sorting here; this is just a time-optimized way to do an
    // inclusion check.  std::lower_bound will return the first i >= aItem
    // and the second i > aItem check rules out i == aItem.
    ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

    if( i == m_items.end() || *i > aItem )
        m_items.insert( i, aItem );
}


void SELECTION::Remove( EDA_ITEM* aItem )
{
    ITER i = std::lower_bound( m_items.begin(), m_items.end(), aItem );

    if( !( i == m_items.end() || *i > aItem ) )
        m_items.erase( i );
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


/// Returns the center point of the selection area bounding box.
VECTOR2I SELECTION::GetCenter() const
{
    KICAD_T textTypes[] = { SCH_TEXT_T, SCH_LABEL_T, SCH_GLOBAL_LABEL_T, SCH_HIER_LABEL_T, EOT };
    bool    hasOnlyText = true;

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

    EDA_RECT bbox;

    if( hasOnlyText )
    {
        wxPoint center( 0, 0 );

        for( EDA_ITEM* item : m_items )
            center += item->GetPosition();

        center = center / static_cast<unsigned>( m_items.size() );
        return static_cast<VECTOR2I>( center );
    }

    for( EDA_ITEM* item : m_items )
    {
        if( !item->IsType( textTypes ) )
            bbox.Merge( item->GetBoundingBox() );
    }

    return static_cast<VECTOR2I>( bbox.GetCenter() );
}


EDA_RECT SELECTION::GetBoundingBox() const
{
    EDA_RECT bbox;

    for( EDA_ITEM* item : m_items )
        bbox.Merge( item->GetBoundingBox() );

    return bbox;
}


bool SELECTION::HasType( KICAD_T aType ) const
{
    for( auto item : m_items )
    {
        if( item->Type() == aType )
            return true;
    }

    return false;
}


const std::vector<KIGFX::VIEW_ITEM*> SELECTION::updateDrawList() const
{
    std::vector<VIEW_ITEM*> items;

    for( auto item : m_items )
        items.push_back( item );

    return items;
}


bool SELECTION::AreAllItemsIdentical() const
{
    return ( std::all_of( m_items.begin() + 1, m_items.end(),
                    [&]( const EDA_ITEM* r )
                    {
                        return r->Type() == m_items.front()->Type();
                    } ) );
}


bool SELECTION::OnlyContains( std::vector<KICAD_T> aList ) const
{
    return ( std::all_of( m_items.begin(), m_items.end(),
            [&]( const EDA_ITEM* r )
            {
                bool ok = false;

                for( const KICAD_T& type : aList )
                {
                    if( r->Type() == type )
                    {
                        ok = true;
                        break;
                    }
                }

                return ok;
            } ) );
}

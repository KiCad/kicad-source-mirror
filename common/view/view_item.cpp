/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
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

#include <gal/definitions.h>

#include <view/view_item.h>
#include <view/view.h>

using namespace KIGFX;

void VIEW_ITEM::ViewSetVisible( bool aIsVisible )
{
    // update only if the visibility has really changed
    if( m_visible != aIsVisible )
    {
        m_visible = aIsVisible;
        ViewUpdate( APPEARANCE );
    }
}


void VIEW_ITEM::ViewRelease()
{
    if( m_view && m_view->IsDynamic() )
        m_view->Remove( this );
}


void VIEW_ITEM::getLayers( int* aLayers, int& aCount ) const
{
    int* layersPtr = aLayers;

    for( unsigned int i = 0; i < m_layers.size(); ++i )
    {
        if( m_layers[i] )
            *layersPtr++ = i;
    }

    aCount = m_layers.count();
}


int VIEW_ITEM::getGroup( int aLayer ) const
{
    for( int i = 0; i < m_groupsSize; ++i )
    {
        if( m_groups[i].first == aLayer )
            return m_groups[i].second;
    }

    return -1;
}


std::vector<int> VIEW_ITEM::getAllGroups() const
{
    std::vector<int> groups( m_groupsSize );

    for( int i = 0; i < m_groupsSize; ++i )
    {
        groups[i] = m_groups[i].second;
    }

    return groups;
}


void VIEW_ITEM::setGroup( int aLayer, int aId )
{
    // Look if there is already an entry for the layer
    for( int i = 0; i < m_groupsSize; ++i )
    {
        if( m_groups[i].first == aLayer )
        {
            m_groups[i].second = aId;
            return;
        }
    }

    // If there was no entry for the given layer - create one
    GroupPair* newGroups = new GroupPair[m_groupsSize + 1];

    if( m_groupsSize > 0 )
    {
        std::copy( m_groups, m_groups + m_groupsSize, newGroups );
        delete[] m_groups;
    }

    m_groups = newGroups;
    newGroups[m_groupsSize++] = GroupPair( aLayer, aId );
}


void VIEW_ITEM::deleteGroups()
{
    delete[] m_groups;
    m_groups = NULL;
    m_groupsSize = 0;
}

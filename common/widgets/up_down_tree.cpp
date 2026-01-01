/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
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

#include <widgets/up_down_tree.h>

// Need to use wxRTTI macros in order for OnCompareItems to work properly
// See: https://docs.wxwidgets.org/3.1/classwx_tree_ctrl.html#ab90a465793c291ca7aa827a576b7d146
wxIMPLEMENT_ABSTRACT_CLASS( UP_DOWN_TREE, wxTreeCtrl );


int UP_DOWN_TREE::OnCompareItems( const wxTreeItemId& item1, const wxTreeItemId& item2 )
{
    if( m_sortMap[item1] < m_sortMap[item2] )
        return -1;
    else if( m_sortMap[item1] > m_sortMap[item2] )
        return 1;
    else
        return 0;
}


void UP_DOWN_TREE::prepareForSort( const wxTreeItemId& aItem )
{
    // Number all siblings sequentially with even numbers.  The item moved will be given
    // an odd number.

    wxTreeItemId first = aItem;

    while( GetPrevSibling( first ) )
        first = GetPrevSibling( first );

    int idx = 2;
    m_sortMap.clear();

    m_sortMap[first] = idx;

    while( GetNextSibling( first ) )
    {
        first = GetNextSibling( first );
        idx += 2;
        m_sortMap[first] = idx;
    }
}


void UP_DOWN_TREE::MoveItemUp( const wxTreeItemId& aItem )
{
    prepareForSort( aItem );
    m_sortMap[aItem] = m_sortMap[aItem] - 3;
    SortChildren( GetItemParent( aItem ) );
}


void UP_DOWN_TREE::MoveItemDown( const wxTreeItemId& aItem )
{
    prepareForSort( aItem );
    m_sortMap[aItem] = m_sortMap[aItem] + 3;
    SortChildren( GetItemParent( aItem ) );
}



/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wx/dataview.h>
#include <wxdataviewctrl_helpers.h>

wxDataViewItem GetPrevItem( wxDataViewCtrl const& aView, wxDataViewItem const& aItem )
{
    auto prevItem = GetPrevSibling( aView, aItem );

    if( !prevItem.IsOk() )
    {
        prevItem = aView.GetModel()->GetParent( aItem );
    }
    else if( aView.IsExpanded( prevItem ) )
    {
        wxDataViewItemArray children;
        aView.GetModel()->GetChildren( prevItem, children );
        prevItem = children[children.size() - 1];
    }

    return prevItem;
}


wxDataViewItem GetNextItem( wxDataViewCtrl const& aView, wxDataViewItem const& aItem )
{
    wxDataViewItem nextItem;

    if( !aItem.IsOk() )
    {
        // No selection. Select the first.
        wxDataViewItemArray children;
        aView.GetModel()->GetChildren( aItem, children );
        return children[0];
    }

    if( aView.IsExpanded( aItem ) )
    {
        wxDataViewItemArray children;
        aView.GetModel()->GetChildren( aItem, children );
        nextItem = children[0];
    }
    else
    {
        // Walk up levels until we find one that has a next sibling.
        for( wxDataViewItem walk = aItem; walk.IsOk(); walk = aView.GetModel()->GetParent( walk ) )
        {
            nextItem = GetNextSibling( aView, walk );

            if( nextItem.IsOk() )
                break;
        }
    }

    return nextItem;
}


wxDataViewItem GetPrevSibling( wxDataViewCtrl const& aView, wxDataViewItem const& aItem )
{
    wxDataViewItemArray siblings;
    wxDataViewItem      invalid;
    wxDataViewItem      parent = aView.GetModel()->GetParent( aItem );

    aView.GetModel()->GetChildren( parent, siblings );

    for( size_t i = 0; i < siblings.size(); ++i )
    {
        if( siblings[i] == aItem )
        {
            if( i == 0 )
                return invalid;
            else
                return siblings[i - 1];
        }
    }

    return invalid;
}


wxDataViewItem GetNextSibling( wxDataViewCtrl const& aView, wxDataViewItem const& aItem )
{
    wxDataViewItemArray siblings;
    wxDataViewItem      invalid;
    wxDataViewItem      parent = aView.GetModel()->GetParent( aItem );

    aView.GetModel()->GetChildren( parent, siblings );

    for( size_t i = 0; i < siblings.size(); ++i )
    {
        if( siblings[i] == aItem )
        {
            if( i == siblings.size() - 1 )
                return invalid;
            else
                return siblings[i + 1];
        }
    }

    return invalid;
}

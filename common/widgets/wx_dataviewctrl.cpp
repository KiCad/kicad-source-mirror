/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <widgets/wx_dataviewctrl.h>
#include <wx/dataview.h>

wxDataViewItem WX_DATAVIEWCTRL::GetPrevItem( wxDataViewItem const& aItem )
{
    wxDataViewItem prevItem = GetPrevSibling( aItem );

    if( !prevItem.IsOk() )
    {
        prevItem = GetModel()->GetParent( aItem );
    }
    else if( IsExpanded( prevItem ) )
    {
        wxDataViewItemArray children;
        GetModel()->GetChildren( prevItem, children );

        if( children.size() )
            prevItem = children[children.size() - 1];
    }

    return prevItem;
}


wxDataViewItem WX_DATAVIEWCTRL::GetNextItem( wxDataViewItem const& aItem )
{
    wxDataViewItem nextItem;
    wxDataViewItem invalid;

    if( !aItem.IsOk() )
    {
        // No selection. Select the first.
        wxDataViewItemArray children;
        GetModel()->GetChildren( aItem, children );

        if( children.size() )
            return children[0];

        return invalid;
    }

    if( IsExpanded( aItem ) )
    {
        wxDataViewItemArray children;
        GetModel()->GetChildren( aItem, children );

        if( children.size() )
            return children[0];

        return invalid;
    }
    else
    {
        // Walk up levels until we find one that has a next sibling.
        for( wxDataViewItem walk = aItem; walk.IsOk(); walk = GetModel()->GetParent( walk ) )
        {
            nextItem = GetNextSibling( walk );

            if( nextItem.IsOk() )
                break;
        }
    }

    return nextItem;
}


wxDataViewItem WX_DATAVIEWCTRL::GetPrevSibling( wxDataViewItem const& aItem )
{
    wxDataViewItemArray siblings;
    wxDataViewItem      invalid;
    wxDataViewItem      parent = GetModel()->GetParent( aItem );

    GetModel()->GetChildren( parent, siblings );

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


wxDataViewItem WX_DATAVIEWCTRL::GetNextSibling( wxDataViewItem const& aItem )
{
    wxDataViewItemArray siblings;
    wxDataViewItem      invalid;
    wxDataViewItem      parent = GetModel()->GetParent( aItem );

    GetModel()->GetChildren( parent, siblings );

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


void recursiveDescent( WX_DATAVIEWCTRL* aCtrl, wxDataViewItem aItem, bool aExpand )
{
    wxDataViewItemArray children;

    aCtrl->GetModel()->GetChildren( aItem, children );

    for( size_t i = 0; i < children.size(); ++i )
        recursiveDescent( aCtrl, children[i], aExpand );

    if( aItem )
    {
        if( aExpand )
            aCtrl->Expand( aItem );
        else
            aCtrl->Collapse( aItem );
    }
}


void WX_DATAVIEWCTRL::ExpandAll()
{
    recursiveDescent( this, wxDataViewItem(), true );
}


void WX_DATAVIEWCTRL::CollapseAll()
{
    recursiveDescent( this, wxDataViewItem(), false );
}
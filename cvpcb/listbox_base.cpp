/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file listboxes.cpp
 * @brief Implementation of class for displaying footprint list and component lists.
 */

#include <cvpcb_mainframe.h>
#include <listboxes.h>


/******************************************************************************
* Basic class (from wxListView) to display component and footprint lists
* Not directly used: the 2 list boxes actually used are derived from it
******************************************************************************/

ITEMS_LISTBOX_BASE::ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                                        const wxPoint& aLocation, const wxSize& aSize,
                                        long aStyle) :
    wxListView( aParent, aId, aLocation, aSize, LISTBOX_STYLE | aStyle ), columnWidth(0)
{
    InsertColumn( 0, wxEmptyString );
}


ITEMS_LISTBOX_BASE::~ITEMS_LISTBOX_BASE()
{
}


void ITEMS_LISTBOX_BASE::UpdateWidth( int aLine )
{
    // Less than zero: recalculate width of all items.
    if( aLine < 0 )
    {
        columnWidth = 0;
        for( int ii = 0; ii < GetItemCount(); ii++ )
        {
            UpdateLineWidth( (unsigned)ii );
        }
    }

    // Zero or above: update from a single line.
    else
    {
        if( aLine < GetItemCount() )
            UpdateLineWidth( (unsigned)aLine );
    }
}


/*
 * Calculate the width of the given line, and increase the column width
 * if needed. This is effectively the wxListCtrl code for autosizing.
 * NB. it relies on the caller checking the given line number is valid.
 */
void ITEMS_LISTBOX_BASE::UpdateLineWidth( unsigned aLine )
{
    wxClientDC dc( this );
    wxCoord w;
    int newWidth = 10;  // Value of AUTOSIZE_COL_MARGIN from wxWidgets source.

    dc.SetFont( GetFont() );
    dc.GetTextExtent( GetItemText( aLine, 0 ) + " ", &w, NULL );
    newWidth += w;

    if( newWidth > columnWidth )
    {
        columnWidth = newWidth;
        SetColumnWidth( 0, columnWidth );
    }
}


/*
 * Return an index for the selected item
 */
int ITEMS_LISTBOX_BASE::GetSelection()
{
    return GetFirstSelected();
}

/*  Removes all selection in list
*/
void ITEMS_LISTBOX_BASE::DeselectAll()
{
    for( int i = 0; i < GetItemCount(); i++ )
        Select( i, false );
}


CVPCB_MAINFRAME* ITEMS_LISTBOX_BASE::GetParent() const
{
    return (CVPCB_MAINFRAME*) wxListView::GetParent();
}

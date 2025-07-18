/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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

/**
 * @file listbox_base.cpp
 * @brief Implementation of class for displaying footprint and symbol lists.
 */

#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <wx/dcclient.h>


ITEMS_LISTBOX_BASE::ITEMS_LISTBOX_BASE( CVPCB_MAINFRAME* aParent, wxWindowID aId,
                                        const wxPoint& aLocation, const wxSize& aSize,
                                        long aStyle ) :
        wxListView( aParent, aId, aLocation, aSize, LISTBOX_STYLE | aStyle ),
        m_isClosing( false ),
        m_columnWidth( 0 )
{
    InsertColumn( 0, wxEmptyString );
}


void ITEMS_LISTBOX_BASE::UpdateWidth( int aLine )
{
    wxClientDC dc( this );
    int itemCount = GetItemCount();

    // Less than zero: recalculate width of all items.
    if( aLine < 0 )
    {
        m_columnWidth = 0;

        for( int ii = 0; ii < itemCount; ii++ )
            UpdateLineWidth( (unsigned)ii, dc );
    }
    // Zero or above: update from a single line.
    else
    {
        if( aLine < itemCount )
            UpdateLineWidth( (unsigned)aLine, dc );
    }
}


void ITEMS_LISTBOX_BASE::UpdateLineWidth( unsigned aLine, wxClientDC& dc )
{
    wxCoord w;
    int newWidth = 10;  // Value of AUTOSIZE_COL_MARGIN from wxWidgets source.
    wxString str;

    dc.SetFont( GetFont() );

    if( IsVirtual() )
        str = OnGetItemText( aLine, 0 );
    else
        str = GetItemText( aLine, 0 );
    str += wxS( " " );

    dc.GetTextExtent( str, &w, nullptr );
    newWidth += w;

    if( newWidth > m_columnWidth )
    {
        m_columnWidth = newWidth;
        SetColumnWidth( 0, m_columnWidth );
    }
}


int ITEMS_LISTBOX_BASE::GetSelection()
{
    return GetFirstSelected();
}


void ITEMS_LISTBOX_BASE::DeselectAll()
{
    for( int i = GetFirstSelected(); i >= 0; i = GetNextSelected( i ) )
    {
        Select( i, false );
    }
}


CVPCB_MAINFRAME* ITEMS_LISTBOX_BASE::GetParent() const
{
    return (CVPCB_MAINFRAME*) wxListView::GetParent();
}

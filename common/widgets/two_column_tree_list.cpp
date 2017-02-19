/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Chris Pavlina <pavlina.chris@gmail.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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
 * @file two_column_tree_list.cpp
 */

#include <wx/dataview.h>
#include <widgets/two_column_tree_list.h>

/**
 * Extra margin to compensate for vertical scrollbar
 */
static const int HORIZ_MARGIN = 30;


TWO_COLUMN_TREE_LIST::TWO_COLUMN_TREE_LIST( wxWindow* aParent, wxWindowID aID,
                const wxPoint & aPos, const wxSize & aSize, long aStyle, const wxString & aName )
    : wxTreeListCtrl( aParent, aID, aPos, aSize, aStyle, aName ),
      m_rubber_band_column( 0 ),
      m_clamped_min_width( 50 )
{
    Bind( wxEVT_SIZE, &TWO_COLUMN_TREE_LIST::OnSize, this );
    GetDataView()->SetIndent( 10 );
}


void TWO_COLUMN_TREE_LIST::AutosizeColumns()
{
    wxSizeEvent dummy;
    OnSize( dummy );
}


void TWO_COLUMN_TREE_LIST::OnSize( wxSizeEvent& aEvent )
{
    wxDataViewCtrl* view = GetDataView();

    if( !view )
        return;

    wxRect rect = GetClientRect();
    view->SetSize( rect );

#ifdef wxHAS_GENERIC_DATAVIEWCTRL
    {
        wxWindow* win_view = GetView();
        win_view->Refresh();
        win_view->Update();
    }
#endif

    // Find the maximum width of both columns
    int clamped_column = ( m_rubber_band_column == 0 ) ? 1 : 0;
    int clamped_column_width = 0;
    int rubber_max_width = 0;

    for( wxTreeListItem item = GetFirstItem(); item.IsOk(); item = GetNextItem( item ) )
    {
        const wxString& text = GetItemText( item, clamped_column );
        int width = WidthFor( text );

        if( clamped_column == 0 )
        {
            width += 4 * view->GetIndent();
        }

        if( width > clamped_column_width )
            clamped_column_width = width;

        width = WidthFor( GetItemText( item, m_rubber_band_column ) );
        if( width > rubber_max_width )
            rubber_max_width = width;
    }

    if( clamped_column_width < m_clamped_min_width )
        clamped_column_width = m_clamped_min_width;

    // Rubber column width is only limited if the rubber column is on the LEFT.
    // If on the right, let the horiz scrollbar show.

    int rubber_width = 0;

    if( m_rubber_band_column == 0 )
        rubber_width = rect.width - clamped_column_width - HORIZ_MARGIN;
    else
        rubber_width = rubber_max_width;

    if( rubber_width <= 0 )
        rubber_width = 1;

    wxASSERT( m_rubber_band_column == 0 || m_rubber_band_column == 1 );

    if( GetColumnCount() >= 2 )
    {
        SetColumnWidth( m_rubber_band_column, rubber_width );
        SetColumnWidth( clamped_column, clamped_column_width );
    }
}


/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/tokenzr.h>
#include <wx/dc.h>
#include "wx_grid.h"


void WX_GRID::SetTable( wxGridTableBase* aTable )
{
    // wxGrid::SetTable() messes up the column widths from wxFormBuilder so we have to save
    // and restore them.
    int* formBuilderColWidths = new int[ GetNumberCols() ];

    for( int i = 0; i < GetNumberCols(); ++i )
        formBuilderColWidths[ i ] = GetColSize( i );

    wxGrid::SetTable( aTable );

    for( int i = 0; i < GetNumberCols(); ++i )
        SetColSize( i, formBuilderColWidths[ i ] );

    delete[] formBuilderColWidths;
}


void WX_GRID::DestroyTable( wxGridTableBase* aTable )
{
    // wxGrid's destructor will crash trying to look up the cell attr if the edit control
    // is left open.  Normally it's closed in Validate(), but not if the user hit Cancel.
    DisableCellEditControl();

    wxGrid::SetTable( nullptr );
    delete aTable;
}


wxString WX_GRID::GetShownColumns()
{
    wxString shownColumns;

    for( int i = 0; i < GetNumberCols(); ++i )
    {
        if( IsColShown( i ) )
        {
            if( shownColumns.Length() )
                shownColumns << wxT( " " );
            shownColumns << i;
        }
    }

    return shownColumns;
}


void WX_GRID::ShowHideColumns( const wxString& shownColumns )
{
    for( int i = 0; i < GetNumberCols(); ++i )
        HideCol( i );

    wxStringTokenizer shownTokens( shownColumns );

    while( shownTokens.HasMoreTokens() )
    {
        long colNumber;
        shownTokens.GetNextToken().ToLong( &colNumber );

        if( colNumber >= 0 && colNumber < GetNumberCols() )
            ShowCol( colNumber );
    }
}


// An re-implementation of wxGrid::DrawColLabel which left-aligns the first column when
// there are no row labels.
void WX_GRID::DrawColLabel( wxDC& dc, int col )
{
    if( GetColWidth( col ) <= 0 || m_colLabelHeight <= 0 )
        return;

    int colLeft = GetColLeft( col );

    wxRect rect( colLeft, 0, GetColWidth( col ), m_colLabelHeight );
    static wxGridColumnHeaderRendererDefault rend;

    // It is reported that we need to erase the background to avoid display
    // artefacts, see #12055.
    wxDCBrushChanger setBrush( dc, m_colWindow->GetBackgroundColour() );
    dc.DrawRectangle(rect);

    rend.DrawBorder( *this, dc, rect );

    int hAlign, vAlign;
    GetColLabelAlignment( &hAlign, &vAlign );
    const int orient = GetColLabelTextOrientation();

    if( col == 0 && GetRowLabelSize() == 0 )
        hAlign = wxALIGN_LEFT;

    rend.DrawLabel( *this, dc, GetColLabelValue( col ), rect, hAlign, vAlign, orient );
}

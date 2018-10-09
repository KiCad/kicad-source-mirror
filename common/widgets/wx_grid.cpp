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


#define MIN_GRIDCELL_MARGIN 3


WX_GRID::WX_GRID( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                  long style, const wxString& name ) :
        wxGrid( parent, id, pos, size, style, name ),
        m_weOwnTable( false )
{}


WX_GRID::~WX_GRID()
{
    if( m_weOwnTable )
        DestroyTable( GetTable() );
}


void WX_GRID::SetTable( wxGridTableBase* aTable, bool aTakeOwnership )
{
    // wxGrid::SetTable() messes up the column widths from wxFormBuilder so we have to save
    // and restore them.
    int numberCols = GetNumberCols();
    int* formBuilderColWidths = new int[numberCols];

    for( int i = 0; i < numberCols; ++i )
        formBuilderColWidths[ i ] = GetColSize( i );

    wxGrid::SetTable( aTable );
    // wxGrid::SetTable() may change the number of columns,
    // so prevent out-of-bounds access to formBuildColWidths
    numberCols = std::min( numberCols, GetNumberCols() );

    for( int i = 0; i < numberCols; ++i )
    {
        // correct wxFormBuilder width for large fonts and/or long translations
        int headingWidth = GetTextExtent( GetColLabelValue( i ) ).x + 2 * MIN_GRIDCELL_MARGIN;

        SetColSize( i, std::max( formBuilderColWidths[ i ], headingWidth ) );
    }

    delete[] formBuilderColWidths;

    Connect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), NULL, this );

    m_weOwnTable = aTakeOwnership;
}


void WX_GRID::DestroyTable( wxGridTableBase* aTable )
{
    // wxGrid's destructor will crash trying to look up the cell attr if the edit control
    // is left open.  Normally it's closed in Validate(), but not if the user hit Cancel.
    CommitPendingChanges( true /* quiet mode */ );

    Disconnect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), NULL, this );

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


bool WX_GRID::CommitPendingChanges( bool aQuietMode )
{
    if( !IsCellEditControlEnabled() )
        return true;

    if( !aQuietMode && SendEvent( wxEVT_GRID_EDITOR_HIDDEN ) == -1 )
        return false;

    HideCellEditControl();

    // do it after HideCellEditControl()
    m_cellEditCtrlEnabled = false;

    int row = m_currentCellCoords.GetRow();
    int col = m_currentCellCoords.GetCol();

    wxString oldval = GetCellValue( row, col );
    wxString newval;

    wxGridCellAttr* attr = GetCellAttr( row, col );
    wxGridCellEditor* editor = attr->GetEditor( this, row, col );

    bool changed = editor->EndEdit( row, col, this, oldval, &newval );

    editor->DecRef();
    attr->DecRef();

    if( changed )
    {
        if( !aQuietMode && SendEvent(wxEVT_GRID_CELL_CHANGING, newval) == -1 )
            return false;

        editor->ApplyEdit(row, col, this);

        // for compatibility reasons dating back to wx 2.8 when this event
        // was called wxEVT_GRID_CELL_CHANGE and wxEVT_GRID_CELL_CHANGING
        // didn't exist we allow vetoing this one too
        if( !aQuietMode && SendEvent( wxEVT_GRID_CELL_CHANGED, oldval ) == -1 )
        {
            // Event has been vetoed, set the data back.
            SetCellValue( row, col, oldval );
            return false;
        }
    }

    return true;
}


void WX_GRID::onGridColMove( wxGridEvent& aEvent )
{
    // wxWidgets won't move an open editor, so better just to close it
    CommitPendingChanges( true );
}

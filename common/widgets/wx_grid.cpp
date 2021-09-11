/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/settings.h>
#include <wx/tokenzr.h>
#include <wx/dc.h>
#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>
#include <algorithm>


#define MIN_GRIDCELL_MARGIN 3


WX_GRID::WX_GRID( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                  long style, const wxString& name ) :
        wxGrid( parent, id, pos, size, style, name ),
        m_weOwnTable( false )
{
    SetDefaultCellOverflow( false );

    // Make sure the GUI font scales properly on GTK
    SetDefaultCellFont( KIUI::GetControlFont( this ) );
}


WX_GRID::~WX_GRID()
{
    if( m_weOwnTable )
        DestroyTable( GetTable() );
}


void WX_GRID::SetColLabelSize( int aHeight )
{
    if( aHeight == 0 )
    {
        wxGrid::SetColLabelSize( 0 );
        return;
    }

    // Make sure the GUI font scales properly on GTK
    wxFont headingFont = KIUI::GetControlFont( this );
    headingFont.MakeBold();
    SetLabelFont( headingFont );

    // Correct wxFormBuilder height for large fonts
    int minHeight = headingFont.GetPixelSize().y + 2 * MIN_GRIDCELL_MARGIN;
    wxGrid::SetColLabelSize( std::max( aHeight, minHeight ) );
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

    // wxGrid::SetTable() may change the number of columns, so prevent out-of-bounds access
    // to formBuilderColWidths
    numberCols = std::min( numberCols, GetNumberCols() );

    for( int i = 0; i < numberCols; ++i )
    {
        // correct wxFormBuilder width for large fonts and/or long translations
        int headingWidth = GetTextExtent( GetColLabelValue( i ) ).x + 2 * MIN_GRIDCELL_MARGIN;

        SetColSize( i, std::max( formBuilderColWidths[ i ], headingWidth ) );
    }

    delete[] formBuilderColWidths;

    Connect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), nullptr, this );

    m_weOwnTable = aTakeOwnership;
}


void WX_GRID::DestroyTable( wxGridTableBase* aTable )
{
    // wxGrid's destructor will crash trying to look up the cell attr if the edit control
    // is left open.  Normally it's closed in Validate(), but not if the user hit Cancel.
    CommitPendingChanges( true /* quiet mode */ );

    Disconnect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), nullptr, this );

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


void WX_GRID::DrawColLabel( wxDC& dc, int col )
{
    if( GetColWidth( col ) <= 0 || m_colLabelHeight <= 0 )
        return;

    int colLeft = GetColLeft( col );

    wxRect rect( colLeft, 0, GetColWidth( col ), m_colLabelHeight );
    static wxGridColumnHeaderRendererDefault rend;

    // It is reported that we need to erase the background to avoid display
    // artifacts, see #12055.
    // wxWidgets renamed this variable between 3.1.2 and 3.1.3 ...
#if wxCHECK_VERSION( 3, 1, 3 )
    wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
#else
    wxDCBrushChanger setBrush( dc, m_colWindow->GetBackgroundColour() );
#endif
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
        if( !aQuietMode && SendEvent( wxEVT_GRID_CELL_CHANGING, newval ) == -1 )
            return false;

        editor->ApplyEdit( row, col, this );

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


int WX_GRID::GetVisibleWidth( int aCol, bool aHeader, bool aContents, bool aKeep )
{
    int size = 0;

    if( aCol < 0 )
    {
        if( aKeep )
            size = GetRowLabelSize();

        // The 1.1 scale factor is due to the fact row labels use a bold font, bigger than
        // the normal font.
        // TODO: use a better way to evaluate the text size, for bold font
        for( int row = 0; aContents && row < GetNumberRows(); row++ )
            size = std::max( size, int( GetTextExtent( GetRowLabelValue( row ) + "M" ).x * 1.1 ) );
    }
    else
    {
        if( aKeep )
            size = GetColSize( aCol );

        // 'M' is generally the widest character, so we buffer the column width by default to
        // ensure we don't write a continuous line of text at the column header
        if( aHeader )
        {
            EnsureColLabelsVisible();

            // The 1.1 scale factor is due to the fact headers use a bold font, bigger than
            // the normal font.
            size = std::max( size, int( GetTextExtent( GetColLabelValue( aCol ) + "M" ).x * 1.1 ) );
        }

        for( int row = 0; aContents && row < GetNumberRows(); row++ )
        {
            // If we have text, get the size.  Otherwise, use a placeholder for the checkbox
            if( GetTable()->CanGetValueAs( row, aCol, wxGRID_VALUE_STRING ) )
                size = std::max( size, GetTextExtent( GetCellValue( row, aCol ) + "M" ).x );
            else
                size = std::max( size, GetTextExtent( "MM" ).x );
        }
    }

    return size;
}


void WX_GRID::EnsureColLabelsVisible()
{
    // The 1.1 scale factor is due to the fact row labels use a bold font, bigger than
    // the normal font
    // TODO: use a better way to evaluate the text size, for bold font
    int line_height = int( GetTextExtent( "Mj" ).y * 1.1 ) + 3;
    int row_height = GetColLabelSize();

    // Headers can be multiline. Fix the Column Label Height to show the full header
    // However GetTextExtent does not work on multiline strings,
    // and do not return the full text height (only the height of one line)
    for( int col = 0; col < GetNumberCols(); col++ )
    {
        int nl_count = GetColLabelValue( col ).Freq( '\n' );

        if( nl_count )
        {
            // Col Label height must be able to show nl_count+1 lines
            if( row_height < line_height * ( nl_count+1 ) )
                row_height += line_height * nl_count;
        }
    }

    SetColLabelSize( row_height );
}

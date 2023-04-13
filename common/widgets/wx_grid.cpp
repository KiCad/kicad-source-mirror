/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/settings.h>

#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>
#include <algorithm>
#include <core/kicad_algo.h>
#include <gal/color4d.h>

#define MIN_GRIDCELL_MARGIN 3


wxColour getBorderColour()
{
    KIGFX::COLOR4D bg = wxSystemSettings::GetColour( wxSYS_COLOUR_FRAMEBK );
    KIGFX::COLOR4D fg = wxSystemSettings::GetColour( wxSYS_COLOUR_ACTIVEBORDER );
    KIGFX::COLOR4D border = fg.Mix( bg, 0.50 );
    return border.ToColour();
}


class WX_GRID_CORNER_HEADER_RENDERER : public wxGridCornerHeaderRendererDefault
{
public:
    void DrawBorder( const wxGrid& grid, wxDC& dc, wxRect& rect ) const override
    {
        wxDCBrushChanger SetBrush( dc, *wxTRANSPARENT_BRUSH );
        wxDCPenChanger   SetPen( dc, wxPen( getBorderColour(), 1 ) );

        rect.SetTop( rect.GetTop() + 1 );
        rect.SetLeft( rect.GetLeft() + 1 );
        rect.SetBottom( rect.GetBottom() - 1 );
        rect.SetRight( rect.GetRight() - 1 );
        dc.DrawRectangle( rect );
    }
};


class WX_GRID_COLUMN_HEADER_RENDERER : public wxGridColumnHeaderRendererDefault
{
public:
    void DrawBorder( const wxGrid& grid, wxDC& dc, wxRect& rect ) const override
    {
        wxDCBrushChanger SetBrush( dc, *wxTRANSPARENT_BRUSH );
        wxDCPenChanger   SetPen( dc, wxPen( getBorderColour(), 1 ) );

        rect.SetTop( rect.GetTop() + 1 );
        rect.SetLeft( rect.GetLeft() );
        rect.SetBottom( rect.GetBottom() - 1 );
        rect.SetRight( rect.GetRight() - 1 );
        dc.DrawRectangle( rect );
    }
};


class WX_GRID_ROW_HEADER_RENDERER : public wxGridRowHeaderRendererDefault
{
public:
    void DrawBorder( const wxGrid& grid, wxDC& dc, wxRect& rect ) const override
    {
        wxDCBrushChanger SetBrush( dc, *wxTRANSPARENT_BRUSH );
        wxDCPenChanger   SetPen( dc, wxPen( getBorderColour(), 1 ) );

        rect.SetTop( rect.GetTop() + 1 );
        rect.SetLeft( rect.GetLeft() + 1 );
        rect.SetBottom( rect.GetBottom() - 1 );
        rect.SetRight( rect.GetRight() );
        dc.DrawRectangle( rect );
    }
};


WX_GRID::WX_GRID( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size,
                  long style, const wxString& name ) :
        wxGrid( parent, id, pos, size, style, name ),
        m_weOwnTable( false )
{
    SetDefaultCellOverflow( false );

    // Make sure the GUI font scales properly
    SetDefaultCellFont( KIUI::GetControlFont( this ) );
    SetLabelFont( KIUI::GetControlFont( this ) );

    if( GetColLabelSize() > 0 )
        SetColLabelSize( GetColLabelSize() + 4 );

#if wxCHECK_VERSION( 3, 1, 3 )
    Connect( wxEVT_DPI_CHANGED, wxDPIChangedEventHandler( WX_GRID::onDPIChanged ), nullptr, this );
#endif

    Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( WX_GRID::onCellEditorShown ), nullptr, this );
    Connect( wxEVT_GRID_EDITOR_HIDDEN, wxGridEventHandler( WX_GRID::onCellEditorHidden ), nullptr, this );
}


WX_GRID::~WX_GRID()
{
    if( m_weOwnTable )
        DestroyTable( GetTable() );

#if wxCHECK_VERSION( 3, 1, 3 )
    Disconnect( wxEVT_DPI_CHANGED, wxDPIChangedEventHandler( WX_GRID::onDPIChanged ), nullptr, this );
#endif

}


#if wxCHECK_VERSION( 3, 1, 3 )
void WX_GRID::onDPIChanged(wxDPIChangedEvent& aEvt)
{
    /// This terrible hack is a way to avoid the incredibly disruptive resizing of grids that happens on Macs
    /// when moving a window between monitors of different DPIs.
#ifndef __WXMAC__
    aEvt.Skip();
#endif
}
#endif


void WX_GRID::SetColLabelSize( int aHeight )
{
    if( aHeight == 0 )
    {
        wxGrid::SetColLabelSize( 0 );
        return;
    }

    // Correct wxFormBuilder height for large fonts
    int minHeight = GetLabelFont().GetPixelSize().y + 2 * MIN_GRIDCELL_MARGIN;
    wxGrid::SetColLabelSize( std::max( aHeight, minHeight ) );
}


void WX_GRID::SetLabelFont( const wxFont& aFont )
{
    wxGrid::SetLabelFont( KIUI::GetControlFont( this ) );
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
    Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( WX_GRID::onGridCellSelect ), nullptr, this );

    m_weOwnTable = aTakeOwnership;
}


void WX_GRID::onGridCellSelect( wxGridEvent& aEvent )
{
    // Highlight the selected cell.
    // Calling SelectBlock() allows a visual effect when cells are selected by tab or arrow keys.
    // Otherwise, one cannot really know what actual cell is selected.
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( row >= 0 && col >= 0 )
        SelectBlock( row, col, row, col, false );
}


void WX_GRID::onCellEditorShown( wxGridEvent& aEvent )
{
    if( alg::contains( m_autoEvalCols, aEvent.GetCol() ) )
    {
        int row = aEvent.GetRow();
        int col = aEvent.GetCol();

        const std::pair<wxString, wxString>& beforeAfter = m_evalBeforeAfter[ { row, col } ];

        if( GetCellValue( row, col ) == beforeAfter.second )
            SetCellValue( row, col, beforeAfter.first );
    }
}


void WX_GRID::onCellEditorHidden( wxGridEvent& aEvent )
{
    if( alg::contains( m_autoEvalCols, aEvent.GetCol() ) )
    {
        UNITS_PROVIDER* unitsProvider = m_unitsProviders[ aEvent.GetCol() ];

        if( !unitsProvider )
            unitsProvider = m_unitsProviders.begin()->second;

        m_eval->SetDefaultUnits( unitsProvider->GetUserUnits() );

        int row = aEvent.GetRow();
        int col = aEvent.GetCol();

        CallAfter(
              [this, row, col, unitsProvider]()
              {
                  wxString stringValue = GetCellValue( row, col );

                  if( m_eval->Process( stringValue ) )
                  {
                      int      val = unitsProvider->ValueFromString( m_eval->Result() );
                      wxString evalValue = unitsProvider->StringFromValue( val, true );

                      if( stringValue != evalValue )
                      {
                          SetCellValue( row, col, evalValue );
                          m_evalBeforeAfter[ { row, col } ] = { stringValue, evalValue };
                      }
                  }
              } );
    }

    aEvent.Skip();
}


void WX_GRID::DestroyTable( wxGridTableBase* aTable )
{
    // wxGrid's destructor will crash trying to look up the cell attr if the edit control
    // is left open.  Normally it's closed in Validate(), but not if the user hit Cancel.
    CommitPendingChanges( true /* quiet mode */ );

    Disconnect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), nullptr, this );
    Disconnect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( WX_GRID::onGridCellSelect ), nullptr, this );

    wxGrid::SetTable( nullptr );
    delete aTable;
}


wxString WX_GRID::GetShownColumnsAsString()
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


std::bitset<64> WX_GRID::GetShownColumns()
{
    std::bitset<64> shownColumns;

    for( int ii = 0; ii < GetNumberCols(); ++ii )
        shownColumns[ii] = IsColShown( ii );

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
            ShowCol( (int) colNumber );
    }
}


void WX_GRID::ShowHideColumns( const std::bitset<64>& aShownColumns )
{
    for( int ii = 0; ii < GetNumberCols(); ++ ii )
    {
        if( aShownColumns[ii] )
            ShowCol( ii );
        else
            HideCol( ii );
    }
}


void WX_GRID::DrawCornerLabel( wxDC& dc )
{
    if( m_nativeColumnLabels )
        wxGrid::DrawCornerLabel( dc );

    wxRect rect( wxSize( m_rowLabelWidth, m_colLabelHeight ) );

    static WX_GRID_CORNER_HEADER_RENDERER rend;

    // It is reported that we need to erase the background to avoid display
    // artifacts, see #12055.
    {
        // wxWidgets renamed this variable between 3.1.2 and 3.1.3 ...
#if wxCHECK_VERSION( 3, 1, 3 )
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
#else
        wxDCBrushChanger setBrush( dc, m_colWindow->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colWindow->GetBackgroundColour() );
#endif
        dc.DrawRectangle( rect.Inflate( 1 ) );
    }

    rend.DrawBorder( *this, dc, rect );
}


void WX_GRID::DrawColLabel( wxDC& dc, int col )
{
    if( m_nativeColumnLabels )
        wxGrid::DrawColLabel( dc, col );

    if( GetColWidth( col ) <= 0 || m_colLabelHeight <= 0 )
        return;

    wxRect rect( GetColLeft( col ), 0, GetColWidth( col ), m_colLabelHeight );

    static WX_GRID_COLUMN_HEADER_RENDERER rend;

    // It is reported that we need to erase the background to avoid display
    // artifacts, see #12055.
    {
        // wxWidgets renamed this variable between 3.1.2 and 3.1.3 ...
#if wxCHECK_VERSION( 3, 1, 3 )
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
#else
        wxDCBrushChanger setBrush( dc, m_colWindow->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colWindow->GetBackgroundColour() );
#endif
        dc.DrawRectangle( rect.Inflate( 1 ) );
    }

    rend.DrawBorder( *this, dc, rect );

    // Make sure fonts get scaled correctly on GTK HiDPI monitors
    dc.SetFont( GetLabelFont() );

    int hAlign, vAlign;
    GetColLabelAlignment( &hAlign, &vAlign );
    const int orient = GetColLabelTextOrientation();

    if( col == 0 )
        hAlign = wxALIGN_LEFT;

    if( hAlign == wxALIGN_LEFT )
        rect.SetLeft( rect.GetLeft() + MIN_GRIDCELL_MARGIN );

    rend.DrawLabel( *this, dc, GetColLabelValue( col ), rect, hAlign, vAlign, orient );
}


void WX_GRID::DrawRowLabel( wxDC& dc, int row )
{
    if ( GetRowHeight( row ) <= 0 || m_rowLabelWidth <= 0 )
        return;

    wxRect rect( 0, GetRowTop( row ), m_rowLabelWidth, GetRowHeight( row ) );

    static WX_GRID_ROW_HEADER_RENDERER rend;

    // It is reported that we need to erase the background to avoid display
    // artifacts, see #12055.
    {
        // wxWidgets renamed this variable between 3.1.2 and 3.1.3 ...
#if wxCHECK_VERSION( 3, 1, 3 )
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
#else
        wxDCBrushChanger setBrush( dc, m_colWindow->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colWindow->GetBackgroundColour() );
#endif
        dc.DrawRectangle( rect.Inflate( 1 ) );
    }

    rend.DrawBorder( *this, dc, rect );

    // Make sure fonts get scaled correctly on GTK HiDPI monitors
    dc.SetFont( GetLabelFont() );

    int hAlign, vAlign;
    GetRowLabelAlignment(&hAlign, &vAlign);

    if( hAlign == wxALIGN_LEFT )
        rect.SetLeft( rect.GetLeft() + MIN_GRIDCELL_MARGIN );

    rend.DrawLabel( *this, dc, GetRowLabelValue( row ), rect, hAlign, vAlign, wxHORIZONTAL );
}


bool WX_GRID::CancelPendingChanges()
{
    if( !IsCellEditControlEnabled() )
        return true;

    HideCellEditControl();

    // do it after HideCellEditControl()
    m_cellEditCtrlEnabled = false;

    int row = m_currentCellCoords.GetRow();
    int col = m_currentCellCoords.GetCol();

    wxString oldval = GetCellValue( row, col );
    wxString newval;

    wxGridCellAttr* attr = GetCellAttr( row, col );
    wxGridCellEditor* editor = attr->GetEditor( this, row, col );

    editor->EndEdit( row, col, this, oldval, &newval );

    editor->DecRef();
    attr->DecRef();

    return true;
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


void WX_GRID::SetUnitsProvider( UNITS_PROVIDER* aProvider, int aCol )
{
    m_unitsProviders[ aCol ] = aProvider;

    if( !m_eval )
        m_eval = std::make_unique<NUMERIC_EVALUATOR>( aProvider->GetUserUnits() );
}


int WX_GRID::GetUnitValue( int aRow, int aCol )
{
    UNITS_PROVIDER* unitsProvider = m_unitsProviders[ aCol ];

    if( !unitsProvider )
        unitsProvider = m_unitsProviders.begin()->second;

    wxString stringValue = GetCellValue( aRow, aCol );

    if( alg::contains( m_autoEvalCols, aCol ) )
    {
        m_eval->SetDefaultUnits( unitsProvider->GetUserUnits() );

        if( m_eval->Process( stringValue ) )
            stringValue = m_eval->Result();
    }

    return unitsProvider->ValueFromString( stringValue );
}


void WX_GRID::SetUnitValue( int aRow, int aCol, int aValue )
{
    UNITS_PROVIDER* unitsProvider = m_unitsProviders[ aCol ];

    if( !unitsProvider )
        unitsProvider = m_unitsProviders.begin()->second;

    SetCellValue( aRow, aCol, unitsProvider->StringFromValue( aValue, true ) );
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

        for( int row = 0; aContents && row < GetNumberRows(); row++ )
            size = std::max( size, int( GetTextExtent( GetRowLabelValue( row ) + wxS( "M" ) ).x ) );
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

            size = std::max( size, int( GetTextExtent( GetColLabelValue( aCol ) + wxS( "M" ) ).x ) );
        }

        for( int row = 0; aContents && row < GetNumberRows(); row++ )
        {
            // If we have text, get the size.  Otherwise, use a placeholder for the checkbox
            if( GetTable()->CanGetValueAs( row, aCol, wxGRID_VALUE_STRING ) )
                size = std::max( size, GetTextExtent( GetCellValue( row, aCol ) + wxS( "M" ) ).x );
            else
                size = std::max( size, GetTextExtent( "MM" ).x );
        }
    }

    return size;
}


void WX_GRID::EnsureColLabelsVisible()
{
    int line_height = int( GetTextExtent( "Mj" ).y ) + 3;
    int row_height = GetColLabelSize();
    int initial_row_height = row_height;

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

    // Update the column label size, but only if needed, to avoid generating useless
    // and perhaps annoying UI events when the size does not change
    if( initial_row_height != row_height )
        SetColLabelSize( row_height );
}

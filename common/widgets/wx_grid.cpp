/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/colour.h>
#include <wx/tokenzr.h>
#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/event.h> // Needed for textentry.h on MSW
#include <wx/textentry.h>

#include <widgets/indicator_icon.h>
#include <widgets/grid_icon_text_helpers.h>
#include <widgets/wx_grid.h>
#include <widgets/ui_common.h>
#include <algorithm>
#include <core/kicad_algo.h>
#include <gal/color4d.h>
#include <kiplatform/ui.h>
#include <utility>

#include <pgm_base.h>
#include <settings/common_settings.h>
#include <dialog_shim.h>


wxGridCellAttr* WX_GRID_TABLE_BASE::enhanceAttr( wxGridCellAttr* aInputAttr, int aRow, int aCol,
                                                 wxGridCellAttr::wxAttrKind aKind  )
{
    wxGridCellAttr* attr = aInputAttr;

    if( wxGridCellAttrProvider* provider = GetAttrProvider() )
    {
        wxGridCellAttr* providerAttr = provider->GetAttr( aRow, aCol, aKind );

        if( providerAttr )
        {
            attr = new wxGridCellAttr;
            attr->SetKind( wxGridCellAttr::Merged );

            if( aInputAttr )
            {
                attr->MergeWith( aInputAttr );
                aInputAttr->DecRef();
            }

            attr->MergeWith( providerAttr );
            providerAttr->DecRef();
        }
    }

    return attr;
}


#define MIN_GRIDCELL_MARGIN FromDIP( 2 )


void WX_GRID::CellEditorSetMargins( wxTextEntryBase* aEntry )
{
    // This is consistent with wxGridCellTextEditor. But works differently across platforms of course.
    aEntry->SetMargins( 0, 0 );
}


void WX_GRID::CellEditorTransformSizeRect( wxRect& aRect )
{
#if defined( __WXMSW__ ) || defined( __WXGTK__ )
    aRect.Deflate( 2 );
#endif
}


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


/**
 * Attribute provider that provides attributes (or modifies the existing attribute) to alternate
 * a row color between the odd and even rows.
 */
class WX_GRID_ALT_ROW_COLOR_PROVIDER : public wxGridCellAttrProvider
{
public:
    WX_GRID_ALT_ROW_COLOR_PROVIDER( const wxColor& aBaseColor ) :
            wxGridCellAttrProvider(),
            m_attrEven( new wxGridCellAttr() )
    {
        UpdateColors( aBaseColor );
    }

    void UpdateColors( const wxColor& aBaseColor )
    {
        // Choose the default color, taking into account if the dark mode theme is enabled
        wxColor rowColor = aBaseColor.ChangeLightness( KIPLATFORM::UI::IsDarkTheme() ? 105 : 95 );

        m_attrEven->SetBackgroundColour( rowColor );
    }

    wxGridCellAttr* GetAttr( int row, int col, wxGridCellAttr::wxAttrKind kind ) const override
    {
        wxGridCellAttrPtr cellAttr( wxGridCellAttrProvider::GetAttr( row, col, kind ) );

        // Just pass through the cell attribute on odd rows (start normal to allow for the
        // header row)
        if( !( row % 2 ) )
            return cellAttr.release();

        if( !cellAttr )
        {
            cellAttr = m_attrEven;
        }
        else
        {
            if( !cellAttr->HasBackgroundColour() )
            {
                cellAttr = cellAttr->Clone();
                cellAttr->SetBackgroundColour( m_attrEven->GetBackgroundColour() );
            }
        }

        return cellAttr.release();
    }

private:
    wxGridCellAttrPtr m_attrEven;
};


WX_GRID::WX_GRID( wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style,
                  const wxString& name ) :
        wxGrid( parent, id, pos, size, style, name ),
        m_weOwnTable( false )
{
    // Grids with comboboxes need a bit of extra height; other grids look better if they're consistent.
    SetDefaultRowSize( GetDefaultRowSize() + FromDIP( 4 ) );

    SetDefaultCellOverflow( false );

    // Make sure the GUI font scales properly
    SetDefaultCellFont( KIUI::GetControlFont( this ) );
    SetLabelFont( KIUI::GetControlFont( this ) );

    m_rowIconProvider = new ROW_ICON_PROVIDER( KIUI::c_IndicatorSizeDIP, this );

    Connect( wxEVT_DPI_CHANGED, wxDPIChangedEventHandler( WX_GRID::onDPIChanged ), nullptr, this );
    Connect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( WX_GRID::onCellEditorShown ), nullptr, this );
    Connect( wxEVT_GRID_EDITOR_HIDDEN, wxGridEventHandler( WX_GRID::onCellEditorHidden ), nullptr, this );
}


WX_GRID::~WX_GRID()
{
    if( m_weOwnTable )
        DestroyTable( GetTable() );

    delete m_rowIconProvider;

    Disconnect( wxEVT_GRID_EDITOR_SHOWN, wxGridEventHandler( WX_GRID::onCellEditorShown ), nullptr, this );
    Disconnect( wxEVT_GRID_EDITOR_HIDDEN, wxGridEventHandler( WX_GRID::onCellEditorHidden ), nullptr, this );
    Disconnect( wxEVT_DPI_CHANGED, wxDPIChangedEventHandler( WX_GRID::onDPIChanged ), nullptr, this );
}


void WX_GRID::onDPIChanged(wxDPIChangedEvent& aEvt)
{
    CallAfter( [&]()
               {
                   wxGrid::SetColLabelSize( wxGRID_AUTOSIZE );
               } );

    aEvt.Skip();
}


void WX_GRID::SetColLabelSize( int aHeight )
{
    if( aHeight == 0 || aHeight == wxGRID_AUTOSIZE )
    {
        wxGrid::SetColLabelSize( aHeight );
        return;
    }

    // Correct wxFormBuilder height for large fonts
    int minHeight = GetCharHeight() + 2 * MIN_GRIDCELL_MARGIN;

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

    EnableAlternateRowColors( Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Appearance.grid_striping );

    Connect( wxEVT_GRID_COL_MOVE, wxGridEventHandler( WX_GRID::onGridColMove ), nullptr, this );
    Connect( wxEVT_GRID_SELECT_CELL, wxGridEventHandler( WX_GRID::onGridCellSelect ), nullptr, this );

    m_weOwnTable = aTakeOwnership;
}


void WX_GRID::EnableAlternateRowColors( bool aEnable )
{
    wxGridTableBase* table = wxGrid::GetTable();

    wxCHECK_MSG( table, /* void */, "Tried to enable alternate row colors without a table assigned to the grid" );

    if( aEnable )
    {
        wxColor color = wxGrid::GetDefaultCellBackgroundColour();
        table->SetAttrProvider( new WX_GRID_ALT_ROW_COLOR_PROVIDER( color ) );
    }
    else
    {
        table->SetAttrProvider( nullptr );
    }
}


void WX_GRID::onGridCellSelect( wxGridEvent& aEvent )
{
    // Highlight the selected cell.
    // Calling SelectBlock() allows a visual effect when cells are selected by tab or arrow keys.
    // Otherwise, one cannot really know what actual cell is selected.
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( row >= 0 && row < GetNumberRows() && col >= 0 && col < GetNumberCols() )
    {
        if( GetSelectionMode() == wxGrid::wxGridSelectCells )
        {
            SelectBlock( row, col, row, col, false );
        }
        else if( GetSelectionMode() == wxGrid::wxGridSelectRows
                 || GetSelectionMode() == wxGrid::wxGridSelectRowsOrColumns )
        {
            SelectBlock( row, 0, row, GetNumberCols() - 1, false );
        }
        else if( GetSelectionMode() == wxGrid::wxGridSelectColumns )
        {
            SelectBlock( 0, col, GetNumberRows() - 1, col, false );
        }
    }
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
    const int col = aEvent.GetCol();

    if( alg::contains( m_autoEvalCols, col ) )
    {
        UNITS_PROVIDER* unitsProvider = getUnitsProvider( col );

        auto cellUnitsData = getColumnUnits( col );
        EDA_UNITS cellUnits = cellUnitsData.first;
        EDA_DATA_TYPE cellDataType = cellUnitsData.second;

        m_eval->SetDefaultUnits( cellUnits );

        const int row = aEvent.GetRow();

        // Determine if this cell is marked as holding nullable values
        bool              isNullable = false;
        wxGridCellEditor* cellEditor = GetCellEditor( row, col );

        if( cellEditor )
        {
            if( const GRID_CELL_NULLABLE_INTERFACE* nullable =
                        dynamic_cast<GRID_CELL_NULLABLE_INTERFACE*>( cellEditor ) )
                isNullable = nullable->IsNullable();

            cellEditor->DecRef();
        }

        CallAfter(
                [this, row, col, isNullable, unitsProvider, cellDataType]()
                {
                    // Careful; if called from CommitPendingChange() in a delete operation, the cell may
                    // no longer exist.
                    if( row >= GetNumberRows() || col >= GetNumberCols() )
                        return;

                    wxString stringValue = GetCellValue( row, col );
                    bool     processedOk = true;

                    if( stringValue != UNITS_PROVIDER::NullUiString )
                        processedOk = m_eval->Process( stringValue );

                    if( processedOk )
                    {
                        wxString evalValue;

                        if( isNullable )
                        {
                            std::optional<int> val;

                            if( stringValue == UNITS_PROVIDER::NullUiString )
                            {
                                val = unitsProvider->OptionalValueFromString( UNITS_PROVIDER::NullUiString,
                                                                              cellDataType );
                            }
                            else
                            {
                                val = unitsProvider->OptionalValueFromString( m_eval->Result(), cellDataType );
                            }

                            evalValue = unitsProvider->StringFromOptionalValue( val, true, cellDataType );
                        }
                        else
                        {
                            int val = unitsProvider->ValueFromString( m_eval->Result(), cellDataType );
                            evalValue = unitsProvider->StringFromValue( val, true, cellDataType );
                        }

                        if( stringValue != evalValue )
                        {
                            SetCellValue( row, col, evalValue );
                            m_evalBeforeAfter[{ row, col }] = { stringValue, evalValue };
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

    wxStringTokenizer shownTokens( shownColumns, " \t\r\n", wxTOKEN_STRTOK );

    while( shownTokens.HasMoreTokens() )
    {
        long colNumber;
        shownTokens.GetNextToken().ToLong( &colNumber );

        if( colNumber >= 0 && colNumber < GetNumberCols() )
            ShowCol( (int) colNumber );
    }
}


void WX_GRID::DrawCornerLabel( wxDC& dc )
{
    if( m_nativeColumnLabels )
        wxGrid::DrawCornerLabel( dc );

    wxRect rect( wxSize( m_rowLabelWidth, m_colLabelHeight ) );

    static WX_GRID_CORNER_HEADER_RENDERER rend;

    // It is reported that we need to erase the background to avoid display artifacts; see #12055.
    {
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
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

    // It is reported that we need to erase the background to avoid display artifacts; see #12055.
    {
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
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
    if( GetRowHeight( row ) <= 0 || m_rowLabelWidth <= 0 )
        return;

    wxRect rect( 0, GetRowTop( row ), m_rowLabelWidth, GetRowHeight( row ) );

    static WX_GRID_ROW_HEADER_RENDERER rend;

    // It is reported that we need to erase the background to avoid display artifacts; see #12055.
    {
        wxDCBrushChanger setBrush( dc, m_colLabelWin->GetBackgroundColour() );
        wxDCPenChanger   setPen( dc, m_colLabelWin->GetBackgroundColour() );
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

        if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
            dlg->OnModify();
    }

    return true;
}


void WX_GRID::OnAddRow( const std::function<std::pair<int, int>()>& aAdder )
{
    if( !CommitPendingChanges() )
        return;

    auto [row, editCol] = aAdder();

    // wx documentation is wrong, SetGridCursor does not make visible.
    SetFocus();
    MakeCellVisible( row, std::max( editCol, 0 ) );
    SetGridCursor( row, std::max( editCol, 0 ) );

    if( editCol >= 0 )
    {
        EnableCellEditControl( true );
        ShowCellEditControl();
    }
}


void WX_GRID::OnDeleteRows( const std::function<void( int row )>& aDeleter )
{
    OnDeleteRows(
            []( int row )
            {
                return true;
            },
            aDeleter );
}


void WX_GRID::OnDeleteRows( const std::function<bool( int row )>& aFilter,
                            const std::function<void( int row )>& aDeleter )
{
    wxArrayInt selectedRows = GetSelectedRows();

    if( selectedRows.empty() && GetGridCursorRow() >= 0 )
        selectedRows.push_back( GetGridCursorRow() );

    if( selectedRows.empty() )
        return;

    for( int row : selectedRows )
    {
        if( !aFilter( row ) )
            return;
    }

    if( !CommitPendingChanges() )
        return;

    // Reverse sort so deleting a row doesn't change the indexes of the other rows.
    selectedRows.Sort(
            []( int* first, int* second )
            {
                return *second - *first;
            } );

    int nextSelRow = selectedRows.back() - 1;

    if( nextSelRow >= 0 )
    {
        GoToCell( nextSelRow, GetGridCursorCol() );
        SetGridCursor( nextSelRow, GetGridCursorCol() );
    }

    for( int row : selectedRows )
        aDeleter( row );
}


void WX_GRID::SwapRows( int aRowA, int aRowB )
{
    for( int col = 0; col < GetNumberCols(); ++col )
    {
        wxString temp = GetCellValue( aRowA, col );
        SetCellValue( aRowA, col, GetCellValue( aRowB, col ) );
        SetCellValue( aRowB, col, temp );
    }
}


void WX_GRID::OnMoveRowUp( const std::function<void( int row )>& aMover )
{
    OnMoveRowUp(
            []( int row )
            {
                return true;
            },
            aMover );
}


void WX_GRID::OnMoveRowUp( const std::function<bool( int row )>& aFilter,
                           const std::function<void( int row )>& aMover )
{
    if( !CommitPendingChanges() )
        return;

    int i = GetGridCursorRow();

    if( i > 0 && aFilter( i ) )
    {
        aMover( i );

        SetGridCursor( i - 1, GetGridCursorCol() );
        MakeCellVisible( GetGridCursorRow(), GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void WX_GRID::OnMoveRowDown( const std::function<void( int row )>& aMover )
{
    OnMoveRowDown(
            []( int row )
            {
                return true;
            },
            aMover );
}


void WX_GRID::OnMoveRowDown( const std::function<bool( int row )>& aFilter,
                             const std::function<void( int row )>& aMover )
{
    if( !CommitPendingChanges() )
        return;

    int i = GetGridCursorRow();

    if( i + 1 < GetNumberRows() && aFilter( i ) )
    {
        aMover( i );

        SetGridCursor( i + 1, GetGridCursorCol() );
        MakeCellVisible( GetGridCursorRow(), GetGridCursorCol() );
    }
    else
    {
        wxBell();
    }
}


void WX_GRID::SetUnitsProvider( UNITS_PROVIDER* aProvider, int aCol )
{
    m_unitsProviders[ aCol ] = aProvider;

    if( !m_eval )
        m_eval = std::make_unique<NUMERIC_EVALUATOR>( aProvider->GetUserUnits() );
}


void WX_GRID::SetAutoEvalColUnits( const int col, EDA_UNITS aUnit, EDA_DATA_TYPE aUnitType )
{
    m_autoEvalColsUnits[col] = std::make_pair( aUnit, aUnitType );
}


void WX_GRID::SetAutoEvalColUnits( const int col, EDA_UNITS aUnit )
{
    const EDA_DATA_TYPE type = UNITS_PROVIDER::GetTypeFromUnits( aUnit );
    SetAutoEvalColUnits( col, aUnit, type );
}


int WX_GRID::GetUnitValue( int aRow, int aCol )
{
    wxString stringValue = GetCellValue( aRow, aCol );

    auto [cellUnits, cellDataType] = getColumnUnits( aCol );

    if( alg::contains( m_autoEvalCols, aCol ) )
    {
        m_eval->SetDefaultUnits( cellUnits );

        if( m_eval->Process( stringValue ) )
            stringValue = m_eval->Result();
    }

    return getUnitsProvider( aCol )->ValueFromString( stringValue, cellDataType );
}


std::optional<int> WX_GRID::GetOptionalUnitValue( int aRow, int aCol )
{
    wxString stringValue = GetCellValue( aRow, aCol );

    auto [cellUnits, cellDataType] = getColumnUnits( aCol );

    if( alg::contains( m_autoEvalCols, aCol ) )
    {
        m_eval->SetDefaultUnits( cellUnits );

        if( stringValue != UNITS_PROVIDER::NullUiString && m_eval->Process( stringValue ) )
            stringValue = m_eval->Result();
    }

    return getUnitsProvider( aCol )->OptionalValueFromString( stringValue, cellDataType );
}


void WX_GRID::SetUnitValue( int aRow, int aCol, int aValue )
{
    EDA_DATA_TYPE cellDataType;

    if( m_autoEvalColsUnits.contains( aCol ) )
        cellDataType = m_autoEvalColsUnits[aCol].second;
    else
        cellDataType = EDA_DATA_TYPE::DISTANCE;

    SetCellValue( aRow, aCol, getUnitsProvider( aCol )->StringFromValue( aValue, true, cellDataType ) );
}


void WX_GRID::SetOptionalUnitValue( int aRow, int aCol, std::optional<int> aValue )
{
    EDA_DATA_TYPE cellDataType;

    if( m_autoEvalColsUnits.contains( aCol ) )
        cellDataType = m_autoEvalColsUnits[aCol].second;
    else
        cellDataType = EDA_DATA_TYPE::DISTANCE;

    SetCellValue( aRow, aCol, getUnitsProvider( aCol )->StringFromOptionalValue( aValue, true, cellDataType ) );
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


std::pair<EDA_UNITS, EDA_DATA_TYPE> WX_GRID::getColumnUnits( const int aCol ) const
{
    if( m_autoEvalColsUnits.contains( aCol ) )
        return { m_autoEvalColsUnits.at( aCol ).first, m_autoEvalColsUnits.at( aCol ).second };

    // Legacy - default always DISTANCE
    return { getUnitsProvider( aCol )->GetUserUnits(), EDA_DATA_TYPE::DISTANCE };
}


void WX_GRID::SetupColumnAutosizer( int aFlexibleCol )
{
    const int colCount = GetNumberCols();

    for( int ii = 0; ii < GetNumberCols(); ++ii )
        m_autosizedCols[ii] = GetColSize( ii );

    m_flexibleCol = aFlexibleCol;

    wxASSERT_MSG( m_flexibleCol < colCount, "Flexible column index does not exist in grid" );

    Bind( wxEVT_UPDATE_UI,
          [this]( wxUpdateUIEvent& aEvent )
          {
              RecomputeGridWidths();
              aEvent.Skip();
          } );

    Bind( wxEVT_SIZE,
          [this]( wxSizeEvent& aEvent )
          {
              onSizeEvent( aEvent );
              aEvent.Skip();
          } );

    // Handles the case when the user changes the cell content to be longer than the current column size
    Bind( wxEVT_GRID_CELL_CHANGED,
          [this]( wxGridEvent& aEvent )
          {
              m_gridWidthsDirty = true;
              aEvent.Skip();
          } );
}


void WX_GRID::RecomputeGridWidths()
{
    if( m_gridWidthsDirty )
    {
        const int width = GetSize().GetX() - wxSystemSettings::GetMetric( wxSYS_VSCROLL_X );

        std::optional<int> flexibleMinWidth;

        for( const auto& [colIndex, minWidth] : m_autosizedCols )
        {
            if( GetColSize( colIndex ) != 0 )
            {
                AutoSizeColumn( colIndex );
                const int colSize = GetColSize( colIndex );

                int minWidthScaled = FromDIP( minWidth );
                SetColSize( colIndex, std::max( minWidthScaled, colSize ) );

                if( colIndex == m_flexibleCol )
                    flexibleMinWidth = minWidthScaled;
            }
        }

        // Gather all the widths except the flexi one
        int nonFlexibleWidth = 0;

        for( int i = 0; i < GetNumberCols(); ++i )
        {
            if( i != m_flexibleCol )
                nonFlexibleWidth += GetColSize( i );
        }

        if( GetColSize( m_flexibleCol ) != 0 )
            SetColSize( m_flexibleCol, std::max( flexibleMinWidth.value_or( 0 ), width - nonFlexibleWidth ) );

        // Store the state for next time
        m_gridWidth = GetSize().GetX();
        m_gridWidthsDirty = false;
    }
}


void WX_GRID::onSizeEvent( wxSizeEvent& aEvent )
{
    const int width = aEvent.GetSize().GetX();

    if( width != m_gridWidth )
        m_gridWidthsDirty = true;
}

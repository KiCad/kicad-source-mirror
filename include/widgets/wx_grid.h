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

#ifndef KICAD_WX_GRID_H
#define KICAD_WX_GRID_H

#include <bitset>
#include <memory>
#include <utility>
#include <vector>

#include <wx/event.h>
#include <wx/grid.h>
#include <wx/version.h>

#include <libeval/numeric_evaluator.h>
#include <units_provider.h>

class wxTextEntryBase;


class WX_GRID_TABLE_BASE : public wxGridTableBase
{
protected:
    wxGridCellAttr* enhanceAttr( wxGridCellAttr* aInputAttr, int aRow, int aCol,
                                 wxGridCellAttr::wxAttrKind aKind  );
};


class WX_GRID : public wxGrid
{
public:
    // Constructor has to be wxFormBuilder-compatible
    WX_GRID( wxWindow *parent, wxWindowID id,
             const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
             long style = wxWANTS_CHARS, const wxString& name = wxGridNameStr );

    ~WX_GRID() override;

    /**
     * Hide wxGrid's SetColLabelSize() method with one which makes sure the size is tall
     * enough for the system GUI font.
     * @param height
     */
    void SetColLabelSize( int aHeight );        // Yes, we're hiding a non-virtual method

    /**
     * Hide wxGrid's SetLabelFont() because for some reason on MSW it's a one-shot and
     * subsequent calls to it have no effect.
     */
    void SetLabelFont( const wxFont& aFont );   // Yes, we're hiding a non-virtual method

    /**
     * Enable alternate row highlighting, where every odd row has a different background
     * color than the even rows.s
     *
     * @param aEnable flag to specify to enable alternate row striping in the grid
     */
    void EnableAlternateRowColors( bool aEnable = true );

    /**
     * Get a tokenized string containing the shown column indexes.
     * Tokens are separated by spaces.
     */
    wxString GetShownColumnsAsString();
    std::bitset<64> GetShownColumns();

    /**
     * Show/hide the grid columns based on a tokenized string of shown column indexes.
     */
    void ShowHideColumns( const wxString& shownColumns );

    /**
     * A more performant version of ShowHideColumns (primarily for OnUpdateUI handlers).
     */
    void ShowHideColumns( const std::bitset<64>& aShownColumns );

    /**
     * Hide wxGrid's SetTable() method with one which doesn't mess up the grid column
     * widths when setting the table.
     */
    void SetTable( wxGridTableBase* table, bool aTakeOwnership = false );

    /**
     * Work-around for a bug in wxGrid which crashes when deleting the table if the
     * cell edit control was not closed.
     */
    void DestroyTable( wxGridTableBase* aTable );

    /**
     * Close any open cell edit controls.
     * @param aQuietMode if true don't send events (ie: for row/col delete operations)
     * @return false if validation failed
     */
    bool CommitPendingChanges( bool aQuietMode = false );
    bool CancelPendingChanges();

    /**
     * Set a UNITS_PROVIDER to enable use of unit- and eval-based Getters.
     * @param aProvider
     */
    void SetUnitsProvider( UNITS_PROVIDER* aProvider, int aCol = 0 );

    void SetAutoEvalCols( const std::vector<int>& aCols ) { m_autoEvalCols = aCols; }

    /**
     * Apply standard KiCad unit and eval services to a numeric cell.
     * @param aRow the cell row index to fetch
     * @param aCol the cell column index to fetch
     * @param aIsOptional if true, indicates to the unit provider the value is optional
     * @return the value held by the cell in internal units
     */
    int GetUnitValue( int aRow, int aCol );


    /**
     * Apply standard KiCad unit and eval services to a numeric cell.
     * @param aRow the cell row index to fetch
     * @param aCol the cell column index to fetch
     * @return the value held by the cell in internal units
     */
    std::optional<int> GetOptionalUnitValue( int aRow, int aCol );

    /**
     * Set a unitized cell's value.
     */
    void SetUnitValue( int aRow, int aCol, int aValue );

    /**
     * Set a unitized cell's optional value
     */
    void SetOptionalUnitValue( int aRow, int aCol, std::optional<int> aValue );

    /**
     * Calculates the specified column based on the actual size of the text
     * on screen.  Will return the maximum value of all calculated widths.
     * @param aCol - Integer value of the column to resize.  Specify -1 for the row labels.
     * @param aHeader - Include the header in the width calculation
     * @param aContents - Include the full contents of the column
     * @param aKeep - Use the current size as a minimum value
     * @return The new size of the column
     */
    int GetVisibleWidth( int aCol, bool aHeader = true, bool aContents = true, bool aKeep = false );

    /**
     * Ensure the height of the row displaying the column labels is enough, even
     * if labels are multiline texts
     */
    void EnsureColLabelsVisible();

    /**
     * WxWidgets has a bunch of bugs in its handling of wxGrid mouse events which close cell
     * editors right after opening them.  Helpfully, it already has a bunch of work-arounds in
     * place (such as the SetInSetFocus() hack), including one to make slow clicks work.  We
     * re-purpose this hack to work-around the bugs when we want to open an editor.
     */
    void ShowEditorOnMouseUp() { m_waitForSlowClick = true; }
    void CancelShowEditorOnMouseUp() { m_waitForSlowClick = false; }

    /**
     * wxWidgets recently added an ASSERT which fires if the position is greater than or equal
     * to the number of rows (even if the delete count is 0).  Needless to say, this makes using
     * DeleteRows for clearing a lot more cumbersome so we add a helper here.
     */
    void ClearRows()
    {
        if( GetNumberRows() )
            DeleteRows( 0, GetNumberRows() );
    }

    /**
     * A helper function to set OS-specific margins for text-based cell editors.
     */
    static void CellEditorSetMargins( wxTextEntryBase* aEntry );

    /**
     * A helper function to tweak sizes of text-based cell editors depending on OS.
     */
    static void CellEditorTransformSizeRect( wxRect& aRect );

    /**
     * Grids that have column sizes automatically set to fill the available width don't want
     * to shrink afterwards (because wxGrid reports the aggregate column size as the bestSize.
     * @param aSize
     */
    void OverrideMinSize( double aXPct, double aYPct )
    {
        wxSize size = DoGetBestSize();
        m_minSizeOverride = wxSize( KiROUND( size.x * aXPct ), KiROUND( size.y * aYPct ) );
    }
    wxSize DoGetBestSize() const override
    {
        if( m_minSizeOverride )
            return m_minSizeOverride.value();
        else
            return wxGrid::DoGetBestSize();
    }

protected:
    /**
     * A re-implementation of wxGrid::DrawColLabel which left-aligns the first column and draws
     * flat borders.
     */
    void DrawColLabel( wxDC& dc, int col ) override;

    /**
     * A re-implementation of wxGrid::DrawRowLabel which draws flat borders.
     */
    void DrawRowLabel( wxDC& dc, int row ) override;

    /**
     * A re-implementation of wxGrid::DrawCornerLabel which draws flat borders.
     */
    void DrawCornerLabel( wxDC& dc ) override;

    void onGridColMove( wxGridEvent& aEvent );
    void onGridCellSelect( wxGridEvent& aEvent );
    void onCellEditorShown( wxGridEvent& aEvent );
    void onCellEditorHidden( wxGridEvent& aEvent );

    void onDPIChanged(wxDPIChangedEvent& event);

protected:
    bool                               m_weOwnTable;

    std::map<int, UNITS_PROVIDER*>     m_unitsProviders;
    std::unique_ptr<NUMERIC_EVALUATOR> m_eval;
    std::vector<int>                   m_autoEvalCols;

    std::map< std::pair<int, int>, std::pair<wxString, wxString> > m_evalBeforeAfter;

    std::optional<wxSize>              m_minSizeOverride;
};

#endif //KICAD_WX_GRID_H

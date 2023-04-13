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

#include <memory>
#include <vector>
#include <memory>
#include <bitset>
#include <wx/event.h>
#include <wx/grid.h>
#include <wx/version.h>
#include <units_provider.h>
#include <libeval/numeric_evaluator.h>


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
     * @return the value held by the cell in internal units
     */
    int GetUnitValue( int aRow, int aCol );

    /**
     * Set a unitized cell's value.
     */
    void SetUnitValue( int aRow, int aCol, int aValue );

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


#if wxCHECK_VERSION( 3, 1, 3 )
    void onDPIChanged(wxDPIChangedEvent& event);
#endif

protected:
    bool                               m_weOwnTable;

    std::map<int, UNITS_PROVIDER*>     m_unitsProviders;
    std::unique_ptr<NUMERIC_EVALUATOR> m_eval;
    std::vector<int>                   m_autoEvalCols;

    std::map< std::pair<int, int>, std::pair<wxString, wxString> > m_evalBeforeAfter;
};

#endif //KICAD_WX_GRID_H

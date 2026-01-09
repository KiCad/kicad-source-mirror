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

#pragma once

#include <bitset>
#include <memory>
#include <utility>
#include <vector>

#include <wx/event.h>
#include <wx/grid.h>
#include <wx/version.h>

#include <kicommon.h>
#include <libeval/numeric_evaluator.h>
#include <units_provider.h>

class wxTextEntryBase;
class ROW_ICON_PROVIDER;


enum KICOMMON_API GROUP_TYPE
{
    GROUP_SINGLETON,
    GROUP_COLLAPSED,
    GROUP_COLLAPSED_DURING_SORT,
    GROUP_EXPANDED,
    CHILD_ITEM
};


class KICOMMON_API WX_GRID_TABLE_BASE : public wxGridTableBase
{
public:
    ~WX_GRID_TABLE_BASE() override
    {
        for( const auto& [col, attr] : m_colAttrs )
            wxSafeDecRef( attr );
    }

    void SetColAttr( wxGridCellAttr* aAttr, int aCol ) override
    {
        wxSafeDecRef( m_colAttrs[aCol] );
        m_colAttrs[aCol] = aAttr;
    }

    wxGridCellAttr* GetAttr( int aRow, int aCol, wxGridCellAttr::wxAttrKind aKind ) override
    {
        if( m_colAttrs[aCol] )
        {
            m_colAttrs[aCol]->IncRef();
            return enhanceAttr( m_colAttrs[aCol], aRow, aCol, aKind );
        }

        return enhanceAttr( nullptr, aRow, aCol, aKind );
    }

    virtual bool IsExpanderColumn( int aCol ) const { return false; }
    virtual GROUP_TYPE GetGroupType( int aRow ) const { return GROUP_SINGLETON; }

    void Clear() override
    {
        if( GetNumberRows() )
            DeleteRows( 0, GetNumberRows() );
    }

protected:
    wxGridCellAttr* enhanceAttr( wxGridCellAttr* aInputAttr, int aRow, int aCol,
                                 wxGridCellAttr::wxAttrKind aKind  );

protected:
    std::map<int, wxGridCellAttr*> m_colAttrs;
};


class KICOMMON_API WX_GRID : public wxGrid
{
public:
    // Constructor has to be wxFormBuilder-compatible
    WX_GRID( wxWindow *parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize, long style = wxWANTS_CHARS,
             const wxString& name = wxGridNameStr );

    ~WX_GRID() override;

    /**
     * Hide wxGrid's SetColLabelSize() method with one which makes sure the size is tall
     * enough for the system GUI font.
     *
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
     * color than the even rows.
     *
     * @param aEnable flag to specify to enable alternate row striping in the grid.
     */
    void EnableAlternateRowColors( bool aEnable = true );

    /**
     * Get a tokenized string containing the shown column indexes.
     *
     * Tokens are separated by spaces.
     */
    wxString GetShownColumnsAsString();
    std::bitset<64> GetShownColumns();

    /**
     * Show/hide the grid columns based on a tokenized string of shown column indexes.
     */
    void ShowHideColumns( const wxString& shownColumns );

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
     *
     * @param aQuietMode if true don't send events (ie: for row/col delete operations).
     * @return false if validation failed.
     */
    bool CommitPendingChanges( bool aQuietMode = false );
    bool CancelPendingChanges();

    void OnAddRow( const std::function<std::pair<int, int>()>& aAdder );

    /**
     * Handles a row deletion event.  This is a bit tricky due to the potential for stale
     * selections, so we code it only once here.
     */
    void OnDeleteRows( const std::function<void( int row )>& aDeleter );

    void OnDeleteRows( const std::function<bool( int row )>& aFilter,
                       const std::function<void( int row )>& aDeleter );

    /**
     * These aren't that tricky, but might as well share code.
     */
    void SwapRows( int aRowA, int aRowB );
    void OnMoveRowUp( const std::function<void( int row )>& aMover );
    void OnMoveRowDown( const std::function<void( int row )>& aMover );
    void OnMoveRowUp( const std::function<bool( int row )>& aFilter,
                      const std::function<void( int row )>& aMover );
    void OnMoveRowDown( const std::function<bool( int row )>& aFilter,
                        const std::function<void( int row )>& aMover );

    /**
     * Set a EUNITS_PROVIDER to enable use of unit- and eval-based Getters.
     *
     * @param aProvider
     */
    void SetUnitsProvider( UNITS_PROVIDER* aProvider, int aCol = 0 );

    void SetAutoEvalCols( const std::vector<int>& aCols ) { m_autoEvalCols = aCols; }

    /**
     * Set the unit and unit data type to use for a given column
     */
    void SetAutoEvalColUnits( int col, EDA_UNITS aUnit, EDA_DATA_TYPE aUnitType );

    /**
     * Set the unit to use for a given column. The unit data type is inferred from the unit type
     */
    void SetAutoEvalColUnits( int col, EDA_UNITS aUnit );

    /**
     * Apply standard KiCad unit and eval services to a numeric cell.
     *
     * @param aRow the cell row index to fetch.
     * @param aCol the cell column index to fetch.
     * @param aIsOptional if true, indicates to the unit provider the value is optional.
     * @return the value held by the cell in internal units.
     */
    int GetUnitValue( int aRow, int aCol );

    /**
     * Apply standard KiCad unit and eval services to a numeric cell.
     *
     * @param aRow the cell row index to fetch.
     * @param aCol the cell column index to fetch.
     * @return the value held by the cell in internal units.
     */
    std::optional<int> GetOptionalUnitValue( int aRow, int aCol );

    /**
     * Set a unitized cell's value.
     */
    void SetUnitValue( int aRow, int aCol, int aValue );

    /**
     * Set a unitized cell's optional value.
     */
    void SetOptionalUnitValue( int aRow, int aCol, std::optional<int> aValue );

    /**
     * Calculate the specified column based on the actual size of the text on screen.
     *
     * @param aCol is the index of the column to resize.  Specify -1 for the row labels.
     * @param aHeader is the header in the width calculation.
     * @param aContents is the full contents of the column.
     * @param aKeep is the current size as a minimum value.
     * @return The maximum value of all calculated widths.
     */
    int GetVisibleWidth( int aCol, bool aHeader = true, bool aContents = true, bool aKeep = false );

    /**
     * Ensure the height of the row displaying the column labels is enough, even
     * if labels are multiline texts.
     */
    void EnsureColLabelsVisible();

    /**
     * WxWidgets has a bunch of bugs in its handling of wxGrid mouse events which close cell
     * editors right after opening them.
     *
     * Helpfully, it already has a bunch of work-arounds in place (such as the SetInSetFocus()
     * hack), including one to make slow clicks work.  We re-purpose this hack to work-around
     * the bugs when we want to open an editor.
     */
    void ShowEditorOnMouseUp() { m_waitForSlowClick = true; }
    void CancelShowEditorOnMouseUp() { m_waitForSlowClick = false; }

    /**
     * wxWidgets recently added an ASSERT which fires if the position is greater than or equal
     * to the number of rows (even if the delete count is 0).  Needless to say, this makes using
     * DeleteRows for clearing a lot more cumbersome so we add a helper here.
     */
    void ClearRows( bool aUpdateLabels = true )
    {
        if( GetNumberRows() > 0 )
            DeleteRows( 0, GetNumberRows(), aUpdateLabels );
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
     *
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

    /**
     * Set autosize behaviour using wxFormBuilder column widths as minimums, with a single specified
     * growable column.
     */
    void SetupColumnAutosizer( int aFlexibleCol );

    void SetGridWidthsDirty() { m_gridWidthsDirty = true; }

    ROW_ICON_PROVIDER* GetRowIconProvider() const { return m_rowIconProvider; }

    void RecomputeGridWidths();

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

    UNITS_PROVIDER* getUnitsProvider( int aCol ) const
    {
        if( m_unitsProviders.contains( aCol ) )
            return m_unitsProviders.at( aCol );
        else
            return m_unitsProviders.begin()->second;
    }

    /**
     * Returns the units and data type associated with a given column
     */
    std::pair<EDA_UNITS, EDA_DATA_TYPE> getColumnUnits( int aCol ) const;

private:
    void onSizeEvent( wxSizeEvent& aEvent );

protected:
    bool                       m_weOwnTable;

    std::map<int, UNITS_PROVIDER*>                                 m_unitsProviders;
    std::unique_ptr<NUMERIC_EVALUATOR>                             m_eval;
    std::vector<int>                                               m_autoEvalCols;
    std::unordered_map<int, std::pair<EDA_UNITS, EDA_DATA_TYPE>>   m_autoEvalColsUnits;
    std::map< std::pair<int, int>, std::pair<wxString, wxString> > m_evalBeforeAfter;

    std::optional<wxSize>      m_minSizeOverride;

    std::map<int, int>         m_autosizedCols;     // map of col : min_width
    int                        m_flexibleCol;

    bool                       m_gridWidthsDirty = true;
    int                        m_gridWidth = 0;

    ROW_ICON_PROVIDER*         m_rowIconProvider;
};

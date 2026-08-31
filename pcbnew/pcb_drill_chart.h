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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PCB_DRILL_CHART_H
#define PCB_DRILL_CHART_H

#include <algorithm>
#include <map>

#include <drill/drill_chart_model.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_span.h>
#include <pcb_table.h>


/**
 * Bring every chart on the board up to date.
 *
 * A chart is derived data. Anything that reads one calls this first, so a chart can never
 * show a board that has moved on. Charts already built at the current drill generation cost
 * one integer comparison each.
 */
void RefreshDrillCharts( BOARD& aBoard );


/**
 * A drill chart placed on the board, kept in step with the holes.
 *
 * The chart owns a materialized copy of its columns rather than pointing at a template, so
 * importing or editing a template can never rewrite a fabrication drawing that has already
 * been approved. Grouping and symbols come from the board's shared symbol profile so that a
 * chart and the drill map beside it cannot disagree.
 */
class PCB_DRILL_CHART : public PCB_TABLE
{
public:
    PCB_DRILL_CHART( BOARD_ITEM* aParent );

    PCB_DRILL_CHART( const PCB_DRILL_CHART& aOther );

    ~PCB_DRILL_CHART() override = default;

    PCB_DRILL_CHART& operator=( const PCB_DRILL_CHART& ) = delete;

    static inline bool ClassOf( const EDA_ITEM* aItem )
    {
        return aItem && PCB_DRILL_CHART_T == aItem->Type();
    }

    wxString GetClass() const override { return wxT( "PCB_DRILL_CHART" ); }

    EDA_ITEM* Clone() const override { return new PCB_DRILL_CHART( *this ); }

    DRILL_CHART_FILTER& Filter() { return m_filter; }
    const DRILL_CHART_FILTER& Filter() const { return m_filter; }

    std::vector<DRILL_CHART_COLUMN>& Columns() { return m_columns; }
    const std::vector<DRILL_CHART_COLUMN>& Columns() const { return m_columns; }

    DRILL_CHART_UNITS GetUnits() const { return m_units; }
    void SetUnits( DRILL_CHART_UNITS aUnits ) { m_units = aUnits; }

    int GetPrecision() const { return m_precision; }

    /**
     * Clamped. The value reaches a "%.*f" format, where a hostile file asking for two
     * billion decimals would try to allocate gigabytes.
     */
    void SetPrecision( int aPrecision ) { m_precision = std::clamp( aPrecision, 0, 6 ); }

    bool GetShowTotals() const { return m_showTotals; }
    void SetShowTotals( bool aShow ) { m_showTotals = aShow; }

    /**
     * Board drill generation the cells were built at. A chart is derived data, so anything
     * that reads it refreshes it first and this is the cheap test for whether it has to.
     */
    uint64_t GetBuiltGeneration() const { return m_builtGeneration; }

    /**
     * Copy a template's formatting. Called when a chart is placed, never afterwards.
     */
    void ApplyTemplate( const DRILL_CHART_TEMPLATE& aTemplate );


    /**
     * Regenerate the cells from the board.
     *
     * Cells are reused rather than recreated so their UUIDs survive, which keeps selection
     * restore and file diffs meaningful. Symbol assignments are worked out on a copy of the
     * board's profile. AAssignedProfile receives it so the caller can commit it as an
     * undoable board change once placement has actually happened. Passing nullptr discards
     * new assignments, which is what a preview wants.
     */
    void RebuildCells( const BOARD& aBoard, DRILL_SYMBOL_PROFILE* aAssignedProfile = nullptr );

    /**
     * Curated shape index for each generated row, by table row.
     *
     * A shape mark has no text, so without this the symbol column of every default chart is
     * blank. Part of the generated snapshot, so it is serialized with the cells rather than
     * recomputed, which would let a stale chart silently re-symbolize itself.
     */
    const std::map<int, int>& RowShapes() const { return m_rowShapes; }
    std::map<int, int>& RowShapes() { return m_rowShapes; }

    /**
     * Drill group each generated row reports on, by table row.
     *
     * A rebuild reorders rows as holes come and go, so this is what lets a row's formatting
     * follow its group rather than its position. Serialized, because the first rebuild after
     * a load would otherwise have nothing to match the loaded rows against.
     */
    const std::map<int, std::string>& RowKeys() const { return m_rowKeys; }
    std::map<int, std::string>& RowKeys() { return m_rowKeys; }

    /**
     * True for a row that reports a drill group, false for the title, heading and totals.
     */
    bool IsDataRow( int aRow ) const;

    /**
     * Table column the symbol is drawn in, or -1 when the chart has no symbol column.
     */
    int GetSymbolColumn() const { return m_symbolColumn; }
    void SetSymbolColumn( int aCol ) { m_symbolColumn = aCol; }

    INSPECT_RESULT Visit( INSPECTOR inspector, void* testData,
                          const std::vector<KICAD_T>& aScanTypes ) override;

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const BOARD_ITEM& aOther ) const override;

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

protected:
    void swapData( BOARD_ITEM* aImage ) override;

private:
    /**
     * The board data this chart reports on, as chart rows.
     */
    std::vector<DRILL_CHART_GROUP> buildGroups( const BOARD& aBoard ) const;

    /**
     * Move each row's cells to the row that reports the same drill group.
     *
     * Formatting lives on the cells, so without this a rebuild that adds or removes a group
     * would hand a row's formatting to whichever group happened to land on its index. Rows
     * whose group has gone are deleted and rows for a new group are created, leaving the
     * table exactly aRows by aCols.
     */
    void migrateRows( int aRows, int aCols, int aFirstDataRow,
                      const std::vector<std::string>& aNewRowKeys );

    DRILL_CHART_FILTER m_filter;

    std::vector<DRILL_CHART_COLUMN> m_columns;

    DRILL_CHART_UNITS m_units;
    int               m_precision;
    bool              m_showTotals;

    std::map<int, int>         m_rowShapes;
    std::map<int, std::string> m_rowKeys;
    int                        m_symbolColumn;

    uint64_t m_builtGeneration;
};

#endif // PCB_DRILL_CHART_H

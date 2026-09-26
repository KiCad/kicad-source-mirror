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

#include <map>
#include <optional>
#include <vector>

#include <board_tables/generated_table_refresh.h>
#include <drill/drill_chart_model.h>
#include <drill/drill_chart_template.h>
#include <drill/drill_span.h>
#include <drill/drill_symbol_profile.h>
#include <pcb_generated_table.h>


/**
 * A drill chart's rows, with the symbol each one is drawn with.
 */
struct DRILL_CHART_CONTENT : GENERATED_TABLE_CONTENT
{
    std::vector<std::optional<int>> m_RowShapes; ///< Per m_Rows entry, empty when the mark is not a shape
    int                             m_SymbolColumn = -1;
};


/**
 * The board's drill symbol profile as a refresh assigns new symbols into it.
 *
 * Seeded from the board, not default-constructed. Assignment builds on whatever it is handed,
 * so a default would drop the board's grouping and its existing marks.
 */
struct DRILL_PROFILE_PENDING : GENERATED_TABLE_PENDING
{
    explicit DRILL_PROFILE_PENDING( BOARD& aBoard );

    void Commit( BOARD& aBoard ) override;

    DRILL_SYMBOL_PROFILE m_Profile;
};


/**
 * A drill chart placed on the board, kept in step with the holes.
 *
 * Grouping and symbols come from the board's shared symbol profile so that a chart and the
 * drill map beside it cannot disagree.
 */
class PCB_DRILL_CHART : public PCB_GENERATED_TABLE
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

    const GENERATED_TABLE_SCHEMA& Schema() const override { return DrillChartSchema(); }

    DRILL_CHART_FILTER& Filter() { return m_filter; }
    const DRILL_CHART_FILTER& Filter() const { return m_filter; }

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
     * One integer comparison, so a board with nothing drill related changed pays nothing.
     */
    bool IsStale( const BOARD& aBoard ) const override;

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
     * True for a row that reports a drill group, false for the title, heading and totals.
     */
    bool IsDataRow( int aRow ) const override;

    /**
     * Table column the symbol is drawn in, or -1 when the chart has no symbol column.
     */
    int GetSymbolColumn() const { return m_symbolColumn; }
    void SetSymbolColumn( int aCol ) { m_symbolColumn = aCol; }

    wxString GetItemDescription( UNITS_PROVIDER* aUnitsProvider, bool aFull ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    bool operator==( const BOARD_ITEM& aOther ) const override;

    void Serialize( google::protobuf::Any& aContainer ) const override;
    bool Deserialize( const google::protobuf::Any& aContainer ) override;

protected:
    std::unique_ptr<GENERATED_TABLE_CONTENT> generate( const BOARD&             aBoard,
                                                       GENERATED_TABLE_REFRESH* aRefresh ) const override;

    void onRebuilt( const BOARD& aBoard, const GENERATED_TABLE_CONTENT& aContent ) override;

    void swapData( BOARD_ITEM* aImage ) override;

private:
    /**
     * The board data this chart reports on, as chart rows grouped the way aProfile says.
     */
    std::vector<DRILL_CHART_GROUP> buildGroups( const BOARD& aBoard, const DRILL_SYMBOL_PROFILE& aProfile ) const;

    /**
     * The typed id a column at this index holds, so generate's switch stays exhaustive
     * even though a stored column only carries the schema's plain int.
     */
    DRILL_CHART_COLUMN_ID columnId( int aCol ) const;

    DRILL_CHART_FILTER m_filter;

    bool m_showTotals;

    std::map<int, int> m_rowShapes;
    int                m_symbolColumn;

    uint64_t m_builtGeneration;
};

#endif // PCB_DRILL_CHART_H

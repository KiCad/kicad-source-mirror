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

#ifndef PCB_GENERATED_TABLE_H
#define PCB_GENERATED_TABLE_H

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <board_tables/generated_table_schema.h>
#include <pcb_table.h>

class GENERATED_TABLE_REFRESH;


/**
 * One data row of a generated table, and the identity its formatting follows across rebuilds.
 */
struct GENERATED_TABLE_ROW
{
    std::string           m_Key;
    std::vector<wxString> m_Cells;
};


/**
 * What a generated table reports, one entry per cell below the heading row.
 */
struct GENERATED_TABLE_CONTENT
{
    virtual ~GENERATED_TABLE_CONTENT() = default;

    std::vector<GENERATED_TABLE_ROW>   m_Rows;    ///< Keyed, below the heading row
    std::vector<std::vector<wxString>> m_Trailer; ///< Unkeyed rows below the data, e.g. totals
};


/**
 * A table whose cell text is derived from the board and rebuilt whenever it goes stale.
 *
 * The table owns a materialized copy of its columns rather than pointing at a template, so
 * importing or editing a template can never rewrite a drawing that has already been approved.
 * Subclasses say what the rows are. Laying them out, keeping each row's formatting with the
 * row that still reports the same thing, and holding the table's position are done here.
 */
class PCB_GENERATED_TABLE : public PCB_TABLE
{
public:
    virtual const GENERATED_TABLE_SCHEMA& Schema() const = 0;

    std::vector<GENERATED_TABLE_COLUMN>&       Columns() { return m_columns; }
    const std::vector<GENERATED_TABLE_COLUMN>& Columns() const { return m_columns; }

    GENERATED_TABLE_UNITS GetUnits() const { return m_units; }
    void                  SetUnits( GENERATED_TABLE_UNITS aUnits ) { m_units = aUnits; }

    int GetPrecision() const { return m_precision; }

    /**
     * Clamped. The value reaches a "%.*f" format, where a hostile file asking for two
     * billion decimals would try to allocate gigabytes.
     */
    void SetPrecision( int aPrecision ) { m_precision = std::clamp( aPrecision, 0, 6 ); }

    /**
     * What each generated row reports on, by table row.
     *
     * A rebuild reorders rows as the board changes, so this is what lets a row's formatting
     * follow what it reports rather than its position. Serialized, because the first rebuild
     * after a load would otherwise have nothing to match the loaded rows against.
     */
    const std::map<int, std::string>& RowKeys() const { return m_rowKeys; }
    std::map<int, std::string>&       RowKeys() { return m_rowKeys; }

    /**
     * True when the cells no longer say what the board says. The default compares freshly
     * generated text with the cells, so a subclass with a cheaper test should override it.
     */
    virtual bool IsStale( const BOARD& aBoard ) const;

    /**
     * Regenerate the cells from the board.
     *
     * Cells are reused rather than recreated so their UUIDs survive, which keeps selection
     * restore and file diffs meaningful. Board state the rebuild wants to change is left in
     * aRefresh for the caller to commit once the table has actually been placed. A null
     * aRefresh is a preview and leaves no pending board state behind.
     */
    void RebuildCells( const BOARD& aBoard, GENERATED_TABLE_REFRESH* aRefresh = nullptr );

    /**
     * True for a row that reports on the board, false for the heading and trailer rows.
     */
    virtual bool IsDataRow( int aRow ) const { return m_rowKeys.count( aRow ) > 0; }

    INSPECT_RESULT Visit( INSPECTOR aInspector, void* aTestData, const std::vector<KICAD_T>& aScanTypes ) override;

protected:
    PCB_GENERATED_TABLE( BOARD_ITEM* aParent, KICAD_T aType, int aLineWidth );

    PCB_GENERATED_TABLE( const PCB_GENERATED_TABLE& aOther ) = default;

    /**
     * The rows this table reports. A null aRefresh must leave no pending board state behind.
     */
    virtual std::unique_ptr<GENERATED_TABLE_CONTENT> generate( const BOARD&             aBoard,
                                                               GENERATED_TABLE_REFRESH* aRefresh ) const = 0;

    /**
     * Called once the cells are laid out, with the content they were built from.
     */
    virtual void onRebuilt( const BOARD& aBoard, const GENERATED_TABLE_CONTENT& aContent ) {}

    /**
     * Called for each cell a rebuild creates, before its text is set.
     */
    virtual void styleNewCell( PCB_TABLECELL* aCell, bool aIsHeading ) {}

    void swapData( BOARD_ITEM* aImage ) override;

    /**
     * Equality of the members this class owns and of the underlying PCB_TABLE.
     */
    bool generatedEquals( const PCB_GENERATED_TABLE& aOther ) const;

    std::vector<GENERATED_TABLE_COLUMN> m_columns;
    GENERATED_TABLE_UNITS               m_units;
    int                                 m_precision;
    std::map<int, std::string>          m_rowKeys;

private:
    /**
     * Move each row's cells to the row that reports the same thing.
     *
     * Formatting lives on the cells, so without this a rebuild that adds or removes a row
     * would hand a row's formatting to whichever row happened to land on its index. Rows
     * whose key has gone are deleted and rows for a new key are created, leaving the table
     * exactly aRows by aCols.
     */
    void migrateRows( int aRows, int aCols, const std::vector<std::string>& aNewRowKeys );
};

#endif // PCB_GENERATED_TABLE_H

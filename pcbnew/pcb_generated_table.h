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
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <board_tables/generated_table_schema.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>

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

    /**
     * The columns the table shows, which are the columns of its cells. Edit an entry in place
     * if need be, but add, remove or hide one through SetColumns.
     */
    std::vector<GENERATED_TABLE_COLUMN>&       Columns() { return m_columns; }
    const std::vector<GENERATED_TABLE_COLUMN>& Columns() const { return m_columns; }

    /**
     * Every column the table has been given, in order, with hidden ones marked by m_Visible.
     */
    std::vector<GENERATED_TABLE_COLUMN> AllColumns() const;

    /**
     * Replace the columns of a built table, moving each column's cells to where its id now sits
     * so their formatting goes with them. A hidden column keeps its place; showing it again
     * creates cells formatted from their rows. Call RebuildCells after.
     */
    void SetColumns( std::vector<GENERATED_TABLE_COLUMN> aColumns );

    /**
     * Replace the columns without touching any cell, for a loader whose cells already match
     * the visible ones.
     */
    void RestoreColumns( const std::vector<GENERATED_TABLE_COLUMN>& aColumns );

    GENERATED_TABLE_UNITS GetUnits() const { return m_units; }
    void                  SetUnits( GENERATED_TABLE_UNITS aUnits ) { m_units = aUnits; }

    int GetPrecision() const { return m_precision; }

    /**
     * Clamped. The value reaches a "%.*f" format, where a hostile file asking for two
     * billion decimals would try to allocate gigabytes.
     */
    void SetPrecision( int aPrecision ) { m_precision = std::clamp( aPrecision, 0, 6 ); }

    /**
     * The first data row's height, with RowHeightsDiffer saying whether it speaks for every data
     * row. Setting it sets every data row, each no shorter than its text. Heading and trailer rows
     * keep heights of their own.
     */
    int  GetRowHeightProp() const;
    void SetRowHeightProp( int aHeight );
    bool RowHeightsDiffer() const;

    /**
     * Set one row's height, no shorter than its text. Margins are the default, not a minimum.
     */
    void SetRowHeightToFit( int aRow, int aHeight );

    /**
     * The first cell's font, with FontNamesDiffer saying whether it speaks for every shown cell.
     */
    wxString GetFontNameProp() const;
    void     SetFontNameProp( const wxString& aFontName );
    bool     FontNamesDiffer() const;

    /**
     * The first cell's text height, with FontSizesDiffer saying whether every shown cell has
     * that size, square.
     */
    int  GetFontSizeProp() const;
    void SetFontSizeProp( int aSize );
    bool FontSizesDiffer() const;

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
     *
     * A table loaded without row keys has only position to go on, so every row below the
     * heading counts as data. A kind with trailer rows overrides this to tell them apart.
     */
    virtual bool IsDataRow( int aRow ) const
    {
        if( m_rowKeys.empty() )
            return aRow >= 1 && aRow < GetRowCount();

        return m_rowKeys.count( aRow ) > 0;
    }

    INSPECT_RESULT Visit( INSPECTOR aInspector, void* aTestData, const std::vector<KICAD_T>& aScanTypes ) override;

    double Similarity( const BOARD_ITEM& aOther ) const override;

    void GetMsgPanelInfo( EDA_DRAW_FRAME* aFrame, std::vector<MSG_PANEL_ITEM>& aList ) override;

    /**
     * PCB_TABLE::Flip reverses the columns in either direction, as it does for a plain table's
     * text, so the column set is reversed with them. Mirror goes through here too.
     */
    void Flip( const VECTOR2I& aCentre, FLIP_DIRECTION aFlipDirection ) override;

    /**
     * Copy everything a table of this kind holds, including what a subclass adds.
     */
    void CopyFrom( const BOARD_ITEM* aOther ) override;

    /**
     * Take \a aOther's settings and placement but keep this table's cells, so formatting applied
     * to them survives. Columns go through SetColumns, so call RebuildCells after.
     */
    virtual void ApplySettings( const PCB_GENERATED_TABLE& aOther );

protected:
    PCB_GENERATED_TABLE( BOARD_ITEM* aParent, KICAD_T aType, int aLineWidth );

    PCB_GENERATED_TABLE( const PCB_GENERATED_TABLE& aOther );

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
     * Called once a rebuild has moved the rows to where the new content puts them, before
     * anything asks which rows are data rows under the new layout.
     */
    virtual void onRowsMigrated() {}

    /**
     * Called for each cell a rebuild creates, before its text is set.
     */
    virtual void styleNewCell( PCB_TABLECELL* aCell, bool aIsHeading ) {}

    /**
     * Called once the columns have been reversed, for any column index a subclass keeps.
     */
    virtual void onColumnsReversed() {}

    void swapData( BOARD_ITEM* aImage ) override;

    /**
     * Row heights are the user's, carried by key through migrateRows. Every row keeps the height
     * it held while its text still fits inside it, and otherwise takes what its text needs with
     * margins. A data row that held none takes the height most data rows held, or when none held
     * one, what the tallest data row's text needs with margins.
     */
    void setAutosizedRowHeights( const std::vector<int>& aNeeded ) override;

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
     * A hidden column and its place among all columns.
     */
    struct HIDDEN_COLUMN
    {
        GENERATED_TABLE_COLUMN m_Column;
        int                    m_Position = 0; ///< In AllColumns()

        bool operator==( const HIDDEN_COLUMN& aOther ) const = default;
    };

    enum class CELL_ORIGIN
    {
        EMPTY,    ///< Nothing filled the slot yet
        MOVED,    ///< A shown column's cell, carried to its new column
        CREATED   ///< New, including a cell in a column shown again
    };

    /**
     * SetColumns' work in progress, laying the old cells out for the new columns.
     */
    struct COLUMN_REMAP
    {
        int                         m_Rows = 0;
        int                         m_OldCols = 0;
        int                         m_NewCols = 0;
        std::vector<PCB_TABLECELL*> m_Cells;     ///< The new layout, row by row
        std::vector<CELL_ORIGIN>    m_Origin;    ///< Where each of m_Cells came from
        std::vector<bool>           m_Carried;   ///< Old cells that went somewhere, by old index
        std::map<int, int>          m_ColWidths; ///< By new column
    };

    /**
     * Carry each shown column's cells and width to its new position.
     */
    void moveShownCells( const std::vector<GENERATED_TABLE_COLUMN>& aShown, COLUMN_REMAP& aRemap ) const;

    /**
     * Put every cell in the geometry its slot had, so the table stays where it was, creating
     * those nothing filled.
     */
    void placeCells( COLUMN_REMAP& aRemap );

    /**
     * Style cells placeCells created from another cell in their row.
     */
    void styleCreatedCells( const std::vector<CELL_ORIGIN>& aOrigin );

    /**
     * The rows below the heading that IsDataRow accepts, in order.
     */
    std::vector<int> dataRows() const;

    /**
     * The height \a aRow needs by its cells' line count, line spacing, pen width and margins. Unlike
     * a glyph outline, it is the same for any text in the same font, size and line count.
     */
    int metricRowHeight( int aRow ) const;

    /**
     * The smallest height \a aRow may take with its text still inside it. Centred text needs no
     * margin, while top or bottom aligned text keeps the margin it is set in by.
     */
    int minRowHeight( int aRow ) const;

    /**
     * The nearest data cell in \a aCol among \a aStyled, or null when the column has none.
     */
    const PCB_TABLECELL* nearestDataCell( int aRow, int aCol, const std::set<KIID>& aStyled ) const;

    /**
     * Give a new cell the text formatting of the nearest data cell in its column among
     * \a aStyled, or the kind's defaults when there is none or the cell is a heading.
     */
    void styleFromColumn( PCB_TABLECELL* aCell, int aRow, int aCol, const std::set<KIID>& aStyled );

    /** Visit every shown cell. */
    void forEachCell( const std::function<void( PCB_TABLECELL* )>& aFunc );

    /**
     * True when \a aTest holds for any cell forEachCell would visit, stopping at the first.
     */
    bool anyCell( const std::function<bool( const PCB_TABLECELL* )>& aTest ) const;

    /**
     * The first cell forEachCell visits, which the font getters speak for, or null.
     */
    const PCB_TABLECELL* firstCell() const;

    /**
     * Move each row's cells to the row that reports the same thing.
     *
     * Formatting lives on the cells, so without this a rebuild that adds or removes a row
     * would hand a row's formatting to whichever row happened to land on its index. Rows
     * whose key has gone are deleted and rows for a new key are created, leaving the table
     * exactly aRows by aCols.
     *
     * A table without row keys has only position to go on for its data rows, and its trailers
     * still follow their role.
     */
    void migrateRows( int aRows, int aCols, const std::vector<std::string>& aNewRowKeys );

    std::vector<HIDDEN_COLUMN> m_hidden;    ///< By position
};

#endif // PCB_GENERATED_TABLE_H

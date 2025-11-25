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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef EDIT_TABLE_TOOL_BASE_H
#define EDIT_TABLE_TOOL_BASE_H

#include <tool/tool_base.h>
#include <tool/selection.h>
#include <tool/actions.h>
#include <wx/translation.h>

class BASE_SCREEN;


/**
 * SCH_TABLE_EDIT_TOOL and PCB_TABLE_EDIT_TOOL share most of their algorithms, which are
 * implemented here.
 */


template<typename T_TABLE, typename T_TABLECELL, typename T_COMMIT>
class EDIT_TABLE_TOOL_BASE
{
protected:
    void addMenus( CONDITIONAL_MENU& selToolMenu )
    {
        auto cellSelection = SELECTION_CONDITIONS::MoreThan( 0 )
                             && SELECTION_CONDITIONS::OnlyTypes( { SCH_TABLECELL_T, PCB_TABLECELL_T } );

        auto cellBlockSelection = []( const SELECTION& sel )
        {
            if( sel.Size() < 2 )
                return false;

            int colMin = std::numeric_limits<int>::max();
            int colMax = 0;
            int rowMin = std::numeric_limits<int>::max();
            int rowMax = 0;
            int selectedArea = 0;

            for( EDA_ITEM* item : sel )
            {
                if( T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item ) )
                {
                    colMin = std::min( colMin, cell->GetColumn() );
                    colMax = std::max( colMax, cell->GetColumn() + cell->GetColSpan() );
                    rowMin = std::min( rowMin, cell->GetRow() );
                    rowMax = std::max( rowMax, cell->GetRow() + cell->GetRowSpan() );

                    selectedArea += cell->GetColSpan() * cell->GetRowSpan();
                }
            }

            return selectedArea == ( colMax - colMin ) * ( rowMax - rowMin );
        };

        auto mergedCellsSelection = []( const SELECTION& sel )
        {
            for( EDA_ITEM* item : sel )
            {
                if( T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item ) )
                {
                    if( cell->GetColSpan() > 1 || cell->GetRowSpan() > 1 )
                        return true;
                }
            }

            return false;
        };

        //
        // Add editing actions to the selection tool menu
        //
        selToolMenu.AddSeparator( 100 );
        selToolMenu.AddItem( ACTIONS::addRowAbove,   cellSelection && SELECTION_CONDITIONS::Idle, 100 );
        selToolMenu.AddItem( ACTIONS::addRowBelow,   cellSelection && SELECTION_CONDITIONS::Idle, 100 );
        selToolMenu.AddItem( ACTIONS::addColBefore,  cellSelection && SELECTION_CONDITIONS::Idle, 100 );
        selToolMenu.AddItem( ACTIONS::addColAfter,   cellSelection && SELECTION_CONDITIONS::Idle, 100 );

        selToolMenu.AddSeparator( 100 );
        selToolMenu.AddItem( ACTIONS::deleteRows,    cellSelection && SELECTION_CONDITIONS::Idle, 100 );
        selToolMenu.AddItem( ACTIONS::deleteColumns, cellSelection && SELECTION_CONDITIONS::Idle, 100 );

        selToolMenu.AddSeparator( 100 );
        selToolMenu.AddItem( ACTIONS::mergeCells,    cellSelection && cellBlockSelection, 100 );
        selToolMenu.AddItem( ACTIONS::unmergeCells,  cellSelection && mergedCellsSelection, 100 );

        selToolMenu.AddSeparator( 100 );
        selToolMenu.AddItem( ACTIONS::editTable,     cellSelection && SELECTION_CONDITIONS::Idle, 100 );

        selToolMenu.AddSeparator( 100 );
        selToolMenu.AddItem( ACTIONS::exportTableCSV, cellSelection && SELECTION_CONDITIONS::Idle, 100 );

        selToolMenu.AddSeparator( 100 );
    }

    int doAddRowAbove( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();
        T_TABLECELL*     topmost = nullptr;

        for( EDA_ITEM* item : selection )
        {
            T_TABLECELL* cell = static_cast<T_TABLECELL*>( item );

            if( !topmost || cell->GetRow() < topmost->GetRow() )
                topmost = cell;
        }

        if( !topmost )
            return 0;

        int      row = topmost->GetRow();
        T_TABLE* table = static_cast<T_TABLE*>( topmost->GetParent() );
        T_COMMIT commit( getToolMgr() );
        VECTOR2I pos = table->GetPosition();

        // Make a copy of the source row before things start moving around
        std::vector<T_TABLECELL*> sources;
        sources.reserve( table->GetColCount() );

        for( int col = 0; col < table->GetColCount(); ++col )
            sources.push_back( table->GetCell( row, col ) );

        commit.Modify( table, getScreen() );

        for( int col = 0; col < table->GetColCount(); ++col )
        {
            T_TABLECELL* cell = copyCell( sources[col] );
            table->InsertCell( row * table->GetColCount(), cell );
        }

        for( int afterRow = table->GetRowCount() - 1; afterRow > row; afterRow-- )
            table->SetRowHeight( afterRow, table->GetRowHeight( afterRow - 1 ) );

        table->SetPosition( pos );
        table->Normalize();

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        commit.Push( _( "Add Row Above" ) );

        return 0;
    }

    int doAddRowBelow( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();
        T_TABLECELL*     bottommost = nullptr;

        if( selection.Empty() )
            return 0;

        for( EDA_ITEM* item : selection )
        {
            T_TABLECELL* cell = static_cast<T_TABLECELL*>( item );

            if( !bottommost || cell->GetRow() > bottommost->GetRow() )
                bottommost = cell;
        }

        if( !bottommost )
            return 0;

        int      row = bottommost->GetRow();
        T_TABLE* table = static_cast<T_TABLE*>( bottommost->GetParent() );
        T_COMMIT commit( getToolMgr() );
        VECTOR2I pos = table->GetPosition();

        // Make a copy of the source row before things start moving around
        std::vector<T_TABLECELL*> sources;
        sources.reserve( table->GetColCount() );

        for( int col = 0; col < table->GetColCount(); ++col )
            sources.push_back( table->GetCell( row, col ) );

        commit.Modify( table, getScreen() );

        for( int col = 0; col < table->GetColCount(); ++col )
        {
            T_TABLECELL* cell = copyCell( sources[col] );
            table->InsertCell( ( row + 1 ) * table->GetColCount(), cell );
        }

        for( int afterRow = table->GetRowCount() - 1; afterRow > row; afterRow-- )
            table->SetRowHeight( afterRow, table->GetRowHeight( afterRow - 1 ) );

        table->SetPosition( pos );
        table->Normalize();

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        commit.Push( _( "Add Row Below" ) );

        return 0;
    }

    int doAddColumnBefore( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();
        T_TABLECELL*     leftmost = nullptr;

        for( EDA_ITEM* item : selection )
        {
            T_TABLECELL* cell = static_cast<T_TABLECELL*>( item );

            if( !leftmost || cell->GetColumn() < leftmost->GetColumn() )
                leftmost = cell;
        }

        if( !leftmost )
            return 0;

        int      col = leftmost->GetColumn();
        T_TABLE* table = static_cast<T_TABLE*>( leftmost->GetParent() );
        int      rowCount = table->GetRowCount();
        T_COMMIT commit( getToolMgr() );
        VECTOR2I pos = table->GetPosition();

        // Make a copy of the source column before things start moving around
        std::vector<T_TABLECELL*> sources;
        sources.reserve( rowCount );

        for( int row = 0; row < rowCount; ++row )
            sources.push_back( table->GetCell( row, col ) );

        commit.Modify( table, getScreen() );
        table->SetColCount( table->GetColCount() + 1 );

        for( int row = 0; row < rowCount; ++row )
        {
            T_TABLECELL* cell = copyCell( sources[row] );
            table->InsertCell( row * table->GetColCount() + col, cell );
        }

        for( int afterCol = table->GetColCount() - 1; afterCol > col; afterCol-- )
            table->SetColWidth( afterCol, table->GetColWidth( afterCol - 1 ) );

        table->SetPosition( pos );
        table->Normalize();

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        commit.Push( _( "Add Column Before" ) );

        return 0;
    }

    int doAddColumnAfter( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();
        T_TABLECELL*     rightmost = nullptr;

        for( EDA_ITEM* item : selection )
        {
            T_TABLECELL* cell = static_cast<T_TABLECELL*>( item );

            if( !rightmost || cell->GetColumn() > rightmost->GetColumn() )
                rightmost = cell;
        }

        if( !rightmost )
            return 0;

        int      col = rightmost->GetColumn();
        T_TABLE* table = static_cast<T_TABLE*>( rightmost->GetParent() );
        int      rowCount = table->GetRowCount();
        T_COMMIT commit( getToolMgr() );
        VECTOR2I pos = table->GetPosition();

        // Make a copy of the source column before things start moving around
        std::vector<T_TABLECELL*> sources;
        sources.reserve( rowCount );

        for( int row = 0; row < rowCount; ++row )
            sources.push_back( table->GetCell( row, col ) );

        commit.Modify( table, getScreen() );
        table->SetColCount( table->GetColCount() + 1 );

        for( int row = 0; row < rowCount; ++row )
        {
            T_TABLECELL* cell = copyCell( sources[row] );
            table->InsertCell( row * table->GetColCount() + col + 1, cell );
        }

        for( int afterCol = table->GetColCount() - 1; afterCol > col; afterCol-- )
            table->SetColWidth( afterCol, table->GetColWidth( afterCol - 1 ) );

        table->SetPosition( pos );
        table->Normalize();

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        commit.Push( _( "Add Column After" ) );

        return 0;
    }

    int doDeleteRows( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();

        if( selection.Empty() )
            return 0;

        T_TABLE*         table = static_cast<T_TABLE*>( selection[0]->GetParent() );
        std::vector<int> deleted;

        for( T_TABLECELL* cell : table->GetCells() )
            cell->ClearFlags( STRUCT_DELETED );

        for( int row = 0; row < table->GetRowCount(); ++row )
        {
            bool deleteRow = false;

            for( int col = 0; col < table->GetColCount(); ++col )
            {
                if( table->GetCell( row, col )->IsSelected() )
                {
                    deleteRow = true;
                    break;
                }
            }

            if( deleteRow )
            {
                for( int col = 0; col < table->GetColCount(); ++col )
                    table->GetCell( row, col )->SetFlags( STRUCT_DELETED );

                deleted.push_back( row );
            }
        }

        T_COMMIT commit( getToolMgr() );

        if( deleted.size() == (unsigned) table->GetRowCount() )
        {
            commit.Remove( table, getScreen() );
        }
        else
        {
            commit.Modify( table, getScreen() );

            VECTOR2I pos = table->GetPosition();

            // Save old row heights BEFORE deleting cells
            std::map<int, int> oldRowHeights;
            for( int row = 0; row < table->GetRowCount(); ++row )
                oldRowHeights[row] = table->GetRowHeight( row );

            clearSelection();
            table->DeleteMarkedCells();

            for( int row = 0; row < table->GetRowCount(); ++row )
            {
                int old_row = row;

                for( int deletedRow : deleted )
                {
                    if( deletedRow <= old_row )
                        old_row++;
                }

                // Use saved old heights instead of querying the modified table
                int height = ( oldRowHeights.count( old_row ) ) ? oldRowHeights[old_row] : 0;
                table->SetRowHeight( row, height );
            }

            table->SetPosition( pos );
            table->Normalize();

            getToolMgr()->PostEvent( EVENTS::SelectedEvent );
        }

        if( deleted.size() > 1 )
            commit.Push( _( "Delete Rows" ) );
        else
            commit.Push( _( "Delete Row" ) );

        return 0;
    }

    int doDeleteColumns( const TOOL_EVENT& aEvent )
    {
        const SELECTION& selection = getTableCellSelection();

        if( selection.Empty() )
            return 0;

        T_TABLE*         table = static_cast<T_TABLE*>( selection[0]->GetParent() );
        std::vector<int> deleted;

        for( T_TABLECELL* cell : table->GetCells() )
            cell->ClearFlags( STRUCT_DELETED );

        for( int col = 0; col < table->GetColCount(); ++col )
        {
            bool deleteColumn = false;

            for( int row = 0; row < table->GetRowCount(); ++row )
            {
                if( table->GetCell( row, col )->IsSelected() )
                {
                    deleteColumn = true;
                    break;
                }
            }

            if( deleteColumn )
            {
                for( int row = 0; row < table->GetRowCount(); ++row )
                    table->GetCell( row, col )->SetFlags( STRUCT_DELETED );

                deleted.push_back( col );
            }
        }

        T_COMMIT commit( getToolMgr() );

        if( deleted.size() == (unsigned) table->GetColCount() )
        {
            commit.Remove( table, getScreen() );
        }
        else
        {
            commit.Modify( table, getScreen() );

            VECTOR2I pos = table->GetPosition();

            clearSelection();
            table->DeleteMarkedCells();
            table->SetColCount( table->GetColCount() - deleted.size() );

            for( int col = 0; col < table->GetColCount(); ++col )
            {
                int old_col = col;

                for( int deletedCol : deleted )
                {
                    if( deletedCol <= old_col )
                        old_col++;
                }

                table->SetColWidth( col, table->GetColWidth( old_col ) );
            }

            table->SetPosition( pos );
            table->Normalize();

            getToolMgr()->PostEvent( EVENTS::SelectedEvent );
        }

        if( deleted.size() > 1 )
            commit.Push( _( "Delete Columns" ) );
        else
            commit.Push( _( "Delete Column" ) );

        return 0;
    }

    int doMergeCells( const TOOL_EVENT& aEvent )
    {
        const SELECTION& sel = getTableCellSelection();

        if( sel.Empty() )
            return 0;

        int colMin = std::numeric_limits<int>::max();
        int colMax = 0;
        int rowMin = std::numeric_limits<int>::max();
        int rowMax = 0;

        T_COMMIT commit( getToolMgr() );
        T_TABLE* table = static_cast<T_TABLE*>( sel[0]->GetParent() );

        for( EDA_ITEM* item : sel )
        {
            if( T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item ) )
            {
                colMin = std::min( colMin, cell->GetColumn() );
                colMax = std::max( colMax, cell->GetColumn() + cell->GetColSpan() );
                rowMin = std::min( rowMin, cell->GetRow() );
                rowMax = std::max( rowMax, cell->GetRow() + cell->GetRowSpan() );
            }
        }

        wxString content;
        VECTOR2I extents;

        for( int row = rowMin; row < rowMax; ++row )
        {
            extents.y += table->GetRowHeight( row );
            extents.x = 0;

            for( int col = colMin; col < colMax; ++col )
            {
                extents.x += table->GetColWidth( col );

                T_TABLECELL* cell = table->GetCell( row, col );

                if( !cell->GetText().IsEmpty() )
                {
                    if( !content.IsEmpty() )
                        content += "\n";

                    content += cell->GetText();
                }

                commit.Modify( cell, getScreen() );
                cell->SetColSpan( 0 );
                cell->SetRowSpan( 0 );
                cell->SetText( wxEmptyString );
            }
        }

        T_TABLECELL* topLeft = table->GetCell( rowMin, colMin );
        topLeft->SetColSpan( colMax - colMin );
        topLeft->SetRowSpan( rowMax - rowMin );
        topLeft->SetText( content );
        topLeft->SetEnd( topLeft->GetStart() + extents );

        table->Normalize();
        commit.Push( _( "Merge Cells" ) );

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        return 0;
    }

    int doUnmergeCells( const TOOL_EVENT& aEvent )
    {
        const SELECTION& sel = getTableCellSelection();

        if( sel.Empty() )
            return 0;

        T_COMMIT commit( getToolMgr() );
        T_TABLE* table = static_cast<T_TABLE*>( sel[0]->GetParent() );

        for( EDA_ITEM* item : sel )
        {
            if( T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item ) )
            {
                int rowSpan = cell->GetRowSpan();
                int colSpan = cell->GetColSpan();

                for( int row = cell->GetRow(); row < cell->GetRow() + rowSpan; ++row )
                {
                    for( int col = cell->GetColumn(); col < cell->GetColumn() + colSpan; ++col )
                    {
                        T_TABLECELL* target = table->GetCell( row, col );
                        commit.Modify( target, getScreen() );
                        target->SetColSpan( 1 );
                        target->SetRowSpan( 1 );

                        VECTOR2I extents( table->GetColWidth( col ), table->GetRowHeight( row ) );
                        target->SetEnd( target->GetStart() + extents );
                    }
                }
            }
        }

        table->Normalize();
        commit.Push( _( "Unmerge Cells" ) );

        getToolMgr()->PostEvent( EVENTS::SelectedEvent );

        return 0;
    }

public:
    /**
     * Get the bounding box of a cell block selection.
     * @param aSel The selection to analyze
     * @param aColMin Output: minimum column index
     * @param aColMax Output: maximum column index (exclusive)
     * @param aRowMin Output: minimum row index
     * @param aRowMax Output: maximum row index (exclusive)
     * @return true if selection forms a valid contiguous block, false otherwise
     */
    bool getCellBlockBounds( const SELECTION& aSel, int& aColMin, int& aColMax, int& aRowMin, int& aRowMax )
    {
        if( aSel.Size() < 1 )
            return false;

        aColMin = std::numeric_limits<int>::max();
        aColMax = 0;
        aRowMin = std::numeric_limits<int>::max();
        aRowMax = 0;
        int selectedArea = 0;

        for( EDA_ITEM* item : aSel )
        {
            if( T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item ) )
            {
                aColMin = std::min( aColMin, cell->GetColumn() );
                aColMax = std::max( aColMax, cell->GetColumn() + cell->GetColSpan() );
                aRowMin = std::min( aRowMin, cell->GetRow() );
                aRowMax = std::max( aRowMax, cell->GetRow() + cell->GetRowSpan() );

                selectedArea += cell->GetColSpan() * cell->GetRowSpan();
            }
        }

        // Check if selection is contiguous (for single cell, we allow it)
        if( aSel.Size() == 1 )
            return true;

        return selectedArea == ( aColMax - aColMin ) * ( aRowMax - aRowMin );
    }

    /**
     * Validate if paste-into-cells is possible for the given selection.
     * @param aSel The target cell selection
     * @param aErrorMsg Output: error message if validation fails
     * @return true if paste-into is valid, false otherwise
     */
    bool validatePasteIntoSelection( const SELECTION& aSel, wxString& aErrorMsg )
    {
        if( aSel.Empty() )
        {
            aErrorMsg = _( "No cells selected" );
            return false;
        }

        // Check if all selected items are table cells from the same table
        T_TABLE* table = nullptr;

        for( EDA_ITEM* item : aSel )
        {
            T_TABLECELL* cell = dynamic_cast<T_TABLECELL*>( item );

            if( !cell )
            {
                aErrorMsg = _( "Selection contains non-cell items" );
                return false;
            }

            if( !table )
            {
                table = static_cast<T_TABLE*>( cell->GetParent() );
            }
            else if( cell->GetParent() != table )
            {
                aErrorMsg = _( "Selected cells are from different tables" );
                return false;
            }

            // Check for merged cells (block paste-into for now)
            if( cell->GetColSpan() > 1 || cell->GetRowSpan() > 1 )
            {
                aErrorMsg = _( "Cannot paste into merged cells" );
                return false;
            }
        }

        // Check if selection is contiguous
        int colMin, colMax, rowMin, rowMax;

        if( !getCellBlockBounds( aSel, colMin, colMax, rowMin, rowMax ) )
        {
            aErrorMsg = _( "Selected cells must form a contiguous block" );
            return false;
        }

        return true;
    }

    /**
     * Paste text content from source table into selected cells.
     * @param aSel The target cell selection
     * @param aSourceTable The table containing cells to paste
     * @param aCommit The commit object for tracking changes
     * @return true if paste succeeded, false otherwise
     */
    bool pasteCellsIntoSelection( const SELECTION& aSel, T_TABLE* aSourceTable, T_COMMIT& aCommit )
    {
        if( !aSourceTable || aSel.Empty() )
            return false;

        // Get target cell range
        int targetColMin, targetColMax, targetRowMin, targetRowMax;

        if( !getCellBlockBounds( aSel, targetColMin, targetColMax, targetRowMin, targetRowMax ) )
            return false;

        T_TABLE* targetTable = static_cast<T_TABLE*>( static_cast<T_TABLECELL*>( aSel[0] )->GetParent() );

        // Get source table dimensions
        int sourceRows = aSourceTable->GetRowCount();
        int sourceCols = aSourceTable->GetColCount();

        int pasteEndRow = targetRowMin + sourceRows;
        int pasteEndCol = targetColMin + sourceCols;

        if( pasteEndRow > targetTable->GetRowCount() || pasteEndCol > targetTable->GetColCount() )
        {
            int rowsToAdd = std::max( 0, pasteEndRow - targetTable->GetRowCount() );
            int colsToAdd = std::max( 0, pasteEndCol - targetTable->GetColCount() );

            VECTOR2I pos = targetTable->GetPosition();
            aCommit.Modify( targetTable, getScreen() );

            for( int i = 0; i < rowsToAdd; ++i )
            {
                int insertRow = targetTable->GetRowCount();
                int clipboardRow = insertRow - targetRowMin;

                std::vector<T_TABLECELL*> sources;
                sources.reserve( aSourceTable->GetColCount() );

                for( int col = 0; col < aSourceTable->GetColCount(); ++col )
                    sources.push_back( aSourceTable->GetCell( clipboardRow, col ) );

                for( int col = 0; col < targetTable->GetColCount(); ++col )
                {
                    T_TABLECELL* sourceCell = ( col < aSourceTable->GetColCount() ) ? sources[col] : sources[0];
                    T_TABLECELL* cell = copyCell( sourceCell );
                    targetTable->InsertCell( insertRow * targetTable->GetColCount(), cell );
                }

                for( int afterRow = targetTable->GetRowCount() - 1; afterRow > insertRow; afterRow-- )
                    targetTable->SetRowHeight( afterRow, targetTable->GetRowHeight( afterRow - 1 ) );

                targetTable->SetRowHeight( insertRow, aSourceTable->GetRowHeight( clipboardRow ) );
            }

            for( int i = 0; i < colsToAdd; ++i )
            {
                int insertCol = targetTable->GetColCount();
                int clipboardCol = insertCol - targetColMin;
                int rowCount = targetTable->GetRowCount();

                targetTable->SetColCount( targetTable->GetColCount() + 1 );

                std::vector<T_TABLECELL*> sources;
                sources.reserve( aSourceTable->GetRowCount() );

                for( int row = 0; row < aSourceTable->GetRowCount(); ++row )
                    sources.push_back( aSourceTable->GetCell( row, clipboardCol ) );

                for( int row = 0; row < rowCount; ++row )
                {
                    T_TABLECELL* sourceCell = ( row < aSourceTable->GetRowCount() ) ? sources[row] : sources[0];
                    T_TABLECELL* cell = copyCell( sourceCell );
                    targetTable->InsertCell( row * targetTable->GetColCount() + insertCol, cell );
                }

                for( int afterCol = targetTable->GetColCount() - 1; afterCol > insertCol; afterCol-- )
                    targetTable->SetColWidth( afterCol, targetTable->GetColWidth( afterCol - 1 ) );

                targetTable->SetColWidth( insertCol, aSourceTable->GetColWidth( clipboardCol ) );
            }

            targetTable->SetPosition( pos );
            targetTable->Normalize();
        }

        for( int srcRow = 0; srcRow < sourceRows; ++srcRow )
        {
            for( int srcCol = 0; srcCol < sourceCols; ++srcCol )
            {
                int destRow = targetRowMin + srcRow;
                int destCol = targetColMin + srcCol;

                if( destRow >= targetTable->GetRowCount() || destCol >= targetTable->GetColCount() )
                    continue;

                T_TABLECELL* sourceCell = aSourceTable->GetCell( srcRow, srcCol );
                T_TABLECELL* targetCell = targetTable->GetCell( destRow, destCol );

                aCommit.Modify( targetCell, getScreen() );

                targetCell->SetText( sourceCell->GetText() );
                targetCell->SetAttributes( *sourceCell, false );
                targetCell->SetStroke( sourceCell->GetStroke() );
                targetCell->SetFillMode( sourceCell->GetFillMode() );
                targetCell->SetFillColor( sourceCell->GetFillColor() );
            }
        }

        targetTable->Normalize();
        return true;
    }

    virtual TOOL_MANAGER* getToolMgr() = 0;
    virtual BASE_SCREEN*  getScreen() = 0;

    virtual const SELECTION& getTableCellSelection() = 0;
    virtual void             clearSelection() = 0;

    virtual T_TABLECELL* copyCell( T_TABLECELL* aSource ) = 0;
};

#endif //EDIT_TABLE_TOOL_BASE_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
                                    && SELECTION_CONDITIONS::OnlyTypes( { SCH_TABLECELL_T,
                                                                          PCB_TABLECELL_T } );

        auto cellBlockSelection =
                [&]( const SELECTION& sel )
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

        auto mergedCellsSelection =
                [&]( const SELECTION& sel )
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
            commit.Remove( table );
        }
        else
        {
            commit.Modify( table, getScreen() );

            VECTOR2I pos = table->GetPosition();

            clearSelection();
            table->DeleteMarkedCells();

            for( int row = 0; row < table->GetRowCount(); ++row )
            {
                int offset = 0;

                for( int deletedRow : deleted )
                {
                    if( deletedRow >= row )
                        offset++;
                }

                table->SetRowHeight( row, table->GetRowHeight( row + offset ) );
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
            commit.Remove( table );
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
                int offset = 0;

                for( int deletedCol : deleted )
                {
                    if( deletedCol >= col )
                        offset++;
                }

                table->SetColWidth( col, table->GetColWidth( col + offset ) );
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

    virtual TOOL_MANAGER* getToolMgr() = 0;
    virtual BASE_SCREEN* getScreen() = 0;

    virtual const SELECTION& getTableCellSelection() = 0;
    virtual void clearSelection() = 0;

    virtual T_TABLECELL* copyCell( T_TABLECELL* aSource ) = 0;
};

#endif //EDIT_TABLE_TOOL_BASE_H

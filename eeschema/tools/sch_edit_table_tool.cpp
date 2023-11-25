/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <kiway.h>
#include <tools/sch_edit_tool.h>
#include <tools/ee_selection_tool.h>
#include <ee_actions.h>
#include <string_utils.h>
#include <sch_table.h>
#include <sch_tablecell.h>
#include <sch_commit.h>
#include <pgm_base.h>
#include <core/kicad_algo.h>
#include <tools/sch_edit_table_tool.h>


SCH_EDIT_TABLE_TOOL::SCH_EDIT_TABLE_TOOL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.TableEditor" )
{
}


bool SCH_EDIT_TABLE_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto tableCellSelection = EE_CONDITIONS::MoreThan( 0 )
                                && EE_CONDITIONS::OnlyTypes( { SCH_TABLECELL_T } );

    auto tableCellBlockSelection =
            [&]( const SELECTION& sel )
    {
        if( sel.CountType( SCH_TABLECELL_T ) < 2 )
            return false;

        int colMin = std::numeric_limits<int>::max();
        int colMax = 0;
        int rowMin = std::numeric_limits<int>::max();
        int rowMax = 0;
        int selectedArea = 0;

        for( EDA_ITEM* item : sel )
        {
            wxCHECK2( item->Type() == SCH_TABLECELL_T, continue );

            SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );
            colMin = std::min( colMin, cell->GetColumn() );
            colMax = std::max( colMax, cell->GetColumn() + cell->GetColSpan() );
            rowMin = std::min( rowMin, cell->GetRow() );
            rowMax = std::max( rowMax, cell->GetRow() + cell->GetRowSpan() );

            selectedArea += cell->GetColSpan() * cell->GetRowSpan();
        }

        return selectedArea == ( colMax - colMin ) * ( rowMax - rowMin );
    };

    auto mergedCellsSelection =
            [&]( const SELECTION& sel )
    {
        for( EDA_ITEM* item : sel )
        {
            if( SCH_TABLECELL* cell = dynamic_cast<SCH_TABLECELL*>( item ) )
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
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddSeparator( 100 );
    selToolMenu.AddItem( EE_ACTIONS::addRowAbove,     tableCellSelection && EE_CONDITIONS::Idle, 100 );
    selToolMenu.AddItem( EE_ACTIONS::addRowBelow,     tableCellSelection && EE_CONDITIONS::Idle, 100 );
    selToolMenu.AddItem( EE_ACTIONS::addColumnBefore, tableCellSelection && EE_CONDITIONS::Idle, 100 );
    selToolMenu.AddItem( EE_ACTIONS::addColumnAfter,  tableCellSelection && EE_CONDITIONS::Idle, 100 );

    selToolMenu.AddSeparator( 100 );
    selToolMenu.AddItem( EE_ACTIONS::deleteRows,      tableCellSelection && EE_CONDITIONS::Idle, 100 );
    selToolMenu.AddItem( EE_ACTIONS::deleteColumns,   tableCellSelection && EE_CONDITIONS::Idle, 100 );

    selToolMenu.AddSeparator( 100 );
    selToolMenu.AddItem( EE_ACTIONS::mergeCells,      tableCellSelection && tableCellBlockSelection, 100 );
    selToolMenu.AddItem( EE_ACTIONS::unmergeCells,    tableCellSelection && mergedCellsSelection, 100 );

    selToolMenu.AddSeparator( 100 );

    return true;
}


SCH_TABLECELL* copyCell( SCH_TABLECELL* aSource )
{
    SCH_TABLECELL* cell = new SCH_TABLECELL();

    cell->SetEnd( aSource->GetEnd() - aSource->GetStart() );
    cell->SetFillMode( aSource->GetFillMode() );
    cell->SetFillColor( aSource->GetFillColor() );

    return cell;
}


int SCH_EDIT_TABLE_TOOL::AddRowAbove( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&  selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
    SCH_TABLECELL* topmost = nullptr;

    for( EDA_ITEM* item : selection )
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );

        if( !topmost || cell->GetRow() < topmost->GetRow() )
            topmost = cell;
    }

    if( !topmost )
        return 0;

    int        row = topmost->GetRow();
    SCH_TABLE* table = static_cast<SCH_TABLE*>( topmost->GetParent() );
    SCH_COMMIT commit( m_toolMgr );

    // Make a copy of the source row before things start moving around
    std::vector<SCH_TABLECELL*> sources;
    sources.reserve( table->GetColCount() );

    for( int col = 0; col < table->GetColCount(); ++col )
        sources.push_back( table->GetCell( row, col ) );

    commit.Modify( table, m_frame->GetScreen() );

    for( int col = 0; col < table->GetColCount(); ++col )
    {
        SCH_TABLECELL* cell = copyCell( sources[col] );
        table->InsertCell( row * table->GetColCount(), cell );
    }

    table->Normalize();

    commit.Push( _( "Add Row Above" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::AddRowBelow( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&  selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
    SCH_TABLECELL* bottommost = nullptr;

    if( selection.Empty() )
        return 0;

    for( EDA_ITEM* item : selection )
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );

        if( !bottommost || cell->GetRow() > bottommost->GetRow() )
            bottommost = cell;
    }

    if( !bottommost )
        return 0;

    int        row = bottommost->GetRow();
    SCH_TABLE* table = static_cast<SCH_TABLE*>( bottommost->GetParent() );
    SCH_COMMIT commit( m_toolMgr );

    // Make a copy of the source row before things start moving around
    std::vector<SCH_TABLECELL*> sources;
    sources.reserve( table->GetColCount() );

    for( int col = 0; col < table->GetColCount(); ++col )
        sources.push_back( table->GetCell( row, col ) );

    commit.Modify( table, m_frame->GetScreen() );

    for( int col = 0; col < table->GetColCount(); ++col )
    {
        SCH_TABLECELL* cell = copyCell( sources[col] );
        table->InsertCell( ( row + 1 ) * table->GetColCount(), cell );
    }

    table->Normalize();

    commit.Push( _( "Add Row Below" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::AddColumnBefore( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&  selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
    SCH_TABLECELL* leftmost = nullptr;

    for( EDA_ITEM* item : selection )
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );

        if( !leftmost || cell->GetColumn() < leftmost->GetColumn() )
            leftmost = cell;
    }

    if( !leftmost )
        return 0;

    int        col = leftmost->GetColumn();
    SCH_TABLE* table = static_cast<SCH_TABLE*>( leftmost->GetParent() );
    int        rowCount = table->GetRowCount();
    SCH_COMMIT commit( m_toolMgr );

    // Make a copy of the source column before things start moving around
    std::vector<SCH_TABLECELL*> sources;
    sources.reserve( rowCount );

    for( int row = 0; row < rowCount; ++row )
        sources.push_back( table->GetCell( row, col ) );

    commit.Modify( table, m_frame->GetScreen() );
    table->SetColCount( table->GetColCount() + 1 );

    for( int row = 0; row < rowCount; ++row )
    {
        SCH_TABLECELL* cell = copyCell( sources[row] );
        table->InsertCell( row * table->GetColCount() + col, cell );
    }

    table->Normalize();

    commit.Push( _( "Add Column Before" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::AddColumnAfter( const TOOL_EVENT& aEvent )
{
    EE_SELECTION&  selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
    SCH_TABLECELL* rightmost = nullptr;

    for( EDA_ITEM* item : selection )
    {
        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );

        if( !rightmost || cell->GetColumn() > rightmost->GetColumn() )
            rightmost = cell;
    }

    if( !rightmost )
        return 0;

    int        col = rightmost->GetColumn();
    SCH_TABLE* table = static_cast<SCH_TABLE*>( rightmost->GetParent() );
    int        rowCount = table->GetRowCount();
    SCH_COMMIT commit( m_toolMgr );

    // Make a copy of the source column before things start moving around
    std::vector<SCH_TABLECELL*> sources;
    sources.reserve( rowCount );

    for( int row = 0; row < rowCount; ++row )
        sources.push_back( table->GetCell( row, col ) );

    commit.Modify( table, m_frame->GetScreen() );
    table->SetColCount( table->GetColCount() + 1 );

    for( int row = 0; row < rowCount; ++row )
    {
        SCH_TABLECELL* cell = copyCell( sources[row] );
        table->InsertCell( row * table->GetColCount() + col + 1, cell );
    }

    table->Normalize();

    commit.Push( _( "Add Column After" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::DeleteColumns( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );

    if( selection.Empty() )
        return 0;

    SCH_TABLE* table = static_cast<SCH_TABLE*>( selection[0]->GetParent() );
    int        deleted = 0;

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

            deleted++;
        }
    }

    SCH_COMMIT commit( m_toolMgr );

    if( deleted == table->GetColCount() )
    {
        commit.Remove( table, m_frame->GetScreen() );
    }
    else
    {
        commit.Modify( table, m_frame->GetScreen() );

        table->DeleteMarkedCells();
        table->SetColCount( table->GetColCount() - deleted );
        table->Normalize();
    }

    if( deleted > 1 )
        commit.Push( _( "Delete Columns" ) );
    else
        commit.Push( _( "Delete Column" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::DeleteRows( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );

    if( selection.Empty() )
        return 0;

    SCH_TABLE* table = static_cast<SCH_TABLE*>( selection[0]->GetParent() );
    int        deleted = 0;

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

            deleted++;
        }
    }

    SCH_COMMIT commit( m_toolMgr );

    if( deleted == table->GetRowCount() )
    {
        commit.Remove( table, m_frame->GetScreen() );
    }
    else
    {
        commit.Modify( table, m_frame->GetScreen() );

        table->DeleteMarkedCells();
        table->Normalize();
    }

    if( deleted > 1 )
        commit.Push( _( "Delete Rows" ) );
    else
        commit.Push( _( "Delete Row" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::MergeCells( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& sel = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );

    if( sel.Empty() )
        return 0;

    int colMin = std::numeric_limits<int>::max();
    int colMax = 0;
    int rowMin = std::numeric_limits<int>::max();
    int rowMax = 0;

    SCH_COMMIT commit( m_toolMgr );
    SCH_TABLE* table = static_cast<SCH_TABLE*>( sel[0]->GetParent() );

    for( EDA_ITEM* item : sel )
    {
        wxCHECK2( item->Type() == SCH_TABLECELL_T, continue );

        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );
        colMin = std::min( colMin, cell->GetColumn() );
        colMax = std::max( colMax, cell->GetColumn() + cell->GetColSpan() );
        rowMin = std::min( rowMin, cell->GetRow() );
        rowMax = std::max( rowMax, cell->GetRow() + cell->GetRowSpan() );
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

            SCH_TABLECELL* cell = table->GetCell( row, col );

            if( !cell->GetText().IsEmpty() )
            {
                if( !content.IsEmpty() )
                    content += "\n";

                content += cell->GetText();
            }

            commit.Modify( cell, m_frame->GetScreen() );
            cell->SetColSpan( 0 );
            cell->SetRowSpan( 0 );
            cell->SetText( wxEmptyString );
        }
    }

    SCH_TABLECELL* topLeft = table->GetCell( rowMin, colMin );
    topLeft->SetColSpan( colMax - colMin );
    topLeft->SetRowSpan( rowMax - rowMin );
    topLeft->SetText( content );
    topLeft->SetEnd( topLeft->GetStart() + extents );

    table->Normalize();
    commit.Push( _( "Merge Cells" ) );

    return 0;
}


int SCH_EDIT_TABLE_TOOL::UnmergeCells( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& sel = m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );

    if( sel.Empty() )
        return 0;

    SCH_COMMIT commit( m_toolMgr );
    SCH_TABLE* table = static_cast<SCH_TABLE*>( sel[0]->GetParent() );

    for( EDA_ITEM* item : sel )
    {
        wxCHECK2( item->Type() == SCH_TABLECELL_T, continue );

        SCH_TABLECELL* cell = static_cast<SCH_TABLECELL*>( item );
        int            rowSpan = cell->GetRowSpan();
        int            colSpan = cell->GetColSpan();

        for( int row = cell->GetRow(); row < cell->GetRow() + rowSpan; ++row )
        {
            for( int col = cell->GetColumn(); col < cell->GetColumn() + colSpan; ++col )
            {
                SCH_TABLECELL* target = table->GetCell( row, col );
                commit.Modify( target, m_frame->GetScreen() );
                target->SetColSpan( 1 );
                target->SetRowSpan( 1 );

                VECTOR2I extents( table->GetColWidth( col ), table->GetRowHeight( row ) );
                target->SetEnd( target->GetStart() + extents );
            }
        }
    }

    table->Normalize();
    commit.Push( _( "Unmerge Cells" ) );

    return 0;
}


void SCH_EDIT_TABLE_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TABLE_TOOL::AddRowAbove,        EE_ACTIONS::addRowAbove.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddRowBelow,        EE_ACTIONS::addRowBelow.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::AddColumnBefore,    EE_ACTIONS::addColumnBefore.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddColumnAfter,     EE_ACTIONS::addColumnAfter.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::DeleteRows,         EE_ACTIONS::deleteRows.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::DeleteColumns,      EE_ACTIONS::deleteColumns.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::MergeCells,         EE_ACTIONS::mergeCells.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::UnmergeCells,       EE_ACTIONS::unmergeCells.MakeEvent() );
}

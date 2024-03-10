/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_actions.h>
#include <collectors.h>
#include <dialogs/dialog_table_properties.h>
#include <tools/pcb_edit_table_tool.h>


PCB_EDIT_TABLE_TOOL::PCB_EDIT_TABLE_TOOL() :
        PCB_TOOL_BASE( "pcbnew.TableEditor" )
{
}


bool PCB_EDIT_TABLE_TOOL::Init()
{
    PCB_TOOL_BASE::Init();

    addMenus( m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->GetToolMenu().GetMenu() );

    return true;
}


PCB_TABLECELL* PCB_EDIT_TABLE_TOOL::copyCell( PCB_TABLECELL* aSource )
{
    PCB_TABLECELL* cell = new PCB_TABLECELL( aSource->GetParent() );

    cell->SetStart( aSource->GetStart() );
    cell->SetEnd( aSource->GetEnd() );

    return cell;
}


const SELECTION& PCB_EDIT_TABLE_TOOL::getTableCellSelection()
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();

    return selTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    if( !dynamic_cast<PCB_TABLECELL*>( aCollector[ i ] ) )
                        aCollector.Remove( aCollector[ i ] );
                }
            },
            false /* prompt user regarding locked items */ );
}


void PCB_EDIT_TABLE_TOOL::clearSelection()
{
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );
};


int PCB_EDIT_TABLE_TOOL::EditTable( const TOOL_EVENT& aEvent )
{
    const SELECTION& selection = getTableCellSelection();
    bool             clearSelection = selection.IsHover();
    PCB_TABLE*       parentTable = nullptr;

    for( EDA_ITEM* item : selection.Items() )
    {
        if( item->Type() != PCB_TABLECELL_T )
            return 0;

        PCB_TABLE* table = static_cast<PCB_TABLE*>( item->GetParent() );

        if( !parentTable )
        {
            parentTable = table;
        }
        else if( parentTable != table )
        {
            parentTable = nullptr;
            break;
        }
    }

    if( parentTable )
    {
        DIALOG_TABLE_PROPERTIES dlg( frame(), parentTable );

        dlg.ShowQuasiModal();   // Scintilla's auto-complete requires quasiModal
    }

    if( clearSelection )
        m_toolMgr->RunAction( PCB_ACTIONS::selectionClear );

    return 0;
}


void PCB_EDIT_TABLE_TOOL::setTransitions()
{
    Go( &PCB_EDIT_TABLE_TOOL::AddRowAbove,        ACTIONS::addRowAbove.MakeEvent() );
    Go( &PCB_EDIT_TABLE_TOOL::AddRowBelow,        ACTIONS::addRowBelow.MakeEvent() );

    Go( &PCB_EDIT_TABLE_TOOL::AddColumnBefore,    ACTIONS::addColBefore.MakeEvent() );
    Go( &PCB_EDIT_TABLE_TOOL::AddColumnAfter,     ACTIONS::addColAfter.MakeEvent() );

    Go( &PCB_EDIT_TABLE_TOOL::DeleteRows,         ACTIONS::deleteRows.MakeEvent() );
    Go( &PCB_EDIT_TABLE_TOOL::DeleteColumns,      ACTIONS::deleteColumns.MakeEvent() );

    Go( &PCB_EDIT_TABLE_TOOL::MergeCells,         ACTIONS::mergeCells.MakeEvent() );
    Go( &PCB_EDIT_TABLE_TOOL::UnmergeCells,       ACTIONS::unmergeCells.MakeEvent() );

    Go( &PCB_EDIT_TABLE_TOOL::EditTable,          ACTIONS::editTable.MakeEvent() );
}

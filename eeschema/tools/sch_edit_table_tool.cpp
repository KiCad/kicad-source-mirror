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

#include <ee_actions.h>
#include <tools/sch_edit_table_tool.h>
#include <dialogs/dialog_table_properties.h>


SCH_EDIT_TABLE_TOOL::SCH_EDIT_TABLE_TOOL() :
        EE_TOOL_BASE<SCH_EDIT_FRAME>( "eeschema.TableEditor" )
{
}


bool SCH_EDIT_TABLE_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    addMenus( m_selectionTool->GetToolMenu().GetMenu() );

    return true;
}


int SCH_EDIT_TABLE_TOOL::EditTable( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::EditableItems );
    bool          clearSelection = selection.IsHover();
    SCH_TABLE*    parentTable = nullptr;

    for( EDA_ITEM* item : selection.Items() )
    {
        if( item->Type() != SCH_TABLECELL_T )
            return 0;

        SCH_TABLE* table = static_cast<SCH_TABLE*>( item->GetParent() );

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
        DIALOG_TABLE_PROPERTIES dlg( m_frame, parentTable );

        // QuasiModal required for Scintilla auto-complete
        dlg.ShowQuasiModal();
    }

    if( clearSelection )
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

    return 0;
}


SCH_TABLECELL* SCH_EDIT_TABLE_TOOL::copyCell( SCH_TABLECELL* aSource )
{
    SCH_TABLECELL* cell = new SCH_TABLECELL();

    cell->SetEnd( aSource->GetEnd() - aSource->GetStart() );
    cell->SetFillMode( aSource->GetFillMode() );
    cell->SetFillColor( aSource->GetFillColor() );

    return cell;
}


const SELECTION& SCH_EDIT_TABLE_TOOL::getTableCellSelection()
{
    return m_selectionTool->RequestSelection( { SCH_TABLECELL_T } );
}


void SCH_EDIT_TABLE_TOOL::setTransitions()
{
    Go( &SCH_EDIT_TABLE_TOOL::AddRowAbove,        ACTIONS::addRowAbove.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddRowBelow,        ACTIONS::addRowBelow.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::AddColumnBefore,    ACTIONS::addColBefore.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::AddColumnAfter,     ACTIONS::addColAfter.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::DeleteRows,         ACTIONS::deleteRows.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::DeleteColumns,      ACTIONS::deleteColumns.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::MergeCells,         ACTIONS::mergeCells.MakeEvent() );
    Go( &SCH_EDIT_TABLE_TOOL::UnmergeCells,       ACTIONS::unmergeCells.MakeEvent() );

    Go( &SCH_EDIT_TABLE_TOOL::EditTable,          ACTIONS::editTable.MakeEvent() );
}

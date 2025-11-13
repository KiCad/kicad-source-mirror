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

#ifndef PCB_EDIT_TABLE_TOOL_H
#define PCB_EDIT_TABLE_TOOL_H

#include <tools/pcb_tool_base.h>
#include <tool/edit_table_tool_base.h>
#include <pcb_table.h>
#include <pcb_tablecell.h>
#include <board_commit.h>


class PCB_EDIT_TABLE_TOOL : public PCB_TOOL_BASE,
                            public EDIT_TABLE_TOOL_BASE<PCB_TABLE, PCB_TABLECELL, BOARD_COMMIT>
{
public:
    PCB_EDIT_TABLE_TOOL();
    ~PCB_EDIT_TABLE_TOOL() override { }

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    int AddRowAbove( const TOOL_EVENT& aEvent )     { return doAddRowAbove( aEvent ); }
    int AddRowBelow( const TOOL_EVENT& aEvent )     { return doAddRowBelow( aEvent ); }
    int AddColumnBefore( const TOOL_EVENT& aEvent ) { return doAddColumnBefore( aEvent ); }
    int AddColumnAfter( const TOOL_EVENT& aEvent )  { return doAddColumnAfter( aEvent ); }
    int DeleteRows( const TOOL_EVENT& aEvent )      { return doDeleteRows( aEvent ); }
    int DeleteColumns( const TOOL_EVENT& aEvent )   { return doDeleteColumns( aEvent ); }

    int MergeCells( const TOOL_EVENT& aEvent )      { return doMergeCells( aEvent ); }
    int UnmergeCells( const TOOL_EVENT& aEvent )    { return doUnmergeCells( aEvent ); }

    int EditTable( const TOOL_EVENT& aEvent );
    int ExportTableToCSV( const TOOL_EVENT& aEvent );

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    TOOL_MANAGER* getToolMgr() override { return m_toolMgr; }
    BASE_SCREEN* getScreen() override { return nullptr; }

    const SELECTION& getTableCellSelection() override;
    void clearSelection() override;

    PCB_TABLECELL* copyCell( PCB_TABLECELL* aSource ) override;
};

#endif //PCB_EDIT_TABLE_TOOL_H

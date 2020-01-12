/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string>
#include <wx/filedlg.h>
#include <wx/wx.h>

#include <dialogs/dialog_board_renum.h>
#include <dialogs/dialog_board_renum_base.h>
#include <tools/pcb_renum_tool.h>

#ifndef __linux__ //Include Windows functions
#include <windows.h>
#endif


PCB_RENUM_TOOL::PCB_RENUM_TOOL() : PCB_TOOL_BASE( "pcbnew.RenumTool" ), m_frame( nullptr )
{
}


bool PCB_RENUM_TOOL::Init()
{
    return true;
}


void PCB_RENUM_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int PCB_RENUM_TOOL::ShowRenumDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_RENUM dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


void PCB_RENUM_TOOL::setTransitions()
{
    Go( &PCB_RENUM_TOOL::ShowRenumDialog, PCB_ACTIONS::boardRenum.MakeEvent() );
}

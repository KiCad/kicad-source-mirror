/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Brian Piccioni brian@documenteddesigns.com
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Brian Piccioni <brian@documenteddesigns.com>
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
#include <wx/filedlg.h>
#include <tools/board_reannotate_tool.h>


BOARD_REANNOTATE_TOOL::BOARD_REANNOTATE_TOOL() :
     PCB_TOOL_BASE( "pcbnew.ReannotateTool" ),
     m_frame( nullptr )
{
}


bool BOARD_REANNOTATE_TOOL::Init()
{
    return true;
}


void BOARD_REANNOTATE_TOOL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_EDIT_FRAME>();
}


int BOARD_REANNOTATE_TOOL::ShowReannotateDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_BOARD_REANNOTATE dialog( m_frame );
    dialog.ShowModal();
    return 0;
}


void BOARD_REANNOTATE_TOOL::setTransitions()
{
    Go( &BOARD_REANNOTATE_TOOL::ShowReannotateDialog, PCB_ACTIONS::boardReannotate.MakeEvent() );
}

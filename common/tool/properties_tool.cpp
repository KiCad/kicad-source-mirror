/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <eda_draw_frame.h>
#include <tool/actions.h>
#include <tool/properties_tool.h>
#include <widgets/properties_panel.h>


int PROPERTIES_TOOL::UpdateProperties( const TOOL_EVENT& aEvent )
{
    EDA_DRAW_FRAME* editFrame = getEditFrame<EDA_DRAW_FRAME>();

    if( editFrame )
        editFrame->UpdateProperties();

    return 0;
}


void PROPERTIES_TOOL::setTransitions()
{
    TOOL_EVENT undoRedoPostEvt = { TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL };
    Go( &PROPERTIES_TOOL::UpdateProperties, undoRedoPostEvt );
    Go( &PROPERTIES_TOOL::UpdateProperties, EVENTS::PointSelectedEvent );
    Go( &PROPERTIES_TOOL::UpdateProperties, EVENTS::SelectedEvent );
    Go( &PROPERTIES_TOOL::UpdateProperties, EVENTS::UnselectedEvent );
    Go( &PROPERTIES_TOOL::UpdateProperties, EVENTS::ClearedEvent );
    Go( &PROPERTIES_TOOL::UpdateProperties, EVENTS::SelectedItemsModified );
}

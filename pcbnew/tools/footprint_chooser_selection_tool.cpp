/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tools/pcb_viewer_tools.h>
#include <tool/actions.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/footprint_chooser_selection_tool.h>


FOOTPRINT_CHOOSER_SELECTION_TOOL::FOOTPRINT_CHOOSER_SELECTION_TOOL() :
        TOOL_INTERACTIVE( "footprintChooserDummySelectionTool" )
{
}


bool FOOTPRINT_CHOOSER_SELECTION_TOOL::Init()
{
    PCB_VIEWER_TOOLS* viewerTools = m_toolMgr->GetTool<PCB_VIEWER_TOOLS>();
    CONDITIONAL_MENU& menu = viewerTools->GetToolMenu().GetMenu();

    menu.AddSeparator( 1 );
    menu.AddCheckItem( ACTIONS::toggleGrid,           SELECTION_CONDITIONS::ShowAlways, 1 );
    menu.AddCheckItem( ACTIONS::cursorSmallCrosshairs,    SELECTION_CONDITIONS::ShowAlways, 1 );
    menu.AddCheckItem( ACTIONS::cursorFullCrosshairs,     SELECTION_CONDITIONS::ShowAlways, 1 );
    menu.AddCheckItem( ACTIONS::cursor45Crosshairs,       SELECTION_CONDITIONS::ShowAlways, 1 );

    menu.AddSeparator( 10 );
    menu.AddCheckItem( ACTIONS::inchesUnits,          SELECTION_CONDITIONS::ShowAlways, 10 );
    menu.AddCheckItem( ACTIONS::milsUnits,            SELECTION_CONDITIONS::ShowAlways, 10 );
    menu.AddCheckItem( ACTIONS::millimetersUnits,     SELECTION_CONDITIONS::ShowAlways, 10 );

    menu.AddSeparator( 20 );
    menu.AddCheckItem( PCB_ACTIONS::showPadNumbers,   SELECTION_CONDITIONS::ShowAlways, 20 );
    menu.AddCheckItem( PCB_ACTIONS::padDisplayMode,   SELECTION_CONDITIONS::ShowAlways, 20 );
    menu.AddCheckItem( PCB_ACTIONS::textOutlines,     SELECTION_CONDITIONS::ShowAlways, 20 );
    menu.AddCheckItem( PCB_ACTIONS::graphicsOutlines, SELECTION_CONDITIONS::ShowAlways, 20 );

    return true;
}


int FOOTPRINT_CHOOSER_SELECTION_TOOL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu      = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );

    if( conditionalMenu )
        conditionalMenu->Evaluate( m_selection );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


void FOOTPRINT_CHOOSER_SELECTION_TOOL::setTransitions()
{
    Go( &FOOTPRINT_CHOOSER_SELECTION_TOOL::UpdateMenu,    ACTIONS::updateMenu.MakeEvent() );
}

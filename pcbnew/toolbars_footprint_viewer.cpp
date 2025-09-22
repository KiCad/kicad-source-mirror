/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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


#include <advanced_config.h>
#include <macros.h>
#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/pcb_actions.h>
#include "footprint_viewer_frame.h"
#include "pcbnew_id.h"
#include <widgets/wx_menubar.h>
#include <wx/choice.h>

#include <toolbars_footprint_viewer.h>

std::optional<TOOLBAR_CONFIGURATION> FOOTPRINT_VIEWER_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::RIGHT:
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::TOP_MAIN:
        config.AppendAction( PCB_ACTIONS::previousFootprint )
              .AppendAction( PCB_ACTIONS::nextFootprint );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendAction( ACTIONS::show3DViewer )
              .AppendAction( PCB_ACTIONS::saveFpToBoard );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::gridSelect );

        config.AppendSeparator()
              .AppendControl( ACTION_TOOLBAR_CONTROLS::zoomSelect )
              .AppendAction( PCB_ACTIONS::fpAutoZoom);
        break;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::selectionTool )
              .AppendAction( ACTIONS::measureTool );

        config.AppendSeparator()
              .AppendAction( ACTIONS::toggleGrid )
              .AppendAction( ACTIONS::togglePolarCoords )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::millimetersUnits )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits ) )
                            .AppendSeparator()
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Crosshair modes" ) )
                            .AddAction( ACTIONS::cursorSmallCrosshairs )
                            .AddAction( ACTIONS::cursorFullCrosshairs )
                            .AddAction( ACTIONS::cursor45Crosshairs ) );

        config.AppendSeparator()
              .AppendAction( PCB_ACTIONS::showPadNumbers )
              .AppendAction( PCB_ACTIONS::padDisplayMode )
              .AppendAction( PCB_ACTIONS::textOutlines )
              .AppendAction( PCB_ACTIONS::graphicsOutlines );

        if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
            config.AppendAction( ACTIONS::toggleBoundingBoxes );

        break;
    }

    // clang-format on
    return config;
}


void FOOTPRINT_VIEWER_FRAME::doReCreateMenuBar()
{
    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();

    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();


    //----- File menu -----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->AddClose( _( "Footprint Viewer" ) );

    //----- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::show3DViewer );

    //----- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}

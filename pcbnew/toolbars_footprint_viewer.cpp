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


std::optional<TOOLBAR_CONFIGURATION> FOOTPRINT_VIEWER_FRAME::DefaultTopMainToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    /* TODO (ISM): Convert to actions
    m_tbTopMain->AddTool( ID_MODVIEW_PREVIOUS, wxEmptyString,
            KiScaledBitmap( BITMAPS::lib_previous, this ),
            _( "Display previous footprint" ) );
    m_tbTopMain->AddTool( ID_MODVIEW_NEXT, wxEmptyString,
            KiScaledBitmap( BITMAPS::lib_next, this ),
            _( "Display next footprint" ) );
*/

    config.AppendSeparator()
          .AppendAction( ACTIONS::zoomRedraw )
          .AppendAction( ACTIONS::zoomInCenter )
          .AppendAction( ACTIONS::zoomOutCenter )
          .AppendAction( ACTIONS::zoomFitScreen )
          .AppendAction( ACTIONS::zoomTool );

    config.AppendSeparator()
          .AppendAction( ACTIONS::show3DViewer );

    /* TODO (ISM): Convert to action
    m_tbTopMain->AddTool( ID_ADD_FOOTPRINT_TO_BOARD, wxEmptyString,
            KiScaledBitmap( BITMAPS::insert_module_board, this ),
            _( "Insert footprint in board" ) );
*/

    config.AppendSeparator()
          .AppendControl( m_tbGridSelectName );

    config.AppendSeparator()
          .AppendControl( m_tbZoomSelectName );

    // Option to run Zoom automatique on footprint selection change
    /* TODO (ISM): Convert to action
    m_tbTopMain->AddTool( ID_FPVIEWER_AUTOZOOM_TOOL, wxEmptyString,
            KiScaledBitmap( BITMAPS::zoom_auto_fit_in_page, this ),
            _( "Automatic Zoom on footprint change" ),
            wxITEM_CHECK );
*/

    // clang-format on
    return config;
}


std::optional<TOOLBAR_CONFIGURATION> FOOTPRINT_VIEWER_FRAME::DefaultLeftToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    config.AppendAction( ACTIONS::selectionTool )
          .AppendAction( ACTIONS::measureTool );

    config.AppendSeparator()
          .AppendAction( ACTIONS::toggleGrid )
          .AppendAction( ACTIONS::togglePolarCoords )
          .AppendAction( ACTIONS::inchesUnits )
          .AppendAction( ACTIONS::milsUnits )
          .AppendAction( ACTIONS::millimetersUnits )
          .AppendAction( ACTIONS::toggleCursorStyle );

    config.AppendSeparator()
          .AppendAction( PCB_ACTIONS::showPadNumbers )
          .AppendAction( PCB_ACTIONS::padDisplayMode )
          .AppendAction( PCB_ACTIONS::textOutlines )
          .AppendAction( PCB_ACTIONS::graphicsOutlines );

    if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
        config.AppendAction( ACTIONS::toggleBoundingBoxes );

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

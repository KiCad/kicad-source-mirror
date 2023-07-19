/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
* Copyright (C) 2023 CERN
* Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sim/simulator_frame.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <widgets/wx_menubar.h>
#include "menus_helpers.h"


void SIMULATOR_FRAME::ReCreateHToolbar()
{
    if( m_toolBar )
    {
        m_toolBar->ClearToolbar();
        m_toolBar->SetToolManager( GetToolManager() );
    }
    else
    {
        EDA_BASE_FRAME* parent = dynamic_cast<EDA_BASE_FRAME*>( this );

        wxCHECK( parent, /* void */ );

        m_toolBar = new ACTION_TOOLBAR( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                        KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
    }

    m_toolBar->Add( EE_ACTIONS::openWorkbook );
    m_toolBar->Add( EE_ACTIONS::saveWorkbook );

    m_toolBar->AddScaledSeparator( this );
    m_toolBar->Add( EE_ACTIONS::newAnalysisTab );
    m_toolBar->Add( EE_ACTIONS::simAnalysisProperties );

    m_toolBar->AddScaledSeparator( this );
    m_toolBar->Add( EE_ACTIONS::runSimulation );
    m_toolBar->Add( EE_ACTIONS::stopSimulation );

    m_toolBar->AddScaledSeparator( this );
    m_toolBar->Add( ACTIONS::zoomInCenter );
    m_toolBar->Add( ACTIONS::zoomOutCenter );
    m_toolBar->Add( ACTIONS::zoomFitScreen );

    m_toolBar->AddScaledSeparator( this );
    m_toolBar->Add( EE_ACTIONS::simProbe );
    m_toolBar->Add( EE_ACTIONS::simTune );

    m_toolBar->AddScaledSeparator( this );
    m_toolBar->Add( EE_ACTIONS::editUserDefinedSignals );
    m_toolBar->Add( EE_ACTIONS::showNetlist );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_toolBar->KiRealize();

    m_toolBar->Refresh();
}


void SIMULATOR_FRAME::doReCreateMenuBar()
{
    COMMON_CONTROL* tool = m_toolManager->GetTool<COMMON_CONTROL>();
    EDA_BASE_FRAME* base_frame = dynamic_cast<EDA_BASE_FRAME*>( this );

    // base_frame == nullptr should not happen, but it makes Coverity happy
    wxCHECK( base_frame, /* void */ );

    // wxWidgets handles the OSX Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = base_frame->GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, tool );

    fileMenu->Add( EE_ACTIONS::newAnalysisTab );

    fileMenu->AppendSeparator();
    fileMenu->Add( EE_ACTIONS::openWorkbook );
    fileMenu->Add( EE_ACTIONS::saveWorkbook );
    fileMenu->Add( EE_ACTIONS::saveWorkbookAs );

    fileMenu->AppendSeparator();
    fileMenu->Add( EE_ACTIONS::exportPlotAsPNG );
    fileMenu->Add( EE_ACTIONS::exportPlotAsCSV );

    fileMenu->AppendSeparator();
    fileMenu->AddClose( _( "Simulator" ) );


    //-- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, tool );

    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,               ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleLegend,          ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleDottedSecondary, ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleDarkModePlots,   ACTION_MENU::CHECK );


    //-- Simulation menu -----------------------------------------------------------
    //
    ACTION_MENU* simulationMenu = new ACTION_MENU( false, tool );

    simulationMenu->Add( EE_ACTIONS::newAnalysisTab );
    simulationMenu->Add( EE_ACTIONS::simAnalysisProperties );
    simulationMenu->Add( EE_ACTIONS::runSimulation );

    simulationMenu->AppendSeparator();
    simulationMenu->Add( EE_ACTIONS::simProbe );
    simulationMenu->Add( EE_ACTIONS::simTune );

    simulationMenu->AppendSeparator();
    simulationMenu->Add( EE_ACTIONS::editUserDefinedSignals );
    simulationMenu->Add( EE_ACTIONS::showNetlist );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, tool );

    //prefsMenu->Add( ACTIONS::configurePaths );
    //prefsMenu->Add( EE_ACTIONS::showSimLibTable );

    // We can't use ACTIONS::showPreferences yet because wxWidgets moves this on
    // Mac, and it needs the wxID_PREFERENCES id to find it.
    prefsMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    BITMAPS::preference );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, tool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( simulationMenu, _( "&Simulation" ) );
    menuBar->Append( prefsMenu, _( "&Preferences" ) );
    base_frame->AddStandardHelpMenu( menuBar );

    base_frame->SetMenuBar( menuBar );
    delete oldMenuBar;
}

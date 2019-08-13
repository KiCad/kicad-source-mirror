/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 Wayne Stambaugh <stambaughw@gmail.com>
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

#include "footprint_edit_frame.h"

#include <advanced_config.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tool/actions.h>
#include <tools/selection_tool.h>
#include <tools/pcb_actions.h>
#include "help_common_strings.h"
#include "pcbnew.h"
#include "pcbnew_id.h"


void FOOTPRINT_EDIT_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    auto modifiedDocumentCondition = [this]( const SELECTION& sel ) {
        return !GetBoard()->Modules().empty() && GetScreen()->IsModify();
    };
    auto haveFootprintCondition = [ this ] ( const SELECTION& aSelection ) {
        return GetBoard()->GetFirstModule() != nullptr;
    };
    auto footprintTargettedCondition = [ this ] ( const SELECTION& aSelection ) {
        return GetTargetFPID().IsValid();
    };

    //-- File menu ----------------------------------------------------------
    //
    CONDITIONAL_MENU* fileMenu = new CONDITIONAL_MENU( false, selTool );

    fileMenu->AddItem( ACTIONS::newLibrary,            SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::addLibrary,            SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( PCB_ACTIONS::newFootprint,      SELECTION_CONDITIONS::ShowAlways );
#ifdef KICAD_SCRIPTING
    fileMenu->AddItem( PCB_ACTIONS::createFootprint,   SELECTION_CONDITIONS::ShowAlways );
#endif

    fileMenu->AddSeparator();
    if( IsCurrentFPFromBoard() )
        fileMenu->AddItem( PCB_ACTIONS::saveToBoard,   modifiedDocumentCondition );
    else
        fileMenu->AddItem( PCB_ACTIONS::saveToLibrary, modifiedDocumentCondition );
    fileMenu->AddItem( ACTIONS::saveAs,                footprintTargettedCondition );
    fileMenu->AddItem( ACTIONS::revert,                modifiedDocumentCondition );

    fileMenu->AddSeparator();

    ACTION_MENU* submenuImport = new ACTION_MENU( false );
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );

    submenuImport->Add( PCB_ACTIONS::importFootprint );
    submenuImport->Add( _( "&Import Graphics..." ),
                        _( "Import 2D Drawing file to Footprint Editor on Drawings layer" ),
                        ID_GEN_IMPORT_GRAPHICS_FILE, import_vector_xpm );

    fileMenu->AddMenu( submenuImport,                SELECTION_CONDITIONS::ShowAlways );

    CONDITIONAL_MENU* submenuExport = new CONDITIONAL_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );

    submenuExport->AddItem( PCB_ACTIONS::exportFootprint, haveFootprintCondition );
    submenuExport->AddItem( ID_MODEDIT_SAVE_PNG, _( "Export View as &PNG..." ),
                            _( "Create a PNG file from the current view" ),
                            plot_xpm,               SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddMenu( submenuExport,               SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::print,              haveFootprintCondition );

    fileMenu->AddSeparator();
    fileMenu->AddClose( _( "Footprint Editor" ) );

    fileMenu->Resolve();

    //-- Edit menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetUndoCommandCount() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetRedoCommandCount() > 0;
    };
    auto noActiveToolCondition = [ this ] ( const SELECTION& aSelection ) {
        return ToolStackIsEmpty();
    };

    editMenu->AddItem( ACTIONS::undo,                     enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                     enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                      SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                     SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,                    noActiveToolCondition );
    editMenu->AddItem( ACTIONS::doDelete,                 SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::duplicate,                SELECTION_CONDITIONS::NotEmpty );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::footprintProperties,  haveFootprintCondition );
    editMenu->AddItem( PCB_ACTIONS::defaultPadProperties, SELECTION_CONDITIONS::ShowAlways );

    editMenu->Resolve();

    //-- View menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto polarCoordsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowPolarCoords();
    };
    auto imperialUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == INCHES;
    };
    auto metricUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == MILLIMETRES;
    };
    auto fullCrosshairCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalDisplayOptions().m_fullscreenCursor;
    };
    auto sketchPadsCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPadFill;
    };
    auto sketchEdgesCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayModEdgeFill;
    };
    auto contrastModeCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_ContrastModeDisplay;
    };
    auto searchTreeShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsSearchTreeShown();
    };

    viewMenu->AddItem( ACTIONS::showFootprintBrowser,       SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::show3DViewer,               SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,               SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,              SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,              SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                 SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,            gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,             SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddCheckItem( PCB_ACTIONS::togglePolarCoords, polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,     imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,       metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,     fullCrosshairCondition );

    viewMenu->AddSeparator();
    // Drawing Mode Submenu
    CONDITIONAL_MENU* drawingModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( add_zone_xpm );

    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::padDisplayMode,     sketchPadsCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::moduleEdgeOutlines, sketchEdgesCondition );
    viewMenu->AddMenu( drawingModeSubMenu );

    // Contrast Mode Submenu
    CONDITIONAL_MENU* contrastModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    contrastModeSubMenu->SetTitle( _( "&Contrast Mode" ) );
    contrastModeSubMenu->SetIcon( contrast_mode_xpm );

    contrastModeSubMenu->AddCheckItem( ACTIONS::highContrastMode, contrastModeCondition );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec,     SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc,     SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddMenu( contrastModeSubMenu );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( PCB_ACTIONS::toggleFootprintTree,     searchTreeShownCondition );

    viewMenu->Resolve();

    //-- Place menu -------------------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( PCB_ACTIONS::placePad,    haveFootprintCondition );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::placeText,   haveFootprintCondition );
    placeMenu->AddItem( PCB_ACTIONS::drawArc,     haveFootprintCondition );
    placeMenu->AddItem( PCB_ACTIONS::drawCircle,  haveFootprintCondition );
    placeMenu->AddItem( PCB_ACTIONS::drawLine,    haveFootprintCondition );
    placeMenu->AddItem( PCB_ACTIONS::drawPolygon, haveFootprintCondition );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::setAnchor,   haveFootprintCondition );
    placeMenu->AddItem( ACTIONS::gridSetOrigin,   haveFootprintCondition );

    placeMenu->Resolve();

    //-- Inspect menu ------------------------------------------------------
    //
    CONDITIONAL_MENU* inspectMenu = new CONDITIONAL_MENU( false, selTool );

    inspectMenu->AddItem( ACTIONS::measureTool,   haveFootprintCondition );
    inspectMenu->Resolve();

    //-- Tools menu --------------------------------------------------------
    //
    wxMenu* toolsMenu = new wxMenu;

    AddMenuItem( toolsMenu, ID_MODEDIT_LOAD_MODULE_FROM_BOARD,
                 _( "&Load Footprint from PCB..." ),
                 _( "Load a footprint from the current board into the editor" ),
                 KiBitmap( load_module_board_xpm ) );

    AddMenuItem( toolsMenu, ID_ADD_FOOTPRINT_TO_BOARD,
                 _( "&Insert Footprint on PCB" ),
                 _( "Insert footprint onto current board" ),
                 KiBitmap( insert_module_board_xpm ) );


    //-- Preferences menu -------------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, selTool );

    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    prefsMenu->AddItem( ACTIONS::configurePaths,             SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable,      SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                      SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    AddMenuLanguageList( prefsMenu, selTool );

    prefsMenu->AddSeparator();
    prefsMenu->AddCheckItem( ACTIONS::acceleratedGraphics,   acceleratedGraphicsCondition );
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics,      standardGraphicsCondition );

    prefsMenu->Resolve();

    //--MenuBar -----------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( prefsMenu, _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}

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
#include "hotkeys.h"
#include "pcbnew.h"
#include "pcbnew_id.h"


void FOOTPRINT_EDIT_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    auto modifiedDocumentCondition = [ this ] ( const SELECTION& sel ) {
        return GetBoard()->m_Modules && GetScreen()->IsModify();
    };
    auto libraryPartCondition = [ this ] ( const SELECTION& sel ) {
        LIB_ID libId = getTargetFPID();
        const wxString& libName = libId.GetLibNickname();
        const wxString& partName = libId.GetLibItemName();

       return( !libName.IsEmpty() || !partName.IsEmpty() );
    };

            //-- File menu -----------------------------------------------
    //
    CONDITIONAL_MENU* fileMenu = new CONDITIONAL_MENU( false, selTool );

    fileMenu->AddItem( ID_MODEDIT_CREATE_NEW_LIB,
                       _( "New Library..." ),
                       _( "Creates an empty library" ),
                       new_library_xpm,              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_MODEDIT_ADD_LIBRARY,
                       _( "Add Library..." ),
                       _( "Adds a previously created library" ),
                       add_library_xpm,              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_MODEDIT_NEW_MODULE,
                       AddHotkeyName( _( "&New Footprint..." ), m_hotkeysDescrList, HK_NEW ),
                       _( "Create a new footprint" ),
                       new_footprint_xpm,            SELECTION_CONDITIONS::ShowAlways );

#ifdef KICAD_SCRIPTING
    fileMenu->AddItem( ID_MODEDIT_NEW_MODULE_FROM_WIZARD,
                       _( "&Create Footprint..." ),
                       _( "Create a new footprint using the footprint wizard" ),
                       module_wizard_xpm,            SELECTION_CONDITIONS::ShowAlways );
#endif

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::save,                modifiedDocumentCondition );
    fileMenu->AddItem( ACTIONS::saveAs,              libraryPartCondition );
    fileMenu->AddItem( ID_MODEDIT_REVERT_PART,
                       _( "Revert" ),
                       _( "Throw away changes" ),
                       undo_xpm,                     modifiedDocumentCondition );

    fileMenu->AddSeparator();

    ACTION_MENU* submenuImport = new ACTION_MENU();
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );

    submenuImport->Add( _( "&Footprint..." ),
                        _( "Import a footprint from file" ),
                        ID_MODEDIT_IMPORT_PART, import_module_xpm );

    submenuImport->Add( _( "&Graphics..." ),
                        _( "Import 2D Drawing file to Footprint Editor on Drawings layer" ),
                        ID_GEN_IMPORT_GRAPHICS_FILE, import_vector_xpm );

    fileMenu->AddMenu( submenuImport,                SELECTION_CONDITIONS::ShowAlways );

    CONDITIONAL_MENU* submenuExport = new CONDITIONAL_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );

    submenuExport->AddItem( ID_MODEDIT_EXPORT_PART, _( "&Footprint..." ),
                            _( "Export current footprint to a file" ),
                            export_module_xpm,      modifiedDocumentCondition );

    submenuExport->AddItem( ID_MODEDIT_SAVE_PNG, _( "View as &PNG..." ),
                            _( "Create a PNG file from the current view" ),
                            plot_xpm,               SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddMenu( submenuExport,               SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::print,              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via wxID_EXIT
    fileMenu->AddItem( wxID_EXIT, _( "Quit" ), "", exit_xpm, SELECTION_CONDITIONS::ShowAlways );

    //-- Edit menu -----------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetUndoCommandCount() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetRedoCommandCount() > 0;
    };
    auto noActiveToolCondition = [ this ] ( const SELECTION& aSelection ) {
        return GetToolId() == ID_NO_TOOL_SELECTED;
    };

    editMenu->AddItem( ACTIONS::undo,    enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,    enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,     SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,    SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,   noActiveToolCondition );

    // Properties
    AddMenuItem( editMenu, ID_MODEDIT_EDIT_MODULE_PROPERTIES,
                 _( "&Footprint Properties..." ),
                 _( "Edit footprint properties" ),
                 KiBitmap( module_options_xpm ) );

    AddMenuItem( editMenu, ID_MODEDIT_PAD_SETTINGS,
                 _( "Default Pad Properties..." ),
                 _( "Edit default pad properties" ),
                 KiBitmap( options_pad_xpm ) );

    editMenu->AppendSeparator();

    AddMenuItem( editMenu, ID_MODEDIT_DELETE_PART,
                 _( "&Delete Footprint from Library" ),
                 _( "Delete the current footprint" ),
                 KiBitmap( delete_xpm ) );

    //-- View menu -----------------------------------------------
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

    viewMenu->AddItem( ID_OPEN_MODULE_VIEWER,
                       _( "Footprint &Library Browser" ),
                       _( "Browse footprint libraries" ),
                       modview_icon_xpm, SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_MENU_PCB_SHOW_3D_FRAME,
                       AddHotkeyName( _( "&3D Viewer" ), m_hotkeysDescrList, HK_3D_VIEWER ),
                       _( "Show footprint in 3D viewer" ),
                       three_d_xpm, SELECTION_CONDITIONS::ShowAlways );

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

    contrastModeSubMenu->AddCheckItem( PCB_ACTIONS::highContrastMode,   contrastModeCondition );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec, SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc, SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddMenu( contrastModeSubMenu );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ID_MODEDIT_SHOW_HIDE_SEARCH_TREE,
                            _( "&Search Tree" ), _( "Toggles the search tree visibility" ),
                            search_tree_xpm, searchTreeShownCondition );

    //-- Place menu -----------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( PCB_ACTIONS::placePad,    SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AppendSeparator();
    placeMenu->AddItem( PCB_ACTIONS::placeText,   SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawArc,     SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawCircle,  SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawLine,    SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawPolygon, SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AppendSeparator();
    placeMenu->AddItem( PCB_ACTIONS::setAnchor,   SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( ACTIONS::gridSetOrigin,   SELECTION_CONDITIONS::ShowAlways );


    //-- Inspect menu -----------------------------------------------
    //
    wxMenu* inspectMenu = new wxMenu;

    AddMenuItem( inspectMenu, ID_MODEDIT_MEASUREMENT_TOOL,
                 AddHotkeyName( _( "&Measure" ), m_hotkeysDescrList, HK_MEASURE_TOOL ),
                 _( "Measure distance" ),
                 KiBitmap( measurement_xpm ) );

    //-- Tools menu -----------------------------------------------
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


    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, selTool );

    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    prefsMenu->AddItem( ID_PREFERENCES_CONFIGURE_PATHS, _( "&Configure Paths..." ),
                        _( "Edit path configuration environment variables" ),
                        path_xpm,            SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddItem( ID_PCB_LIB_TABLE_EDIT, _( "Manage &Footprint Libraries..." ),
                        _( "Edit the global and project footprint library tables." ),
                        library_table_xpm,   SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddItem( wxID_PREFERENCES,
                        AddHotkeyName( _( "Preferences..." ), g_Module_Editor_Hotkeys_Descr, HK_PREFERENCES ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,      SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    Pgm().AddMenuLanguageList( prefsMenu );

    prefsMenu->AddSeparator();
    prefsMenu->AddCheckItem( ACTIONS::acceleratedGraphics, acceleratedGraphicsCondition );
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics, standardGraphicsCondition );

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

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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

#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_actions.h>
#include "eeschema_id.h"
#include "sch_edit_frame.h"

extern void AddMenuLanguageList( CONDITIONAL_MENU* aMasterMenu, TOOL_INTERACTIVE* aControlTool );


void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    auto modifiedDocumentCondition = [] ( const SELECTION& sel ) {
        SCH_SHEET_LIST sheetList( g_RootSheet );
        return sheetList.IsModified();
    };

    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, selTool );
    static ACTION_MENU* openRecentMenu;

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        // Add this menu to list menu managed by m_fileHistory
        // (the file history will be updated when adding/removing files in history)
        if( openRecentMenu )
            Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

        openRecentMenu = new ACTION_MENU( false );
        openRecentMenu->SetTool( selTool );
        openRecentMenu->SetTitle( _( "Open Recent" ) );
        openRecentMenu->SetIcon( recent_xpm );

        Kiface().GetFileHistory().UseMenu( openRecentMenu );
        Kiface().GetFileHistory().AddFilesToMenu( openRecentMenu );

        fileMenu->AddItem( ACTIONS::doNew,         EE_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ACTIONS::open,          EE_CONDITIONS::ShowAlways );
        fileMenu->AddMenu( openRecentMenu,         EE_CONDITIONS::ShowAlways );
        fileMenu->AddSeparator();
    }

    fileMenu->AddItem( ACTIONS::save,              modifiedDocumentCondition );
    fileMenu->AddItem( ACTIONS::saveAs,            EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::saveAll,           modifiedDocumentCondition );

    fileMenu->AddSeparator();

    fileMenu->AddItem( ID_APPEND_PROJECT, _( "Append Schematic Sheet Content..." ),
                       _( "Append schematic sheet content from another project to the current sheet" ),
                       add_document_xpm,           EE_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_IMPORT_NON_KICAD_SCH, _( "Import Non KiCad Schematic..." ),
                       _( "Replace current schematic sheet with one imported from another application" ),
                       import_document_xpm,        EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false );
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );
    submenuImport->Add( EE_ACTIONS::importFPAssignments );
    fileMenu->AddMenu( submenuImport,              EE_CONDITIONS::ShowAlways );


    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );
    submenuExport->Add( EE_ACTIONS::drawSheetOnClipboard );
    submenuExport->Add( EE_ACTIONS::exportNetlist );
    fileMenu->AddMenu( submenuExport,              EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::pageSettings,      EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::print,             EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::plot,              EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "Eeschema" ) );

    fileMenu->Resolve();

    //-- Edit menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetUndoCommandCount() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetRedoCommandCount() > 0;
    };

    editMenu->AddItem( ACTIONS::undo,                       enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                       enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                        EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                       EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,                      EE_CONDITIONS::Idle );
    editMenu->AddItem( ACTIONS::doDelete,                   EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::duplicate,                  EE_CONDITIONS::NotEmpty );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::find,                       EE_CONDITIONS::ShowAlways );
    editMenu->AddItem( ACTIONS::findAndReplace,             EE_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::deleteTool,                 EE_CONDITIONS::ShowAlways );
    editMenu->AddItem( EE_ACTIONS::editTextAndGraphics,     EE_CONDITIONS::ShowAlways );
    editMenu->AddItem( EE_ACTIONS::updateFieldsFromLibrary, EE_CONDITIONS::ShowAlways );

    editMenu->Resolve();

    //-- View menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };
    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
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
    auto hiddenPinsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowAllPins();
    };

    viewMenu->AddItem( ACTIONS::showSymbolBrowser,        EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EE_ACTIONS::navigateHierarchy,     EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EE_ACTIONS::leaveSheet,            belowRootSheetCondition );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                 EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,               EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,          gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,           EE_CONDITIONS::ShowAlways );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,   imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,     metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,   fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( EE_ACTIONS::toggleHiddenPins, hiddenPinsCondition );

#ifdef __APPLE__
    viewMenu->AddSeparator();
#endif

    viewMenu->Resolve();

    //-- Place menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( EE_ACTIONS::placeSymbol,            EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placePower,             EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawWire,               EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawBus,                EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeBusWireEntry,      EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeBusBusEntry,       EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeNoConnect,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeJunction,          EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeLabel,             EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeGlobalLabel,       EE_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( EE_ACTIONS::placeHierLabel, EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSheet,              EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::importSheetPin,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSheetPin,          EE_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( EE_ACTIONS::drawLines,              EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSchematicText,     EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeImage,             EE_CONDITIONS::ShowAlways );

    placeMenu->Resolve();

    //-- Inspect menu -----------------------------------------------
    //
    CONDITIONAL_MENU* inspectMenu = new CONDITIONAL_MENU( false, selTool );

    inspectMenu->AddItem( EE_ACTIONS::runERC,               EE_CONDITIONS::ShowAlways );
#ifdef KICAD_SPICE
    inspectMenu->AddItem( EE_ACTIONS::runSimulation,        EE_CONDITIONS::ShowAlways );
#endif

    inspectMenu->Resolve();

    //-- Tools menu -----------------------------------------------
    //
    CONDITIONAL_MENU* toolsMenu = new CONDITIONAL_MENU( false, selTool );

    auto remapSymbolsCondition = [] ( const SELECTION& aSel ) {
        SCH_SCREENS schematic;

        // The remapping can only be performed on legacy projects.
        return schematic.HasNoFullyDefinedLibIds();
    };

    toolsMenu->AddItem( ACTIONS::updatePcbFromSchematic,    EE_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( EE_ACTIONS::showPcbNew,             EE_CONDITIONS::ShowAlways );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( ACTIONS::showSymbolEditor,          EE_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( ID_RESCUE_CACHED, _( "Rescue Symbols..." ),
                        _( "Find old symbols in project and rename/rescue them" ),
                        rescue_xpm,                         EE_CONDITIONS::ShowAlways );

    toolsMenu->AddItem( ID_REMAP_SYMBOLS, _( "Remap Symbols..." ),
                        _( "Remap legacy library symbols to symbol library table" ),
                        rescue_xpm,                         remapSymbolsCondition );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( EE_ACTIONS::editSymbolFields,       EE_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( EE_ACTIONS::editSymbolLibraryLinks, EE_CONDITIONS::ShowAlways );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( EE_ACTIONS::annotate,               EE_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( EE_ACTIONS::showBusManager,         EE_CONDITIONS::ShowAlways );

    toolsMenu->AddSeparator();
    toolsMenu->AddItem( EE_ACTIONS::assignFootprints,       EE_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( EE_ACTIONS::generateBOM,            EE_CONDITIONS::ShowAlways );

    toolsMenu->Resolve();

    //-- Preferences menu -----------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, selTool );

    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    prefsMenu->AddItem( ACTIONS::configurePaths,           EE_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showSymbolLibTable,       EE_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                    EE_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    AddMenuLanguageList( prefsMenu, selTool );

    prefsMenu->AddSeparator();
    prefsMenu->AddCheckItem( ACTIONS::acceleratedGraphics, acceleratedGraphicsCondition );
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics,    standardGraphicsCondition );

    prefsMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
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



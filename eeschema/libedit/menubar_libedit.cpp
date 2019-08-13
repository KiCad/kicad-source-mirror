/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <lib_manager.h>
#include "eeschema_id.h"
#include "lib_edit_frame.h"


void LIB_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    auto modifiedDocumentCondition = [ this ] ( const SELECTION& sel ) {
        LIB_ID libId = getTargetLibId();
        const wxString& libName = libId.GetLibNickname();
        const wxString& partName = libId.GetLibItemName();
        bool readOnly = libName.IsEmpty() || m_libMgr->IsLibraryReadOnly( libName );

        if( partName.IsEmpty() )
            return ( !readOnly && m_libMgr->IsLibraryModified( libName ) );
        else
            return ( !readOnly && m_libMgr->IsPartModified( partName, libName ) );
    };

    //-- File menu -----------------------------------------------
    //
    CONDITIONAL_MENU* fileMenu = new CONDITIONAL_MENU( false, selTool );

    fileMenu->AddItem( ACTIONS::newLibrary,          EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::addLibrary,          EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( EE_ACTIONS::newSymbol,        EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::save,                modifiedDocumentCondition );
    fileMenu->AddItem( ACTIONS::saveCopyAs,          EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::saveAll,             EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::revert,              modifiedDocumentCondition );

    fileMenu->AddSeparator();
    fileMenu->AddItem( EE_ACTIONS::importSymbol,     EE_CONDITIONS::ShowAlways );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );
    submenuExport->Add( EE_ACTIONS::exportSymbol );
    submenuExport->Add( EE_ACTIONS::exportSymbolView );
    submenuExport->Add( EE_ACTIONS::exportSymbolAsSVG );
    fileMenu->AddMenu( submenuExport,              EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddClose( _( "Library Editor" ) );

    fileMenu->Resolve();

    //-- Edit menu -----------------------------------------------
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart() && GetScreen() && GetScreen()->GetUndoCommandCount() != 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart() && GetScreen() && GetScreen()->GetRedoCommandCount() != 0;
    };
    auto havePartCondition = [ this ] ( const SELECTION& sel ) {
        return GetCurPart();
    };

    editMenu->AddItem( ACTIONS::undo,                enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                 EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,               EE_CONDITIONS::Idle );
    editMenu->AddItem( ACTIONS::doDelete,            EE_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::duplicate,           EE_CONDITIONS::NotEmpty );

    editMenu->AddSeparator();
    editMenu->AddItem( EE_ACTIONS::symbolProperties, havePartCondition );
    editMenu->AddItem( EE_ACTIONS::pinTable,         havePartCondition );

    editMenu->Resolve();

    //-- View menu -----------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

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
    auto compTreeShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsSearchTreeShown();
    };

    viewMenu->AddItem( ACTIONS::showSymbolBrowser,         EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,              EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                  EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,           gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,            EE_CONDITIONS::ShowAlways );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,    imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,      metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,    fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( EE_ACTIONS::showComponentTree, compTreeShownCondition );

    viewMenu->Resolve();

    //-- Place menu -----------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( EE_ACTIONS::placeSymbolPin,        EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSymbolText,       EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolRectangle,   EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolCircle,      EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolArc,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSymbolLines,       EE_CONDITIONS::ShowAlways );

    placeMenu->Resolve();

    //-- Inspect menu -----------------------------------------------
    //
    CONDITIONAL_MENU* inspectMenu = new CONDITIONAL_MENU( false, selTool );

    auto datasheetAvailableCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCurPart() != nullptr;
    };

    inspectMenu->AddItem( EE_ACTIONS::showDatasheet,       datasheetAvailableCondition );
    inspectMenu->AddItem( EE_ACTIONS::runERC,              EE_CONDITIONS::ShowAlways );

    inspectMenu->Resolve();

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
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics, standardGraphicsCondition );

    prefsMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( prefsMenu, _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}

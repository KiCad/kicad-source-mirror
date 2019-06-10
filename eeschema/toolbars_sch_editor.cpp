/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <eeschema_id.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tools/ee_actions.h>

/* Create  the main Horizontal Toolbar for the schematic editor
 */
void SCH_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR,
                                            wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
    wxString msg;

    // Set up toolbar
    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        m_mainToolBar->Add( ACTIONS::doNew );
        m_mainToolBar->Add( ACTIONS::open );
    }

    m_mainToolBar->Add( ACTIONS::saveAll );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::pageSettings );
    m_mainToolBar->Add( ACTIONS::print );
    m_mainToolBar->Add( ACTIONS::plot );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::paste );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::find );
    m_mainToolBar->Add( ACTIONS::findAndReplace );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( EE_ACTIONS::navigateHierarchy );
    m_mainToolBar->Add( EE_ACTIONS::leaveSheet );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::showSymbolEditor );
    m_mainToolBar->Add( ACTIONS::showSymbolBrowser );
    m_mainToolBar->Add( ACTIONS::showFootprintEditor );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( EE_ACTIONS::annotate );
    m_mainToolBar->Add( EE_ACTIONS::runERC );
    m_mainToolBar->Add( EE_ACTIONS::assignFootprints );
    m_mainToolBar->Add( EE_ACTIONS::editSymbolFields );
    m_mainToolBar->Add( EE_ACTIONS::generateBOM );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( EE_ACTIONS::showPcbNew );

    m_mainToolBar->AddTool( ID_BACKANNO_ITEMS, wxEmptyString,
                            KiScaledBitmap( import_footprint_names_xpm, this ),
                            _( "Back-import symbol footprint associations from .cmp file created by Pcbnew" ) );

    // after adding the tools to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


/* Create Vertical Right Toolbar
 */
void SCH_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->Add( EE_ACTIONS::selectionTool,          ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::highlightNetCursor,     ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbol,            ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placePower,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawWire,               ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawBus,                ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeBusWireEntry,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeBusBusEntry,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeNoConnect,         ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeJunction,          ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeLabel,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeGlobalLabel,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeHierarchicalLabel, ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSheet,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::importSheetPin,         ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSheetPin,          ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( EE_ACTIONS::drawLines,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSchematicText,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeImage,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::deleteItemCursor,       ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->Realize();
}


/* Create Vertical Left Toolbar (Option Toolbar)
 */
void SCH_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR,
                                               wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->Add( ACTIONS::toggleGrid,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::imperialUnits,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::toggleHiddenPins, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::toggleForceHV,    ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->Realize();
}


void SCH_EDIT_FRAME::SyncToolbars()
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOpts = GetGalDisplayOptions();
    SCH_SHEET_LIST              sheetList( g_RootSheet );

    m_mainToolBar->Toggle( ACTIONS::saveAll, sheetList.IsModified() );
    m_mainToolBar->Toggle( ACTIONS::undo, GetScreen() && GetScreen()->GetUndoCommandCount() > 0 );
    m_mainToolBar->Toggle( ACTIONS::redo, GetScreen() && GetScreen()->GetRedoCommandCount() > 0 );
    m_mainToolBar->Toggle( ACTIONS::zoomTool, GetToolId() == ID_ZOOM_SELECTION );
    m_mainToolBar->Refresh();

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid,             IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits,            GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits,          GetUserUnits() == INCHES );
    m_optionsToolBar->Toggle( ACTIONS::toggleCursorStyle,      galOpts.m_fullscreenCursor );
    m_optionsToolBar->Toggle( EE_ACTIONS::toggleHiddenPins,    GetShowAllPins() );
    m_optionsToolBar->Toggle( EE_ACTIONS::toggleForceHV,       GetForceHVLines() );
    m_optionsToolBar->Refresh();

    m_drawToolBar->Toggle( EE_ACTIONS::selectionTool,          GetToolId() == ID_NO_TOOL_SELECTED );
    m_drawToolBar->Toggle( EE_ACTIONS::highlightNetCursor,     GetToolId() == ID_HIGHLIGHT_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSymbol,            GetToolId() == ID_COMPONENT_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::placePower,             GetToolId() == ID_PLACE_POWER_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::drawWire,               GetToolId() == ID_WIRE_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::drawBus,                GetToolId() == ID_BUS_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeBusWireEntry,      GetToolId() == ID_WIRETOBUS_ENTRY_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeBusBusEntry,       GetToolId() == ID_BUSTOBUS_ENTRY_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeNoConnect,         GetToolId() == ID_NOCONNECT_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeJunction,          GetToolId() == ID_JUNCTION_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeLabel,             GetToolId() == ID_LABEL_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeGlobalLabel,       GetToolId() == ID_GLOBALLABEL_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeHierarchicalLabel, GetToolId() == ID_HIERLABEL_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::drawSheet,              GetToolId() == ID_SHEET_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::importSheetPin,         GetToolId() == ID_IMPORT_SHEETPIN_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSheetPin,          GetToolId() == ID_SHEETPIN_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::drawLines,              GetToolId() == ID_SCHEMATIC_LINE_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSchematicText,     GetToolId() == ID_SCHEMATIC_TEXT_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::placeImage,             GetToolId() == ID_PLACE_IMAGE_TOOL );
    m_drawToolBar->Toggle( EE_ACTIONS::deleteItemCursor,       GetToolId() == ID_DELETE_TOOL );
    m_drawToolBar->Refresh();
}

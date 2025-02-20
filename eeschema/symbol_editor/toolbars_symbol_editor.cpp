/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <eeschema_id.h>
#include <symbol_edit_frame.h>
#include <sch_painter.h>
#include <symbol_editor_settings.h>
#include <symbol_library_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/wx_aui_utils.h>

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


std::optional<TOOLBAR_CONFIGURATION> SYMBOL_EDIT_FRAME::DefaultLeftToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    config.AppendAction( ACTIONS::toggleGrid )
          .AppendAction( ACTIONS::toggleGridOverrides )
          .AppendAction( ACTIONS::inchesUnits )
          .AppendAction( ACTIONS::milsUnits )
          .AppendAction( ACTIONS::millimetersUnits )
          .AppendAction( ACTIONS::toggleCursorStyle );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showElectricalTypes )
          .AppendAction( EE_ACTIONS::showHiddenPins )
          .AppendAction( EE_ACTIONS::showHiddenFields );
    //    .AppendAction( EE_ACTIONS::togglePinAltIcons );

    if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
        config.AppendAction( ACTIONS::toggleBoundingBoxes );

    config.AppendSeparator()
          .AppendAction( ACTIONS::showLibraryTree )
          .AppendAction( ACTIONS::showProperties );

    /* TODO: Implement context menus
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_tbLeft->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );
    */

    // clang-format on
    return config;
}


std::optional<TOOLBAR_CONFIGURATION> SYMBOL_EDIT_FRAME::DefaultRightToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    config.AppendAction( ACTIONS::selectionTool );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::placeSymbolPin )
          .AppendAction( EE_ACTIONS::placeSymbolText )
          .AppendAction( EE_ACTIONS::drawSymbolTextBox )
          .AppendAction( EE_ACTIONS::drawRectangle )
          .AppendAction( EE_ACTIONS::drawCircle )
          .AppendAction( EE_ACTIONS::drawArc )
          .AppendAction( EE_ACTIONS::drawBezier )
          .AppendAction( EE_ACTIONS::drawSymbolLines )
          .AppendAction( EE_ACTIONS::drawSymbolPolygon )
          .AppendAction( EE_ACTIONS::placeSymbolAnchor )
          .AppendAction( ACTIONS::deleteTool);

    // clang-format on
    return config;
}


std::optional<TOOLBAR_CONFIGURATION> SYMBOL_EDIT_FRAME::DefaultTopMainToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    config.AppendAction( EE_ACTIONS::newSymbol );

    if( !IsSymbolFromSchematic() )
        config.AppendAction( ACTIONS::saveAll );
    else
        config.AppendAction( ACTIONS::save );

    config.AppendSeparator()
          .AppendAction( ACTIONS::undo )
          .AppendAction( ACTIONS::redo );

    config.AppendSeparator()
          .AppendAction( ACTIONS::zoomRedraw )
          .AppendAction( ACTIONS::zoomInCenter )
          .AppendAction( ACTIONS::zoomOutCenter )
          .AppendAction( ACTIONS::zoomFitScreen )
          .AppendAction( ACTIONS::zoomTool );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::rotateCCW )
          .AppendAction( EE_ACTIONS::rotateCW )
          .AppendAction( EE_ACTIONS::mirrorV )
          .AppendAction( EE_ACTIONS::mirrorH );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::symbolProperties )
          .AppendAction( EE_ACTIONS::pinTable );

    config.AppendSeparator()
          .AppendAction( ACTIONS::showDatasheet )
          .AppendAction( EE_ACTIONS::checkSymbol );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showDeMorganStandard )
          .AppendAction( EE_ACTIONS::showDeMorganAlternate );

    config.AppendSeparator()
          .AppendControl( "control.SymEditUnitSelector" );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::toggleSyncedPinsMode );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::addSymbolToSchematic );

    // clang-format on
    return config;
}


void SYMBOL_EDIT_FRAME::configureToolbars()
{
    SCH_BASE_FRAME::configureToolbars();

    auto unitDisplayFactory = [this]( ACTION_TOOLBAR* aToolbar )
        {
            if( !m_unitSelectBox )
            {
                m_unitSelectBox = new wxComboBox( aToolbar, ID_LIBEDIT_SELECT_UNIT_NUMBER,
                                                  wxEmptyString, wxDefaultPosition,
                                                  wxSize( LISTBOX_WIDTH, -1 ), 0,
                                                  nullptr, wxCB_READONLY );
            }

            aToolbar->Add( m_unitSelectBox );
        };

    RegisterCustomToolbarControlFactory("control.SymEditUnitSelector", _( "Unit number display" ),
                            _( "Displays the unit being currently edited" ),
                                         unitDisplayFactory );
}

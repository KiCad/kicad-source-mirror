/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <api/api_plugin_manager.h>
#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <kiface_base.h>
#include <bitmaps.h>
#include <eeschema_id.h>
#include <pgm_base.h>
#include <python_scripting.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <widgets/design_block_pane.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>


std::optional<TOOLBAR_CONFIGURATION> SCH_EDIT_FRAME::DefaultLeftToolbarConfig()
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
          .AppendAction( EE_ACTIONS::toggleHiddenPins );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::lineModeFree )
          .AppendAction( EE_ACTIONS::lineMode90 )
          .AppendAction( EE_ACTIONS::lineMode45 );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::toggleAnnotateAuto );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showHierarchy )
          .AppendAction( ACTIONS::showProperties );

    if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
        config.AppendAction( ACTIONS::toggleBoundingBoxes );

    /* TODO (ISM): Handle context menus
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_tbLeft->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );
    */

    // clang-format on
    return config;
}


std::optional<TOOLBAR_CONFIGURATION> SCH_EDIT_FRAME::DefaultRightToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    config.AppendAction( ACTIONS::selectionTool )
          .AppendAction( EE_ACTIONS::highlightNetTool );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::placeSymbol )
          .AppendAction( EE_ACTIONS::placePower )
          .AppendAction( EE_ACTIONS::drawWire )
          .AppendAction( EE_ACTIONS::drawBus )
          .AppendAction( EE_ACTIONS::placeBusWireEntry )
          .AppendAction( EE_ACTIONS::placeNoConnect )
          .AppendAction( EE_ACTIONS::placeJunction )
          .AppendAction( EE_ACTIONS::placeLabel )
          .AppendAction( EE_ACTIONS::placeClassLabel )
          .AppendAction( EE_ACTIONS::drawRuleArea )
          .AppendAction( EE_ACTIONS::placeGlobalLabel )
          .AppendAction( EE_ACTIONS::placeHierLabel )
          .AppendAction( EE_ACTIONS::drawSheet )
          .AppendAction( EE_ACTIONS::placeSheetPin )
          .AppendAction( EE_ACTIONS::syncAllSheetsPins );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::placeSchematicText )
          .AppendAction( EE_ACTIONS::drawTextBox )
          .AppendAction( EE_ACTIONS::drawTable )
          .AppendAction( EE_ACTIONS::drawRectangle )
          .AppendAction( EE_ACTIONS::drawCircle )
          .AppendAction( EE_ACTIONS::drawArc )
          .AppendAction( EE_ACTIONS::drawBezier )
          .AppendAction( EE_ACTIONS::drawLines )
          .AppendAction( EE_ACTIONS::placeImage )
          .AppendAction( ACTIONS::deleteTool );

    // clang-format on
    return config;
}


std::optional<TOOLBAR_CONFIGURATION> SCH_EDIT_FRAME::DefaultTopMainToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        config.AppendAction( ACTIONS::doNew );
        config.AppendAction( ACTIONS::open );
    }

    config.AppendAction( ACTIONS::save );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::schematicSetup );

    config.AppendSeparator()
          .AppendAction( ACTIONS::pageSettings )
          .AppendAction( ACTIONS::print )
          .AppendAction( ACTIONS::plot );

    config.AppendSeparator()
          .AppendAction( ACTIONS::paste );

    config.AppendSeparator()
          .AppendAction( ACTIONS::undo )
          .AppendAction( ACTIONS::redo );

    config.AppendSeparator()
          .AppendAction( ACTIONS::find )
          .AppendAction( ACTIONS::findAndReplace );

    config.AppendSeparator()
          .AppendAction( ACTIONS::zoomRedraw )
          .AppendAction( ACTIONS::zoomInCenter )
          .AppendAction( ACTIONS::zoomOutCenter )
          .AppendAction( ACTIONS::zoomFitScreen )
          .AppendAction( ACTIONS::zoomFitObjects )
          .AppendAction( ACTIONS::zoomTool );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::navigateBack )
          .AppendAction( EE_ACTIONS::navigateUp )
          .AppendAction( EE_ACTIONS::navigateForward );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::rotateCCW )
          .AppendAction( EE_ACTIONS::rotateCW )
          .AppendAction( EE_ACTIONS::mirrorV )
          .AppendAction( EE_ACTIONS::mirrorH );

    config.AppendSeparator()
          .AppendAction( ACTIONS::showSymbolEditor )
          .AppendAction( ACTIONS::showSymbolBrowser )
          .AppendAction( ACTIONS::showFootprintEditor );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::annotate )
          .AppendAction( EE_ACTIONS::runERC )
          .AppendAction( EE_ACTIONS::showSimulator )
          .AppendAction( EE_ACTIONS::assignFootprints )
          .AppendAction( EE_ACTIONS::editSymbolFields )
          .AppendAction( EE_ACTIONS::generateBOM );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showPcbNew );

    // Insert all the IPC plugins here on the toolbar
    config.AppendControl( "control.SCHPlugin" );

    // clang-format on
    return config;
}

void SCH_EDIT_FRAME::configureToolbars()
{
    SCH_BASE_FRAME::configureToolbars();

    // IPC/Scripting plugin control
    // TODO (ISM): Clean this up to make IPC actions just normal tool actions to get rid of this entire
    // control
    auto pluginControlFactory = [this]( ACTION_TOOLBAR* aToolbar )
        {
            // Add scripting console and API plugins
            bool scriptingAvailable = SCRIPTING::IsWxAvailable();

            #ifdef KICAD_IPC_API
            bool haveApiPlugins = Pgm().GetCommonSettings()->m_Api.enable_server &&
                    !Pgm().GetPluginManager().GetActionsForScope( PluginActionScope() ).empty();
            #else
            bool haveApiPlugins = false;
            #endif

            if( scriptingAvailable || haveApiPlugins )
            {
                aToolbar->AddScaledSeparator( aToolbar->GetParent() );

                if( haveApiPlugins )
                    AddApiPluginTools( aToolbar );
            }
        };

    RegisterCustomToolbarControlFactory( "control.SCHPlugin", _( "IPC/Scripting plugins" ),
                                         _( "Region to hold the IPC/Scripting action buttons" ),
                                         pluginControlFactory );

}
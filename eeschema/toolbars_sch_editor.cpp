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
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <widgets/design_block_pane.h>
#include <widgets/hierarchy_pane.h>
#include <widgets/wx_aui_utils.h>
#include <widgets/sch_properties_panel.h>
#include <widgets/sch_search_pane.h>
#include <toolbars_sch_editor.h>


std::optional<TOOLBAR_CONFIGURATION> SCH_EDIT_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::toggleGrid )
              .AppendAction( ACTIONS::toggleGridOverrides )
              .AppendAction( ACTIONS::inchesUnits )
              .AppendAction( ACTIONS::milsUnits )
              .AppendAction( ACTIONS::millimetersUnits )
              .AppendAction( ACTIONS::toggleCursorStyle );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::toggleHiddenPins );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::lineModeFree )
              .AppendAction( SCH_ACTIONS::lineMode90 )
              .AppendAction( SCH_ACTIONS::lineMode45 );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::toggleAnnotateAuto );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::showHierarchy )
              .AppendAction( ACTIONS::showProperties );

        if( ADVANCED_CFG::GetCfg().m_DrawBoundingBoxes )
            config.AppendAction( ACTIONS::toggleBoundingBoxes );

        /* TODO (ISM): Handle context menus
        EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
        std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
        gridMenu->Add( ACTIONS::gridProperties );
        m_tbLeft->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );
        */
        break;

    case TOOLBAR_LOC::RIGHT:
        config.AppendAction( ACTIONS::selectionTool )
              .AppendAction( SCH_ACTIONS::highlightNetTool );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::placeSymbol )
              .AppendAction( SCH_ACTIONS::placePower )
              .AppendAction( SCH_ACTIONS::drawWire )
              .AppendAction( SCH_ACTIONS::drawBus )
              .AppendAction( SCH_ACTIONS::placeBusWireEntry )
              .AppendAction( SCH_ACTIONS::placeNoConnect )
              .AppendAction( SCH_ACTIONS::placeJunction )
              .AppendAction( SCH_ACTIONS::placeLabel )
              .AppendAction( SCH_ACTIONS::placeClassLabel )
              .AppendAction( SCH_ACTIONS::drawRuleArea )
              .AppendAction( SCH_ACTIONS::placeGlobalLabel )
              .AppendAction( SCH_ACTIONS::placeHierLabel )
              .AppendAction( SCH_ACTIONS::drawSheet )
              .AppendAction( SCH_ACTIONS::placeSheetPin )
              .AppendAction( SCH_ACTIONS::syncAllSheetsPins );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::placeSchematicText )
              .AppendAction( SCH_ACTIONS::drawTextBox )
              .AppendAction( SCH_ACTIONS::drawTable )
              .AppendAction( SCH_ACTIONS::drawRectangle )
              .AppendAction( SCH_ACTIONS::drawCircle )
              .AppendAction( SCH_ACTIONS::drawArc )
              .AppendAction( SCH_ACTIONS::drawBezier )
              .AppendAction( SCH_ACTIONS::drawLines )
              .AppendAction( SCH_ACTIONS::placeImage )
              .AppendAction( ACTIONS::deleteTool );

        break;

    case TOOLBAR_LOC::TOP_MAIN:
        if( Kiface().IsSingle() )   // not when under a project mgr
        {
            config.AppendAction( ACTIONS::doNew );
            config.AppendAction( ACTIONS::open );
        }

        config.AppendAction( ACTIONS::save );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::schematicSetup );

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
              .AppendAction( SCH_ACTIONS::navigateBack )
              .AppendAction( SCH_ACTIONS::navigateUp )
              .AppendAction( SCH_ACTIONS::navigateForward );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::rotateCCW )
              .AppendAction( SCH_ACTIONS::rotateCW )
              .AppendAction( SCH_ACTIONS::mirrorV )
              .AppendAction( SCH_ACTIONS::mirrorH );

        config.AppendSeparator()
              .AppendAction( ACTIONS::showSymbolEditor )
              .AppendAction( ACTIONS::showSymbolBrowser )
              .AppendAction( ACTIONS::showFootprintEditor );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::annotate )
              .AppendAction( SCH_ACTIONS::runERC )
              .AppendAction( SCH_ACTIONS::showSimulator )
              .AppendAction( SCH_ACTIONS::assignFootprints )
              .AppendAction( SCH_ACTIONS::editSymbolFields )
              .AppendAction( SCH_ACTIONS::generateBOM );

        config.AppendSeparator()
              .AppendAction( SCH_ACTIONS::showPcbNew );

        // Insert all the IPC plugins here on the toolbar
        // TODO (ISM): Move this to individual actions for each script
        config.AppendControl( ACTION_TOOLBAR_CONTROLS::ipcScripting );

        break;
    }

    // clang-format on
    return config;
}


void SCH_EDIT_FRAME::configureToolbars()
{
    SCH_BASE_FRAME::configureToolbars();

    // IPC/Scripting plugin control
    // TODO (ISM): Clean this up to make IPC actions just normal tool actions to get rid of this entire
    // control
    auto pluginControlFactory =
        [this]( ACTION_TOOLBAR* aToolbar )
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

    RegisterCustomToolbarControlFactory( ACTION_TOOLBAR_CONTROLS::ipcScripting, pluginControlFactory );

}
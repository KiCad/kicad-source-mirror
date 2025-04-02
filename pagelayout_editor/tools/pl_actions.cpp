/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
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

#include <tool/tool_action.h>
#include <bitmaps.h>
#include <eda_item.h>
#include <drawing_sheet/ds_data_item.h>

#include "tools/pl_actions.h"

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// PL_DRAWING_TOOLS
//
TOOL_ACTION PL_ACTIONS::drawLine( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveDrawing.drawLine" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Lines" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::add_graphical_segments )
        .Flags( AF_ACTIVATE )
        .Parameter( DS_DATA_ITEM::DS_SEGMENT ) );

TOOL_ACTION PL_ACTIONS::drawRectangle( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveDrawing.drawRectangle" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Rectangles" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::add_rectangle )
        .Flags( AF_ACTIVATE )
        .Parameter( DS_DATA_ITEM::DS_RECT ) );

TOOL_ACTION PL_ACTIONS::placeText( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveDrawing.placeText" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Text" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::text )
        .Flags( AF_ACTIVATE )
        .Parameter( DS_DATA_ITEM::DS_TEXT ) );

TOOL_ACTION PL_ACTIONS::placeImage( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveDrawing.placeImage" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Place Bitmaps" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::image )
        .Flags( AF_ACTIVATE )
        .Parameter( DS_DATA_ITEM::DS_BITMAP ) );


// PL_EDIT_TOOL
//
TOOL_ACTION PL_ACTIONS::move( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveMove.move" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'M' )
        .LegacyHotkeyName( "Move Item" )
        .FriendlyName( _( "Move" ) )
        .Icon( BITMAPS::move )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION PL_ACTIONS::appendImportedDrawingSheet( TOOL_ACTION_ARGS()
        .Name( "plEditor.InteractiveEdit.appendWorksheet" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Append Existing Drawing Sheet..." ) )
        .Tooltip( _( "Append an existing drawing sheet file to the current file" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::import )
        .Flags( AF_ACTIVATE ) );


// PL_EDITOR_CONTROL
//
TOOL_ACTION PL_ACTIONS::showInspector( TOOL_ACTION_ARGS()
        .Name( "plEditor.EditorControl.ShowInspector" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Design Inspector" ) )
        .Icon( BITMAPS::spreadsheet ) );

TOOL_ACTION PL_ACTIONS::previewSettings( TOOL_ACTION_ARGS()
        .Name( "plEditor.EditorControl.PreviewSettings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Page Preview Settings..." ) )
        .Tooltip( _( "Edit preview data for page size and title block" ) )
        .Icon( BITMAPS::sheetset ) );

TOOL_ACTION PL_ACTIONS::layoutNormalMode( TOOL_ACTION_ARGS()
        .Name( "plEditor.EditorControl.LayoutNormalMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Title Block in Preview Mode" ) )
        .Tooltip( _( "Text placeholders will be replaced with preview data" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::pagelayout_normal_view_mode ) );

TOOL_ACTION PL_ACTIONS::layoutEditMode( TOOL_ACTION_ARGS()
        .Name( "plEditor.EditorControl.LayoutEditMode" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Title Block in Edit Mode" ) )
        .Tooltip( _( "Text placeholders are shown as ${keyword} tokens" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::pagelayout_special_view_mode  ) );

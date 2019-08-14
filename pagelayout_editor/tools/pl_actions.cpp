/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tools/pl_actions.h>
#include <ws_data_item.h>


// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// PL_DRAWING_TOOLS
//
TOOL_ACTION PL_ACTIONS::drawLine( "plEditor.InteractiveDrawing.drawLine",
        AS_GLOBAL, 0, "",
        _( "Add Line" ), _( "Add a line" ),
        add_graphical_segments_xpm, AF_ACTIVATE, (void*) WS_DATA_ITEM::WS_SEGMENT );

TOOL_ACTION PL_ACTIONS::drawRectangle( "plEditor.InteractiveDrawing.drawRectangle",
        AS_GLOBAL, 0, "",
        _( "Add Rectangle" ), _( "Add a rectangle" ),
        add_rectangle_xpm, AF_ACTIVATE, (void*) WS_DATA_ITEM::WS_RECT );

TOOL_ACTION PL_ACTIONS::placeText( "plEditor.InteractiveDrawing.placeText",
        AS_GLOBAL, 0, "",
        _( "Add Text" ), _( "Add a text item" ),
        text_xpm, AF_ACTIVATE, (void*) WS_DATA_ITEM::WS_TEXT );

TOOL_ACTION PL_ACTIONS::placeImage( "plEditor.InteractiveDrawing.placeImage",
        AS_GLOBAL, 0, "",
        _( "Add Bitmap" ), _( "Add a bitmap image" ),
        image_xpm, AF_ACTIVATE, (void*) WS_DATA_ITEM::WS_BITMAP );


// PL_EDIT_TOOL
//
TOOL_ACTION PL_ACTIONS::move( "plEditor.InteractiveMove.move",
        AS_GLOBAL,
        'M', LEGACY_HK_NAME( "Move Item" ),
        _( "Move" ), _( "Moves the selected item(s)" ),
        move_xpm, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::appendImportedWorksheet( "plEditor.InteractiveEdit.appendWorksheet",
        AS_GLOBAL, 0, "",
        _( "Append Existing Page Layout File..." ),
        _( "Append an existing page layout design file to current file" ),
        import_xpm, AF_ACTIVATE );


// PL_EDITOR_CONTROL
//
TOOL_ACTION PL_ACTIONS::toggleBackground( "plEditor.EditorControl.ToggleBackground",
        AS_GLOBAL, 0, "",
        _( "Background White" ), _( "Switch between white and black background" ),
        palette_xpm );

TOOL_ACTION PL_ACTIONS::showInspector( "plEditor.EditorControl.ShowInspector",
        AS_GLOBAL, 0, "",
        _( "Show Design Inspector" ), _( "Show the list of items in page layout" ),
        spreadsheet_xpm );

TOOL_ACTION PL_ACTIONS::previewSettings( "plEditor.EditorControl.PreviewSettings",
        AS_GLOBAL, 0, "",
        _( "Page Preview Settings..." ), _( "Edit preview data for page size and title block" ),
        sheetset_xpm );


// PL_SELECTION_TOOL
//
TOOL_ACTION PL_ACTIONS::selectionActivate( "plEditor.InteractiveSelection",
        AS_GLOBAL, 0, "", "", "",       // No description, not shown anywhere
        nullptr, AF_ACTIVATE );

TOOL_ACTION PL_ACTIONS::selectionMenu( "plEditor.InteractiveSelection.SelectionMenu",
        AS_GLOBAL );

TOOL_ACTION PL_ACTIONS::addItemToSel( "plEditor.InteractiveSelection.AddItemToSel",
        AS_GLOBAL );

TOOL_ACTION PL_ACTIONS::addItemsToSel( "plEditor.InteractiveSelection.AddItemsToSel",
        AS_GLOBAL );

TOOL_ACTION PL_ACTIONS::removeItemFromSel( "plEditor.InteractiveSelection.RemoveItemFromSel",
        AS_GLOBAL );

TOOL_ACTION PL_ACTIONS::removeItemsFromSel( "plEditor.InteractiveSelection.RemoveItemsFromSel",
        AS_GLOBAL );

TOOL_ACTION PL_ACTIONS::clearSelection( "plEditor.InteractiveSelection.ClearSelection",
        AS_GLOBAL );



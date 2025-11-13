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

#include <bitmaps.h>
#include <common.h>
#include <eda_units.h>
#include <frame_type.h>
#include <settings/app_settings.h>
#include <tool/actions.h>
#include <tool/tool_action.h>
#include <tool/tool_event.h>
#include <tool/selection_tool.h>

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

TOOL_ACTION ACTIONS::doNew( TOOL_ACTION_ARGS()
        .Name( "common.Control.new" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'N' )
        .LegacyHotkeyName( "New" )
        .FriendlyName( _( "New..." ) )
        .Tooltip( _( "Create a new document in the editor" ) )
        .Icon( BITMAPS::new_generic ) );

TOOL_ACTION ACTIONS::newLibrary( TOOL_ACTION_ARGS()
        .Name( "common.Control.newLibrary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "New Library..." ) )
        .Tooltip( _( "Create a new library folder" ) )
        .Icon( BITMAPS::new_library ) );

TOOL_ACTION ACTIONS::addLibrary( TOOL_ACTION_ARGS()
        .Name( "common.Control.addLibrary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Library..." ) )
        .Tooltip( _( "Add an existing library folder" ) )
        .Icon( BITMAPS::add_library ) );

TOOL_ACTION ACTIONS::open( TOOL_ACTION_ARGS()
        .Name( "common.Control.open" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'O' )
        .LegacyHotkeyName( "Open" )
        .FriendlyName( _( "Open..." ) )
        .Tooltip( _( "Open existing document" ) )
        .Icon( BITMAPS::directory_open ) );

TOOL_ACTION ACTIONS::openWithTextEditor( TOOL_ACTION_ARGS()
        .Name( "common.Control.openWithTextEditor" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit in a Text Editor..." ) )
        .Tooltip( _( "Open a library file with a text editor" ) )
        .Icon( BITMAPS::editor ) );

TOOL_ACTION ACTIONS::openDirectory( TOOL_ACTION_ARGS()
        .Name( "common.Control.openDirectory" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Open in file explorer..." ) )
        .Tooltip( _( "Open a library file with system file explorer" ) )
        .Icon( BITMAPS::directory_browser ) );

TOOL_ACTION ACTIONS::save( TOOL_ACTION_ARGS()
        .Name( "common.Control.save" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'S' )
        .LegacyHotkeyName( "Save" )
        .FriendlyName( _( "Save" ) )
        .Tooltip( _( "Save changes" ) )
        .Icon( BITMAPS::save ) );

TOOL_ACTION ACTIONS::saveAs( TOOL_ACTION_ARGS()
        .Name( "common.Control.saveAs" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'S' )
        .LegacyHotkeyName( "Save As" )
        .FriendlyName( _( "Save As..." ) )
        .Tooltip( _( "Save current document to another location" ) )
        .Icon( BITMAPS::save_as ) );

TOOL_ACTION ACTIONS::saveCopy( TOOL_ACTION_ARGS()
        .Name( "common.Control.saveCopy" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Save a Copy..." ) )
        .Tooltip( _( "Save a copy of the current document to another location" ) )
        .Icon( BITMAPS::save_as ) );

TOOL_ACTION ACTIONS::saveAll( TOOL_ACTION_ARGS()
        .Name( "common.Control.saveAll" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Save All" ) )
        .Tooltip( _( "Save all changes" ) )
        .Icon( BITMAPS::save ) );

TOOL_ACTION ACTIONS::revert( TOOL_ACTION_ARGS()
        .Name( "common.Control.revert" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Revert" ) )
        .Tooltip( _( "Throw away changes" ) )
        .Icon( BITMAPS::restore_from_file )
        );

TOOL_ACTION ACTIONS::pageSettings( TOOL_ACTION_ARGS()
        .Name( "common.Control.pageSettings" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Page Settings..." ) )
        .Tooltip( _( "Settings for paper size and title block info" ) )
        .Icon( BITMAPS::sheetset ) );

TOOL_ACTION ACTIONS::print( TOOL_ACTION_ARGS()
        .Name( "common.Control.print" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'P' )
        .LegacyHotkeyName( "Print" )
        .FriendlyName( _( "Print..." ) )
        .Icon( BITMAPS::print_button ) );

TOOL_ACTION ACTIONS::plot( TOOL_ACTION_ARGS()
        .Name( "common.Control.plot" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Plot..." ) )
        .Icon( BITMAPS::plot ) );

TOOL_ACTION ACTIONS::quit( TOOL_ACTION_ARGS()
        .Name( "common.Control.quit" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Quit" ) )
        .Tooltip( _( "Close the current editor" ) )
        .Icon( BITMAPS::exit ) );


// Selection actions
TOOL_ACTION ACTIONS::selectionActivate( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection" )
        .Scope( AS_GLOBAL )
        // No description, not shown anywhere
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::selectionCursor( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.cursor" )
        .Scope( AS_GLOBAL )
        .Parameter<CLIENT_SELECTION_FILTER>( nullptr ) );

TOOL_ACTION ACTIONS::selectItem( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.selectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::selectItems( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.selectItems" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::unselectItem( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.unselectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::unselectItems( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.unselectItems" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::reselectItem( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.reselectItem" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::selectionClear( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.clear" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::selectionMenu( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.selectionMenu" )
        .Scope( AS_GLOBAL ) );


// Group actions
TOOL_ACTION ACTIONS::group( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.group" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Group Items" ) )
        .Tooltip( _( "Group the selected items so that they are treated as a single item" ) )
        .Icon( BITMAPS::group ) );

TOOL_ACTION ACTIONS::ungroup( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.ungroup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Ungroup Items" ) )
        .Tooltip( _( "Ungroup any selected groups" ) )
        .Icon( BITMAPS::group_ungroup ) );

TOOL_ACTION ACTIONS::addToGroup( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.addToGroup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Items" ) )
        .Tooltip( _( "Add items to group" ) )
        .Icon( BITMAPS::group_remove ) );

TOOL_ACTION ACTIONS::removeFromGroup( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.removeFromGroup" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove Items" ) )
        .Tooltip( _( "Remove items from group" ) )
        .Icon( BITMAPS::group_remove ) );

TOOL_ACTION ACTIONS::groupEnter( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.groupEnter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Enter Group" ) )
        .Tooltip( _( "Enter the group to edit items" ) )
        .Icon( BITMAPS::group_enter ) );

TOOL_ACTION ACTIONS::groupLeave( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.groupLeave" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Leave Group" ) )
        .Tooltip( _( "Leave the current group" ) )
        .Icon( BITMAPS::group_leave ) );

// GROUP_TOOL
TOOL_ACTION ACTIONS::groupProperties( TOOL_ACTION_ARGS()
        .Name( "common.Groups.groupProperties" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::pickNewGroupMember( TOOL_ACTION_ARGS()
        .Name( "common.Groups.selectNewGroupMember" )
        .Scope( AS_GLOBAL ) );

// Generic Edit Actions
TOOL_ACTION ACTIONS::cancelInteractive( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.cancel" )
        .Scope( AS_GLOBAL )
        // ESC key is handled in the dispatcher
        .FriendlyName( _( "Cancel" ) )
        .Tooltip( _( "Cancel current tool" ) )
        .Icon( BITMAPS::cancel )
        .Flags( AF_NONE ) );

TOOL_ACTION ACTIONS::finishInteractive( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.finish" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_END )
        .FriendlyName( _( "Finish" ) )
        .Tooltip( _( "Finish current tool" ) )
        .Icon( BITMAPS::checked_ok )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE ) );

TOOL_ACTION ACTIONS::showContextMenu( TOOL_ACTION_ARGS()
        .Name( "common.Control.showContextMenu" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Show Context Menu" ) )
        .Tooltip( _( "Perform the right-mouse-button action" ) )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT_CLICK )
        .ToolbarState( TOOLBAR_STATE::HIDDEN ) );

TOOL_ACTION ACTIONS::updateMenu( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.updateMenu" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::undo( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.undo" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'Z' )
        .LegacyHotkeyName( "Undo" )
        .FriendlyName( _( "Undo" ) )
        .Icon( BITMAPS::undo ) );

TOOL_ACTION ACTIONS::redo( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.redo" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'Z' )
#else
        .DefaultHotkey( MD_CTRL + 'Y' )
#endif
        .LegacyHotkeyName( "Redo" )
        .FriendlyName( _( "Redo" ) )
        .Icon( BITMAPS::redo ) );

// The following actions need to have a hard-coded UI ID using a wx-specific ID
// to fix things like search controls in standard file dialogs. If wxWidgets
// doesn't find these specific IDs somewhere in the menus then it won't enable
// cut/copy/paste.
TOOL_ACTION ACTIONS::cut( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.cut" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'X' )
        .LegacyHotkeyName( "Cut" )
        .FriendlyName( _( "Cut" ) )
        .Tooltip( _( "Cut selected item(s) to clipboard" ) )
        .Icon( BITMAPS::cut )
        .Flags( AF_NONE )
        .UIId( wxID_CUT ) );

TOOL_ACTION ACTIONS::copy( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.copy" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'C' )
        .LegacyHotkeyName( "Copy" )
        .FriendlyName( _( "Copy" ) )
        .Tooltip( _( "Copy selected item(s) to clipboard" ) )
        .Icon( BITMAPS::copy )
        .Flags( AF_NONE )
        .UIId( wxID_COPY ) );

TOOL_ACTION ACTIONS::copyAsText( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.copyAsText" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'C' )
        .FriendlyName( _( "Copy as Text" ) )
        .Tooltip( _( "Copy selected item(s) to clipboard as text" ) )
        .Icon( BITMAPS::copy )
        .Flags( AF_NONE ) );

TOOL_ACTION ACTIONS::paste( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.paste" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'V' )
        .LegacyHotkeyName( "Paste" )
        .FriendlyName( _( "Paste" ) )
        .Tooltip( _( "Paste item(s) from clipboard" ) )
        .Icon( BITMAPS::paste )
        .Flags( AF_NONE )
        .UIId( wxID_PASTE ) );

TOOL_ACTION ACTIONS::selectSetRect( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.selectSetRect" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Rectangle" ) )
        .Tooltip( _( "Set selection mode to use rectangle" ) )
        .Icon( BITMAPS::cursor ) );

TOOL_ACTION ACTIONS::selectSetLasso( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.selectSetLasso" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Lasso" ) )
        .Tooltip( _( "Set selection mode to use polygon lasso" ) )
        .Icon( BITMAPS::lasso ) );

TOOL_ACTION ACTIONS::selectAll( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.selectAll" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'A' )
        .FriendlyName( _( "Select All" ) )
        .Tooltip( _( "Select all items on screen" ) ) );

TOOL_ACTION ACTIONS::unselectAll( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.unselectAll" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'A' )
        .FriendlyName( _( "Unselect All" ) )
        .Tooltip( _( "Unselect all items on screen" ) ) );

TOOL_ACTION ACTIONS::pasteSpecial( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.pasteSpecial" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'V' )
        .FriendlyName( _( "Paste Special..." ) )
        .Tooltip( _( "Paste item(s) from clipboard with options" ) )
        .Icon( BITMAPS::paste_special ) );

TOOL_ACTION ACTIONS::duplicate( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.duplicate" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'D' )
        .LegacyHotkeyName( "Duplicate" )
        .FriendlyName( _( "Duplicate" ) )
        .Tooltip( _( "Duplicates the selected item(s)" ) )
        .Icon( BITMAPS::duplicate ) );

TOOL_ACTION ACTIONS::doDelete( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.delete" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( WXK_BACK )
#else
        .DefaultHotkey( WXK_DELETE )
#endif
        .LegacyHotkeyName( "Delete Item" )
        .FriendlyName( _( "Delete" ) )
        .Tooltip( _( "Delete selected item(s)" ) )  // differentiation from deleteTool, below
        .Icon( BITMAPS::trash )
        .Parameter( ACTIONS::REMOVE_FLAGS::NORMAL ) );

TOOL_ACTION ACTIONS::deleteTool( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.deleteTool" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Interactive Delete Tool" ) )
        .Tooltip( _( "Delete clicked items" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::delete_cursor )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::leftJustify( TOOL_ACTION_ARGS()
        .Name( "common.Control.leftJustify" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Left Justify" ) )
        .Tooltip( _( "Left-justify fields and text items" ) )
        .Icon( BITMAPS::text_align_left ) );

TOOL_ACTION ACTIONS::centerJustify( TOOL_ACTION_ARGS()
        .Name( "common.Control.centerJustify" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Center Justify" ) )
        .Tooltip( _( "Center-justify fields and text items" ) )
        .Icon( BITMAPS::text_align_center ) );

TOOL_ACTION ACTIONS::rightJustify( TOOL_ACTION_ARGS()
        .Name( "common.Control.rightJustify" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Right Justify" ) )
        .Tooltip( _( "Right-justify fields and text items" ) )
        .Icon( BITMAPS::text_align_right ) );

TOOL_ACTION ACTIONS::expandAll( TOOL_ACTION_ARGS()
        .Name( "common.Control.expandAll" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Expand All" ) )
        .Icon( BITMAPS::up ) );     // JEY TODO: need icon

TOOL_ACTION ACTIONS::collapseAll( TOOL_ACTION_ARGS()
        .Name( "common.Control.collapseAll" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Collapse All" ) )
        .Icon( BITMAPS::down ) );   // JEY TODO: need icon

// This is the generic increment action, and will need the parameter
// to be filled in by the event producer.
TOOL_ACTION ACTIONS::increment( TOOL_ACTION_ARGS()
        .Name( "eeschema.Interactive.increment" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Increment" ) )
        .Tooltip( _( "Increment the selected item(s)" ) ) );

TOOL_ACTION ACTIONS::incrementPrimary( TOOL_ACTION_ARGS()
        .Name( "eeschema.Interactive.incrementPrimary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Increment Primary" ) )
        .Tooltip( _( "Increment the primary field of the selected item(s)" ) )
        .Parameter( ACTIONS::INCREMENT{ 1, 0 } ) );

TOOL_ACTION ACTIONS::decrementPrimary( TOOL_ACTION_ARGS()
        .Name( "eeschema.Interactive.decrementPrimary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Decrement Primary" ) )
        .Tooltip( _( "Decrement the primary field of the selected item(s)" ) )
        .Parameter( ACTIONS::INCREMENT{ -1, 0 } ) );

TOOL_ACTION ACTIONS::incrementSecondary( TOOL_ACTION_ARGS()
        .Name( "eeschema.Interactive.incrementSecondary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Increment Secondary" ) )
        .Tooltip( _( "Increment the secondary field of the selected item(s)" ) )
        .Parameter( ACTIONS::INCREMENT{ 1, 1 } ) );

TOOL_ACTION ACTIONS::decrementSecondary( TOOL_ACTION_ARGS()
        .Name( "eeschema.Interactive.decrementSecondary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Decrement Secondary" ) )
        .Tooltip( _( "Decrement the secondary field of the selected item(s)" ) )
        .Parameter( ACTIONS::INCREMENT{ -1, 1 } ) );

TOOL_ACTION ACTIONS::selectColumns( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.SelectColumns" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select Column(s)" ) )
        .Tooltip( _( "Select complete column(s) containing the current selected cell(s)" ) )
        .Icon( BITMAPS::table_select_column ) );

TOOL_ACTION ACTIONS::selectRows( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.Rows" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select Row(s)" ) )
        .Tooltip( _( "Select complete row(s) containing the current selected cell(s)" ) )
        .Icon( BITMAPS::table_select_row ) );

TOOL_ACTION ACTIONS::selectTable( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.SelectTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select Table" ) )
        .Tooltip( _( "Select parent table of selected cell(s)" ) )
        .Icon( BITMAPS::table_select ) );

TOOL_ACTION ACTIONS::addRowAbove( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.addRowAbove" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Row Above" ) )
        .Tooltip( _( "Insert a new table row above the selected cell(s)" ) )
        .Icon( BITMAPS::table_add_row_above ) );

TOOL_ACTION ACTIONS::addRowBelow( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.addRowBelow" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Row Below" ) )
        .Tooltip( _( "Insert a new table row below the selected cell(s)" ) )
        .Icon( BITMAPS::table_add_row_below ) );

TOOL_ACTION ACTIONS::addColBefore( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.addColBefore" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Column Before" ) )
        .Tooltip( _( "Insert a new table column before the selected cell(s)" ) )
        .Icon( BITMAPS::table_add_column_before ) );

TOOL_ACTION ACTIONS::addColAfter( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.addColAfter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Add Column After" ) )
        .Tooltip( _( "Insert a new table column after the selected cell(s)" ) )
        .Icon( BITMAPS::table_add_column_after ) );

TOOL_ACTION ACTIONS::deleteRows( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.deleteRows" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Delete Row(s)" ) )
        .Tooltip( _( "Delete rows containing the currently selected cell(s)" ) )
        .Icon( BITMAPS::table_delete_row ) );

TOOL_ACTION ACTIONS::deleteColumns( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.deleteColumns" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Delete Column(s)" ) )
        .Tooltip( _( "Delete columns containing the currently selected cell(s)" ) )
        .Icon( BITMAPS::table_delete_column ) );

TOOL_ACTION ACTIONS::mergeCells( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.mergeCells" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Merge Cells" ) )
        .Tooltip( _( "Turn selected table cells into a single cell" ) )
        .Icon( BITMAPS::table ) );   // JEY TODO: need icon

TOOL_ACTION ACTIONS::unmergeCells( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.unmergeCell" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unmerge Cells" ) )
        .Tooltip( _( "Turn merged table cells back into separate cells." ) )
        .Icon( BITMAPS::table ) );   // JEY TODO: need icon

TOOL_ACTION ACTIONS::editTable( TOOL_ACTION_ARGS()
        .Name( "pcbnew.TableEditor.editTable" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'E' )
        .FriendlyName( _( "Edit Table..." ) )
        .Icon( BITMAPS::table_edit ) );

TOOL_ACTION ACTIONS::exportTableCSV( TOOL_ACTION_ARGS()
        .Name( "common.TableEditor.exportTableCSV" )
        .Scope( AS_GLOBAL )
        .MenuText( _( "Export Table to CSV..." ) )
        .Tooltip( _( "Export table contents to CSV file with resolved text variables" ) )
        .Icon( BITMAPS::export_file ) );

TOOL_ACTION ACTIONS::activatePointEditor( TOOL_ACTION_ARGS()
        .Name( "common.Control.activatePointEditor" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::pointEditorArcKeepCenter( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.arcKeepCenter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Keep Arc Center, Adjust Radius" ) )
        .Tooltip( _( "Switch arc editing mode to keep center, adjust radius and endpoints" ) )
        .Parameter( ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS ) );

TOOL_ACTION ACTIONS::pointEditorArcKeepEndpoint( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.arcKeepEndpoint" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Keep Arc Endpoints or Direction of Starting Point" ) )
        .Tooltip( _( "Switch arc editing mode to keep endpoints, or to keep direction of the other point" ) )
        .Parameter( ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION ) );

TOOL_ACTION ACTIONS::pointEditorArcKeepRadius( TOOL_ACTION_ARGS()
        .Name( "pcbnew.PointEditor.arcKeepRadius" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Keep Arc Radius and Center, adjust angle" ) )
        .Tooltip( _( "Switch arc editing mode to maintaining radius when endpoint are moved" ) )
        .Parameter( ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE ) );

TOOL_ACTION ACTIONS::cycleArcEditMode( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.cycleArcEditMode" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + ' ' )
        .FriendlyName( _( "Cycle Arc Editing Mode" ) )
        .Tooltip( _( "Switch to a different method of editing arcs" ) ) );

TOOL_ACTION ACTIONS::showSearch( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.search" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'G' )
        .LegacyHotkeyName( "Search" )
        .FriendlyName( _( "Search" ) )
        .Tooltip( _( "Show/hide the search panel" ) )
        .Icon( BITMAPS::find ) );

TOOL_ACTION ACTIONS::find( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.find" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'F' )
        .LegacyHotkeyName( "Find" )
        .FriendlyName( _( "Find" ) )
        .Icon( BITMAPS::find ) );

TOOL_ACTION ACTIONS::findAndReplace( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.findAndReplace" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_ALT + 'F' )
        .LegacyHotkeyName( "Find and Replace" )
        .FriendlyName( _( "Find and Replace" ) )
        .Icon( BITMAPS::find_replace ) );

TOOL_ACTION ACTIONS::findNext( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.findNext" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_F3 )
        .LegacyHotkeyName( "Find Next" )
        .FriendlyName( _( "Find Next" ) )
        .Icon( BITMAPS::find ) );

TOOL_ACTION ACTIONS::findPrevious( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.findPrevious" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_F3 ) )
        .LegacyHotkeyName( "Find Previous" )
        .FriendlyName( _( "Find Previous" ) )
        .Icon( BITMAPS::find ) );

TOOL_ACTION ACTIONS::findNextMarker( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.findNextMarker" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + static_cast<int>( WXK_F3 ) )
        .LegacyHotkeyName( "Find Next Marker" )
        .FriendlyName( _( "Find Next Marker" ) )
        .Icon( BITMAPS::find ) );

TOOL_ACTION ACTIONS::replaceAndFindNext( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.replaceAndFindNext" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Replace and Find Next" ) )
        .Icon( BITMAPS::find_replace ) );

TOOL_ACTION ACTIONS::replaceAll( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.replaceAll" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Replace All" ) )
        .Icon( BITMAPS::find_replace ) );

TOOL_ACTION ACTIONS::updateFind( TOOL_ACTION_ARGS()
        .Name( "common.Control.updateFind" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );


// Marker Controls
TOOL_ACTION ACTIONS::prevMarker( TOOL_ACTION_ARGS()
        .Name( "common.Checker.prevMarker" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Previous Marker" ) )
        .Icon( BITMAPS::marker_previous ) );

TOOL_ACTION ACTIONS::nextMarker( TOOL_ACTION_ARGS()
        .Name( "common.Checker.nextMarker" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Next Marker" ) )
        .Icon( BITMAPS::marker_next ) );

TOOL_ACTION ACTIONS::excludeMarker( TOOL_ACTION_ARGS()
        .Name( "common.Checker.excludeMarker" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Exclude Marker" ) )
        .Tooltip( _( "Mark current violation in Checker window as an exclusion" ) )
        .Icon( BITMAPS::marker_exclude ) );

// View Controls
TOOL_ACTION ACTIONS::zoomRedraw( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomRedraw" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( MD_CTRL + 'R' )
#else
        .DefaultHotkey( WXK_F5 )
#endif
        .LegacyHotkeyName( "Zoom Redraw" )
        .FriendlyName( _( "Refresh" ) )
        .Icon( BITMAPS::refresh ) );

TOOL_ACTION ACTIONS::zoomFitScreen( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomFitScreen" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( MD_CTRL + '0' )
#else
        .DefaultHotkey( WXK_HOME )
#endif
        .LegacyHotkeyName( "Zoom Auto" )
        .FriendlyName( _( "Zoom to Fit" ) )
        .Tooltip( _( "Zoom to worksheet area if exists or edited object" ) )
        .Icon( BITMAPS::zoom_fit_in_page ) );

TOOL_ACTION ACTIONS::zoomFitObjects( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomFitObjects" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_HOME ) )
        .FriendlyName( _( "Zoom to All Objects" ) )
        .Tooltip( _( "Zoom to all objects on screen" ) )
        .Icon( BITMAPS::zoom_fit_to_objects ) );

TOOL_ACTION ACTIONS::zoomFitSelection( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomFitSelection" )
        .Scope( AS_GLOBAL )
        .Tooltip( _( "Zoom to items currently selected" ) )
        .FriendlyName( _( "Zoom to Selected Objects" ) )
        .Icon( BITMAPS::zoom_fit_to_objects ) );

TOOL_ACTION ACTIONS::zoomIn( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomIn" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( MD_CTRL + '+' )
#else
        .DefaultHotkey( WXK_F1 )
#endif
        .LegacyHotkeyName( "Zoom In" )
        .FriendlyName( _( "Zoom In at Cursor" ) )
        .Icon( BITMAPS::zoom_in ) );

TOOL_ACTION ACTIONS::zoomOut( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomOut" )
        .Scope( AS_GLOBAL )
#if defined( __WXMAC__ )
        .DefaultHotkey( MD_CTRL + '-' )
#else
        .DefaultHotkey( WXK_F2 )
#endif
        .LegacyHotkeyName( "Zoom Out" )
        .FriendlyName( _( "Zoom Out at Cursor" ) )
        .Icon( BITMAPS::zoom_out ) );

TOOL_ACTION ACTIONS::zoomInCenter( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomInCenter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom In" ) )
        .Icon( BITMAPS::zoom_in ) );

TOOL_ACTION ACTIONS::zoomOutCenter( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomOutCenter" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom Out" ) )
        .Icon( BITMAPS::zoom_out ) );

TOOL_ACTION ACTIONS::zoomInHorizontally( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomInHorizontally" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom In Horizontally" ) )
        .Tooltip( _( "Zoom in horizontally the plot area" ) )
        .Icon( BITMAPS::zoom_in_horizontally ) );

TOOL_ACTION ACTIONS::zoomOutHorizontally( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomOutHorizontally" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom Out Horizontally" ) )
        .Tooltip( _( "Zoom out horizontally the plot area" ) )
        .Icon( BITMAPS::zoom_out_horizontally ) );

TOOL_ACTION ACTIONS::zoomInVertically( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomInVertically" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom In Vertically" ) )
        .Tooltip( _( "Zoom in vertically the plot area" ) )
        .Icon( BITMAPS::zoom_in_vertically ) );

TOOL_ACTION ACTIONS::zoomOutVertically( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomOutVertically" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Zoom Out Vertically" ) )
        .Tooltip( _( "Zoom out vertically the plot area" ) )
        .Icon( BITMAPS::zoom_out_vertically ) );

TOOL_ACTION ACTIONS::zoomCenter( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomCenter" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_F4 )
        .LegacyHotkeyName( "Zoom Center" )
        .FriendlyName( _( "Center on Cursor" ) )
        .Icon( BITMAPS::zoom_center_on_screen ) );

TOOL_ACTION ACTIONS::zoomTool( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomTool" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_F5 ) )
        .LegacyHotkeyName( "Zoom to Selection" )
        .FriendlyName( _( "Zoom to Selection Area" ) )
        .Tooltip( _( "Zoom to an area selection created by a mouse drag" ) )
        .Icon( BITMAPS::zoom_area )
        .ToolbarState( { TOOLBAR_STATE::TOGGLE, TOOLBAR_STATE::CANCEL } )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::zoomUndo( TOOL_ACTION_ARGS()
        .Name( "common.Control.undoZoom" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Undo Last Zoom" ) )
        .Tooltip( _( "Return zoom to level prior to last zoom action" ) )
        .Icon( BITMAPS::undo ) );

TOOL_ACTION ACTIONS::zoomRedo( TOOL_ACTION_ARGS()
        .Name( "common.Control.redoZoom" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Redo Last Zoom" ) )
        .Tooltip( _( "Return zoom to level prior to last zoom undo" ) )
        .Icon( BITMAPS::redo ) );

TOOL_ACTION ACTIONS::zoomPreset( TOOL_ACTION_ARGS()
        .Name( "common.Control.zoomPreset" )
        .Scope( AS_GLOBAL )
        .Parameter<int>( 0 ) );      // Default parameter is the 0th item in the list

TOOL_ACTION ACTIONS::centerContents( TOOL_ACTION_ARGS()
        .Name( "common.Control.centerContents" )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::centerSelection( TOOL_ACTION_ARGS()
        .Name( "common.Control.centerSelection" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Pan to Center Selected Objects" ) ) );

// Cursor control
TOOL_ACTION ACTIONS::cursorUp( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorUp" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_UP )
        .FriendlyName( _( "Cursor Up" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_UP ) );

TOOL_ACTION ACTIONS::cursorDown( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorDown" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_DOWN )
        .FriendlyName( _( "Cursor Down" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DOWN ) );

TOOL_ACTION ACTIONS::cursorLeft( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorLeft" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_LEFT )
        .FriendlyName( _( "Cursor Left" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_LEFT ) );

TOOL_ACTION ACTIONS::cursorRight( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorRight" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_RIGHT )
        .FriendlyName( _( "Cursor Right" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT ) );


TOOL_ACTION ACTIONS::cursorUpFast( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorUpFast" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_UP ) )
        .FriendlyName( _( "Cursor Up Fast" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_UP_FAST ) );

TOOL_ACTION ACTIONS::cursorDownFast( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorDownFast" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_DOWN ) )
        .FriendlyName( _( "Cursor Down Fast" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DOWN_FAST ) );

TOOL_ACTION ACTIONS::cursorLeftFast( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorLeftFast" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_LEFT ) )
        .FriendlyName( _( "Cursor Left Fast" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_LEFT_FAST ) );

TOOL_ACTION ACTIONS::cursorRightFast( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorRightFast" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_RIGHT ) )
        .FriendlyName( _( "Cursor Right Fast" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT_FAST ) );

TOOL_ACTION ACTIONS::cursorClick( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorClick" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_RETURN )
        .LegacyHotkeyName( "Mouse Left Click" )
        .FriendlyName( _( "Click" ) )
        .Tooltip( _( "Performs left mouse button click" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_CLICK ) );

TOOL_ACTION ACTIONS::cursorDblClick( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorDblClick" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_END )
        .LegacyHotkeyName( "Mouse Left Double Click" )
        .FriendlyName( _( "Double-click" ) )
        .Tooltip( _( "Performs left mouse button double-click" ) )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DBL_CLICK ) );

TOOL_ACTION ACTIONS::refreshPreview( TOOL_ACTION_ARGS()
        .Name( "common.Control.refreshPreview" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::pinLibrary( TOOL_ACTION_ARGS()
        .Name( "common.Control.pinLibrary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Pin Library" ) )
        .Tooltip( _( "Keep the library at the top of the list" ) ) );

TOOL_ACTION ACTIONS::unpinLibrary( TOOL_ACTION_ARGS()
        .Name( "common.Control.unpinLibrary" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Unpin Library" ) )
        .Tooltip( _( "No longer keep the library at the top of the list" ) ) );

TOOL_ACTION ACTIONS::showLibraryTree( TOOL_ACTION_ARGS()
        .Name( "common.Control.showLibraryTree" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Library Tree" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::search_tree ) );

TOOL_ACTION ACTIONS::hideLibraryTree( TOOL_ACTION_ARGS()
        .Name( "common.Control.hideLibraryTree" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Hide Library Tree" ) )
        .Icon( BITMAPS::search_tree ) );

TOOL_ACTION ACTIONS::libraryTreeSearch( TOOL_ACTION_ARGS()
        .Name( "common.Control.libraryTreeSearch" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Focus Library Tree Search Field" ) )
        .DefaultHotkey( MD_CTRL + 'L' )
        .ToolbarState( TOOLBAR_STATE::HIDDEN ) );

TOOL_ACTION ACTIONS::panUp( TOOL_ACTION_ARGS()
        .Name( "common.Control.panUp" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_UP ) )
        .FriendlyName( _( "Pan Up" ) )
        .Flags( AF_NONE )
        .Parameter( CURSOR_UP ) );

TOOL_ACTION ACTIONS::panDown( TOOL_ACTION_ARGS()
        .Name( "common.Control.panDown" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_DOWN ) )
        .FriendlyName( _( "Pan Down" ) )
        .Flags( AF_NONE )
        .Parameter( CURSOR_DOWN ) );

TOOL_ACTION ACTIONS::panLeft( TOOL_ACTION_ARGS()
        .Name( "common.Control.panLeft" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_LEFT ) )
        .FriendlyName( _( "Pan Left" ) )
        .Flags( AF_NONE )
        .Parameter( CURSOR_LEFT ) );

TOOL_ACTION ACTIONS::panRight( TOOL_ACTION_ARGS()
        .Name( "common.Control.panRight" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + static_cast<int>( WXK_RIGHT ) )
        .FriendlyName( _( "Pan Right" ) )
        .Flags( AF_NONE )
        .Parameter( CURSOR_RIGHT ) );

// Grid control
TOOL_ACTION ACTIONS::gridFast1( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridFast1" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '1' )
        .LegacyHotkeyName( "Switch Grid To Fast Grid1" )
        .FriendlyName( _( "Switch to Fast Grid 1" ) ) );

TOOL_ACTION ACTIONS::gridFast2( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridFast2" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '2' )
        .LegacyHotkeyName( "Switch Grid To Fast Grid2" )
        .FriendlyName( _( "Switch to Fast Grid 2" ) ) );

TOOL_ACTION ACTIONS::gridFastCycle( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridFastCycle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '4' )
        .LegacyHotkeyName( "Switch Grid To Next Fast Grid" )
        .FriendlyName( _( "Cycle Fast Grid"  ) ) );

TOOL_ACTION ACTIONS::gridNext( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridNext" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'N' )
        .LegacyHotkeyName( "Switch Grid To Next" )
        .FriendlyName( _("Switch to Next Grid" ) ) );

TOOL_ACTION ACTIONS::gridPrev( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridPrev" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_SHIFT + 'N' )
        .LegacyHotkeyName( "Switch Grid To Previous" )
        .FriendlyName( _( "Switch to Previous Grid" ) ) );

TOOL_ACTION ACTIONS::gridSetOrigin( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridSetOrigin" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Set Grid Origin" )
        .FriendlyName( _( "Grid Origin" ) )
        .Tooltip( _( "Place the grid origin point" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::grid_select_axis )
        .Parameter<VECTOR2D*>( nullptr ) );

TOOL_ACTION ACTIONS::gridResetOrigin( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridResetOrigin" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Reset Grid Origin" )
        .FriendlyName( _( "Reset Grid Origin" ) ) );

TOOL_ACTION ACTIONS::gridPreset( TOOL_ACTION_ARGS()
        .Name( "common.Control.gridPreset" )
        .Scope( AS_GLOBAL )
        .Parameter<int>( 0 ) );          // Default to the 1st element of the list

TOOL_ACTION ACTIONS::toggleGrid( TOOL_ACTION_ARGS()
        .Name( "common.Control.toggleGrid" )
        .Scope( AS_GLOBAL)
        .FriendlyName( _( "Show Grid" ) )
        .Tooltip( _( "Display background grid in the edit window" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::grid ) );

TOOL_ACTION ACTIONS::toggleGridOverrides( TOOL_ACTION_ARGS()
        .Name( "common.Control.toggleGridOverrides" )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'G' )
        .Scope( AS_GLOBAL)
        .FriendlyName( _( "Grid Overrides" ) )
        .Tooltip( _( "Enables item-specific grids that override the current grid" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::grid_override ) );

TOOL_ACTION ACTIONS::gridProperties( TOOL_ACTION_ARGS()
        .Name( "common.Control.editGrids" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Edit Grids..." ) )
        .Tooltip( _( "Edit grid definitions" ) )
        .Icon( BITMAPS::grid_select ) );

TOOL_ACTION ACTIONS::gridOrigin(  TOOL_ACTION_ARGS()
        .Name( "common.Control.editGridOrigin" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Grid Origin..." ) )
        .Tooltip( _( "Set the grid origin point" ) )
        .Icon( BITMAPS::grid_select_axis ) );

TOOL_ACTION ACTIONS::inchesUnits( TOOL_ACTION_ARGS()
        .Name( "common.Control.imperialUnits" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Inches" ) )
        .Icon( BITMAPS::unit_inch )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Flags( AF_NONE )
        .Parameter( EDA_UNITS::INCH ) );

TOOL_ACTION ACTIONS::milsUnits( TOOL_ACTION_ARGS()
        .Name( "common.Control.mils" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Mils" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::unit_mil )
        .Flags( AF_NONE )
        .Parameter( EDA_UNITS::MILS ) );

TOOL_ACTION ACTIONS::millimetersUnits( TOOL_ACTION_ARGS()
        .Name( "common.Control.metricUnits" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Millimeters" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::unit_mm )
        .Flags( AF_NONE )
        .Parameter( EDA_UNITS::MM ) );

TOOL_ACTION ACTIONS::updateUnits( TOOL_ACTION_ARGS()
        .Name( "common.Control.updateUnits" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::updatePreferences( TOOL_ACTION_ARGS()
        .Name( "common.Control.updatePreferences" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::selectLibTreeColumns( TOOL_ACTION_ARGS()
        .Name( "common.Control.selectColumns" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select Columns..." ) ) );

TOOL_ACTION ACTIONS::toggleUnits( TOOL_ACTION_ARGS()
        .Name( "common.Control.toggleUnits" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + 'U' )
        .LegacyHotkeyName( "Switch Units" )
        .FriendlyName( _( "Switch units" ) )
        .Tooltip( _( "Switch between imperial and metric units" ) )
        .Icon( BITMAPS::unit_mm ) );

TOOL_ACTION ACTIONS::togglePolarCoords( TOOL_ACTION_ARGS()
        .Name( "common.Control.togglePolarCoords" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Polar Coordinates" ) )
        .Tooltip( _( "Switch between polar and cartesian coordinate systems" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::polar_coord ) );

TOOL_ACTION ACTIONS::resetLocalCoords( TOOL_ACTION_ARGS()
        .Name( "common.Control.resetLocalCoords" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( ' ' )
        .LegacyHotkeyName( "Reset Local Coordinates" )
        .FriendlyName( _( "Reset Local Coordinates" ) ) );

TOOL_ACTION ACTIONS::toggleCursor( TOOL_ACTION_ARGS()
        .Name( "common.Control.toggleCursor" )
        .Scope( AS_GLOBAL )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Toggle Cursor Display (Modern Toolset only)" )
        .FriendlyName( _( "Always Show Crosshairs" ) )
        .Tooltip( _( "Display crosshairs even when not drawing objects" ) )
        .Icon( BITMAPS::cursor ) );

TOOL_ACTION ACTIONS::cursorSmallCrosshairs( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorSmallCrosshairs" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Small crosshairs" ) )
        .Tooltip( _( "Use small crosshairs aligned at 0 and 90 degrees" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::cursor_shape ) );

TOOL_ACTION ACTIONS::cursorFullCrosshairs( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursorFullCrosshairs" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Full-Window Crosshairs" ) )
        .Tooltip( _( "Display full-window crosshairs aligned at 0 and 90 degrees" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::cursor_fullscreen ) );

TOOL_ACTION ACTIONS::cursor45Crosshairs( TOOL_ACTION_ARGS()
        .Name( "common.Control.cursor45Crosshairs" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "45 Degree Crosshairs" ) )
        .Tooltip( _( "Display full-window crosshairs aligned at 45 and 135 degrees" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::cursor_fullscreen45 ) );

TOOL_ACTION ACTIONS::highContrastMode( TOOL_ACTION_ARGS()
        .Name( "common.Control.highContrastMode" )
        .Scope( AS_GLOBAL )
        .LegacyHotkeyName( "Toggle High Contrast Mode" )
        .FriendlyName( _( "Inactive Layer View Mode" ) )
        .Tooltip( _( "Toggle inactive layers between normal and dimmed" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::contrast_mode ) );

TOOL_ACTION ACTIONS::highContrastModeCycle( TOOL_ACTION_ARGS()
        .Name( "common.Control.highContrastModeCycle" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'H' )
        .FriendlyName( _( "Inactive Layer View Mode (3-state)" ) )
        .Tooltip( _( "Cycle inactive layers between normal, dimmed, and hidden" ) )
        .Icon( BITMAPS::contrast_mode ) );

TOOL_ACTION ACTIONS::toggleBoundingBoxes( TOOL_ACTION_ARGS()
        .Name( "common.Control.toggleBoundingBoxes" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Draw Bounding Boxes" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::gerbview_show_negative_objects ) );

TOOL_ACTION ACTIONS::selectionTool( TOOL_ACTION_ARGS()
        .Name( "common.InteractiveSelection.selectionTool" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Select item(s)" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::cursor )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::measureTool( TOOL_ACTION_ARGS()
        .Name( "common.Interactive.measureTool" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + MD_SHIFT + 'M' )
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        .LegacyHotkeyName( "Measure Distance (Modern Toolset only)" )
        .FriendlyName( _( "Measure Tool" ) )
        .Tooltip( _( "Interactively measure distance between points" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::measurement )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::pickerTool( TOOL_ACTION_ARGS()
        .Name( "common.InteractivePicker.pickerTool" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL )
        .Flags( AF_ACTIVATE ) );

TOOL_ACTION ACTIONS::pickerSubTool( TOOL_ACTION_ARGS()
        .Name( "common.InteractivePicker.pickerSubTool" )
        .ToolbarState( TOOLBAR_STATE::HIDDEN )
        .Scope( AS_GLOBAL ) );

TOOL_ACTION ACTIONS::showProjectManager( TOOL_ACTION_ARGS()
        .Name( "common.Control.showProjectManager" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Switch to Project Manager" ) )
        .Tooltip( _( "Show project window" ) )
        .Icon( BITMAPS::icon_kicad_24 ) );

TOOL_ACTION ACTIONS::show3DViewer( TOOL_ACTION_ARGS()
        .Name( "common.Control.show3DViewer" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_ALT + '3' )
        .LegacyHotkeyName( "3D Viewer" )
        .FriendlyName( _( "3D Viewer" ) )
        .Tooltip( _( "Show 3D viewer window" ) )
        .Icon( BITMAPS::three_d ) );

TOOL_ACTION ACTIONS::showSymbolBrowser( TOOL_ACTION_ARGS()
        .Name( "common.Control.showSymbolBrowser" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Symbol Library Browser" ) )
        .Icon( BITMAPS::library_browser )
        .Flags( AF_NONE)
        .Parameter( FRAME_SCH_VIEWER ) );

TOOL_ACTION ACTIONS::showSymbolEditor( TOOL_ACTION_ARGS()
        .Name( "common.Control.showSymbolEditor" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Symbol Editor" ) )
        .Tooltip( _( "Create, delete and edit schematic symbols" ) )
        .Icon( BITMAPS::libedit )
        .Flags( AF_NONE )
        .Parameter( FRAME_SCH_SYMBOL_EDITOR ) );

TOOL_ACTION ACTIONS::showFootprintBrowser( TOOL_ACTION_ARGS()
        .Name( "common.Control.showFootprintBrowser" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Footprint Library Browser" ) )
        .Icon( BITMAPS::library_browser )
        .Flags( AF_NONE )
        .Parameter( FRAME_FOOTPRINT_VIEWER ) );

TOOL_ACTION ACTIONS::showFootprintEditor( TOOL_ACTION_ARGS()
        .Name( "common.Control.showFootprintEditor" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Footprint Editor" ) )
        .Tooltip( _( "Create, delete and edit board footprints" ) )
        .Icon( BITMAPS::module_editor )
        .Flags( AF_NONE )
        .Parameter( FRAME_FOOTPRINT_EDITOR ) );

TOOL_ACTION ACTIONS::showCalculatorTools( TOOL_ACTION_ARGS()
        .Name( "common.Control.showCalculatorTools" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Calculator Tools" ) )
        .Tooltip( _( "Run component calculations, track width calculations, etc." ) )
        .Icon( BITMAPS::icon_pcbcalculator_24 )
        .Flags( AF_NONE )
        .Parameter( FRAME_CALC ) );

TOOL_ACTION ACTIONS::showProperties( TOOL_ACTION_ARGS()
        .Name( "common.Control.showProperties" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Properties" ) )
        .Tooltip( _( "Show/hide the properties manager" ) )
        .ToolbarState( TOOLBAR_STATE::TOGGLE )
        .Icon( BITMAPS::tools ) );

TOOL_ACTION ACTIONS::showDatasheet( TOOL_ACTION_ARGS()
        .Name( "common.Control.showDatasheet" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( 'D' )
        .LegacyHotkeyName( "Show Datasheet" )
        .FriendlyName( _( "Show Datasheet" ) )
        .Tooltip( _( "Open the datasheet in a browser" ) )
        .Icon( BITMAPS::datasheet ) );

TOOL_ACTION ACTIONS::updatePcbFromSchematic( TOOL_ACTION_ARGS()
        .Name( "common.Control.updatePcbFromSchematic" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( WXK_F8 )
        .LegacyHotkeyName( "Update PCB from Schematic" )
        .FriendlyName( _( "Update PCB from Schematic..." ) )
        .Tooltip( _( "Update PCB with changes made to schematic" ) )
        .Icon( BITMAPS::update_pcb_from_sch ) );

TOOL_ACTION ACTIONS::updateSchematicFromPcb( TOOL_ACTION_ARGS()
        .Name( "common.Control.updateSchematicFromPCB" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Update Schematic from PCB..." ) )
        .Tooltip( _( "Update schematic with changes made to PCB" ) )
        .Icon( BITMAPS::update_sch_from_pcb ) );

TOOL_ACTION ACTIONS::openPreferences( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.openPreferences" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + ',' )
        .FriendlyName( _( "Preferences..." ) )
        .Tooltip( _( "Show preferences for all open tools" ) )
        .Icon( BITMAPS::preference )
        .UIId( wxID_PREFERENCES ) );

TOOL_ACTION ACTIONS::configurePaths( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.configurePaths" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Configure Paths..." ) )
        .Tooltip( _( "Edit path configuration environment variables" ) )
        .Icon( BITMAPS::path ) );

TOOL_ACTION ACTIONS::showSymbolLibTable( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.showSymbolLibTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Manage Symbol Libraries..." ) )
        .Tooltip( _( "Edit the global and project symbol library lists" ) )
        .Icon( BITMAPS::library_table ) );

TOOL_ACTION ACTIONS::showFootprintLibTable( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.showFootprintLibTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Manage Footprint Libraries..." ) )
        .Tooltip( _( "Edit the global and project footprint library lists" ) )
        .Icon( BITMAPS::library_table ) );

TOOL_ACTION ACTIONS::showDesignBlockLibTable( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.showDesignBLockLibTable" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Manage Design Block Libraries..." ) )
        .Tooltip( _( "Edit the global and project design block library lists" ) )
        .Icon( BITMAPS::library_table ) );

TOOL_ACTION ACTIONS::gettingStarted( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.gettingStarted" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Getting Started with KiCad" ) )
        .Tooltip( _( "Open \"Getting Started in KiCad\" guide for beginners" ) )
        .Icon( BITMAPS::help ) );

TOOL_ACTION ACTIONS::help( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.help" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Help" ) )
        .Tooltip( _( "Open product documentation in a web browser" ) )
        .Icon( BITMAPS::help_online ) );

TOOL_ACTION ACTIONS::about( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.about" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "About KiCad" ) )
        .UIId( wxID_ABOUT )
        .Icon( BITMAPS::about ) );

TOOL_ACTION ACTIONS::listHotKeys( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.listHotKeys" )
        .Scope( AS_GLOBAL )
        .DefaultHotkey( MD_CTRL + static_cast<int>( WXK_F1 ) )
        .LegacyHotkeyName( "List Hotkeys" )
        .FriendlyName( _( "List Hotkeys..." ) )
        .Tooltip( _( "Displays current hotkeys table and corresponding commands" ) )
        .Icon( BITMAPS::hotkeys ) );

TOOL_ACTION ACTIONS::getInvolved( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.getInvolved" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Get Involved" ) )
        .Tooltip( _( "Open \"Contribute to KiCad\" in a web browser" ) )
        .Icon( BITMAPS::info ) );

TOOL_ACTION ACTIONS::donate( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.donate" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Donate" ) )
        .Tooltip( _( "Open \"Donate to KiCad\" in a web browser" ) ) );

TOOL_ACTION ACTIONS::reportBug( TOOL_ACTION_ARGS()
        .Name( "common.SuiteControl.reportBug" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Report Bug" ) )
        .Tooltip( _( "Report a problem with KiCad" ) )
        .Icon( BITMAPS::bug ) );

TOOL_ACTION ACTIONS::ddAddLibrary( TOOL_ACTION_ARGS()
        .Name( "common.Control.ddaddLibrary" )
        .Scope( AS_GLOBAL ) );

// API

TOOL_ACTION ACTIONS::pluginsReload( TOOL_ACTION_ARGS()
        .Name( "common.API.pluginsReload" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Refresh Plugins" ) )
        .Tooltip( _( "Reload all python plugins and refresh plugin menus" ) )
        .Icon( BITMAPS::reload ) );

// Embedding Files

TOOL_ACTION ACTIONS::embeddedFiles( TOOL_ACTION_ARGS()
        .Name( "common.Embed.embededFile" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Embedded Files" ) )
        .Tooltip( _( "Manage embedded files" ) ) );

TOOL_ACTION ACTIONS::removeFile( TOOL_ACTION_ARGS()
        .Name( "common.Embed.removeFile" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Remove File" ) )
        .Tooltip( _( "Remove an embedded file" ) ) );

TOOL_ACTION ACTIONS::extractFile( TOOL_ACTION_ARGS()
        .Name( "common.Embed.extractFile" )
        .Scope( AS_GLOBAL )
        .FriendlyName( _( "Extract File" ) )
        .Tooltip( _( "Extract an embedded file" ) ) );

// System-wide selection Events

const TOOL_EVENT EVENTS::PointSelectedEvent( TC_MESSAGE, TA_ACTION,
                                             "common.Interactive.pointSelected" );
const TOOL_EVENT EVENTS::SelectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.selected" );
const TOOL_EVENT EVENTS::UnselectedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.unselected" );
const TOOL_EVENT EVENTS::ClearedEvent( TC_MESSAGE, TA_ACTION, "common.Interactive.cleared" );

const TOOL_EVENT EVENTS::ConnectivityChangedEvent( TC_MESSAGE, TA_ACTION,
                                                   "common.Interactive.connectivityChanged" );

const TOOL_EVENT EVENTS::SelectedItemsModified( TC_MESSAGE, TA_ACTION,
                                                "common.Interactive.modified" );
const TOOL_EVENT EVENTS::SelectedItemsMoved( TC_MESSAGE, TA_ACTION, "common.Interactive.moved" );
const TOOL_EVENT EVENTS::InhibitSelectionEditing( TC_MESSAGE, TA_ACTION,
                                                  "common.Interactive.inhibit" );
const TOOL_EVENT EVENTS::UninhibitSelectionEditing( TC_MESSAGE, TA_ACTION,
                                                    "common.Interactive.uninhibit" );

const TOOL_EVENT EVENTS::DisambiguatePoint( TC_MESSAGE, TA_ACTION,
                                            "common.Interactive.disambiguate" );

const TOOL_EVENT EVENTS::GridChangedByKeyEvent( TC_MESSAGE, TA_ACTION,
                                                "common.Interactive.gridChangedByKey" );

const TOOL_EVENT
        EVENTS::ContrastModeChangedByKeyEvent( TC_MESSAGE, TA_ACTION,
                                               "common.Interactive.contrastModeChangedByKeyEvent" );

// System-wide undo/redo Events

const TOOL_EVENT EVENTS::UndoRedoPreEvent( TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL );
const TOOL_EVENT EVENTS::UndoRedoPostEvent( TC_MESSAGE, TA_UNDO_REDO_POST, AS_GLOBAL );

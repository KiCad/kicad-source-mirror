/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bitmaps.h>

#include <cvpcb_mainframe.h>
#include <listboxes.h>
#include <tools/cvpcb_actions.h>

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s


// Selection tool action for the footprint viewer window
TOOL_ACTION CVPCB_ACTIONS::selectionActivate( "cvpcb.FootprintViewerInteractiveSelection",
        AS_GLOBAL, 0, "",
        "",
        "",
        NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere

TOOL_ACTION CVPCB_ACTIONS::controlActivate( "cvpcb.Control",
        AS_GLOBAL, 0, "",
        "",
        "",
        NULL, AF_ACTIVATE ); // No description, it is not supposed to be shown anywhere


// Action to show the footprint viewer window
TOOL_ACTION CVPCB_ACTIONS::showFootprintViewer( "cvpcb.Control.ShowFootprintViewer",
        AS_GLOBAL, 0, "",
        _( "View selected footprint" ),
        _( "View the selected footprint in the footprint viewer" ),
        show_footprint_xpm );


// Actions to handle management tasks
TOOL_ACTION CVPCB_ACTIONS::showEquFileTable( "cvpcb.Control.ShowEquFileTable",
        AS_GLOBAL, 0, "",
        _( "Manage Footprint Association Files" ),
        _( "Configure footprint association file (.equ) list.  These files are "
           "used to automatically assign footprint names from symbol values." ) );

TOOL_ACTION CVPCB_ACTIONS::saveAssociations( "cvpcb.Control.SaveAssocations",
        AS_GLOBAL,
        MD_CTRL + 'S', LEGACY_HK_NAME( "Save" ),
        _( "Save to Schematic" ),
        _( "Save footprint associations in schematic symbol footprint fields" ),
        save_xpm );

// Actions to navigate the display
TOOL_ACTION CVPCB_ACTIONS::changeFocusRight( "cvpcb.Control.changeFocusRight",
        AS_GLOBAL,
        WXK_TAB, "",
        "",
        "",
        nullptr, AF_NONE,
        (void*) CVPCB_MAINFRAME::CHANGE_FOCUS_RIGHT );

TOOL_ACTION CVPCB_ACTIONS::changeFocusLeft( "cvpcb.Control.changeFocusLeft",
        AS_GLOBAL,
        MD_SHIFT + WXK_TAB, "",
        "",
        "",
        nullptr, AF_NONE,
        (void*) CVPCB_MAINFRAME::CHANGE_FOCUS_LEFT );

// Actions to navigate the component list
TOOL_ACTION CVPCB_ACTIONS::gotoNextNA( "cvpcb.Control.GotoNextNA",
        AS_GLOBAL, 0, "",
        _( "Select next unassigned symbol" ),
        _( "Select next symbol with no footprint assignment" ),
        right_xpm, AF_NONE,
        (void*) CVPCB_MAINFRAME::ITEM_NEXT );

TOOL_ACTION CVPCB_ACTIONS::gotoPreviousNA( "cvpcb.Control.GotoPreviousNA",
        AS_GLOBAL, 0, "",
        _( "Select previous unassigned symbol" ),
        _( "Select previous symbol with no footprint assignment" ),
        left_xpm, AF_NONE,
        (void*) CVPCB_MAINFRAME::ITEM_PREV );


// Actions to modify component associations
TOOL_ACTION CVPCB_ACTIONS::associate( "cvpcb.Association.Associate",
        AS_GLOBAL,
        WXK_RETURN, "",
        _( "Assign footprint" ),
        _( "Assign footprint to selected symbols" ),
        auto_associate_xpm );

TOOL_ACTION CVPCB_ACTIONS::autoAssociate( "cvpcb.Association.AutoAssociate",
        AS_GLOBAL, 0, "",
        _( "Automatically assign footprints" ),
        _( "Perform automatic footprint assignment" ),
        auto_associate_xpm );

TOOL_ACTION CVPCB_ACTIONS::deleteAssoc( "cvpcb.Association.Delete",
        AS_GLOBAL,
        WXK_DELETE, "",
        _( "Delete association" ),
        _( "Delete selected footprint associations" ),
        delete_association_xpm );

TOOL_ACTION CVPCB_ACTIONS::deleteAll( "cvpcb.Association.DeleteAll",
        AS_GLOBAL, 0, "",
        _( "Delete all footprint associations" ),
        _( "Delete all footprint associations" ),
        delete_association_xpm );


// Actions to filter the footprint list
TOOL_ACTION CVPCB_ACTIONS::FilterFPbyFPFilters( "cvpcb.Control.FilterFPbyFPFilters",
        AS_GLOBAL, 0, "",
        _( "Use symbol footprint filters" ),
        _( "Filter footprint list by footprint filters defined in the symbol" ),
        module_filtered_list_xpm, AF_NONE,
        (void*) FOOTPRINTS_LISTBOX::FILTERING_BY_COMPONENT_FP_FILTERS );

TOOL_ACTION CVPCB_ACTIONS::filterFPbyPin( "cvpcb.Control.FilterFPByPin",
        AS_GLOBAL, 0, "",
        _( "Filter by pin count" ),
        _( "Filter footprint list by pin count" ),
        module_pin_filtered_list_xpm, AF_NONE,
        (void*) FOOTPRINTS_LISTBOX::FILTERING_BY_PIN_COUNT );

TOOL_ACTION CVPCB_ACTIONS::FilterFPbyLibrary( "cvpcb.Control.FilterFPbyLibrary",
        AS_GLOBAL, 0, "",
        _( "Filter by library" ),
        _( "Filter footprint list by library" ),
        module_library_list_xpm, AF_NONE,
        (void*) FOOTPRINTS_LISTBOX::FILTERING_BY_LIBRARY );

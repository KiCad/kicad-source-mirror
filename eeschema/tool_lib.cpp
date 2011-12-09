/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2011 KiCad Developers, see change_log.txt for contributors.
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

/**
 * @file tool_lib.cpp
 */

#include "fctsys.h"
#include "hotkeys.h"
#include "eeschema_id.h"

#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "dialog_helpers.h"

#include "help_common_strings.h"

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


extern int ExportPartId;
extern int ImportPartId;
extern int CreateNewLibAndSavePartId;


void LIB_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_VToolBar != NULL )
        return;

    m_VToolBar = new EDA_TOOLBAR( TOOLBAR_TOOL, this, ID_V_TOOLBAR, false );

    // Set up toolbar
    m_VToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                         _( "Deselect current tool" ), wxITEM_CHECK );

    m_VToolBar->AddTool( ID_LIBEDIT_PIN_BUTT, wxEmptyString, KiBitmap( pin_xpm ),
                         HELP_ADD_PIN, wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_TEXT_BUTT, wxEmptyString, KiBitmap( add_text_xpm ),
                         HELP_ADD_BODYTEXT, wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_RECT_BUTT, wxEmptyString, KiBitmap( add_rectangle_xpm ),
                         HELP_ADD_BODYRECT, wxITEM_CHECK );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, wxEmptyString, KiBitmap( add_circle_xpm ),
                         HELP_ADD_BODYCIRCLE, wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_ARC_BUTT, wxEmptyString, KiBitmap( add_arc_xpm ),
                         HELP_ADD_BODYARC, wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_BODY_LINE_BUTT, wxEmptyString, KiBitmap( add_polygon_xpm ),
                         HELP_ADD_BODYPOLYGON, wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, wxEmptyString, KiBitmap( anchor_xpm ),
                         _( "Move part anchor" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_IMPORT_BODY_BUTT, wxEmptyString, KiBitmap( import_xpm ),
                         _( "Import existing drawings" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_EXPORT_BODY_BUTT, wxEmptyString, KiBitmap( export_xpm ),
                         _( "Export current drawing" ), wxITEM_CHECK  );

    m_VToolBar->AddTool( ID_LIBEDIT_DELETE_ITEM_BUTT, wxEmptyString, KiBitmap( delete_body_xpm ),
                         HELP_DELETE_ITEMS, wxITEM_CHECK  );

    m_VToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    // Create the toolbar if not exists
    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new EDA_TOOLBAR( TOOLBAR_MAIN, this, ID_H_TOOLBAR, true );

    // Set up toolbar
    m_HToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_LIB, wxEmptyString, KiBitmap( save_library_xpm ),
                         _( "Save current library to disk" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SELECT_CURRENT_LIB, wxEmptyString, KiBitmap( library_xpm ),
                         _( "Select working library" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_DELETE_PART, wxEmptyString, KiBitmap( delete_xpm ),
                         _( "Delete component in current library" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_NEW_PART, wxEmptyString, KiBitmap( new_component_xpm ),
                         _( "Create a new component" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SELECT_PART, wxEmptyString,
                         KiBitmap( import_cmp_from_lib_xpm ),
                         _( "Load component to edit from the current library" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_NEW_PART_FROM_EXISTING, wxEmptyString,
                         KiBitmap( copycomponent_xpm ),
                         _( "Create a new component from the current one" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_PART, wxEmptyString,
                         KiBitmap( save_part_in_mem_xpm ),
                         _( "Update current component in current library" ) );

    m_HToolBar->AddTool( ImportPartId, wxEmptyString, KiBitmap( import_xpm ),
                         _( "Import component" ) );

    m_HToolBar->AddTool( ExportPartId, wxEmptyString, KiBitmap( export_xpm ),
                         _( "Export component" ) );

    m_HToolBar->AddTool( CreateNewLibAndSavePartId, wxEmptyString, KiBitmap( new_library_xpm ),
                         _( "Save current component to new library" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Undo last command" ), s_Schematic_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_HToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ), msg );
    msg = AddHotkeyName( _( "Redo the last command" ), s_Schematic_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_HToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_PART, wxEmptyString,
                         KiBitmap( part_properties_xpm ), _( "Edit component properties" ) );

    m_HToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, wxEmptyString, KiBitmap( add_text_xpm ),
                         _( "Add and remove fields and edit field properties" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_CHECK_PART, wxEmptyString, KiBitmap( erc_xpm ),
                         _( "Test for duplicate and off grid pins" ) );

    m_HToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_ZOOM_IN, s_Libedit_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, s_Libedit_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, s_Libedit_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, s_Libedit_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_DE_MORGAN_NORMAL_BUTT, wxEmptyString, KiBitmap( morgan1_xpm ),
                         _( "Show as \"De Morgan\" normal part" ), wxITEM_CHECK );
    m_HToolBar->AddTool( ID_DE_MORGAN_CONVERT_BUTT, wxEmptyString, KiBitmap( morgan2_xpm ),
                         _( "Show as \"De Morgan\" convert part" ), wxITEM_CHECK );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_LIBEDIT_VIEW_DOC, wxEmptyString, KiBitmap( datasheet_xpm ),
                         _( "Edit document file" ) );

    m_HToolBar->AddSeparator();
    m_partSelectBox = new wxComboBox( m_HToolBar,
                                      ID_LIBEDIT_SELECT_PART_NUMBER,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, NULL, wxCB_READONLY );
    m_HToolBar->AddControl( m_partSelectBox );

    m_aliasSelectBox = new wxComboBox( m_HToolBar,
                                       ID_LIBEDIT_SELECT_ALIAS,
                                       wxEmptyString,
                                       wxDefaultPosition,
                                       wxSize( LISTBOX_WIDTH, -1 ),
                                       0, NULL, wxCB_READONLY );
    m_HToolBar->AddControl( m_aliasSelectBox );

    m_HToolBar->AddSeparator();
    msg = _( "Edit pins per part or body style (Use carefully!)" );
    m_HToolBar->AddTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, wxEmptyString, KiBitmap( pin2pin_xpm ),
                         msg, wxITEM_CHECK );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_HToolBar->Realize();
}


void LIB_EDIT_FRAME::CreateOptionToolbar()
{
    if( m_OptionsToolBar )
        return;

    m_OptionsToolBar = new EDA_TOOLBAR( TOOLBAR_OPTION, this, ID_OPT_TOOLBAR, false );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ), _( "Units in inches" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_OptionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_OptionsToolBar->Realize();
}

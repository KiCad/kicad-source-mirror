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

#include <fctsys.h>
#include <hotkeys.h>
#include <eeschema_id.h>

#include <general.h>
#include <libeditframe.h>
#include <dialog_helpers.h>

#include <help_common_strings.h>

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
    if( m_drawToolBar != NULL )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ),
                            _( "Deselect current tool" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LIBEDIT_PIN_BUTT, wxEmptyString, KiBitmap( pin_xpm ),
                            HELP_ADD_PIN, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_TEXT_BUTT, wxEmptyString, KiBitmap( add_text_xpm ),
                            HELP_ADD_BODYTEXT, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_RECT_BUTT, wxEmptyString, KiBitmap( add_rectangle_xpm ),
                            HELP_ADD_BODYRECT, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, wxEmptyString, KiBitmap( add_circle_xpm ),
                            HELP_ADD_BODYCIRCLE, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_ARC_BUTT, wxEmptyString, KiBitmap( add_arc_xpm ),
                            HELP_ADD_BODYARC, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_LINE_BUTT, wxEmptyString, KiBitmap( add_polygon_xpm ),
                            HELP_ADD_BODYPOLYGON, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, wxEmptyString, KiBitmap( anchor_xpm ),
                            _( "Move part anchor" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_IMPORT_BODY_BUTT, wxEmptyString, KiBitmap( import_xpm ),
                            _( "Import existing drawings" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_EXPORT_BODY_BUTT, wxEmptyString, KiBitmap( export_xpm ),
                            _( "Export current drawing" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_DELETE_ITEM_BUTT, wxEmptyString, KiBitmap( delete_xpm ),
                            HELP_DELETE_ITEMS, wxITEM_CHECK  );

    m_drawToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    // Create the toolbar if not exists
    if( m_mainToolBar != NULL )
        return;

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_LIB, wxEmptyString,
                            KiBitmap( save_library_xpm ),
                            _( "Save current library to disk" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SELECT_CURRENT_LIB, wxEmptyString, KiBitmap( library_xpm ),
                            _( "Select working library" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_DELETE_PART, wxEmptyString, KiBitmap( delete_xpm ),
                            _( "Delete component in current library" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_TO_LIBVIEW, wxEmptyString, KiBitmap( library_browse_xpm ),
                            HELP_RUN_LIB_VIEWER );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_LIBEDIT_NEW_PART, wxEmptyString, KiBitmap( new_component_xpm ),
                            _( "Create a new component" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SELECT_PART, wxEmptyString,
                            KiBitmap( import_cmp_from_lib_xpm ),
                            _( "Load component to edit from the current library" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_NEW_PART_FROM_EXISTING, wxEmptyString,
                            KiBitmap( copycomponent_xpm ),
                            _( "Create a new component from the current one" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SAVE_CURRENT_PART, wxEmptyString,
                         KiBitmap( save_part_in_mem_xpm ),
                            _( "Update current component in current library" ) );

    m_mainToolBar->AddTool( ImportPartId, wxEmptyString, KiBitmap( import_xpm ),
                            _( "Import component" ) );

    m_mainToolBar->AddTool( ExportPartId, wxEmptyString, KiBitmap( export_xpm ),
                            _( "Export component" ) );

    m_mainToolBar->AddTool( CreateNewLibAndSavePartId, wxEmptyString, KiBitmap( new_library_xpm ),
                            _( "Save current component to new library" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Undo last command" ), g_Libedit_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiBitmap( undo_xpm ), msg );
    msg = AddHotkeyName( _( "Redo the last command" ), g_Libedit_Hokeys_Descr, HK_REDO,
                         IS_COMMENT );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiBitmap( redo_xpm ), msg );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_PART, wxEmptyString,
                            KiBitmap( part_properties_xpm ), _( "Edit component properties" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, wxEmptyString,
                            KiBitmap( add_text_xpm ),
                            _( "Add and remove fields and edit field properties" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_LIBEDIT_CHECK_PART, wxEmptyString, KiBitmap( erc_xpm ),
                            _( "Test for duplicate and off grid pins" ) );

    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( HELP_ZOOM_IN, g_Libedit_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Libedit_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Libedit_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( HELP_ZOOM_FIT, g_Libedit_Hokeys_Descr, HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_DE_MORGAN_NORMAL_BUTT, wxEmptyString, KiBitmap( morgan1_xpm ),
                            _( "Show as \"De Morgan\" normal part" ), wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_DE_MORGAN_CONVERT_BUTT, wxEmptyString, KiBitmap( morgan2_xpm ),
                            _( "Show as \"De Morgan\" convert part" ), wxITEM_CHECK );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_LIBEDIT_VIEW_DOC, wxEmptyString, KiBitmap( datasheet_xpm ),
                            _( "Edit document file" ) );

    m_mainToolBar->AddSeparator();
    m_partSelectBox = new wxComboBox( m_mainToolBar,
                                      ID_LIBEDIT_SELECT_PART_NUMBER,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, NULL, wxCB_READONLY );
    m_mainToolBar->AddControl( m_partSelectBox );

    m_aliasSelectBox = new wxComboBox( m_mainToolBar,
                                       ID_LIBEDIT_SELECT_ALIAS,
                                       wxEmptyString,
                                       wxDefaultPosition,
                                       wxSize( LISTBOX_WIDTH, -1 ),
                                       0, NULL, wxCB_READONLY );
    m_mainToolBar->AddControl( m_aliasSelectBox );

    m_mainToolBar->AddSeparator();
    msg = _( "Edit pins per part or body style (Use carefully!)" );
    m_mainToolBar->AddTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, wxEmptyString, KiBitmap( pin2pin_xpm ),
                            msg, wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_LIBEDIT_EDIT_PIN_BY_TABLE, wxEmptyString, KiBitmap( pin_table_xpm ),
                            _( "Show pin table" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void LIB_EDIT_FRAME::CreateOptionToolbar()
{
    if( m_optionsToolBar )
        return;

    m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                         wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString, KiBitmap( grid_xpm ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiBitmap( unit_inch_xpm ), _( "Units in inches" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiBitmap( unit_mm_xpm ),
                               _( "Units in millimeters" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiBitmap( cursor_shape_xpm ),
                               _( "Change cursor shape" ), wxITEM_CHECK );

    m_optionsToolBar->Realize();
}

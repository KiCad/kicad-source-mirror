/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <lib_edit_frame.h>
#include <dialog_helpers.h>
#include <bitmaps.h>

#include <help_common_strings.h>

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


void LIB_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiScaledBitmap( cursor_xpm, this ),
                            _( "Deselect current tool" ), wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LIBEDIT_PIN_BUTT, wxEmptyString, KiScaledBitmap( pin_xpm, this ),
                            HELP_ADD_PIN, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_TEXT_BUTT, wxEmptyString,
                            KiScaledBitmap( text_xpm, this ),
                            HELP_ADD_BODYTEXT, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_RECT_BUTT, wxEmptyString,
                            KiScaledBitmap( add_rectangle_xpm, this ),
                            HELP_ADD_BODYRECT, wxITEM_CHECK );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_CIRCLE_BUTT, wxEmptyString,
                            KiScaledBitmap( add_circle_xpm, this ),
                            HELP_ADD_BODYCIRCLE, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_ARC_BUTT, wxEmptyString,
                            KiScaledBitmap( add_arc_xpm, this ),
                            HELP_ADD_BODYARC, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_BODY_LINE_BUTT, wxEmptyString,
                            KiScaledBitmap( add_polygon_xpm, this ),
                            HELP_ADD_BODYPOLYGON, wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_ANCHOR_ITEM_BUTT, wxEmptyString,
                            KiScaledBitmap( anchor_xpm, this ),
                            _( "Move symbol anchor" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_IMPORT_BODY_BUTT, wxEmptyString,
                            KiScaledBitmap( import_xpm, this ),
                            _( "Import existing drawings" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_EXPORT_BODY_BUTT, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Export current drawing" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_DELETE_ITEM_BUTT, wxEmptyString,
                            KiScaledBitmap( delete_xpm, this ),
                            HELP_DELETE_ITEMS, wxITEM_CHECK  );

    m_drawToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                          KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_LIBEDIT_NEW_LIBRARY, wxEmptyString,
                            KiScaledBitmap( new_library_xpm, this ),
                            _( "Create a new library" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_ADD_LIBRARY, wxEmptyString,
                            KiScaledBitmap( add_library_xpm, this ),
                            _( "Add an existing library" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SAVE_LIBRARY, wxEmptyString,
                            KiScaledBitmap( save_library_xpm, this ),
                            _( "Save current library" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_LIBEDIT_NEW_PART, wxEmptyString,
                            KiScaledBitmap( new_component_xpm, this ),
                            _( "Create new symbol" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SAVE_PART, wxEmptyString,
                            KiScaledBitmap( save_part_xpm, this ),
                            _( "Save current symbol" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_IMPORT_PART, wxEmptyString,
                            KiScaledBitmap( import_part_xpm, this ),
                            _( "Import symbol" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_EXPORT_PART, wxEmptyString,
                            KiScaledBitmap( export_part_xpm, this ),
                            _( "Export symbol" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( wxID_PASTE, wxEmptyString, KiScaledBitmap( paste_xpm, this ),
                            _( "Paste" ) );

    KiScaledSeparator( m_mainToolBar, this );

    msg = AddHotkeyName( HELP_UNDO, g_Libedit_Hokeys_Descr, HK_UNDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString, KiScaledBitmap( undo_xpm, this ), msg );

    msg = AddHotkeyName( HELP_REDO, g_Libedit_Hokeys_Descr, HK_REDO, IS_COMMENT );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString, KiScaledBitmap( redo_xpm, this ), msg );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_PART, wxEmptyString,
                            KiScaledBitmap( part_properties_xpm, this ),
                            _( "Edit symbol properties" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_GET_FRAME_EDIT_FIELDS, wxEmptyString,
                            KiScaledBitmap( text_xpm, this ),
                            _( "Edit field properties" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_LIBEDIT_CHECK_PART, wxEmptyString, KiScaledBitmap( erc_xpm, this ),
                            _( "Check duplicate and off grid pins" ) );

    KiScaledSeparator( m_mainToolBar, this );

    msg = AddHotkeyName( HELP_ZOOM_REDRAW, g_Libedit_Hokeys_Descr, HK_ZOOM_REDRAW, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                            KiScaledBitmap( zoom_redraw_xpm, this ), msg );

    msg = AddHotkeyName( HELP_ZOOM_IN, g_Libedit_Hokeys_Descr, HK_ZOOM_IN, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiScaledBitmap( zoom_in_xpm, this ), msg );

    msg = AddHotkeyName( HELP_ZOOM_OUT, g_Libedit_Hokeys_Descr, HK_ZOOM_OUT, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiScaledBitmap( zoom_out_xpm, this ), msg );

    msg = AddHotkeyName( _( "Zoom to fit symbol" ), g_Libedit_Hokeys_Descr,
                         HK_ZOOM_AUTO, IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                            KiScaledBitmap( zoom_fit_in_page_xpm, this ), msg );

    m_mainToolBar->AddTool( ID_ZOOM_SELECTION, wxEmptyString, KiScaledBitmap( zoom_area_xpm, this ),
                            _( "Zoom to selection" ), wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                            KiScaledBitmap( morgan1_xpm, this ),
                            _( "Show as \"De Morgan\" normal symbol" ), wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                            KiScaledBitmap( morgan2_xpm, this ),
                            _( "Show as \"De Morgan\" convert symbol" ), wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_LIBEDIT_VIEW_DOC, wxEmptyString,
                            KiScaledBitmap( datasheet_xpm, this ),
                            _( "Show associated datasheet or document" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_partSelectBox = new wxComboBox( m_mainToolBar,
                                      ID_LIBEDIT_SELECT_PART_NUMBER,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, nullptr, wxCB_READONLY );
    m_mainToolBar->AddControl( m_partSelectBox );

    m_aliasSelectBox = new wxComboBox( m_mainToolBar,
                                       ID_LIBEDIT_SELECT_ALIAS,
                                       wxEmptyString,
                                       wxDefaultPosition,
                                       wxSize( LISTBOX_WIDTH, -1 ),
                                       0, nullptr, wxCB_READONLY );
    m_mainToolBar->AddControl( m_aliasSelectBox );

    m_mainToolBar->AddSeparator();
    KiScaledSeparator( m_mainToolBar, this );
    msg = _( "Synchronized pin edit mode\n"
             "Synchronized pin edit mode propagates to other units all pin changes except pin number modification.\n"
             "Enabled by default for multiunit parts with interchangeable units." );
    m_mainToolBar->AddTool( ID_LIBEDIT_SYNC_PIN_EDIT, wxEmptyString,
                            KiScaledBitmap( pin2pin_xpm, this ), msg, wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_LIBEDIT_EDIT_PIN_BY_TABLE, wxEmptyString,
                            KiScaledBitmap( pin_table_xpm, this ),
                            _( "Show pin table" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new wxAuiToolBar( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                             KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SHOW_GRID, wxEmptyString,
                               KiScaledBitmap( grid_xpm, this ),
                               _( "Turn grid off" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_INCH, wxEmptyString,
                               KiScaledBitmap( unit_inch_xpm, this ), _( "Set units to inches" ),
                               wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_UNIT_MM, wxEmptyString,
                               KiScaledBitmap( unit_mm_xpm, this ),
                               _( "Set units to millimeters" ), wxITEM_CHECK );

#ifndef __APPLE__
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape" ), wxITEM_CHECK );
#else
    m_optionsToolBar->AddTool( ID_TB_OPTIONS_SELECT_CURSOR, wxEmptyString,
                               KiScaledBitmap( cursor_shape_xpm, this ),
                               _( "Change cursor shape (not supported in Legacy Toolset)" ),
                               wxITEM_CHECK  );
#endif

    m_optionsToolBar->AddTool( ID_LIBEDIT_SHOW_ELECTRICAL_TYPE, wxEmptyString,
                               KiScaledBitmap( pin_show_etype_xpm, this ),
                               _( "Show pins electrical type" ), wxITEM_CHECK );

    m_optionsToolBar->AddTool( ID_LIBEDIT_SHOW_HIDE_SEARCH_TREE, wxEmptyString,
                               KiScaledBitmap( search_tree_xpm, this ),
                               _( "Toggles the search tree" ), wxITEM_CHECK );

    m_optionsToolBar->Realize();
}

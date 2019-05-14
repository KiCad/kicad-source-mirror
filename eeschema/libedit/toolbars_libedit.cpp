/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <ee_hotkeys.h>
#include <eeschema_id.h>
#include <tool/action_toolbar.h>
#include <general.h>
#include <lib_edit_frame.h>
#include <dialog_helpers.h>
#include <bitmaps.h>

#include <help_common_strings.h>
#include <tools/ee_actions.h>

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
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->Add( EE_ACTIONS::selectionTool,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolPin,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolText,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolRectangle,  ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolCircle,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolArc,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolLines,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolAnchor,    ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddTool( ID_LIBEDIT_IMPORT_BODY_BUTT, wxEmptyString,
                            KiScaledBitmap( import_xpm, this ),
                            _( "Import existing drawings" ), wxITEM_CHECK  );

    m_drawToolBar->AddTool( ID_LIBEDIT_EXPORT_BODY_BUTT, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Export current drawing" ), wxITEM_CHECK  );

    m_drawToolBar->Add( EE_ACTIONS::deleteItemCursor,       ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateHToolbar()
{
    wxString msg;

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_LIBEDIT_NEW_PART, wxEmptyString,
                            KiScaledBitmap( new_component_xpm, this ),
                            _( "Create new symbol" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_SAVE_ALL, wxEmptyString,
                            KiScaledBitmap( save_xpm, this ),
                            _( "Save all changes" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( EE_ACTIONS::symbolProperties );
    m_mainToolBar->Add( EE_ACTIONS::pinTable );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_LIBEDIT_VIEW_DOC, wxEmptyString,
                            KiScaledBitmap( datasheet_xpm, this ),
                            _( "Show associated datasheet or document" ) );

    m_mainToolBar->AddTool( ID_LIBEDIT_CHECK_PART, wxEmptyString, KiScaledBitmap( erc_xpm, this ),
                            _( "Check duplicate and off grid pins" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_DE_MORGAN_NORMAL_BUTT, wxEmptyString,
                            KiScaledBitmap( morgan1_xpm, this ),
                            _( "Show as \"De Morgan\" normal symbol" ), wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_DE_MORGAN_CONVERT_BUTT, wxEmptyString,
                            KiScaledBitmap( morgan2_xpm, this ),
                            _( "Show as \"De Morgan\" convert symbol" ), wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_partSelectBox = new wxComboBox( m_mainToolBar,
                                      ID_LIBEDIT_SELECT_PART_NUMBER,
                                      wxEmptyString,
                                      wxDefaultPosition,
                                      wxSize( LISTBOX_WIDTH, -1 ),
                                      0, nullptr, wxCB_READONLY );
    m_mainToolBar->AddControl( m_partSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    msg = _( "Synchronized pin edit mode\n"
             "Synchronized pin edit mode propagates to other units all pin changes except pin number modification.\n"
             "Enabled by default for multiunit parts with interchangeable units." );
    m_mainToolBar->AddTool( ID_LIBEDIT_SYNC_PIN_EDIT, wxEmptyString,
                            KiScaledBitmap( pin2pin_xpm, this ), msg, wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ADD_PART_TO_SCHEMATIC, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Add symbol to schematic" ) );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void LIB_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR,
                                               wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    ACTION_TOOLBAR* optToolbar = static_cast<ACTION_TOOLBAR*>( m_optionsToolBar );

    optToolbar->Add( ACTIONS::toggleGrid,             ACTION_TOOLBAR::TOGGLE );
    optToolbar->Add( ACTIONS::imperialUnits,          ACTION_TOOLBAR::TOGGLE );
    optToolbar->Add( ACTIONS::metricUnits,            ACTION_TOOLBAR::TOGGLE );
    optToolbar->Add( ACTIONS::toggleCursorStyle,      ACTION_TOOLBAR::TOGGLE );
    optToolbar->Add( EE_ACTIONS::showElectricalTypes, ACTION_TOOLBAR::TOGGLE );
    optToolbar->Add( EE_ACTIONS::showComponentTree,   ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->Realize();
}


void LIB_EDIT_FRAME::SyncMenusAndToolbars()
{
    m_mainToolBar->Toggle( ACTIONS::zoomTool, GetToolId() == ID_ZOOM_SELECTION );
    m_mainToolBar->Refresh();

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid, IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits, GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits, GetUserUnits() == INCHES );

    KIGFX::GAL_DISPLAY_OPTIONS& galOpts = GetGalDisplayOptions();
    m_optionsToolBar->Toggle( ACTIONS::toggleCursorStyle, galOpts.m_fullscreenCursor );

    m_optionsToolBar->Toggle( EE_ACTIONS::showElectricalTypes, GetShowElectricalType() );
    m_optionsToolBar->Toggle( EE_ACTIONS::showComponentTree, IsSearchTreeShown() );

    m_optionsToolBar->Refresh();

    m_drawToolBar->Toggle( EE_ACTIONS::selectionTool,        GetToolId() == ID_NO_TOOL_SELECTED );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSymbolPin,       GetToolId() == ID_LIBEDIT_PIN_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSymbolText,      GetToolId() == ID_LIBEDIT_BODY_TEXT_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::drawSymbolRectangle,  GetToolId() == ID_LIBEDIT_BODY_RECT_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::drawSymbolCircle,     GetToolId() == ID_LIBEDIT_BODY_CIRCLE_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::drawSymbolArc,        GetToolId() == ID_LIBEDIT_BODY_ARC_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::drawSymbolLines,      GetToolId() == ID_LIBEDIT_BODY_LINE_BUTT );
    m_drawToolBar->Toggle( EE_ACTIONS::placeSymbolAnchor,    GetToolId() == ID_LIBEDIT_ANCHOR_ITEM_BUTT );

    m_drawToolBar->Refresh();
}

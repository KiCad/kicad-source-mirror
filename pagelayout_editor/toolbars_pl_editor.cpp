/**
 * @file toolbars_pl_editor.cpp
 * @brief Build tool bars
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <common.h>
#include <macros.h>
#include <bitmaps.h>
#include <pl_editor_id.h>
#include <pl_editor_frame.h>
#include <hotkeys.h>

void PL_EDITOR_FRAME::ReCreateHToolbar( void )
{
    if( m_mainToolBar != NULL )
        return;

    wxString      msg;

    // Standard file commands
    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    m_mainToolBar->AddTool( wxID_NEW, wxEmptyString, KiBitmap( pagelayout_new_xpm ),
                            _( "New page layout design" ) );

    m_mainToolBar->AddTool( wxID_OPEN, wxEmptyString, KiBitmap( pagelayout_load_xpm ),
                            _( "Load a page layout file. Previous data will be deleted" ) );

    m_mainToolBar->AddTool( wxID_SAVE, wxEmptyString, KiBitmap( save_project_xpm ),
                            _( "Save page layout design" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_SHEET_SET, wxEmptyString, KiBitmap( sheetset_xpm ),
                            _( "Page settings" ) );

    m_mainToolBar->AddTool( wxID_PRINT, wxEmptyString, KiBitmap( print_button_xpm ),
                            _( "Print page layout" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_CUT, wxEmptyString, KiBitmap( delete_xpm ),
                            _( "Delete selected item" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( wxID_UNDO, wxEmptyString,
                            KiBitmap( undo_xpm ), wxEmptyString );
    m_mainToolBar->AddTool( wxID_REDO, wxEmptyString,
                            KiBitmap( redo_xpm ), wxEmptyString );

    // Standard Zoom controls:
    m_mainToolBar->AddSeparator();
    msg = AddHotkeyName( _( "Zoom in" ), PlEditorHokeysDescr, HK_ZOOM_IN,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom out" ), PlEditorHokeysDescr, HK_ZOOM_OUT,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ), msg );

    msg = AddHotkeyName( _( "Redraw view" ), PlEditorHokeysDescr, HK_ZOOM_REDRAW,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString, KiBitmap( zoom_redraw_xpm ), msg );

    msg = AddHotkeyName( _( "Zoom auto" ), PlEditorHokeysDescr, HK_ZOOM_AUTO,  IS_COMMENT );
    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ), msg );

    // Display mode switch
    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_SHOW_REAL_MODE, wxEmptyString,
                            KiBitmap( pagelayout_normal_view_mode_xpm ),
                            _( "Show title block like it will be displayed in applications\n"
                               "texts with format are replaced by the full text"),
                            wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_SHOW_PL_EDITOR_MODE,
                            wxEmptyString, KiBitmap( pagelayout_special_view_mode_xpm ),
                            _( "Show title block in edit mode: texts are shown as is:\n"
                               "texts with format are displayed with no change"),
                            wxITEM_CHECK );

    m_mainToolBar->AddSeparator();

    wxString choiceList[5] =
    {
        _("Left Top paper corner"),
        _("Right Bottom page corner"),
        _("Left Bottom page corner"),
        _("Right Top page corner"),
        _("Left Top page corner")
    };

    m_originSelectBox = new wxChoice( m_mainToolBar, ID_SELECT_COORDINATE_ORIGIN,
                                      wxDefaultPosition, wxDefaultSize,
                                      5, choiceList );
    m_mainToolBar->AddControl( m_originSelectBox );
    m_originSelectBox->SetToolTip( _(" Origin of coordinates displayed to the status bar") );

    int minwidth = 0;
    for( int ii = 0; ii < 5; ii++ )
    {
        int width = GetTextSize( choiceList[ii], m_originSelectBox ).x;
        minwidth = std::max( minwidth, width );
    }
    m_originSelectBox->SetMinSize( wxSize( minwidth, -1 ) );
    m_originSelectBox->SetSelection( m_originSelectChoice );

    wxString pageList[5] =
    {
        _("Page 1"),
        _("Other pages")
    };

    m_pageSelectBox = new wxChoice( m_mainToolBar, ID_SELECT_PAGE_NUMBER,
                                    wxDefaultPosition, wxDefaultSize,
                                    2, pageList );
    m_mainToolBar->AddControl( m_pageSelectBox );
    m_pageSelectBox->SetToolTip( _("Simulate page 1 or other pages to show how items\n"\
                                 "which are not on all page are displayed") );
    m_pageSelectBox->SetSelection( 0 );


    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void PL_EDITOR_FRAME::ReCreateVToolbar( void )
{
    if( m_drawToolBar )
        return;

    m_drawToolBar = new wxAuiToolBar( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_VERTICAL );

    // Set up toolbar
    m_drawToolBar->AddTool( ID_NO_TOOL_SELECTED, wxEmptyString, KiBitmap( cursor_xpm ) );
    m_drawToolBar->AddSeparator();

    m_drawToolBar->Realize();
}


void PL_EDITOR_FRAME::ReCreateOptToolbar( void )
{
}

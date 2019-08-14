/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2013-2019 CERN
 * @author Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tools/pl_actions.h>
#include <pl_editor_id.h>
#include <pl_editor_frame.h>

void PL_EDITOR_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    wxString      msg;

    m_mainToolBar->Add( ACTIONS::doNew );
    m_mainToolBar->Add( ACTIONS::open );
    m_mainToolBar->Add( ACTIONS::save );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::print );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( PL_ACTIONS::showInspector );
    m_mainToolBar->Add( PL_ACTIONS::previewSettings );

    // Display mode switch
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_SHOW_REAL_MODE, wxEmptyString,
                            KiScaledBitmap( pagelayout_normal_view_mode_xpm, this ),
                            _( "Show title block in preview mode:\n"
                               "text placeholders will be replaced with preview data"),
                            wxITEM_CHECK );
    m_mainToolBar->AddTool( ID_SHOW_PL_EDITOR_MODE, wxEmptyString,
                            KiScaledBitmap( pagelayout_special_view_mode_xpm, this ),
                            _( "Show title block in edit mode:\n"
                               "text placeholders show as %-tokens"),
                            wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    wxString choiceList[5] =
    {
        _("Left Top paper corner"),
        _("Right Bottom page corner"),
        _("Left Bottom page corner"),
        _("Right Top page corner"),
        _("Left Top page corner")
    };

    m_originSelectBox = new wxChoice( m_mainToolBar, ID_SELECT_COORDINATE_ORIGIN,
                                      wxDefaultPosition, wxDefaultSize, 5, choiceList );
    m_mainToolBar->AddControl( m_originSelectBox );
    m_originSelectBox->SetToolTip( _("Origin of coordinates displayed to the status bar") );

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
                                    wxDefaultPosition, wxDefaultSize, 2, pageList );
    m_mainToolBar->AddControl( m_pageSelectBox );
    m_pageSelectBox->SetToolTip( _("Simulate page 1 or other pages to show how items\n"\
                                 "which are not on all page are displayed") );
    m_pageSelectBox->SetSelection( 0 );


    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void PL_EDITOR_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_drawToolBar->Add( ACTIONS::selectionTool,              ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( PL_ACTIONS::drawLine,                ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::drawRectangle,           ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::placeText,               ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::placeImage,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::appendImportedWorksheet, ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( ACTIONS::deleteTool,                 ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->Realize();
}


void PL_EDITOR_FRAME::ReCreateOptToolbar()
{
}


void PL_EDITOR_FRAME::SyncToolbars()
{
#define TOGGLE_TOOL( toolbar, tool ) toolbar->Toggle( tool, IsCurrentTool( tool ) )

    m_mainToolBar->Toggle( ACTIONS::save, GetScreen() && GetScreen()->IsModify() );
    m_mainToolBar->Toggle( ACTIONS::undo, GetScreen() && GetScreen()->GetUndoCommandCount() > 0 );
    m_mainToolBar->Toggle( ACTIONS::redo, GetScreen() && GetScreen()->GetRedoCommandCount() > 0 );
    TOGGLE_TOOL( m_mainToolBar, ACTIONS::zoomTool );
    m_mainToolBar->Refresh();

    TOGGLE_TOOL( m_drawToolBar, ACTIONS::selectionTool );
    TOGGLE_TOOL( m_drawToolBar, PL_ACTIONS::drawLine );
    TOGGLE_TOOL( m_drawToolBar, PL_ACTIONS::drawRectangle );
    TOGGLE_TOOL( m_drawToolBar, PL_ACTIONS::placeText );
    TOGGLE_TOOL( m_drawToolBar, PL_ACTIONS::placeImage );
    TOGGLE_TOOL( m_drawToolBar, ACTIONS::deleteTool );

    m_drawToolBar->Toggle( PL_ACTIONS::appendImportedWorksheet, false );  // Not really a tool
    m_drawToolBar->Refresh();
}

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

#include <bitmaps.h>
#include <tool/action_toolbar.h>

#include "pl_editor_id.h"
#include "pl_editor_frame.h"
#include "tools/pl_actions.h"

void PL_EDITOR_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    wxString      msg;

    m_mainToolBar->Add( ACTIONS::doNew );
    m_mainToolBar->Add( ACTIONS::open );
    m_mainToolBar->Add( ACTIONS::save );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::print );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PL_ACTIONS::showInspector );
    m_mainToolBar->Add( PL_ACTIONS::previewSettings );

    // Display mode switch
    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PL_ACTIONS::layoutNormalMode, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( PL_ACTIONS::layoutEditMode,   ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->AddScaledSeparator( this );

    wxString choiceList[5] =
    {
        _("Left Top paper corner"),
        _("Right Bottom page corner"),
        _("Left Bottom page corner"),
        _("Right Top page corner"),
        _("Left Top page corner")
    };

    if( !m_originSelectBox )
    {
        m_originSelectBox = new wxChoice( m_mainToolBar, ID_SELECT_COORDINATE_ORIGIN,
                                          wxDefaultPosition, wxDefaultSize, 5, choiceList );
    }


    m_mainToolBar->AddControl( m_originSelectBox );
    m_originSelectBox->SetToolTip( _("Origin of coordinates displayed to the status bar") );

    int minwidth = 0;

    for( int ii = 0; ii < 5; ii++ )
    {
        int width = KIUI::GetTextSize( choiceList[ii], m_originSelectBox ).x;
        minwidth = std::max( minwidth, width );
    }

    m_originSelectBox->SetMinSize( wxSize( minwidth, -1 ) );
    m_originSelectBox->SetSelection( m_originSelectChoice );

    wxString pageList[5] =
    {
        _("Page 1"),
        _("Other pages")
    };

    if( !m_pageSelectBox )
    {
        m_pageSelectBox = new wxChoice( m_mainToolBar, ID_SELECT_PAGE_NUMBER,
                                        wxDefaultPosition, wxDefaultSize, 2, pageList );
    }

    m_mainToolBar->AddControl( m_pageSelectBox );
    m_pageSelectBox->SetToolTip( _("Simulate page 1 or other pages to show how items\n"\
                                 "which are not on all page are displayed") );
    m_pageSelectBox->SetSelection( 0 );


    // Go through and ensure the comboboxes are the correct size, since the strings in the
    // box could have changed widths.
    m_mainToolBar->UpdateControlWidth( ID_SELECT_COORDINATE_ORIGIN );
    m_mainToolBar->UpdateControlWidth( ID_SELECT_PAGE_NUMBER );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();
}


void PL_EDITOR_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
    {
        m_drawToolBar->ClearToolbar();
    }
    else
    {
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_drawToolBar->SetAuiManager( &m_auimgr );
    }

    m_drawToolBar->Add( ACTIONS::selectionTool,              ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( PL_ACTIONS::drawLine,                ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::drawRectangle,           ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::placeText,               ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::placeImage,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PL_ACTIONS::appendImportedWorksheet, ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( ACTIONS::deleteTool,                 ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->KiRealize();
}


void PL_EDITOR_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR,
                                               wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    m_optionsToolBar->Add( ACTIONS::toggleGrid, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits, ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->KiRealize();
}


void PL_EDITOR_FRAME::UpdateToolbarControlSizes()
{
    if( m_mainToolBar )
    {
        // Update the item widths
        m_mainToolBar->UpdateControlWidth( ID_SELECT_COORDINATE_ORIGIN );
        m_mainToolBar->UpdateControlWidth( ID_SELECT_PAGE_NUMBER );

        // Update the toolbar with the new widths
        m_mainToolBar->KiRealize();
    }
}

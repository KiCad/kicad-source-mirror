/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tool/ui/toolbar_context_menu_registry.h>
#include <tools/pl_actions.h>
#include <tools/pl_selection_tool.h>
#include <wx/choice.h>

#include "pl_editor_id.h"
#include "pl_editor_frame.h"
#include <toolbars_pl_editor.h>


std::optional<TOOLBAR_CONFIGURATION> PL_EDITOR_TOOLBAR_SETTINGS::DefaultToolbarConfig( TOOLBAR_LOC aToolbar )
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    switch( aToolbar )
    {
    // No aux toolbar
    case TOOLBAR_LOC::TOP_AUX:
        return std::nullopt;

    case TOOLBAR_LOC::LEFT:
        config.AppendAction( ACTIONS::toggleGrid )
              .WithContextMenu(
                      []( TOOL_MANAGER* aToolMgr )
                      {
                          PL_SELECTION_TOOL* selTool = aToolMgr->GetTool<PL_SELECTION_TOOL>();
                          auto               menu = std::make_unique<ACTION_MENU>( false, selTool );
                          menu->Add( ACTIONS::gridProperties );
                          return menu;
                      } )
              .AppendGroup( TOOLBAR_GROUP_CONFIG( _( "Units" ) )
                            .AddAction( ACTIONS::millimetersUnits )
                            .AddAction( ACTIONS::inchesUnits )
                            .AddAction( ACTIONS::milsUnits ) );
        break;

    case TOOLBAR_LOC::RIGHT:
        config.AppendAction( ACTIONS::selectionTool );

        config.AppendSeparator()
              .AppendAction( PL_ACTIONS::drawLine )
              .AppendAction( PL_ACTIONS::drawRectangle )
              .AppendAction( PL_ACTIONS::placeText )
              .AppendAction( PL_ACTIONS::placeImage )
              .AppendAction( PL_ACTIONS::appendImportedDrawingSheet );

        config.AppendSeparator()
              .AppendAction( ACTIONS::deleteTool );
        break;

    case TOOLBAR_LOC::TOP_MAIN:
        config.AppendAction( ACTIONS::doNew )
              .AppendAction( ACTIONS::open )
              .AppendAction( ACTIONS::save );

        config.AppendSeparator()
              .AppendAction( ACTIONS::print );

        config.AppendSeparator()
              .AppendAction( ACTIONS::undo )
              .AppendAction( ACTIONS::redo );

        config.AppendSeparator()
              .AppendAction( ACTIONS::zoomRedraw )
              .AppendAction( ACTIONS::zoomInCenter )
              .AppendAction( ACTIONS::zoomOutCenter )
              .AppendAction( ACTIONS::zoomFitScreen )
              .AppendAction( ACTIONS::zoomTool );

        config.AppendSeparator()
              .AppendAction( PL_ACTIONS::showInspector )
              .AppendAction( PL_ACTIONS::previewSettings );

        // Display mode switch
        config.AppendSeparator()
              .AppendAction( PL_ACTIONS::layoutNormalMode )
              .AppendAction( PL_ACTIONS::layoutEditMode );

        config.AppendSeparator()
              .AppendControl( PL_EDITOR_ACTION_TOOLBAR_CONTROLS::originSelector )
              .AppendControl( PL_EDITOR_ACTION_TOOLBAR_CONTROLS::pageSelect );
        break;
    }

    // clang-format on
    return config;
}


void PL_EDITOR_FRAME::configureToolbars()
{
    EDA_DRAW_FRAME::configureToolbars();

    auto originSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                if( !m_originSelectBox )
                {
                    m_originSelectBox = new wxChoice( aToolbar, ID_SELECT_COORDINATE_ORIGIN,
                                                      wxDefaultPosition, wxDefaultSize, 5, m_originChoiceList );
                }

                m_originSelectBox->SetToolTip( _("Origin of coordinates displayed to the status bar") );
                m_originSelectBox->SetSelection( m_originSelectChoice );

                aToolbar->Add(  m_originSelectBox );
            };

    RegisterCustomToolbarControlFactory( PL_EDITOR_ACTION_TOOLBAR_CONTROLS::originSelector, originSelectorFactory );


    auto pageSelectorFactory =
            [this]( ACTION_TOOLBAR* aToolbar )
            {
                wxString pageList[5] =
                {
                    _("Page 1"),
                    _("Other pages")
                };

                if( !m_pageSelectBox )
                {
                    m_pageSelectBox = new wxChoice( aToolbar, ID_SELECT_PAGE_NUMBER,
                                                    wxDefaultPosition, wxDefaultSize, 2, pageList );
                }

                m_pageSelectBox->SetToolTip( _("Simulate page 1 or other pages to show how items\n"\
                                               "which are not on all page are displayed") );
                m_pageSelectBox->SetSelection( 0 );

                aToolbar->Add( m_pageSelectBox );
            };

    RegisterCustomToolbarControlFactory( PL_EDITOR_ACTION_TOOLBAR_CONTROLS::pageSelect, pageSelectorFactory );
}


void PL_EDITOR_FRAME::ClearToolbarControl( int aId )
{
    EDA_DRAW_FRAME::ClearToolbarControl( aId );

    switch( aId )
    {
    case ID_SELECT_COORDINATE_ORIGIN: m_originSelectBox = nullptr; break;
    case ID_SELECT_PAGE_NUMBER:       m_pageSelectBox = nullptr;   break;
    }
}


ACTION_TOOLBAR_CONTROL PL_EDITOR_ACTION_TOOLBAR_CONTROLS::originSelector( "control.OriginSelector",
                                                                          _( "Origin selector" ),
                                                                          _( "Select the origin of the status bar coordinates" ),
                                                                          { FRAME_PL_EDITOR } );
ACTION_TOOLBAR_CONTROL PL_EDITOR_ACTION_TOOLBAR_CONTROLS::pageSelect( "control.PageSelect",
                                                                      _( "Page selector" ),
                                                                      _( "Select the page to simulate item displays" ),
                                                                      { FRAME_PL_EDITOR } );



void PL_EDITOR_FRAME::UpdateToolbarControlSizes()
{
    // Ensure the origin selector is a minimum size
    int minwidth = 0;

    for( int ii = 0; ii < 5; ii++ )
    {
        int width = KIUI::GetTextSize( m_originChoiceList[ii], m_originSelectBox ).x;
        minwidth = std::max( minwidth, width );
    }

    m_originSelectBox->SetMinSize( wxSize( minwidth, -1 ) );

    // Base class actually will go through and update the sizes of the controls
    EDA_DRAW_FRAME::UpdateToolbarControlSizes();
}

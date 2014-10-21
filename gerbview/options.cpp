/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file options.cpp
 * @brief Set some general options of GerbView.
 */


#include <fctsys.h>
#include <common.h>
#include <class_drawpanel.h>

#include <gerbview.h>
#include <gerbview_id.h>


/**
 * Function OnSelectOptionToolbar
 *  called to validate current choices
 */
void GERBVIEW_FRAME::OnSelectOptionToolbar( wxCommandEvent& event )
{
    int id = event.GetId();
    bool state;
    switch( id )
    {
        case ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG:
            state = ! m_show_layer_manager_tools;
            id = ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR;
            break;

        default:
            state = m_optionsToolBar->GetToolToggled( id );
            break;
    }

    switch( id )
    {
    case ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH:
        m_DisplayOptions.m_DisplayFlashedItemsFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_LINES_SKETCH:
        m_DisplayOptions.m_DisplayLinesFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_POLYGONS_SKETCH:
        m_DisplayOptions.m_DisplayPolygonsFill = not state;
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_DCODES:
        SetElementVisibility( DCODES_VISIBLE, state );
        m_canvas->Refresh( true );
        break;

    case ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR:
        // show/hide auxiliary Vertical layers and visibility manager toolbar
        m_show_layer_manager_tools = state;
        m_auimgr.GetPane( wxT( "m_LayersManagerToolBar" ) ).Show( m_show_layer_manager_tools );
        m_auimgr.Update();
        GetMenuBar()->SetLabel( ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                                m_show_layer_manager_tools ?
                                _("Hide &Layers Manager" ) : _("Show &Layers Manager" ));
        break;

    default:
        wxMessageBox( wxT( "GERBVIEW_FRAME::OnSelectOptionToolbar error" ) );
        break;
    }
}


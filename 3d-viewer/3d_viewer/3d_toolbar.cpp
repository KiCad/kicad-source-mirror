/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file 3d_toolbar.cpp
 */

#include <fctsys.h>

#include <eda_3d_viewer.h>
#include <3d_canvas/cinfo3d_visu.h>
#include <menus_helpers.h>
#include <3d_viewer_id.h>


void EDA_3D_VIEWER::ReCreateMainToolbar()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::ReCreateMainToolbar" );

    if( m_mainToolBar != NULL )
    {
        // Simple update to the list of old files.
        SetToolbars();
        return;
    }

    m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                      KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_mainToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                            KiBitmap( import3d_xpm ), _( "Reload board" ) );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                            KiBitmap( copy_xpm ),
                            _( "Copy 3D image to clipboard" ) );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_TOOL_SET_VISIBLE_ITEMS, wxEmptyString,
                            KiBitmap( read_setup_xpm ),
                            _( "Set display options, and some layers visibility" ) );
    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_RENDER_CURRENT_VIEW, wxEmptyString, KiBitmap( render_mode_xpm ),
                            _( "Render current view using Raytracing" ), wxITEM_CHECK );

    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ),
                            _( "Zoom in" ) );

    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ),
                            _( "Zoom out" ) );

    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                            KiBitmap( zoom_redraw_xpm ),
                            _( "Redraw view" ) );

    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ),
                            _( "Zoom to fit 3D model" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_X_NEG, wxEmptyString,
                            KiBitmap( rotate_neg_x_xpm ),
                            _( "Rotate X Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_X_POS, wxEmptyString,
                            KiBitmap( rotate_pos_x_xpm ),
                            _( "Rotate X Counterclockwise" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_Y_NEG, wxEmptyString,
                            KiBitmap( rotate_neg_y_xpm ),
                            _( "Rotate Y Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Y_POS, wxEmptyString,
                            KiBitmap( rotate_pos_y_xpm ),
                            _( "Rotate Y Counterclockwise" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ROTATE3D_Z_NEG, wxEmptyString,
                            KiBitmap( rotate_neg_z_xpm ),
                            _( "Rotate Z Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Z_POS, wxEmptyString,
                            KiBitmap( rotate_pos_z_xpm ),
                            _( "Rotate Z Counterclockwise" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_MOVE3D_LEFT, wxEmptyString, KiBitmap( left_xpm ),
                            _( "Move left" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_RIGHT, wxEmptyString, KiBitmap( right_xpm ),
                            _( "Move right" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_UP, wxEmptyString, KiBitmap( up_xpm ),
                            _( "Move up" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_DOWN, wxEmptyString, KiBitmap( down_xpm ),
                            _( "Move down" ) );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->AddTool( ID_ORTHO, wxEmptyString, KiBitmap( ortho_xpm ),
                            _( "Enable/Disable orthographic projection" ),
                            wxITEM_CHECK );

    m_mainToolBar->Realize();
}


void EDA_3D_VIEWER::SetToolbars()
{
}

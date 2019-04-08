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

    wxWindowUpdateLocker dummy( this );

    if( m_mainToolBar )
    {
        m_mainToolBar->Clear();
    }
    else
    {
        m_mainToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
    }

    // Set up toolbar
    m_mainToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                            KiScaledBitmap( import3d_xpm, this ), _( "Reload board" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                            KiScaledBitmap( copy_xpm, this ),
                            _( "Copy 3D image to clipboard" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_TOOL_SET_VISIBLE_ITEMS, wxEmptyString,
                            KiScaledBitmap( read_setup_xpm, this ),
                            _( "Set display options, and some layers visibility" ) );
    m_mainToolBar->AddSeparator();

    m_mainToolBar->AddTool( ID_RENDER_CURRENT_VIEW, wxEmptyString,
                            KiScaledBitmap( render_mode_xpm, this ),
                            _( "Render current view using Raytracing" ), wxITEM_CHECK );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ZOOM_IN, wxEmptyString,
                            KiScaledBitmap( zoom_in_xpm, this ),
                            _( "Zoom in" ) );

    m_mainToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString,
                            KiScaledBitmap( zoom_out_xpm, this ),
                            _( "Zoom out" ) );

    m_mainToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                            KiScaledBitmap( zoom_redraw_xpm, this ),
                            _( "Redraw view" ) );

    m_mainToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString,
                            KiScaledBitmap( zoom_fit_in_page_xpm, this ),
                            _( "Zoom to fit 3D model" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ROTATE3D_X_NEG, wxEmptyString,
                            KiScaledBitmap( rotate_neg_x_xpm, this ),
                            _( "Rotate X Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_X_POS, wxEmptyString,
                            KiScaledBitmap( rotate_pos_x_xpm, this ),
                            _( "Rotate X Counterclockwise" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ROTATE3D_Y_NEG, wxEmptyString,
                            KiScaledBitmap( rotate_neg_y_xpm, this ),
                            _( "Rotate Y Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Y_POS, wxEmptyString,
                            KiScaledBitmap( rotate_pos_y_xpm, this ),
                            _( "Rotate Y Counterclockwise" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ROTATE3D_Z_NEG, wxEmptyString,
                            KiScaledBitmap( rotate_neg_z_xpm, this ),
                            _( "Rotate Z Clockwise" ) );

    m_mainToolBar->AddTool( ID_ROTATE3D_Z_POS, wxEmptyString,
                            KiScaledBitmap( rotate_pos_z_xpm, this ),
                            _( "Rotate Z Counterclockwise" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_MOVE3D_LEFT, wxEmptyString,
                            KiScaledBitmap( left_xpm, this ),
                            _( "Move left" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_RIGHT, wxEmptyString,
                            KiScaledBitmap( right_xpm, this ),
                            _( "Move right" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_UP, wxEmptyString,
                            KiScaledBitmap( up_xpm, this ),
                            _( "Move up" ) );

    m_mainToolBar->AddTool( ID_MOVE3D_DOWN, wxEmptyString,
                            KiScaledBitmap( down_xpm, this ),
                            _( "Move down" ) );

    KiScaledSeparator( m_mainToolBar, this );

    m_mainToolBar->AddTool( ID_ORTHO, wxEmptyString,
                            KiScaledBitmap( ortho_xpm, this ),
                            _( "Enable/Disable orthographic projection" ),
                            wxITEM_CHECK );

    m_mainToolBar->Realize();
}
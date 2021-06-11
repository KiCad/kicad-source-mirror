/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/wupdlock.h>

#include <bitmaps.h>
#include <eda_3d_viewer.h>
#include <menus_helpers.h>
#include <tool/action_toolbar.h>
#include <tools/3d_actions.h>
#include <3d_viewer_id.h>

void EDA_3D_VIEWER::ReCreateMainToolbar()
{
    wxLogTrace( m_logTrace, "EDA_3D_VIEWER::ReCreateMainToolbar" );

    wxWindowUpdateLocker dummy( this );

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

    // Set up toolbar
    m_mainToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                            KiScaledBitmap( BITMAPS::import3d, this ), _( "Reload board" ) );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                            KiScaledBitmap( BITMAPS::copy, this ),
                            _( "Copy 3D image to clipboard" ) );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddTool( ID_RENDER_CURRENT_VIEW, wxEmptyString,
                            KiScaledBitmap( BITMAPS::render_mode, this ),
                            _( "Render current view using Raytracing" ), wxITEM_CHECK );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateXCW );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateXCCW );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateYCW );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateYCCW );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateZCW );
    m_mainToolBar->Add( EDA_3D_ACTIONS::rotateZCCW );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::flipView );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::moveLeft );
    m_mainToolBar->Add( EDA_3D_ACTIONS::moveRight );
    m_mainToolBar->Add( EDA_3D_ACTIONS::moveUp );
    m_mainToolBar->Add( EDA_3D_ACTIONS::moveDown );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::toggleOrtho, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->KiRealize();
}

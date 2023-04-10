/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <wx/choice.h>

#include <bitmaps.h>
#include <dialogs/eda_view_switcher.h>
#include <eda_3d_viewer_frame.h>
#include <menus_helpers.h>
#include <tool/action_toolbar.h>
#include <tools/eda_3d_actions.h>
#include <3d_viewer_id.h>

void EDA_3D_VIEWER_FRAME::ReCreateMainToolbar()
{
    wxLogTrace( m_logTrace, wxT( "EDA_3D_VIEWER_FRAME::ReCreateMainToolbar" ) );

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

    // Show the hotkey to open the windows list selector:
    wxString keyName = KeyNameFromKeyCode( VIEWPORT_SWITCH_KEY );

    m_viewportsLabel = new wxStaticText( m_mainToolBar, wxID_ANY,
                                         wxString::Format( _( "Viewports (%s+Tab):" ), keyName ) );
   	m_viewportsLabel->Wrap( -1 );

   	m_cbViewports = new wxChoice( m_mainToolBar, wxID_ANY );

    for( std::pair<const wxString, VIEWPORT3D>& pair : m_viewports )
        m_cbViewports->Append( pair.first, static_cast<void*>( &pair.second ) );

    m_cbViewports->Append( wxT( "---" ) );
    m_cbViewports->Append( _( "Save viewport..." ) );
    m_cbViewports->Append( _( "Delete viewport..." ) );

    m_cbViewports->SetToolTip( wxString::Format( _( "Save and restore view orientation and zoom.\n"
                                                    "Use %s+Tab to activate selector.\n"
                                                    "Successive Tabs while holding %s down will "
                                                    "cycle through viewports in the popup." ),
                                                 keyName, keyName ) );

    m_cbViewports->SetSelection( m_cbViewports->GetCount() - 3 );

    m_lastSelectedViewport = nullptr;

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

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EDA_3D_ACTIONS::showTHT, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EDA_3D_ACTIONS::showSMD, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EDA_3D_ACTIONS::showVirtual, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EDA_3D_ACTIONS::showNotInPosFile, ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EDA_3D_ACTIONS::showDNP, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddControl( m_viewportsLabel );
    m_mainToolBar->AddControl( m_cbViewports );

    m_mainToolBar->KiRealize();
}



/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <3d_viewer.h>
#include <menus_helpers.h>


void EDA_3D_FRAME::ReCreateHToolbar()
{
    if( m_HToolBar != NULL )
    {
        // Simple update to the list of old files.
        SetToolbars();
        return;
    }

    m_HToolBar = new wxAuiToolBar( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                   wxAUI_TB_DEFAULT_STYLE | wxAUI_TB_HORZ_LAYOUT );

    // Set up toolbar
    m_HToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                         KiBitmap( import3d_xpm ),
                         _( "Reload board" ) );

#if (defined(__WINDOWS__) || defined(__APPLE__ ) )

    // Does not work properly under linux
    m_HToolBar->AddSeparator();

    m_HToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                         KiBitmap( copy_button_xpm ),
                         _( "Copy 3D Image to Clipboard" ) );
#endif

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, KiBitmap( zoom_in_xpm ),
                         _( "Zoom in" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, KiBitmap( zoom_out_xpm ),
                         _( "Zoom out" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         KiBitmap( zoom_redraw_xpm ),
                         _( "Redraw view" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, KiBitmap( zoom_fit_in_page_xpm ),
                         _( "Fit in page" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_X_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_x_xpm ),
                         _( "Rotate X <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_X_POS, wxEmptyString,
                         KiBitmap( rotate_pos_x_xpm ),
                         _( "Rotate X ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_Y_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_y_xpm ),
                         _( "Rotate Y <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_Y_POS, wxEmptyString,
                         KiBitmap( rotate_pos_y_xpm ),
                         _( "Rotate Y ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_Z_NEG, wxEmptyString,
                         KiBitmap( rotate_neg_z_xpm ),
                         _( "Rotate Z <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_Z_POS, wxEmptyString,
                         KiBitmap( rotate_pos_z_xpm ),
                         _( "Rotate Z ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MOVE3D_LEFT, wxEmptyString, KiBitmap( left_xpm ),
                         _( "Move left <-" ) );

    m_HToolBar->AddTool( ID_MOVE3D_RIGHT, wxEmptyString, KiBitmap( right_xpm ),
                         _( "Move right ->" ) );

    m_HToolBar->AddTool( ID_MOVE3D_UP, wxEmptyString, KiBitmap( up_xpm ),
                         _( "Move up ^" ) );

    m_HToolBar->AddTool( ID_MOVE3D_DOWN, wxEmptyString, KiBitmap( down_xpm ),
                         _( "Move down v" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ORTHO, wxEmptyString, KiBitmap( ortho_xpm ),
                         _( "Enable/Disable orthographic projection" ),
                         wxITEM_CHECK );

    m_HToolBar->Realize();
}


void EDA_3D_FRAME::ReCreateVToolbar()
{
}


void EDA_3D_FRAME::ReCreateMenuBar()
{
    bool full_options = true;

    // If called from the display frame of CvPcb, only some options are relevant
    if( Parent()->GetName() == wxT( "CmpFrame" ) ) {
        full_options = false;
    }

    wxMenuBar* menuBar   = new wxMenuBar;
    wxMenu*    fileMenu  = new wxMenu;
    wxMenu*    prefsMenu = new wxMenu;

    menuBar->Append( fileMenu, _( "&File" ) );

    fileMenu->Append( ID_MENU_SCREENCOPY_PNG, _( "Create Image (png format)" ) );
    fileMenu->Append( ID_MENU_SCREENCOPY_JPEG, _( "Create Image (jpeg format)" ) );

#if (defined(__WINDOWS__) || defined(__APPLE__ ) )
    // Does not work properly under linux
    fileMenu->AppendSeparator();
    fileMenu->Append( ID_TOOL_SCREENCOPY_TOCLIBBOARD, _( "Copy 3D Image to Clipboard" ) );
#endif
    fileMenu->AppendSeparator();
    fileMenu->Append( wxID_EXIT, _( "&Exit" ) );

    menuBar->Append( prefsMenu, _( "&Preferences" ) );

    AddMenuItem( prefsMenu, ID_MENU3D_BGCOLOR_SELECTION,
                 _( "Choose background color" ), KiBitmap( palette_xpm ) );

    wxMenuItem* item;
    item = AddMenuItem( prefsMenu, ID_MENU3D_AXIS_ONOFF,
            _( "Show 3D &Axis" ), KiBitmap( axis3d_front_xpm ), wxITEM_CHECK );
    item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_AXIS]);


    if( full_options )
    {
       item = AddMenuItem( prefsMenu, ID_MENU3D_MODULE_ONOFF,
               _( "Show 3D F&ootprints" ), KiBitmap( shape_3d_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_MODULE]);

       item = AddMenuItem( prefsMenu, ID_MENU3D_ZONE_ONOFF,
               _( "Show Zone &Filling" ), KiBitmap( add_zone_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ZONE]);

       item = AddMenuItem( prefsMenu, ID_MENU3D_COMMENTS_ONOFF,
               _( "Show &Comments Layer" ), KiBitmap( edit_sheet_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_COMMENTS]);

       item = AddMenuItem( prefsMenu, ID_MENU3D_DRAWINGS_ONOFF,
               _( "Show &Drawings Layer" ), KiBitmap( add_polygon_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_DRAWINGS]);

       item = AddMenuItem( prefsMenu, ID_MENU3D_ECO1_ONOFF,
               _( "Show Eco&1 Layer" ), KiBitmap( tools_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ECO1]);

       item = AddMenuItem( prefsMenu, ID_MENU3D_ECO2_ONOFF,
               _( "Show Eco&2 Layer" ), KiBitmap( tools_xpm ), wxITEM_CHECK );
       item->Check(g_Parm_3D_Visu.m_DrawFlags[g_Parm_3D_Visu.FL_ECO2]);

    }

    SetMenuBar( menuBar );
}


void EDA_3D_FRAME::SetToolbars()
{
}

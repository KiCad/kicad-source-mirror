/*******************************************************************/
/*	3d_toolbar.cpp: construction des tool bars de la frame visu 3d */
/*******************************************************************/

#include "fctsys.h"
#include "macros.h"

#include "bitmaps.h"

#include "id.h"

#include "3d_viewer.h"


/*********************************************/
void WinEDA3D_DrawFrame::ReCreateHToolbar()
/*********************************************/
{
    if( m_HToolBar != NULL )
    {           // simple mise a jour de la liste des fichiers anciens
        SetToolbars();
        return;
    }


    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );

    SetToolBar( m_HToolBar );

    // Set up toolbar
    m_HToolBar->AddTool( ID_RELOAD3D_BOARD, wxEmptyString,
                         wxBitmap( import3d_xpm ),
                         _( "Reload board" ) );
#ifdef __WINDOWS__  // do not work properly under linux
    m_HToolBar-> AddSeparator();

    m_HToolBar->AddTool( ID_TOOL_SCREENCOPY_TOCLIBBOARD, wxEmptyString,
                         wxBitmap( copy_button ),
                        _( "Copy 3D Image to Clipboard" ) );
#endif

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ZOOM_IN, wxEmptyString, wxBitmap( zoom_in_xpm ),
                         _( "Zoom in" ) );

    m_HToolBar->AddTool( ID_ZOOM_OUT, wxEmptyString, wxBitmap( zoom_out_xpm ),
                         _( "Zoom out" ) );

    m_HToolBar->AddTool( ID_ZOOM_REDRAW, wxEmptyString,
                         wxBitmap( zoom_redraw_xpm ),
                         _( "Redraw view" ) );

    m_HToolBar->AddTool( ID_ZOOM_PAGE, wxEmptyString, wxBitmap( zoom_auto_xpm ),
                         _( "Zoom auto" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_X_NEG, wxEmptyString,
                         wxBitmap( rotate_neg_X_xpm ),
                         _( "Rotate X <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_X_POS, wxEmptyString,
                         wxBitmap( rotate_pos_X_xpm ),
                         _( "Rotate X ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_Y_NEG, wxEmptyString,
                         wxBitmap( rotate_neg_Y_xpm ),
                         _( "Rotate Y <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_Y_POS, wxEmptyString,
                         wxBitmap( rotate_pos_Y_xpm ),
                         _( "Rotate Y ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_ROTATE3D_Z_NEG, wxEmptyString,
                         wxBitmap( rotate_neg_Z_xpm ),
                         _( "Rotate Z <-" ) );

    m_HToolBar->AddTool( ID_ROTATE3D_Z_POS, wxEmptyString,
                         wxBitmap( rotate_pos_Z_xpm ),
                         _( "Rotate Z ->" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_MOVE3D_LEFT, wxEmptyString, wxBitmap( left_xpm ),
                         _( "Move left <-" ) );

    m_HToolBar->AddTool( ID_MOVE3D_RIGHT, wxEmptyString, wxBitmap( right_xpm ),
                         _( "Move right ->" ) );

    m_HToolBar->AddTool( ID_MOVE3D_UP, wxEmptyString, wxBitmap( up_xpm ),
                         _( "Move Up ^" ) );

    m_HToolBar->AddTool( ID_MOVE3D_DOWN, wxEmptyString, wxBitmap( down_xpm ),
                         _( "Move Down" ) );

    m_HToolBar->Realize();

//	SetToolbars();
}


/*********************************************/
void WinEDA3D_DrawFrame::ReCreateVToolbar()
/*********************************************/
{
}


/**********************************************/
void WinEDA3D_DrawFrame::ReCreateMenuBar()
/**********************************************/
{
bool full_options = true;

    // If called from the display frame of cvpcb, only some options are relevant
    if ( m_Parent->m_FrameName == wxT( "CmpFrame" ) )   // Called from cvpcb !
        full_options =  false;  // Do not display all options

    wxMenuBar* menuBar = new wxMenuBar;

    wxMenu*    fileMenu = new wxMenu;

    menuBar->Append( fileMenu, _( "&File" ) );

    fileMenu->Append( ID_MENU_SCREENCOPY_PNG, _( "Create Image (png format)" ) );
    fileMenu->Append( ID_MENU_SCREENCOPY_JPEG, _( "Create Image (jpeg format)" ) );
    fileMenu->AppendSeparator();
    fileMenu->Append( wxID_EXIT, _( "&Exit" ) );

    wxMenu* referencesMenu = new wxMenu;
    menuBar->Append( referencesMenu, _( "&Preferences" ) );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_BGCOLOR_SELECTION,
                  _( "Choose background color" ), palette_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_AXIS_ONOFF,
                  _( "3D Axis On/Off" ), axis3d_front_xpm );

    if ( full_options )
    {
    ADD_MENUITEM( referencesMenu, ID_MENU3D_MODULE_ONOFF,
                  _( "3D Footprints Shapes On/Off" ), shape_3d_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_ZONE_ONOFF,
                  _( "Zone Filling On/Off" ), add_zone_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_COMMENTS_ONOFF,
                  _( "Comments Layer On/Off" ), edit_sheet_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_DRAWINGS_ONOFF,
                  _( "Drawings Layer On/Off" ), add_polygon_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_ECO1_ONOFF,
                  _( "Eco1 Layer On/Off" ), tools_xpm );

    ADD_MENUITEM( referencesMenu, ID_MENU3D_ECO2_ONOFF,
                  _( "Eco2 Layer On/Off" ), tools_xpm );
              }

    SetMenuBar( menuBar );
}


/*****************************************/
void WinEDA3D_DrawFrame::SetToolbars()
/*****************************************/
{
}

/***************************************************/
/*	tool_cvpcb.cpp: construction du menu principal */
/***************************************************/

#include "fctsys.h"

#include "common.h"
#include "cvpcb.h"
#include "trigo.h"

#include "protos.h"

#include "bitmaps.h"

#include "id.h"


/*********************************************/
void WinEDA_CvpcbFrame::ReCreateHToolbar()
/*********************************************/
{
    wxConfig* config = wxGetApp().m_EDA_Config;

    if( m_HToolBar != NULL )
        return;

    m_HToolBar = new WinEDA_Toolbar( TOOLBAR_MAIN, this, ID_H_TOOLBAR, TRUE );
    SetToolBar( m_HToolBar );

    m_HToolBar->AddTool( ID_CVPCB_READ_INPUT_NETLIST, wxBitmap( open_xpm ),
                         _( "Open a NetList file" ) );

    m_HToolBar->AddTool( ID_CVPCB_SAVEQUITCVPCB, wxBitmap( save_xpm ),
                         _( "Save NetList and Footprints List files" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_CONFIGWINDOW, wxBitmap( config_xpm ),
                         _( "Configuration" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_SCREENCMP, wxBitmap( module_xpm ),
                         _( "View selected footprint" ) );

    m_HToolBar->AddTool( ID_CVPCB_AUTO_ASSOCIE, wxBitmap( auto_associe_xpm ),
                         _( "Automatic Association" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_GOTO_PREVIOUSNA, wxBitmap( left_xpm ),
                         _( "Select previous free component" ) );

    m_HToolBar->AddTool( ID_CVPCB_GOTO_FIRSTNA, wxBitmap( right_xpm ),
                         _( "Select next free component" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_DEL_ASSOCIATIONS,
                         wxBitmap( delete_association_xpm ),
                         _( "Delete all associations" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_CVPCB_CREATE_STUFF_FILE,
                         wxBitmap( save_cmpstuff_xpm ),
                         _( "Create stuff file (component/footprint list)" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddTool( ID_PCB_DISPLAY_FOOTPRINT_DOC,
                         wxBitmap( file_footprint_xpm ),
                         _( "Display footprints list documentation" ) );

    m_HToolBar->AddSeparator();
    m_HToolBar->AddSeparator();
    m_HToolBar->AddRadioTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
                              wxEmptyString,
                              wxBitmap( module_filtered_list_xpm ),
                              wxNullBitmap,
                              _( "Display the filtered footprint list for the current component" ) );
    m_HToolBar->AddRadioTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
                              wxEmptyString, wxBitmap( module_full_list_xpm ),
                              wxNullBitmap,
                              _( "Display the full footprint list (without filtering)" ) );

    if( config )
    {
        wxString key = wxT( FILTERFOOTPRINTKEY );
        int      opt = config->Read( key, (long) 1 );
        m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST, opt );
        m_HToolBar->ToggleTool( ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST, !opt );
    }

    // after adding the buttons to the toolbar, must call Realize() to reflect
    // the changes
    m_HToolBar->Realize();
}


/*******************************************/
void WinEDA_CvpcbFrame::ReCreateMenuBar()
/*******************************************/

/* Creation des menus de la fenetre principale
 */
{
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();
    /* Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly. */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    wxMenu* filesMenu = new wxMenu;
    item = new wxMenuItem( filesMenu, ID_LOAD_PROJECT,
                           _( "&Open" ),
                           _( "Open a NetList file" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT,
                           _( "&Save As..." ),
                           _( "Save New NetList and Footprints List files" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_CVPCB_QUIT, _( "E&xit" ),
                           _( "Quit Cvpcb" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

    // Creation des selections des anciens fichiers
    wxGetApp().m_fileHistory.AddFilesToMenu( filesMenu );

    // Menu Configuration:
    wxMenu* configmenu = new wxMenu;
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Configuration" ),
                           _( "Setting Libraries, Directories and others..." ) );
    item->SetBitmap( config_xpm );
    configmenu->Append( item );

    // Font selection and setup
    AddFontSelectionMenu( configmenu );

    wxGetApp().AddMenuLanguageList( configmenu );

    configmenu->AppendSeparator();
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE,
                           _( "&Save config" ),
                           _( "Save configuration in current dir" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;
    item = new wxMenuItem( helpMenu, ID_CVPCB_DISPLAY_HELP, _( "&Contents" ),
                           _( "Open the cvpcb manual" ) );
    item->SetBitmap( help_xpm );
    helpMenu->Append( item );
    item = new wxMenuItem( helpMenu, ID_CVPCB_DISPLAY_LICENCE,
                           _( "&About cvpcb" ),
                           _( "About cvpcb schematic to pcb converter" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    // Associate the menu bar with the frame
    SetMenuBar( menuBar );
}

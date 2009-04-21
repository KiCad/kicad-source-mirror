/***************************************/
/* menucfg : buils the cvpcb main menu */
/***************************************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "cvstruct.h"

#include "bitmaps.h"

#include "id.h"


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

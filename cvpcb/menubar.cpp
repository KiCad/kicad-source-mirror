/*
 * menubar.cpp
 * Build the CvPCB MenuBar
 */
#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"

#include "cvpcb.h"
#include "cvstruct.h"
#include "cvpcb_id.h"

#include "bitmaps.h"


void WinEDA_CvpcbFrame::ReCreateMenuBar()
{
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove(0);

    // Recreate all menus:

    wxMenu* filesMenu = new wxMenu;
    item = new wxMenuItem( filesMenu, ID_LOAD_PROJECT,
                           _( "&Open" ),
                           _( "Open a net list file" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    /* Open Recent submenu */
    wxMenu* openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentMenu, -1, _( "Open &Recent" ),
                                        _("Open a recent opened netlist document" ),
                                        open_project_xpm );



    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT,
                           _( "&Save As..." ),
                           _( "Save new net list and footprint list files" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    /* Quit on all platforms except WXMAC */
#if !defined(__WXMAC__)

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ), _( "Quit CvPCB" ) );
    filesMenu->Append( item );

#endif /* !defined( __WXMAC__) */

    // Menu Configuration:
    wxMenu* configmenu = new wxMenu;
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Configuration" ),
                           _( "Set libraries and library search paths" ) );
    item->SetBitmap( config_xpm );
    configmenu->Append( item );

    wxGetApp().AddMenuLanguageList( configmenu );

    item = new wxMenuItem( configmenu, ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                           _( "Keep Open On Save" ),
                           _( "Prevent CVPcb from exiting after saving netlist file" ),
                           wxITEM_CHECK );
    configmenu->Append( item );
    configmenu->AppendSeparator();
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE,
                           _( "&Save Project File" ),
                           _( "Save changes to the project file" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the cvpcb manual" ) );
    item->SetBitmap( online_help_xpm );
    helpMenu->Append( item );

    /* About on all platforms except WXMAC */
#if !defined(__WXMAC__)

    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT,
                           _( "&About" ),
                           _( "About cvpcb schematic to pcb converter" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

#endif /* !defined(__WXMAC__) */

    // Create the menubar and append all submenus
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}

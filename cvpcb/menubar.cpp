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
#include "cvpcb_mainframe.h"
#include "cvpcb_id.h"

#include "bitmaps.h"


void CVPCB_MAINFRAME::ReCreateMenuBar()
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
    ADD_MENUITEM_WITH_HELP( filesMenu, ID_LOAD_PROJECT,
                  _( "&Open" ), _( "Open a net list file" ),
                    open_document_xpm );

    /* Open Recent submenu */
    wxMenu* openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentMenu, -1, _( "Open &Recent" ),
                                        _("Open a recent opened netlist document" ),
                                        open_project_xpm );



    filesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( filesMenu, ID_SAVE_PROJECT,
                           _( "&Save As..." ),
                           _( "Save new net list and footprint list files" ),
                            save_xpm );

    /* Quit on all platforms except WXMAC */
#if !defined(__WXMAC__)

    filesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( filesMenu, wxID_EXIT,
                _( "&Quit" ), _( "Quit CvPCB" ),
                exit_xpm );

#endif /* !defined( __WXMAC__) */

    // Menu Configuration:
    wxMenu* configmenu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( configmenu, ID_CONFIG_REQ, _( "&Configuration" ),
                  _( "Set libraries and library search paths" ),
                    config_xpm );

    wxGetApp().AddMenuLanguageList( configmenu );

    item = new wxMenuItem( configmenu, ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                           _( "Keep Open On Save" ),
                           _( "Prevent CVPcb from exiting after saving netlist file" ),
                           wxITEM_CHECK );
    configmenu->Append( item );

    configmenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( configmenu, ID_CONFIG_SAVE,
                    _( "&Save Project File" ),
                    _( "Save changes to the project file" ),
                    save_setup_xpm );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    ADD_MENUITEM_WITH_HELP( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the cvpcb manual" ),
                            online_help_xpm );

    /* About on all platforms except WXMAC */
#if !defined(__WXMAC__)

    ADD_MENUITEM_WITH_HELP( helpMenu, ID_KICAD_ABOUT,
                           _( "&About" ),
                           _( "About cvpcb schematic to pcb converter" ),
                            info_xpm );

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

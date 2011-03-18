/*************************************/
/*  menubar.cpp: Build the main menu */
/*************************************/

#include "fctsys.h"

#include "appl_wxstruct.h"
#include "common.h"

//#include "macros.h"
#include "gerbview.h"
#include "bitmaps.h"
#include "gerbview_id.h"
#include "hotkeys.h"


void GERBVIEW_FRAME::ReCreateMenuBar( void )
{
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:
    wxMenu* filesMenu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( filesMenu, wxID_FILE, _( "Load &Gerber File" ),
                            _(
                                "Load a new Gerber file on the current layer. Previous data will be deleted" ),
                            open_xpm );

    ADD_MENUITEM_WITH_HELP( filesMenu, ID_GERBVIEW_LOAD_DRILL_FILE,
                            _( "Load &EXCELLON Drill File" ),
                            _( "Load excellon drill file" ),
                            open_xpm );

    ADD_MENUITEM_WITH_HELP( filesMenu, ID_GERBVIEW_LOAD_DCODE_FILE,
                            _( "Load &DCodes" ),
                            _( "Load D-Codes definition file" ),
                            open_xpm );

    // Open Recent submenus
    wxMenu* openRecentGbrMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentGbrMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentGbrMenu,
                                        wxID_ANY, _( "Open &Recent Gerber File" ),
                                        _( "Open a recent opened Gerber file" ),
                                        open_project_xpm );

    wxMenu* openRecentDrlMenu = new wxMenu();
    m_drillFileHistory.AddFilesToMenu( openRecentDrlMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentDrlMenu,
                                        wxID_ANY, _( "Open Recent &Drill File" ),
                                        _( "Open a recent opened drill file" ),
                                        open_project_xpm );


    filesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( filesMenu, ID_GERBVIEW_ERASE_ALL,
                            _( "&Clear All" ),
                            _( "Clear all layers. All data will be deleted" ),
                            delete_body_xpm );

    filesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( filesMenu, ID_GERBVIEW_EXPORT_TO_PCBNEW,
                            _( "Export to &Pcbnew" ),
                            _( "Export data in pcbnew format" ),
                            export_xpm );


    filesMenu->AppendSeparator();

    ADD_MENUITEM_WITH_HELP( filesMenu, wxID_PRINT,
                            _( "P&rint" ),
                            _( "Print gerber" ),
                            print_button );

    filesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( filesMenu, ID_EXIT,
                            _( "E&xit" ),
                            _( "Quit Gerbview" ),
                            exit_xpm );

    // Configuration and preferences:
    wxMenu* configmenu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( configmenu, ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                            _( "Hide &Layers Manager" ),
                            _( "Show/hide the layers manager toolbar" ),
                            layers_manager_xpm );

    ADD_MENUITEM_WITH_HELP( configmenu, ID_GERBVIEW_OPTIONS_SETUP,
                            _( "&Options" ),
                            _( "Set options to draw items" ),
                            preference_xpm );

    wxGetApp().AddMenuLanguageList( configmenu );

    AddHotkeyConfigMenu( configmenu );


    wxMenu* miscellaneous_menu = new wxMenu;
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_GERBVIEW_SHOW_LIST_DCODES,
                            _( "&List DCodes" ),
                            _( "List and edit D-codes" ), show_dcodenumber_xpm );
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_GERBVIEW_SHOW_SOURCE,
                            _( "&Show Source" ),
                            _( "Show source file for the current layer" ),
                            tools_xpm );

    miscellaneous_menu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_GERBVIEW_GLOBAL_DELETE,
                            _( "&Clear Layer" ),
                            _( "Clear current layer" ), general_deletions_xpm );

    miscellaneous_menu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( miscellaneous_menu, ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,
                            _( "&Text Editor" ),
                            _( "Select your preferred text editor" ),
                            editor_xpm );


    // Menu Help:
    wxMenu* helpMenu = new wxMenu;
    AddHelpVersionInfoMenuEntry( helpMenu );
    ADD_MENUITEM_WITH_HELP( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                            _( "Open the gerbview manual" ), help_xpm );
    ADD_MENUITEM_WITH_HELP( helpMenu, ID_KICAD_ABOUT, _( "&About Gerbview" ),
                            _( "About gerbview gerber and drill viewer" ),
                            online_help_xpm );

    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( miscellaneous_menu, _( "&Miscellaneous" ) );

    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}

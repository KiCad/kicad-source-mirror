/**
 * @file cvpcb/menubar.cpp
 * @brief (Re)Create the menubar for cvpcb
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

/**
 * @brief (Re)Create the menubar for the cvpcb mainframe
 */
void CVPCB_MAINFRAME::ReCreateMenuBar()
{
    // Create and try to get the current  menubar
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )     // Delete all menus
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove(0);

    // Recreate all menus:

    // Menu File:
    wxMenu* filesMenu = new wxMenu;

    // Open
    ADD_MENUITEM_WITH_HELP( filesMenu,
                            ID_LOAD_PROJECT,
                            _( "&Open" ),
                            _( "Open a net list file" ),
                            open_document_xpm );

    // Open Recent submenu
    static wxMenu* openRecentMenu;
    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentMenu );
    openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu( );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentMenu, -1,
                                        _( "Open &Recent" ),
                                        _("Open a recent opened netlist document" ),
                                        open_project_xpm );

    // Separator
    filesMenu->AppendSeparator();

    // Save as
    ADD_MENUITEM_WITH_HELP( filesMenu,
                            ID_SAVE_PROJECT,
                            _( "&Save As..." ),
                            _( "Save new net list and footprint list files" ),
                            save_xpm );

    // Separator
    filesMenu->AppendSeparator();

    // Quit
    ADD_MENUITEM_WITH_HELP( filesMenu,
                            wxID_EXIT,
                            _( "&Quit" ),
                            _( "Quit CvPCB" ),
                            exit_xpm );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Options (Preferences on WXMAC)
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            wxID_PREFERENCES,
#ifdef __WXMAC__
                            _( "&Preferences..." ),
#else
                            _( "&Options" ),
#endif // __WXMAC__
                            _( "Set libraries and library search paths" ),
                            config_xpm );

    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Keep open on save
    item = new wxMenuItem( preferencesMenu, ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
                           _( "Keep Open On Save" ),
                           _( "Prevent CVPcb from exiting after saving netlist file" ),
                           wxITEM_CHECK );
    preferencesMenu->Append( item );
    SETBITMAPS( window_close_xpm );

    // Separator
    preferencesMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( preferencesMenu, ID_CONFIG_SAVE,
                    _( "&Save Project File" ),
                    _( "Save changes to the project file" ),
                    save_setup_xpm );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    ADD_MENUITEM_WITH_HELP( helpMenu, wxID_HELP, _( "&Contents" ),
                           _( "Open the Cvpcb handbook" ),
                            online_help_xpm );

    // About
    ADD_MENUITEM_WITH_HELP( helpMenu, wxID_ABOUT,
                           _( "&About CvPCB" ),
                           _( "About CvPCB schematic to pcb converter" ),
                            info_xpm );

    // Create the menubar and append all submenus
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}

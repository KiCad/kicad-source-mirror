/**
 * @file gerbview/menubar.cpp
 * @brief (Re)Create the main menubar for GerbView
 */
#include "fctsys.h"

#include "appl_wxstruct.h"
#include "common.h"

#include "gerbview.h"
#include "bitmaps.h"
#include "gerbview_id.h"
#include "hotkeys.h"

/**
 * @brief (Re)Create the menubar for the gerbview frame
 */
void GERBVIEW_FRAME::ReCreateMenuBar( void )
{
    // Create and try to get the current menubar
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // Load
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_FILE,
                            _( "Load &Gerber File" ),
    _( "Load a new Gerber file on the current layer. Previous data will be deleted" ),
                            gerber_file_xpm );

    // Excellon
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_GERBVIEW_LOAD_DRILL_FILE,
                            _( "Load &EXCELLON Drill File" ),
                            _( "Load excellon drill file" ),
                            gerbview_drill_file_xpm );

    // Dcodes
    ADD_MENUITEM_WITH_HELP( fileMenu, ID_GERBVIEW_LOAD_DCODE_FILE,
                            _( "Load &DCodes" ),
                            _( "Load D-Codes definition file" ),
                            gerber_open_dcode_file_xpm );

    // Recent gerber files
    static wxMenu* openRecentGbrMenu;
    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentGbrMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentGbrMenu );
    openRecentGbrMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentGbrMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu();
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, openRecentGbrMenu,
                                        wxID_ANY,
                                        _( "Open &Recent Gerber File" ),
                                        _( "Open a recent opened Gerber file" ),
                                        gerber_recent_files_xpm );

    // Recent drill files
    static wxMenu* openRecentDrlMenu;
    if( openRecentDrlMenu )
    wxGetApp().m_fileHistory.RemoveMenu( openRecentDrlMenu );
        openRecentDrlMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentDrlMenu );
    m_drillFileHistory.AddFilesToMenu( );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, openRecentDrlMenu,
                                        wxID_ANY,
                                        _( "Open Recent &Drill File" ),
                                        _( "Open a recent opened drill file" ),
                                        open_project_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Clear all
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_GERBVIEW_ERASE_ALL,
                            _( "&Clear All" ),
                            _( "Clear all layers. All data will be deleted" ),
                            gerbview_clear_layers_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Export to pcbnew
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_GERBVIEW_EXPORT_TO_PCBNEW,
                            _( "Export to &Pcbnew" ),
                            _( "Export data in pcbnew format" ),
                            export_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Print
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_PRINT,
                            _( "P&rint" ),
                            _( "Print gerber" ),
                            print_button );

    // Separator
    fileMenu->AppendSeparator();

    // Exit
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_EXIT,
                            _( "E&xit" ),
                            _( "Quit Gerbview" ),
                            exit_xpm );

    // Menu for configuration and preferences
    wxMenu* configMenu = new wxMenu;

    // Hide layer manager
    ADD_MENUITEM_WITH_HELP( configMenu,
                            ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                            _( "Hide &Layers Manager" ),
                            _( "Show/hide the layers manager toolbar" ),
                            layers_manager_xpm );

    // Options (Preferences on WXMAC)
    ADD_MENUITEM_WITH_HELP( configMenu,
                            wxID_PREFERENCES,
#ifdef __WXMAC__
                            _( "Preferences..." ),
#else
                            _( "&Options" ),
#endif // __WXMAC__
                            _( "Set options to draw items" ),
                            preference_xpm );

    // Language submenu
    wxGetApp().AddMenuLanguageList( configMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configMenu );

    // Menu miscellaneous
    wxMenu* miscellaneousMenu = new wxMenu;

    // List dcodes
    ADD_MENUITEM_WITH_HELP( miscellaneousMenu,
                            ID_GERBVIEW_SHOW_LIST_DCODES,
                            _( "&List DCodes" ),
                            _( "List and edit D-codes" ),
                            show_dcodenumber_xpm );

    // Show source
    ADD_MENUITEM_WITH_HELP( miscellaneousMenu,
                            ID_GERBVIEW_SHOW_SOURCE,
                            _( "&Show Source" ),
                            _( "Show source file for the current layer" ),
                            tools_xpm );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Clear layer
    ADD_MENUITEM_WITH_HELP( miscellaneousMenu,
                            ID_GERBVIEW_GLOBAL_DELETE,
                            _( "&Clear Layer" ),
                            _( "Clear current layer" ),
                            general_deletions_xpm );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Text editor
    ADD_MENUITEM_WITH_HELP( miscellaneousMenu,
                            ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,
                            _( "&Text Editor" ),
                            _( "Select your preferred text editor" ),
                            editor_xpm );

    // Menu Help
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_HELP,
                            _( "&Contents" ),
                            _( "Open the Gerbview handbook" ),
                            help_xpm );

    // About gerbview
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_ABOUT,
                            _( "&About GerbView" ),
                            _( "About gerbview gerber and drill viewer" ),
                            online_help_xpm );

    // Append menus to the menubar
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( configMenu, _( "&Preferences" ) );
    menuBar->Append( miscellaneousMenu, _( "&Miscellaneous" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}

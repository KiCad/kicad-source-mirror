/**
 * @file gerbview/menubar.cpp
 * @brief (Re)Create the main menubar for GerbView
 */
#include "fctsys.h"

#include "appl_wxstruct.h"

#include "gerbview.h"
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
    AddMenuItem( fileMenu,
                 wxID_FILE,
                 _( "Load &Gerber File" ),
                 _( "Load a new Gerber file on the current layer. Previous data will be deleted" ),
                 KiBitmap( gerber_file_xpm ) );

    // Excellon
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_LOAD_DRILL_FILE,
                 _( "Load &EXCELLON Drill File" ),
                 _( "Load excellon drill file" ),
                 KiBitmap( gerbview_drill_file_xpm ) );

    // Dcodes
    AddMenuItem( fileMenu, ID_GERBVIEW_LOAD_DCODE_FILE,
                 _( "Load &DCodes" ),
                 _( "Load D-Codes definition file" ),
                 KiBitmap( gerber_open_dcode_file_xpm ) );

    // Recent gerber files
    static wxMenu* openRecentGbrMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentGbrMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentGbrMenu );

    openRecentGbrMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentGbrMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu();
    AddMenuItem( fileMenu, openRecentGbrMenu,
                 wxID_ANY,
                 _( "Open &Recent Gerber File" ),
                 _( "Open a recent opened Gerber file" ),
                 KiBitmap( gerber_recent_files_xpm ) );

    // Recent drill files
    static wxMenu* openRecentDrlMenu;

    if( openRecentDrlMenu )
        m_drillFileHistory.RemoveMenu( openRecentDrlMenu );

    openRecentDrlMenu = new wxMenu();
    m_drillFileHistory.UseMenu( openRecentDrlMenu );
    m_drillFileHistory.AddFilesToMenu( );
    AddMenuItem( fileMenu, openRecentDrlMenu,
                 wxID_ANY,
                 _( "Open Recent &Drill File" ),
                 _( "Open a recent opened drill file" ),
                 KiBitmap( open_project_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Clear all
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_ERASE_ALL,
                 _( "&Clear All" ),
                 _( "Clear all layers. All data will be deleted" ),
                 KiBitmap( gerbview_clear_layers_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Export to pcbnew
    AddMenuItem( fileMenu,
                 ID_GERBVIEW_EXPORT_TO_PCBNEW,
                 _( "Export to &Pcbnew" ),
                 _( "Export data in pcbnew format" ),
                 KiBitmap( export_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Print
    AddMenuItem( fileMenu,
                 wxID_PRINT,
                 _( "P&rint" ),
                 _( "Print gerber" ),
                 KiBitmap( print_button_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Exit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "E&xit" ),
                 _( "Quit Gerbview" ),
                 KiBitmap( exit_xpm ) );

    // Menu for configuration and preferences
    wxMenu* configMenu = new wxMenu;

    // Hide layer manager
    AddMenuItem( configMenu,
                 ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,
                 _( "Hide &Layers Manager" ),
                 _( "Show/hide the layers manager toolbar" ),
                 KiBitmap( layers_manager_xpm ) );

    // Options (Preferences on WXMAC)

#ifdef __WXMAC__
    configMenu->Append(wxID_PREFERENCES);
#else
    AddMenuItem( configMenu,
                 wxID_PREFERENCES,
                 _( "&Options" ),
                 _( "Set options to draw items" ),
                 KiBitmap( preference_xpm ) );
#endif // __WXMAC__


    // Language submenu
    wxGetApp().AddMenuLanguageList( configMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( configMenu );

    // Menu miscellaneous
    wxMenu* miscellaneousMenu = new wxMenu;

    // List dcodes
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_SHOW_LIST_DCODES,
                 _( "&List DCodes" ),
                 _( "List and edit D-codes" ),
                 KiBitmap( show_dcodenumber_xpm ) );

    // Show source
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_SHOW_SOURCE,
                 _( "&Show Source" ),
                 _( "Show source file for the current layer" ),
                 KiBitmap( tools_xpm ) );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Clear layer
    AddMenuItem( miscellaneousMenu,
                 ID_GERBVIEW_GLOBAL_DELETE,
                 _( "&Clear Layer" ),
                 _( "Clear current layer" ),
                 KiBitmap( general_deletions_xpm ) );

    // Separator
    miscellaneousMenu->AppendSeparator();

    // Text editor
    AddMenuItem( miscellaneousMenu,
                 ID_MENU_GERBVIEW_SELECT_PREFERED_EDITOR,
                 _( "&Text Editor" ),
                 _( "Select your preferred text editor" ),
                 KiBitmap( editor_xpm ) );

    // Menu Help
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "&Contents" ),
                 _( "Open the Gerbview handbook" ),
                 KiBitmap( help_xpm ) );

    // About gerbview
    AddMenuItem( helpMenu,
                 wxID_ABOUT,
                 _( "&About GerbView" ),
                 _( "About gerbview gerber and drill viewer" ),
                 KiBitmap( online_help_xpm ) );

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

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright (C) 2004-2021 KiCad Developers, see change_log.txt for contributors.
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

#include "kicad_id.h"
#include "pgm_kicad.h"
#include "project_tree_pane.h"
#include <advanced_config.h>
#include <bitmaps.h>
#include <build_version.h>
#include <dialogs/panel_kicad_launcher.h>
#include <eda_base_frame.h>
#include <filehistory.h>
#include <kiplatform/app.h>
#include <kiway.h>
#include <kiway_express.h>
#include <launch_ext.h>
#include <panel_hotkeys_editor.h>
#include <reporter.h>
#include <project/project_local_settings.h>
#include <sch_file_versions.h>
#include <settings/settings_manager.h>
#include <tool/action_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <wildcards_and_files_ext.h>
#include <widgets/app_progress_dialog.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/dcclient.h>
#include <atomic>


#include <../pcbnew/plugins/kicad/kicad_plugin.h>   // for SEXPR_BOARD_FILE_VERSION def


#ifdef __WXMAC__
#include <MacTypes.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "kicad_manager_frame.h"
#include "kicad_settings.h"


#define SEP()   wxFileName::GetPathSeparator()


// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_IDLE( KICAD_MANAGER_FRAME::OnIdle )

    // Menu events
    EVT_MENU( wxID_EXIT, KICAD_MANAGER_FRAME::OnExit )
    EVT_MENU( ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR, KICAD_MANAGER_FRAME::OnOpenFileInTextEditor )
    EVT_MENU( ID_BROWSE_IN_FILE_EXPLORER, KICAD_MANAGER_FRAME::OnBrowseInFileExplorer )
    EVT_MENU( ID_SAVE_AND_ZIP_FILES, KICAD_MANAGER_FRAME::OnArchiveFiles )
    EVT_MENU( ID_READ_ZIP_ARCHIVE, KICAD_MANAGER_FRAME::OnUnarchiveFiles )
    EVT_MENU( ID_IMPORT_CADSTAR_ARCHIVE_PROJECT, KICAD_MANAGER_FRAME::OnImportCadstarArchiveFiles )
    EVT_MENU( ID_IMPORT_EAGLE_PROJECT, KICAD_MANAGER_FRAME::OnImportEagleFiles )

    // Range menu events
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    KICAD_MANAGER_FRAME::language_change )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, KICAD_MANAGER_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, KICAD_MANAGER_FRAME::OnClearFileHistory )

    // Special functions
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )
END_EVENT_TABLE()


#ifdef PCM
// See below the purpose of this include
#include <wx/xml/xml.h>
#endif

KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow* parent, const wxString& title,
                                          const wxPoint& pos, const wxSize&   size ) :
        EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME_T, title, pos, size,
                        KICAD_DEFAULT_DRAWFRAME_STYLE, KICAD_MANAGER_FRAME_NAME, &::Kiway ),
        m_leftWin( nullptr ),
        m_launcher( nullptr ),
        m_mainToolBar( nullptr )
{
    m_active_project = false;
    m_leftWinWidth = 250;       // Default value
    m_aboutTitle = "KiCad";

#ifdef PCM
    // JPC: A very ugly hack to fix an issue on Linux: if the wxbase315u_xml_gcc_custom.so is
    // used **only** in PCM, it is not found in some cases at run time.
    // So just use it in the main module to avoid a not found issue
    // wxbase315u_xml_gcc_custom shared object when launching Kicad
    wxXmlDocument dummy;
#endif

    // Create the status line (bottom of the frame).  Left half is for project name; right half
    // is for Reporter (currently used by archiver/unarchiver).
    CreateStatusBar( 2 );
    GetStatusBar()->SetFont( KIUI::GetStatusFont( this ) );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_32 ) );
    icon_bundle.AddIcon( icon );
    icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_16 ) );
    icon_bundle.AddIcon( icon );

    SetIcons( icon_bundle );

    // Load the settings
    LoadSettings( config() );

    // Left window: is the box which display tree project
    m_leftWin = new PROJECT_TREE_PANE( this );

    setupTools();
    setupUIConditions();

    m_launcher = new PANEL_KICAD_LAUNCHER( this );

    RecreateBaseHToolbar();
    ReCreateMenuBar();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetFlags( wxAUI_MGR_LIVE_RESIZE );

    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().HToolbar().Name( "MainToolbar" ).Left()
                      .Layer( 2 ) );

    // BestSize() does not always set the actual pane size of m_leftWin to the required value.
    // It happens when m_leftWin is too large (roughly > 1/3 of the kicad manager frame width.
    // (Well, BestSize() sets the best size... not the window size)
    // A trick is to use MinSize() to set the required pane width,
    // and after give a reasonable MinSize value
    m_auimgr.AddPane( m_leftWin, EDA_PANE().Palette().Name( "ProjectTree" ).Left().Layer( 1 )
                      .Caption( _( "Project Files" ) ).PaneBorder( false )
                      .MinSize( m_leftWinWidth, -1 ).BestSize( m_leftWinWidth, -1 ) );

    m_auimgr.AddPane( m_launcher, EDA_PANE().Canvas().Name( "Launcher" ).Center()
                      .Caption( _( "Editors" ) ).PaneBorder( false )
                      .MinSize( m_launcher->GetBestSize() ) );

    m_auimgr.Update();

    // Now the actual m_leftWin size is set, give it a reasonable min width
    m_auimgr.GetPane( m_leftWin ).MinSize( 250, -1 );

    wxSizer* mainSizer = GetSizer();

    // Only fit the initial window size the first time KiCad is run.
    if( mainSizer && config()->m_Window.state.size_x == 0 && config()->m_Window.state.size_y == 0 )
        mainSizer->Fit( this );

    if( ADVANCED_CFG::GetCfg().m_HideVersionFromTitle )
        SetTitle( wxT( "KiCad" ) );
    else
        SetTitle( wxString( "KiCad " ) + GetBuildVersion() );

    // Do not let the messages window have initial focus
    m_leftWin->SetFocus();

    // Ensure the window is on top
    Raise();
}


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;

    m_auimgr.UnInit();
}


void KICAD_MANAGER_FRAME::setupTools()
{
    // Create the manager
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( nullptr, nullptr, nullptr, config(), this );
    m_actions = new KICAD_MANAGER_ACTIONS();

    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager );

    // Attach the events to the tool dispatcher
    Bind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Bind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    // Register tools
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new KICAD_MANAGER_CONTROL );
    m_toolManager->InitTools();
}


void KICAD_MANAGER_FRAME::setupUIConditions()
{
    EDA_BASE_FRAME::setupUIConditions();

    ACTION_MANAGER* manager = m_toolManager->GetActionManager();

    wxASSERT( manager );

    auto activeProject =
        [this] ( const SELECTION& )
        {
            return m_active_project;
        };

    ACTION_CONDITIONS activeProjectCond;
    activeProjectCond.Enable( activeProject );

    manager->SetConditions( ACTIONS::saveAs,                       activeProjectCond );
    manager->SetConditions( KICAD_MANAGER_ACTIONS::closeProject,   activeProjectCond );

    // TODO: Switch this to an action
    RegisterUIUpdateHandler( ID_SAVE_AND_ZIP_FILES, activeProjectCond );
}


wxWindow* KICAD_MANAGER_FRAME::GetToolCanvas() const
{
    return m_leftWin;
}


APP_SETTINGS_BASE* KICAD_MANAGER_FRAME::config() const
{
    APP_SETTINGS_BASE* ret = PgmTop().PgmSettings();
    wxASSERT( ret );
    return ret;
}


KICAD_SETTINGS* KICAD_MANAGER_FRAME::kicadSettings() const
{
    KICAD_SETTINGS* ret = dynamic_cast<KICAD_SETTINGS*>( config() );
    wxASSERT( ret );
    return ret;
}


const wxString KICAD_MANAGER_FRAME::GetProjectFileName() const
{
    return Pgm().GetSettingsManager().IsProjectOpen() ? Prj().GetProjectFullName() :
                                                        wxString( wxEmptyString );
}


const wxString KICAD_MANAGER_FRAME::SchFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( KiCadSchematicFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::SchLegacyFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( LegacySchematicFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( PcbFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbLegacyFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( LegacyPcbFileExtension );
   return fn.GetFullPath();
}


void KICAD_MANAGER_FRAME::ReCreateTreePrj()
{
    m_leftWin->ReCreateTreePrj();
}


const SEARCH_STACK& KICAD_MANAGER_FRAME::sys_search()
{
    return PgmTop().SysSearch();
}


wxString KICAD_MANAGER_FRAME::help_name()
{
    return PgmTop().GetHelpFileName();
}


void KICAD_MANAGER_FRAME::OnSize( wxSizeEvent& event )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    PrintPrjInfo();

    event.Skip();
}


bool KICAD_MANAGER_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    KICAD_SETTINGS* settings = kicadSettings();
    settings->m_OpenProjects = GetSettingsManager()->GetOpenProjects();

    // CloseProject will recursively ask all the open editors if they need to save changes.
    // If any of them cancel then we need to cancel closing the KICAD_MANAGER_FRAME.
    if( CloseProject( true ) )
    {
        // Don't propagate event to frames which have already been closed
        aEvent.StopPropagation();

        return true;
    }
    else
    {
        if( aEvent.CanVeto() )
            aEvent.Veto();

        return false;
    }
}


void KICAD_MANAGER_FRAME::doCloseWindow()
{
#ifdef _WINDOWS_
    // For some obscure reason, on Windows, when killing Kicad from the Windows task manager
    // if a editor frame (schematic, library, board editor or fp editor) is open and has
    // some edition to save, OnCloseWindow is run twice *at the same time*, creating race
    // conditions between OnCloseWindow() code.
    // Therefore I added (JPC) a ugly hack to discard the second call (unwanted) during
    // execution of the first call (only one call is right).
    // Note also if there is no change made in editors, this behavior does not happen.
    static std::atomic<unsigned int> lock_close_event( 0 );

    if( ++lock_close_event > 1 )    // Skip extra calls
    {
        return;
    }
#endif

    m_leftWin->Show( false );

    Destroy();

#ifdef _WINDOWS_
    lock_close_event = 0;   // Reenable event management
#endif
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


bool KICAD_MANAGER_FRAME::CloseProject( bool aSave )
{
    if( !Kiway().PlayersClose( false ) )
        return false;

    // Give a timeslice for the save-changes? dialog (and for the project to actually close)
    wxSafeYield();

    // Save the project file for the currently loaded project.
    if( m_active_project )
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

        mgr.TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );

        if( aSave )
            mgr.SaveProject();

        m_active_project = false;
        mgr.UnloadProject( &Prj() );
    }

    SetStatusText( "" );

    m_leftWin->EmptyTreePrj();

    Pgm().m_Quitting = true;

    return true;
}


void KICAD_MANAGER_FRAME::LoadProject( const wxFileName& aProjectFileName )
{
    // The project file should be valid by the time we get here or something has gone wrong.
    if( !aProjectFileName.Exists() )
        return;

    // Any open KIFACE's must be closed if they are not part of the new project.
    // (We never want a KIWAY_PLAYER open on a KIWAY that isn't in the same project.)
    // User is prompted here to close those KIWAY_PLAYERs:
    if( !CloseProject( true ) )
        return;

    m_active_project = true;

    Pgm().GetSettingsManager().LoadProject( aProjectFileName.GetFullPath() );

    LoadWindowState( aProjectFileName.GetFullName() );

    if( aProjectFileName.IsDirWritable() )
        SetMruPath( Prj().GetProjectPath() ); // Only set MRU path if we have write access. Why?

    UpdateFileHistory( Prj().GetProjectFullName() );

    m_leftWin->ReCreateTreePrj();

    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );

    PrintPrjInfo();

    KIPLATFORM::APP::RegisterApplicationRestart( aProjectFileName.GetFullPath() );
    m_openSavedWindows = true;
}


void KICAD_MANAGER_FRAME::CreateNewProject( const wxFileName& aProjectFileName,
                                            bool aCreateStubFiles )
{
    wxCHECK_RET( aProjectFileName.DirExists() && aProjectFileName.IsDirWritable(),
                 "Project folder must exist and be writable to create a new project." );

    // If the project is legacy, convert it
    if( !aProjectFileName.FileExists() )
    {
        wxFileName legacyPro( aProjectFileName );
        legacyPro.SetExt( LegacyProjectFileExtension );

        if( legacyPro.FileExists() )
        {
            GetSettingsManager()->LoadProject( legacyPro.GetFullPath() );
            GetSettingsManager()->SaveProject();

            wxRemoveFile( legacyPro.GetFullPath() );
        }
        else
        {
            // Copy template project file from template folder.
            wxString srcFileName = sys_search().FindValidPath( "kicad.kicad_pro" );

            wxFileName destFileName( aProjectFileName );
            destFileName.SetExt( ProjectFileExtension );

            // Create a minimal project file if the template project file could not be copied
            if( !wxFileName::FileExists( srcFileName )
                || !wxCopyFile( srcFileName, destFileName.GetFullPath() ) )
            {
                wxFFile file( destFileName.GetFullPath(), "wb" );

                if( file.IsOpened() )
                    file.Write( wxT( "{\n}\n") );

                // wxFFile dtor will close the file
            }
        }
    }

    // Create a "stub" for a schematic root sheet and a board if requested.
    // It will avoid messages from the schematic editor or the board editor to create a new file
    // And forces the user to create main files under the right name for the project manager
    if( aCreateStubFiles )
    {
        wxFileName fn( aProjectFileName.GetFullPath() );
        fn.SetExt( KiCadSchematicFileExtension );

        // If a <project>.kicad_sch file does not exist, create a "stub" file ( minimal schematic
        // file ).
        if( !fn.FileExists() )
        {
            wxFFile file( fn.GetFullPath(), "wb" );

            if( file.IsOpened() )
                file.Write( wxString::Format( "(kicad_sch (version %d) (generator eeschema)\n"
                                              "  (paper \"A4\")\n  (lib_symbols)\n"
                                              "  (symbol_instances)\n)\n",
                                              SEXPR_SCHEMATIC_FILE_VERSION ) );

            // wxFFile dtor will close the file
        }

        // If a <project>.kicad_pcb or <project>.brd file does not exist,
        // create a .kicad_pcb "stub" file
        fn.SetExt( KiCadPcbFileExtension );
        wxFileName leg_fn( fn );
        leg_fn.SetExt( LegacyPcbFileExtension );

        if( !fn.FileExists() && !leg_fn.FileExists() )
        {
            wxFFile file( fn.GetFullPath(), "wb" );

            if( file.IsOpened() )
                // Create a small dummy file as a stub for pcbnew:
                file.Write( wxString::Format( "(kicad_pcb (version %d) (generator pcbnew)\n)",
                                              SEXPR_BOARD_FILE_VERSION ) );

            // wxFFile dtor will close the file
        }
    }

    UpdateFileHistory( aProjectFileName.GetFullPath() );

    m_openSavedWindows = true;
}


void KICAD_MANAGER_FRAME::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    // show all files in file dialog (in Kicad all files are editable texts):
    wxString wildcard = AllFilesWildcard();

    wxString default_dir = Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Load File to Edit" ), default_dir,
                      wxEmptyString, wildcard, wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = wxT( "\"" );
    filename += dlg.GetPath() + wxT( "\"" );

    if( !dlg.GetPath().IsEmpty() && !Pgm().GetTextEditor().IsEmpty() )
        m_toolManager->RunAction( KICAD_MANAGER_ACTIONS::openTextEditor, true, &filename );
}


void KICAD_MANAGER_FRAME::OnBrowseInFileExplorer( wxCommandEvent& event )
{
    // open project directory in host OS's file explorer
    LaunchExternal( Prj().GetProjectPath() );
}


void KICAD_MANAGER_FRAME::RefreshProjectTree()
{
    m_leftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::language_change( wxCommandEvent& event )
{
    int id = event.GetId();
    Kiway().SetLanguage( id );
}


void KICAD_MANAGER_FRAME::ShowChangedLanguage()
{
    // call my base class
    EDA_BASE_FRAME::ShowChangedLanguage();

    // tooltips in toolbars
    RecreateBaseHToolbar();
    m_launcher->CreateLaunchers();

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CommonSettingsChanged( bool aEnvVarsChanged, bool aTextVarsChanged )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged, aTextVarsChanged );
}


void KICAD_MANAGER_FRAME::ProjectChanged()
{
    wxString file  = GetProjectFileName();
    wxString title;

    if( !file.IsEmpty() )
    {
        wxFileName fn( file );

        title = fn.GetName();

        if( !fn.IsDirWritable() )
            title += wxS( " " ) + _( "[Read Only]" );
    }
    else
    {
        title = _( "[no project loaded]" );
    }

    if( ADVANCED_CFG::GetCfg().m_HideVersionFromTitle )
        title += wxT( " \u2014 " ) + wxString( "KiCad" );
    else
        title += wxT( " \u2014 " ) + wxString( "KiCad " ) + GetMajorMinorVersion();

    SetTitle( title );
}


void KICAD_MANAGER_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    auto settings = dynamic_cast<KICAD_SETTINGS*>( aCfg );

    wxCHECK( settings, /*void*/ );

    m_leftWinWidth = settings->m_LeftWinWidth;
}


void KICAD_MANAGER_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    auto settings = dynamic_cast<KICAD_SETTINGS*>( aCfg );

    wxCHECK( settings, /*void*/);

    settings->m_LeftWinWidth = m_leftWin->GetSize().x;
}


void KICAD_MANAGER_FRAME::InstallPreferences( PAGED_DIALOG* aParent,
                                              PANEL_HOTKEYS_EDITOR* aHotkeysPanel  )
{
    aHotkeysPanel->AddHotKeys( GetToolManager() );
}


void KICAD_MANAGER_FRAME::PrintPrjInfo()
{
    // wxStatusBar's wxELLIPSIZE_MIDDLE flag doesn't work (at least on Mac).

    wxString     status = wxString::Format( _( "Project: %s" ), Prj().GetProjectFullName() );
    wxStatusBar* statusBar = GetStatusBar();
    int          width = statusBar->GetSize().GetWidth() / 2;

    if( width > 20 )
    {
        wxClientDC dc( this );
        status = wxControl::Ellipsize( status, dc, wxELLIPSIZE_MIDDLE, width );
    }

    SetStatusText( status );
}


bool KICAD_MANAGER_FRAME::IsProjectActive()
{
    return m_active_project;
}


void KICAD_MANAGER_FRAME::OnIdle( wxIdleEvent& aEvent )
{
    /**
     * We start loading the saved previously open windows on idle to avoid locking up the GUI
     * earlier in project loading. This gives us the visual effect of a opened KiCad project but
     * with a "busy" progress reporter
     */
    if( !m_openSavedWindows )
        return;

    m_openSavedWindows = false;

    if( Pgm().GetCommonSettings()->m_Session.remember_open_files )
    {
        int previousOpenCount =
                std::count_if( Prj().GetLocalSettings().m_files.begin(),
                               Prj().GetLocalSettings().m_files.end(),
                               [&]( const PROJECT_FILE_STATE& f )
                               {
                                   return !f.fileName.EndsWith( ProjectFileExtension ) && f.open;
                               } );

        if( previousOpenCount > 0 )
        {
            APP_PROGRESS_DIALOG progressReporter( _( "Restoring session" ), wxEmptyString,
                                                  previousOpenCount, this );

            int i = 0;

            for( const PROJECT_FILE_STATE& file : Prj().GetLocalSettings().m_files )
            {
                if( file.open )
                {
                    progressReporter.Update( i++,
                            wxString::Format( _( "Restoring '%s'" ), file.fileName ) );

                    wxFileName fn( file.fileName );

                    if( fn.GetExt() == LegacySchematicFileExtension
                            || fn.GetExt() == KiCadSchematicFileExtension )
                    {
                        GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::editSchematic, true );
                    }
                    else if( fn.GetExt() == LegacyPcbFileExtension
                             || fn.GetExt() == KiCadPcbFileExtension )
                    {
                        GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::editPCB, true );
                    }
                }

                wxYield();
            }
        }
    }

    // clear file states regardless if we opened windows or not due to setting
    Prj().GetLocalSettings().ClearFileState();
}

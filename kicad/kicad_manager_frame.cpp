/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 CERN (www.cern.ch)
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include "pcm.h"
#include "pgm_kicad.h"
#include "project_tree_pane.h"
#include "local_history_pane.h"
#include "widgets/bitmap_button.h"

#include <advanced_config.h>
#include <background_jobs_monitor.h>
#include <bitmaps.h>
#include <build_version.h>
#include <confirm.h>
#include <dialogs/panel_kicad_launcher.h>
#include <dialogs/panel_jobset.h>
#include <dialogs/dialog_edit_cfg.h>
#include <local_history.h>
#include <wx/msgdlg.h>
#include <eda_base_frame.h>
#include <executable_names.h>
#include <file_history.h>
#include <local_history.h>
#include <policy_keys.h>
#include <gestfich.h>
#include <kiplatform/app.h>
#include <kiplatform/environment.h>
#include <kiplatform/policy.h>
#include <build_version.h>
#include <kiway.h>
#include <kiway_express.h>
#include <launch_ext.h>
#include <lockfile.h>
#include <notifications_manager.h>
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
#include <toolbars_kicad_manager.h>
#include <wildcards_and_files_ext.h>
#include <widgets/app_progress_dialog.h>
#include <widgets/kistatusbar.h>
#include <wx/ffile.h>
#include <wx/filedlg.h>
#include <wx/dnd.h>
#include <wx/process.h>
#include <wx/snglinst.h>
#include <atomic>
#include <update_manager.h>
#include <jobs/jobset.h>

#include <../pcbnew/pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>   // for SEXPR_BOARD_FILE_VERSION def


#ifdef __WXMAC__
#include <MacTypes.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#include "kicad_manager_frame.h"
#include "settings/kicad_settings.h"

#include <project/project_file.h>


#define EDITORS_CAPTION _( "Editors" )
#define PROJECT_FILES_CAPTION _( "Project Files" )

#define ID_INIT_WATCHED_PATHS 52913

#define SEP()   wxFileName::GetPathSeparator()


// Menubar and toolbar event table
BEGIN_EVENT_TABLE( KICAD_MANAGER_FRAME, EDA_BASE_FRAME )
    // Window events
    EVT_SIZE( KICAD_MANAGER_FRAME::OnSize )
    EVT_IDLE( KICAD_MANAGER_FRAME::OnIdle )

    // Menu events
    EVT_MENU( wxID_EXIT, KICAD_MANAGER_FRAME::OnExit )
    EVT_MENU( ID_EDIT_LOCAL_FILE_IN_TEXT_EDITOR, KICAD_MANAGER_FRAME::OnOpenFileInTextEditor )
    EVT_MENU( ID_EDIT_ADVANCED_CFG, KICAD_MANAGER_FRAME::OnEditAdvancedCfg )
    EVT_MENU( ID_IMPORT_CADSTAR_ARCHIVE_PROJECT, KICAD_MANAGER_FRAME::OnImportCadstarArchiveFiles )
    EVT_MENU( ID_IMPORT_EAGLE_PROJECT, KICAD_MANAGER_FRAME::OnImportEagleFiles )
    EVT_MENU( ID_IMPORT_EASYEDA_PROJECT, KICAD_MANAGER_FRAME::OnImportEasyEdaFiles )
    EVT_MENU( ID_IMPORT_EASYEDAPRO_PROJECT, KICAD_MANAGER_FRAME::OnImportEasyEdaProFiles )
    EVT_MENU( ID_IMPORT_ALTIUM_PROJECT, KICAD_MANAGER_FRAME::OnImportAltiumProjectFiles )

    // Range menu events
    EVT_MENU_RANGE( ID_LANGUAGE_CHOICE, ID_LANGUAGE_CHOICE_END,
                    KICAD_MANAGER_FRAME::language_change )

    EVT_MENU_RANGE( ID_FILE1, ID_FILEMAX, KICAD_MANAGER_FRAME::OnFileHistory )
    EVT_MENU( ID_FILE_LIST_CLEAR, KICAD_MANAGER_FRAME::OnClearFileHistory )

    // Special functions
    EVT_MENU( ID_INIT_WATCHED_PATHS, KICAD_MANAGER_FRAME::OnChangeWatchedPaths )

    // Drop files event
    EVT_DROP_FILES( KICAD_MANAGER_FRAME::OnDropFiles )

END_EVENT_TABLE()

// See below the purpose of this include
#include <wx/xml/xml.h>
#include <widgets/wx_aui_art_providers.h>

KICAD_MANAGER_FRAME::KICAD_MANAGER_FRAME( wxWindow* parent, const wxString& title,
                                          const wxPoint& pos, const wxSize&   size ) :
        EDA_BASE_FRAME( parent, KICAD_MAIN_FRAME_T, title, pos, size, KICAD_DEFAULT_DRAWFRAME_STYLE,
                        KICAD_MANAGER_FRAME_NAME, &::Kiway, unityScale ),
        m_openSavedWindows( false ),
        m_restoredFromHistory( false ),
        m_active_project( false ),
        m_showHistoryPanel( false ),
        m_leftWin( nullptr ),
        m_historyPane( nullptr ),
        m_launcher( nullptr ),
        m_lastToolbarIconSize( 0 ),
        m_pcmButton( nullptr ),
        m_pcmUpdateCount( 0 )
{
    const int defaultLeftWinWidth = FromDIP( 250 );

    m_leftWinWidth = defaultLeftWinWidth; // Default value
    m_aboutTitle = "KiCad";

    // JPC: A very ugly hack to fix an issue on Linux: if the wxbase315u_xml_gcc_custom.so is
    // used **only** in PCM, it is not found in some cases at run time.
    // So just use it in the main module to avoid a not found issue
    // wxbase315u_xml_gcc_custom shared object when launching Kicad
    wxXmlDocument dummy;

    // Create the status line (bottom of the frame).  Left half is for project name; right half
    // is for Reporter (currently used by archiver/unarchiver and PCM).
    // Note: this is a KISTATUSBAR status bar. Therefore the specified number of fields
    // is the extra number of fields, not the full field count.
    // We need here 2 fields: the extra fiels to display the project name, and another field
    // to display a info (specific to Windows) using the FIELD_OFFSET_BGJOB_TEXT id offset (=1)
    // So the extra field count is 1
    CreateStatusBar( 2 );
    Pgm().GetBackgroundJobMonitor().RegisterStatusBar( (KISTATUSBAR*) GetStatusBar() );
    Pgm().GetNotificationsManager().RegisterStatusBar( (KISTATUSBAR*) GetStatusBar() );
    Pgm().RegisterLibraryLoadStatusBar( (KISTATUSBAR*) GetStatusBar() );
    GetStatusBar()->SetFont( KIUI::GetStatusFont( this ) );

    // Give an icon
    wxIcon icon;
    wxIconBundle icon_bundle;

    if( IsNightlyVersion())
    {
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly, 48 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly, 128 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly, 256 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly_32 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_nightly_16 ) );
        icon_bundle.AddIcon( icon );
    }
    else
    {
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad, 48 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad, 128 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad, 256 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_32 ) );
        icon_bundle.AddIcon( icon );
        icon.CopyFromBitmap( KiBitmap( BITMAPS::icon_kicad_16 ) );
        icon_bundle.AddIcon( icon );
    }

    SetIcons( icon_bundle );

    // Load the settings
    LoadSettings( config() );

    // Left window: is the box which display tree project
    m_leftWin = new PROJECT_TREE_PANE( this );

    setupTools();
    setupUIConditions();

    m_toolbarSettings = GetToolbarSettings<KICAD_MANAGER_TOOLBAR_SETTINGS>( "kicad-toolbars" );
    configureToolbars();
    RecreateToolbars();
    ReCreateMenuBar();

    m_auimgr.SetManagedWindow( this );
    m_auimgr.SetFlags( wxAUI_MGR_LIVE_RESIZE );

    m_auimgr.AddPane( m_tbLeft, EDA_PANE().VToolbar().Name( "TopMainToolbar" ).Left().Layer( 2 ) );

    // BestSize() does not always set the actual pane size of m_leftWin to the required value.
    // It happens when m_leftWin is too large (roughly > 1/3 of the kicad manager frame width.
    // (Well, BestSize() sets the best size... not the window size)
    // A trick is to use MinSize() to set the required pane width,
    // and after give a reasonable MinSize value
    m_auimgr.AddPane( m_leftWin, EDA_PANE().Palette().Name( "ProjectTree" ).Left().Layer( 1 )
                      .Caption( PROJECT_FILES_CAPTION ).PaneBorder( false )
                      .MinSize( m_leftWinWidth, -1 ).BestSize( m_leftWinWidth, -1 ) );

    m_historyPane = new LOCAL_HISTORY_PANE( this );
    m_auimgr.AddPane( m_historyPane,
                      EDA_PANE().Palette().Name( "LocalHistory" ).Left().Layer( 1 ).Position( 1 )
                              .Caption( _( "Local History" ) ).PaneBorder( false )
                              .Floatable( false ).Movable( false ).CloseButton( true ).Hide() );

    if( m_showHistoryPanel )
        m_auimgr.GetPane( m_historyPane ).Show();

    wxSize client_size = GetClientSize();
    m_notebook = new wxAuiNotebook( this, wxID_ANY, wxPoint( client_size.x, client_size.y ),
                                    FromDIP( wxSize( 700, 590 ) ),
                                    wxAUI_NB_TOP | wxAUI_NB_CLOSE_ON_ALL_TABS | wxAUI_NB_TAB_MOVE
                                            | wxAUI_NB_SCROLL_BUTTONS | wxNO_BORDER );

    m_notebook->SetArtProvider( new WX_AUI_TAB_ART() );

    m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &KICAD_MANAGER_FRAME::onNotebookPageCloseRequest, this );
    m_notebook->Bind( wxEVT_AUINOTEBOOK_PAGE_CLOSED, &KICAD_MANAGER_FRAME::onNotebookPageCountChanged, this );
    m_launcher = new PANEL_KICAD_LAUNCHER( m_notebook );

    m_notebook->Freeze();
    m_launcher->SetClosable( false );
    m_notebook->AddPage( m_launcher, EDITORS_CAPTION, false );
    m_notebook->SetTabCtrlHeight( 0 );
    m_notebook->Thaw();

    m_auimgr.AddPane( m_notebook, EDA_PANE().Canvas().Name( "Editors" ).Center().Caption( EDITORS_CAPTION )
                                            .PaneBorder( false ).MinSize( m_notebook->GetBestSize() ) );

    m_auimgr.Update();

    // Now the actual m_leftWin size is set, give it a reasonable min width
    m_auimgr.GetPane( m_leftWin ).MinSize( defaultLeftWinWidth, -1 );


    wxSizer* mainSizer = GetSizer();

    // Only fit the initial window size the first time KiCad is run.
    if( mainSizer && config()->m_Window.state.size_x == 0 && config()->m_Window.state.size_y == 0 )
    {
        Layout();
        mainSizer->Fit( this );
        Center();
    }

    if( ADVANCED_CFG::GetCfg().m_HideVersionFromTitle )
        SetTitle( wxT( "KiCad" ) );
    else
        SetTitle( wxString( "KiCad " ) + GetMajorMinorVersion() );

    // Do not let the messages window have initial focus
    m_leftWin->SetFocus();

    // Init for dropping files
    m_acceptedExts.emplace( FILEEXT::ProjectFileExtension, &KICAD_MANAGER_ACTIONS::loadProject );
    m_acceptedExts.emplace( FILEEXT::LegacyProjectFileExtension, &KICAD_MANAGER_ACTIONS::loadProject );

    // Gerber files
    // Note that all gerber files are aliased as GerberFileExtension
    m_acceptedExts.emplace( FILEEXT::GerberFileExtension, &KICAD_MANAGER_ACTIONS::viewDroppedGerbers );
    m_acceptedExts.emplace( FILEEXT::GerberJobFileExtension, &KICAD_MANAGER_ACTIONS::viewDroppedGerbers );
    m_acceptedExts.emplace( FILEEXT::DrillFileExtension, &KICAD_MANAGER_ACTIONS::viewDroppedGerbers );

    DragAcceptFiles( true );

    // Ensure the window is on top
    Raise();
}


KICAD_MANAGER_FRAME::~KICAD_MANAGER_FRAME()
{
    Unbind( wxEVT_CHAR, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );
    Unbind( wxEVT_CHAR_HOOK, &TOOL_DISPATCHER::DispatchWxEvent, m_toolDispatcher );

    m_notebook->Unbind( wxEVT_AUINOTEBOOK_PAGE_CLOSE, &KICAD_MANAGER_FRAME::onNotebookPageCloseRequest, this );
    m_notebook->Unbind( wxEVT_AUINOTEBOOK_PAGE_CLOSED, &KICAD_MANAGER_FRAME::onNotebookPageCountChanged, this );

    Pgm().GetBackgroundJobMonitor().UnregisterStatusBar( (KISTATUSBAR*) GetStatusBar() );
    Pgm().GetNotificationsManager().UnregisterStatusBar( (KISTATUSBAR*) GetStatusBar() );
    Pgm().UnregisterLibraryLoadStatusBar( (KISTATUSBAR*) GetStatusBar() );

    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    if( m_pcm )
        m_pcm->StopBackgroundUpdate();

    delete m_actions;
    delete m_toolManager;
    delete m_toolDispatcher;

    m_auimgr.UnInit();
}

void KICAD_MANAGER_FRAME::HideTabsIfNeeded()
{
    if( m_notebook->GetPageCount() == 1 )
        m_notebook->SetTabCtrlHeight( 0 );
    else
        m_notebook->SetTabCtrlHeight( -1 );
}


void KICAD_MANAGER_FRAME::onNotebookPageCountChanged( wxAuiNotebookEvent& evt )
{
    HideTabsIfNeeded();
}


void KICAD_MANAGER_FRAME::onNotebookPageCloseRequest( wxAuiNotebookEvent& evt )
{
    wxAuiNotebook* notebook = (wxAuiNotebook*) evt.GetEventObject();
    wxWindow*      page = notebook->GetPage( evt.GetSelection() );

    if( PANEL_NOTEBOOK_BASE* panel = dynamic_cast<PANEL_NOTEBOOK_BASE*>( page ) )
    {
        if( panel->GetClosable() )
        {
            if( !panel->GetCanClose() )
                evt.Veto();

            CallAfter(
                    [this]()
                    {
                        SaveOpenJobSetsToLocalSettings();
                    } );
        }
        else
        {
            evt.Veto();
        }
    }
}


wxStatusBar* KICAD_MANAGER_FRAME::OnCreateStatusBar( int number, long style, wxWindowID id,
                                                     const wxString& name )
{
    return new KISTATUSBAR( number, this, id,
                            static_cast<KISTATUSBAR::STYLE_FLAGS>(
                                    KISTATUSBAR::NOTIFICATION_ICON | KISTATUSBAR::CANCEL_BUTTON
                                    | KISTATUSBAR::WARNING_ICON ) );
}


void KICAD_MANAGER_FRAME::CreatePCM()
{
    // creates the PLUGIN_CONTENT_MANAGER, if not exists
    if( m_pcm )
        return;

    m_pcm = std::make_shared<PLUGIN_CONTENT_MANAGER>(
            [this]( int aUpdateCount )
            {
                m_pcmUpdateCount = aUpdateCount;

                if( aUpdateCount > 0 )
                {
                    Pgm().GetNotificationsManager().CreateOrUpdate(
                            wxS( "pcm" ),
                            _( "PCM Updates Available" ),
                            wxString::Format( _( "%d package update(s) available" ), aUpdateCount ),
                            wxT( "" ) );
                }
                else
                {
                    Pgm().GetNotificationsManager().Remove( wxS( "pcm" ) );
                }

                CallAfter(
                        [this]()
                        {
                            updatePcmButtonBadge();
                        } );
            });

    m_pcm->SetRepositoryList( kicadSettings()->m_PcmRepositories );
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

#define ENABLE( x ) ACTION_CONDITIONS().Enable( x )

    ACTION_CONDITIONS activeProjectCond;
    activeProjectCond.Enable( activeProject );

    manager->SetConditions( ACTIONS::saveAs,                       activeProjectCond );
    manager->SetConditions( KICAD_MANAGER_ACTIONS::closeProject,   activeProjectCond );
    manager->SetConditions( KICAD_MANAGER_ACTIONS::archiveProject, activeProjectCond );
    manager->SetConditions( KICAD_MANAGER_ACTIONS::newJobsetFile,  activeProjectCond );
    manager->SetConditions( KICAD_MANAGER_ACTIONS::openJobsetFile, activeProjectCond );

    auto historyCond =
            [this]( const SELECTION& )
            {
                return HistoryPanelShown();
            };

    manager->SetConditions( KICAD_MANAGER_ACTIONS::showLocalHistory,
                            ACTION_CONDITIONS().Check( historyCond ) );

    // These are just here for text boxes, search boxes, etc. in places such as the standard
    // file dialogs.
    manager->SetConditions( ACTIONS::cut,     ENABLE( SELECTION_CONDITIONS::ShowNever ) );
    manager->SetConditions( ACTIONS::copy,    ENABLE( SELECTION_CONDITIONS::ShowNever ) );
    manager->SetConditions( ACTIONS::paste,   ENABLE( SELECTION_CONDITIONS::ShowNever ) );

#undef ENABLE
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
    return Pgm().GetSettingsManager().IsProjectOpen() ? Prj().GetProjectFullName()
                                                      : wxString( wxEmptyString );
}


const wxString KICAD_MANAGER_FRAME::SchFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( FILEEXT::KiCadSchematicFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::SchLegacyFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( FILEEXT::LegacySchematicFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( FILEEXT::PcbFileExtension );
   return fn.GetFullPath();
}


const wxString KICAD_MANAGER_FRAME::PcbLegacyFileName()
{
   wxFileName   fn( GetProjectFileName() );

   fn.SetExt( FILEEXT::LegacyPcbFileExtension );
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

#if defined( _WIN32 )
    KISTATUSBAR* statusBar = static_cast<KISTATUSBAR*>( GetStatusBar() );
    statusBar->SetEllipsedTextField( m_FileWatcherInfo, 1 );
#endif

    event.Skip();
}


void KICAD_MANAGER_FRAME::DoWithAcceptedFiles()
{
    // All fileNames are now in m_AcceptedFiles vector.
    // Check if contains a project file name and load project.
    // If not, open files in dedicated app.
    for( const wxFileName& fileName : m_AcceptedFiles )
    {
        wxString ext = fileName.GetExt();

        if( ext == FILEEXT::ProjectFileExtension || ext == FILEEXT::LegacyProjectFileExtension )
        {
            wxString fn = fileName.GetFullPath();
            m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( fileName.GetExt() ), &fn );

            return;
        }
    }

    // Then stock gerber files in gerberFiles and run action for other files.
    wxString gerberFiles;

    // Gerbview editor should be able to open Gerber and drill files
    for( const wxFileName& fileName : m_AcceptedFiles )
    {
        wxString ext = fileName.GetExt();

        if( ext == FILEEXT::GerberJobFileExtension || ext == FILEEXT::DrillFileExtension
            || FILEEXT::IsGerberFileExtension( ext ) )
        {
            gerberFiles += wxT( '\"' );
            gerberFiles += fileName.GetFullPath() + wxT( '\"' );
            gerberFiles = gerberFiles.Pad( 1 );
        }
        else
        {
            wxString fn = fileName.GetFullPath();
            m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( fileName.GetExt() ), &fn );
        }
    }

    // Execute Gerbviewer
    if( !gerberFiles.IsEmpty() )
    {
        wxString fullEditorName = FindKicadFile( GERBVIEW_EXE );

        if( wxFileExists( fullEditorName ) )
        {
            wxString command = fullEditorName + " " + gerberFiles;
            m_toolManager->RunAction<wxString*>( *m_acceptedExts.at( FILEEXT::GerberFileExtension ), &command );
        }
    }
}


bool KICAD_MANAGER_FRAME::canCloseWindow( wxCloseEvent& aEvent )
{
    KICAD_SETTINGS* settings = kicadSettings();
    settings->m_OpenProjects = GetSettingsManager()->GetOpenProjects();

    for( size_t i = 0; i < m_notebook->GetPageCount(); i++ )
    {
        wxWindow* page = m_notebook->GetPage( i );

        if( PANEL_NOTEBOOK_BASE* panel = dynamic_cast<PANEL_NOTEBOOK_BASE*>( page ) )
        {
            if( !panel->GetCanClose() )
                return false;
        }
    }

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
    Pgm().m_Quitting = true;

    Destroy();

#ifdef _WINDOWS_
    lock_close_event = 0;   // Reenable event management
#endif
}


void KICAD_MANAGER_FRAME::SaveOpenJobSetsToLocalSettings( bool aIsExplicitUserSave )
{
    PROJECT_LOCAL_SETTINGS& cfg = Prj().GetLocalSettings();

    if( !aIsExplicitUserSave && !cfg.ShouldAutoSave() )
        return;

    cfg.m_OpenJobSets.clear();

    for( size_t i = 0; i < m_notebook->GetPageCount(); i++ )
    {
        if( PANEL_JOBSET* jobset = dynamic_cast<PANEL_JOBSET*>( m_notebook->GetPage( i ) ) )
        {
            wxFileName jobsetFn( jobset->GetFilePath() );
            jobsetFn.MakeRelativeTo( Prj().GetProjectPath() );
            cfg.m_OpenJobSets.emplace_back( jobsetFn.GetFullPath() );
        }
    }

    cfg.SaveToFile( Prj().GetProjectPath() );
}


void KICAD_MANAGER_FRAME::OnExit( wxCommandEvent& event )
{
    Close( true );
}


bool KICAD_MANAGER_FRAME::CloseProject( bool aSave )
{
    if( !Kiway().PlayersClose( false ) )
        return false;

    // Abort any in-progress background load, since the threads depend on the project not changing
    KIFACE *schface = Kiway().KiFACE( KIWAY::FACE_SCH );
    schface->CancelPreload();

    KIFACE *pcbface = Kiway().KiFACE( KIWAY::FACE_PCB );
    pcbface->CancelPreload();

    // Save the project file for the currently loaded project.
    if( m_active_project )
    {
        SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();

        if( Prj().GetLocalSettings().ShouldAutoSave() && Prj().GetProjectFile().ShouldAutoSave() )
        {
            mgr.TriggerBackupIfNeeded( NULL_REPORTER::GetInstance() );

            if( aSave )
                mgr.SaveProject();
        }

        // Ensure the Last_Save tag is at HEAD before closing. This handles the case where
        // autosave commits were made after the last explicit save - without this, the next
        // project load would offer to restore the autosave state, which is incorrect after
        // a clean close.
        wxString projPath = Prj().GetProjectPath();

        if( !projPath.IsEmpty() && Kiway().LocalHistory().HistoryExists( projPath ) )
        {
            if( Kiway().LocalHistory().HeadNewerThanLastSave( projPath ) )
            {
                // Commit the current on-disk state and tag it so Last_Save matches HEAD
                if( Kiway().LocalHistory().CommitFullProjectSnapshot( projPath, wxS( "Close" ) ) )
                {
                    Kiway().LocalHistory().TagSave( projPath, wxS( "project" ) );
                }
            }
        }

        m_active_project = false;
        // Enforce local history size limit (if enabled) once all pending saves/backups are done.
        if( Pgm().GetCommonSettings() && Pgm().GetCommonSettings()->m_Backup.enabled )
        {
            auto limit = Pgm().GetCommonSettings()->m_Backup.limit_total_size;

            if( limit > 0 )
                Kiway().LocalHistory().EnforceSizeLimit( Prj().GetProjectPath(), (size_t) limit );
        }

        // Unregister the project saver before unloading the project to prevent
        // dangling references
        Kiway().LocalHistory().UnregisterSaver( &Prj() );

        mgr.UnloadProject( &Prj() );
    }

    SetStatusText( "" );

    // Traverse pages in reverse order so deleting them doesn't mess up our iterator.
    for( int i = (int) m_notebook->GetPageCount() - 1; i >= 0; i-- )
    {
        wxWindow* page = m_notebook->GetPage( i );

        if( PANEL_NOTEBOOK_BASE* panel = dynamic_cast<PANEL_NOTEBOOK_BASE*>( page ) )
        {
            if( panel->GetProjectTied() )
                m_notebook->DeletePage( i );
        }
    }

    m_leftWin->EmptyTreePrj();
    HideTabsIfNeeded();

    return true;
}


void KICAD_MANAGER_FRAME::OpenJobsFile( const wxFileName& aFileName, bool aCreate,
                                        bool aResaveProjectPreferences )
{
    for( size_t i = 0; i < m_notebook->GetPageCount(); i++ )
    {
        if( PANEL_JOBSET* panel = dynamic_cast<PANEL_JOBSET*>( m_notebook->GetPage( i ) ) )
        {
            if( aFileName.GetFullPath() == panel->GetFilePath() )
            {
                m_notebook->SetSelection( i );
                return;
            }
        }
    }

    try
    {
        std::unique_ptr<JOBSET> jobsFile = std::make_unique<JOBSET>( aFileName.GetFullPath().ToStdString() );

        jobsFile->LoadFromFile();

        if( aCreate && !aFileName.FileExists() )
        {
            JOBSET_DESTINATION* dest = jobsFile->AddNewDestination( JOBSET_DESTINATION_T::FOLDER );
            dest->m_outputHandler->SetOutputPath( aFileName.GetName() );
            jobsFile->SaveToFile( wxEmptyString, true );
        }

        PANEL_JOBSET* jobPanel = new PANEL_JOBSET( m_notebook, this, std::move( jobsFile ) );
        jobPanel->SetProjectTied( true );
        jobPanel->SetClosable( true );
        m_notebook->AddPage( jobPanel, aFileName.GetFullName(), true );
        HideTabsIfNeeded();

        if( aResaveProjectPreferences )
            SaveOpenJobSetsToLocalSettings();
    }
    catch( ... )
    {
        DisplayErrorMessage( this, _( "Error opening jobs file" ) );
    }
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

    // NB: when loading a legacy project SETTINGS_MANAGER::LoadProject() will convert it to
    // current extension.  Be very careful with aProjectFileName vs. Prj().GetProjectPath()
    // from here on out.

    Pgm().GetSettingsManager().LoadProject( aProjectFileName.GetFullPath() );
    LoadWindowState( aProjectFileName.GetFullName() );

    if( aProjectFileName.IsDirWritable() )
        SetMruPath( Prj().GetProjectPath() );

    Kiway().LocalHistory().Init( Prj().GetProjectPath() );

    if( Kiway().LocalHistory().HeadNewerThanLastSave( Prj().GetProjectPath() ) )
    {
        wxString head = Kiway().LocalHistory().GetHeadHash( Prj().GetProjectPath() );
        if( wxMessageBox( _( "A newer local history snapshot is available. Restore it?" ),
                          _( "Restore" ), wxYES_NO | wxICON_QUESTION, this ) == wxYES )
        {
            Kiway().LocalHistory().RestoreCommit( Prj().GetProjectPath(), head, this );
        }
        else
        {
            // User declined to restore - commit the current on-disk state and tag it
            // so we don't prompt again on next load
            if( Kiway().LocalHistory().CommitFullProjectSnapshot( Prj().GetProjectPath(),
                                                                  wxS( "Declined restore" ) ) )
            {
                Kiway().LocalHistory().TagSave( Prj().GetProjectPath(), wxS( "project" ) );
            }
        }
    }

    // Save history & window state to disk now.  Don't wait around for a crash.
    KICAD_SETTINGS* settings = kicadSettings();
    SaveSettings( settings );
    settings->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( settings ) );

    m_leftWin->ReCreateTreePrj();
    m_historyPane->RefreshHistory( Prj().GetProjectPath() );

    for( const wxString& jobset : Prj().GetLocalSettings().m_OpenJobSets )
    {
        wxFileName jobsetFn( jobset );
        jobsetFn.MakeAbsolute( Prj().GetProjectPath() );

        if( jobsetFn.Exists() )
            OpenJobsFile( jobsetFn.GetFullPath(), false, false );
    }

    // Always start with the apps page
    m_notebook->SetSelection( 0 );

    // Rebuild the list of watched paths.
    // however this is possible only when the main loop event handler is running,
    // so we use it to run the rebuild function.
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED, ID_INIT_WATCHED_PATHS );

    wxPostEvent( this, cmd );

    PrintPrjInfo();

    KIPLATFORM::APP::RegisterApplicationRestart( aProjectFileName.GetFullPath() );
    m_openSavedWindows = true;

    KIPLATFORM::ENV::AddToRecentDocs( aProjectFileName.GetFullPath() );

    // Now that we have a new project, trigger a library preload, which will load in any
    // project-specific symbol and footprint libraries into the manager
    CallAfter( [&]()
            {
                KIFACE *schface = Kiway().KiFACE( KIWAY::FACE_SCH );
                schface->PreloadLibraries( &Kiway() );

                KIFACE *pcbface = Kiway().KiFACE( KIWAY::FACE_PCB );
                pcbface->PreloadLibraries( &Kiway() );

                Pgm().PreloadDesignBlockLibraries( &Kiway() );
            } );
}


void KICAD_MANAGER_FRAME::CreateNewProject( const wxFileName& aProjectFileName, bool aCreateStubFiles )
{
    wxCHECK_RET( aProjectFileName.DirExists() && aProjectFileName.IsDirWritable(),
                 "Project folder must exist and be writable to create a new project." );

    // If the project is legacy, convert it
    if( !aProjectFileName.FileExists() )
    {
        wxFileName legacyPro( aProjectFileName );
        legacyPro.SetExt( FILEEXT::LegacyProjectFileExtension );

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
            destFileName.SetExt( FILEEXT::ProjectFileExtension );

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
        fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

        // If a <project>.kicad_sch file does not exist, create a "stub" file ( minimal schematic
        // file ).
        if( !fn.FileExists() )
        {
            wxFFile file( fn.GetFullPath(), "wb" );

            if( file.IsOpened() )
            {
                file.Write( wxString::Format( "(kicad_sch\n"
                                              "\t(version %d)\n"
                                              "\t(generator \"eeschema\")\n"
                                              "\t(generator_version \"%s\")\n"
                                              "\t(uuid %s)\n"
                                              "\t(paper \"A4\")\n"
                                              "\t(lib_symbols)\n"
                                              "\t(sheet_instances\n"
                                              "\t\t(path \"/\"\n"
                                              "\t\t\t(page \"1\")\n"
                                              "\t\t)\n"
                                              "\t)\n"
                                              "\t(embedded_fonts no)\n"
                                              ")",
                                              SEXPR_SCHEMATIC_FILE_VERSION, GetMajorMinorVersion(),
                                              KIID().AsString() ) );
            }

            // wxFFile dtor will close the file
        }

        // If a <project>.kicad_pcb or <project>.brd file does not exist,
        // create a .kicad_pcb "stub" file
        fn.SetExt( FILEEXT::KiCadPcbFileExtension );
        wxFileName leg_fn( fn );
        leg_fn.SetExt( FILEEXT::LegacyPcbFileExtension );

        if( !fn.FileExists() && !leg_fn.FileExists() )
        {
            wxFFile file( fn.GetFullPath(), "wb" );

            if( file.IsOpened() )
            {
                // Create a small dummy file as a stub for pcbnew:
                file.Write( wxString::Format( "(kicad_pcb (version %d) (generator \"pcbnew\") (generator_version \"%s\")\n)",
                                              SEXPR_BOARD_FILE_VERSION, GetMajorMinorVersion() ) );
            }

            // wxFFile dtor will close the file
        }
    }

    // Save history & window state to disk now.  Don't wait around for a crash.
    KICAD_SETTINGS* settings = kicadSettings();
    SaveSettings( settings );
    settings->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( settings ) );

    m_openSavedWindows = true;
}


void KICAD_MANAGER_FRAME::OnOpenFileInTextEditor( wxCommandEvent& event )
{
    // show all files in file dialog (in Kicad all files are editable texts):
    wxString wildcard = FILEEXT::AllFilesWildcard();

    wxString default_dir = Prj().GetProjectPath();

    wxFileDialog dlg( this, _( "Edit File in Text Editor" ), default_dir,  wxEmptyString, wildcard,
                      wxFD_OPEN );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString filename = dlg.GetPath();

    if( !dlg.GetPath().IsEmpty() && !Pgm().GetTextEditor().IsEmpty() )
        m_toolManager->RunAction<wxString*>( KICAD_MANAGER_ACTIONS::openTextEditor, &filename );
}


void KICAD_MANAGER_FRAME::OnEditAdvancedCfg( wxCommandEvent& WXUNUSED( event ) )
{
    DIALOG_EDIT_CFG dlg( this );
    dlg.ShowModal();
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
    RecreateToolbars();
    m_launcher->CreateLaunchers();

    // update captions
    int pageId = m_notebook->FindPage( m_launcher );

    if( pageId != wxNOT_FOUND )
        m_notebook->SetPageText( pageId, EDITORS_CAPTION );

    m_auimgr.GetPane( m_leftWin ).Caption( PROJECT_FILES_CAPTION );
    m_auimgr.Update();

    m_leftWin->FileWatcherReset();

    PrintPrjInfo();
}


void KICAD_MANAGER_FRAME::CommonSettingsChanged( int aFlags )
{
    EDA_BASE_FRAME::CommonSettingsChanged( aFlags );

    if( m_pcm && ( aFlags & ENVVARS_CHANGED ) )
        m_pcm->ReadEnvVar();

    COMMON_SETTINGS* settings = Pgm().GetCommonSettings();

    if( m_lastToolbarIconSize == 0
        || m_lastToolbarIconSize != settings->m_Appearance.toolbar_icon_size )
    {
        onToolbarSizeChanged();
        m_lastToolbarIconSize = settings->m_Appearance.toolbar_icon_size;
    }

    m_leftWin->ReCreateTreePrj();
}


void KICAD_MANAGER_FRAME::ProjectChanged()
{
    wxString file = GetProjectFileName();

    // empty file string means no project loaded
    if( !Prj().IsNullProject() &&
        Prj().GetProjectLock() == nullptr )
    {
        LOCKFILE lockFile( file );

        if( !lockFile.Valid() && lockFile.IsLockedByMe() )
        {
            // If we cannot acquire the lock but we appear to be the one who
            // locked it, check to see if there is another KiCad instance running.
            // If there is not, then we can override the lock.  This could happen if
            // KiCad crashed or was interrupted
            if( !Pgm().SingleInstance()->IsAnotherRunning() )
            {
                lockFile.OverrideLock();
            }
        }

        if( !lockFile.Valid() )
        {
            wxString msg;
            msg.Printf( _( "Project '%s' is already open by '%s' at '%s'." ), file, lockFile.GetUsername(),
                        lockFile.GetHostname() );

            if( AskOverrideLock( this, msg ) )
            {
                lockFile.OverrideLock();
            }
        }

        Prj().SetReadOnly( !lockFile.Valid() || Prj().GetProjectFile().IsReadOnly() );
        Prj().SetProjectLock( new LOCKFILE( std::move( lockFile ) ) );
    }

    wxString title;

    if( !file.IsEmpty() )
    {
        wxFileName fn( file );

        title = fn.GetName();

        if( Prj().IsReadOnly() )
            title += wxS( " " ) + _( "[Read Only]" );
    }
    else
    {
        title = _( "[no project loaded]" );
    }

    if( ADVANCED_CFG::GetCfg().m_HideVersionFromTitle )
        title += wxT( " \u2014 " ) + wxString( wxS( "KiCad" ) );
    else
        title += wxT( " \u2014 " ) + wxString( wxS( "KiCad " ) ) + GetMajorMinorVersion();

    SetTitle( title );

    // Register project file saver. Ensures project file participates in
    // autosave history commits without affecting dirty state.
    Kiway().LocalHistory().RegisterSaver( &Prj(),
        [this]( const wxString& aProjectPath, std::vector<wxString>& aFiles )
        {
            Prj().SaveToHistory( aProjectPath, aFiles );
        } );
}


void KICAD_MANAGER_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::LoadSettings( aCfg );

    auto settings = dynamic_cast<KICAD_SETTINGS*>( aCfg );

    wxCHECK( settings, /*void*/ );

    m_leftWinWidth = settings->m_LeftWinWidth;
    m_showHistoryPanel = settings->m_ShowHistoryPanel;
}


void KICAD_MANAGER_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    EDA_BASE_FRAME::SaveSettings( aCfg );

    auto settings = dynamic_cast<KICAD_SETTINGS*>( aCfg );

    wxCHECK( settings, /*void*/);

    settings->m_LeftWinWidth = m_leftWin->GetSize().x;
    settings->m_ShowHistoryPanel = m_historyPane &&
                                   m_auimgr.GetPane( m_historyPane ).IsShown();

    if( !m_isClosing )
        settings->m_OpenProjects = GetSettingsManager()->GetOpenProjects();
}


void KICAD_MANAGER_FRAME::PrintPrjInfo()
{
    // wxStatusBar's wxELLIPSIZE_MIDDLE flag doesn't work (at least on Mac).

    wxString     status = wxString::Format( _( "Project: %s" ), Prj().GetProjectFullName() );
    KISTATUSBAR* statusBar = static_cast<KISTATUSBAR*>( GetStatusBar() );
    statusBar->SetEllipsedTextField( status, 0 );
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
                                   return !f.fileName.EndsWith( FILEEXT::ProjectFileExtension ) && f.open;
                               } );

        if( previousOpenCount > 0 )
        {
            APP_PROGRESS_DIALOG progressReporter( _( "Restoring session" ), wxEmptyString,
                                                  previousOpenCount, this );

            // We don't currently support opening more than one view per file
            std::set<wxString> openedFiles;

            int i = 0;

            for( const PROJECT_FILE_STATE& file : Prj().GetLocalSettings().m_files )
            {
                if( file.open && !openedFiles.count( file.fileName ) )
                {
                    progressReporter.Update( i++, wxString::Format( _( "Restoring '%s'" ), file.fileName ) );

                    openedFiles.insert( file.fileName );
                    wxFileName fn( file.fileName );

                    if( fn.GetExt() == FILEEXT::LegacySchematicFileExtension
                        || fn.GetExt() == FILEEXT::KiCadSchematicFileExtension )
                    {
                        GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::editSchematic );
                    }
                    else if( fn.GetExt() == FILEEXT::LegacyPcbFileExtension
                             || fn.GetExt() == FILEEXT::KiCadPcbFileExtension )
                    {
                        GetToolManager()->RunAction( KICAD_MANAGER_ACTIONS::editPCB );
                    }
                }

                wxYield();
            }
        }
    }

    // clear file states regardless if we opened windows or not due to setting
    Prj().GetLocalSettings().ClearFileState();

    // After restore from history, mark open editors as dirty so user is prompted to save
    if( m_restoredFromHistory )
    {
        m_restoredFromHistory = false;

        // Mark schematic editor as dirty if open
        if( KIWAY_PLAYER* schFrame = Kiway().Player( FRAME_SCH, false ) )
            schFrame->OnModify();

        // Mark PCB editor as dirty if open
        if( KIWAY_PLAYER* pcbFrame = Kiway().Player( FRAME_PCB_EDITOR, false ) )
            pcbFrame->OnModify();
    }

    KICAD_SETTINGS* settings = kicadSettings();

    if( KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM ) != KIPLATFORM::POLICY::PBOOL::DISABLED
        && settings->m_PcmUpdateCheck )
    {
        if( !m_pcm )
            CreatePCM();

        m_pcm->RunBackgroundUpdate();
    }

#ifdef KICAD_UPDATE_CHECK
    if( !m_updateManager && settings->m_KiCadUpdateCheck )
    {
        m_updateManager = std::make_unique<UPDATE_MANAGER>();
        m_updateManager->CheckForUpdate( this );
    }
#endif

    // This little diddy is needed to get the window put into the Mac dock icon's context menu.
    Raise();
}


void KICAD_MANAGER_FRAME::SetPcmButton( BITMAP_BUTTON* aButton )
{
    m_pcmButton = aButton;

    updatePcmButtonBadge();
}


void KICAD_MANAGER_FRAME::updatePcmButtonBadge()
{
    if( m_pcmButton )
    {
        if( m_pcmUpdateCount > 0 )
        {
            m_pcmButton->SetShowBadge( true );
            m_pcmButton->SetBadgeText( wxString::Format( "%d", m_pcmUpdateCount ) );
        }
        else
        {
            m_pcmButton->SetShowBadge( false );
        }

        m_pcmButton->Refresh();
    }
}


void KICAD_MANAGER_FRAME::onToolbarSizeChanged()
{
    // No idea why, but the same mechanism used in EDA_DRAW_FRAME doesn't work here
    // the only thing that seems to work is to blow it all up and start from scratch.
    m_auimgr.DetachPane( m_tbLeft );
    delete m_tbLeft;
    m_tbLeft = nullptr;
    RecreateToolbars();
    m_auimgr.AddPane( m_tbLeft, EDA_PANE().HToolbar().Name( "TopMainToolbar" ).Left()
                      .Layer( 2 ) );

    m_auimgr.Update();
}


void KICAD_MANAGER_FRAME::ToggleLocalHistory()
{
    wxAuiPaneInfo& pane = m_auimgr.GetPane( m_historyPane );
    bool show = !pane.IsShown();
    pane.Show( show );

    if( show )
        m_historyPane->RefreshHistory( Prj().GetProjectPath() );

    m_auimgr.Update();
}


void KICAD_MANAGER_FRAME::RestoreCommitFromHistory( const wxString& aHash )
{
    if( !Kiway().PlayersClose( true ) )
        return;

    if( Kiway().LocalHistory().RestoreCommit( Prj().GetProjectPath(), aHash, this ) )
    {
        m_restoredFromHistory = true;  // Mark editors dirty when they reopen
    }

    m_leftWin->ReCreateTreePrj();
    m_openSavedWindows = true;
    m_historyPane->RefreshHistory( Prj().GetProjectPath() );
}


bool KICAD_MANAGER_FRAME::HistoryPanelShown()
{
    return m_historyPane && m_auimgr.GetPane( m_historyPane ).IsShown();
}

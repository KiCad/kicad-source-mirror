/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <wildcards_and_files_ext.h>
#include <env_vars.h>
#include <executable_names.h>
#include <pgm_base.h>
#include <pgm_kicad.h>
#include <policy_keys.h>
#include <kiway.h>
#include <kicad_manager_frame.h>
#include <kiplatform/policy.h>
#include <kiplatform/secrets.h>
#include <confirm.h>
#include <kidialog.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <tool/selection.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tool/common_control.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <dialogs/panel_design_block_lib_table.h>
#include <dialogs/dialog_template_selector.h>
#include <dialogs/git/dialog_git_repository.h>
#include <git/git_clone_handler.h>
#include <gestfich.h>
#include <paths.h>
#include <wx/dir.h>
#include <wx/filedlg.h>
#include <wx/ffile.h>
#include "dialog_pcm.h"
#include <project/project_archiver.h>
#include <project_tree_pane.h>
#include <project_tree.h>
#include <project_tree_traverser.h>
#include <launch_ext.h>

#include "widgets/filedlg_new_project.h"

KICAD_MANAGER_CONTROL::KICAD_MANAGER_CONTROL() :
        TOOL_INTERACTIVE( "kicad.Control" ),
        m_frame( nullptr ),
        m_inShowPlayer( false )
{
}


void KICAD_MANAGER_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<KICAD_MANAGER_FRAME>();
}


wxFileName KICAD_MANAGER_CONTROL::newProjectDirectory( wxString* aFileName, bool isRepo )
{
    wxString default_filename = aFileName ? *aFileName : wxString();

    wxString        default_dir = m_frame->GetMruPath();
    wxFileDialog    dlg( m_frame, _( "Create New Project" ), default_dir, default_filename,
                         ( isRepo ? wxString( "" ) : FILEEXT::ProjectFileWildcard() ),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    dlg.AddShortcut( PATHS::GetDefaultUserProjectsPath() );

    // Add a "Create a new directory" checkbox
    FILEDLG_NEW_PROJECT newProjectHook;
    dlg.SetCustomizeHook( newProjectHook );

    if( dlg.ShowModal() == wxID_CANCEL )
        return wxFileName();

    wxFileName pro( dlg.GetPath() );

    // wxFileName automatically extracts an extension.  But if it isn't
    // a .pro extension, we should keep it as part of the filename
    if( !pro.GetExt().IsEmpty() && pro.GetExt().ToStdString() != FILEEXT::ProjectFileExtension )
        pro.SetName( pro.GetName() + wxT( "." ) + pro.GetExt() );

    pro.SetExt( FILEEXT::ProjectFileExtension ); // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    // Append a new directory with the same name of the project file.
    bool createNewDir = false;

    createNewDir = newProjectHook.GetCreateNewDir();

    if( createNewDir )
        pro.AppendDir( pro.GetName() );

    // Check if the project directory is empty if it already exists.
    wxDir directory( pro.GetPath() );

    if( !pro.DirExists() )
    {
        if( !pro.Mkdir() )
        {
            wxString msg;
            msg.Printf( _( "Folder '%s' could not be created.\n\n"
                           "Make sure you have write permissions and try again." ),
                        pro.GetPath() );
            DisplayErrorMessage( m_frame, msg );
            return wxFileName();
        }
    }
    else if( directory.HasFiles() )
    {
        wxString msg = _( "The selected folder is not empty.  It is recommended that you "
                          "create projects in their own empty folder.\n\n"
                          "Do you want to continue?" );

        if( !IsOK( m_frame, msg ) )
            return wxFileName();
    }

    return pro;
}


static wxFileName ensureDefaultProjectTemplate()
{
    ENV_VAR_MAP_CITER it = Pgm().GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

    if( it == Pgm().GetLocalEnvVariables().end() || it->second.GetValue() == wxEmptyString )
        return wxFileName();

    wxFileName templatePath;
    templatePath.AssignDir( it->second.GetValue() );
    templatePath.AppendDir( "default" );

    if( templatePath.DirExists() )
        return templatePath;

    if( !templatePath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        return wxFileName();

    wxFileName metaDir = templatePath;
    metaDir.AppendDir( METADIR );

    if( !metaDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL ) )
        return wxFileName();

    wxFileName infoFile = metaDir;
    infoFile.SetFullName( METAFILE_INFO_HTML );
    wxFFile info( infoFile.GetFullPath(), wxT( "w" ) );

    if( !info.IsOpened() )
        return wxFileName();

    info.Write( wxT( "<html><head><title>Default</title></head><body></body></html>" ) );
    info.Close();

    wxFileName proFile = templatePath;
    proFile.SetFullName( wxT( "default.kicad_pro" ) );
    wxFFile proj( proFile.GetFullPath(), wxT( "w" ) );

    if( !proj.IsOpened() )
        return wxFileName();

    proj.Write( wxT( "{}" ) );
    proj.Close();

    return templatePath;
}

int KICAD_MANAGER_CONTROL::NewProject( const TOOL_EVENT& aEvent )
{
    wxFileName defaultTemplate = ensureDefaultProjectTemplate();

    if( !defaultTemplate.IsOk() )
    {
        wxFileName pro = newProjectDirectory();

        if( !pro.IsOk() )
            return -1;

        m_frame->CreateNewProject( pro );
        m_frame->LoadProject( pro );

        return 0;
    }

    KICAD_SETTINGS*                settings = GetAppSettings<KICAD_SETTINGS>( "kicad" );
    std::vector<std::pair<wxString, wxFileName>> titleDirList;
    wxFileName                     templatePath;

    ENV_VAR_MAP_CITER itUser = Pgm().GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

    if( itUser != Pgm().GetLocalEnvVariables().end() && itUser->second.GetValue() != wxEmptyString )
    {
        templatePath.AssignDir( itUser->second.GetValue() );
        titleDirList.emplace_back( _( "User Templates" ), templatePath );
    }

    std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( Pgm().GetLocalEnvVariables(),
                                                                  wxT( "TEMPLATE_DIR" ) );

    if( v && !v->IsEmpty() )
    {
        templatePath.AssignDir( *v );
        titleDirList.emplace_back( _( "System Templates" ), templatePath );
    }

    // Use last used template if available, otherwise fall back to default
    wxFileName templateToSelect = defaultTemplate;

    if( !settings->m_LastUsedTemplate.IsEmpty() )
    {
        wxFileName lastUsed;
        lastUsed.AssignDir( settings->m_LastUsedTemplate );

        if( lastUsed.DirExists() )
            templateToSelect = lastUsed;
    }

    DIALOG_TEMPLATE_SELECTOR ps( m_frame, settings->m_TemplateWindowPos, settings->m_TemplateWindowSize,
                                 titleDirList, templateToSelect );

    int result = ps.ShowModal();

    settings->m_TemplateWindowPos = ps.GetPosition();
    settings->m_TemplateWindowSize = ps.GetSize();

    // Check if user wants to edit a template instead of creating new project
    if( result == wxID_APPLY )
    {
        wxString projectToEdit = ps.GetProjectToEdit();

        if( !projectToEdit.IsEmpty() && wxFileExists( projectToEdit ) )
        {
            m_frame->LoadProject( wxFileName( projectToEdit ) );
            return 0;
        }
    }

    if( result != wxID_OK )
        return -1;

    PROJECT_TEMPLATE* selectedTemplate = ps.GetSelectedTemplate();

    if( !selectedTemplate )
        selectedTemplate = ps.GetDefaultTemplate();

    if( !selectedTemplate )
    {
        wxMessageBox( _( "No project template was selected.  Cannot generate new project." ), _( "Error" ),
                      wxOK | wxICON_ERROR, m_frame );

        return -1;
    }

    wxString        default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString        title = _( "New Project Folder" );
    wxFileDialog    dlg( m_frame, title, default_dir, wxEmptyString, FILEEXT::ProjectFileWildcard(),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    dlg.AddShortcut( PATHS::GetDefaultUserProjectsPath() );

    FILEDLG_NEW_PROJECT newProjectHook;
    dlg.SetCustomizeHook( newProjectHook );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName fn( dlg.GetPath() );

    if( !fn.GetExt().IsEmpty() && fn.GetExt().ToStdString() != FILEEXT::ProjectFileExtension )
        fn.SetName( fn.GetName() + wxT( "." ) + fn.GetExt() );

    fn.SetExt( FILEEXT::ProjectFileExtension );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    bool createNewDir = false;
    createNewDir = newProjectHook.GetCreateNewDir();

    if( createNewDir )
        fn.AppendDir( fn.GetName() );

    if( !fn.DirExists() && !fn.Mkdir() )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Folder '%s' could not be created.\n\n"
                                                           "Make sure you have write permissions and try again." ),
                                                        fn.GetPath() ) );
        return -1;
    }

    if( !fn.IsDirWritable() )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Insufficient permissions to write to folder '%s'." ),
                                                        fn.GetPath() ) );
        return -1;
    }

    std::vector< wxFileName > destFiles;

    if( selectedTemplate->GetDestinationFiles( fn, destFiles ) )
    {
        std::vector<wxFileName> overwrittenFiles;

        for( const wxFileName& file : destFiles )
        {
            if( file.FileExists() )
                overwrittenFiles.push_back( file );
        }

        if( !overwrittenFiles.empty() )
        {
            wxString extendedMsg = _( "Overwriting files:" ) + "\n";

            for( const wxFileName& file : overwrittenFiles )
                extendedMsg += "\n" + file.GetFullName();

            KIDIALOG msgDlg( m_frame, _( "Similar files already exist in the destination folder." ),
                             _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            msgDlg.SetExtendedMessage( extendedMsg );
            msgDlg.SetOKLabel( _( "Overwrite" ) );
            msgDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( msgDlg.ShowModal() == wxID_CANCEL )
                return -1;
        }
    }

    wxString errorMsg;

    if( !selectedTemplate->CreateProject( fn, &errorMsg ) )
    {
        DisplayErrorMessage( m_frame, _( "A problem occurred creating new project from template." ), errorMsg );
        return -1;
    }

    // Save the last used template path for pre-selection next time
    wxFileName templateDir = selectedTemplate->GetHtmlFile();
    templateDir.RemoveLastDir();  // Remove "meta" directory
    settings->m_LastUsedTemplate = templateDir.GetPath();

    m_frame->CreateNewProject( fn.GetFullPath() );
    m_frame->LoadProject( fn );
    return 0;
}


int KICAD_MANAGER_CONTROL::NewFromRepository( const TOOL_EVENT& aEvent )
{
    DIALOG_GIT_REPOSITORY dlg( m_frame, nullptr );

    dlg.SetTitle( _( "Clone Project from Git Repository" ) );

    int ret = dlg.ShowModal();

    if( ret != wxID_OK )
        return -1;

    wxString   project_name = dlg.GetRepoName();
    wxFileName pro = newProjectDirectory( &project_name, true );

    if( !pro.IsOk() )
        return -1;

    PROJECT_TREE_PANE *pane = static_cast<PROJECT_TREE_PANE*>( m_frame->GetToolCanvas() );


    GIT_CLONE_HANDLER cloneHandler( pane->m_TreeProject->GitCommon() );

    cloneHandler.SetRemote( dlg.GetFullURL() );
    cloneHandler.SetClonePath( pro.GetPath() );
    cloneHandler.SetUsername( dlg.GetUsername() );
    cloneHandler.SetPassword( dlg.GetPassword() );
    cloneHandler.SetSSHKey( dlg.GetRepoSSHPath() );

    cloneHandler.SetProgressReporter( std::make_unique<WX_PROGRESS_REPORTER>( m_frame, _( "Clone Repository" ), 1,
                                                                              PR_NO_ABORT ) );

    if( !cloneHandler.PerformClone() )
    {
        DisplayErrorMessage( m_frame, cloneHandler.GetErrorString() );
        return -1;
    }

    std::vector<wxString> projects = cloneHandler.GetProjectDirs();

    if( projects.empty() )
    {
        DisplayErrorMessage( m_frame, _( "No project files were found in the repository." ) );
        return -1;
    }

    // Currently, we pick the first project file we find in the repository.
    // TODO: Look into spare checkout to allow the user to pick a partial repository
    wxString dest = pro.GetPath() + wxFileName::GetPathSeparator() + projects.front();
    m_frame->LoadProject( dest );

    KIPLATFORM::SECRETS::StoreSecret( dlg.GetRepoURL(), dlg.GetUsername(), dlg.GetPassword() );
    Prj().GetLocalSettings().m_GitRepoUsername = dlg.GetUsername();
    Prj().GetLocalSettings().m_GitSSHKey = dlg.GetRepoSSHPath();

    if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_SSH )
        Prj().GetLocalSettings().m_GitRepoType = "ssh";
    else if( dlg.GetRepoType() == KIGIT_COMMON::GIT_CONN_TYPE::GIT_CONN_HTTPS )
        Prj().GetLocalSettings().m_GitRepoType = "https";
    else
        Prj().GetLocalSettings().m_GitRepoType = "local";

    return 0;
}


int KICAD_MANAGER_CONTROL::NewJobsetFile( const TOOL_EVENT& aEvent )
{
    wxString     default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxFileDialog dlg( m_frame, _( "Create New Jobset" ), default_dir, wxEmptyString, FILEEXT::JobsetFileWildcard(),
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName jobsetFn( dlg.GetPath() );

    // Check if the file already exists
    bool fileExists = wxFileExists( jobsetFn.GetFullPath() );

    if( fileExists )
    {
        // Remove the existing file so that a new one can be created
        if( !wxRemoveFile( jobsetFn.GetFullPath() ) )
        {
            return -1;
        }
    }

    m_frame->OpenJobsFile( jobsetFn.GetFullPath(), true );

    return 0;
}




int KICAD_MANAGER_CONTROL::openProject( const wxString& aDefaultDir )
{
    wxString wildcard = FILEEXT::AllProjectFilesWildcard()
                        + "|" + FILEEXT::ProjectFileWildcard()
                        + "|" + FILEEXT::LegacyProjectFileWildcard();

    wxFileDialog dlg( m_frame, _( "Open Existing Project" ), aDefaultDir, wxEmptyString, wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    dlg.AddShortcut( PATHS::GetDefaultUserProjectsPath() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName pro( dlg.GetPath() );

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    // You'd think wxFD_FILE_MUST_EXIST and the wild-cards would enforce these.  Sentry
    // indicates otherwise (at least on MSW).
    if( !pro.Exists() || (   pro.GetExt() != FILEEXT::ProjectFileExtension
                          && pro.GetExt() != FILEEXT::LegacyProjectFileExtension ) )
    {
        return -1;
    }

    m_frame->LoadProject( pro );

    return 0;
}


int KICAD_MANAGER_CONTROL::OpenDemoProject( const TOOL_EVENT& aEvent )
{
    return openProject( PATHS::GetStockDemosPath() );
}


int KICAD_MANAGER_CONTROL::OpenProject( const TOOL_EVENT& aEvent )
{
    return openProject( m_frame->GetMruPath() );
}


int KICAD_MANAGER_CONTROL::OpenJobsetFile( const TOOL_EVENT& aEvent )
{
    wxString     default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxFileDialog dlg( m_frame, _( "Open Jobset" ), default_dir, wxEmptyString, FILEEXT::JobsetFileWildcard(),
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName jobsetFn( dlg.GetPath() );

    m_frame->OpenJobsFile( jobsetFn.GetFullPath(), true );

    return 0;
}


int KICAD_MANAGER_CONTROL::CloseProject( const TOOL_EVENT& aEvent )
{
    m_frame->CloseProject( true );
    return 0;
}


int KICAD_MANAGER_CONTROL::LoadProject( const TOOL_EVENT& aEvent )
{
    if( aEvent.Parameter<wxString*>() )
        m_frame->LoadProject( wxFileName( *aEvent.Parameter<wxString*>() ) );
    return 0;
}


int KICAD_MANAGER_CONTROL::ArchiveProject( const TOOL_EVENT& aEvent )
{
    wxFileName fileName = m_frame->GetProjectFileName();

    fileName.SetExt( FILEEXT::ArchiveFileExtension );

    wxFileDialog dlg( m_frame, _( "Archive Project Files" ), fileName.GetPath(), fileName.GetFullName(),
                      FILEEXT::ZipFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return 0;

    wxFileName zipFile = dlg.GetPath();

    wxString currdirname = fileName.GetPathWithSep();
    wxDir dir( currdirname );

    if( !dir.IsOpened() )   // wxWidgets display a error message on issue.
        return 0;

    STATUSBAR_REPORTER reporter( m_frame->GetStatusBar(), 1 );
    PROJECT_ARCHIVER archiver;

    archiver.Archive( currdirname, zipFile.GetFullPath(), reporter, true, true );
    return 0;
}


int KICAD_MANAGER_CONTROL::UnarchiveProject( const TOOL_EVENT& aEvent )
{
    m_frame->UnarchiveFiles();
    return 0;
}


int KICAD_MANAGER_CONTROL::ExploreProject( const TOOL_EVENT& aEvent )
{
    // Open project directory in host OS's file explorer
    LaunchExternal( Prj().GetProjectPath() );
    return 0;
}

int KICAD_MANAGER_CONTROL::RestoreLocalHistory( const TOOL_EVENT& aEvent )
{
    m_frame->RestoreLocalHistory();
    return 0;
}


int KICAD_MANAGER_CONTROL::ToggleLocalHistory( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleLocalHistory();
    return 0;
}


int KICAD_MANAGER_CONTROL::ViewDroppedViewers( const TOOL_EVENT& aEvent )
{
    if( aEvent.Parameter<wxString*>() )
        wxExecute( *aEvent.Parameter<wxString*>(), wxEXEC_ASYNC );

    return 0;
}



int KICAD_MANAGER_CONTROL::SaveProjectAs( const TOOL_EVENT& aEvent )
{
    wxString     msg;

    wxFileName   currentProjectFile( Prj().GetProjectFullName() );
    wxString     currentProjectDirPath = currentProjectFile.GetPath();
    wxString     currentProjectName = Prj().GetProjectName();

    wxString     default_dir = m_frame->GetMruPath();

    Prj().GetProjectFile().SaveToFile( currentProjectDirPath );
    Prj().GetLocalSettings().SaveToFile( currentProjectDirPath );

    if( default_dir == currentProjectDirPath
            || default_dir == currentProjectDirPath + wxFileName::GetPathSeparator() )
    {
        // Don't start within the current project
        wxFileName default_dir_fn( default_dir );
        default_dir_fn.RemoveLastDir();
        default_dir = default_dir_fn.GetPath();
    }

    wxFileDialog dlg( m_frame, _( "Save Project To" ), default_dir, wxEmptyString, wxEmptyString, wxFD_SAVE );

    dlg.AddShortcut( PATHS::GetDefaultUserProjectsPath() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName newProjectDir( dlg.GetPath(), wxEmptyString );

    if( !newProjectDir.IsAbsolute() )
        newProjectDir.MakeAbsolute();

    if( wxDirExists( newProjectDir.GetFullPath() ) )
    {
        msg.Printf( _( "'%s' already exists." ), newProjectDir.GetFullPath() );
        DisplayErrorMessage( m_frame, msg );
        return -1;
    }

    if( !wxMkdir( newProjectDir.GetFullPath() ) )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Folder '%s' could not be created.\n\n"
                                                           "Please make sure you have sufficient permissions." ),
                                                        newProjectDir.GetPath() ) );
        return -1;
    }

    if( !newProjectDir.IsDirWritable() )
    {
        DisplayErrorMessage( m_frame, wxString::Format( _( "Insufficient permissions to write to folder '%s'." ),
                                                        newProjectDir.GetFullPath() ) );
        return -1;
    }

    const wxString&   newProjectDirPath = newProjectDir.GetFullPath();
    const wxString&   newProjectName = newProjectDir.GetDirs().Last();
    wxDir             currentProjectDir( currentProjectDirPath );

    PROJECT_TREE_TRAVERSER traverser( m_frame, currentProjectDirPath, currentProjectName,
                                     newProjectDirPath, newProjectName );

    currentProjectDir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
        DisplayErrorMessage( m_frame, traverser.GetErrors() );

    if( !traverser.GetNewProjectFile().FileExists() )
        m_frame->CreateNewProject( traverser.GetNewProjectFile() );

    m_frame->LoadProject( traverser.GetNewProjectFile() );

    return 0;
}


int KICAD_MANAGER_CONTROL::Refresh( const TOOL_EVENT& aEvent )
{
    m_frame->RefreshProjectTree();
    return 0;
}


int KICAD_MANAGER_CONTROL::UpdateMenu( const TOOL_EVENT& aEvent )
{
    ACTION_MENU*      actionMenu = aEvent.Parameter<ACTION_MENU*>();
    CONDITIONAL_MENU* conditionalMenu = dynamic_cast<CONDITIONAL_MENU*>( actionMenu );
    SELECTION         dummySel;

    if( conditionalMenu )
        conditionalMenu->Evaluate( dummySel );

    if( actionMenu )
        actionMenu->UpdateAll();

    return 0;
}


int KICAD_MANAGER_CONTROL::ShowPlayer( const TOOL_EVENT& aEvent )
{
    FRAME_T       playerType = aEvent.Parameter<FRAME_T>();
    KIWAY_PLAYER* player;

    if( playerType == FRAME_SCH && !m_frame->IsProjectActive() )
    {
        DisplayInfoMessage( m_frame, _( "Create (or open) a project to edit a schematic." ), wxEmptyString );
        return -1;
    }
    else if( playerType == FRAME_PCB_EDITOR && !m_frame->IsProjectActive() )
    {
        DisplayInfoMessage( m_frame, _( "Create (or open) a project to edit a pcb." ), wxEmptyString );
        return -1;
    }

    if( m_inShowPlayer )
        return -1;

    REENTRANCY_GUARD guard( &m_inShowPlayer );

    try
    {
        player = m_frame->Kiway().Player( playerType, true );
    }
    catch( const IO_ERROR& err )
    {
        wxLogError( _( "Application failed to load:\n" ) + err.What() );
        return -1;
    }

    if ( !player )
    {
        wxLogError( _( "Application cannot start." ) );
        return -1;
    }

    if( !player->IsVisible() )   // A hidden frame might not have the document loaded.
    {
        wxString filepath;

        if( playerType == FRAME_SCH )
        {
            wxFileName  kicad_schematic( m_frame->SchFileName() );
            wxFileName  legacy_schematic( m_frame->SchLegacyFileName() );

            if( !legacy_schematic.FileExists() || kicad_schematic.FileExists() )
                filepath = kicad_schematic.GetFullPath();
            else
                filepath = legacy_schematic.GetFullPath();
        }
        else if( playerType == FRAME_PCB_EDITOR )
        {
            wxFileName  kicad_board( m_frame->PcbFileName() );
            wxFileName  legacy_board( m_frame->PcbLegacyFileName() );

            if( !legacy_board.FileExists() || kicad_board.FileExists() )
                filepath = kicad_board.GetFullPath();
            else
                filepath = legacy_board.GetFullPath();
        }

        if( !filepath.IsEmpty() )
        {
            std::vector<wxString> file_list{ filepath };

            if( !player->OpenProjectFiles( file_list ) )
            {
                player->Destroy();
                return -1;
            }
        }

        wxBusyCursor busy;
        player->Show( true );
    }

    // Needed on Windows, other platforms do not use it, but it creates no issue
    if( player->IsIconized() )
        player->Iconize( false );

    player->Raise();

    // Raising the window does not set the focus on Linux.  This should work on
    // any platform.
    if( wxWindow::FindFocus() != player )
        player->SetFocus();

    // Save window state to disk now.  Don't wait around for a crash.
    if( Pgm().GetCommonSettings()->m_Session.remember_open_files
            && !player->GetCurrentFileName().IsEmpty()
            && Prj().GetLocalSettings().ShouldAutoSave() )
    {
        wxFileName rfn( player->GetCurrentFileName() );
        rfn.MakeRelativeTo( Prj().GetProjectPath() );

        WINDOW_SETTINGS windowSettings;
        player->SaveWindowSettings( &windowSettings );

        Prj().GetLocalSettings().SaveFileState( rfn.GetFullPath(), &windowSettings, true );
        Prj().GetLocalSettings().SaveToFile( Prj().GetProjectPath() );
    }

    return 0;
}


int KICAD_MANAGER_CONTROL::Execute( const TOOL_EVENT& aEvent )
{
    wxString execFile;
    wxString param;

    if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::viewGerbers ) )
        execFile = GERBVIEW_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::convertImage ) )
        execFile = BITMAPCONVERTER_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::showCalculator ) )
        execFile = PCB_CALCULATOR_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editDrawingSheet ) )
        execFile = PL_EDITOR_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::openTextEditor ) )
        execFile = Pgm().GetTextEditor();
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editOtherSch ) )
        execFile = EESCHEMA_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editOtherPCB ) )
        execFile = PCBNEW_EXE;
    else
        wxFAIL_MSG( "Execute(): unexpected request" );

    if( execFile.IsEmpty() )
        return 0;

    if( aEvent.Parameter<wxString*>() )
        param = *aEvent.Parameter<wxString*>();
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::viewGerbers ) && m_frame->IsProjectActive() )
        param = m_frame->Prj().GetProjectPath();

    COMMON_CONTROL* commonControl = m_toolMgr->GetTool<COMMON_CONTROL>();
    return commonControl->Execute( execFile, param );
}


int KICAD_MANAGER_CONTROL::ShowPluginManager( const TOOL_EVENT& aEvent )
{
    if( KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM ) == KIPLATFORM::POLICY::PBOOL::DISABLED )
    {
        // policy disables the plugin manager
        return 0;
    }

    // For some reason, after a click or a double click the bitmap button calling
    // PCM keeps the focus althougt the focus was not set to this button.
    // This hack force removing the focus from this button
    m_frame->SetFocus();
    wxSafeYield();

    if( !m_frame->GetPcm() )
        m_frame->CreatePCM();

    DIALOG_PCM pcm( m_frame, m_frame->GetPcm() );
    pcm.ShowModal();

    const std::unordered_set<PCM_PACKAGE_TYPE>& changed = pcm.GetChangedPackageTypes();

    if( changed.count( PCM_PACKAGE_TYPE::PT_PLUGIN ) || changed.count( PCM_PACKAGE_TYPE::PT_FAB ) )
    {
        std::string payload = "";
        m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_RELOAD_PLUGINS, payload );
    }

    KICAD_SETTINGS* settings = GetAppSettings<KICAD_SETTINGS>( "kicad" );

    if( changed.count( PCM_PACKAGE_TYPE::PT_LIBRARY )
        && ( settings->m_PcmLibAutoAdd || settings->m_PcmLibAutoRemove ) )
    {
        KIWAY& kiway = m_frame->Kiway();

        // Reset state containing global lib tables
        if( KIFACE* kiface = kiway.KiFACE( KIWAY::FACE_SCH, false ) )
            kiface->Reset();

        if( KIFACE* kiface = kiway.KiFACE( KIWAY::FACE_PCB, false ) )
            kiface->Reset();

        // Reload lib tables
        std::string payload = "";

        kiway.ExpressMail( FRAME_FOOTPRINT_EDITOR, MAIL_RELOAD_LIB, payload );
        kiway.ExpressMail( FRAME_FOOTPRINT_VIEWER, MAIL_RELOAD_LIB, payload );
        kiway.ExpressMail( FRAME_CVPCB, MAIL_RELOAD_LIB, payload );
        kiway.ExpressMail( FRAME_SCH_SYMBOL_EDITOR, MAIL_RELOAD_LIB, payload );
        kiway.ExpressMail( FRAME_SCH_VIEWER, MAIL_RELOAD_LIB, payload );
    }

    if( changed.count( PCM_PACKAGE_TYPE::PT_COLORTHEME ) )
        Pgm().GetSettingsManager().ReloadColorSettings();

    return 0;
}


void KICAD_MANAGER_CONTROL::setTransitions()
{
    Go( &KICAD_MANAGER_CONTROL::NewProject,         KICAD_MANAGER_ACTIONS::newProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewFromRepository,  KICAD_MANAGER_ACTIONS::newFromRepository.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewJobsetFile,      KICAD_MANAGER_ACTIONS::newJobsetFile.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenDemoProject,    KICAD_MANAGER_ACTIONS::openDemoProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject,        KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenJobsetFile,     KICAD_MANAGER_ACTIONS::openJobsetFile.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::CloseProject,       KICAD_MANAGER_ACTIONS::closeProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::SaveProjectAs,      ACTIONS::saveAs.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::LoadProject,        KICAD_MANAGER_ACTIONS::loadProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ViewDroppedViewers, KICAD_MANAGER_ACTIONS::viewDroppedGerbers.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ArchiveProject,     KICAD_MANAGER_ACTIONS::archiveProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::UnarchiveProject,   KICAD_MANAGER_ACTIONS::unarchiveProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ExploreProject,     KICAD_MANAGER_ACTIONS::openProjectDirectory.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::RestoreLocalHistory, KICAD_MANAGER_ACTIONS::restoreLocalHistory.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ToggleLocalHistory, KICAD_MANAGER_ACTIONS::showLocalHistory.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Refresh,            ACTIONS::zoomRedraw.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::UpdateMenu,         ACTIONS::updateMenu.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,         KICAD_MANAGER_ACTIONS::editSchematic.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,         KICAD_MANAGER_ACTIONS::editSymbols.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,         KICAD_MANAGER_ACTIONS::editPCB.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,         KICAD_MANAGER_ACTIONS::editFootprints.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::viewGerbers.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::convertImage.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::showCalculator.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::editDrawingSheet.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::openTextEditor.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::editOtherSch.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,            KICAD_MANAGER_ACTIONS::editOtherPCB.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ShowPluginManager,  KICAD_MANAGER_ACTIONS::showPluginManager.MakeEvent() );
}

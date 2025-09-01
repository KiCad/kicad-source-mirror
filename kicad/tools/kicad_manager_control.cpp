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
#include <design_block_lib_table.h>
#include "dialog_pcm.h"
#include <project/project_archiver.h>
#include <project_tree_pane.h>
#include <project_tree.h>
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


int KICAD_MANAGER_CONTROL::NewProject( const TOOL_EVENT& aEvent )
{

    wxFileName pro = newProjectDirectory();

    if( !pro.IsOk() )
        return -1;

    m_frame->CreateNewProject( pro );
    m_frame->LoadProject( pro );

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

    cloneHandler.SetProgressReporter( std::make_unique<WX_PROGRESS_REPORTER>( m_frame, _( "Cloning Repository" ), 1 ) );

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


int KICAD_MANAGER_CONTROL::NewFromTemplate( const TOOL_EVENT& aEvent )
{
    SETTINGS_MANAGER&              mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*                settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );
    std::map<wxString, wxFileName> titleDirMap;

    wxFileName  templatePath;

    // KiCad system template path.
    std::optional<wxString> v = ENV_VAR::GetVersionedEnvVarValue( Pgm().GetLocalEnvVariables(),
                                                                  wxT( "TEMPLATE_DIR" ) );

    if( v && !v->IsEmpty() )
    {
        templatePath.AssignDir( *v );
        titleDirMap.emplace( _( "System Templates" ), templatePath );
    }

    // User template path.
    ENV_VAR_MAP_CITER it = Pgm().GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

    if( it != Pgm().GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
    {
        templatePath.AssignDir( it->second.GetValue() );
        titleDirMap.emplace( _( "User Templates" ), templatePath );
    }

    DIALOG_TEMPLATE_SELECTOR ps( m_frame, settings->m_TemplateWindowPos, settings->m_TemplateWindowSize,
                                 titleDirMap );

    // Show the project template selector dialog
    int result = ps.ShowModal();

    settings->m_TemplateWindowPos = ps.GetPosition();
    settings->m_TemplateWindowSize = ps.GetSize();

    if( result != wxID_OK )
        return -1;

    if( !ps.GetSelectedTemplate() )
    {
        wxMessageBox( _( "No project template was selected.  Cannot generate new project." ), _( "Error" ),
                      wxOK | wxICON_ERROR, m_frame );

        return -1;
    }

    // Get project destination folder and project file name.
    wxString        default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString        title = _( "New Project Folder" );
    wxFileDialog    dlg( m_frame, title, default_dir, wxEmptyString, FILEEXT::ProjectFileWildcard(),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    // Add a "Create a new directory" checkbox
    FILEDLG_NEW_PROJECT newProjectHook;
    dlg.SetCustomizeHook( newProjectHook );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName fn( dlg.GetPath() );

    // wxFileName automatically extracts an extension.  But if it isn't a .kicad_pro extension,
    // we should keep it as part of the filename
    if( !fn.GetExt().IsEmpty() && fn.GetExt().ToStdString() != FILEEXT::ProjectFileExtension )
        fn.SetName( fn.GetName() + wxT( "." ) + fn.GetExt() );

    fn.SetExt( FILEEXT::ProjectFileExtension );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    bool createNewDir = false;
    createNewDir = newProjectHook.GetCreateNewDir();

    // Append a new directory with the same name of the project file.
    if( createNewDir )
        fn.AppendDir( fn.GetName() );

    // Check if the project directory is empty if it already exists.

    if( !fn.DirExists() )
    {
        if( !fn.Mkdir() )
        {
            wxString msg;
            msg.Printf( _( "Folder '%s' could not be created.\n\n"
                           "Make sure you have write permissions and try again." ),
                        fn.GetPath() );
            DisplayErrorMessage( m_frame, msg );
            return -1;
        }
    }

    if( !fn.IsDirWritable() )
    {
        wxString msg;

        msg.Printf( _( "Insufficient permissions to write to folder '%s'." ), fn.GetPath() );
        wxMessageDialog msgDlg( m_frame, msg, _( "Error" ), wxICON_ERROR | wxOK | wxCENTER );
        msgDlg.ShowModal();
        return -1;
    }

    // Make sure we are not overwriting anything in the destination folder.
    std::vector< wxFileName > destFiles;

    if( ps.GetSelectedTemplate()->GetDestinationFiles( fn, destFiles ) )
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

    // The selected template widget contains the template we're attempting to use to
    // create a project
    if( !ps.GetSelectedTemplate()->CreateProject( fn, &errorMsg ) )
    {
        wxMessageDialog createDlg( m_frame, _( "A problem occurred creating new project from template." ),
                                   _( "Error" ), wxOK | wxICON_ERROR );

        if( !errorMsg.empty() )
            createDlg.SetExtendedMessage( errorMsg );

        createDlg.ShowModal();
        return -1;
    }

    m_frame->CreateNewProject( fn.GetFullPath() );
    m_frame->LoadProject( fn );
    return 0;
}


int KICAD_MANAGER_CONTROL::openProject( const wxString& aDefaultDir )
{
    wxString wildcard = FILEEXT::AllProjectFilesWildcard()
                        + "|" + FILEEXT::ProjectFileWildcard()
                        + "|" + FILEEXT::LegacyProjectFileWildcard();

    wxFileDialog dlg( m_frame, _( "Open Existing Project" ), aDefaultDir, wxEmptyString, wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

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

int KICAD_MANAGER_CONTROL::ViewDroppedViewers( const TOOL_EVENT& aEvent )
{
    if( aEvent.Parameter<wxString*>() )
        wxExecute( *aEvent.Parameter<wxString*>(), wxEXEC_ASYNC );
    return 0;
}

class SAVE_AS_TRAVERSER : public wxDirTraverser
{
public:
    SAVE_AS_TRAVERSER( KICAD_MANAGER_FRAME* aFrame,
                       const wxString& aSrcProjectDirPath,
                       const wxString& aSrcProjectName,
                       const wxString& aNewProjectDirPath,
                       const wxString& aNewProjectName ) :
            m_frame( aFrame ),
            m_projectDirPath( aSrcProjectDirPath ),
            m_projectName( aSrcProjectName ),
            m_newProjectDirPath( aNewProjectDirPath ),
            m_newProjectName( aNewProjectName )
    {
    }

    virtual wxDirTraverseResult OnFile( const wxString& aSrcFilePath ) override
    {
        // Recursion guard for a Save As to a location inside the source project.
        if( aSrcFilePath.StartsWith( m_newProjectDirPath + wxFileName::GetPathSeparator() ) )
            return wxDIR_CONTINUE;

        wxFileName destFile( aSrcFilePath );
        wxString   ext = destFile.GetExt();
        bool       atRoot = destFile.GetPath() == m_projectDirPath;

        if( ext == FILEEXT::LegacyProjectFileExtension
          || ext == FILEEXT::ProjectFileExtension
          || ext == FILEEXT::ProjectLocalSettingsFileExtension )
        {
            wxString destPath = destFile.GetPath();

            if( destPath.StartsWith( m_projectDirPath ) )
            {
                destPath.Replace( m_projectDirPath, m_newProjectDirPath, false );
                destFile.SetPath( destPath );
            }

            if( destFile.GetName() == m_projectName )
            {
                destFile.SetName( m_newProjectName );

                if( atRoot && ext != FILEEXT::ProjectLocalSettingsFileExtension )
                    m_newProjectFile = destFile;
            }

            if( ext == FILEEXT::LegacyProjectFileExtension )
            {
                // All paths in the settings file are relative so we can just do a straight copy
                KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
            }
            else if( ext == FILEEXT::ProjectFileExtension )
            {
                PROJECT_FILE projectFile( aSrcFilePath );
                projectFile.LoadFromFile();
                projectFile.SaveAs( destFile.GetPath(), destFile.GetName() );
            }
            else if( ext == FILEEXT::ProjectLocalSettingsFileExtension )
            {
                PROJECT_LOCAL_SETTINGS projectLocalSettings( nullptr, aSrcFilePath );
                projectLocalSettings.LoadFromFile();
                projectLocalSettings.SaveAs( destFile.GetPath(), destFile.GetName() );
            }
        }
        else if( ext == FILEEXT::KiCadSchematicFileExtension
                 || ext == FILEEXT::KiCadSchematicFileExtension + FILEEXT::BackupFileSuffix
                 || ext == FILEEXT::LegacySchematicFileExtension
                 || ext == FILEEXT::LegacySchematicFileExtension + FILEEXT::BackupFileSuffix
                 || ext == FILEEXT::SchematicSymbolFileExtension
                 || ext == FILEEXT::LegacySymbolLibFileExtension
                 || ext == FILEEXT::LegacySymbolDocumentFileExtension
                 || ext == FILEEXT::KiCadSymbolLibFileExtension
                 || ext == FILEEXT::NetlistFileExtension
                 || destFile.GetName() == FILEEXT::SymbolLibraryTableFileName )
        {
            KIFACE* eeschema = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH );
            eeschema->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == FILEEXT::KiCadPcbFileExtension
                 || ext == FILEEXT::KiCadPcbFileExtension + FILEEXT::BackupFileSuffix
                 || ext == FILEEXT::LegacyPcbFileExtension
                 || ext == FILEEXT::KiCadFootprintFileExtension
                 || ext == FILEEXT::LegacyFootprintLibPathExtension
                 || ext == FILEEXT::FootprintAssignmentFileExtension
                 || destFile.GetName() == FILEEXT::FootprintLibraryTableFileName )
        {
            KIFACE* pcbnew = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB );
            pcbnew->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == FILEEXT::DrawingSheetFileExtension )
        {
            KIFACE* pleditor = m_frame->Kiway().KiFACE( KIWAY::FACE_PL_EDITOR );
            pleditor->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == FILEEXT::GerberJobFileExtension
               || ext == FILEEXT::DrillFileExtension
                 || FILEEXT::IsGerberFileExtension( ext ) )
        {
            KIFACE* gerbview = m_frame->Kiway().KiFACE( KIWAY::FACE_GERBVIEW );
            gerbview->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( destFile.GetName().StartsWith( FILEEXT::LockFilePrefix )
                 && ext == FILEEXT::LockFileExtension )
        {
            // Ignore lock files
        }
        else
        {
            // Everything we don't recognize just gets a straight copy.
            wxString  destPath = destFile.GetPathWithSep();
            wxString  destName = destFile.GetName();
            wxUniChar pathSep = wxFileName::GetPathSeparator();

            wxString srcProjectFootprintLib = pathSep + m_projectName + ".pretty" + pathSep;
            wxString newProjectFootprintLib = pathSep + m_newProjectName + ".pretty" + pathSep;

            if( destPath.StartsWith( m_projectDirPath ) )
                destPath.Replace( m_projectDirPath, m_newProjectDirPath, false );

            destPath.Replace( srcProjectFootprintLib, newProjectFootprintLib, true );

            if( destName == m_projectName && ext != wxT( "zip" ) /* don't rename archives */ )
                destFile.SetName( m_newProjectName );

            destFile.SetPath( destPath );

            KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
        }

        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir( const wxString& aSrcDirPath ) override
    {
        // Recursion guard for a Save As to a location inside the source project.
        if( aSrcDirPath.StartsWith( m_newProjectDirPath ) )
            return wxDIR_CONTINUE;

        wxFileName destDir( aSrcDirPath );
        wxString   destDirPath = destDir.GetPathWithSep();
        wxUniChar  pathSep = wxFileName::GetPathSeparator();

        if( destDirPath.StartsWith( m_projectDirPath + pathSep )
          || destDirPath.StartsWith( m_projectDirPath + PROJECT_BACKUPS_DIR_SUFFIX ) )
        {
            destDirPath.Replace( m_projectDirPath, m_newProjectDirPath, false );
            destDir.SetPath( destDirPath );
        }

        if( destDir.GetName() == m_projectName )
        {
            if( destDir.GetExt() == "pretty" )
                destDir.SetName( m_newProjectName );
#if 0
            // WAYNE STAMBAUGH TODO:
            // If we end up with a symbol equivalent to ".pretty" we'll want to handle it here....
            else if( destDir.GetExt() == "sym_lib_dir_extension" )
                destDir.SetName( m_newProjectName );
#endif
        }

        if( !wxMkdir( destDir.GetFullPath() ) )
        {
            wxString msg;

            if( !m_errors.empty() )
                m_errors += "\n";

            msg.Printf( _( "Cannot copy folder '%s'." ), destDir.GetFullPath() );
            m_errors += msg;
        }

        return wxDIR_CONTINUE;
    }

    wxString GetErrors() { return m_errors; }

    wxFileName GetNewProjectFile() { return m_newProjectFile; }

private:
    KICAD_MANAGER_FRAME* m_frame;

    wxString             m_projectDirPath;
    wxString             m_projectName;
    wxString             m_newProjectDirPath;
    wxString             m_newProjectName;

    wxFileName           m_newProjectFile;
    wxString             m_errors;
};


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
        msg.Printf( _( "Folder '%s' could not be created.\n\n"
                       "Please make sure you have write permissions and try again." ),
                    newProjectDir.GetPath() );
        DisplayErrorMessage( m_frame, msg );
        return -1;
    }

    if( !newProjectDir.IsDirWritable() )
    {
        msg.Printf( _( "Insufficient permissions to write to folder '%s'." ),
                    newProjectDir.GetFullPath() );
        wxMessageDialog msgDlg( m_frame, msg, _( "Error!" ), wxICON_ERROR | wxOK | wxCENTER );
        msgDlg.ShowModal();
        return -1;
    }

    const wxString&   newProjectDirPath = newProjectDir.GetFullPath();
    const wxString&   newProjectName = newProjectDir.GetDirs().Last();
    wxDir             currentProjectDir( currentProjectDirPath );

    SAVE_AS_TRAVERSER traverser( m_frame, currentProjectDirPath, currentProjectName, newProjectDirPath,
                                 newProjectName );

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


class TERMINATE_HANDLER : public wxProcess
{
public:
    TERMINATE_HANDLER( const wxString& appName )
    { }

    void OnTerminate( int pid, int status ) override
    {
        delete this;
    }
};


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

    TERMINATE_HANDLER* callback = new TERMINATE_HANDLER( execFile );

    long pid = ExecuteFile( execFile, param, callback );

    if( pid > 0 )
    {
#ifdef __WXMAC__
        wxString script = wxString::Format( wxS( "tell application \"System Events\"\n"
                          "  set frontmost of the first process whose unix id is %l to true\n"
                          "end tell" ), pid );

        // This non-parameterized use of wxExecute is fine because script is not derived
        // from user input.
        wxExecute( wxString::Format( "osascript -e '%s'", script ) );
#endif
    }
    else
    {
        delete callback;
    }

    return 0;
}


int KICAD_MANAGER_CONTROL::ShowPluginManager( const TOOL_EVENT& aEvent )
{
    if( KIPLATFORM::POLICY::GetPolicyBool( POLICY_KEY_PCM )
        == KIPLATFORM::POLICY::PBOOL::DISABLED )
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

    SETTINGS_MANAGER& mgr = Pgm().GetSettingsManager();
    KICAD_SETTINGS*   settings = mgr.GetAppSettings<KICAD_SETTINGS>( "kicad" );

    if( changed.count( PCM_PACKAGE_TYPE::PT_LIBRARY )
        && ( settings->m_PcmLibAutoAdd || settings->m_PcmLibAutoRemove ) )
    {
        // Reset project tables
        Prj().SetElem( PROJECT::ELEM::SYMBOL_LIB_TABLE, nullptr );
        Prj().SetElem( PROJECT::ELEM::FPTBL, nullptr );
        Prj().SetElem( PROJECT::ELEM::DESIGN_BLOCK_LIB_TABLE, nullptr );

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
    Go( &KICAD_MANAGER_CONTROL::NewFromTemplate,    KICAD_MANAGER_ACTIONS::newFromTemplate.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewFromRepository,  KICAD_MANAGER_ACTIONS::newFromRepository.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewJobsetFile,        KICAD_MANAGER_ACTIONS::newJobsetFile.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenDemoProject,    KICAD_MANAGER_ACTIONS::openDemoProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject,        KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenJobsetFile,        KICAD_MANAGER_ACTIONS::openJobsetFile.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::CloseProject,       KICAD_MANAGER_ACTIONS::closeProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::SaveProjectAs,      ACTIONS::saveAs.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::LoadProject,        KICAD_MANAGER_ACTIONS::loadProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ViewDroppedViewers, KICAD_MANAGER_ACTIONS::viewDroppedGerbers.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Refresh,         ACTIONS::zoomRedraw.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::UpdateMenu,      ACTIONS::updateMenu.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,      KICAD_MANAGER_ACTIONS::editSchematic.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,      KICAD_MANAGER_ACTIONS::editSymbols.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,      KICAD_MANAGER_ACTIONS::editPCB.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,      KICAD_MANAGER_ACTIONS::editFootprints.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::viewGerbers.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::convertImage.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::showCalculator.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::editDrawingSheet.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::openTextEditor.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::editOtherSch.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::editOtherPCB.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ShowPluginManager, KICAD_MANAGER_ACTIONS::showPluginManager.MakeEvent() );
}

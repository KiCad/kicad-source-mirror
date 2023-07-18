/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <executable_names.h>
#include <pgm_base.h>
#include <policy_keys.h>
#include <kiway.h>
#include <kicad_manager_frame.h>
#include <kiplatform/policy.h>
#include <confirm.h>
#include <eda_tools.h>
#include <project/project_file.h>
#include <project/project_local_settings.h>
#include <settings/settings_manager.h>
#include <settings/kicad_settings.h>
#include <tool/selection.h>
#include <tool/tool_event.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <dialogs/dialog_template_selector.h>
#include <gestfich.h>
#include <paths.h>
#include <wx/checkbox.h>
#include <wx/dir.h>
#include <wx/filedlg.h>
#include "dialog_pcm.h"
#include <macros.h>
#include <sch_io_mgr.h>
#include <io_mgr.h>
#include <import_proj.h>

#if wxCHECK_VERSION( 3, 1, 7 )
#include "widgets/filedlg_new_project.h"
#else
#include "widgets/legacyfiledlg_new_project.h"
#endif

KICAD_MANAGER_CONTROL::KICAD_MANAGER_CONTROL() :
        TOOL_INTERACTIVE( "kicad.Control" ),
        m_frame( nullptr )
{
}


void KICAD_MANAGER_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<KICAD_MANAGER_FRAME>();
}


int KICAD_MANAGER_CONTROL::NewProject( const TOOL_EVENT& aEvent )
{
    wxString        default_dir = m_frame->GetMruPath();
    wxFileDialog    dlg( m_frame, _( "Create New Project" ), default_dir, wxEmptyString,
                         ProjectFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    // Add a "Create a new directory" checkbox
#if wxCHECK_VERSION( 3, 1, 7 )
    FILEDLG_NEW_PROJECT newProjectHook;
    dlg.SetCustomizeHook( newProjectHook );
#else
    dlg.SetExtraControlCreator( &LEGACYFILEDLG_NEW_PROJECT::Create );
#endif

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName pro( dlg.GetPath() );

    // wxFileName automatically extracts an extension.  But if it isn't
    // a .pro extension, we should keep it as part of the filename
    if( !pro.GetExt().IsEmpty() && pro.GetExt().ToStdString() != ProjectFileExtension )
        pro.SetName( pro.GetName() + wxT( "." ) + pro.GetExt() );

    pro.SetExt( ProjectFileExtension );     // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    // Append a new directory with the same name of the project file.
    bool createNewDir = false;

#if wxCHECK_VERSION( 3, 1, 7 )
    createNewDir = newProjectHook.GetCreateNewDir();
#else
    createNewDir = static_cast<LEGACYFILEDLG_NEW_PROJECT*>( dlg.GetExtraControl() )->CreateNewDir();
#endif

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
            return -1;
        }
    }
    else if( directory.HasFiles() )
    {
        wxString msg = _( "The selected folder is not empty.  It is recommended that you "
                          "create projects in their own empty folder.\n\n"
                          "Do you want to continue?" );

        if( !IsOK( m_frame, msg ) )
            return -1;
    }

    m_frame->CreateNewProject( pro );
    m_frame->LoadProject( pro );

    return 0;
}


int KICAD_MANAGER_CONTROL::NewFromTemplate( const TOOL_EVENT& aEvent )
{
    DIALOG_TEMPLATE_SELECTOR* ps = new DIALOG_TEMPLATE_SELECTOR( m_frame );

    wxFileName  templatePath;

    // KiCad system template path.
    ENV_VAR_MAP_CITER it =  Pgm().GetLocalEnvVariables().find( "KICAD7_TEMPLATE_DIR" );

    if( it != Pgm().GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
    {
        templatePath.AssignDir( it->second.GetValue() );
        ps->AddTemplatesPage( _( "System Templates" ), templatePath );
    }

    // User template path.
    it = Pgm().GetLocalEnvVariables().find( "KICAD_USER_TEMPLATE_DIR" );

    if( it != Pgm().GetLocalEnvVariables().end() && it->second.GetValue() != wxEmptyString )
    {
        templatePath.AssignDir( it->second.GetValue() );
        ps->AddTemplatesPage( _( "User Templates" ), templatePath );
    }

    // Show the project template selector dialog
    if( ps->ShowModal() != wxID_OK )
        return -1;

    if( !ps->GetSelectedTemplate() )
    {
        wxMessageBox( _( "No project template was selected.  Cannot generate new project." ),
                      _( "Error" ), wxOK | wxICON_ERROR, m_frame );

        return -1;
    }

    // Get project destination folder and project file name.
    wxString        default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString        title = _( "New Project Folder" );
    wxFileDialog    dlg( m_frame, title, default_dir, wxEmptyString, ProjectFileWildcard(),
                         wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    // Add a "Create a new directory" checkbox
#if wxCHECK_VERSION( 3, 1, 7 )
    FILEDLG_NEW_PROJECT newProjectHook;
    dlg.SetCustomizeHook( newProjectHook );
#else
    dlg.SetExtraControlCreator( &LEGACYFILEDLG_NEW_PROJECT::Create );
#endif


    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName fn( dlg.GetPath() );

    // wxFileName automatically extracts an extension.  But if it isn't a .kicad_pro extension,
    // we should keep it as part of the filename
    if( !fn.GetExt().IsEmpty() && fn.GetExt().ToStdString() != ProjectFileExtension )
        fn.SetName( fn.GetName() + wxT( "." ) + fn.GetExt() );

    fn.SetExt( ProjectFileExtension );

    if( !fn.IsAbsolute() )
        fn.MakeAbsolute();

    bool createNewDir = false;
#if wxCHECK_VERSION( 3, 1, 7 )
    createNewDir = newProjectHook.GetCreateNewDir();
#else
    createNewDir = static_cast<LEGACYFILEDLG_NEW_PROJECT*>( dlg.GetExtraControl() )->CreateNewDir();
#endif

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

    if( ps->GetSelectedTemplate()->GetDestinationFiles( fn, destFiles ) )
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

            KIDIALOG msgDlg( m_frame,
                             _( "Similar files already exist in the destination folder." ),
                             _( "Confirmation" ),
                             wxOK | wxCANCEL | wxICON_WARNING );
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
    if( !ps->GetSelectedTemplate()->CreateProject( fn, &errorMsg ) )
    {
        wxMessageDialog createDlg( m_frame,
                                   _( "A problem occurred creating new project from template." ),
                                   _( "Error" ),
                                   wxOK | wxICON_ERROR );

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
    wxString wildcard = AllProjectFilesWildcard()
                        + "|" + ProjectFileWildcard()
                        + "|" + LegacyProjectFileWildcard();

    wxFileDialog dlg( m_frame, _( "Open Existing Project" ), aDefaultDir, wxEmptyString, wildcard,
                      wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName pro( dlg.GetPath() );

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    if( !pro.FileExists() )
        return -1;

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


int KICAD_MANAGER_CONTROL::CloseProject( const TOOL_EVENT& aEvent )
{
    m_frame->CloseProject( true );
    return 0;
}


int KICAD_MANAGER_CONTROL::ImportNonKicadProj( const TOOL_EVENT& aEvent )
{
    if( !aEvent.Parameter<wxString*>() )
        return -1;

    wxFileName droppedFileName( *aEvent.Parameter<wxString*>() );

    wxString schFileExtension, pcbFileExtension;
    int      schFileType, pcbFileType;

    // Define extensions to use according to dropped file.
    // Eagle project.
    if( droppedFileName.GetExt() == EagleSchematicFileExtension
        || droppedFileName.GetExt() == EaglePcbFileExtension )
    {
        // Check if droppedFile is an eagle file.
        // If not, return and do not import files.
        if( !IsFileFromEDATool( droppedFileName, EDA_TOOLS::EAGLE ) )
            return -1;

        schFileExtension = EagleSchematicFileExtension;
        pcbFileExtension = EaglePcbFileExtension;
        schFileType = SCH_IO_MGR::SCH_EAGLE;
        pcbFileType = IO_MGR::EAGLE;
    }
    // Cadstar project.
    else if( droppedFileName.GetExt() == CadstarSchematicFileExtension
             || droppedFileName.GetExt() == CadstarPcbFileExtension )
    {
        schFileExtension = CadstarSchematicFileExtension;
        pcbFileExtension = CadstarPcbFileExtension;
        schFileType = SCH_IO_MGR::SCH_CADSTAR_ARCHIVE;
        pcbFileType = IO_MGR::CADSTAR_PCB_ARCHIVE;
    }
    else
    {
        return -1;
    }

    IMPORT_PROJ_HELPER importProj( m_frame, droppedFileName.GetFullPath(), schFileExtension,
                                   pcbFileExtension );

    // Check if the project directory is empty
    wxDir directory( importProj.GetProjPath() );

    if( directory.HasFiles() )
    {
        // Append a new directory with the same name of the project file
        // Keep iterating until we find an empty directory
        importProj.CreateEmptyDirForProject();

        if( !wxMkdir( importProj.GetProjPath() ) )
            return -1;
    }

    importProj.SetProjAbsolutePath();

    if( !importProj.CopyImportedFiles( false ) )
    {
        wxRmdir( importProj.GetProjPath() ); // Remove the previous created directory, before leaving.
        return -1;
    }

    m_frame->CloseProject( true );

    m_frame->CreateNewProject( importProj.GetProjFullPath(), false /* Don't create stub files */ );
    m_frame->LoadProject( importProj.GetProj() );

    importProj.AssociateFilesWithProj( schFileType, pcbFileType );
    m_frame->ReCreateTreePrj();
    m_frame->LoadProject( importProj.GetProj() );
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

        if( ext == LegacyProjectFileExtension
          || ext == ProjectFileExtension
          || ext == ProjectLocalSettingsFileExtension )
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

                if( atRoot && ext != ProjectLocalSettingsFileExtension )
                    m_newProjectFile = destFile;
            }

            if( ext == LegacyProjectFileExtension )
            {
                // All paths in the settings file are relative so we can just do a straight copy
                KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
            }
            else if( ext == ProjectFileExtension )
            {
                PROJECT_FILE projectFile( aSrcFilePath );
                projectFile.LoadFromFile();
                projectFile.SaveAs( destFile.GetPath(), destFile.GetName() );
            }
            else if( ext == ProjectLocalSettingsFileExtension )
            {
                PROJECT_LOCAL_SETTINGS projectLocalSettings( nullptr, aSrcFilePath );
                projectLocalSettings.LoadFromFile();
                projectLocalSettings.SaveAs( destFile.GetPath(), destFile.GetName() );
            }
        }
        else if( ext == KiCadSchematicFileExtension
               || ext == KiCadSchematicFileExtension + BackupFileSuffix
               || ext == LegacySchematicFileExtension
               || ext == LegacySchematicFileExtension + BackupFileSuffix
               || ext == SchematicSymbolFileExtension
               || ext == LegacySymbolLibFileExtension
               || ext == LegacySymbolDocumentFileExtension
               || ext == KiCadSymbolLibFileExtension
               || ext == NetlistFileExtension
               || destFile.GetName() == "sym-lib-table" )
        {
            KIFACE* eeschema = m_frame->Kiway().KiFACE( KIWAY::FACE_SCH );
            eeschema->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == KiCadPcbFileExtension
               || ext == KiCadPcbFileExtension + BackupFileSuffix
               || ext == LegacyPcbFileExtension
               || ext == KiCadFootprintFileExtension
               || ext == LegacyFootprintLibPathExtension
               || ext == FootprintAssignmentFileExtension
               || destFile.GetName() == "fp-lib-table" )
        {
            KIFACE* pcbnew = m_frame->Kiway().KiFACE( KIWAY::FACE_PCB );
            pcbnew->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == DrawingSheetFileExtension )
        {
            KIFACE* pleditor = m_frame->Kiway().KiFACE( KIWAY::FACE_PL_EDITOR );
            pleditor->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if( ext == GerberJobFileExtension
               || ext == DrillFileExtension
               || IsGerberFileExtension(ext) )
        {
            KIFACE* gerbview = m_frame->Kiway().KiFACE( KIWAY::FACE_GERBVIEW );
            gerbview->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else if(destFile.GetName().StartsWith( LockFilePrefix ) && ext == LockFileExtension )
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

    wxFileDialog dlg( m_frame, _( "Save Project To" ), default_dir, wxEmptyString, wxEmptyString,
                      wxFD_SAVE );

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

    SAVE_AS_TRAVERSER traverser( m_frame, currentProjectDirPath, currentProjectName,
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
        DisplayInfoMessage( m_frame, _( "Create (or open) a project to edit a schematic." ),
                            wxEmptyString );
        return -1;
    }
    else if( playerType == FRAME_PCB_EDITOR && !m_frame->IsProjectActive() )
    {
        DisplayInfoMessage( m_frame, _( "Create (or open) a project to edit a pcb." ),
                            wxEmptyString );
        return -1;
    }

    // Prevent multiple KIWAY_PLAYER loading at one time
    if( !m_loading.try_lock() )
        return -1;

    const std::lock_guard<std::mutex> lock( m_loading, std::adopt_lock );

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
            if( !player->OpenProjectFiles( std::vector<wxString>( 1, filepath ) ) )
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
        // This non-parameterized use of wxExecute is fine because execFile is not derived
        // from user input.
        wxExecute( "osascript -e 'activate application \"" + execFile + "\"'" );
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

    if( changed.count( PCM_PACKAGE_TYPE::PT_PLUGIN ) )
    {
        std::string payload = "";
        m_frame->Kiway().ExpressMail( FRAME_PCB_EDITOR, MAIL_RELOAD_PLUGINS, payload );
    }

    KICAD_SETTINGS* settings = Pgm().GetSettingsManager().GetAppSettings<KICAD_SETTINGS>();

    if( changed.count( PCM_PACKAGE_TYPE::PT_LIBRARY )
        && ( settings->m_PcmLibAutoAdd || settings->m_PcmLibAutoRemove ) )
    {
        // Reset project tables
        Prj().SetElem( PROJECT::ELEM_SYMBOL_LIB_TABLE, nullptr );
        Prj().SetElem( PROJECT::ELEM_FPTBL, nullptr );

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
    Go( &KICAD_MANAGER_CONTROL::OpenDemoProject,    KICAD_MANAGER_ACTIONS::openDemoProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject,        KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::CloseProject,       KICAD_MANAGER_ACTIONS::closeProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::SaveProjectAs,      ACTIONS::saveAs.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::LoadProject,        KICAD_MANAGER_ACTIONS::loadProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ImportNonKicadProj, KICAD_MANAGER_ACTIONS::importNonKicadProj.MakeEvent() );
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

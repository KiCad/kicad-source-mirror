/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <kicad_manager_frame.h>
#include <confirm.h>
#include <settings/settings_manager.h>
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
#include <common.h>

#ifdef PCM
#include "dialog_pcm.h"
#endif


///> Helper widget to select whether a new directory should be created for a project.
class DIR_CHECKBOX : public wxPanel
{
public:
    DIR_CHECKBOX( wxWindow* aParent )
            : wxPanel( aParent )
    {
        m_cbCreateDir = new wxCheckBox( this, wxID_ANY,
                                        _( "Create a new folder for the project" ) );
        m_cbCreateDir->SetValue( true );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_cbCreateDir, 0, wxALL, 8 );

        SetSizerAndFit( sizer );
    }

    bool CreateNewDir() const
    {
        return m_cbCreateDir->GetValue();
    }

    static wxWindow* Create( wxWindow* aParent )
    {
        return new DIR_CHECKBOX( aParent );
    }

protected:
    wxCheckBox* m_cbCreateDir;
};


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
    dlg.SetExtraControlCreator( &DIR_CHECKBOX::Create );

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
    if( static_cast<DIR_CHECKBOX*>( dlg.GetExtraControl() )->CreateNewDir() )
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
    wxString    envStr;

    // KiCad system template path.
    ENV_VAR_MAP_CITER it =  Pgm().GetLocalEnvVariables().find( "KICAD6_TEMPLATE_DIR" );

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
    dlg.SetExtraControlCreator( &DIR_CHECKBOX::Create );

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

    // Append a new directory with the same name of the project file.
    if( static_cast<DIR_CHECKBOX*>( dlg.GetExtraControl() )->CreateNewDir() )
        fn.AppendDir( fn.GetName() );

    // Check if the project directory is empty if it already exists.
    wxDir directory( fn.GetPath() );

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
        std::vector< wxFileName > overwrittenFiles;

        for( const auto& file : destFiles )
        {
            if( file.FileExists() )
                overwrittenFiles.push_back( file );
        }

        if( !overwrittenFiles.empty() )
        {
            wxString extendedMsg = _( "Overwriting files:" ) + "\n";

            for( const auto& file : overwrittenFiles )
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
    wxString wildcard = AllProjectFilesWildcard() + "|" + ProjectFileWildcard() + "|"
                        + LegacyProjectFileWildcard();

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
        if( aSrcFilePath.StartsWith( m_newProjectDirPath ) )
            return wxDIR_CONTINUE;

        wxFileName destFile( aSrcFilePath );
        wxString   ext = destFile.GetExt();
        bool       atRoot = destFile.GetPath() == m_projectDirPath;

        if( ext == "pro" )
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

                if( atRoot )
                    m_newProjectFile = destFile;
            }

            // Currently all paths in the settings file are relative, so we can just do a
            // straight copy
            KiCopyFile( aSrcFilePath, destFile.GetFullPath(), m_errors );
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
        else if( ext == GerberFileExtension
               || ext == GerberJobFileExtension
               || ext == DrillFileExtension
               || IsProtelExtension( ext ) )
        {
            KIFACE* gerbview = m_frame->Kiway().KiFACE( KIWAY::FACE_GERBVIEW );
            gerbview->SaveFileAs( m_projectDirPath, m_projectName, m_newProjectDirPath,
                                  m_newProjectName, aSrcFilePath, m_errors );
        }
        else
        {
            // Everything we don't recognize just gets a straight copy.
            wxString destPath = destFile.GetPathWithSep();
            wxString destName = destFile.GetName();
            wxUniChar  pathSep = wxFileName::GetPathSeparator();

            wxString srcProjectFootprintLib = pathSep + m_projectName + ".pretty" + pathSep;
            wxString newProjectFootprintLib = pathSep + m_newProjectName + ".pretty" + pathSep;

            if( destPath.StartsWith( m_projectDirPath ) )
            {
                destPath.Replace( m_projectDirPath, m_newProjectDirPath, false );
                destFile.SetPath( destPath );
            }

            if( destName == m_projectName )
                destFile.SetName( m_newProjectName );

            destPath.Replace( srcProjectFootprintLib, newProjectFootprintLib, true );
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

    wxFileName newProjectDir( dlg.GetPath() );

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
    const wxString&   newProjectName = newProjectDir.GetName();
    wxDir             currentProjectDir( currentProjectDirPath );

    SAVE_AS_TRAVERSER traverser( m_frame, currentProjectDirPath, currentProjectName,
                                 newProjectDirPath, newProjectName );

    currentProjectDir.Traverse( traverser );

    if( !traverser.GetErrors().empty() )
        DisplayErrorMessage( m_frame, traverser.GetErrors() );

    if( traverser.GetNewProjectFile().FileExists() )
    {
        m_frame->CreateNewProject( traverser.GetNewProjectFile() );
        m_frame->LoadProject( traverser.GetNewProjectFile() );
    }

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
        wxMessageBox( _( "Application failed to load:\n" ) + err.What(), _( "KiCad Error" ),
                      wxOK | wxICON_ERROR, m_frame );
        return -1;
    }

    if ( !player )
    {
        wxMessageBox( _( "Application failed to load." ), _( "KiCad Error" ),
                      wxOK | wxICON_ERROR, m_frame );
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
    TERMINATE_HANDLER( const wxString& appName ) :
            m_appName( appName )
    { }

    void OnTerminate( int pid, int status ) override
    {
        wxString msg = wxString::Format( _( "%s closed [pid=%d]\n" ), m_appName, pid );

        wxWindow* window = wxWindow::FindWindowByName( KICAD_MANAGER_FRAME_NAME );

        if( window )    // Should always happen.
        {
            // Be sure the kicad frame manager is found
            // This dynamic cast is not really mandatory, but ...
            KICAD_MANAGER_FRAME* frame = dynamic_cast<KICAD_MANAGER_FRAME*>( window );

            if( frame )
                frame->PrintMsg( msg );
        }

        delete this;
    }

private:
    wxString m_appName;
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
#ifdef PCM
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::showPluginManager ) )
    {
        DIALOG_PCM* pcm = new DIALOG_PCM( m_frame );
        pcm->ShowModal();
        pcm->Destroy();
    }
#endif
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
        wxString msg = wxString::Format( _( "%s %s opened [pid=%ld]\n" ), execFile, param, pid );
        m_frame->PrintMsg( msg );

#ifdef __WXMAC__
        wxExecute( wxString::Format( "osascript -e 'activate application \"%s\"' ", execFile ) );
#endif
    }
    else
    {
        delete callback;
    }

    return 0;
}


void KICAD_MANAGER_CONTROL::setTransitions()
{
    Go( &KICAD_MANAGER_CONTROL::NewProject,      KICAD_MANAGER_ACTIONS::newProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewFromTemplate, KICAD_MANAGER_ACTIONS::newFromTemplate.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenDemoProject, KICAD_MANAGER_ACTIONS::openDemoProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject,     KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::CloseProject,    KICAD_MANAGER_ACTIONS::closeProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::SaveProjectAs,   ACTIONS::saveAs.MakeEvent() );

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

#ifdef PCM
    Go( &KICAD_MANAGER_CONTROL::Execute,         KICAD_MANAGER_ACTIONS::showPluginManager.MakeEvent() );
#endif
}

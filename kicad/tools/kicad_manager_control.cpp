/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gestfich.h>
#include <wildcards_and_files_ext.h>
#include <executable_names.h>
#include <pgm_base.h>
#include <kiway.h>
#include <kicad_manager_frame.h>
#include <confirm.h>
#include <bitmaps.h>
#include <tool/selection.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <dialogs/dialog_template_selector.h>

///> Helper widget to select whether a new directory should be created for a project
class DIR_CHECKBOX : public wxPanel
{
public:
    DIR_CHECKBOX( wxWindow* aParent )
            : wxPanel( aParent )
    {
        m_cbCreateDir = new wxCheckBox( this, wxID_ANY,
                                        _( "Create a new directory for the project" ) );
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
    if( !pro.GetExt().IsEmpty()
        && pro.GetExt().ToStdString() != ProjectFileExtension )
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
            msg.Printf( _( "Directory \"%s\" could not be created.\n\n"
                           "Please make sure you have write permissions and try again." ),
                        pro.GetPath() );
            DisplayErrorMessage( m_frame, msg );
            return -1;
        }
    }
    else if( directory.HasFiles() )
    {
        wxString msg = _( "The selected directory is not empty.  It is recommended that you "
                          "create projects in their own empty directory.\n\nDo you "
                          "want to continue?" );

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
    ENV_VAR_MAP_CITER it =  Pgm().GetLocalEnvVariables().find( "KICAD_TEMPLATE_DIR" );

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

    // wxFileName automatically extracts an extension.  But if it isn't
    // a .pro extension, we should keep it as part of the filename
    if( !fn.GetExt().IsEmpty()
        && fn.GetExt().ToStdString() != ProjectFileExtension )
        fn.SetName( fn.GetName() + wxT( "." ) + fn.GetExt() );

    fn.SetExt( ProjectFileExtension );     // enforce extension

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
            msg.Printf( _( "Directory \"%s\" could not be created.\n\n"
                           "Please make sure you have write permissions and try again." ),
                        fn.GetPath() );
            DisplayErrorMessage( m_frame, msg );
            return -1;
        }
    }

    if( !fn.IsDirWritable() )
    {
        wxString msg;

        msg.Printf( _( "Cannot write to folder \"%s\"." ), fn.GetPath() );
        wxMessageDialog msgDlg( m_frame, msg, _( "Error!" ), wxICON_ERROR | wxOK | wxCENTER );
        msgDlg.SetExtendedMessage( _( "Please check your access permissions to this folder "
                                      "and try again." ) );
        msgDlg.ShowModal();
        return -1;
    }

    m_frame->ClearMsg();

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
                                   _( "A problem occurred creating new project from template!" ),
                                   _( "Template Error" ),
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


int KICAD_MANAGER_CONTROL::OpenProject( const TOOL_EVENT& aEvent )
{
    wxString     default_dir = m_frame->GetMruPath();
    wxFileDialog dlg( m_frame, _( "Open Existing Project" ), default_dir, wxEmptyString,
                      ProjectFileWildcard(), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    wxFileName pro( dlg.GetPath() );
    pro.SetExt( ProjectFileExtension );     // enforce extension

    if( !pro.IsAbsolute() )
        pro.MakeAbsolute();

    if( !pro.FileExists() )
        return -1;

    m_frame->LoadProject( pro );
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

    if( !player->IsVisible() )   // A hidden frame might not have the document loaded.
    {
        wxString filepath;
        
        if( playerType == FRAME_SCH )
        {
            filepath = m_frame->SchFileName();
        }
        else if( playerType == FRAME_PCB )
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
                return -1;
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
private:
    wxString m_appName;

public:
    TERMINATE_HANDLER( const wxString& appName ) :
            m_appName( appName )
    { }

    void OnTerminate( int pid, int status ) override
    {
        wxString msg = wxString::Format( _( "%s closed [pid=%d]\n" ),
                                         m_appName,
                                         pid );

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
};


int KICAD_MANAGER_CONTROL::Execute( const TOOL_EVENT& aEvent )
{
    wxString execFile;
    wxString params;

    if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::viewGerbers ) )
        execFile = GERBVIEW_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::convertImage ) )
        execFile = BITMAPCONVERTER_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::showCalculator ) )
        execFile = PCB_CALCULATOR_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editWorksheet ) )
        execFile = PL_EDITOR_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::openTextEditor ) )
        execFile = Pgm().GetEditorName();
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editOtherSch ) )
        execFile = EESCHEMA_EXE;
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::editOtherPCB ) )
        execFile = PCBNEW_EXE;
    else
        wxFAIL_MSG( "Execute(): unexpected request" );

    if( execFile.IsEmpty() )
        return 0;

    if( aEvent.Parameter<wxString*>() )
        params = *aEvent.Parameter<wxString*>();
    else if( aEvent.IsAction( &KICAD_MANAGER_ACTIONS::viewGerbers ) )
        params = m_frame->Prj().GetProjectPath();

    if( !params.empty() )
        AddDelimiterString( params );

    TERMINATE_HANDLER* callback = new TERMINATE_HANDLER( execFile );

    long pid = ExecuteFile( m_frame, execFile, params, callback );

    if( pid > 0 )
    {
        wxString msg = wxString::Format( _( "%s %s opened [pid=%ld]\n" ),
                                         execFile,
                                         params,
                                         pid );
        m_frame->PrintMsg( msg );

#ifdef __WXMAC__
        msg.Printf( "osascript -e 'activate application \"%s\"' ", execFile );
        system( msg.c_str() );
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
    Go( &KICAD_MANAGER_CONTROL::NewProject,    KICAD_MANAGER_ACTIONS::newProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewFromTemplate, KICAD_MANAGER_ACTIONS::newFromTemplate.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject,   KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Refresh,       ACTIONS::zoomRedraw.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::UpdateMenu,    ACTIONS::updateMenu.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,    KICAD_MANAGER_ACTIONS::editSchematic.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,    KICAD_MANAGER_ACTIONS::editSymbols.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,    KICAD_MANAGER_ACTIONS::editPCB.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::ShowPlayer,    KICAD_MANAGER_ACTIONS::editFootprints.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::viewGerbers.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::convertImage.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::showCalculator.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::editWorksheet.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::openTextEditor.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::editOtherSch.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::Execute,       KICAD_MANAGER_ACTIONS::editOtherPCB.MakeEvent() );
}

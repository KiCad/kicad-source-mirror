/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <kicad_manager_frame.h>
#include <bitmaps.h>
#include <wildcards_and_files_ext.h>
#include <tool/selection.h>
#include <tool/tool_event.h>
#include <tool/tool_manager.h>
#include <tools/kicad_manager_actions.h>
#include <tools/kicad_manager_control.h>
#include <confirm.h>
#include <dialogs/dialog_template_selector.h>
#include <pgm_base.h>

TOOL_ACTION KICAD_MANAGER_ACTIONS::newProject( "kicad.Control.newProject",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_NEW ),
        _( "New Project..." ), _( "Create new blank project" ),
        new_project_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::newFromTemplate( "kicad.Control.newFromTemplate",
        AS_GLOBAL, 0, // TOOL_ACTION::LegacyHotKey( HK_NEW_PRJ_TEMPLATE ),
        _( "New Project from Template..." ), _( "Create new project from template" ),
        new_project_with_template_xpm );

TOOL_ACTION KICAD_MANAGER_ACTIONS::openProject( "kicad.Control.openProject",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_OPEN ),
        _( "Open Project..." ), _( "Open an existing project" ),
        open_project_xpm );


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

    if( ps->GetSelectedTemplate() == NULL )
    {
        wxMessageBox( _( "No project template was selected.  Cannot generate new project." ),
                      _( "Error" ),
                      wxOK | wxICON_ERROR, m_frame );

        return -1;
    }

    // Get project destination folder and project file name.
    wxString        default_dir = wxFileName( Prj().GetProjectFullName() ).GetPathWithSep();
    wxString        title = _( "New Project Folder" );
    wxFileDialog    dlg( m_frame, title, default_dir, wxEmptyString,
                         ProjectFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

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


void KICAD_MANAGER_CONTROL::setTransitions()
{
    Go( &KICAD_MANAGER_CONTROL::NewProject,  KICAD_MANAGER_ACTIONS::newProject.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::NewProject,  KICAD_MANAGER_ACTIONS::newFromTemplate.MakeEvent() );
    Go( &KICAD_MANAGER_CONTROL::OpenProject, KICAD_MANAGER_ACTIONS::openProject.MakeEvent() );

    Go( &KICAD_MANAGER_CONTROL::Refresh,     ACTIONS::zoomRedraw.MakeEvent() );
}

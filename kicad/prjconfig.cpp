/**
 * @file prjconfig.cpp
 * Load and save project configuration files (*.pro)
 */
#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "prjconfig.h"
#include "kicad.h"
#include "tree_project_frame.h"

#include "build_version.h"

static const wxString GeneralGroupName( wxT( "/general" ) );

/* Kicad project file entry names. */
static const wxString SchematicRootNameEntry( wxT( "RootSch" ) );
static const wxString BoardFileNameEntry( wxT( "BoardNm" ) );


void WinEDA_MainFrame::CreateNewProject( const wxString PrjFullFileName )
{
    wxString   filename;
    wxFileName newProjectName = PrjFullFileName;

    /* Init default config filename */
    filename = wxGetApp().FindLibraryPath( wxT( "kicad.pro" ) );

    /* Check if file kicad.pro exist in template directory */
    if( wxFileName::FileExists( filename ) )
    {
      wxCopyFile( filename, PrjFullFileName );
    }
    else
    {
      DisplayInfoMessage( NULL, _( "Project template file <kicad.pro> not found " ) );
      return;
    }

    /* Init schematic filename */
    m_SchematicRootFileName = wxFileName( newProjectName.GetName(),
                                          SchematicFileExtension ).GetFullName();

    /* Init pcb board filename */
    m_BoardFileName = wxFileName( newProjectName.GetName(),
                                  BoardFileExtension ).GetFullName();

    /* Init project filename */
    m_ProjectFileName = newProjectName;

    /* Write settings to project file */
    wxGetApp().WriteProjectConfig( PrjFullFileName, GeneralGroupName, NULL );
}

/**
 * Loading a new project
 */
void WinEDA_MainFrame::OnLoadProject( wxCommandEvent& event )
{
    int style;
    wxString title;

    if( event.GetId() != wxID_ANY )
    {
        if( event.GetId() == ID_NEW_PROJECT )
        {
            title = _( "Create New Project" );
            style = wxFD_SAVE | wxFD_OVERWRITE_PROMPT;
        }
        else
        {
            title = _( "Open Existing Project" );
            style = wxFD_OPEN | wxFD_FILE_MUST_EXIST;
        }

        SetLastProject( m_ProjectFileName.GetFullPath() );
        wxFileDialog dlg( this, title, wxGetCwd(), wxEmptyString,
                          ProjectFileWildcard, style );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        m_ProjectFileName = dlg.GetPath();

        if( event.GetId() == ID_NEW_PROJECT )
            CreateNewProject( m_ProjectFileName.GetFullPath() );

        SetLastProject( m_ProjectFileName.GetFullPath() );
    }

    wxLogDebug( wxT( "Loading Kicad project file: " ) +
                m_ProjectFileName.GetFullPath() );

    /* Check if project file exists and if it is not noname.pro */
    wxString filename = m_ProjectFileName.GetFullName();

    if( !m_ProjectFileName.FileExists() && !filename.IsSameAs(wxT("noname.pro")))
    {
        DisplayError( this, _( "Kicad project file <" ) +
                      m_ProjectFileName.GetFullPath() + _( "> not found" ) );
        return;
    }

    wxSetWorkingDirectory( m_ProjectFileName.GetPath() );
    wxGetApp().ReadProjectConfig( m_ProjectFileName.GetFullPath(),
                                  GeneralGroupName, NULL, false );

    SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
              wxT( " " ) +  m_ProjectFileName.GetFullPath() );
    SetLastProject( m_ProjectFileName.GetFullPath() );
    m_LeftWin->ReCreateTreePrj();

    PrintMsg( _( "Working dir: " ) + m_ProjectFileName.GetPath() +
              _( "\nProject: " ) + m_ProjectFileName.GetFullName() +
              wxT( "\n" ) );

#ifdef KICAD_PYTHON
    PyHandler::GetInstance()->TriggerEvent( wxT( "kicad::LoadProject" ),
                                            PyHandler::Convert( m_ProjectFileName.GetFullPath() ) );
#endif
}


/**
 * Save the project top level configuration parameters.
 */
void WinEDA_MainFrame::OnSaveProject( wxCommandEvent& event )
{
    wxString fn;

    wxFileDialog dlg( this, _( "Save Project File" ), wxGetCwd(),
                      m_ProjectFileName.GetFullName(), ProjectFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    m_ProjectFileName = dlg.GetPath();

    wxGetApp().WriteProjectConfig( m_ProjectFileName.GetFullPath(),
                                   GeneralGroupName, NULL );
}


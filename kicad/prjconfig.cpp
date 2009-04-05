/*************************************************************/
/** prjconfig.cpp : load and save configuration (file *.pro) */
/*************************************************************/


#ifdef KICAD_PYTHON
#include <pyhandler.h>
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "prjconfig.h"
#include "id.h"
#include "kicad.h"

static const wxString GeneralGroupName( wxT( "/general" ) );

/* Kicad project file entry namse. */
static const wxString SchematicRootNameEntry( wxT( "RootSch" ) );
static const wxString BoardFileNameEntry( wxT( "BoardNm" ) );


void WinEDA_MainFrame::CreateNewProject( const wxString PrjFullFileName )
{
    wxFileName fn;
    wxFileName newProjectName = PrjFullFileName;

    // Init default config filename
    fn.SetPath( ReturnKicadDatasPath() );
    fn.AppendDir( wxT( "template" ) );
    fn.SetName( wxT( "kicad" ) );
    fn.SetExt( ProjectFileExtension );

    if( !fn.FileExists() )
    {
        DisplayInfo( NULL, _( "Template file not found " ) + fn.GetFullPath() );
        return;
    }
    else
    {
        wxCopyFile( fn.GetFullPath(), PrjFullFileName );
    }

    m_SchematicRootFileName = wxFileName( newProjectName.GetName(),
                                          SchematicFileExtension ).GetFullName();

    m_BoardFileName = wxFileName( newProjectName.GetName(),
                                  BoardFileExtension ).GetFullName();

    m_ProjectFileName = newProjectName;
    wxGetApp().WriteProjectConfig( PrjFullFileName, GeneralGroupName, NULL );
}


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

    if( !m_ProjectFileName.FileExists() )
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

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( m_ProjectFileName.GetFullPath(),
                                   GeneralGroupName, NULL );
}

/*************/
/** cfg.cpp **/
/*************/

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "gestfich.h"
#include "param_config.h"
#include "cvpcb.h"
#include "cvpcb_mainframe.h"


#define GROUP wxT("/cvpcb")
#define GROUPLIB wxT("/pcbnew/libraries")
#define GROUPEQU wxT("/cvpcb/libraries")


/**
 * Return project file parameter list for CVPcb.
 *
 * Populate the project file parameter array specific to CVPcb if it hasn't
 * already been populated and return a reference to the array to the caller.
 * Creating the parameter list at run time has the advantage of being able
 * to define local variables.  The old method of statically building the array
 * at compile time requiring global variable definitions.
 */
PARAM_CFG_ARRAY& CVPCB_MAINFRAME::GetProjectFileParameters( void )
{
    if( !m_projectFileParams.empty() )
        return m_projectFileParams;

    m_projectFileParams.push_back( new PARAM_CFG_BASE( GROUPLIB,
                                                       PARAM_COMMAND_ERASE ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "LibName" ),
                                                               &m_ModuleLibNames,
                                                               GROUPLIB ) );
    m_projectFileParams.push_back( new PARAM_CFG_LIBNAME_LIST( wxT( "EquName" ),
                                                               &m_AliasLibNames,
                                                               GROUPEQU ) );
    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "NetIExt" ),
                                                           &m_NetlistFileExtension ) );
    m_projectFileParams.push_back( new PARAM_CFG_FILENAME( wxT( "LibDir" ),
                                                           &m_UserLibraryPath,
                                                           GROUPLIB ) );

    return m_projectFileParams;
}


/**
 * Reads the configuration
 * 1 - bed cvpcb.cnf
 * 2 - if not in path of  <cvpcb.exe> / cvpcb.cnf
 * 3 - If not found: init variables to default values
 *
 * Note:
 * The path of the executable must be in cvpcb.exe.
 *
 */
void CVPCB_MAINFRAME::LoadProjectFile( const wxString& FileName )
{
    wxFileName fn = FileName;

    m_ModuleLibNames.Clear();
    m_AliasLibNames.Clear();

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( m_UserLibraryPath );

    wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP,
                                  GetProjectFileParameters(), FALSE );

    if( m_NetlistFileExtension.IsEmpty() )
        m_NetlistFileExtension = wxT( "net" );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( m_UserLibraryPath, 1 );
}


void CVPCB_MAINFRAME::Update_Config( wxCommandEvent& event )
{
    SaveProjectFile( m_NetlistFileName.GetFullPath() );
}


void CVPCB_MAINFRAME::SaveProjectFile( const wxString& fileName )
{
    wxFileName fn = fileName;

    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP,
                                   GetProjectFileParameters() );
}

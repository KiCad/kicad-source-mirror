/***************************************/
/** cfg.cpp : configuration de CVPCB  **/
/***************************************/

/* lit ou met a jour la configuration de CVPCB */

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "gestfich.h"
#include "param_config.h"
#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


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
PARAM_CFG_ARRAY& WinEDA_CvpcbFrame::GetProjectFileParameters( void )
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
    m_projectFileParams.push_back( new PARAM_CFG_WXSTRING( wxT( "LibDir" ),
                                                           &m_UserLibraryPath,
                                                           GROUPLIB ) );

    return m_projectFileParams;
}


/**
 * lit la configuration
 * 1 - lit cvpcb.cnf
 * 2 - si non trouve lit <chemin de cvpcb.exe>/cvpcb.cnf
 * 3 - si non trouve: init des variables aux valeurs par defaut
 *
 * Remarque:
 * le chemin de l'executable cvpcb.exe doit etre dans BinDir
 *
 */
void WinEDA_CvpcbFrame::LoadProjectFile( const wxString& FileName )
{
    wxFileName fn = FileName;

    /* Init des valeurs par defaut */
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


/* fonction relai d'appel a Save_Config,
 * la vraie fonction de sauvegarde de la config
 */
void WinEDA_CvpcbFrame::Update_Config( wxCommandEvent& event )
{
    SaveProjectFile( m_NetlistFileName.GetFullPath() );
}


void WinEDA_CvpcbFrame::SaveProjectFile( const wxString& fileName )
{
    wxFileName fn = fileName;

    fn.SetExt( ProjectFileExtension );

    wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                      fn.GetFullName(), ProjectFileWildcard, wxFD_SAVE );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    /* ecriture de la configuration */
    wxGetApp().WriteProjectConfig( dlg.GetPath(), GROUP,
                                   GetProjectFileParameters() );
}

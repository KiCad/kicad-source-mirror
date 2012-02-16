/*************/
/** cfg.cpp **/
/*************/

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <id.h>
#include <common.h>
#include <gestfich.h>
#include <param_config.h>
#include <cvpcb.h>
#include <cvpcb_mainframe.h>
#include <wildcards_and_files_ext.h>


#define GROUP wxT("/cvpcb")
#define GROUPLIB wxT("/pcbnew/libraries")
#define GROUPEQU wxT("/cvpcb/libraries")


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


void CVPCB_MAINFRAME::LoadProjectFile( const wxString& aFileName )
{
    wxFileName fn = aFileName;

    m_ModuleLibNames.Clear();
    m_AliasLibNames.Clear();

    if( fn.GetExt() != ProjectFileExtension )
        fn.SetExt( ProjectFileExtension );

    wxGetApp().RemoveLibraryPath( m_UserLibraryPath );

    wxGetApp().ReadProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters(), false );

    if( m_NetlistFileExtension.IsEmpty() )
        m_NetlistFileExtension = wxT( "net" );

    /* User library path takes precedent over default library search paths. */
    wxGetApp().InsertLibraryPath( m_UserLibraryPath, 1 );
}


void CVPCB_MAINFRAME::SaveProjectFile( wxCommandEvent& aEvent )
{
    wxFileName fn = m_NetlistFileName;

    fn.SetExt( ProjectFileExtension );

    if( aEvent.GetId() == ID_SAVE_PROJECT_AS || !m_NetlistFileName.IsOk() )
    {
        wxFileDialog dlg( this, _( "Save Project File" ), fn.GetPath(),
                          wxEmptyString, ProjectFileWildcard, wxFD_SAVE );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();

        if( !fn.HasExt() )
            fn.SetExt( ProjectFileExtension );
    }

    if( !IsWritable( fn ) )
        return;

    wxGetApp().WriteProjectConfig( fn.GetFullPath(), GROUP, GetProjectFileParameters() );
}

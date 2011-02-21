/**************/
/*  init.cpp  */
/**************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gr_basic.h"
#include "gestfich.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "cvpcb_mainframe.h"
#include "cvstruct.h"

#include "build_version.h"

/*
 * Set the module to the selected component
 * Selects the next component
 */
void CVPCB_MAINFRAME::SetNewPkg( const wxString& package )
{
    COMPONENT* Component;
    bool       isUndefined = false;
    int        NumCmp;
    wxString   msg;

    if( m_components.empty() )
        return;

    NumCmp = m_ListCmp->GetSelection();
    if( NumCmp < 0 )
    {
        NumCmp = 0;
        m_ListCmp->SetSelection( NumCmp, true );
    }

    Component = &m_components[ NumCmp ];

    if( Component == NULL )
        return;

    isUndefined = Component->m_Module.IsEmpty();

    Component->m_Module = package;

    msg.Printf( CMP_FORMAT, NumCmp + 1,
                 GetChars( Component->m_Reference ),
                 GetChars( Component->m_Value ),
                 GetChars( Component->m_Module ) );
    m_modified = true;

    if( isUndefined )
        m_undefinedComponentCnt -= 1;

    m_ListCmp->SetString( NumCmp, msg );
    m_ListCmp->SetSelection( NumCmp, FALSE );

    // We activate next component:
    if( NumCmp < (m_ListCmp->GetCount() - 1) )
        NumCmp++;
    m_ListCmp->SetSelection( NumCmp, TRUE );

    DisplayStatus();
}


/*
 * Read the netlist format and file components.
 */
bool CVPCB_MAINFRAME::ReadNetList()
{
    wxString   msg;
    int        error_level;

    error_level = ReadSchematicNetlist();

    if( error_level < 0 )
    {
        msg.Printf( _( "File <%s> does not appear to be a valid Kicad net list file." ),
                    GetChars( m_NetlistFileName.GetFullPath() ) );
        ::wxMessageBox( msg, _( "File Error" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    LoadComponentFile( m_NetlistFileName.GetFullPath() );

    if( m_ListCmp == NULL )
        return false;

    LoadProjectFile( m_NetlistFileName.GetFullPath() );
    LoadFootprintFiles( );
    BuildFOOTPRINTS_LISTBOX();

    m_ListCmp->Clear();
    m_undefinedComponentCnt = 0;

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    GetChars( component.m_Reference ),
                    GetChars( component.m_Value ),
                    GetChars( component.m_Module ) );
        m_ListCmp->AppendLine( msg );
        if( component.m_Module.IsEmpty() )
            m_undefinedComponentCnt += 1;
    }

    if( !m_components.empty() )
        m_ListCmp->SetSelection( 0, TRUE );

    DisplayStatus();

    /* Update the title of the main window. */
    SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
              wxT( " " ) + m_NetlistFileName.GetFullPath() );
    return true;
}


/*
 * Backup and NetList cmp
 * The full name of the netlist file must be in FFileName.
 * The file name is deducted in cmp
 */
int CVPCB_MAINFRAME::SaveNetList( const wxString& fileName )
{
    wxFileName fn;

    if( !fileName && m_NetlistFileName.IsOk() )
        fn = m_NetlistFileName;
    else
        fn = wxFileName( wxGetCwd(), _( "unamed" ), NetExtBuffer );

    wxFileDialog dlg( this, _( "Save Net and Component List" ), fn.GetPath(),
                      fn.GetFullName(), NetlistFileWildcard,
                      wxFD_SAVE/*| wxFD_OVERWRITE_PROMPT*/ );

    if( dlg.ShowModal() == wxID_CANCEL )
        return -1;

    if( SaveComponentList( dlg.GetPath() ) == 0 )
    {
        DisplayError( this, _( "Unable to create component file (.cmp)" ) );
        return 0;
    }

    FILE* netlist = wxFopen( dlg.GetPath(), wxT( "wt" ) );

    if( netlist == 0 )
    {
        DisplayError( this, _( "Unable to create net list file" ) );
        return 0;
    }

    GenNetlistPcbnew( netlist, m_isEESchemaNetlist );

    return 1;
}

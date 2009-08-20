/*******************************************************************/
/*  CVPCB: Routines de base :										  */
/* lecture Netliste et creation des fenetres composants et modules */
/*******************************************************************/

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gr_basic.h"
#include "gestfich.h"
#include "id.h"
#include "appl_wxstruct.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"


/* routines locales : */


/*
 *   - Affecte un module au composant selectionne
 *   - Selectionne le composant suivant
 */
void WinEDA_CvpcbFrame::SetNewPkg( const wxString& package )
{
    COMPONENT* Component;
    bool       isUndefined = false;
    int        NumCmp;
    wxString   Line;

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

    Line.Printf( CMP_FORMAT, NumCmp + 1,
                 Component->m_Reference.GetData(), Component->m_Value.GetData(),
                 Component->m_Module.GetData() );
    m_modified = true;

    if( isUndefined )
        m_undefinedComponentCnt -= 1;

    m_ListCmp->SetString( NumCmp, Line );
    m_ListCmp->SetSelection( NumCmp, FALSE );

    // We activate next component:
    if( NumCmp < (m_ListCmp->GetCount() - 1) )
        NumCmp++;
    m_ListCmp->SetSelection( NumCmp, TRUE );

    Line.Printf( _( "Components: %d (free: %d)" ),
                 m_components.size(), m_undefinedComponentCnt );
    SetStatusText( Line, 1 );
}



/*
 * Lecture de la netliste selon format, ainsi que du fichier des composants
 */
bool WinEDA_CvpcbFrame::ReadNetList()
{
    wxString   msg;
    int        error_level;

    error_level = ReadSchematicNetlist();

    if( error_level < 0 )
    {
        msg.Printf( _( "File <%s> does not appear to be a valid Kicad net list file." ),
                    m_NetlistFileName.GetFullPath().c_str() );
        ::wxMessageBox( msg, _( "File Error" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    /* lecture des correspondances */
    LoadComponentFile( m_NetlistFileName.GetFullPath(), m_components );

    if( m_ListCmp == NULL )
        return false;

    LoadProjectFile( m_NetlistFileName.GetFullPath() );
    LoadFootprintFiles( m_ModuleLibNames, m_footprints );
    BuildFOOTPRINTS_LISTBOX();

    m_ListCmp->Clear();
    m_undefinedComponentCnt = 0;

    BOOST_FOREACH( COMPONENT& component, m_components )
    {
        msg.Printf( CMP_FORMAT, m_ListCmp->GetCount() + 1,
                    component.m_Reference.GetData(),
                    component.m_Value.GetData(),
                    component.m_Module.GetData() );
        m_ListCmp->AppendLine( msg );
        if( component.m_Module.IsEmpty() )
            m_undefinedComponentCnt += 1;
    }

    if( !m_components.empty() )
        m_ListCmp->SetSelection( 0, TRUE );

    msg.Printf( _( "Components: %d (free: %d)" ), m_components.size(),
                m_undefinedComponentCnt );
    SetStatusText( msg, 1 );

    /* Mise a jour du titre de la fenetre principale */
    SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
              wxT( " " ) + m_NetlistFileName.GetFullPath() );
    return true;
}


/*
 * Sauvegarde des fichiers netliste et cmp
 *   Le nom complet du fichier Netliste doit etre dans FFileName.
 *   Le nom du fichier cmp en est deduit
 */
int WinEDA_CvpcbFrame::SaveNetList( const wxString& fileName )
{
    wxFileName fn;

    if( !fileName && m_NetlistFileName.IsOk() )
        fn = m_NetlistFileName;
    else
        fn = wxFileName( wxGetCwd(), _( "unamed" ), NetExtBuffer );

    wxFileDialog dlg( this, _( "Save Net and Component List" ), fn.GetPath(),
                      fn.GetFullName(), NetlistFileWildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

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
        DisplayError( this, _( "Unable to create netlist file" ) );
        return 0;
    }

    GenNetlistPcbnew( netlist, m_components, m_isEESchemaNetlist,
                      m_rightJustify );

    return 1;
}

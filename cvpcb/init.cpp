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


/**********************************************************/
void WinEDA_CvpcbFrame::SetNewPkg( const wxString& package )
/*********************************************************/

/*
 *   - Affecte un module au composant selectionne
 *   - Selectionne le composant suivant
 */
{
    STORECMP* Composant;
    int       ii, NumCmp, IsNew = 1;
    wxString  Line;

    if( g_BaseListeCmp == NULL )
        return;

    NumCmp = m_ListCmp->GetSelection();
    if( NumCmp < 0 )
    {
        NumCmp = 0;
        m_ListCmp->SetSelection( NumCmp, TRUE );
    }

    Composant = g_BaseListeCmp;
    for( ii = 0; Composant != NULL; Composant = Composant->Pnext, ii++ )
    {
        if( NumCmp == ii )
            break;
    }

    if( Composant == NULL )
        return;
    if( !Composant->m_Module.IsEmpty() )
        IsNew = 0;

    Composant->m_Module = package;

    Line.Printf( CMP_FORMAT, ii + 1,
                 Composant->m_Reference.GetData(), Composant->m_Valeur.GetData(),
                 Composant->m_Module.GetData() );
    modified = 1;
    if( IsNew )
        composants_non_affectes -= 1;

    m_ListCmp->SetString( NumCmp, Line );
    m_ListCmp->SetSelection( NumCmp, FALSE );

    // We activate next component:
    if( NumCmp < (m_ListCmp->GetCount() - 1) )
        NumCmp++;
    m_ListCmp->SetSelection( NumCmp, TRUE );

    Line.Printf( _( "Components: %d (free: %d)" ),
                 nbcomp, composants_non_affectes );
    SetStatusText( Line, 1 );
}


/********************************************/
bool WinEDA_CvpcbFrame::ReadNetList()
/*******************************************/

/* Lecture de la netliste selon format, ainsi que du fichier des composants
 */
{
    STORECMP* Composant;
    wxString  msg;
    int       ii;
    int       error_level;

    error_level = ReadSchematicNetlist();

    if( error_level < 0 )
    {
        msg.Printf( _( "File <%s> does not appear to be a valid Kicad " \
                       "net list file." ),
                    m_NetlistFileName.GetFullPath().c_str() );
        ::wxMessageBox( msg, _( "File Error" ), wxOK | wxICON_ERROR, this );
        return false;
    }

    /* lecture des correspondances */
    loadcmp( m_NetlistFileName.GetFullPath() );

    if( m_ListCmp == NULL )
        return false;

    Read_Config( m_NetlistFileName.GetFullPath() );

    listlib();
    BuildFootprintListBox();

    m_ListCmp->Clear();
    Composant = g_BaseListeCmp;

    composants_non_affectes = 0;
    for( ii = 1; Composant != NULL; Composant = Composant->Pnext, ii++ )
    {
        msg.Printf( CMP_FORMAT, ii,
                    Composant->m_Reference.GetData(),
                    Composant->m_Valeur.GetData(),
                    Composant->m_Module.GetData() );
        m_ListCmp->AppendLine( msg );
        if( Composant->m_Module.IsEmpty() )
            composants_non_affectes += 1;
    }

    if( g_BaseListeCmp )
        m_ListCmp->SetSelection( 0, TRUE );

    msg.Printf( _( "Components: %d (free: %d)" ), nbcomp,
                composants_non_affectes );
    SetStatusText( msg, 1 );

    /* Mise a jour du titre de la fenetre principale */
    SetTitle( wxGetApp().GetTitle() + wxT( " " ) + GetBuildVersion() +
              wxT( " " ) + m_NetlistFileName.GetFullPath() );
    return true;
}


/*****************************************************************/
int WinEDA_CvpcbFrame::SaveNetList( const wxString& fileName )
/*****************************************************************/

/* Sauvegarde des fichiers netliste et cmp
 *   Le nom complet du fichier Netliste doit etre dans FFileName.
 *   Le nom du fichier cmp en est deduit
 */
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

    GenNetlistPcbnew( netlist );

    return 1;
}

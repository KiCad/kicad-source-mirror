/**********************/
/* CVPCB: autosel.cpp  */
/**********************/

/* Routines de selection automatique des modules */

#include "fctsys.h"
#include "common.h"
#include "confirm.h"
#include "gestfich.h"
#include "appl_wxstruct.h"
#include "kicad_string.h"

#include "cvpcb.h"
#include "protos.h"
#include "cvstruct.h"

#define QUOTE '\''

class AUTOMODULE
{
public:
    int         m_Type;
    AUTOMODULE* Pnext;
    wxString    m_Name;
    wxString    m_LibName;

    AUTOMODULE() { m_Type = 0; Pnext = NULL; }
};


/* routines locales : */
static int  auto_select( WinEDA_CvpcbFrame* frame,
                         STORECMP*          Cmp,
                         AUTOMODULE*        BaseListeMod );
static char * ReadQuotedText(wxString & aTarget, char * aText);

/*************************************************************/
void WinEDA_CvpcbFrame::AssocieModule( wxCommandEvent& event )
/*************************************************************/

/* Called by the automatic association button
 *  Read *.equ files to try to find acorresponding footprint
 * for each component that is not already linked to a footprint ( a "free" component )
 * format of a line:
 * 'cmp_ref' 'footprint_name'
 */
{
    wxFileName  fn;
    wxString    msg, tmp;
    char        Line[1024];
    FILE*       file;
    AUTOMODULE* ItemModule, * NextMod;
    AUTOMODULE* BaseListeMod = NULL;
    STORECMP*   Component;
    int         nb_correspondances = 0;

    if( nbcomp <= 0 )
        return;

    /* recherche des equivalences a travers les fichiers possibles */
    for( unsigned ii = 0; ii < g_ListName_Equ.GetCount(); ii++ )
    {
        fn = g_ListName_Equ[ii];
        fn.SetExt( EquivFileExtension );

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            msg.Printf( _( "Footprint alias library file <%s> could not be " \
                           "found in the default search paths." ),
                        fn.GetFullName().c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Error opening alias library <%s>." ), tmp.c_str() );
            wxMessageBox( msg, titleLibLoadError, wxOK | wxICON_ERROR );
            continue;
        }

        /* lecture fichier n */
        while( GetLine( file, Line, NULL,  sizeof(Line) ) != NULL )
        {
            char * text = Line;
            text = ReadQuotedText(tmp, text);

            if ( text == NULL || (*text == 0 )  )
                continue;

            ItemModule = new AUTOMODULE();
            ItemModule->Pnext = BaseListeMod;
            BaseListeMod = ItemModule;

            /* stockage du composant ( 'namecmp'  'namelib')
             *  name et namelib */
            ItemModule->m_Name = tmp;

            text++;
            ReadQuotedText(ItemModule->m_LibName, text);

            nb_correspondances++;
        }

        fclose( file );
    }


    /* display some info */
    msg.Printf( _( "%d equivalences" ), nb_correspondances );
    SetStatusText( msg, 0 );
    wxMessageBox(msg);

    Component = g_BaseListeCmp;
    for( unsigned ii = 0; Component != NULL; Component = Component->Pnext, ii++ )
    {
        m_ListCmp->SetSelection( ii, TRUE );
        if( Component->m_Module.IsEmpty() )
            auto_select( this, Component, BaseListeMod );
    }

    /* free memory: */
    for( ItemModule = BaseListeMod; ItemModule != NULL; ItemModule = NextMod )
    {
        NextMod = ItemModule->Pnext;
        delete ItemModule;
    }

    BaseListeMod = NULL;
}

/***************************************************/
char * ReadQuotedText(wxString & aTarget, char * aText)
/***************************************************/
/** read the string between quotes and put it in aTarget
 * put text in aTarget
 * return a pointer to the last read char (the second quote if Ok)
*/
{
    // search the first quote:
    for( ; *aText != 0; aText++ )
    {
        if( *aText == QUOTE )
            break;
    }

    if ( *aText == 0 )
        return NULL;

    aText++;
    for(; *aText != 0; aText++ )
    {
        if( *aText == QUOTE )
            break;
        aTarget.Append(*aText);
    }

    return aText;
}


/****************************************************************/
int auto_select( WinEDA_CvpcbFrame* frame, STORECMP* Cmp,
                        AUTOMODULE* BaseListeMod )
/****************************************************************/

/* associe automatiquement composant et Module
 *   Retourne;
 *       0 si OK
 *       1 si module specifie non trouve en liste librairie
 *       2 si pas de module specifie dans la liste des equivalences
 */
{
    AUTOMODULE* ItemModule;
    STOREMOD*   Module;
    wxString    msg;

    /* examen de la liste des correspondances */
    ItemModule = BaseListeMod;
    for( ; ItemModule != NULL; ItemModule = ItemModule->Pnext )
    {
        if( ItemModule->m_Name.CmpNoCase( Cmp->m_Valeur ) != 0 )
            continue;

        /* Correspondance trouvee, recherche nom module dans la liste des
         *  modules disponibles en librairie */
        Module = g_BaseListePkg;
        for( ; Module != NULL; Module = Module->Pnext )
        {
            if( ItemModule->m_LibName.CmpNoCase( Module->m_Module ) == 0 )
            {
                frame->SetNewPkg( Module->m_Module );
                return 0;
            }
        }

        msg.Printf( _( "Component %s: Footprint %s not found in libraries" ),
                    Cmp->m_Valeur.GetData(), ItemModule->m_LibName.GetData() );
        DisplayError( frame, msg, 10 );
        return 2;
    }

    return 1;
}

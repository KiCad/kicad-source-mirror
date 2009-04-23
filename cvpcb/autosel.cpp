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


/*
 * read the string between quotes and put it in aTarget
 * put text in aTarget
 * return a pointer to the last read char (the second quote if Ok)
 */
char * ReadQuotedText(wxString & aTarget, char * aText)
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


/*
 * Called by the automatic association button
 * Read *.equ files to try to find corresponding footprint
 * for each component that is not already linked to a footprint ( a "free"
 * component )
 * format of a line:
 * 'cmp_ref' 'footprint_name'
 */
void WinEDA_CvpcbFrame::AssocieModule( wxCommandEvent& event )
{
    COMPONENT_LIST::iterator iCmp;
    FOOTPRINT_LIST::iterator iFp;
    wxFileName  fn;
    wxString    msg, tmp;
    char        Line[1024];
    FILE*       file;
    AUTOMODULE* ItemModule, * NextMod;
    AUTOMODULE* BaseListeMod = NULL;
    COMPONENT*  Component;
    FOOTPRINT*  footprint;
    size_t      ii;
    int         nb_correspondances = 0;

    if( m_components.empty() )
        return;

    /* recherche des equivalences a travers les fichiers possibles */
    for( ii = 0; ii < m_AliasLibNames.GetCount(); ii++ )
    {
        fn = m_AliasLibNames[ii];
        fn.SetExt( FootprintAliasFileExtension );

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

    for( iCmp = m_components.begin(); iCmp != m_components.end(); ++iCmp )
    {
        Component = *iCmp;
        m_ListCmp->SetSelection( m_components.IndexOf( Component ), TRUE );

        if( Component->m_Module.IsEmpty() )
        {
            ItemModule = BaseListeMod;
            for( ; ItemModule != NULL; ItemModule = ItemModule->Pnext )
            {
                if( ItemModule->m_Name.CmpNoCase( Component->m_Valeur ) != 0 )
                    continue;

                for( iFp = m_footprints.begin(); iFp != m_footprints.end(); ++iFp )
                {
                    footprint = *iFp;

                    if( ItemModule->m_LibName.CmpNoCase( footprint->m_Module ) == 0 )
                    {
                        SetNewPkg( footprint->m_Module );
                        break;
                    }
                }

                msg.Printf( _( "Component %s: Footprint %s not found in " \
                               "libraries" ), Component->m_Valeur.GetData(),
                            ItemModule->m_LibName.GetData() );
                DisplayError( this, msg, 10 );
            }
        }
    }

    /* free memory: */
    for( ItemModule = BaseListeMod; ItemModule != NULL; ItemModule = NextMod )
    {
        NextMod = ItemModule->Pnext;
        delete ItemModule;
    }

    BaseListeMod = NULL;
}

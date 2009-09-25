/****************************/
/*  EESchema - database.cpp */
/****************************/

/* Routine de selection d'un composant en librairie
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "eda_doc.h"
#include "kicad_string.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "class_library.h"

#include <boost/foreach.hpp>


/*
 *   Routine de selection du nom d'un composant en librairie pour chargement,
 *   Keys pointe la liste des mots cles de filtrage
 *   Si Keys = "", recherche des composants qui correspondent
 *  au masque BufName( avec * et ? )
 *
 *  Retourne
 *   TRUE si composant selectionne
 *   FALSE si commande annulee
 *   place le nom du composant a charger, selectionne a partir d'une liste dans
 *   BufName
 */
wxString DataBaseGetName( WinEDA_DrawFrame* frame, wxString& Keys,
                          wxString& BufName )
{
    wxArrayString  nameList;
    wxString       msg;

    BufName.MakeUpper();
    Keys.MakeUpper();

    /* Examen de la liste des librairies pour comptage */
    BOOST_FOREACH( CMP_LIBRARY& lib, CMP_LIBRARY::GetLibraryList() )
    {
        lib.SearchEntryNames( nameList, BufName, Keys );
    }

    if( nameList.IsEmpty() )
    {
        msg = _( "No components found matching " );
        if( !BufName.IsEmpty() )
        {
            msg += _( "name search criteria <" ) + BufName + wxT( "> " );
            if( !Keys.IsEmpty() )
                msg += _( "and " );
        }

        if( !Keys.IsEmpty() )
            msg += _( "key search criteria <" ) + Keys + wxT( "> " );

        DisplayError( frame, msg );

        return wxEmptyString;
    }

    wxSingleChoiceDialog dlg( frame, wxEmptyString, _( "Select Component" ),
                              nameList );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetStringSelection().IsEmpty() )
        return wxEmptyString;

    return dlg.GetStringSelection();
}


void DisplayCmpDoc( wxString& Name )
{
    CMP_LIB_ENTRY* CmpEntry = NULL;

    CmpEntry = CMP_LIBRARY::FindLibraryEntry( Name );

    if( CmpEntry == NULL )
        return;

    wxLogDebug( wxT( "Selected component <%s>, m_Doc: <%s>, m_KeyWord: <%s>." ),
                (const wxChar*) Name, (const wxChar*) CmpEntry->m_Doc,
                (const wxChar*) CmpEntry->m_KeyWord );

    Name  = wxT( "Description: " ) + CmpEntry->m_Doc;
    Name += wxT( "\nKey Words: " ) + CmpEntry->m_KeyWord;
}

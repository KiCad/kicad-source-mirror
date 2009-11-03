/****************************/
/*  EESchema - database.cpp */
/****************************/

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
 * Routine name selection of a component library for loading,
 * Keys leading the list of the keywords filter
 * If Keys = "", research components that correspond
 * BufName mask (with * and?)
 *
 * Returns
 * TRUE if the selected component
 * FALSE canceled order
 * Place the name of the component has loaded, select from a list in
 * BufName
 */
wxString DataBaseGetName( WinEDA_DrawFrame* frame, wxString& Keys,
                          wxString& BufName )
{
    wxArrayString  nameList;
    wxString       msg;

    BufName.MakeUpper();
    Keys.MakeUpper();

    /* Review the list of libraries for counting. */
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
                GetChars( Name ), GetChars( CmpEntry->m_Doc ),
                GetChars( CmpEntry->m_KeyWord ) );

    Name  = wxT( "Description: " ) + CmpEntry->m_Doc;
    Name += wxT( "\nKey Words: " ) + CmpEntry->m_KeyWord;
}

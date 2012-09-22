/**
 * @file database.cpp
 */

#include "fctsys.h"
#include "confirm.h"
#include "eda_doc.h"
#include "kicad_string.h"
#include "wxstruct.h"

#include "protos.h"
#include "class_library.h"
#include "dialog_helpers.h"

#include <boost/foreach.hpp>

extern void DisplayCmpDocAndKeywords( wxString& Name );

// Used in DataBaseGetName: this is a callback function for EDA_LIST_DIALOG
// to display keywords and description of a component
void DisplayCmpDocAndKeywords( wxString& Name )
{
    LIB_ALIAS* CmpEntry = NULL;

    CmpEntry = CMP_LIBRARY::FindLibraryEntry( Name );

    if( CmpEntry == NULL )
        return;

    Name  = wxT( "Description: " ) + CmpEntry->GetDescription();
    Name += wxT( "\nKey Words: " ) + CmpEntry->GetKeyWords();
}

/*
 * Displays a list of filterd components found in libraries for selection,
 * Keys is a list of keywords to filter components which do not match these keywords
 * If Keys is empty, list components that match BufName mask (with * and?)
 *
 * Returns the name of the selected component, or an ampty string
 */
wxString DataBaseGetName( EDA_DRAW_FRAME* frame, wxString& Keys, wxString& BufName )
{
    wxArrayString  nameList;
    wxString       msg;

#ifndef KICAD_KEEPCASE
    BufName.MakeUpper();
#endif
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

        DisplayInfoMessage( frame, msg );

        return wxEmptyString;
    }

    // Show candidate list:
    wxString cmpname;
    EDA_LIST_DIALOG dlg( frame, _( "Select Component" ), nameList, cmpname,
                         DisplayCmpDocAndKeywords );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    cmpname = dlg.GetTextSelection();
    return cmpname;
}

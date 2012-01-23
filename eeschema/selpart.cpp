/**
 * @file selpart.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <confirm.h>
#include <wxstruct.h>

#include <general.h>
#include <protos.h>
#include <class_library.h>
#include <dialog_helpers.h>


CMP_LIBRARY* SelectLibraryFromList( EDA_DRAW_FRAME* frame )
{
    static wxString OldLibName;
    wxString        msg;
    wxArrayString   libNamesList;
    int             count = CMP_LIBRARY::GetLibraryCount();
    CMP_LIBRARY*    Lib = NULL;

    if( count == 0 )
    {
        DisplayError( frame, _( "No component libraries are loaded." ) );
        return NULL;
    }

    libNamesList = CMP_LIBRARY::GetLibraryNames();

    msg.Printf( _( " Select 1 of %d libraries." ), count );

    wxSingleChoiceDialog dlg( frame, msg, _( "Select Library" ), libNamesList );

    int index = libNamesList.Index( OldLibName );

    if( index != wxNOT_FOUND )
        dlg.SetSelection( index );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetStringSelection().IsEmpty() )
        return NULL;

    Lib = CMP_LIBRARY::FindLibrary( dlg.GetStringSelection() );

    if( Lib != NULL )
        OldLibName = dlg.GetStringSelection();

    return Lib;
}


int DisplayComponentsNamesInLib( EDA_DRAW_FRAME* frame,
                                 CMP_LIBRARY* Library,
                                 wxString& Buffer, wxString& OldName )
{
    wxArrayString  nameList;

    if( Library == NULL )
        Library = SelectLibraryFromList( frame );

    if( Library == NULL )
        return 0;

    Library->GetEntryNames( nameList );

    EDA_LIST_DIALOG dlg( frame, _( "Select Component" ), nameList, OldName, DisplayCmpDoc );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    Buffer = dlg.GetTextSelection();

    return 1;
}


int GetNameOfPartToLoad( EDA_DRAW_FRAME* frame, CMP_LIBRARY* Library, wxString& BufName )
{
    int             ii;
    static wxString OldCmpName;

    ii = DisplayComponentsNamesInLib( frame, Library, BufName, OldCmpName );
    if( ii <= 0 )
        return 0;

    OldCmpName = BufName;
    return 1;
}

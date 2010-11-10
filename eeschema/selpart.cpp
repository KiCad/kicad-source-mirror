/**************************/
/* EESchema - selpart.cpp */
/**************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "confirm.h"
#include "wxstruct.h"

#include "general.h"
#include "protos.h"
#include "class_library.h"


CMP_LIBRARY* SelectLibraryFromList( WinEDA_DrawFrame* frame )
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


int DisplayComponentsNamesInLib( WinEDA_DrawFrame* frame,
                                 CMP_LIBRARY* Library,
                                 wxString& Buffer, wxString& OldName )
{
    size_t         i;
    wxArrayString  nameList;
    const wxChar** ListNames;

    if( Library == NULL )
        Library = SelectLibraryFromList( frame );

    if( Library == NULL )
        return 0;

    Library->GetEntryNames( nameList );

    ListNames = (const wxChar**) MyZMalloc( ( nameList.GetCount() + 1 ) *
                                            sizeof( wxChar* ) );

    if( ListNames == NULL )
        return 0;

    for( i = 0; i < nameList.GetCount(); i++ )
        ListNames[i] = (const wxChar*) nameList[i];

    WinEDAListBox dlg( frame, _( "Select Component" ), ListNames, OldName,
                       DisplayCmpDoc, wxColour( 255, 255, 255 ) );

    if( !OldName.IsEmpty() )
        dlg.m_List->SetStringSelection( OldName );

    dlg.MoveMouseToOrigin();

    int rsp = dlg.ShowModal();

    if( rsp >= 0 )
        Buffer = ListNames[rsp];

    free( ListNames );

    if( rsp < 0 )
        return 0;

    return 1;
}


int GetNameOfPartToLoad( WinEDA_DrawFrame* frame, CMP_LIBRARY* Library,
                         wxString& BufName )
{
    int             ii;
    static wxString OldCmpName;

    ii = DisplayComponentsNamesInLib( frame, Library, BufName, OldCmpName );
    if( ii <= 0 )
        return 0;

    OldCmpName = BufName;
    return 1;
}

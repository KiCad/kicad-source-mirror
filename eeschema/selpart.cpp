/**
 * @file selpart.cpp
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <confirm.h>
#include <draw_frame.h>

#include <general.h>
#include <protos.h>
#include <class_library.h>
#include <dialog_helpers.h>


CMP_LIBRARY* SelectLibraryFromList( EDA_DRAW_FRAME* frame )
{
    static wxString OldLibName;
    wxArrayString   libNamesList;
    CMP_LIBRARY*    Lib = NULL;

    int count = CMP_LIBRARY::GetLibraryCount();
    if( count == 0 )
    {
        DisplayError( frame, _( "No component libraries are loaded." ) );
        return NULL;
    }

    wxArrayString headers;
    headers.Add( wxT("Library") );

    libNamesList = CMP_LIBRARY::GetLibraryNames();
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < libNamesList.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( libNamesList[i] );
        itemsToDisplay.push_back( item );
    }
    EDA_LIST_DIALOG dlg( frame, _( "Select Library" ), headers, itemsToDisplay, OldLibName );

    if( dlg.ShowModal() != wxID_OK )
        return NULL;

    wxString libname = dlg.GetTextSelection();

    if( libname.IsEmpty() )
        return NULL;

    Lib = CMP_LIBRARY::FindLibrary( libname );

    if( Lib != NULL )
        OldLibName = libname;

    return Lib;
}

extern void DisplayCmpDocAndKeywords( wxString& Name );

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

    wxArrayString headers;
    headers.Add( wxT("Component") );
    headers.Add( wxT("Library") );
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < nameList.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( nameList[i] );
        item.Add( Library->GetLogicalName() );
        itemsToDisplay.push_back( item );
    }
    EDA_LIST_DIALOG dlg( frame, _( "Select Component" ), headers, itemsToDisplay,
                         OldName, DisplayCmpDocAndKeywords );

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

    if( ii <= 0 || BufName.IsEmpty() )
        return 0;

    OldCmpName = BufName;
    return 1;
}

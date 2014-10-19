/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

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


PART_LIB* SelectLibraryFromList( EDA_DRAW_FRAME* aFrame )
{
    PROJECT&    prj = aFrame->Prj();

    if( PART_LIBS* libs = prj.SchLibs() )
    {
        if( !libs->GetLibraryCount() )
        {
            DisplayError( aFrame, _( "No component libraries are loaded." ) );
            return NULL;
        }

        wxArrayString headers;

        headers.Add( wxT( "Library" ) );

        wxArrayString   libNamesList = libs->GetLibraryNames();

        std::vector<wxArrayString> itemsToDisplay;

        // Conversion from wxArrayString to vector of ArrayString
        for( unsigned i = 0; i < libNamesList.GetCount(); i++ )
        {
            wxArrayString item;

            item.Add( libNamesList[i] );

            itemsToDisplay.push_back( item );
        }

        wxString old_lib_name = prj.GetRString( PROJECT::SCH_LIB_SELECT );

        EDA_LIST_DIALOG dlg( aFrame, _( "Select Library" ), headers, itemsToDisplay, old_lib_name );

        if( dlg.ShowModal() != wxID_OK )
            return NULL;

        wxString libname = dlg.GetTextSelection();

        if( !libname )
            return NULL;

        PART_LIB* lib = libs->FindLibrary( libname );

        if( lib )
            prj.SetRString( PROJECT::SCH_LIB_SELECT, libname );

        return lib;
    }

    return NULL;
}


void DisplayCmpDocAndKeywords( wxString& aName, void* aData );


int DisplayComponentsNamesInLib( EDA_DRAW_FRAME* frame,
                                 PART_LIB* Library,
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
                         OldName, DisplayCmpDocAndKeywords, frame->Prj().SchLibs() );

    if( dlg.ShowModal() != wxID_OK )
        return 0;

    Buffer = dlg.GetTextSelection();

    return 1;
}


int GetNameOfPartToLoad( EDA_DRAW_FRAME* frame, PART_LIB* Library, wxString& BufName )
{
    int             ii;
    static wxString OldCmpName;

    ii = DisplayComponentsNamesInLib( frame, Library, BufName, OldCmpName );

    if( ii <= 0 || BufName.IsEmpty() )
        return 0;

    OldCmpName = BufName;
    return 1;
}

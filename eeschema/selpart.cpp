/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <pgm_base.h>
#include <sch_base_frame.h>

#include <general.h>
#include <class_library.h>
#include <dialog_helpers.h>

// Used in DisplayListComponentsInLib: this is a callback function for EDA_LIST_DIALOG
// to display keywords and description of a component
static void DisplayCmpDocAndKeywords( wxString& aName, void* aData )
{
    PART_LIBS*  libs = (PART_LIBS*) aData;

    wxASSERT( libs );

    LIB_ALIAS* part = libs->FindLibraryEntry( aName );

    if( !part )
        return;

    aName  = wxT( "Description: " ) + part->GetDescription();
    aName += wxT( "\nKey Words: " ) + part->GetKeyWords();
}


PART_LIB* SCH_BASE_FRAME::SelectLibraryFromList()
{
    PROJECT&    prj = Prj();

    if( PART_LIBS* libs = prj.SchLibs() )
    {
        if( !libs->GetLibraryCount() )
        {
            DisplayError( this, _( "No component libraries are loaded." ) );
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

        EDA_LIST_DIALOG dlg( this, _( "Select Library" ), headers, itemsToDisplay, old_lib_name );

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



bool SCH_BASE_FRAME::DisplayListComponentsInLib( PART_LIB*  aLibrary,
                        wxString&  aBuffer, wxString&  aPreviousChoice )
{
    wxArrayString  nameList;

    if( aLibrary == NULL )
        aLibrary = SelectLibraryFromList();

    if( aLibrary == NULL )
        return false;

    aLibrary->GetEntryNames( nameList );

    wxArrayString headers;
    headers.Add( wxT("Component") );
    headers.Add( wxT("Library") );
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < nameList.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( nameList[i] );
        item.Add( aLibrary->GetLogicalName() );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Component" ), headers, itemsToDisplay,
                         aPreviousChoice, DisplayCmpDocAndKeywords, Prj().SchLibs() );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aBuffer = dlg.GetTextSelection();

    return true;
}


bool SCH_BASE_FRAME::SelectPartNameToLoad( PART_LIB* aLibrary, wxString& aBufName )
{
    int             ii;
    static wxString previousCmpName;

    ii = DisplayListComponentsInLib( aLibrary, aBufName, previousCmpName );

    if( ii <= 0 || aBufName.IsEmpty() )
        return false;

    previousCmpName = aBufName;
    return true;
}

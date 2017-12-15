/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2015-2017 KiCad Developers, see CHANGELOG.TXT for contributors.
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
#include <symbol_lib_table.h>

#include <general.h>
#include <class_library.h>
#include <dialog_helpers.h>


static void DisplayCmpDocAndKeywords( wxString& aSelection, void* aData )
{
    SYMBOL_LIB_TABLE* libs = (SYMBOL_LIB_TABLE*) aData;

    wxASSERT( libs );

    LIB_ID id;

    if( id.Parse( aSelection ) != -1 )
    {
        aSelection = _( "Invalid symbol library indentifier!" );
        return;
    }

    LIB_ALIAS* part = nullptr;

    try
    {
        part = libs->LoadSymbol( id );
    }
    catch( const IO_ERROR& ioe )
    {
        aSelection.Printf( _( "Error occurred loading symbol \"%s\" from library \"%s\"." ),
                           id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );
        return;
    }

    if( !part )
        return;

    aSelection  = _( "Description: " ) + part->GetDescription() + "\n";
    aSelection += _( "Key Words: " ) + part->GetKeyWords();
}


wxString SCH_BASE_FRAME::SelectLibraryFromList()
{
    PROJECT& prj = Prj();

    if( prj.SchSymbolLibTable()->IsEmpty() )
    {
        DisplayError( this, _( "No symbol libraries are loaded." ) );
        return wxEmptyString;
    }

    wxArrayString headers;

    headers.Add( _( "Library" ) );

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString > libNicknames = prj.SchSymbolLibTable()->GetLogicalLibs();

    // Conversion from wxArrayString to vector of ArrayString
    for( auto name : libNicknames )
    {
        wxArrayString item;

        item.Add( name );
        itemsToDisplay.push_back( item );
    }

    wxString old_lib_name = prj.GetRString( PROJECT::SCH_LIB_SELECT );

    EDA_LIST_DIALOG dlg( this, _( "Select Symbol Library" ), headers, itemsToDisplay,
                         old_lib_name );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString libname = dlg.GetTextSelection();

    if( !libname.empty() )
    {
        if( prj.SchSymbolLibTable()->HasLibrary( libname ) )
            prj.SetRString( PROJECT::SCH_LIB_SELECT, libname );
        else
            libname = wxEmptyString;
    }

    return libname;
}



bool SCH_BASE_FRAME::DisplayListComponentsInLib( wxString& aLibrary, wxString& aBuffer,
                                                 wxString& aPreviousChoice )
{
    wxArrayString nameList;

    if( !aLibrary )
        aLibrary = SelectLibraryFromList();

    if( !aLibrary )
        return false;

    try
    {
        Prj().SchSymbolLibTable()->EnumerateSymbolLib( aLibrary, nameList );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;

        msg.Printf( _( "Error occurred loading symbol library \"%s\"." ), aLibrary );
        DisplayErrorMessage( this, msg, ioe.What() );
        return false;
    }

    wxArrayString headers;
    headers.Add( _( "Library:Symbol" ) );

    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < nameList.GetCount(); i++ )
    {
        LIB_ID id;
        wxArrayString item;
        id.SetLibItemName( nameList[i], false );
        id.SetLibNickname( aLibrary );
        item.Add( id.Format() );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Select Symbol" ), headers, itemsToDisplay, aPreviousChoice,
                         DisplayCmpDocAndKeywords, Prj().SchSymbolLibTable() );

    if( dlg.ShowModal() != wxID_OK )
        return false;

    aBuffer = dlg.GetTextSelection();

    return true;
}


bool SCH_BASE_FRAME::SelectPartNameToLoad( wxString& aLibrary, wxString& aBufName )
{
    static wxString previousCmpName;

    if( !DisplayListComponentsInLib( aLibrary, aBufName, previousCmpName ) || aBufName.empty() )
        return false;

    previousCmpName = aBufName;
    return true;
}

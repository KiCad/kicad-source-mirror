/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file pcbnew/loadcmp.cpp
 * @brief Footprints selection and loading functions.
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "eda_doc.h"
#include "kicad_string.h"
#include "appl_wxstruct.h"
#include "wxPcbStruct.h"
#include "dialog_helpers.h"
#include "filter_reader.h"
#include "gr_basic.h"
#include "macros.h"
#include "pcbcommon.h"

#include "class_board.h"
#include "class_module.h"

#include "pcbnew.h"
#include "module_editor_frame.h"
#include "footprint_info.h"
#include "class_footprint_library.h"
#include "dialog_get_component.h"


static void DisplayCmpDoc( wxString& Name );

static FOOTPRINT_LIST MList;


bool FOOTPRINT_EDIT_FRAME::Load_Module_From_BOARD( MODULE* aModule )
{
    MODULE* newModule;
    PCB_BASE_FRAME* parent = (PCB_BASE_FRAME*) GetParent();

    if( aModule == NULL )
    {
        if( ! parent->GetBoard() || ! parent->GetBoard()->m_Modules )
            return false;

        aModule = Select_1_Module_From_BOARD( parent->GetBoard() );
    }

    if( aModule == NULL )
        return false;

    SetCurItem( NULL );

    Clear_Pcb( false );

    GetBoard()->m_Status_Pcb = 0;
    newModule = new MODULE( GetBoard() );
    newModule->Copy( aModule );
    newModule->m_Link = aModule->m_TimeStamp;

    aModule = newModule;

    GetBoard()->Add( aModule );

    aModule->m_Flags = 0;

    GetBoard()->m_NetInfo->BuildListOfNets();

    GetScreen()->SetCrossHairPosition( wxPoint( 0, 0 ) );
    PlaceModule( aModule, NULL );

    if( aModule->GetLayer() != LAYER_N_FRONT )
        aModule->Flip( aModule->m_Pos );

    Rotate_Module( NULL, aModule, 0, false );
    GetScreen()->ClrModify();
    Zoom_Automatique( false );

    return true;
}


MODULE* PCB_BASE_FRAME::Load_Module_From_Library( const wxString& library, wxDC* DC )
{
    MODULE*     module;
    wxPoint     curspos = GetScreen()->GetCrossHairPosition();
    wxString    moduleName, keys;
    bool        AllowWildSeach = true;

    static wxArrayString HistoryList;
    static wxString      lastCommponentName;

    /* Ask for a component name or key words */
    DIALOG_GET_COMPONENT dlg( this, GetComponentDialogPosition(), HistoryList,
                          _( "Place Module" ), false );

    dlg.SetComponentName( lastCommponentName );

    if( dlg.ShowModal() == wxID_CANCEL )
        return NULL;

    moduleName = dlg.GetComponentName();

    if( moduleName.IsEmpty() )  /* Cancel command */
    {
        DrawPanel->MoveCursorToCrossHair();
        return NULL;
    }

    moduleName.MakeUpper();

    if( moduleName[0] == '=' )   // Selection by keywords
    {
        AllowWildSeach = false;
        keys = moduleName.AfterFirst( '=' );
        moduleName = Select_1_Module_From_List( this, library, wxEmptyString, keys );

        if( moduleName.IsEmpty() )  /* Cancel command */
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;
        }
    }
    else if( ( moduleName.Contains( wxT( "?" ) ) )
            || ( moduleName.Contains( wxT( "*" ) ) ) )  // Selection wild card
    {
        AllowWildSeach = false;
        moduleName     = Select_1_Module_From_List( this, library, moduleName, wxEmptyString );
        if( moduleName.IsEmpty() )
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;    /* Cancel command. */
        }
    }

    module = GetModuleLibrary( library, moduleName, false );

    if( ( module == NULL ) && AllowWildSeach )    /* Search with wild card */
    {
        AllowWildSeach = false;
        wxString wildname = wxChar( '*' ) + moduleName + wxChar( '*' );
        moduleName = wildname;
        moduleName = Select_1_Module_From_List( this, library, moduleName, wxEmptyString );

        if( moduleName.IsEmpty() )
        {
            DrawPanel->MoveCursorToCrossHair();
            return NULL;    /* Cancel command. */
        }
        else
        {
            module = GetModuleLibrary( library, moduleName, true );
        }
    }

    GetScreen()->SetCrossHairPosition( curspos );
    DrawPanel->MoveCursorToCrossHair();

    if( module )
    {
        lastCommponentName = moduleName;
        AddHistoryComponentName( HistoryList, moduleName );

        module->m_Flags     = IS_NEW;
        module->m_Link      = 0;
        module->m_TimeStamp = GetTimeStamp();
        GetBoard()->m_Status_Pcb = 0;
        module->SetPosition( curspos );

        /* TODO: call RecalculateAllTracksNetcode() only if some pads pads have
         * a netname.
         * If all pads are not connected (usually the case in module libraries,
         * rebuild only the pad and list of nets ( faster)
         */


//        GetBoard()->m_Pcb->m_NetInfo->BuildListOfNets();
        RecalculateAllTracksNetcode();

        if( DC )
            module->Draw( DrawPanel, DC, GR_OR );
    }

    return module;
}


MODULE* PCB_BASE_FRAME::GetModuleLibrary( const wxString& aLibraryFullFilename,
                                          const wxString& aModuleName,
                                          bool            aDisplayMessageError )
{
    wxFileName fn;
    wxString   msg, tmp;
    MODULE*    newModule;
    FILE*      file = NULL;

    bool       one_lib = aLibraryFullFilename.IsEmpty() ? false : true;

    for( unsigned ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
    {
        if( one_lib )
            fn = aLibraryFullFilename;
        else
            fn = wxFileName( wxEmptyString, g_LibraryNames[ii], ModuleFileExtension );

        tmp = wxGetApp().FindLibraryPath( fn );

        if( !tmp )
        {
            if( aDisplayMessageError )
            {
                msg.Printf( _( "PCB footprint library file <%s> not found in search paths." ),
                            GetChars( fn.GetFullName() ) );
                wxMessageBox( msg, _( "Library Load Error" ), wxOK | wxICON_ERROR, this );
            }

            continue;
        }

        file = wxFopen( tmp, wxT( "rt" ) );

        if( file == NULL )
        {
            msg.Printf( _( "Could not open PCB footprint library file <%s>." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ), wxOK | wxICON_ERROR, this );
            continue;
        }

        FILE_LINE_READER fileReader( file, tmp );

        FILTER_READER reader( fileReader );

        msg.Printf( _( "Scan Lib: %s" ), GetChars( tmp ) );
        SetStatusText( msg );

        FOOTPRINT_LIBRARY curr_lib( file, &reader );

        if( !curr_lib.IsLibrary() )
        {
            msg.Printf( _( "<%s> is not a valid KiCad PCB footprint library file." ),
                        GetChars( tmp ) );
            wxMessageBox( msg, _( "Library Load Error" ), wxOK | wxICON_ERROR, this );
            return NULL;
        }

        // Reading the list of modules in the library.
        curr_lib.ReadSectionIndex();
        bool found = curr_lib.FindInList( aModuleName );

        // Read library.
        if( found  )
        {
            wxString   name;

            fileReader.Rewind();

            while( reader.ReadLine() )
            {
                char* line = reader.Line();

                StrPurge( line + 8 );

                if( strnicmp( line, "$MODULE", 7 ) != 0 )
                    continue;

                // Read module name.
                name = FROM_UTF8( line + 8 );

                if( name.CmpNoCase( aModuleName ) == 0 )
                {
                    newModule = new MODULE( GetBoard() );

                    // Temporarily switch the locale to standard C (needed to print
                    // floating point numbers like 1.3)
                    LOCALE_IO   toggle;

                    newModule->ReadDescr( &reader );

                    GetBoard()->Add( newModule, ADD_APPEND );
                    SetStatusText( wxEmptyString );
                    return newModule;
                }
            }
        }

        if( one_lib )
            break;
    }

    if( aDisplayMessageError )
    {
        msg.Printf( _( "Module <%s> not found" ), GetChars( aModuleName ) );
        DisplayError( NULL, msg );
    }

    return NULL;
}


wxString PCB_BASE_FRAME::Select_1_Module_From_List( EDA_DRAW_FRAME* aWindow,
                                                    const wxString& aLibraryFullFilename,
                                                    const wxString& aMask,
                                                    const wxString& aKeyWord )
{
    static wxString OldName;    /* Save the name of the last module loaded. */
    wxString        CmpName;
    wxString        msg;
    wxArrayString   libnames_list;

    if( aLibraryFullFilename.IsEmpty() )
        libnames_list = g_LibraryNames;
    else
        libnames_list.Add( aLibraryFullFilename );

    /* Find modules in libraries. */
    MList.ReadFootprintFiles( libnames_list );

    wxArrayString footprint_names_list;

    if( !aKeyWord.IsEmpty() ) // Create a list of modules found by keyword.
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            if( KeyWordOk( aKeyWord, MList.GetItem(ii).m_KeyWord ) )
                footprint_names_list.Add( MList.GetItem(ii).m_Module );
        }
    }
    else if( !aMask.IsEmpty() ) // Create a list of modules found by pattern
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
        {
            wxString& candidate = MList.GetItem(ii).m_Module;

            if( WildCompareString( aMask, candidate, false ) )
                footprint_names_list.Add( candidate );
        }
    }
    else        // Create the full list of modules
    {
        for( unsigned ii = 0; ii < MList.GetCount(); ii++ )
            footprint_names_list.Add( MList.GetItem(ii).m_Module );
    }

    if( footprint_names_list.GetCount() )
    {
        msg.Printf( _( "Modules [%d items]" ), footprint_names_list.GetCount() );
        EDA_LIST_DIALOG dlg( aWindow, msg, footprint_names_list, OldName,
                             DisplayCmpDoc, GetComponentDialogPosition() );

        if( dlg.ShowModal() == wxID_OK )
            CmpName = dlg.GetTextSelection();
        else
            CmpName.Empty();
    }
    else
    {
        DisplayError( aWindow, _( "No footprint found" ) );
        CmpName.Empty();
    }

    if( CmpName != wxEmptyString )
        OldName = CmpName;

    return CmpName;
}


/* Find and display the doc Component Name
 * The list of doc is pointed to by mlist.
 */
static void DisplayCmpDoc( wxString& Name )
{
    FOOTPRINT_INFO* module_info = MList.GetModuleInfo( Name );

    if( !module_info )
    {
        Name.Empty();
        return;
    }

    Name  = module_info->m_Doc.IsEmpty() ? wxT( "No Doc" ) : module_info->m_Doc;
    Name += wxT( "\nKeyW: " );
    Name += module_info->m_KeyWord.IsEmpty() ? wxT( "No Keyword" ) : module_info->m_KeyWord;
}


MODULE* FOOTPRINT_EDIT_FRAME::Select_1_Module_From_BOARD( BOARD* aPcb )
{
    MODULE*         module;
    static wxString OldName;       /* Save name of last module selected. */
    wxString        CmpName, msg;

    wxArrayString listnames;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
        listnames.Add( module->m_Reference->m_Text );

    msg.Printf( _( "Modules [%d items]" ), listnames.GetCount() );

    EDA_LIST_DIALOG dlg( this, msg, listnames, wxEmptyString );
    dlg.SortList();

    if( dlg.ShowModal() == wxID_OK )
        CmpName = dlg.GetTextSelection();
    else
        return NULL;

    OldName = CmpName;

    module = aPcb->m_Modules;

    for( ; module != NULL; module = (MODULE*) module->Next() )
    {
        if( CmpName == module->m_Reference->m_Text )
            break;
    }

    return module;
}

/**
 * @file pcbnew/netlist.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 Jean-Pierre Charras.
 * Copyright (C) 1992-2011 KiCad Developers, see change_log.txt for contributors.
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

#include <fctsys.h>
#include <appl_wxstruct.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <richio.h>
#include <dialog_helpers.h>
#include <wxPcbStruct.h>
#include <netlist_reader.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <io_mgr.h>

#include <algorithm>


void PCB_EDIT_FRAME::ReadPcbNetlist( const wxString& aNetlistFileName,
                                     const wxString& aCmpFileName,
                                     REPORTER*       aReporter,
                                     bool            aChangeFootprints,
                                     bool            aDeleteUnconnectedTracks,
                                     bool            aDeleteExtraFootprints,
                                     bool            aSelectByTimeStamp,
                                     bool            aIsDryRun )
{
    wxString        msg;
    NETLIST         netlist;
    NETLIST_READER* netlistReader;

    try
    {
        netlistReader = NETLIST_READER::GetNetlistReader( &netlist, aNetlistFileName,
                                                          aCmpFileName );

        if( netlistReader == NULL )
        {
            msg.Printf( _( "Cannot open netlist file \"%s\"." ), GetChars( aNetlistFileName ) );
            wxMessageBox( msg, _( "Netlist Load Error." ), wxOK | wxICON_ERROR, this );
            return;
        }

        std::auto_ptr< NETLIST_READER > nlr( netlistReader );
        SetLastNetListRead( aNetlistFileName );
        netlistReader->LoadNetlist();
        loadFootprints( netlist, aReporter );
    }
    catch( IO_ERROR& ioe )
    {
        msg = wxString::Format( _( "Error loading netlist.\n%s" ), ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return;
    }

    netlist.SetIsDryRun( aIsDryRun );
    netlist.SetFindByTimeStamp( aSelectByTimeStamp );
    netlist.SetDeleteExtraFootprints( aDeleteExtraFootprints );
    netlist.SetReplaceFootprints( aChangeFootprints );

    // Clear undo and redo lists to avoid inconsistencies between lists
    if( !netlist.IsDryRun() )
        GetScreen()->ClearUndoRedoList();

    netlist.SortByReference();
    GetBoard()->ReplaceNetlist( netlist, aReporter );

    // If it was a dry run, nothing has changed so we're done.
    if( netlist.IsDryRun() )
        return;

    OnModify();

    SetCurItem( NULL );

    if( aDeleteUnconnectedTracks && GetBoard()->m_Track )
    {
        // Remove erroneous tracks.  This should probably pushed down to the #BOARD object.
        RemoveMisConnectedTracks();
    }

    // Rebuild the board connectivity:
    Compile_Ratsnest( NULL, true );
    SetMsgPanel( GetBoard() );
    m_canvas->Refresh();
}


MODULE* PCB_EDIT_FRAME::ListAndSelectModuleName()
{
    MODULE* Module;

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayError( this, _( "No Modules" ) );
        return 0;
    }

    wxArrayString listnames;
    Module = (MODULE*) GetBoard()->m_Modules;

    for( ; Module != NULL; Module = (MODULE*) Module->Next() )
        listnames.Add( Module->GetReference() );

    wxArrayString headers;
    headers.Add( wxT( "Module" ) );
    std::vector<wxArrayString> itemsToDisplay;

    // Conversion from wxArrayString to vector of ArrayString
    for( unsigned i = 0; i < listnames.GetCount(); i++ )
    {
        wxArrayString item;
        item.Add( listnames[i] );
        itemsToDisplay.push_back( item );
    }

    EDA_LIST_DIALOG dlg( this, _( "Components" ), headers, itemsToDisplay, wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return NULL;

    wxString ref = dlg.GetTextSelection();
    Module = (MODULE*) GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        if( Module->GetReference() == ref )
            break;
    }

    return Module;
}


void PCB_EDIT_FRAME::loadFootprints( NETLIST& aNetlist, REPORTER* aReporter )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxString   msg;
    wxString   lastFootprintLibName;
    COMPONENT* component;
    MODULE*    module;

    if( aNetlist.IsEmpty() )
        return;

    aNetlist.SortByFootprintLibName();

    wxString   libPath;
    wxFileName fn;

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

        if( ii == 0 || component->GetFootprintLibName() != lastFootprintLibName )
        {
            module = NULL;

            for( unsigned ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
            {
                fn = wxFileName( wxEmptyString, g_LibraryNames[ii],
                                 LegacyFootprintLibPathExtension );

                libPath = wxGetApp().FindLibraryPath( fn );

                if( !libPath )
                    continue;

                module = pi->FootprintLoad( libPath, component->GetFootprintLibName() );

                if( module )
                {
                    lastFootprintLibName = component->GetFootprintLibName();
                    break;
                }
            }

            if( module == NULL )
            {
                wxString msg;
                msg.Printf( _( "Component `%s` footprint <%s> was not found in any libraries." ),
                            GetChars( component->GetReference() ),
                            GetChars( component->GetFootprintLibName() ) );
                THROW_IO_ERROR( msg );
            }
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( module == NULL )
                continue;            // Module does not exist in any library.

            module = new MODULE( *module );
        }

        wxASSERT( module != NULL );
        component->SetModule( module );
    }
}

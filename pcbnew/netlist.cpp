/**
 * @file pcbnew/netlist.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see change_log.txt for contributors.
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
#include <dialog_helpers.h>
#include <wxPcbStruct.h>
#include <netlist_reader.h>
#include <reporter.h>
#include <wildcards_and_files_ext.h>
#include <fpid.h>
#include <fp_lib_table.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <io_mgr.h>


void PCB_EDIT_FRAME::ReadPcbNetlist( const wxString& aNetlistFileName,
                                     const wxString& aCmpFileName,
                                     REPORTER*       aReporter,
                                     bool            aChangeFootprints,
                                     bool            aDeleteUnconnectedTracks,
                                     bool            aDeleteExtraFootprints,
                                     bool            aSelectByTimeStamp,
                                     bool            aDeleteSinglePadNets,
                                     bool            aIsDryRun )
{
    wxString        msg;
    NETLIST         netlist;
    NETLIST_READER* netlistReader;

    netlist.SetIsDryRun( aIsDryRun );
    netlist.SetFindByTimeStamp( aSelectByTimeStamp );
    netlist.SetDeleteExtraFootprints( aDeleteExtraFootprints );
    netlist.SetReplaceFootprints( aChangeFootprints );

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
        msg.Printf( _( "Error loading netlist.\n%s" ), ioe.errorText.GetData() );
        wxMessageBox( msg, _( "Netlist Load Error" ), wxOK | wxICON_ERROR );
        return;
    }

    // Clear undo and redo lists to avoid inconsistencies between lists
    if( !netlist.IsDryRun() )
        GetScreen()->ClearUndoRedoList();

    netlist.SortByReference();
    GetBoard()->ReplaceNetlist( netlist, aDeleteSinglePadNets, aReporter );

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
    RefreshCanvas();
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


#if !defined( USE_FP_LIB_TABLE )

void PCB_EDIT_FRAME::loadFootprints( NETLIST& aNetlist, REPORTER* aReporter )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxString   msg;
    wxString   lastFootprintLibName;
    wxArrayString nofoundFootprints;    // A list of footprints used in netlist
                                        // but not found in any library
                                        // to avoid a full search in all libs
                                        // each time a non existent footprint is needed
    COMPONENT* component;
    MODULE*    module = 0;
    MODULE*    fpOnBoard;


    if( aNetlist.IsEmpty() )
        return;

    aNetlist.SortByFootprintName();

    wxString   libPath;
    wxFileName fn;

    PLUGIN::RELEASER pi( IO_MGR::PluginFind( IO_MGR::LEGACY ) );

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

        if( component->GetFootprintName().IsEmpty() )
        {
            if( aReporter )
            {
                msg.Printf( _( "No footprint defined for component `%s`.\n" ),
                            GetChars( component->GetReference() ) );
                aReporter->Report( msg );
            }

            continue;
        }

        // Check if component footprint is already on BOARD and only load the footprint from
        // the library if it's needed.
        if( aNetlist.IsFindByTimeStamp() )
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetTimeStamp(), true );
        else
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetReference() );

        bool footprintMisMatch = fpOnBoard &&
                                 fpOnBoard->GetLibRef() != component->GetFootprintName();

        if( footprintMisMatch && !aNetlist.GetReplaceFootprints() )
        {
            if( aReporter )
            {
                msg.Printf( _( "* Warning: component `%s` has footprint <%s> and should be  <%s>\n" ),
                            GetChars( component->GetReference() ),
                            GetChars( fpOnBoard->GetLibRef() ),
                            GetChars( component->GetFootprintName() ) );
                aReporter->Report( msg );
            }

            continue;
        }

        if( !aNetlist.GetReplaceFootprints() )
            footprintMisMatch = false;

        bool loadFootprint = (fpOnBoard == NULL) || footprintMisMatch;

        if( loadFootprint && (component->GetFootprintName() != lastFootprintLibName) )
        {
            module = NULL;

            // Speed up the search: a search for a non existent footprint
            // is hightly costly in time becuse the full set of libs is read.
            // So it should be made only once.
            // Therefore search in not found list first:
            bool alreadySearched = false;
            for( unsigned ii = 0; ii < nofoundFootprints.GetCount(); ii++ )
            {
                if( component->GetFootprintName() == nofoundFootprints[ii] )
                {
                    alreadySearched = true;
                    break;
                }
            }

            if( alreadySearched )
                continue;

            for( unsigned ii = 0; ii < g_LibraryNames.GetCount(); ii++ )
            {
                fn = wxFileName( wxEmptyString, g_LibraryNames[ii],
                                 LegacyFootprintLibPathExtension );

                libPath = wxGetApp().FindLibraryPath( fn );

                if( !libPath )
                {
                    if( aReporter )
                    {
                        msg.Printf( _( "*** Warning: Cannot find footprint library file \"%s\" "
                                       "in any of the standard KiCad library search paths. ***\n" ),
                                    GetChars( fn.GetFullPath() ) );
                        aReporter->Report( msg );
                    }

                    continue;
                }

                module = pi->FootprintLoad( libPath, component->GetFootprintName() );

                if( module )
                {
                    lastFootprintLibName = component->GetFootprintName();
                    break;
                }
            }

            if( module == NULL && !alreadySearched )
            {
                if( aReporter )
                {
                    msg.Printf( _( "*** Warning: component `%s` footprint <%s> was not found in "
                                   "any libraries. ***\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetFootprintName() ) );
                    aReporter->Report( msg );
                }

                nofoundFootprints.Add( component->GetFootprintName() );

                continue;
            }
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( module == NULL )
                continue;            // Module does not exist in any library.

            module = new MODULE( *module );
        }

        if( loadFootprint && module != NULL )
            component->SetModule( module );
    }
}

#else

void PCB_EDIT_FRAME::loadFootprints( NETLIST& aNetlist, REPORTER* aReporter )
    throw( IO_ERROR, PARSE_ERROR )
{
    wxString   msg;
    wxString   lastFootprintLibName;
    COMPONENT* component;
    MODULE*    module = 0;
    MODULE*    fpOnBoard;

    if( aNetlist.IsEmpty() || m_footprintLibTable->IsEmpty() )
        return;

    aNetlist.SortByFootprintName();

    for( unsigned ii = 0; ii < aNetlist.GetCount(); ii++ )
    {
        component = aNetlist.GetComponent( ii );

        if( component->GetFootprintName().IsEmpty() )
        {
            if( aReporter )
            {
                msg.Printf( _( "No footprint defined for component `%s`.\n" ),
                            GetChars( component->GetReference() ) );
                aReporter->Report( msg );
            }

            continue;
        }

        // Check if component footprint is already on BOARD and only load the footprint from
        // the library if it's needed.
        if( aNetlist.IsFindByTimeStamp() )
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetTimeStamp(), true );
        else
            fpOnBoard = m_Pcb->FindModule( aNetlist.GetComponent( ii )->GetReference() );

        bool footprintMisMatch = fpOnBoard &&
                                 fpOnBoard->GetLibRef() != component->GetFootprintName();

        if( footprintMisMatch && !aNetlist.GetReplaceFootprints() )
        {
            if( aReporter )
            {
                msg.Printf( _( "* Warning: component `%s` has footprint <%s> and should be  <%s>\n" ),
                            GetChars( component->GetReference() ),
                            GetChars( fpOnBoard->GetLibRef() ),
                            GetChars( component->GetFootprintName() ) );
                aReporter->Report( msg );
            }

            continue;
        }

        if( !aNetlist.GetReplaceFootprints() )
            footprintMisMatch = false;

        bool loadFootprint = (fpOnBoard == NULL) || footprintMisMatch;

        if( loadFootprint && (component->GetFootprintName() != lastFootprintLibName) )
        {
            module = NULL;

            FPID fpid;

            if( fpid.Parse( TO_UTF8( component->GetFootprintName() ) ) >= 0 )
            {
                if( aReporter )
                {
                    msg.Printf( _( "*** Warning: Component \"%s\" footprint ID <%s> is not "
                                   "valid. ***\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetFootprintName() ) );
                    aReporter->Report( msg );
                }

                continue;
            }

            module = PCB_BASE_FRAME::loadFootprint( fpid );

            if( module )
            {
                lastFootprintLibName = component->GetFootprintName();
            }

            if( module == NULL )
            {
                if( aReporter )
                {
                    wxString msg;
                    msg.Printf( _( "*** Warning: component `%s` footprint <%s> was not found in "
                                   "any libraries in the footprint library table. ***\n" ),
                                GetChars( component->GetReference() ),
                                GetChars( component->GetFootprintName() ) );
                    aReporter->Report( msg );
                }

                continue;
            }
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( module == NULL )
                continue;            // Module does not exist in any library.

            module = new MODULE( *module );
        }

        if( loadFootprint && module != NULL )
            component->SetModule( module );
    }
}

#endif

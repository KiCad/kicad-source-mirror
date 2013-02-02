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

/*
 *  Functions to read a netlist:
 *  - Load new footprints and initialize net info
 *  - Test for missing or extra footprints
 *  - Recalculate full connectivity info
 *
 *  Important remark:
 *  When reading a netlist, Pcbnew must identify existing footprints (link
 * between existing footprints an components in netlist)
 *  This identification can be made from 2 fields:
 *      - The reference (U2, R5 ..): this is the normal mode
 *      - The Time Stamp : useful after a full schematic
 * reannotation because references can be changed for the component linked to its footprint.
 * So when reading a netlist, ReadPcbNetlist() can use references or time stamps
 * to identify footprints on board and the corresponding component in schematic.
 *  If we want to fully reannotate a schematic this sequence must be used
 *   1 - SAVE your board !!!
 *   2 - Create and read the netlist (to ensure all info is correct, mainly
 * references and time stamp)
 *   3 - Reannotate the schematic (references will be changed, but not time stamps )
 *   4 - Recreate and read the new netlist using the Time Stamp identification
 * (that reinit the new references)
 */



#include <fctsys.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <richio.h>
#include <dialog_helpers.h>
#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <dialog_netlist.h>
#include <netlist_reader.h>

#include <algorithm>

/**
 * Function OpenNetlistFile
 *  used to open a netlist file
 */
static FILE* OpenNetlistFile( const wxString& aFullFileName )
{
    if( aFullFileName.IsEmpty() )
        return NULL;  // No filename: exit

    FILE* file = wxFopen( aFullFileName, wxT( "rt" ) );

    if( file == NULL )
    {
        wxString msg;
        msg.Printf( _( "Netlist file %s not found" ), GetChars( aFullFileName ) );
        wxMessageBox( msg );
    }

    return file;
}



/* Update footprints (load missing footprints and delete on request extra footprints)
 * Update connectivity info ( Net Name list )
 * Update Reference, value and "TIME STAMP"
 * param aNetlistFullFilename = netlist file name (*.net)
 * param aCmpFullFileName = cmp/footprint list file name (*.cmp) if not found,
 * param aMessageWindow  = a wxTextCtrl to print messages (can be NULL).
 * param aChangeFootprint = true to change existing footprints
 *                              when the netlist gives a different footprint.
 *                           false to keep existing footprints
 * param aDeleteBadTracks - true to erase erroneous tracks after updating connectivity info.
 * param aDeleteExtraFootprints - true to remove unlocked footprints found on board but not
 *                                 in netlist.
 * param aSelect_By_Timestamp - true to use schematic timestamps instead of schematic references
 *                              to identify footprints on board
 *                              (Must be used after a full reannotation in schematic).
 * param aUseCmpFileForFootprintsNames = false to use only the netlist to know the
 *                      fontprint names of each component.
 *                                     = true to use the .cmp file created by CvPcb
 * return true if Ok
 */
bool PCB_EDIT_FRAME::ReadPcbNetlist( const wxString& aNetlistFullFilename,
                                     const wxString& aCmpFullFileName,
                                     wxTextCtrl*     aMessageWindow,
                                     bool            aChangeFootprint,
                                     bool            aDeleteBadTracks,
                                     bool            aDeleteExtraFootprints,
                                     bool            aSelect_By_Timestamp )
{
    FILE*   netfile = OpenNetlistFile( aNetlistFullFilename );

    if( !netfile )
        return false;

    SetLastNetListRead( aNetlistFullFilename );
    bool useCmpfile = !aCmpFullFileName.IsEmpty() && wxFileExists( aCmpFullFileName );

    if( aMessageWindow )
    {
        wxString msg;
        msg.Printf( _( "Reading Netlist \"%s\"" ), GetChars( aNetlistFullFilename ) );
        aMessageWindow->AppendText( msg + wxT( "\n" ) );

        if( useCmpfile )
        {
            msg.Printf( _( "Using component/footprint link file \"%s\"" ),
                        GetChars( aCmpFullFileName ) );
            aMessageWindow->AppendText( msg + wxT( "\n" ) );
        }

        if( aSelect_By_Timestamp )
        {
            msg.Printf( _( "Using time stamp selection" ),
                        GetChars( aCmpFullFileName ) );
            aMessageWindow->AppendText( msg + wxT( "\n" ) );
        }
     }

    // Clear undo and redo lists to avoid inconsistencies between lists
    GetScreen()->ClearUndoRedoList();

    OnModify();

    // Clear flags and pointers to avoid inconsistencies
    GetBoard()->m_Status_Pcb = 0;
    SetCurItem( NULL );

    wxBusyCursor   dummy;      // Shows an hourglass while calculating

    NETLIST_READER netList_Reader( this, aMessageWindow );
    netList_Reader.m_UseTimeStamp     = aSelect_By_Timestamp;
    netList_Reader.m_ChangeFootprints = aChangeFootprint;
    netList_Reader.m_UseCmpFile = useCmpfile;
    netList_Reader.SetFilesnames( aNetlistFullFilename, aCmpFullFileName );

    // True to read footprint filters section: true for CvPcb, false for Pcbnew
    netList_Reader.ReadLibpartSectionSetOpt( false );

    bool success = netList_Reader.ReadNetList( netfile );
    if( !success )
    {
        wxMessageBox( _("Netlist read error") );
        return false;
    }

    // Delete footprints not found in netlist:
    if( aDeleteExtraFootprints )
    {
        if( IsOK( NULL,
            _( "Ok to delete not locked footprints not found in netlist?" ) ) )
            netList_Reader.RemoveExtraFootprints();
    }

    // Rebuild the board connectivity:
    Compile_Ratsnest( NULL, true );

    if( aDeleteBadTracks && GetBoard()->m_Track )
    {
        // Remove erroneous tracks
        if( RemoveMisConnectedTracks() )
            Compile_Ratsnest( NULL, true );
    }

    SetMsgPanel( GetBoard() );
    m_canvas->Refresh();

    return true;
}


/**
 * build and shows a list of existing modules on board
 * The user can select a module from this list
 * @return a pointer to the selected module or NULL
 */
MODULE* PCB_EDIT_FRAME::ListAndSelectModuleName( void )
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
        listnames.Add( Module->m_Reference->m_Text );

    EDA_LIST_DIALOG dlg( this, _( "Components" ), listnames, wxEmptyString );

    if( dlg.ShowModal() != wxID_OK )
        return NULL;

    wxString ref = dlg.GetTextSelection();
    Module = (MODULE*) GetBoard()->m_Modules;

    for( ; Module != NULL; Module = Module->Next() )
    {
        if( Module->m_Reference->m_Text == ref )
            break;
    }

    return Module;
}


/*
 * Function Test_Duplicate_Missing_And_Extra_Footprints
 * Build a list of duplicate, missing and extra footprints
 * from the current board and a netlist netlist :
 * Shows 3 lists:
 *  1 - duplicate footprints on board
 *  2 - missing footprints (found in netlist but not on board)
 *  3 - footprints not in netlist but on board
 * param aFilename = the full filename netlist
 * param aDuplicate = the list of duplicate modules to populate
 * param aMissing = the list of missing module references and values
 *      to populate. For each missing item, the first string is the ref,
 *                   the second is the value.
 * param aNotInNetlist = the list of not-in-netlist modules to populate
 */
bool PCB_EDIT_FRAME::Test_Duplicate_Missing_And_Extra_Footprints(
        const wxString& aFilename,
        std::vector <MODULE*>& aDuplicate,
        wxArrayString& aMissing,
        std::vector <MODULE*>& aNotInNetlist )
{
    FILE*   netfile = OpenNetlistFile( aFilename );
    if( !netfile )
        return false;

    // Build the list of references of the net list modules.
    NETLIST_READER netList_Reader( this );
    netList_Reader.SetFilesnames( aFilename, wxEmptyString );
    netList_Reader.BuildModuleListOnlySetOpt( true );
    if( ! netList_Reader.ReadNetList( netfile ) )
        return false;  // error

    COMPONENT_INFO_LIST& moduleInfoList = netList_Reader.GetComponentInfoList();

    // Search for duplicate footprints.
    MODULE* module = GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        MODULE* altmodule = module->Next();

        for( ; altmodule != NULL; altmodule = altmodule->Next() )
        {
            if( module->m_Reference->m_Text.CmpNoCase( altmodule->m_Reference->m_Text ) == 0 )
            {
                aDuplicate.push_back( module );
                break;
            }
        }
    }

    // Search for missing modules on board.
    for( unsigned ii = 0; ii < moduleInfoList.size(); ii++ )
    {
        COMPONENT_INFO* cmp_info = moduleInfoList[ii];
        module = GetBoard()->FindModuleByReference( cmp_info->m_Reference );
        if( module == NULL )    // Module missing, not found in board
        {
            aMissing.Add( cmp_info->m_Reference );
            aMissing.Add( cmp_info->m_Value );
        }
    }

    // Search for modules found on board but not in net list.
    module = GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        unsigned ii;
        for( ii = 0; ii < moduleInfoList.size(); ii++ )
        {
            COMPONENT_INFO* cmp_info = moduleInfoList[ii];
            if( module->m_Reference->m_Text.CmpNoCase( cmp_info->m_Reference ) == 0 )
                break; // Module is in net list.
        }

        if( ii == moduleInfoList.size() )   // Module not found in netlist
            aNotInNetlist.push_back( module );
    }

    return true;
}

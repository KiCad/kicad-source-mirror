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
#include <kicad_string.h>
#include <gestfich.h>
#include <wxPcbStruct.h>
#include <richio.h>
#include <dialog_helpers.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>
#include <dialog_netlist.h>
#include <html_messagebox.h>

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



/**
 * Function ReadPcbNetlist
 * Update footprints (load missing footprints and delete on request extra
 * footprints)
 * Update connectivity info ( Net Name list )
 * Update Reference, value and "TIME STAMP"
 * @param aNetlistFullFilename = netlist file name (*.net)
 * @param aCmpFullFileName = cmp/footprint list file name (*.cmp) if not found,
 * @param aMessageWindow  = a wxTextCtrl to print messages (can be NULL).
 * @param aChangeFootprint = true to change existing footprints
 *                              when the netlist gives a different footprint.
 *                           false to keep existing footprints
 * @param aDeleteBadTracks - true to erase erroneous tracks after updating connectivity info.
 * @param aDeleteExtraFootprints - true to remove unlocked footprints found on board but not
 *                                 in netlist.
 * @param aSelect_By_Timestamp - true to use schematic timestamps instead of schematic references
 *                              to identify footprints on board
 *                              (Must be used after a full reannotation in schematic).
 * @return true if Ok
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

    bool useCmpfile = ! aCmpFullFileName.IsEmpty() && wxFileExists( aCmpFullFileName );

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
    netList_Reader.ReadFootprintFilterSetOpt( true );
    netList_Reader.SetFilesnames( aNetlistFullFilename, aCmpFullFileName );

    bool success = netList_Reader.ReadNetList( netfile );
    if( !success )
    {
        wxMessageBox( _("Netlist read error (unrecognized format)") );
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

    GetBoard()->DisplayInfo( this );
    m_canvas->Refresh();

    return true;
}

/*
 * Function ReadNetList
 * The main function to detect the netlist format,and run the right netlist reader
 * aFile = the already opened file (will be closed by the netlist reader)
 */
bool NETLIST_READER::ReadNetList( FILE* aFile )
{
    // Try to determine the netlist type:
    // Beginning of the first line of known formats, without spaces
    #define HEADERS_COUNT 3
    #define HEADER_ORCADPCB "({EESchemaNetlist"
    #define HEADER_PCB1 "#EESchemaNetlist"
    #define HEADER_KICAD_NETFMT "(export"
    const std::string headers[HEADERS_COUNT] =
    {
        HEADER_ORCADPCB, HEADER_PCB1, HEADER_KICAD_NETFMT
    };

    int format = -1;
    for ( int jj = 0; jj < HEADERS_COUNT; jj++ )
    {
        int imax = headers[jj].size();
        int ii = 0;
        for( ; ii < imax; ii++ )
        {
            int data;
            // Read header, and skip blanks to avoid errors if an header changes
            do
            {
                data = fgetc( aFile );
            } while ( ( data == ' ' ) &&( EOF != data ) ) ;

            if( (int)headers[jj][ii] == data )
                continue;
            break;
        }
        if( ii == imax )    // header found
        {
            format = jj;
            break;
        }
        rewind( aFile );
    }

    rewind( aFile );
    bool success = false;
    switch( format )
    {
        case 0:
            m_typeNetlist = NETLIST_TYPE_ORCADPCB2;
            success = ReadOldFmtdNetList( aFile );
            break;

        case 1:
            m_typeNetlist = NETLIST_TYPE_PCB1;
            success = ReadOldFmtdNetList( aFile );
            break;

        case 2:
            m_typeNetlist = NETLIST_TYPE_KICAD;
            success = ReadKicadNetList( aFile );
            break;

        default:    // Unrecognized format:
            break;

    }

    return success;
}

bool NETLIST_READER::InitializeModules()
{
    if( m_UseCmpFile )  // Try to get footprint name from .cmp file
    {
        readModuleComponentLinkfile();
    }

    if( m_pcbframe == NULL )
        return true;

    for( unsigned ii = 0; ii < m_modulesInNetlist.size(); ii++ )
    {
        MODULE_INFO* currmod_info = m_modulesInNetlist[ii];
        // Test if module is already loaded.
        wxString * idMod = m_UseTimeStamp?
                           &currmod_info->m_TimeStamp : &currmod_info->m_Reference;

        MODULE* module = FindModule( *idMod );
        if( module == NULL )   // not existing, load it
        {
            m_newModulesList.push_back( currmod_info );
        }
    }

    bool success = loadNewModules();

    // Update modules fields
    for( unsigned ii = 0; ii < m_modulesInNetlist.size(); ii++ )
    {
        MODULE_INFO* currmod_info =  m_modulesInNetlist[ii];
        // Test if module is already loaded.
        wxString * idMod = m_UseTimeStamp?
                              &currmod_info->m_TimeStamp : &currmod_info->m_Reference;

        MODULE* module = FindModule( *idMod );
        if( module )
        {
             // Update current module ( reference, value and "Time Stamp")
            module->m_Reference->m_Text = currmod_info->m_Reference;
            module->m_Value->m_Text     = currmod_info->m_Value;
            module->SetPath( currmod_info->m_TimeStamp );
        }
        else   // not existing
        {
        }
    }

    // clear pads netnames
    for( MODULE* module = m_pcbframe->GetBoard()->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad; pad = pad->Next() )
            pad->SetNetname( wxEmptyString );
    }

    return success;
}

void NETLIST_READER::TestFootprintsMatchingAndExchange()
{
    if( m_pcbframe == NULL )
        return;

    for( MODULE* module = m_pcbframe->GetBoard()->m_Modules; module; module = module->Next() )
    {
        // Search for the corresponding module info
        MODULE_INFO * mod_info = NULL;
        for( unsigned ii = 0; ii < m_modulesInNetlist.size(); ii++ )
        {
            MODULE_INFO * candidate =  m_modulesInNetlist[ii];
            // Test if mod_info matches the current module:
            if( candidate->m_Reference.CmpNoCase( module->GetReference() ) == 0 )
            {
                mod_info = candidate;
                break;
            }
        }
        if( mod_info == NULL )   // not found in netlist
             continue;

        if( module->GetLibRef().CmpNoCase( mod_info->m_Footprint ) != 0 )
        {
            if( m_ChangeFootprints )   // footprint exchange allowed.
            {
                MODULE* newModule = m_pcbframe->GetModuleLibrary( wxEmptyString,
                                                                  mod_info->m_Footprint,
                                                                  false );

                if( newModule )
                {
                    // Change old module to the new module (and delete the old one)
                    m_pcbframe->Exchange_Module( module, newModule, NULL );
                    module = newModule;
                }
                else if( m_messageWindow )
                {
                wxString msg;
                msg.Printf( _( "Component \"%s\": module [%s] not found\n" ),
                            GetChars( mod_info->m_Reference ),
                            GetChars( mod_info->m_Footprint ) );

                m_messageWindow->AppendText( msg );
                }
            }
            else if( m_messageWindow )
            {
                wxString msg;
                msg.Printf( _( "Component \"%s\": Mismatch! module is [%s] and netlist said [%s]\n" ),
                            GetChars( mod_info->m_Reference ),
                            GetChars( module->GetLibRef() ),
                            GetChars( mod_info->m_Footprint ) );

                m_messageWindow->AppendText( msg );
            }
        }
    }
}

/**
 * Function SetPadNetName
 *  Update a pad netname
 *  @param aModule = module reference
 *  @param aPadname = pad name (pad num)
 *  @param aNetname = new net name of the pad
 *  @return a pointer to the pad or NULL if the pad is not found
 */
D_PAD* NETLIST_READER::SetPadNetName( const wxString & aModule, const wxString & aPadname,
                      const wxString & aNetname )
{
    MODULE* module = m_pcbframe->GetBoard()->FindModuleByReference( aModule );
    if( module )
    {
        D_PAD * pad = module->FindPadByName( aPadname );
        if( pad )
        {
            pad->SetNetname( aNetname );
            return pad;
        }
        if( m_messageWindow )
        {
            wxString msg;
            msg.Printf( _( "Module [%s]: Pad [%s] not found" ),
                        GetChars( aModule ), GetChars( aPadname ) );
            m_messageWindow->AppendText( msg + wxT( "\n" ) );
        }
    }

    return NULL;
}


/* function RemoveExtraFootprints
 * Remove (delete) not locked footprints found on board, but not in netlist
 */
void NETLIST_READER::RemoveExtraFootprints()
{
    MODULE* nextModule;

    MODULE* module = m_pcbframe->GetBoard()->m_Modules;
    for( ; module != NULL; module = nextModule )
    {
        unsigned ii;
        nextModule = module->Next();

        if( module->m_ModuleStatus & MODULE_is_LOCKED )
            continue;

        for( ii = 0; ii < m_modulesInNetlist.size(); ii++ )
        {
            MODULE_INFO* mod_info = m_modulesInNetlist[ii];
            if( module->m_Reference->m_Text.CmpNoCase( mod_info->m_Reference ) == 0 )
                break; // Module is found in net list.
        }

        if( ii == m_modulesInNetlist.size() )   // Module not found in netlist.
            module->DeleteStructure();
    }
}


/* Search for a module id the modules existing in the current BOARD.
 * aId is a key to identify the module to find:
 * The reference or the full time stamp, according to m_UseTimeStamp
 * Returns the module is found, NULL otherwise.
 */
MODULE* NETLIST_READER::FindModule( const wxString& aId )
{
    MODULE* module = m_pcbframe->GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        if( m_UseTimeStamp ) // identification by time stamp
        {
            if( aId.CmpNoCase( module->m_Path ) == 0 )
                return module;
        }
        else    // identification by Reference
        {
            if( aId.CmpNoCase( module->m_Reference->m_Text ) == 0 )
                return module;
        }
    }

    return NULL;
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
 * @param aNetlistFullFilename = the full filename netlist
 */
void PCB_EDIT_FRAME::Test_Duplicate_Missing_And_Extra_Footprints(
    const wxString& aNetlistFullFilename )
{
    #define ERR_CNT_MAX 100 // Max number of errors to output in dialog
                            // to avoid a too long calculation time
    wxString list;          // The messages to display

    if( GetBoard()->m_Modules == NULL )
    {
        DisplayInfoMessage( this, _( "No modules" ) );
        return;
    }

    FILE*   netfile = OpenNetlistFile( aNetlistFullFilename );
    if( !netfile )
        return;

    SetLastNetListRead( aNetlistFullFilename );

    // Build the list of references of the net list modules.
    NETLIST_READER netList_Reader( this );
    netList_Reader.SetFilesnames( aNetlistFullFilename, wxEmptyString );
    netList_Reader.BuildModuleListOnlySetOpt( true );
    if( ! netList_Reader.ReadNetList( netfile ) )
        return;  // error

    std::vector <MODULE_INFO*>& moduleInfoList = netList_Reader.GetModuleInfoList();

    if( moduleInfoList.size() == 0 )
    {
        wxMessageBox( _( "No modules in NetList" ) );
        return;
    }

    // Search for duplicate footprints.
    list << wxT("<p><b>") << _( "Duplicates:" ) << wxT("</b></p>");

    int err_cnt = 0;
    MODULE* module = GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        MODULE* altmodule = module->Next();

        for( ; altmodule != NULL; altmodule = altmodule->Next() )
        {
            if( module->m_Reference->m_Text.CmpNoCase( altmodule->m_Reference->m_Text ) == 0 )
            {
                if( module->m_Reference->m_Text.IsEmpty() )
                    list << wxT("<br>") << wxT("[noref)");
                else
                    list << wxT("<br>") << module->m_Reference->m_Text;

                list << wxT("  (<i>") << module->m_Value->m_Text << wxT("</i>)");
                err_cnt++;
                break;
            }
        }
        if( ERR_CNT_MAX < err_cnt )
            break;
    }

    // Search for missing modules on board.
    list << wxT("<p><b>") <<  _( "Missing:" ) << wxT("</b></p>");

    for( unsigned ii = 0; ii < moduleInfoList.size(); ii++ )
    {
        MODULE_INFO* mod_info = moduleInfoList[ii];
        module = GetBoard()->FindModuleByReference( mod_info->m_Reference );
        if( module == NULL )    // Module missing, not found in board
        {
            list << wxT("<br>") << mod_info->m_Reference;
            list << wxT("  (<i>") << mod_info->m_Value << wxT("</i>)");
            err_cnt++;
        }
        if( ERR_CNT_MAX < err_cnt )
            break;
    }

    // Search for modules found on board but not in net list.
    list << wxT("<p><b>") << _( "Not in Netlist:" ) << wxT("</b></p>");

    module = GetBoard()->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        unsigned ii;
        for( ii = 0; ii < moduleInfoList.size(); ii++ )
        {
            MODULE_INFO* mod_info = moduleInfoList[ii];
            if( module->m_Reference->m_Text.CmpNoCase( mod_info->m_Reference ) == 0 )
                break; // Module is in net list.
        }

        if( ii == moduleInfoList.size() )   // Module not found in netlist
        {
            if( module->m_Reference->m_Text.IsEmpty() )
                list << wxT("<br>") << wxT("[noref)");
            else
                list << wxT("<br>") << module->m_Reference->m_Text ;
            list << wxT("  (<i>") << module->m_Value->m_Text << wxT("</i>)");
            err_cnt++;
        }
        if( ERR_CNT_MAX < err_cnt )
            break;
    }
    if( ERR_CNT_MAX < err_cnt )
    {
        list << wxT("<p><b>")
             << _( "Too many errors: some are skipped" )
             << wxT("</b></p>");
    }

    HTML_MESSAGE_BOX dlg( this, _( "Check Modules" ) );
    dlg.AddHTML_Text(list);
    dlg.ShowModal();
}


/*
 * function readModuleComponentLinkfile
 * read the *.cmp file ( filename in m_cmplistFullName )
 * giving the equivalence Footprint_names / components
 * to find the footprint name corresponding to aCmpIdent
 * return true if the file can be read
 *
 * Sample file:
 *
 * Cmp-Mod V01 Genere by Pcbnew 29/10/2003-13: 11:6 *
 *  BeginCmp
 *  TimeStamp = /32307DE2/AA450F67;
 *  Reference = C1;
 *  ValeurCmp = 47uF;
 *  IdModule  = CP6;
 *  EndCmp
 *
 */

bool NETLIST_READER::readModuleComponentLinkfile()
{
    wxString refcurrcmp;    // Stores value read from line like Reference = BUS1;
    wxString timestamp;     // Stores value read from line like TimeStamp = /32307DE2/AA450F67;
    wxString footprint;     // Stores value read from line like IdModule  = CP6;

    FILE*    cmpFile = wxFopen( m_cmplistFullName, wxT( "rt" ) );

    if( cmpFile == NULL )
    {
        wxString msg;
        msg.Printf( _( "File <%s> not found, use Netlist for footprints selection" ),
                   GetChars( m_cmplistFullName ) );

        if( m_messageWindow )
            m_messageWindow->AppendText( msg );
        return false;
    }

    // netlineReader dtor will close cmpFile
    FILE_LINE_READER netlineReader( cmpFile, m_cmplistFullName );
    wxString buffer;
    wxString value;

    while( netlineReader.ReadLine() )
    {
        buffer = FROM_UTF8( netlineReader.Line() );

        if( ! buffer.StartsWith( wxT("BeginCmp") ) )
            continue;

        // Begin component description.
        refcurrcmp.Empty();
        footprint.Empty();
        timestamp.Empty();

        while( netlineReader.ReadLine() )
        {
            buffer = FROM_UTF8( netlineReader.Line() );

            if( buffer.StartsWith( wxT("EndCmp") ) )
                break;

            // store string value, stored between '=' and ';' delimiters.
            value = buffer.AfterFirst( '=' );
            value = value.BeforeLast( ';');
            value.Trim(true);
            value.Trim(false);

            if( buffer.StartsWith( wxT("Reference") ) )
            {
                refcurrcmp = value;
                continue;
            }

            if( buffer.StartsWith( wxT("IdModule  =" ) ) )
            {
                footprint = value;
                continue;
            }

            if( buffer.StartsWith( wxT("TimeStamp =" ) ) )
            {
                timestamp = value;
                continue;
            }
        }

        // Find the corresponding item in module info list:
        for( unsigned ii = 0; ii < m_modulesInNetlist.size(); ii++ )
        {
            MODULE_INFO * mod_info = m_modulesInNetlist[ii];
            if( m_UseTimeStamp )    // Use schematic timestamp to locate the footprint
            {
                if( mod_info->m_TimeStamp.CmpNoCase( timestamp ) == 0  &&
                    !timestamp.IsEmpty() )
                {    // Found
                    if( !footprint.IsEmpty() )
                        mod_info->m_Footprint = footprint;
                    break;
                }
            }
            else                   // Use schematic reference to locate the footprint
            {
                if( mod_info->m_Reference.CmpNoCase( refcurrcmp ) == 0 )   // Found!
                {
                    if( !footprint.IsEmpty() )
                        mod_info->m_Footprint = footprint;
                    break;
                }
            }
        }
    }

    return true;
}


/* Function to sort the footprint list, used by loadNewModules.
 * the given list is sorted by name
 */
static bool SortByLibName( MODULE_INFO* ref, MODULE_INFO* cmp )
{
    int ii = ref->m_Footprint.CmpNoCase( cmp->m_Footprint );
    return ii > 0;
}


/* Load new modules from library.
 * If a new module is already loaded it is duplicated, which avoids multiple
 * unnecessary disk or net access to read libraries.
 * return false if a footprint is not found, true if OK
 */
bool NETLIST_READER::loadNewModules()
{
    MODULE_INFO* ref, * cmp;
    MODULE*      Module = NULL;
    wxPoint      ModuleBestPosition;
    BOARD*       pcb = m_pcbframe->GetBoard();
    bool         success = true;

    if( m_newModulesList.size() == 0 )
        return true;

    sort( m_newModulesList.begin(), m_newModulesList.end(), SortByLibName );

    // Calculate the footprint "best" position:
    EDA_RECT bbbox = pcb->ComputeBoundingBox( true );

    if( bbbox.GetWidth() || bbbox.GetHeight() )
    {
        ModuleBestPosition = bbbox.GetEnd();
        ModuleBestPosition.y += 5000;
    }

    ref = cmp = m_newModulesList[0];

    for( unsigned ii = 0; ii < m_newModulesList.size(); ii++ )
    {
        cmp = m_newModulesList[ii];

        if( (ii == 0) || ( ref->m_Footprint != cmp->m_Footprint) )
        {
            // New footprint : must be loaded from a library
            Module = m_pcbframe->GetModuleLibrary( wxEmptyString, cmp->m_Footprint, false );
            ref = cmp;

            if( Module == NULL )
            {
                success = false;
                if( m_messageWindow )
                {
                    wxString msg;
                    msg.Printf( _( "Component [%s]: footprint <%s> not found" ),
                                GetChars( cmp->m_Reference ),
                                GetChars( cmp->m_Footprint ) );

                    msg += wxT("\n");
                    m_messageWindow->AppendText( msg );
                }
                continue;
            }

            Module->SetPosition( ModuleBestPosition );

            /* Update schematic links : reference "Time Stamp" and schematic
             * hierarchical path */
            Module->m_Reference->m_Text = cmp->m_Reference;
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->SetPath( cmp->m_TimeStamp );
        }
        else
        {
            // Footprint already loaded from a library, duplicate it (faster)
            if( Module == NULL )
                continue;            // Module does not exist in library.

            MODULE* newmodule = new MODULE( *Module );
            newmodule->SetParent( pcb );

            pcb->Add( newmodule, ADD_APPEND );

            Module = newmodule;
            Module->m_Reference->m_Text = cmp->m_Reference;
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->SetPath( cmp->m_TimeStamp );
        }
    }

    return success;
}

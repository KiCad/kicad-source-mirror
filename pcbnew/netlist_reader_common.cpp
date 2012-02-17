/**
 * @file pcbnew/netlist_reader_common.cpp
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
#include <wxPcbStruct.h>
#include <richio.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <pcbnew.h>

#include <netlist_reader.h>

#include <algorithm>

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
    #define HEADER_PCBNEW "#EESchemaNetlist"
    #define HEADER_KICAD_NETFMT "(export"
    const std::string headers[HEADERS_COUNT] =
    {
        HEADER_ORCADPCB, HEADER_PCBNEW, HEADER_KICAD_NETFMT
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
            m_typeNetlist = NETLIST_TYPE_PCBNEW;
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

/**
 * Function GetComponentInfoList
 * @return a reference to the libpart info corresponding to a given part
 * @param aPartname = the name of the libpart
 */
LIPBART_INFO* NETLIST_READER::GetLibpart(const wxString & aPartname)
{
    for( unsigned ii = 0; ii < m_libpartList.size(); ii++ )
    {
        if( m_libpartList[ii]->m_Libpart == aPartname )
            return m_libpartList[ii];
    }

    return NULL;
}


bool NETLIST_READER::InitializeModules()
{
    if( m_UseCmpFile )  // Try to get footprint name from .cmp file
    {
        readModuleComponentLinkfile();
    }

    if( m_pcbframe == NULL )
        return true;

    for( unsigned ii = 0; ii < m_componentsInNetlist.size(); ii++ )
    {
        COMPONENT_INFO* currcmp_info = m_componentsInNetlist[ii];
        // Test if module is already loaded.
        wxString * idMod = m_UseTimeStamp?
                           &currcmp_info->m_TimeStamp : &currcmp_info->m_Reference;

        MODULE* module = FindModule( *idMod );
        if( module == NULL )   // not existing, load it
        {
            m_newModulesList.push_back( currcmp_info );
        }
    }

    bool success = loadNewModules();

    // Update modules fields
    for( unsigned ii = 0; ii < m_componentsInNetlist.size(); ii++ )
    {
        COMPONENT_INFO* currcmp_info =  m_componentsInNetlist[ii];
        // Test if module is already loaded.
        wxString * idMod = m_UseTimeStamp?
                              &currcmp_info->m_TimeStamp : &currcmp_info->m_Reference;

        MODULE* module = FindModule( *idMod );
        if( module )
        {
             // Update current module ( reference, value and "Time Stamp")
            module->SetReference( currcmp_info->m_Reference );
            module->SetValue(currcmp_info->m_Value );
            module->SetPath( currcmp_info->m_TimeStamp );
        }
        else   // not existing
        {
        }
    }

    // clear pads netnames
#if 1
    // Clear only footprints found in netlist:
    // This allow to have some footprints added by hand to the board
    // left initialized
    for( unsigned ii = 0; ii < m_componentsInNetlist.size(); ii++ )
    {
        COMPONENT_INFO* currcmp_info =  m_componentsInNetlist[ii];
        // We can used the reference to find the footprint, because
        // it is now updated
        wxString * idMod = &currcmp_info->m_Reference;

        MODULE* module = FindModule( *idMod );
        if( module )
        {
            for( D_PAD* pad = module->m_Pads; pad; pad = pad->Next() )
                pad->SetNetname( wxEmptyString );
        }
    }

#else
    // Clear all footprints
    for( MODULE* module = m_pcbframe->GetBoard()->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->m_Pads; pad; pad = pad->Next() )
            pad->SetNetname( wxEmptyString );
    }
#endif

    return success;
}

void NETLIST_READER::TestFootprintsMatchingAndExchange()
{
#ifdef PCBNEW

    // If a module is "exchanged", the new module is added to the end of
    // module list.

    // Calculates the module count
    int moduleCount = m_pcbframe->GetBoard()->m_Modules.GetCount();

    MODULE* nextmodule;
    MODULE *module = m_pcbframe->GetBoard()->m_Modules;
    for( ; module && moduleCount; module = nextmodule, moduleCount-- )
    {
        // Module can be deleted if exchanged, so store the next module.
        nextmodule = module->Next();

        // Search for the corresponding module info
        COMPONENT_INFO * cmp_info = NULL;
        for( unsigned ii = 0; ii < m_componentsInNetlist.size(); ii++ )
        {
            COMPONENT_INFO * candidate =  m_componentsInNetlist[ii];
            // Test if cmp_info matches the current module:
            if( candidate->m_Reference.CmpNoCase( module->GetReference() ) == 0 )
            {
                cmp_info = candidate;
                break;
            }
        }
        if( cmp_info == NULL )   // not found in netlist
             continue;

        if( module->GetLibRef().CmpNoCase( cmp_info->m_Footprint ) != 0 )
        {
            if( m_ChangeFootprints )   // footprint exchange allowed.
            {
                MODULE* newModule = m_pcbframe->GetModuleLibrary( wxEmptyString,
                                                                  cmp_info->m_Footprint,
                                                                  false );

                if( newModule )
                {
                    // Change old module to the new module (and delete the old one)
                    m_pcbframe->Exchange_Module( module, newModule, NULL );
                }
                else if( m_messageWindow )
                {
                wxString msg;
                msg.Printf( _( "Component \"%s\": module [%s] not found\n" ),
                            GetChars( cmp_info->m_Reference ),
                            GetChars( cmp_info->m_Footprint ) );

                m_messageWindow->AppendText( msg );
                }
            }
            else if( m_messageWindow )
            {
                wxString msg;
                msg.Printf( _( "Component \"%s\": Mismatch! module is [%s] and netlist said [%s]\n" ),
                            GetChars( cmp_info->m_Reference ),
                            GetChars( module->GetLibRef() ),
                            GetChars( cmp_info->m_Footprint ) );

                m_messageWindow->AppendText( msg );
            }
        }
    }
#endif
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
    if( m_pcbframe == NULL )
        return NULL;

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

        for( ii = 0; ii < m_componentsInNetlist.size(); ii++ )
        {
            COMPONENT_INFO* cmp_info = m_componentsInNetlist[ii];
            if( module->GetReference().CmpNoCase( cmp_info->m_Reference ) == 0 )
                break; // Module is found in net list.
        }

        if( ii == m_componentsInNetlist.size() )   // Module not found in netlist.
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
            if( aId.CmpNoCase( module->GetReference() ) == 0 )
                return module;
        }
    }

    return NULL;
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
        for( unsigned ii = 0; ii < m_componentsInNetlist.size(); ii++ )
        {
            COMPONENT_INFO * cmp_info = m_componentsInNetlist[ii];
            if( m_UseTimeStamp )    // Use schematic timestamp to locate the footprint
            {
                if( cmp_info->m_TimeStamp.CmpNoCase( timestamp ) == 0  &&
                    !timestamp.IsEmpty() )
                {    // Found
                    if( !footprint.IsEmpty() )
                        cmp_info->m_Footprint = footprint;
                    break;
                }
            }
            else                   // Use schematic reference to locate the footprint
            {
                if( cmp_info->m_Reference.CmpNoCase( refcurrcmp ) == 0 )   // Found!
                {
                    if( !footprint.IsEmpty() )
                        cmp_info->m_Footprint = footprint;
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
#ifdef PCBNEW
static bool SortByLibName( COMPONENT_INFO* ref, COMPONENT_INFO* cmp )
{
    int ii = ref->m_Footprint.CmpNoCase( cmp->m_Footprint );
    return ii > 0;
}
#endif

/* Load new modules from library.
 * If a new module is already loaded it is duplicated, which avoid multiple
 * unnecessary disk or net access to read libraries.
 * return false if a footprint is not found, true if OK
 */
bool NETLIST_READER::loadNewModules()
{
    bool         success = true;
#ifdef PCBNEW
    COMPONENT_INFO* ref_info, * cmp_info;
    MODULE*      Module = NULL;
    wxPoint      ModuleBestPosition;
    BOARD*       pcb = m_pcbframe->GetBoard();

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

    ref_info = cmp_info = m_newModulesList[0];

    for( unsigned ii = 0; ii < m_newModulesList.size(); ii++ )
    {
        cmp_info = m_newModulesList[ii];

        if( (ii == 0) || ( ref_info->m_Footprint != cmp_info->m_Footprint) )
        {
            // New footprint : must be loaded from a library
            Module = m_pcbframe->GetModuleLibrary( wxEmptyString,
                                                   cmp_info->m_Footprint, false );
            ref_info = cmp_info;

            if( Module == NULL )
            {
                success = false;
                if( m_messageWindow )
                {
                    wxString msg;
                    msg.Printf( _( "Component [%s]: footprint <%s> not found" ),
                                GetChars( cmp_info->m_Reference ),
                                GetChars( cmp_info->m_Footprint ) );

                    msg += wxT("\n");
                    m_messageWindow->AppendText( msg );
                }
                continue;
            }

            Module->SetPosition( ModuleBestPosition );

            /* Update schematic links : reference "Time Stamp" and schematic
             * hierarchical path */
            Module->SetReference( cmp_info->m_Reference );
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->SetPath( cmp_info->m_TimeStamp );
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
            Module->SetReference( cmp_info->m_Reference );
            Module->SetTimeStamp( GetNewTimeStamp() );
            Module->SetPath( cmp_info->m_TimeStamp );
        }
    }
#endif
    return success;
}

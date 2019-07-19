/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.TXT for contributors.
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

#include "netlist_exporter_pspice.h"
#include <fctsys.h>
#include <build_version.h>
#include <confirm.h>

#include <map>
#include <search_stack.h>

#include <sch_edit_frame.h>
#include <netlist.h>
#include <sch_reference_list.h>
#include <env_paths.h>

#include <wx/tokenzr.h>
#include <wx/regex.h>


wxString NETLIST_EXPORTER_PSPICE::GetSpiceDevice( const wxString& aComponent ) const
{
    const auto& spiceItems = GetSpiceItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(), [&]( const SPICE_ITEM& item ) {
        return item.m_refName == aComponent;
    } );

    if( it == spiceItems.end() )
        return wxEmptyString;

    // Prefix the device type if plain reference would result in a different device type
    return it->m_primitive != it->m_refName[0] ?
        wxString( it->m_primitive + it->m_refName ) : it->m_refName;
}


bool NETLIST_EXPORTER_PSPICE::WriteNetlist( const wxString& aOutFileName, unsigned aNetlistOptions )
{
    FILE_OUTPUTFORMATTER outputFile( aOutFileName, wxT( "wt" ), '\'' );

    return Format( &outputFile, aNetlistOptions );
}

void  NETLIST_EXPORTER_PSPICE::ReplaceForbiddenChars( wxString &aNetName )
{
    // some chars are not accepted in netnames in spice netlists, because they are separators
    // they are replaced an underscore or some other allowed char.
    // Note: this is a static function

    aNetName.Replace( "(", "_" );
    aNetName.Replace( ")", "_" );
    aNetName.Replace( " ", "_" );
}


bool NETLIST_EXPORTER_PSPICE::Format( OUTPUTFORMATTER* aFormatter, unsigned aCtl )
{
    // Netlist options
    const bool useNetcodeAsNetName = false;//aCtl & NET_USE_NETCODES_AS_NETNAMES;

    // default title
    m_title = "KiCad schematic";

    if( !ProcessNetlist( aCtl ) )
        return false;

    aFormatter->Print( 0, ".title %s\n", (const char*) m_title.c_str() );

    // Write .include directives
    for( const auto& lib : m_libraries )
    {
        wxString full_path;

        if( ( aCtl & NET_ADJUST_INCLUDE_PATHS ) )
        {
            // Look for the library in known search locations
            full_path = ResolveFile( lib, &Pgm().GetLocalEnvVariables(), m_project );

            if( full_path.IsEmpty() )
            {
                DisplayError( NULL, wxString::Format( _( "Could not find library file %s" ), lib ) );
                full_path = lib;
            }
        }
        else
            full_path = lib;    // just use the unaltered path

        aFormatter->Print( 0, ".include \"%s\"\n", (const char*) full_path.c_str() );
    }

    unsigned int NC_counter = 1;

    for( const auto& item : m_spiceItems )
    {
        if( !item.m_enabled )
            continue;

        wxString device = GetSpiceDevice( item.m_refName );
        aFormatter->Print( 0, "%s ", (const char*) device.c_str() );

        size_t pspiceNodes = item.m_pinSequence.empty() ? item.m_pins.size() : item.m_pinSequence.size();

        for( size_t ii = 0; ii < pspiceNodes; ii++ )
        {
            // Use the custom order if defined, otherwise use the standard pin order as defined in the compon
            size_t activePinIndex = item.m_pinSequence.empty() ? ii : item.m_pinSequence[ii];
            // Valid used Node Indexes are in the set
            // {0,1,2,...m_item.m_pin.size()-1}
            if( activePinIndex >= item.m_pins.size() )
            {
                wxASSERT_MSG( false, "Used an invalid pin number in node sequence" );
                continue;
            }

            NETLIST_OBJECT* pin = item.m_pins[activePinIndex];
            assert( pin );
            wxString netName = pin->GetNetName();

            if( useNetcodeAsNetName )
            {
                assert( m_netMap.count( netName ) );
                aFormatter->Print( 0, "%d ", m_netMap[netName] );
            }
            else
            {
                sprintPinNetName( netName , wxT( "N-%.6d" ), pin, useNetcodeAsNetName );

                // Replace parenthesis with underscore to prevent parse issues with simulators
                ReplaceForbiddenChars( netName );

                // unescape net names that contain a escaped sequence ("{slash}"):
                netName = UnescapeString( netName );

                // Borrow LTSpice's nomenclature for unconnected nets
                if( netName.IsEmpty() )
                    netName = wxString::Format( wxT( "NC_%.2u" ), NC_counter++ );

                aFormatter->Print( 0, "%s ", TO_UTF8( netName ) );
            }
        }

        aFormatter->Print( 0, "%s\n", (const char*) item.m_model.c_str() );
    }

    // Print out all directives found in the text fields on the schematics
    writeDirectives( aFormatter, aCtl );

    aFormatter->Print( 0, ".end\n" );

    return true;
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceField( SPICE_FIELD aField,
        SCH_COMPONENT* aComponent, unsigned aCtl )
{
    SCH_FIELD* field = aComponent->FindField( GetSpiceFieldName( aField ) );
    return field ? field->GetText() : GetSpiceFieldDefVal( aField, aComponent, aCtl );
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceFieldDefVal( SPICE_FIELD aField,
        SCH_COMPONENT* aComponent, unsigned aCtl )
{
    switch( aField )
    {
    case SF_PRIMITIVE:
    {
        const wxString refName = aComponent->GetField( REFERENCE )->GetText();
        return refName.GetChar( 0 );
        break;
    }

    case SF_MODEL:
    {
        wxChar prim = aComponent->GetField( REFERENCE )->GetText().GetChar( 0 );
        wxString value = aComponent->GetField( VALUE )->GetText();

        // Is it a passive component?
        if( aCtl & NET_ADJUST_PASSIVE_VALS && ( prim == 'C' || prim == 'L' || prim == 'R' ) )
        {
            // Regular expression to match common formats used for passive parts description
            // (e.g. 100k, 2k3, 1 uF)
            wxRegEx passiveVal( "^([0-9\\. ]+)([fFpPnNuUmMkKgGtT]|M(e|E)(g|G))?([fFhH]|ohm)?([-1-9 ]*)$" );

            if( passiveVal.Matches( value ) )
            {
                wxString prefix( passiveVal.GetMatch( value, 1 ) );
                wxString unit( passiveVal.GetMatch( value, 2 ) );
                wxString suffix( passiveVal.GetMatch( value, 6 ) );

                prefix.Trim(); prefix.Trim( false );
                unit.Trim(); unit.Trim( false );
                suffix.Trim(); suffix.Trim( false );

                // Make 'mega' units comply with the Spice expectations
                if( unit == "M" )
                    unit = "Meg";

                value = prefix + unit + suffix;
            }
        }

        return value;
        break;
    }

    case SF_ENABLED:
        return wxString( "Y" );
        break;

    case SF_NODE_SEQUENCE:
    {
        wxString nodeSeq;
        std::vector<LIB_PIN*> pins;

        auto part = aComponent->GetPartRef().lock();
        wxCHECK( part, wxString() );
        part->GetPins( pins );

        for( auto pin : pins )
            nodeSeq += pin->GetNumber() + " ";

        nodeSeq.Trim();

        return nodeSeq;
        break;
    }

    case SF_LIB_FILE:
        // There is no default Spice library
        return wxEmptyString;
        break;

    default:
        wxASSERT_MSG( false, "Missing default value definition for a Spice field" );
        break;
    }


    return wxString( "<unknown>" );
}


bool NETLIST_EXPORTER_PSPICE::ProcessNetlist( unsigned aCtl )
{
    const wxString      delimiters( "{:,; }" );
    SCH_SHEET_LIST      sheetList( g_RootSheet );
    // Set of reference names, to check for duplications
    std::set<wxString>  refNames;

    // Prepare list of nets generation (not used here, but...
    for( unsigned ii = 0; ii < m_masterList->size(); ii++ )
        m_masterList->GetItem( ii )->m_Flag = 0;

    m_netMap.clear();
    m_netMap["GND"] = 0;        // 0 is reserved for "GND"
    int netIdx = 1;

    m_libraries.clear();
    m_ReferencesAlreadyFound.Clear();

    UpdateDirectives( aCtl );

    for( unsigned sheet_idx = 0; sheet_idx < sheetList.size(); sheet_idx++ )
    {
        // Process component attributes to find Spice directives
        for( EDA_ITEM* item = sheetList[sheet_idx].LastDrawList(); item; item = item->Next() )
        {
            SCH_COMPONENT* comp = findNextComponentAndCreatePinList( item, &sheetList[sheet_idx] );

            if( !comp )
                break;

            item = comp;

            SPICE_ITEM spiceItem;
            spiceItem.m_parent = comp;

            // Obtain Spice fields
            SCH_FIELD* fieldLibFile = comp->FindField( GetSpiceFieldName( SF_LIB_FILE ) );
            SCH_FIELD* fieldSeq = comp->FindField( GetSpiceFieldName( SF_NODE_SEQUENCE ) );

            spiceItem.m_primitive = GetSpiceField( SF_PRIMITIVE, comp, aCtl )[0];
            spiceItem.m_model = GetSpiceField( SF_MODEL, comp, aCtl );
            spiceItem.m_refName = comp->GetRef( &sheetList[sheet_idx] );

            // Duplicate references will result in simulation errors
            if( refNames.count( spiceItem.m_refName ) )
            {
                DisplayError( NULL, wxT( "There are duplicate components. "
                            "You need to annotate schematics first." ) );
                return false;
            }

            refNames.insert( spiceItem.m_refName );

            // Check to see if component should be removed from Spice netlist
            spiceItem.m_enabled = StringToBool( GetSpiceField( SF_ENABLED, comp, aCtl ) );

            if( fieldLibFile && !fieldLibFile->GetText().IsEmpty() )
                m_libraries.insert( fieldLibFile->GetText() );

            wxArrayString pinNames;

            // Store pin information
            for( unsigned ii = 0; ii < m_SortedComponentPinList.size(); ii++ )
            {
                NETLIST_OBJECT* pin = m_SortedComponentPinList[ii];

                // NETLIST_EXPORTER marks removed pins by setting them to NULL
                if( !pin )
                    continue;

                spiceItem.m_pins.push_back( pin );
                pinNames.Add( pin->GetPinNumText() );

                // Create net mapping
                const wxString& netName = pin->GetNetName();
                if( m_netMap.count( netName ) == 0 )
                    m_netMap[netName] = netIdx++;
            }

            // Check if an alternative pin sequence is available:
            if( fieldSeq )
            {
                // Get the string containing the sequence of nodes:
                const wxString nodeSeqIndexLineStr = fieldSeq->GetText();

                // Verify field exists and is not empty:
                if( !nodeSeqIndexLineStr.IsEmpty() )
                {
                    // Get Alt Pin Name Array From User:
                    wxStringTokenizer tkz( nodeSeqIndexLineStr, delimiters );

                    while( tkz.HasMoreTokens() )
                    {
                        wxString    pinIndex = tkz.GetNextToken();
                        int         seq;

                        // Find PinName In Standard List assign Standard List Index to Name:
                        seq = pinNames.Index( pinIndex );

                        if( seq != wxNOT_FOUND )
                            spiceItem.m_pinSequence.push_back( seq );
                    }
                }
            }

            m_spiceItems.push_back( spiceItem );
        }
    }

    return true;
}


void NETLIST_EXPORTER_PSPICE::UpdateDirectives( unsigned aCtl )
{
    const SCH_SHEET_LIST& sheetList = g_RootSheet;
    wxRegEx couplingK( "^[kK][[:digit:]]*[[:space:]]+[[:alnum:]]+[[:space:]]+[[:alnum:]]+",
            wxRE_ADVANCED );

    m_directives.clear();
    bool controlBlock = false;

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( EDA_ITEM* item = sheetList[i].LastDrawList(); item; item = item->Next() )
        {
            if( item->Type() != SCH_TEXT_T )
                continue;

            wxString text = static_cast<SCH_TEXT*>( item )->GetText();

            if( text.IsEmpty() )
                continue;

            // Analyze each line of a text field
            wxStringTokenizer tokenizer( text, "\r\n" );

            // Flag to follow multiline directives
            bool directiveStarted = false;

            while( tokenizer.HasMoreTokens() )
            {
                wxString line( tokenizer.GetNextToken() );

                // Cleanup: remove preceding and trailing white-space characters
                line.Trim( true ).Trim( false );
                // Convert to lower-case for parsing purposes only
                wxString lowercaseline = line;
                lowercaseline.MakeLower();

                // 'Include' directive stores the library file name, so it
                // can be later resolved using a list of paths
                if( lowercaseline.StartsWith( ".inc" ) )
                {
                    wxString lib = line.AfterFirst( ' ' );

                    if( lib.IsEmpty() )
                        continue;

                    // Strip quotes if present
                    if( ( lib.StartsWith( "\"" ) && lib.EndsWith( "\"" ) )
                        || ( lib.StartsWith( "'" ) && lib.EndsWith( "'" ) ) )
                    {
                        lib = lib.Mid( 1, lib.Length() - 2 );
                    }

                    m_libraries.insert( lib );
                }

                // Store the title to be sure it appears
                // in the first line of output
                else if( lowercaseline.StartsWith( ".title " ) )
                {
                    m_title = line.AfterFirst( ' ' );
                }

                // Handle .control .. .endc blocks
                else if( lowercaseline.IsSameAs( ".control" ) && ( !controlBlock ) )
                {
                    controlBlock = true;
                    m_directives.push_back( line );
                }
                else if( lowercaseline.IsSameAs( ".endc" ) && controlBlock )
                {
                    controlBlock = false;
                    m_directives.push_back( line );
                }

                else if( line.StartsWith( '.' )                           // one-line directives
                        || controlBlock                                   // .control .. .endc block
                        || couplingK.Matches( line )                      // K## L## L## coupling constant
                        || ( directiveStarted && line.StartsWith( '+' ) ) ) // multiline directives
                {
                    m_directives.push_back( line );
                }

                // Mark directive as started or continued in case it is a multi-line one
                directiveStarted = line.StartsWith( '.' )
                    || ( directiveStarted && line.StartsWith( '+' ) );
            }
        }
    }
}


void NETLIST_EXPORTER_PSPICE::writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const
{
    for( auto& dir : m_directives )
    {
        aFormatter->Print( 0, "%s\n", (const char*) dir.c_str() );
    }
}


// Entries in the vector below have to follow the order in SPICE_FIELD enum
const std::vector<wxString> NETLIST_EXPORTER_PSPICE::m_spiceFields = {
    "Spice_Primitive",
    "Spice_Model",
    "Spice_Netlist_Enabled",
    "Spice_Node_Sequence",
    "Spice_Lib_File"
};

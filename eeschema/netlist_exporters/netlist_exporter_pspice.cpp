/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2013 jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.TXT for contributors.
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
#include <build_version.h>
#include <confirm.h>

#include <map>
#include <search_stack.h>

#include <connection_graph.h>
#include <kicad_string.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <env_paths.h>
#include <pgm_base.h>
#include <common.h>

#include <wx/tokenzr.h>
#include <wx/regex.h>


wxString NETLIST_EXPORTER_PSPICE::GetSpiceDevice( const wxString& aSymbol ) const
{
    const std::list<SPICE_ITEM>& spiceItems = GetSpiceItems();

    auto it = std::find_if( spiceItems.begin(), spiceItems.end(),
                            [&]( const SPICE_ITEM& item )
                            {
                                return item.m_refName == aSymbol;
                            } );

    if( it == spiceItems.end() )
        return wxEmptyString;

    // Prefix the device type if plain reference would result in a different device type
    return it->m_primitive != it->m_refName[0] ? wxString( it->m_primitive + it->m_refName )
                                               : it->m_refName;
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

    aFormatter->Print( 0, ".title %s\n", TO_UTF8( m_title ) );

    // Write .include directives
    for( const wxString& curr_lib : m_libraries )
    {
        // First, expand env vars, if any
        wxString libname = ExpandEnvVarSubstitutions( curr_lib, &m_schematic->Prj() );
        wxString full_path;

        if( ( aCtl & NET_ADJUST_INCLUDE_PATHS ) )
        {
            // Look for the library in known search locations
            full_path = ResolveFile( libname, &Pgm().GetLocalEnvVariables(), &m_schematic->Prj() );

            if( full_path.IsEmpty() )
            {
                DisplayError( nullptr, wxString::Format( _( "Could not find library file %s." ),
                                                         libname ) );
                full_path = libname;
            }
        }
        else
        {
            full_path = libname;    // just use the unaltered path
        }

        aFormatter->Print( 0, ".include \"%s\"\n", TO_UTF8( full_path ) );
    }

    unsigned int NC_counter = 1;

    for( const SPICE_ITEM& item : m_spiceItems )
    {
        if( !item.m_enabled )
            continue;

        wxString device = GetSpiceDevice( item.m_refName );
        aFormatter->Print( 0, "%s ", TO_UTF8( device ) );

        size_t pspiceNodes =
                item.m_pinSequence.empty() ? item.m_pins.size() : item.m_pinSequence.size();

        for( size_t ii = 0; ii < pspiceNodes; ii++ )
        {
            // Use the custom order if defined, otherwise use the standard pin order as defined
            // in the symbol.
            size_t activePinIndex = item.m_pinSequence.empty() ? ii : item.m_pinSequence[ii];

            // Valid used Node Indexes are in the set
            // {0,1,2,...m_item.m_pin.size()-1}
            if( activePinIndex >= item.m_pins.size() )
            {
                wxASSERT_MSG( false, "Used an invalid pin number in node sequence" );
                continue;
            }

            wxString netName = item.m_pins[activePinIndex];

            wxASSERT( m_netMap.count( netName ) );

            if( useNetcodeAsNetName )
            {
                aFormatter->Print( 0, "%d ", m_netMap[netName] );
            }
            else
            {
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

        aFormatter->Print( 0, "%s\n", TO_UTF8( item.m_model ) );
    }

    // Print out all directives found in the text fields on the schematics
    writeDirectives( aFormatter, aCtl );

    aFormatter->Print( 0, ".end\n" );

    return true;
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceField( SPICE_FIELD aField, SCH_SYMBOL* aSymbol,
                                                 unsigned aCtl )
{
    SCH_FIELD* field = aSymbol->FindField( GetSpiceFieldName( aField ) );
    return field ? field->GetShownText() : GetSpiceFieldDefVal( aField, aSymbol, aCtl );
}


wxString NETLIST_EXPORTER_PSPICE::GetSpiceFieldDefVal( SPICE_FIELD aField, SCH_SYMBOL* aSymbol,
                                                       unsigned aCtl )
{
    switch( aField )
    {
    case SF_PRIMITIVE:
    {
        const wxString refName = aSymbol->GetField( REFERENCE_FIELD )->GetShownText();
        return refName.GetChar( 0 );
    }

    case SF_MODEL:
    {
        wxChar prim = aSymbol->GetField( REFERENCE_FIELD )->GetShownText().GetChar( 0 );
        wxString value = aSymbol->GetField( VALUE_FIELD )->GetShownText();

        // Is it a passive component?
        if( aCtl & NET_ADJUST_PASSIVE_VALS && ( prim == 'C' || prim == 'L' || prim == 'R' ) )
        {
            // Regular expression to match common formats used for passive parts description
            // (e.g. 100k, 2k3, 1 uF)
            wxRegEx passiveVal(
                    "^([0-9\\. ]+)([fFpPnNuUmMkKgGtT]|M(e|E)(g|G))?([fFhH]|ohm)?([-1-9 ]*)$" );

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
    }

    case SF_ENABLED:
        return wxString( "Y" );

    case SF_NODE_SEQUENCE:
    {
        wxString nodeSeq;
        std::vector<LIB_PIN*> pins;

        wxCHECK( aSymbol->GetLibSymbolRef(), wxString() );
        aSymbol->GetLibSymbolRef()->GetPins( pins );

        for( LIB_PIN* pin : pins )
            nodeSeq += pin->GetNumber() + " ";

        nodeSeq.Trim();

        return nodeSeq;
    }

    case SF_LIB_FILE:
        // There is no default Spice library
        return wxEmptyString;

    default:
        wxASSERT_MSG( false, "Missing default value definition for a Spice field." );
        return wxString( "<unknown>" );
    }
}


bool NETLIST_EXPORTER_PSPICE::ProcessNetlist( unsigned aCtl )
{
    const wxString     delimiters( "{:,; }" );
    SCH_SHEET_LIST     sheetList = m_schematic->GetSheets();
    std::set<wxString> refNames;       // Set of reference names, to check for duplication

    m_netMap.clear();
    m_netMap["GND"] = 0;        // 0 is reserved for "GND"
    int netIdx = 1;

    m_libraries.clear();
    m_referencesAlreadyFound.Clear();
    m_libParts.clear();

    UpdateDirectives( aCtl );

    for( unsigned sheet_idx = 0; sheet_idx < sheetList.size(); sheet_idx++ )
    {
        SCH_SHEET_PATH sheet = sheetList[sheet_idx];

        // Process symbol attributes to find Spice directives
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, &sheet );

            if( !symbol )
                continue;

            CreatePinList( symbol, &sheet, true );
            SPICE_ITEM spiceItem;
            spiceItem.m_parent = symbol;

            // Obtain Spice fields
            SCH_FIELD* fieldLibFile = symbol->FindField( GetSpiceFieldName( SF_LIB_FILE ) );
            SCH_FIELD* fieldSeq     = symbol->FindField( GetSpiceFieldName( SF_NODE_SEQUENCE ) );

            spiceItem.m_primitive = GetSpiceField( SF_PRIMITIVE, symbol, aCtl )[0];
            spiceItem.m_model     = GetSpiceField( SF_MODEL, symbol, aCtl );
            spiceItem.m_refName   = symbol->GetRef( &sheet );

            // Duplicate references will result in simulation errors
            if( refNames.count( spiceItem.m_refName ) )
            {
                DisplayError( nullptr, _( "Multiple symbols have the same reference designator.\n"
                                          "Annotation must be corrected before simulating." ) );
                return false;
            }

            refNames.insert( spiceItem.m_refName );

            // Check to see if symbol should be removed from Spice netlist
            spiceItem.m_enabled = StringToBool( GetSpiceField( SF_ENABLED, symbol, aCtl ) );

            if( fieldLibFile && !fieldLibFile->GetShownText().IsEmpty() )
                m_libraries.insert( fieldLibFile->GetShownText() );

            wxArrayString pinNames;

            // Store pin information
            for( const PIN_INFO& pin : m_sortedSymbolPinList )
            {
                // Create net mapping
                spiceItem.m_pins.push_back( pin.netName );
                pinNames.Add( pin.num );

                if( m_netMap.count( pin.netName ) == 0 )
                    m_netMap[pin.netName] = netIdx++;
            }

            // Check if an alternative pin sequence is available:
            if( fieldSeq )
            {
                // Get the string containing the sequence of nodes:
                const wxString& nodeSeqIndexLineStr = fieldSeq->GetShownText();

                // Verify field exists and is not empty:
                if( !nodeSeqIndexLineStr.IsEmpty() )
                {
                    // Get Alt Pin Name Array From User:
                    wxStringTokenizer tkz( nodeSeqIndexLineStr, delimiters );

                    while( tkz.HasMoreTokens() )
                    {
                        wxString pinIndex = tkz.GetNextToken();
                        int      seq;

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
    const SCH_SHEET_LIST& sheetList = m_schematic->GetSheets();
    wxRegEx couplingK( "^[kK][[:digit:]]*[[:space:]]+[[:alnum:]]+[[:space:]]+[[:alnum:]]+",
                       wxRE_ADVANCED );

    m_directives.clear();
    bool controlBlock = false;
    bool circuitBlock = false;

    for( unsigned i = 0; i < sheetList.size(); i++ )
    {
        for( SCH_ITEM* item : sheetList[i].LastScreen()->Items().OfType( SCH_TEXT_T ) )
        {
            wxString text = static_cast<SCH_TEXT*>( item )->GetShownText();

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

                else if( line.StartsWith( '.' )                    // one-line directives
                        || controlBlock                            // .control .. .endc block
                        || circuitBlock                            // .subckt  .. .ends block
                        || couplingK.Matches( line )               // K## L## L## coupling constant
                        || ( directiveStarted && line.StartsWith( '+' ) ) ) // multiline directives
                {
                    m_directives.push_back( line );
                }

                // Handle .control .. .endc blocks
                if( lowercaseline.IsSameAs( ".control" ) && ( !controlBlock ) )
                    controlBlock = true;

                if( lowercaseline.IsSameAs( ".endc" ) && controlBlock )
                    controlBlock = false;

                // Handle .subckt .. .ends blocks
                if( lowercaseline.StartsWith( ".subckt" ) && ( !circuitBlock ) )
                    circuitBlock = true;

                if( lowercaseline.IsSameAs( ".ends" ) && circuitBlock )
                    circuitBlock = false;

                // Mark directive as started or continued in case it is a multi-line one
                directiveStarted = line.StartsWith( '.' )
                                        || ( directiveStarted && line.StartsWith( '+' ) );
            }
        }
    }
}


void NETLIST_EXPORTER_PSPICE::writeDirectives( OUTPUTFORMATTER* aFormatter, unsigned aCtl ) const
{
    for( const wxString& dir : m_directives )
        aFormatter->Print( 0, "%s\n", TO_UTF8( dir ) );
}


// Entries in the vector below have to follow the order in SPICE_FIELD enum
const std::vector<wxString> NETLIST_EXPORTER_PSPICE::m_spiceFields = {
    "Spice_Primitive",
    "Spice_Model",
    "Spice_Netlist_Enabled",
    "Spice_Node_Sequence",
    "Spice_Lib_File"
};

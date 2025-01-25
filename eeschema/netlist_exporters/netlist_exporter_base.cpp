/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2017 jp.charras at wanadoo.fr
 * Copyright (C) 2013-2017 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <netlist_exporter_base.h>

#include <string_utils.h>

#include <trace_helpers.h>
#include <connection_graph.h>
#include <sch_reference_list.h>
#include <sch_screen.h>
#include <schematic.h>


// a "less than" test on two LIB_SYMBOLs (.m_name wxStrings)
bool LIB_SYMBOL_LESS_THAN::operator()( LIB_SYMBOL* const& libsymbol1,
                                       LIB_SYMBOL* const& libsymbol2 ) const
{
    // Use case specific GetName() wxString compare
    return libsymbol1->GetLibId() < libsymbol2->GetLibId();
}


wxString NETLIST_EXPORTER_BASE::MakeCommandLine( const wxString& aFormatString,
                                                 const wxString& aNetlistFile,
                                                 const wxString& aFinalFile,
                                                 const wxString& aProjectPath )
{
    // Expand format symbols in the command line:
    // %B => base filename of selected output file, minus path and extension.
    // %P => project directory name, without trailing '/' or '\'.
    // %I => full filename of the input file (the intermediate net file).
    // %O => complete filename and path (but without extension) of the user chosen output file.

    wxString   ret  = aFormatString;
    wxFileName in   = aNetlistFile;
    wxFileName out  = aFinalFile;
    wxString str_out  = out.GetFullPath();

    ret.Replace( "%P", aProjectPath, true );
    ret.Replace( "%B", out.GetName(), true );
    ret.Replace( "%I", in.GetFullPath(), true );

#ifdef __WINDOWS__
    // A ugly hack to run xsltproc that has a serious bug on Window since a long time:
    // the filename given after -o option (output filename) cannot use '\' in filename
    // so replace if by '/' if possible (I mean if the filename does not start by "\\"
    // that is a filename on a Windows server)

    if( !str_out.StartsWith( "\\\\" ) )
        str_out.Replace( "\\", "/" );
#endif

    ret.Replace( "%O", str_out, true );

    return ret;
}


SCH_SYMBOL* NETLIST_EXPORTER_BASE::findNextSymbol( EDA_ITEM* aItem,
                                                   const SCH_SHEET_PATH& aSheetPath )
{
    wxCHECK( aItem, nullptr );

    wxString ref;

    if( aItem->Type() != SCH_SYMBOL_T )
        return nullptr;

    // found next symbol
    SCH_SYMBOL* symbol = (SCH_SYMBOL*) aItem;

    // Power symbols and other symbols which have the reference starting with "#" are not
    // included in netlist (pseudo or virtual symbols)
    ref = symbol->GetRef( &aSheetPath );

    if( ref[0] == wxChar( '#' ) )
        return nullptr;

    SCH_SCREEN* screen = aSheetPath.LastScreen();

    wxCHECK( screen, nullptr );

    auto it = screen->GetLibSymbols().find( symbol->GetSchSymbolLibraryName() );

    if( it == screen->GetLibSymbols().end() )
        return nullptr;

    LIB_SYMBOL* libSymbol = it->second;

    // If symbol is a "multi parts per package" type
    if( libSymbol->GetUnitCount() > 1 )
    {
        // test if this reference has already been processed, and if so skip
        if( m_referencesAlreadyFound.Lookup( ref ) )
            return nullptr;
    }

    // record the usage of this library symbol entry.
    m_libParts.insert( libSymbol ); // rejects non-unique pointers

    return symbol;
}


std::vector<PIN_INFO> NETLIST_EXPORTER_BASE::CreatePinList( SCH_SYMBOL* aSymbol,
                                                            const SCH_SHEET_PATH& aSheetPath,
                                                            bool aKeepUnconnectedPins )
{
    std::vector<PIN_INFO> pins;

    if( !aSymbol )
        return pins;

    wxString ref( aSymbol->GetRef( &aSheetPath ) );

    // Power symbols and other symbols which have the reference starting with "#" are not
    // included in netlist (pseudo or virtual symbols)
    if( ( ref[0] == wxChar( '#' ) ) || aSymbol->IsPower() )
        return pins;

    // if( aSymbol->m_FlagControlMulti == 1 )
    //    continue;                                      /* yes */
    // removed because with multiple instances of one schematic (several sheets pointing to
    // 1 screen), this will be erroneously be toggled.

    if( !aSymbol->GetLibSymbolRef() )
        return pins;

    // If symbol is a "multi parts per package" type
    if( aSymbol->GetLibSymbolRef()->GetUnitCount() > 1 )
    {
        // Collect all pins for this reference designator by searching the entire design for
        // other parts with the same reference designator.
        findAllUnitsOfSymbol( aSymbol, aSheetPath, pins, aKeepUnconnectedPins );
        wxLogTrace( traceStackedPins, "CreatePinList(multi): ref='%s' pins=%zu", aSymbol->GetRef( &aSheetPath ), pins.size() );
    }

    else // GetUnitCount() <= 1 means one part per package
    {
        CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

        for( const SCH_PIN* pin : aSymbol->GetPins( &aSheetPath ) )
        {
            if( SCH_CONNECTION* conn = pin->Connection( &aSheetPath ) )
            {
                const wxString& netName = conn->Name();

                if( !aKeepUnconnectedPins )     // Skip unconnected pins if requested
                {
                    CONNECTION_SUBGRAPH* sg = graph->FindSubgraphByName( netName, aSheetPath );

                    if( !sg || sg->GetNoConnect() || sg->GetItems().size() < 2 )
                        continue;
                }

                bool                  valid;
                std::vector<wxString> numbers = pin->GetStackedPinNumbers( &valid );
                wxString              baseName = pin->GetShownName();
                wxLogTrace( traceStackedPins,
                            wxString::Format( "CreatePinList(single): ref='%s' pinNameBase='%s' shownNum='%s' net='%s' "
                                              "valid=%d expand=%zu",
                                              ref, baseName, pin->GetShownNumber(), netName, valid, numbers.size() ) );

                for( const wxString& num : numbers )
                {
                    wxString pinName = baseName.IsEmpty() ? num : baseName + wxT( "_" ) + num;
                    wxLogTrace( traceStackedPins,
                                wxString::Format( " -> emit pin num='%s' name='%s' net='%s'", num, pinName, netName ) );
                    pins.emplace_back( num, netName, pinName );
                }
            }
        }
    }

    // Sort pins in m_SortedSymbolPinList by pin number
    std::sort( pins.begin(), pins.end(),
               []( const PIN_INFO& lhs, const PIN_INFO& rhs )
               {
                   return StrNumCmp( lhs.num, rhs.num, true ) < 0;
               } );

    // Remove duplicate Pins in m_SortedSymbolPinList
    eraseDuplicatePins( pins );

    // record the usage of this library symbol
    m_libParts.insert( aSymbol->GetLibSymbolRef().get() ); // rejects non-unique pointers

    return pins;
}


void NETLIST_EXPORTER_BASE::eraseDuplicatePins( std::vector<PIN_INFO>& aPins )
{
    for( unsigned ii = 0; ii < aPins.size(); ii++ )
    {
        if( aPins[ii].num.empty() ) /* already deleted */
            continue;

        /* Search for duplicated pins
         * If found, remove duplicates. The priority is to keep connected pins
         * and remove unconnected
         * - So this allows (for instance when using multi op amps per package
         * - to connect only one op amp to power
         * Because the pin list is sorted by m_PinNum value, duplicated pins
         * are necessary successive in list
         */
        int idxref = ii;

        for( unsigned jj = ii + 1; jj < aPins.size(); jj++ )
        {
            if(  aPins[jj].num.empty() )   // Already removed
                continue;

            // if other pin num, stop search,
            // because all pins having the same number are consecutive in list.
            if( aPins[idxref].num != aPins[jj].num )
                break;

            aPins[jj].num.clear();
        }
    }
}


void NETLIST_EXPORTER_BASE::findAllUnitsOfSymbol( SCH_SYMBOL* aSchSymbol,
                                                  const SCH_SHEET_PATH& aSheetPath,
                                                  std::vector<PIN_INFO>& aPins,
                                                  bool aKeepUnconnectedPins )
{
    wxString ref = aSchSymbol->GetRef( &aSheetPath );
    wxString ref2;

    CONNECTION_GRAPH* graph = m_schematic->ConnectionGraph();

    for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
    {
        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* comp2 = static_cast<SCH_SYMBOL*>( item );

            ref2 = comp2->GetRef( &sheet );

            if( ref2.CmpNoCase( ref ) != 0 )
                continue;

            for( const SCH_PIN* pin : comp2->GetPins( &sheet ) )
            {
                if( SCH_CONNECTION* conn = pin->Connection( &sheet ) )
                {
                    const wxString& netName = conn->Name();

                    if( !aKeepUnconnectedPins )     // Skip unconnected pins if requested
                    {
                        CONNECTION_SUBGRAPH* sg = graph->FindSubgraphByName( netName, sheet );

                        if( !sg || sg->GetNoConnect() || sg->GetItems().size() < 2 )
                            continue;
                    }

                    bool                        valid;
                    std::vector<wxString> numbers = pin->GetStackedPinNumbers( &valid );
                    wxString                     baseName = pin->GetShownName();
                    wxLogTrace( traceStackedPins,
                               wxString::Format( "CreatePinList(multi): ref='%s' pinNameBase='%s' shownNum='%s' net='%s' valid=%d expand=%zu",
                                                 ref2, baseName, pin->GetShownNumber(), netName, valid, numbers.size() ) );

                    for( const wxString& num : numbers )
                    {
                        wxString pinName = baseName.IsEmpty() ? num : baseName + wxT( "_" ) + num;
                        wxLogTrace( traceStackedPins,
                                    wxString::Format( " -> emit pin num='%s' name='%s' net='%s'", num, pinName, netName ) );
                        aPins.emplace_back( num, netName, pinName );
                    }
                }
            }
        }
    }
}

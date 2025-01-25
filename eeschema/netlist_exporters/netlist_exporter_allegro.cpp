/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <confirm.h>
#include <refdes_utils.h>
#include <sch_edit_frame.h>
#include <sch_reference_list.h>
#include <string_utils.h>
#include <connection_graph.h>
#include <core/kicad_algo.h>
#include <netlist.h>
#include "netlist_exporter_allegro.h"
#include "netlist_exporter_xml.h"
#include <regex>
#include <fmt.h>
#include <fmt/ranges.h>

bool NETLIST_EXPORTER_ALLEGRO::WriteNetlist( const wxString& aOutFileName,
                                             unsigned /* aNetlistOptions */,
                                             REPORTER& aReporter )
{
    m_f = nullptr;
    bool success = true;

    // Create the devices directory
    m_exportPath = wxFileName( aOutFileName ).GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR )
                   + wxString( "devices" );
    if( !wxDirExists( m_exportPath ) )
    {
        if( !wxMkdir( m_exportPath, wxS_DIR_DEFAULT ) )
        {
            wxString msg = wxString::Format( _( "Failed to create directory 'devices' ." ) );
            aReporter.Report( msg, RPT_SEVERITY_ERROR );
            return false;
        }
    }

    // Write the netlist file
    if( ( m_f = wxFopen( aOutFileName, wxT( "wt" ) ) ) == nullptr )
    {
        wxString msg = wxString::Format( _( "Failed to create file '%s'." ), aOutFileName );
        aReporter.Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    try
    {
        fmt::print( m_f, "(NETLIST)\n" );
        fmt::print( m_f, "(Source: {})\n", TO_UTF8( m_schematic->GetFileName() ) );
        fmt::print( m_f, "(Date: {})\n", TO_UTF8( GetISO8601CurrentDateTime() ) );

        m_packageProperties.clear();
        m_componentGroups.clear();
        m_orderedSymbolsSheetpath.clear();
        m_netNameNodes.clear();

        extractComponentsInfo();

        // Start with package definitions, which we create from component groups.
        toAllegroPackages();

        // Write out package properties. NOTE: Allegro doesn't recognize much...
        toAllegroPackageProperties();

        // Write out nets
        toAllegroNets();

        fmt::print( m_f, "$END\n" );
    }
    catch (...)
    {
        success = false;
    }

    // Done with the netlist
    fclose( m_f );

    m_f = nullptr;

    return success;
}


bool NETLIST_EXPORTER_ALLEGRO::CompareSymbolSheetpath( const std::pair<SCH_SYMBOL*,
                                                       SCH_SHEET_PATH>& aItem1,
                                                       const std::pair<SCH_SYMBOL*,
                                                       SCH_SHEET_PATH>& aItem2 )
{
    wxString refText1 = aItem1.first->GetRef( &aItem1.second );
    wxString refText2 = aItem2.first->GetRef( &aItem2.second );

    if( refText1 == refText2 )
    {
        return aItem1.second.PathHumanReadable() < aItem2.second.PathHumanReadable();
    }

    return CompareSymbolRef( refText1, refText2 );
}


bool NETLIST_EXPORTER_ALLEGRO::CompareSymbolRef( const wxString& aRefText1,
                                                 const wxString& aRefText2 )
{
    if( removeTailDigits( aRefText1 ) == removeTailDigits( aRefText2 ) )
    {
        return extractTailNumber( aRefText1 ) < extractTailNumber( aRefText2 );
    }

    return aRefText1 < aRefText2;
}


bool NETLIST_EXPORTER_ALLEGRO::CompareLibPin( const SCH_PIN* aPin1, const SCH_PIN* aPin2 )
{
    // return "lhs < rhs"
    return StrNumCmp( aPin1->GetShownNumber(), aPin2->GetShownNumber(), true ) < 0;
}


void NETLIST_EXPORTER_ALLEGRO::extractComponentsInfo()
{
    m_referencesAlreadyFound.Clear();
    m_libParts.clear();

    for( const SCH_SHEET_PATH& sheet : m_schematic->Hierarchy() )
    {
        m_schematic->SetCurrentSheet( sheet );

        auto cmp =
                [&sheet]( SCH_SYMBOL* a, SCH_SYMBOL* b )
                {
                    return ( StrNumCmp( a->GetRef( &sheet, false ),
                                        b->GetRef( &sheet, false ), true ) < 0 );
                };

        std::set<SCH_SYMBOL*, decltype( cmp )> ordered_symbols( cmp );
        std::multiset<SCH_SYMBOL*, decltype( cmp )> extra_units( cmp );

        for( SCH_ITEM* item : sheet.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            auto        test = ordered_symbols.insert( symbol );

            if( !test.second )
            {
                if( ( *( test.first ) )->m_Uuid > symbol->m_Uuid )
                {
                    extra_units.insert( *( test.first ) );
                    ordered_symbols.erase( test.first );
                    ordered_symbols.insert( symbol );
                }
                else
                {
                    extra_units.insert( symbol );
                }
            }
        }

        for( EDA_ITEM* item : ordered_symbols )
        {
            SCH_SYMBOL* symbol = findNextSymbol( item, sheet );

            if( !symbol || symbol->GetExcludedFromBoard() )
                continue;

            if( symbol->GetLibPins().empty() )
                continue;

            m_packageProperties.insert( std::pair<wxString,
                                        wxString>( sheet.PathHumanReadable(),
                                                   symbol->GetRef( &sheet ) ) );
            m_orderedSymbolsSheetpath.push_back( std::pair<SCH_SYMBOL*,
                                                 SCH_SHEET_PATH>( symbol, sheet ) );
        }
    }

    struct NET_RECORD
    {
        NET_RECORD( const wxString& aName ) :
            m_Name( aName )
        {};

        wxString              m_Name;
        std::vector<NET_NODE> m_Nodes;
    };

    std::vector<NET_RECORD*> nets;

    for( const auto& it : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        wxString                                 net_name  = it.first.Name;
        const std::vector<CONNECTION_SUBGRAPH*>& subgraphs = it.second;
        NET_RECORD*                              net_record = nullptr;

        if( subgraphs.empty() )
            continue;

        nets.emplace_back( new NET_RECORD( net_name ) );
        net_record = nets.back();

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            bool nc = subgraph->GetNoConnect() &&
                      subgraph->GetNoConnect()->Type() == SCH_NO_CONNECT_T;
            const SCH_SHEET_PATH& sheet = subgraph->GetSheet();

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    SCH_PIN* pin = static_cast<SCH_PIN*>( item );
                    SYMBOL*  symbol = pin->GetParentSymbol();

                    if( !symbol || symbol->GetExcludedFromBoard() )
                        continue;

                    net_record->m_Nodes.emplace_back( pin, sheet, nc );
                }
            }
        }
    }

    // Netlist ordering: Net name, then ref des, then pin name
    std::sort( nets.begin(), nets.end(),
               []( const NET_RECORD* a, const NET_RECORD*b )
               {
                   return StrNumCmp( a->m_Name, b->m_Name ) < 0;
               } );

    for( NET_RECORD* net_record : nets )
    {
        // Netlist ordering: Net name, then ref des, then pin name
        std::sort( net_record->m_Nodes.begin(), net_record->m_Nodes.end(),
                   []( const NET_NODE& a, const NET_NODE& b )
                   {
                       wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                       wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                       if( refA == refB )
                           return a.m_Pin->GetShownNumber() < b.m_Pin->GetShownNumber();

                       return refA < refB;
                   } );

        // Some duplicates can exist, for example on multi-unit parts with duplicated pins across
        // units.  If the user connects the pins on each unit, they will appear on separate
        // subgraphs.  Remove those here:
        alg::remove_duplicates( net_record->m_Nodes,
                []( const NET_NODE& a, const NET_NODE& b )
                {
                    wxString refA = a.m_Pin->GetParentSymbol()->GetRef( &a.m_Sheet );
                    wxString refB = b.m_Pin->GetParentSymbol()->GetRef( &b.m_Sheet );

                    return refA == refB && a.m_Pin->GetShownNumber() == b.m_Pin->GetShownNumber();
                } );

        for( const NET_NODE& netNode : net_record->m_Nodes )
        {
            wxString refText = netNode.m_Pin->GetParentSymbol()->GetRef( &netNode.m_Sheet );

            // Skip power symbols and virtual symbols
            if( refText[0] == wxChar( '#' ) )
            {
                continue;
            }

            m_netNameNodes.insert( std::pair<wxString, NET_NODE>( net_record->m_Name, netNode ) );
        }
    }

    for( NET_RECORD* record : nets )
        delete record;
}


void NETLIST_EXPORTER_ALLEGRO::toAllegroPackages()
{
    int groupCount = 1;
    wxString deviceFileCreatingError = wxString( "" );

    //Group the components......
    while(!m_orderedSymbolsSheetpath.empty())
    {
        std::pair<SCH_SYMBOL*, SCH_SHEET_PATH> first_ele = m_orderedSymbolsSheetpath.front();
        m_orderedSymbolsSheetpath.pop_front();
        m_componentGroups.insert( std::pair<int, std::pair<SCH_SYMBOL*,
                                  SCH_SHEET_PATH>>( groupCount, first_ele ) );

        for( auto it = m_orderedSymbolsSheetpath.begin(); it != m_orderedSymbolsSheetpath.end();
             ++it )
        {
            if( it->first->GetValue( false, &it->second, false )
                != first_ele.first->GetValue( false, &first_ele.second, false ) )
            {
                continue;
            }

            if( it->first->GetFootprintFieldText( false, &it->second, false )
                != first_ele.first->GetFootprintFieldText( false, &first_ele.second, false ) )
            {
                continue;
            }

            wxString ref1 = it->first->GetRef( &it->second );
            wxString ref2 = first_ele.first->GetRef( &first_ele.second );

            if( removeTailDigits( ref1 ) == removeTailDigits( ref2 ) )
            {
                m_componentGroups.insert( std::pair<int, std::pair<SCH_SYMBOL*,
                                          SCH_SHEET_PATH>>( groupCount, ( *it ) ) );
                it = m_orderedSymbolsSheetpath.erase( it );

                if( std::distance( it, m_orderedSymbolsSheetpath.begin() ) > 0 )
                    it--;
                else if( it == m_orderedSymbolsSheetpath.end() )
                    break;
            }
        }
        groupCount++;
    }

    struct COMP_PACKAGE_STRUCT
    {
        wxString                                            m_value;
        wxString                                            m_tolerance;
        std::vector<std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>> m_symbolSheetpaths;
    };

    COMP_PACKAGE_STRUCT                     compPackageStruct;
    std::map<wxString, COMP_PACKAGE_STRUCT> compPackageMap;

    for( int groupIndex = 1; groupIndex < groupCount; groupIndex++ )
    {
        auto pairIter = m_componentGroups.equal_range( groupIndex );
        auto beginIter = pairIter.first;
        auto endIter = pairIter.second;

        SCH_SYMBOL* sym = ( beginIter->second ).first;
        SCH_SHEET_PATH sheetPath = ( beginIter->second ).second;

        wxString valueText = sym->GetValue( false, &sheetPath, false );
        wxString footprintText = sym->GetFootprintFieldText( false, &sheetPath, false);
        wxString deviceType = valueText + wxString("_") + footprintText;

        while( deviceType.GetChar(deviceType.Length()-1) == '_' )
        {
            deviceType.RemoveLast();
        }

        deviceType = formatDevice( deviceType );

        wxArrayString fieldArray;
        fieldArray.Add( "Spice_Model" );
        fieldArray.Add( "VALUE" );

        wxString value = getGroupField( groupIndex, fieldArray );

        fieldArray.clear();
        fieldArray.Add( "TOLERANCE" );
        fieldArray.Add( "TOL" );
        wxString tol = getGroupField( groupIndex, fieldArray );

        std::vector<std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>> symbolSheetpaths;

        for( auto iter = beginIter; iter != endIter; iter++ )
        {
            symbolSheetpaths.push_back( std::pair<SCH_SYMBOL*,
                                        SCH_SHEET_PATH>( iter->second.first,
                                                         iter->second.second ) );
        }

        std::stable_sort( symbolSheetpaths.begin(), symbolSheetpaths.end(),
                          NETLIST_EXPORTER_ALLEGRO::CompareSymbolSheetpath );

        compPackageStruct.m_value = value;
        compPackageStruct.m_tolerance = tol;
        compPackageStruct.m_symbolSheetpaths = symbolSheetpaths;
        compPackageMap.insert( std::pair( deviceType, compPackageStruct ) );

        // Write out the corresponding device file
        FILE* d = nullptr;
        wxString deviceFileName = wxFileName( m_exportPath, deviceType,
                                              wxString( "txt" ) ).GetFullPath();

        if( ( d = wxFopen( deviceFileName, wxT( "wt" ) ) ) == nullptr )
        {
            wxString msg;
            msg.Printf( _( "Failed to create file '%s'.\n" ), deviceFileName );
            deviceFileCreatingError += msg;
            continue;
        }

        footprintText = footprintText.AfterLast( ':' );

        wxArrayString footprintAlt;
        wxArrayString footprintArray = sym->GetLibSymbolRef()->GetFPFilters();

        for( const wxString& fp : footprintArray )
        {
            if( ( fp.Find( '*' ) != wxNOT_FOUND ) || ( fp.Find( '?' ) != wxNOT_FOUND ) )
            {
                continue;
            }

            footprintAlt.Add( fp.AfterLast( ':' ) );
        }

        if( footprintText.IsEmpty() )
        {
            if( !footprintAlt.IsEmpty() )
            {
                footprintText = footprintAlt[0];
                footprintAlt.RemoveAt( 0 );
            }
            else
            {
                footprintText = deviceType;
            }
        }

        fmt::print( d, "PACKAGE '{}'\n", TO_UTF8( formatDevice( footprintText ) ) );
        fmt::print( d, "CLASS IC\n" );

        std::vector<SCH_PIN*> pinList = sym->GetLibSymbolRef()->GetPins();

        /*
         * We must erase redundant Pins references in pinList
         * These redundant pins exist because some pins are found more than one time when a
         * symbol has multiple parts per package or has 2 representations (DeMorgan conversion).
         * For instance, a 74ls00 has DeMorgan conversion, with different pin shapes, and
         * therefore each pin  appears 2 times in the list. Common pins (VCC, GND) can also be
         * found more than once.
         */
        sort( pinList.begin(), pinList.end(), NETLIST_EXPORTER_ALLEGRO::CompareLibPin );

        for( int ii = 0; ii < (int) pinList.size() - 1; ii++ )
        {
            if( pinList[ii]->GetNumber() == pinList[ii + 1]->GetNumber() )
            {
                // 2 pins have the same number, remove the redundant pin at index i+1
                pinList.erase( pinList.begin() + ii + 1 );
                ii--;
            }
        }

        unsigned int pinCount = pinList.size();
        fmt::print( d, "PINCOUNT {}\n", pinCount );

        if( pinCount > 0 )
        {
            fmt::print( d, "{}", TO_UTF8( formatFunction( "main", pinList ) ) );
        }

        if( !value.IsEmpty() )
        {
            fmt::print( d, "PACKAGEPROP VALUE {}\n", TO_UTF8( value ) );
        }

        if( !tol.IsEmpty() )
        {
            fmt::print( d, "PACKAGEPROP TOL {}\n", TO_UTF8( tol ) );
        }

        if( !footprintAlt.IsEmpty() )
        {
            fmt::print( d, "PACKAGEPROP ALT_SYMBOLS '({})'\n", fmt::join( footprintAlt, "," ) );
        }

        wxArrayString propArray;
        propArray.Add( "PART_NUMBER" );
        propArray.Add( "mpn" );
        propArray.Add( "mfr_pn" );
        wxString data = getGroupField( groupIndex, propArray );

        if(!data.IsEmpty())
        {
            fmt::print( d, "PACKAGEPROP {} {}\n", TO_UTF8( propArray[0] ), TO_UTF8( data ) );
        }

        propArray.clear();
        propArray.Add( "HEIGHT" );
        data = getGroupField( groupIndex, propArray );

        if(!data.IsEmpty())
        {
            fmt::print( d, "PACKAGEPROP {} {}\n", TO_UTF8( propArray[0] ), TO_UTF8( data ) );
        }

        fmt::print( d, "END\n" );

        fclose( d );
    }

    fmt::print( m_f, "$PACKAGES\n" );

    for( auto iter = compPackageMap.begin(); iter != compPackageMap.end(); iter++ )
    {
        wxString deviceType = iter->first;
        wxString value = iter->second.m_value;
        wxString tolerance = iter->second.m_tolerance;

        if( value.IsEmpty() && tolerance.IsEmpty() )
        {
            fmt::print( m_f, "! '{}' ; ", TO_UTF8( deviceType ) );
        }
        else if( tolerance.IsEmpty() )
        {
            fmt::print( m_f, "! '{}' ! '{}' ; ", TO_UTF8( deviceType ), TO_UTF8( value ) );
        }
        else
        {
            fmt::print( m_f, "! '{}' ! '{}' ! {} ; ", TO_UTF8( deviceType ), TO_UTF8( value ),
                     TO_UTF8( tolerance ) );
        }

        std::vector<std::pair<SCH_SYMBOL*, SCH_SHEET_PATH>> symbolSheetpaths =
                iter->second.m_symbolSheetpaths;

        std::vector<wxString> refTexts;
        for( const auto& [ sym, sheetPath ] : symbolSheetpaths)
            refTexts.push_back( sym->GetRef( &sheetPath ) );

        fmt::print( m_f, "{}", fmt::join( refTexts, ",\n\t" ) );

        fmt::print( m_f, "\n" );
    }

    if( !deviceFileCreatingError.IsEmpty() )
        DisplayError( nullptr, deviceFileCreatingError );
}


wxString NETLIST_EXPORTER_ALLEGRO::formatText( wxString aString )
{
    if( aString.IsEmpty() )
        return wxEmptyString;

    aString.Replace( "\u03BC", "u" );

    std::regex reg( "[!']|[^ -~]" );
    wxString   processedString = wxString( std::regex_replace( std::string( aString ), reg, "?" ) );

    std::regex search_reg( "[^a-zA-Z0-9_/]" );

    if( std::regex_search( std::string( processedString ), search_reg ) )
        return wxString( "'" ) + processedString + wxString( "'" );

    return processedString;
}


wxString NETLIST_EXPORTER_ALLEGRO::formatPin( const SCH_PIN& aPin )
{
    wxString   pinName4Telesis = aPin.GetName() + wxString( "__" ) + aPin.GetNumber();
    std::regex reg( "[^A-Za-z0-9_+?/-]" );
    return wxString( std::regex_replace( std::string( pinName4Telesis ), reg, "?" ) );
}


wxString NETLIST_EXPORTER_ALLEGRO::formatFunction( wxString aName, std::vector<SCH_PIN*> aPinList )
{
    aName.MakeUpper();
    std::list<wxString> pinNameList;

    std::stable_sort( aPinList.begin(), aPinList.end(), NETLIST_EXPORTER_ALLEGRO::CompareLibPin );

    for( auto pin : aPinList )
        pinNameList.push_back( formatPin( *pin ) );

    wxString out_str = "";
    wxString str;

    out_str.Printf( wxT( "PINORDER %s " ), TO_UTF8( aName ) );

    for( const wxString& pinName : pinNameList )
    {
        str.Printf( ",\n\t%s", TO_UTF8( pinName ) );
        out_str += str;
    }
    out_str += wxString( "\n" );

    str.Printf( wxT( "FUNCTION %s %s " ), TO_UTF8( aName ), TO_UTF8( aName ) );
    out_str += str;

    for( auto pin : aPinList )
    {
        str.Printf( ",\n\t%s", TO_UTF8( pin->GetNumber() ) );
        out_str += str;
    }

    out_str += wxString( "\n" );

    return out_str;
}


wxString NETLIST_EXPORTER_ALLEGRO::getGroupField( int aGroupIndex, const wxArrayString& aFieldArray,
                                                  bool aSanitize )
{
    auto pairIter = m_componentGroups.equal_range( aGroupIndex );

    for( auto iter = pairIter.first; iter != pairIter.second; ++iter )
    {
        SCH_SYMBOL* sym = ( iter->second ).first;
        SCH_SHEET_PATH sheetPath = ( iter->second ).second;

        for( const wxString& field : aFieldArray )
        {
            if( SCH_FIELD* fld = sym->FindFieldCaseInsensitive( field ) )
            {
                wxString fieldText = fld->GetShownText( &sheetPath, true );

                if( !fieldText.IsEmpty() )
                {
                    if( aSanitize )
                        return formatText( fieldText );
                    else
                        return fieldText;
                }
            }
        }
    }

    for( auto iter = pairIter.first; iter != pairIter.second; ++iter )
    {
        SCH_SYMBOL* sym = ( iter->second ).first;

        for( const wxString& field : aFieldArray )
        {
            if( SCH_FIELD* fld = sym->GetLibSymbolRef()->FindFieldCaseInsensitive( field ) )
            {
                wxString fieldText = fld->GetShownText( false, 0 );

                if( !fieldText.IsEmpty() )
                {
                    if( aSanitize )
                        return formatText( fieldText );
                    else
                        return fieldText;
                }
            }
        }
    }

    return wxEmptyString;
}


wxString NETLIST_EXPORTER_ALLEGRO::formatDevice( wxString aString )
{
    aString.MakeLower();
    std::regex reg( "[^a-z0-9_-]" );
    return wxString( std::regex_replace( std::string( aString ), reg, "_" ) );
}


void NETLIST_EXPORTER_ALLEGRO::toAllegroPackageProperties()
{
    fmt::print( m_f, "$A_PROPERTIES\n" );

    while( !m_packageProperties.empty() )
    {
        std::multimap<wxString, wxString>::iterator iter = m_packageProperties.begin();
        wxString                                    sheetPathText = iter->first;

        fmt::print( m_f, "'ROOM' '{}' ; ", TO_UTF8( formatText( sheetPathText ) ) );

        std::vector<wxString> refTexts;

        auto pairIter = m_packageProperties.equal_range( sheetPathText );

        for( iter = pairIter.first; iter != pairIter.second; ++iter )
        {
            wxString refText = iter->second;
            refTexts.push_back( refText );
        }

        m_packageProperties.erase( pairIter.first, pairIter.second );

        std::stable_sort( refTexts.begin(), refTexts.end(),
                          NETLIST_EXPORTER_ALLEGRO::CompareSymbolRef );

        fmt::print( m_f, "{}", fmt::join( refTexts, ",\n\t" ) );

        fmt::print( m_f, "\n" );
    }
}


void NETLIST_EXPORTER_ALLEGRO::toAllegroNets()
{
    fmt::print( m_f, "$NETS\n" );

    while( !m_netNameNodes.empty() )
    {
        std::multimap<wxString, NET_NODE>::iterator iter = m_netNameNodes.begin();
        std::vector<NET_NODE>                       netNodes;

        wxString netName = iter->first;

        fmt::print( m_f, "{}; ", TO_UTF8( formatText( netName ).MakeUpper() ) );

        auto pairIter = m_netNameNodes.equal_range( netName );

        for( iter = pairIter.first; iter != pairIter.second; ++iter )
        {
            NET_NODE netNode = iter->second;
            netNodes.push_back( netNode );
        }

        m_netNameNodes.erase( pairIter.first, pairIter.second );

        std::stable_sort( netNodes.begin(), netNodes.end() );

        std::vector<wxString> nets;
        for( const NET_NODE& netNode : netNodes )
        {
            wxString refText = netNode.m_Pin->GetParentSymbol()->GetRef( &netNode.m_Sheet );
            wxString pinText = netNode.m_Pin->GetShownNumber();
            nets.push_back( refText + wxString( "." ) + pinText );
        }

        fmt::print( m_f, "{}", fmt::join( nets, ",\n\t" ) );

        fmt::print( m_f, "\n" );
    }
}


wxString NETLIST_EXPORTER_ALLEGRO::removeTailDigits( wxString aString )
{
    while( ( aString.GetChar( aString.Length() - 1 ) >= '0' )
           && ( aString.GetChar( aString.Length() - 1 ) <= '9' ) )
    {
        aString.RemoveLast();
    }

    return aString;
}


unsigned int NETLIST_EXPORTER_ALLEGRO::extractTailNumber( wxString aString )
{
    wxString numString;

    while( ( aString.GetChar( aString.Length() - 1 ) >= '0' )
           && ( aString.GetChar( aString.Length() - 1 ) <= '9' ) )
    {
        numString.insert( 0, aString.GetChar( aString.Length() - 1 ) );
        aString.RemoveLast();
    }

    unsigned long val;

    //From wxWidgets 3.1.6, here we can use ToUInt instead of ToULong function.
    numString.ToULong( &val );
    return (unsigned int) val;
}

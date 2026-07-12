/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pads_sch_binary_parser.h"

#include "pads_sch_sdb.h"

#include <algorithm>
#include <set>

#include <ki_exception.h>
#include <wx/strconv.h>

namespace PADS_SCH_BINARY
{

namespace
{

    template <typename Item>
    bool idsAreUnique( const std::vector<Item>& aItems )
    {
        std::set<decltype( aItems.front().id )> ids;

        for( const Item& item : aItems )
        {
            if( !item.id.IsValid() || !ids.insert( item.id ).second )
                return false;
        }

        return true;
    }


    template <typename Item, typename Id>
    bool containsId( const std::vector<Item>& aItems, Id aId )
    {
        return std::ranges::any_of( aItems,
                                    [&]( const Item& aItem )
                                    {
                                        return aItem.id == aId;
                                    } );
    }


    [[noreturn]] void throwValidationError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage )
    {
        THROW_IO_ERROR( FormatParserError( aSource, aMessage ) );
    }


    bool endpointIsValid( const PADS_SCH_MODEL& aModel, const MODEL_CONNECTION_ENDPOINT& aEndpoint )
    {
        if( aEndpoint.kind == MODEL_ENDPOINT_KIND::POINT )
            return !aEndpoint.placement && !aEndpoint.pin;

        if( aEndpoint.kind != MODEL_ENDPOINT_KIND::PIN || !aEndpoint.placement || !aEndpoint.pin )
            return false;

        auto placement = std::ranges::find_if( aModel.placements,
                                               [&]( const MODEL_PLACEMENT& aPlacement )
                                               {
                                                   return aPlacement.id == aEndpoint.placement->id;
                                               } );

        if( placement == aModel.placements.end() || !placement->gate )
            return false;

        auto partType = std::ranges::find_if( aModel.partTypes,
                                              [&]( const MODEL_PART_TYPE& aPartType )
                                              {
                                                  return aPartType.id == placement->partType.id;
                                              } );

        if( partType == aModel.partTypes.end() )
            return false;

        auto gate = std::ranges::find_if( partType->gates,
                                          [&]( const MODEL_GATE& aGate )
                                          {
                                              return aGate.id == placement->gate->id;
                                          } );

        if( gate == partType->gates.end() || gate->unit != placement->unit
            || std::ranges::none_of( gate->pins,
                                     [&]( const PIN_REFERENCE& aPin )
                                     {
                                         return aPin.id == aEndpoint.pin->id;
                                     } ) )
        {
            return false;
        }

        auto definition = std::ranges::find_if( aModel.definitions,
                                                [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                {
                                                    return aDefinition.id == gate->definition.id;
                                                } );

        return definition != aModel.definitions.end() && containsId( definition->pins, aEndpoint.pin->id );
    }

} // namespace


wxString FormatParserError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage )
{
    return wxString::Format(
            wxS( "%s: PADS schematic v0x%04X %s (controller %d, record %llu, sheet %d) at offset 0x%llX: %s" ),
            aSource.file, aSource.version, aSource.objectClass, aSource.controller,
            static_cast<unsigned long long>( aSource.recordIndex ), aSource.sheet,
            static_cast<unsigned long long>( aSource.absoluteOffset ), aMessage );
}


bool PADS_SCH_MODEL::HasUniqueTypedIds() const
{
    if( !idsAreUnique( sheets ) || !idsAreUnique( definitions ) || !idsAreUnique( partTypes )
        || !idsAreUnique( placements ) || !idsAreUnique( nets ) || !idsAreUnique( buses ) )
    {
        return false;
    }

    std::vector<MODEL_GATE>           gates;
    std::vector<MODEL_PIN_DEFINITION> pins;

    for( const MODEL_PART_TYPE& partType : partTypes )
        gates.insert( gates.end(), partType.gates.begin(), partType.gates.end() );

    for( const MODEL_SYMBOL_DEFINITION& definition : definitions )
        pins.insert( pins.end(), definition.pins.begin(), definition.pins.end() );

    return idsAreUnique( gates ) && idsAreUnique( pins );
}


bool PADS_SCH_MODEL::AllReferencesResolved() const
{
    for( const MODEL_SHEET& sheet : sheets )
    {
        if( sheet.parent && !containsId( sheets, sheet.parent->id ) )
            return false;
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            auto definition = std::ranges::find_if( definitions,
                                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                    {
                                                        return aDefinition.id == gate.definition.id;
                                                    } );

            if( definition == definitions.end() )
                return false;

            for( const PIN_REFERENCE& pin : gate.pins )
            {
                if( !containsId( definition->pins, pin.id ) )
                    return false;
            }
        }
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !containsId( sheets, placement.sheet.id ) || !containsId( partTypes, placement.partType.id ) )
            return false;

        if( placement.gate )
        {
            auto partType = std::ranges::find_if( partTypes,
                                                  [&]( const MODEL_PART_TYPE& aPartType )
                                                  {
                                                      return aPartType.id == placement.partType.id;
                                                  } );

            if( partType == partTypes.end() )
                return false;

            auto gate = std::ranges::find_if( partType->gates,
                                              [&]( const MODEL_GATE& aGate )
                                              {
                                                  return aGate.id == placement.gate->id;
                                              } );

            if( gate == partType->gates.end() || gate->unit != placement.unit )
                return false;
        }
    }

    for( const MODEL_NET& net : nets )
    {
        if( !containsId( sheets, net.sheet.id ) )
            return false;

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            if( connection.endpoints.empty() )
                return false;

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( !endpointIsValid( *this, endpoint ) )
                    return false;
            }
        }
    }

    for( const MODEL_BUS& bus : buses )
    {
        if( !containsId( sheets, bus.sheet.id ) )
            return false;

        for( const NET_REFERENCE& member : bus.memberNets )
        {
            if( !containsId( nets, member.id ) )
                return false;
        }

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
        {
            if( !containsId( nets, entry.memberNet.id ) )
                return false;
        }
    }

    for( const MODEL_LABEL& label : labels )
    {
        if( !containsId( sheets, label.sheet.id ) )
            return false;
    }

    for( const MODEL_JUNCTION& junction : junctions )
    {
        if( !containsId( sheets, junction.sheet.id ) )
            return false;
    }

    for( const MODEL_TEXT& text : texts )
    {
        if( !containsId( sheets, text.sheet.id ) )
            return false;
    }

    return true;
}


void PADS_SCH_MODEL::ValidateOrThrow() const
{
    if( !HasUniqueTypedIds() )
        throwValidationError( source, wxS( "duplicate or invalid typed controller ID" ) );

    for( const MODEL_SHEET& sheet : sheets )
    {
        if( sheet.parent && !containsId( sheets, sheet.parent->id ) )
            throwValidationError( sheet.parent->source, wxS( "unresolved sheet reference" ) );
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            auto definition = std::ranges::find_if( definitions,
                                                    [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                                    {
                                                        return aDefinition.id == gate.definition.id;
                                                    } );

            if( definition == definitions.end() )
                throwValidationError( gate.definition.source, wxS( "unresolved symbol definition reference" ) );

            for( const PIN_REFERENCE& pin : gate.pins )
            {
                if( !containsId( definition->pins, pin.id ) )
                    throwValidationError( pin.source, wxS( "pin does not belong to gate definition" ) );
            }
        }
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !containsId( sheets, placement.sheet.id ) )
            throwValidationError( placement.sheet.source, wxS( "unresolved placement sheet reference" ) );

        auto partType = std::ranges::find_if( partTypes,
                                              [&]( const MODEL_PART_TYPE& aPartType )
                                              {
                                                  return aPartType.id == placement.partType.id;
                                              } );

        if( partType == partTypes.end() )
            throwValidationError( placement.partType.source, wxS( "unresolved placement part-type reference" ) );

        if( placement.gate )
        {
            auto gate = std::ranges::find_if( partType->gates,
                                              [&]( const MODEL_GATE& aGate )
                                              {
                                                  return aGate.id == placement.gate->id;
                                              } );

            if( gate == partType->gates.end() || gate->unit != placement.unit )
                throwValidationError( placement.gate->source, wxS( "placement gate or unit mismatch" ) );
        }
    }

    for( const MODEL_NET& net : nets )
    {
        if( !containsId( sheets, net.sheet.id ) )
            throwValidationError( net.sheet.source, wxS( "unresolved net sheet reference" ) );

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            if( connection.endpoints.empty() )
                throwValidationError( connection.source, wxS( "connection has no endpoints" ) );

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( !endpointIsValid( *this, endpoint ) )
                {
                    throwValidationError( endpoint.source, wxS( "empty, mixed, or unresolved connection endpoint" ) );
                }
            }
        }
    }

    for( const MODEL_BUS& bus : buses )
    {
        if( !containsId( sheets, bus.sheet.id ) )
            throwValidationError( bus.sheet.source, wxS( "unresolved bus sheet reference" ) );

        for( const NET_REFERENCE& member : bus.memberNets )
        {
            if( !containsId( nets, member.id ) )
                throwValidationError( member.source, wxS( "unresolved bus member-net reference" ) );
        }

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
        {
            if( !containsId( nets, entry.memberNet.id ) )
                throwValidationError( entry.memberNet.source, wxS( "unresolved bus-entry net reference" ) );
        }
    }

    for( const MODEL_LABEL& label : labels )
    {
        if( !containsId( sheets, label.sheet.id ) )
            throwValidationError( label.sheet.source, wxS( "unresolved label sheet reference" ) );
    }

    for( const MODEL_JUNCTION& junction : junctions )
    {
        if( !containsId( sheets, junction.sheet.id ) )
            throwValidationError( junction.sheet.source, wxS( "unresolved junction sheet reference" ) );
    }

    for( const MODEL_TEXT& text : texts )
    {
        if( !containsId( sheets, text.sheet.id ) )
            throwValidationError( text.sheet.source, wxS( "unresolved text sheet reference" ) );
    }

    if( !AllReferencesResolved() )
        throwValidationError( source, wxS( "unresolved or wrong-class controller reference" ) );

    for( const MODEL_SHEET& sheet : sheets )
    {
        std::set<SHEET_ID> ancestors;
        const MODEL_SHEET* current = &sheet;

        while( current->parent )
        {
            if( !ancestors.insert( current->id ).second )
                throwValidationError( current->source, wxS( "cyclic sheet hierarchy" ) );

            auto parent = std::ranges::find_if( sheets,
                                                [&]( const MODEL_SHEET& aSheet )
                                                {
                                                    return aSheet.id == current->parent->id;
                                                } );

            if( parent == sheets.end() )
                break;

            current = &*parent;
        }
    }
}


PADS_SCH_MODEL PADS_SCH_BINARY_PARSER::Parse( const std::vector<uint8_t>& aBytes, const wxString& aSourceName ) const
{
    PADS_SCH_SDB sdb;
    sdb.Load( aBytes );

    PADS_SCH_MODEL model;
    model.version = sdb.Version();
    model.source = { aSourceName, model.version, wxS( "model" ), -1, 0, 0, aBytes.size(), -1 };
    model.settings.source = model.source;

    for( const SCH_SDB_BLOCK& block : sdb.Blocks() )
    {
        if( block.kind != SCH_SDB_BLOCK_KIND::SHEET )
            continue;

        const size_t      index = model.sheets.size();
        SOURCE_PROVENANCE provenance{ aSourceName, model.version, wxS( "sheet" ), block.controller,
                                      index,       block.offset,  block.bytes,    static_cast<int>( index ) };
        MODEL_SHEET       sheet;
        sheet.id = SHEET_ID( static_cast<uint32_t>( index ) );
        sheet.index = index;
        sheet.source = provenance;
        sheet.name.source = provenance;
        model.sheets.push_back( std::move( sheet ) );
    }

    model.ValidateOrThrow();
    return model;
}


SOURCE_STRING PADS_SCH_BINARY_PARSER::DecodeString( const std::vector<uint8_t>& aBytes, uint32_t aCodePage,
                                                    const SOURCE_PROVENANCE&        aSource,
                                                    std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
{
    SOURCE_STRING result{ aBytes, {}, STRING_ENCODING_STATUS::UTF8, aSource };

    if( aBytes.empty() )
        return result;

    const char* data = reinterpret_cast<const char*>( aBytes.data() );

    if( aCodePage != 65001 )
    {
        result.text = wxString::From8BitData( data, aBytes.size() );
        result.encoding = STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE;
        aDiagnostics.push_back( { RPT_SEVERITY_WARNING, aSource,
                                  wxString::Format( wxS( "unknown code page %u; bytes preserved" ), aCodePage ) } );
        return result;
    }

    result.text = wxString::FromUTF8( data, aBytes.size() );

    if( !aBytes.empty() && result.text.empty() )
    {
        result.text = wxString::From8BitData( data, aBytes.size() );
        result.encoding = STRING_ENCODING_STATUS::INVALID_BYTES;
        aDiagnostics.push_back(
                { RPT_SEVERITY_WARNING, aSource, wxS( "invalid UTF-8 bytes; original bytes preserved" ) } );
    }

    return result;
}


void PADS_SCH_BINARY_PARSER::RecordUnknownEnum( const wxString& aEnumName, uint32_t aValue,
                                                const SOURCE_PROVENANCE&        aSource,
                                                std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
{
    aDiagnostics.push_back( { RPT_SEVERITY_WARNING, aSource,
                              wxString::Format( wxS( "unknown %s %u preserved" ), aEnumName, aValue ) } );
}

} // namespace PADS_SCH_BINARY

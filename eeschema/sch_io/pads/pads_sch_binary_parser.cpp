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
#include <map>
#include <set>

#include <ki_exception.h>
#include <wx/strconv.h>

namespace PADS_SCH_BINARY
{

namespace
{

    constexpr uint32_t CP1252_HIGH[] = { 0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
                                         0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
                                         0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
                                         0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178 };


    wxString decodeWindows1252( const std::vector<uint8_t>& aBytes )
    {
        wxString result;

        for( uint8_t byte : aBytes )
        {
            uint32_t codePoint = byte;

            if( byte >= 0x80 && byte <= 0x9F )
                codePoint = CP1252_HIGH[byte - 0x80];

            result += wxUniChar( codePoint );
        }

        return result;
    }


    wxString decodeUnknownCodePage( const std::vector<uint8_t>& aBytes )
    {
        wxString result;

        for( uint8_t byte : aBytes )
            result += wxUniChar( byte < 0x80 ? byte : 0xFFFD );

        return result;
    }


    wxString decodeUtf8( const std::vector<uint8_t>& aBytes, bool& aHadInvalidBytes )
    {
        wxString result;
        aHadInvalidBytes = false;

        for( size_t i = 0; i < aBytes.size(); )
        {
            uint8_t  lead = aBytes[i];
            uint32_t codePoint = 0;
            size_t   continuationCount = 0;
            uint32_t minimum = 0;

            if( lead < 0x80 )
            {
                result += wxUniChar( lead );
                ++i;
                continue;
            }
            else if( lead >= 0xC2 && lead <= 0xDF )
            {
                codePoint = lead & 0x1F;
                continuationCount = 1;
                minimum = 0x80;
            }
            else if( lead >= 0xE0 && lead <= 0xEF )
            {
                codePoint = lead & 0x0F;
                continuationCount = 2;
                minimum = 0x800;
            }
            else if( lead >= 0xF0 && lead <= 0xF4 )
            {
                codePoint = lead & 0x07;
                continuationCount = 3;
                minimum = 0x10000;
            }
            else
            {
                result += wxUniChar( 0xFFFD );
                aHadInvalidBytes = true;
                ++i;
                continue;
            }

            bool valid = i + continuationCount < aBytes.size();

            for( size_t j = 1; valid && j <= continuationCount; ++j )
            {
                uint8_t continuation = aBytes[i + j];
                valid = ( continuation & 0xC0 ) == 0x80;

                if( valid )
                    codePoint = ( codePoint << 6 ) | ( continuation & 0x3F );
            }

            valid = valid && codePoint >= minimum && codePoint <= 0x10FFFF
                    && !( codePoint >= 0xD800 && codePoint <= 0xDFFF );

            if( !valid )
            {
                result += wxUniChar( 0xFFFD );
                aHadInvalidBytes = true;
                ++i;
                continue;
            }

            result += wxUniChar( codePoint );
            i += continuationCount + 1;
        }

        return result;
    }

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


    [[noreturn]] void throwValidationError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage );


    template <typename Item, typename IdAccessor, typename SourceAccessor>
    void validateUniqueIds( const std::vector<Item>& aItems, const wxString& aObjectClass, IdAccessor aId,
                            SourceAccessor aSource )
    {
        std::map<uint32_t, SOURCE_PROVENANCE> declarations;

        for( const Item& item : aItems )
        {
            const auto& id = aId( item );

            if( !id.IsValid() )
                throwValidationError( aSource( item ), wxString::Format( wxS( "invalid %s ID" ), aObjectClass ) );

            auto [first, inserted] = declarations.emplace( id.Value(), aSource( item ) );

            if( !inserted )
            {
                const SOURCE_PROVENANCE& firstSource = first->second;
                wxString                 detail = wxString::Format(
                        wxS( "duplicate %s ID %u; first at v0x%04X %s controller %d record %llu sheet %d "
                                                             "offset 0x%llX" ),
                        aObjectClass, id.Value(), firstSource.version, firstSource.objectClass, firstSource.controller,
                        static_cast<unsigned long long>( firstSource.recordIndex ), firstSource.sheet,
                        static_cast<unsigned long long>( firstSource.absoluteOffset ) );
                throwValidationError( aSource( item ), detail );
            }
        }
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
    auto id = []( const auto& aItem ) -> const auto&
    {
        return aItem.id;
    };
    auto provenance = []( const auto& aItem ) -> const SOURCE_PROVENANCE&
    {
        return aItem.source;
    };
    validateUniqueIds( sheets, wxS( "sheet" ), id, provenance );
    validateUniqueIds( definitions, wxS( "definition" ), id, provenance );
    validateUniqueIds( partTypes, wxS( "part type" ), id, provenance );
    validateUniqueIds( placements, wxS( "placement" ), id, provenance );
    validateUniqueIds( nets, wxS( "net" ), id, provenance );
    validateUniqueIds( buses, wxS( "bus" ), id, provenance );

    std::vector<MODEL_GATE>           gates;
    std::vector<MODEL_PIN_DEFINITION> pins;

    for( const MODEL_PART_TYPE& partType : partTypes )
        gates.insert( gates.end(), partType.gates.begin(), partType.gates.end() );

    for( const MODEL_SYMBOL_DEFINITION& definition : definitions )
        pins.insert( pins.end(), definition.pins.begin(), definition.pins.end() );

    validateUniqueIds( gates, wxS( "gate" ), id, provenance );
    validateUniqueIds( pins, wxS( "pin" ), id, provenance );

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
                                                    std::vector<PARSER_DIAGNOSTIC>& aDiagnostics,
                                                    const wxString&                 aRecordedCodePageName )
{
    SOURCE_STRING result{ aBytes, {}, STRING_ENCODING_STATUS::UTF8, aSource };
    result.codePage = aCodePage;
    const wxString defaultName = aCodePage == 65001  ? wxS( "UTF-8" )
                                 : aCodePage == 1252 ? wxS( "windows-1252" )
                                                     : wxString::Format( wxS( "unknown-%u" ), aCodePage );
    result.codePageName = aRecordedCodePageName.empty() ? defaultName : aRecordedCodePageName;

    if( aCodePage == 1252 )
        result.encoding = STRING_ENCODING_STATUS::CODE_PAGE;
    else if( aCodePage != 65001 )
        result.encoding = STRING_ENCODING_STATUS::UNKNOWN_CODE_PAGE;

    if( aBytes.empty() )
        return result;

    if( aCodePage == 1252 )
    {
        result.text = decodeWindows1252( aBytes );
        return result;
    }

    if( aCodePage != 65001 )
    {
        result.text = decodeUnknownCodePage( aBytes );
        aDiagnostics.push_back( { RPT_SEVERITY_WARNING, aSource,
                                  wxString::Format( wxS( "unknown code page %u; bytes preserved and "
                                                         "non-ASCII bytes decoded as U+FFFD" ),
                                                    aCodePage ) } );
        return result;
    }

    bool invalid = false;
    result.text = decodeUtf8( aBytes, invalid );

    if( invalid )
    {
        result.encoding = STRING_ENCODING_STATUS::INVALID_BYTES;
        aDiagnostics.push_back( { RPT_SEVERITY_WARNING, aSource,
                                  wxS( "invalid UTF-8 bytes replaced with U+FFFD; original bytes preserved" ) } );
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

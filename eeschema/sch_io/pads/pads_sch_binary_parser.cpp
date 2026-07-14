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
#include <ranges>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <ki_exception.h>
#include <wx/strconv.h>

namespace PADS_SCH_BINARY
{

namespace
{

    constexpr size_t   OUTER_DIRECTORY_OFFSET = 0x20;
    constexpr size_t   OUTER_DESCRIPTOR_BYTES = 28;
    constexpr size_t   OUTER_USED_BYTES_OFFSET = 12;
    constexpr size_t   SHEET_HEADER_BYTES = 20;
    constexpr size_t   SHEET_DESCRIPTOR_COUNT = 24;
    constexpr size_t   SHEET_DESCRIPTOR_BYTES = 28;
    constexpr size_t   SHEET_COUNT_OFFSET = 12;
    constexpr size_t   SHEET_USED_BYTES_OFFSET = 16;
    constexpr size_t   SHEET_RECORD_BYTES = 48;
    constexpr size_t   TEXT_RECORD_BYTES = 32;
    constexpr uint32_t DEFAULT_CODE_PAGE = 1252;


    size_t outerControllerOffset( const PADS_SCH_SDB& aSdb, size_t aController )
    {
        size_t offset = aSdb.PayloadOffset() + 4;

        for( size_t controller = 1; controller < aController; ++controller )
            offset += aSdb.Pools()[controller].usedBytes;

        return offset;
    }


    SOURCE_PROVENANCE sourceAt( const wxString& aFile, uint16_t aVersion, const wxString& aObjectClass, int aController,
                                size_t aRecord, size_t aOffset, size_t aLength, int aSheet )
    {
        return { aFile, aVersion, aObjectClass, aController, aRecord, aOffset, aLength, aSheet };
    }


    [[noreturn]] void throwDecodeError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage )
    {
        THROW_IO_ERROR( FormatParserError( aSource, aMessage ) );
    }


    int64_t decodeCoordinate( uint16_t aRaw )
    {
        return static_cast<int64_t>( aRaw ) * 4 - 198144;
    }


    SOURCE_POINT pageExtent( uint8_t aPage, const SOURCE_PROVENANCE& aSource )
    {
        switch( aPage )
        {
        case 'A': return { 22000, 17000, aSource };
        case 'B': return { 34000, 22000, aSource };
        case 'C': return { 44000, 34000, aSource };
        case 'D': return { 68000, 44000, aSource };
        case 'E': return { 88000, 68000, aSource };
        default: throwDecodeError( aSource, wxS( "invalid design page-size token" ) );
        }
    }


    MODEL_JUSTIFICATION horizontalJustification( uint16_t aValue, const SOURCE_PROVENANCE& aSource,
                                                 std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        uint16_t horizontal = aValue >= 8 ? aValue - 8 : ( aValue >= 2 ? aValue - 2 : aValue );

        switch( horizontal )
        {
        case 0: return MODEL_JUSTIFICATION::LEFT;
        case 1: return MODEL_JUSTIFICATION::RIGHT;
        case 4: return MODEL_JUSTIFICATION::CENTER;
        default:
            PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "text justification" ), aValue, aSource, aDiagnostics );
            return MODEL_JUSTIFICATION::LEFT;
        }
    }


    MODEL_JUSTIFICATION verticalJustification( uint16_t aValue )
    {
        if( aValue >= 8 )
            return MODEL_JUSTIFICATION::CENTER;

        if( aValue >= 2 )
            return MODEL_JUSTIFICATION::LEFT;

        return MODEL_JUSTIFICATION::RIGHT;
    }

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
        std::unordered_set<uint32_t> ids;

        for( const Item& item : aItems )
        {
            if( !item.id.IsValid() || !ids.insert( item.id.Value() ).second )
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


    [[noreturn]] void throwValidationError( const SOURCE_PROVENANCE& aSource, const wxString& aMessage )
    {
        THROW_IO_ERROR( FormatParserError( aSource, aMessage ) );
    }


    struct MODEL_INDEX
    {
        std::unordered_map<uint32_t, const MODEL_SHEET*>             sheets;
        std::unordered_map<uint32_t, const MODEL_SYMBOL_DEFINITION*> definitions;
        std::unordered_map<uint32_t, const MODEL_PART_TYPE*>         partTypes;
        std::unordered_map<uint32_t, const MODEL_GATE*>              gates;
        std::unordered_map<uint32_t, const MODEL_PART_TYPE*>         gateOwners;
        std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*>    pins;
        std::unordered_map<uint32_t, const MODEL_SYMBOL_DEFINITION*> pinOwners;
        std::unordered_map<uint32_t, const MODEL_PLACEMENT*>         placements;
        std::unordered_map<uint32_t, const MODEL_NET*>               nets;

        explicit MODEL_INDEX( const PADS_SCH_MODEL& aModel )
        {
            for( const MODEL_SHEET& sheet : aModel.sheets )
                sheets.emplace( sheet.id.Value(), &sheet );

            for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
            {
                definitions.emplace( definition.id.Value(), &definition );

                for( const MODEL_PIN_DEFINITION& pin : definition.pins )
                {
                    pins.emplace( pin.id.Value(), &pin );
                    pinOwners.emplace( pin.id.Value(), &definition );
                }
            }

            for( const MODEL_PART_TYPE& partType : aModel.partTypes )
            {
                partTypes.emplace( partType.id.Value(), &partType );

                for( const MODEL_GATE& gate : partType.gates )
                {
                    gates.emplace( gate.id.Value(), &gate );
                    gateOwners.emplace( gate.id.Value(), &partType );
                }
            }

            for( const MODEL_PLACEMENT& placement : aModel.placements )
                placements.emplace( placement.id.Value(), &placement );

            for( const MODEL_NET& net : aModel.nets )
                nets.emplace( net.id.Value(), &net );
        }
    };


    bool endpointIsValid( const MODEL_INDEX& aIndex, const MODEL_CONNECTION_ENDPOINT& aEndpoint )
    {
        if( aEndpoint.kind == MODEL_ENDPOINT_KIND::POINT )
            return !aEndpoint.placement && !aEndpoint.pin;

        if( aEndpoint.kind != MODEL_ENDPOINT_KIND::PIN || !aEndpoint.placement || !aEndpoint.pin )
            return false;

        auto placement = aIndex.placements.find( aEndpoint.placement->id.Value() );

        if( placement == aIndex.placements.end() || !placement->second->gate )
            return false;

        auto partType = aIndex.partTypes.find( placement->second->partType.id.Value() );

        if( partType == aIndex.partTypes.end() )
            return false;

        auto gate = aIndex.gates.find( placement->second->gate->id.Value() );
        auto gateOwner = aIndex.gateOwners.find( placement->second->gate->id.Value() );

        if( gate == aIndex.gates.end() || gateOwner == aIndex.gateOwners.end() || gateOwner->second != partType->second
            || gate->second->unit != placement->second->unit
            || std::ranges::none_of( gate->second->pins,
                                     [&]( const PIN_REFERENCE& aPin )
                                     {
                                         return aPin.id == aEndpoint.pin->id;
                                     } ) )
        {
            return false;
        }

        auto definition = aIndex.definitions.find( gate->second->definition.id.Value() );
        auto pin = aIndex.pins.find( aEndpoint.pin->id.Value() );
        auto pinOwner = aIndex.pinOwners.find( aEndpoint.pin->id.Value() );

        return definition != aIndex.definitions.end() && pin != aIndex.pins.end() && pinOwner != aIndex.pinOwners.end()
               && pinOwner->second == definition->second;
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

    std::unordered_set<uint32_t> gateIds;
    std::unordered_set<uint32_t> pinIds;

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            if( !gate.id.IsValid() || !gateIds.insert( gate.id.Value() ).second )
                return false;
        }
    }

    for( const MODEL_SYMBOL_DEFINITION& definition : definitions )
    {
        for( const MODEL_PIN_DEFINITION& pin : definition.pins )
        {
            if( !pin.id.IsValid() || !pinIds.insert( pin.id.Value() ).second )
                return false;
        }
    }

    return true;
}


bool PADS_SCH_MODEL::AllReferencesResolved() const
{
    const MODEL_INDEX index( *this );

    for( const MODEL_SHEET& sheet : sheets )
    {
        if( sheet.parent && !index.sheets.contains( sheet.parent->id.Value() ) )
            return false;
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            auto definition = index.definitions.find( gate.definition.id.Value() );

            if( definition == index.definitions.end() )
                return false;

            for( const PIN_REFERENCE& pin : gate.pins )
            {
                auto pinOwner = index.pinOwners.find( pin.id.Value() );

                if( pinOwner == index.pinOwners.end() || pinOwner->second != definition->second )
                    return false;
            }
        }
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !index.sheets.contains( placement.sheet.id.Value() )
            || !index.partTypes.contains( placement.partType.id.Value() ) )
            return false;

        if( placement.gate )
        {
            auto partType = index.partTypes.find( placement.partType.id.Value() );

            if( partType == index.partTypes.end() )
                return false;

            auto gate = index.gates.find( placement.gate->id.Value() );
            auto gateOwner = index.gateOwners.find( placement.gate->id.Value() );

            if( gate == index.gates.end() || gateOwner == index.gateOwners.end()
                || gateOwner->second != partType->second || gate->second->unit != placement.unit )
                return false;
        }
    }

    for( const MODEL_NET& net : nets )
    {
        if( !index.sheets.contains( net.sheet.id.Value() ) )
            return false;

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            if( connection.endpoints.empty() )
                return false;

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( !endpointIsValid( index, endpoint ) )
                    return false;

                if( endpoint.placement )
                {
                    auto placement = index.placements.find( endpoint.placement->id.Value() );

                    if( placement == index.placements.end() || placement->second->sheet.id != net.sheet.id )
                        return false;
                }
            }
        }
    }

    for( const MODEL_BUS& bus : buses )
    {
        if( !index.sheets.contains( bus.sheet.id.Value() ) )
            return false;

        for( const NET_REFERENCE& member : bus.memberNets )
        {
            auto net = index.nets.find( member.id.Value() );

            if( net == index.nets.end() || net->second->sheet.id != bus.sheet.id )
                return false;
        }

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
        {
            auto net = index.nets.find( entry.memberNet.id.Value() );

            if( net == index.nets.end() || net->second->sheet.id != bus.sheet.id
                || std::ranges::none_of( bus.memberNets,
                                         [&]( const NET_REFERENCE& aMember )
                                         {
                                             return aMember.id == entry.memberNet.id;
                                         } ) )
            {
                return false;
            }
        }
    }

    for( const MODEL_LABEL& label : labels )
    {
        if( !index.sheets.contains( label.sheet.id.Value() ) )
            return false;
    }

    for( const MODEL_JUNCTION& junction : junctions )
    {
        if( !index.sheets.contains( junction.sheet.id.Value() ) )
            return false;
    }

    for( const MODEL_TEXT& text : texts )
    {
        if( !index.sheets.contains( text.sheet.id.Value() ) )
            return false;
    }

    for( const MODEL_PAGE_GRAPHIC& graphic : graphics )
    {
        if( !index.sheets.contains( graphic.sheet.id.Value() ) )
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

    std::unordered_map<uint32_t, SOURCE_PROVENANCE> gateDeclarations;
    std::unordered_map<uint32_t, SOURCE_PROVENANCE> pinDeclarations;

    auto validateNestedId = [&]( const auto& aItem, const wxString& aObjectClass, auto& aDeclarations )
    {
        if( !aItem.id.IsValid() )
            throwValidationError( aItem.source, wxString::Format( wxS( "invalid %s ID" ), aObjectClass ) );

        auto [first, inserted] = aDeclarations.emplace( aItem.id.Value(), aItem.source );

        if( !inserted )
        {
            throwValidationError(
                    aItem.source,
                    wxString::Format( wxS( "duplicate %s ID %u; first at v0x%04X %s controller %d record %llu "
                                           "sheet %d offset 0x%llX" ),
                                      aObjectClass, aItem.id.Value(), first->second.version, first->second.objectClass,
                                      first->second.controller,
                                      static_cast<unsigned long long>( first->second.recordIndex ), first->second.sheet,
                                      static_cast<unsigned long long>( first->second.absoluteOffset ) ) );
        }
    };

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
            validateNestedId( gate, wxS( "gate" ), gateDeclarations );
    }

    for( const MODEL_SYMBOL_DEFINITION& definition : definitions )
    {
        for( const MODEL_PIN_DEFINITION& pin : definition.pins )
            validateNestedId( pin, wxS( "pin" ), pinDeclarations );
    }

    const MODEL_INDEX index( *this );

    for( const MODEL_SHEET& sheet : sheets )
    {
        if( sheet.parent && !index.sheets.contains( sheet.parent->id.Value() ) )
            throwValidationError( sheet.parent->source, wxS( "unresolved sheet reference" ) );
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            auto definition = index.definitions.find( gate.definition.id.Value() );

            if( definition == index.definitions.end() )
                throwValidationError( gate.definition.source, wxS( "unresolved symbol definition reference" ) );

            for( const PIN_REFERENCE& pin : gate.pins )
            {
                auto pinOwner = index.pinOwners.find( pin.id.Value() );

                if( pinOwner == index.pinOwners.end() || pinOwner->second != definition->second )
                    throwValidationError( pin.source, wxS( "pin does not belong to gate definition" ) );
            }
        }
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !index.sheets.contains( placement.sheet.id.Value() ) )
            throwValidationError( placement.sheet.source, wxS( "unresolved placement sheet reference" ) );

        auto partType = index.partTypes.find( placement.partType.id.Value() );

        if( partType == index.partTypes.end() )
            throwValidationError( placement.partType.source, wxS( "unresolved placement part-type reference" ) );

        if( placement.gate )
        {
            auto gate = index.gates.find( placement.gate->id.Value() );
            auto gateOwner = index.gateOwners.find( placement.gate->id.Value() );

            if( gate == index.gates.end() || gateOwner == index.gateOwners.end()
                || gateOwner->second != partType->second || gate->second->unit != placement.unit )
                throwValidationError( placement.gate->source, wxS( "placement gate or unit mismatch" ) );
        }
    }

    for( const MODEL_NET& net : nets )
    {
        if( !index.sheets.contains( net.sheet.id.Value() ) )
            throwValidationError( net.sheet.source, wxS( "unresolved net sheet reference" ) );

        for( const MODEL_CONNECTION& connection : net.connections )
        {
            if( connection.endpoints.empty() )
                throwValidationError( connection.source, wxS( "connection has no endpoints" ) );

            for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
            {
                if( !endpointIsValid( index, endpoint ) )
                {
                    throwValidationError( endpoint.source, wxS( "empty, mixed, or unresolved connection endpoint" ) );
                }

                if( endpoint.placement )
                {
                    auto placement = index.placements.find( endpoint.placement->id.Value() );

                    if( placement != index.placements.end() && placement->second->sheet.id != net.sheet.id )
                    {
                        throwValidationError( endpoint.source,
                                              wxS( "connection endpoint placement sheet does not match net sheet" ) );
                    }
                }
            }
        }
    }

    for( const MODEL_BUS& bus : buses )
    {
        if( !index.sheets.contains( bus.sheet.id.Value() ) )
            throwValidationError( bus.sheet.source, wxS( "unresolved bus sheet reference" ) );

        for( const NET_REFERENCE& member : bus.memberNets )
        {
            auto net = index.nets.find( member.id.Value() );

            if( net == index.nets.end() )
                throwValidationError( member.source, wxS( "unresolved bus member-net reference" ) );

            if( net->second->sheet.id != bus.sheet.id )
                throwValidationError( member.source, wxS( "bus member-net sheet does not match bus sheet" ) );
        }

        for( const MODEL_BUS_ENTRY& entry : bus.entries )
        {
            auto net = index.nets.find( entry.memberNet.id.Value() );

            if( net == index.nets.end() )
                throwValidationError( entry.memberNet.source, wxS( "unresolved bus-entry net reference" ) );

            if( net->second->sheet.id != bus.sheet.id )
                throwValidationError( entry.memberNet.source, wxS( "bus-entry net sheet does not match bus sheet" ) );

            if( std::ranges::none_of( bus.memberNets,
                                      [&]( const NET_REFERENCE& aMember )
                                      {
                                          return aMember.id == entry.memberNet.id;
                                      } ) )
            {
                throwValidationError( entry.source, wxS( "bus-entry net is absent from bus member nets" ) );
            }
        }
    }

    for( const MODEL_LABEL& label : labels )
    {
        if( !index.sheets.contains( label.sheet.id.Value() ) )
            throwValidationError( label.sheet.source, wxS( "unresolved label sheet reference" ) );
    }

    for( const MODEL_JUNCTION& junction : junctions )
    {
        if( !index.sheets.contains( junction.sheet.id.Value() ) )
            throwValidationError( junction.sheet.source, wxS( "unresolved junction sheet reference" ) );
    }

    for( const MODEL_TEXT& text : texts )
    {
        if( !index.sheets.contains( text.sheet.id.Value() ) )
            throwValidationError( text.sheet.source, wxS( "unresolved text sheet reference" ) );
    }

    for( const MODEL_PAGE_GRAPHIC& graphic : graphics )
    {
        if( !index.sheets.contains( graphic.sheet.id.Value() ) )
            throwValidationError( graphic.sheet.source, wxS( "unresolved page-graphic sheet reference" ) );
    }

    for( const MODEL_SHEET& sheet : sheets )
    {
        std::set<SHEET_ID> ancestors;
        const MODEL_SHEET* current = &sheet;

        while( current->parent )
        {
            if( !ancestors.insert( current->id ).second )
                throwValidationError( current->source, wxS( "cyclic sheet hierarchy" ) );

            auto parent = index.sheets.find( current->parent->id.Value() );

            if( parent == index.sheets.end() )
                break;

            current = parent->second;
        }
    }
}


PADS_SCH_MODEL PADS_SCH_BINARY_PARSER::Parse( const std::vector<uint8_t>& aBytes, const wxString& aSourceName ) const
{
    PADS_SCH_SDB sdb;
    sdb.Load( aBytes );

    const PADS_IO::BINARY_CURSOR& cursor = sdb.Cursor();
    PADS_SCH_MODEL                model;
    model.version = sdb.Version();
    model.subversion = cursor.U16At( 6 );
    model.source = { aSourceName, model.version, wxS( "model" ), -1, 0, 0, aBytes.size(), -1 };
    model.settings.codePage = DEFAULT_CODE_PAGE;

    const SCH_SDB_POOL& sheetPool = sdb.Pools()[3];

    if( sheetPool.usedBytes != sheetPool.count * SHEET_RECORD_BYTES )
    {
        SOURCE_PROVENANCE source =
                sourceAt( aSourceName, model.version, wxS( "sheet index" ), 3, 0,
                          OUTER_DIRECTORY_OFFSET + 3 * OUTER_DESCRIPTOR_BYTES + OUTER_USED_BYTES_OFFSET, 4, -1 );
        throwDecodeError( source, wxS( "sheet-index byte count does not match 48-byte record count" ) );
    }

    const SCH_SDB_POOL& settingsPool = sdb.Pools()[5];

    if( settingsPool.count != 100 || settingsPool.usedBytes != 400 )
    {
        SOURCE_PROVENANCE source =
                sourceAt( aSourceName, model.version, wxS( "design settings" ), 5, 0,
                          OUTER_DIRECTORY_OFFSET + 5 * OUTER_DESCRIPTOR_BYTES + OUTER_USED_BYTES_OFFSET, 4, -1 );
        throwDecodeError( source, wxS( "design-settings controller is not the required 400-byte record" ) );
    }

    size_t            settingsOffset = outerControllerOffset( sdb, 5 );
    SOURCE_PROVENANCE settingsSource = sourceAt( aSourceName, model.version, wxS( "design settings" ), 5, 0,
                                                 settingsOffset, settingsPool.usedBytes, -1 );
    model.settings.source = settingsSource;

    uint8_t pageDesignator = 0;

    if( std::equal( aBytes.begin() + settingsOffset + 264, aBytes.begin() + settingsOffset + 268, "SIZE" )
        && aBytes[settingsOffset + 269] == 0 )
    {
        pageDesignator = aBytes[settingsOffset + 268];
    }
    else if( std::equal( aBytes.begin() + settingsOffset + 264, aBytes.begin() + settingsOffset + 273, "WDITBSIZE" )
             && aBytes[settingsOffset + 274] == 0 )
    {
        pageDesignator = aBytes[settingsOffset + 273];
    }
    else
    {
        SOURCE_PROVENANCE source = settingsSource;
        source.absoluteOffset += 264;
        source.length = 11;
        throwDecodeError( source, wxS( "invalid design page-size field" ) );
    }

    SOURCE_PROVENANCE pageSource = settingsSource;
    pageSource.absoluteOffset += 264;
    pageSource.length = 11;
    model.settings.pageSize = pageExtent( pageDesignator, pageSource );
    model.settings.defaultLineWidth = static_cast<int64_t>( cursor.U32At( settingsOffset + 12 ) ) * 2;
    model.settings.defaultBusWidth = static_cast<int64_t>( cursor.U32At( settingsOffset + 16 ) ) * 2;

    std::vector<MODEL_FIELD> titleFields;
    const SCH_SDB_POOL&      titleFieldPool = sdb.Pools()[1];
    size_t                   titleOffset = outerControllerOffset( sdb, 1 );
    size_t                   titleEnd = titleOffset + titleFieldPool.usedBytes;
    size_t                   titleRecord = 0;

    while( titleOffset + 6 <= titleEnd
           && std::equal( aBytes.begin() + titleOffset, aBytes.begin() + titleOffset + 6, "Field\n" ) )
    {
        size_t terminator = titleOffset;

        while( terminator < titleEnd && aBytes[terminator] != 0 )
            ++terminator;

        SOURCE_PROVENANCE source = sourceAt( aSourceName, model.version, wxS( "title field" ), 1, titleRecord,
                                             titleOffset, terminator - titleOffset + 1, -1 );

        size_t separator = titleOffset + 6;

        while( separator < terminator && aBytes[separator] != 1 )
            ++separator;

        if( terminator == titleEnd || terminator - titleOffset < 7 || separator == terminator
            || !std::equal( aBytes.begin() + titleOffset, aBytes.begin() + titleOffset + 6, "Field\n" ) )
        {
            throwDecodeError( source, wxS( "invalid title-field name record" ) );
        }

        std::vector<uint8_t> nameBytes( aBytes.begin() + titleOffset + 6, aBytes.begin() + separator );
        std::vector<uint8_t> valueBytes( aBytes.begin() + separator + 1, aBytes.begin() + terminator );
        SOURCE_PROVENANCE    nameSource = source;
        nameSource.absoluteOffset += 6;
        nameSource.length = nameBytes.size();
        SOURCE_PROVENANCE valueSource = source;
        valueSource.absoluteOffset = separator + 1;
        valueSource.length = valueBytes.size();
        MODEL_FIELD field;
        field.source = source;
        field.name = DecodeString( nameBytes, DEFAULT_CODE_PAGE, nameSource, model.diagnostics );
        field.value = DecodeString( valueBytes, DEFAULT_CODE_PAGE, valueSource, model.diagnostics );
        field.presentation.source = source;
        titleFields.push_back( std::move( field ) );
        titleOffset = terminator + 1;
        ++titleRecord;
    }

    if( titleFields.empty() )
    {
        SOURCE_PROVENANCE source = sourceAt( aSourceName, model.version, wxS( "title field" ), 1, titleFields.size(),
                                             outerControllerOffset( sdb, 1 ), titleFieldPool.usedBytes, -1 );
        throwDecodeError( source, wxS( "title-field controller is empty" ) );
    }

    size_t sheetIndexOffset = outerControllerOffset( sdb, 3 );
    auto   sheetBlocks = sdb.Blocks()
                       | std::views::filter(
                               []( const SCH_SDB_BLOCK& aBlock )
                               {
                                   return aBlock.kind == SCH_SDB_BLOCK_KIND::SHEET;
                               } );
    auto sheetBlock = sheetBlocks.begin();

    for( size_t index = 0; index < sheetPool.count; ++index, ++sheetBlock )
    {
        size_t            recordOffset = sheetIndexOffset + index * SHEET_RECORD_BYTES;
        SOURCE_PROVENANCE provenance = sourceAt( aSourceName, model.version, wxS( "sheet" ), 3, index, recordOffset,
                                                 SHEET_RECORD_BYTES, static_cast<int>( index ) );

        if( sheetBlock == sheetBlocks.end() || cursor.U32At( recordOffset ) != sheetBlock->offset
            || cursor.U32At( recordOffset + 4 ) != sheetBlock->bytes )
        {
            throwDecodeError( provenance, wxS( "sheet-index record references the wrong SDB object class" ) );
        }

        constexpr size_t nameOffset = 14;

        if( cursor.U16At( recordOffset + 10 ) != 0xFFFF || cursor.U16At( recordOffset + 12 ) != 0xFFFF )
        {
            throwDecodeError( provenance, wxS( "invalid sheet-index class marker" ) );
        }

        size_t nameEnd = recordOffset + nameOffset;

        while( nameEnd < recordOffset + SHEET_RECORD_BYTES && aBytes[nameEnd] != 0 )
            ++nameEnd;

        if( nameEnd == recordOffset + SHEET_RECORD_BYTES )
            throwDecodeError( provenance, wxS( "unterminated sheet name" ) );

        std::vector<uint8_t> nameBytes( aBytes.begin() + recordOffset + nameOffset, aBytes.begin() + nameEnd );
        SOURCE_PROVENANCE    nameSource = provenance;
        nameSource.absoluteOffset += nameOffset;
        nameSource.length = nameBytes.size();

        MODEL_SHEET sheet;
        sheet.id = SHEET_ID( cursor.U16At( recordOffset + 8 ) );
        sheet.index = index;
        sheet.source = provenance;
        sheet.name = DecodeString( nameBytes, DEFAULT_CODE_PAGE, nameSource, model.diagnostics );

        if( sheet.name.text.empty() )
            sheet.name.text = wxS( "$$$NONE" );

        sheet.pageSize = model.settings.pageSize;
        sheet.defaultLineWidth = model.settings.defaultLineWidth;
        sheet.defaultBusWidth = model.settings.defaultBusWidth;
        sheet.titleBlockFields = titleFields;

        auto title = std::ranges::find_if( sheet.titleBlockFields,
                                           []( const MODEL_FIELD& aField )
                                           {
                                               return aField.name.text == wxS( "Title" );
                                           } );

        if( title != sheet.titleBlockFields.end() )
            sheet.title = title->value;

        for( MODEL_FIELD& field : sheet.titleBlockFields )
        {
            field.source.sheet = static_cast<int>( index );
            field.name.source.sheet = static_cast<int>( index );
            field.value.source.sheet = static_cast<int>( index );
            field.presentation.source.sheet = static_cast<int>( index );
        }

        model.sheets.push_back( std::move( sheet ) );
    }

    size_t sheetIndex = 0;

    for( const SCH_SDB_BLOCK& block : sdb.Blocks() )
    {
        if( block.kind != SCH_SDB_BLOCK_KIND::SHEET )
            continue;

        size_t   descriptors = block.offset + SHEET_HEADER_BYTES;
        size_t   payload = descriptors + SHEET_DESCRIPTOR_COUNT * SHEET_DESCRIPTOR_BYTES;
        uint32_t textCount = cursor.U32At( descriptors + SHEET_COUNT_OFFSET );
        uint32_t textBytes = cursor.U32At( descriptors + SHEET_USED_BYTES_OFFSET );
        uint32_t heapBytes = cursor.U32At( descriptors + SHEET_DESCRIPTOR_BYTES + SHEET_USED_BYTES_OFFSET );

        SOURCE_PROVENANCE controllerSource = sourceAt( aSourceName, model.version, wxS( "text controller" ), 1, 0,
                                                       payload, textBytes, static_cast<int>( sheetIndex ) );

        if( textBytes != textCount * TEXT_RECORD_BYTES )
            throwDecodeError( controllerSource, wxS( "text-controller byte count does not match 32-byte records" ) );

        size_t heapOffset = payload + textBytes;

        if( model.version == 0x000C )
        {
            SOURCE_PROVENANCE heapSource = sourceAt( aSourceName, model.version, wxS( "text string controller" ), 2, 0,
                                                     heapOffset, heapBytes, static_cast<int>( sheetIndex ) );
            model.preservedControllerPayloads.push_back(
                    { controllerSource,
                      PROPERTY_DISPOSITION::PRESERVED,
                      { aBytes.begin() + payload, aBytes.begin() + payload + textBytes } } );
            model.preservedControllerPayloads.push_back(
                    { heapSource,
                      PROPERTY_DISPOSITION::PRESERVED,
                      { aBytes.begin() + heapOffset, aBytes.begin() + heapOffset + heapBytes } } );
            ++sheetIndex;
            continue;
        }

        for( size_t record = 0; record < textCount; ++record )
        {
            size_t recordOffset = payload + record * TEXT_RECORD_BYTES;

            if( cursor.U16At( recordOffset + 24 ) != cursor.U16At( recordOffset + 26 ) )
                continue;

            SOURCE_PROVENANCE textSource = sourceAt( aSourceName, model.version, wxS( "free text" ), 1, record,
                                                     recordOffset, TEXT_RECORD_BYTES, static_cast<int>( sheetIndex ) );
            uint32_t          stringOffset = cursor.U32At( recordOffset + 8 );
            uint16_t          stringBytes = cursor.U16At( recordOffset + 20 );

            if( stringBytes == 0 || stringOffset > heapBytes || stringBytes > heapBytes - stringOffset
                || aBytes[heapOffset + stringOffset + stringBytes - 1] != 0 )
            {
                throwDecodeError( textSource, wxS( "free-text string reference leaves controller 2" ) );
            }

            SOURCE_PROVENANCE stringSource =
                    sourceAt( aSourceName, model.version, wxS( "free text string" ), 2, record,
                              heapOffset + stringOffset, stringBytes - 1, static_cast<int>( sheetIndex ) );
            std::vector<uint8_t> string( aBytes.begin() + stringSource.absoluteOffset,
                                         aBytes.begin() + stringSource.absoluteOffset + stringSource.length );
            uint16_t             justification = cursor.U16At( recordOffset + 18 );

            MODEL_TEXT text;
            text.source = textSource;
            text.sheet = { model.sheets[sheetIndex].id, textSource };
            text.text = DecodeString( string, DEFAULT_CODE_PAGE, stringSource, model.diagnostics );
            text.position = { decodeCoordinate( cursor.U16At( recordOffset + 12 ) ),
                              decodeCoordinate( cursor.U16At( recordOffset + 14 ) ), textSource };
            text.angle = NormalizeAngle( cursor.U16At( recordOffset + 16 ) );
            text.presentation.source = textSource;
            text.presentation.height = static_cast<int64_t>( cursor.U16At( recordOffset + 22 ) ) * 2;
            text.presentation.width = static_cast<int64_t>( cursor.U16At( recordOffset + 30 ) ) * 2;
            text.presentation.horizontalJustification =
                    horizontalJustification( justification, textSource, model.diagnostics );
            text.presentation.verticalJustification = verticalJustification( justification );

            SOURCE_PROVENANCE relationshipSource = textSource;
            relationshipSource.objectClass = wxS( "free text relationship" );
            relationshipSource.absoluteOffset += 28;
            relationshipSource.length = 2;
            uint16_t        relationship = cursor.U16At( relationshipSource.absoluteOffset );
            SOURCE_PROPERTY relationshipProperty;
            relationshipProperty.name.text = wxS( "controller_1_relationship_word_28" );
            relationshipProperty.name.source = relationshipSource;
            relationshipProperty.value.raw = { static_cast<uint8_t>( relationship ),
                                               static_cast<uint8_t>( relationship >> 8 ) };
            relationshipProperty.value.text = wxString::Format( wxS( "%u" ), relationship );
            relationshipProperty.value.encoding = STRING_ENCODING_STATUS::CODE_PAGE;
            relationshipProperty.value.source = relationshipSource;
            relationshipProperty.value.codePage = DEFAULT_CODE_PAGE;
            relationshipProperty.value.codePageName = wxS( "windows-1252" );
            relationshipProperty.disposition = PROPERTY_DISPOSITION::PRESERVED;
            relationshipProperty.source = relationshipSource;
            text.properties.push_back( std::move( relationshipProperty ) );
            model.texts.push_back( std::move( text ) );
        }

        ++sheetIndex;
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

    if( aCodePage != 65001 && aCodePage != 1252 )
    {
        aDiagnostics.push_back( { RPT_SEVERITY_WARNING, aSource,
                                  wxString::Format( wxS( "unknown code page %u; bytes preserved and "
                                                         "non-ASCII bytes decoded as U+FFFD" ),
                                                    aCodePage ) } );
    }

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

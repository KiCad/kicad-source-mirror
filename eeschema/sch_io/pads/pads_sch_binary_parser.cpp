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
#include <array>
#include <functional>
#include <iterator>
#include <map>
#include <ranges>
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
    constexpr size_t   SYMBOL_RECORD_BYTES = 80;
    constexpr size_t   SYMBOL_PIECE_BYTES = 6;
    constexpr size_t   SYMBOL_VERTEX_BYTES = 6;
    constexpr size_t   SYMBOL_ARC_BYTES = 14;
    constexpr size_t   USED_DECAL_BYTES = 108;
    constexpr size_t   TERMINAL_BYTES = 26;
    constexpr size_t   PART_TYPE_BYTES = 76;
    constexpr size_t   GATE_BYTES = 12;
    constexpr size_t   PIN_BYTES = 24;
    constexpr size_t   SIGNAL_PIN_BYTES = 64;
    constexpr size_t   NET_MEMBERSHIP_BYTES = 2;
    constexpr size_t   NET_RECORD_BYTES = 88;
    constexpr size_t   BUS_RECORD_BYTES = 44;
    constexpr size_t   JUNCTION_RECORD_BYTES = 12;
    constexpr size_t   OFFPAGE_RECORD_BYTES = 32;
    constexpr size_t   CONNECTION_RECORD_BYTES = 40;
    constexpr size_t   CONNECTION_VERTEX_BYTES = 8;
    constexpr size_t   PLACEMENT_GROUP_BYTES = 24;
    constexpr size_t   ATTRIBUTE_OFFSET_BYTES = 4;
    constexpr size_t   FONT_RECORD_BYTES = 36;
    constexpr uint32_t DEFAULT_CODE_PAGE = 1252;

    struct GLOBAL_NET_RECORD
    {
        SOURCE_PROVENANCE          source;
        SOURCE_STRING              name;
        uint32_t                   preservedIdentity = 0;
        uint32_t                   preservedRelationship = 0xFFFFFFFF;
        uint32_t                   membershipStart = 0;
        uint16_t                   membershipCount = 0;
        uint32_t                   aliasStringOffset = 0xFFFFFFFF;
        uint16_t                   aliasCount = 0;
        uint16_t                   kindFlags = 0;
        bool                       tombstone = false;
        std::vector<SOURCE_STRING> aliasMembers;
    };


    struct CONNECTIVITY_GLOBALS
    {
        size_t                         membershipBase = 0;
        uint32_t                       membershipCount = 0;
        std::vector<uint16_t>          membershipSheets;
        std::vector<GLOBAL_NET_RECORD> nets;
    };

    struct PLACEMENT_LAYOUT
    {
        uint16_t version;
        bool     decoded;
        size_t   placementBytes;
        size_t   placedPinBytes;
        size_t   fieldBytes;
        size_t   pinStart;
        size_t   componentIdentity;
        size_t   componentGroup;
        size_t   x;
        size_t   y;
        size_t   angle;
        size_t   mirror;
        size_t   partType;
        size_t   decal;
        size_t   gate;
        size_t   pinCount;
        size_t   fieldCount;
        size_t   reference;
        size_t   referenceFont;
        size_t   partTypeFont;
        size_t   referenceField;
        size_t   referenceFieldAngle;
        size_t   partTypeField;
        size_t   partTypeFieldAngle;
        size_t   referenceHeight;
        size_t   partTypeHeight;
        size_t   referenceWidth;
        size_t   partTypeWidth;
        size_t   placedPinOrdinal;
        size_t   customFont;
        size_t   customX;
        size_t   customAngle;
        size_t   customJustification;
        size_t   customLineWidth;
        size_t   customHeight;
        size_t   customWidth;
        size_t   customDisplayFlags;
        size_t   customTail;
    };

    constexpr PLACEMENT_LAYOUT PLACEMENT_LAYOUTS[] = { []
                                                       {
                                                           PLACEMENT_LAYOUT layout{};
                                                           layout.version = 0x000C;
                                                           return layout;
                                                       }(),
                                                       { .version = 0x000D,
                                                         .decoded = true,
                                                         .placementBytes = 136,
                                                         .placedPinBytes = 12,
                                                         .fieldBytes = 24,
                                                         .pinStart = 0x14,
                                                         .componentIdentity = 0x18,
                                                         .componentGroup = 0x1C,
                                                         .x = 0x20,
                                                         .y = 0x22,
                                                         .angle = 0x24,
                                                         .mirror = 0x26,
                                                         .partType = 0x42,
                                                         .decal = 0x44,
                                                         .gate = 0x4A,
                                                         .pinCount = 0x4C,
                                                         .fieldCount = 0x4E,
                                                         .reference = 0x5E,
                                                         .referenceFont = 0,
                                                         .partTypeFont = 2,
                                                         .referenceField = 0x28,
                                                         .referenceFieldAngle = 0x2C,
                                                         .partTypeField = 0x30,
                                                         .partTypeFieldAngle = 0x34,
                                                         .referenceHeight = 0x50,
                                                         .partTypeHeight = 0x52,
                                                         .referenceWidth = 0x58,
                                                         .partTypeWidth = 0x59,
                                                         .placedPinOrdinal = 4,
                                                         .customFont = 0,
                                                         .customX = 8,
                                                         .customAngle = 12,
                                                         .customJustification = 14,
                                                         .customLineWidth = 16,
                                                         .customHeight = 18,
                                                         .customWidth = 20,
                                                         .customDisplayFlags = 21,
                                                         .customTail = 22 } };

    const PLACEMENT_LAYOUT& placementLayout( uint16_t aVersion )
    {
        auto layout = std::ranges::find_if( PLACEMENT_LAYOUTS,
                                            [&]( const PLACEMENT_LAYOUT& aLayout )
                                            {
                                                return aLayout.version == aVersion;
                                            } );

        if( layout == std::end( PLACEMENT_LAYOUTS ) )
            THROW_IO_ERROR( wxString::Format( wxS( "unsupported PADS placement layout v0x%04X" ), aVersion ) );

        return *layout;
    }


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


    int64_t decodeLocalCoordinate( uint16_t aRaw )
    {
        return static_cast<int64_t>( static_cast<int16_t>( aRaw ) ) * 2;
    }


    SOURCE_STRING decodeFixedString( const std::vector<uint8_t>& aBytes, size_t aOffset, size_t aBytesAvailable,
                                     const SOURCE_PROVENANCE& aSource, std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        size_t end = aOffset;

        while( end < aOffset + aBytesAvailable && aBytes[end] != 0 )
            ++end;

        if( end == aOffset + aBytesAvailable )
            throwDecodeError( aSource, wxS( "unterminated fixed string" ) );

        return PADS_SCH_BINARY_PARSER::DecodeString( { aBytes.begin() + aOffset, aBytes.begin() + end },
                                                     DEFAULT_CODE_PAGE, aSource, aDiagnostics );
    }


    SOURCE_PROPERTY sourceProperty( const wxString& aName, const wxString& aValue, const SOURCE_PROVENANCE& aSource )
    {
        SOURCE_PROPERTY property;
        property.name.text = aName;
        property.name.source = aSource;
        property.value.text = aValue;
        property.value.source = aSource;
        property.disposition = PROPERTY_DISPOSITION::EXACT;
        property.source = aSource;
        return property;
    }


    SOURCE_STRING decodedDefinitionFont( int16_t aHandle, const SOURCE_PROVENANCE& aSource )
    {
        SOURCE_STRING font;
        font.source = aSource;

        if( aHandle == -1 || aHandle == -4 )
        {
            font.text = wxS( "Default Font" );
            font.encoding = STRING_ENCODING_STATUS::CODE_PAGE;
            font.codePage = DEFAULT_CODE_PAGE;
            font.codePageName = wxS( "windows-1252" );
        }

        return font;
    }


    uint32_t pinElectricalType( uint8_t aType, const SOURCE_PROVENANCE& aSource,
                                std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        switch( aType )
        {
        case 'U': return 0;
        case 'L': return 1;
        case 'S': return 2;
        case 'B': return 3;
        case 'T': return 4;
        case 'C': return 5;
        case 'E': return 6;
        case 'P':
        case 'G': return 7;
        case 'Z': return 8;
        default:
            PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "pin electrical type" ), aType, aSource, aDiagnostics );
            return 8;
        }
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
        const uint16_t horizontal = aValue & ~uint16_t{ 0x0A };

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

        if( placement == aIndex.placements.end() )
            return false;

        return aIndex.pins.contains( aEndpoint.pin->id.Value() )
               && std::ranges::any_of( placement->second->pins,
                                       [&]( const PIN_REFERENCE& aPin )
                                       {
                                           return aPin.id == aEndpoint.pin->id;
                                       } );
    }


    struct SHEET_CONTROLLERS
    {
        std::array<SCH_SDB_POOL, 23> pools;
        std::array<size_t, 23>       offsets;
    };


    struct PLACEMENT_GLOBALS
    {
        size_t   attributeHeapBase = 0;
        uint32_t attributeHeapBytes = 0;
        size_t   groupBase = 0;
        uint32_t groupCount = 0;
        size_t   attributeOffsetBase = 0;
        uint32_t attributeOffsetCount = 0;
        size_t   fontBase = 0;
        uint32_t fontCount = 0;
    };


    PLACEMENT_GLOBALS placementGlobals( const PADS_SCH_SDB& aSdb, const wxString& aSourceName )
    {
        PLACEMENT_GLOBALS result;

        auto requireOuterStride = [&]( size_t aController, size_t aStride )
        {
            const SCH_SDB_POOL& pool = aSdb.Pools()[aController];

            if( pool.usedBytes != pool.count * aStride )
            {
                SOURCE_PROVENANCE source = sourceAt(
                        aSourceName, aSdb.Version(), wxS( "outer controller directory" ), aController, 0,
                        OUTER_DIRECTORY_OFFSET + aController * OUTER_DESCRIPTOR_BYTES + OUTER_USED_BYTES_OFFSET, 4,
                        -1 );
                throwDecodeError( source,
                                  wxString::Format( wxS( "controller byte count does not match %llu-byte records" ),
                                                    static_cast<unsigned long long>( aStride ) ) );
            }
        };

        requireOuterStride( 6, PLACEMENT_GROUP_BYTES );
        requireOuterStride( 7, ATTRIBUTE_OFFSET_BYTES );
        requireOuterStride( 19, FONT_RECORD_BYTES );
        result.attributeHeapBase = outerControllerOffset( aSdb, 2 );
        result.attributeHeapBytes = aSdb.Pools()[2].usedBytes;
        result.groupBase = outerControllerOffset( aSdb, 6 );
        result.groupCount = aSdb.Pools()[6].count;
        result.attributeOffsetBase = outerControllerOffset( aSdb, 7 );
        result.attributeOffsetCount = aSdb.Pools()[7].count;
        result.fontBase = outerControllerOffset( aSdb, 19 );
        result.fontCount = aSdb.Pools()[19].count;
        return result;
    }


    CONNECTIVITY_GLOBALS connectivityGlobals( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                              const PADS_SCH_SDB& aSdb, const wxString& aSourceName,
                                              PADS_SCH_MODEL& aModel )
    {
        CONNECTIVITY_GLOBALS result;
        const SCH_SDB_POOL&  membershipPool = aSdb.Pools()[4];
        const SCH_SDB_POOL&  netPool = aSdb.Pools()[8];

        if( membershipPool.usedBytes != membershipPool.count * NET_MEMBERSHIP_BYTES )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "net membership directory" ), 4, 0,
                                                 outerControllerOffset( aSdb, 4 ), membershipPool.usedBytes, -1 );
            throwDecodeError( source, wxS( "controller byte count does not match 2-byte records" ) );
        }

        if( netPool.usedBytes != netPool.count * NET_RECORD_BYTES )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "net directory" ), 8, 0,
                                                 outerControllerOffset( aSdb, 8 ), netPool.usedBytes, -1 );
            throwDecodeError( source, wxS( "controller byte count does not match 88-byte records" ) );
        }

        result.membershipBase = outerControllerOffset( aSdb, 4 );
        result.membershipCount = membershipPool.count;
        result.membershipSheets.reserve( membershipPool.count );

        for( size_t record = 0; record < membershipPool.count; ++record )
            result.membershipSheets.push_back( aCursor.U16At( result.membershipBase + record * 2 ) );

        const size_t      netBase = outerControllerOffset( aSdb, 8 );
        const size_t      aliasHeapBase = outerControllerOffset( aSdb, 1 );
        const uint32_t    aliasHeapBytes = aSdb.Pools()[1].usedBytes;
        std::vector<bool> claimedMemberships( membershipPool.count, false );

        result.nets.reserve( netPool.count );

        for( size_t record = 0; record < netPool.count; ++record )
        {
            const size_t      offset = netBase + record * NET_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "global net" ), 8, record, offset,
                                                 NET_RECORD_BYTES, -1 );
            GLOBAL_NET_RECORD net;
            net.source = source;
            net.preservedIdentity = aCursor.U32At( offset );
            net.preservedRelationship = aCursor.U32At( offset + 84 );
            net.membershipStart = aCursor.U32At( offset + 4 );
            net.aliasStringOffset = aCursor.U32At( offset + 8 );
            net.membershipCount = aCursor.U16At( offset + 16 );
            net.aliasCount = aCursor.U16At( offset + 18 );
            net.kindFlags = aCursor.U16At( offset + 22 );
            net.tombstone = net.membershipCount == 0xFFFF;
            SOURCE_PROVENANCE nameSource = source;
            nameSource.absoluteOffset += 24;
            nameSource.length = 56;
            net.name = decodeFixedString( aBytes, offset + 24, 56, nameSource, aModel.diagnostics );

            if( aCursor.U16At( offset + 20 ) != 0 || aCursor.U32At( offset + 80 ) != 0xFFFFFFFF )
            {
                throwDecodeError( source, wxS( "global net record has nonzero padding or a live reserved link" ) );
            }

            if( !net.tombstone
                && ( net.membershipStart > membershipPool.count
                     || net.membershipCount > membershipPool.count - net.membershipStart ) )
            {
                throwDecodeError( source, wxS( "net sheet-membership slice leaves outer controller 4" ) );
            }

            for( uint32_t membership = net.membershipStart;
                 !net.tombstone && membership < net.membershipStart + net.membershipCount; ++membership )
            {
                if( claimedMemberships[membership] )
                    throwDecodeError( source, wxS( "duplicate net sheet-membership ownership" ) );

                if( result.membershipSheets[membership] >= aModel.sheets.size() )
                    throwDecodeError( source, wxS( "net membership references the wrong sheet object class" ) );

                claimedMemberships[membership] = true;
            }

            if( net.aliasCount == 0 )
            {
                if( net.aliasStringOffset != 0xFFFFFFFF )
                    throwDecodeError( source, wxS( "non-alias net has an alias-string handle" ) );
            }
            else
            {
                if( net.aliasStringOffset >= aliasHeapBytes )
                    throwDecodeError( source, wxS( "bus-alias string handle leaves outer controller 1" ) );

                size_t stringOffset = net.aliasStringOffset;

                for( size_t alias = 0; alias < net.aliasCount; ++alias )
                {
                    SOURCE_PROVENANCE aliasSource =
                            sourceAt( aSourceName, aModel.version, wxS( "bus alias member" ), 1, alias,
                                      aliasHeapBase + stringOffset, aliasHeapBytes - stringOffset, -1 );
                    SOURCE_STRING member =
                            decodeFixedString( aBytes, aliasHeapBase + stringOffset, aliasHeapBytes - stringOffset,
                                               aliasSource, aModel.diagnostics );
                    aliasSource.length = member.raw.size();
                    member.source.length = member.raw.size();
                    stringOffset += member.raw.size() + 1;
                    net.aliasMembers.push_back( std::move( member ) );
                }
            }

            result.nets.push_back( std::move( net ) );
        }

        return result;
    }


    SHEET_CONTROLLERS sheetControllers( const PADS_IO::BINARY_CURSOR& aCursor, const SCH_SDB_BLOCK& aBlock )
    {
        SHEET_CONTROLLERS result;
        size_t            descriptor = aBlock.offset + SHEET_HEADER_BYTES;
        size_t            payload = descriptor + 24 * SHEET_DESCRIPTOR_BYTES;

        for( size_t i = 0; i < result.pools.size(); ++i )
        {
            result.pools[i].count = aCursor.U32At( descriptor + i * SHEET_DESCRIPTOR_BYTES + 12 );
            result.pools[i].usedBytes = aCursor.U32At( descriptor + i * SHEET_DESCRIPTOR_BYTES + 16 );
            result.offsets[i] = payload;
            payload += result.pools[i].usedBytes;
        }

        return result;
    }


    void requireFixedController( const SHEET_CONTROLLERS& aControllers, size_t aController, size_t aStride,
                                 const wxString& aFile, uint16_t aVersion, int aSheet )
    {
        const SCH_SDB_POOL& pool = aControllers.pools[aController - 1];

        if( pool.usedBytes != pool.count * aStride )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aFile, aVersion, wxS( "controller directory" ), static_cast<int>( aController ), 0,
                              aControllers.offsets[aController - 1], pool.usedBytes, aSheet );
            throwDecodeError( source, wxString::Format( wxS( "controller byte count does not match %llu-byte records" ),
                                                        static_cast<unsigned long long>( aStride ) ) );
        }
    }


    void decodeDefinitionsAndParts( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                    const SCH_SDB_BLOCK& aBlock, size_t aSheetIndex, const wxString& aSourceName,
                                    PADS_SCH_MODEL& aModel )
    {
        if( aModel.version == 0x000C )
        {
            const SHEET_CONTROLLERS controllers = sheetControllers( aCursor, aBlock );

            for( size_t controller = 3; controller <= 23; ++controller )
            {
                const SCH_SDB_POOL& pool = controllers.pools[controller - 1];

                if( pool.usedBytes == 0 )
                    continue;

                SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "definition controller" ),
                                                     controller, 0, controllers.offsets[controller - 1], pool.usedBytes,
                                                     static_cast<int>( aSheetIndex ) );
                aModel.preservedControllerPayloads.push_back(
                        { source,
                          PROPERTY_DISPOSITION::PRESERVED,
                          { aBytes.begin() + source.absoluteOffset,
                            aBytes.begin() + source.absoluteOffset + source.length } } );
            }

            return;
        }

        const SHEET_CONTROLLERS controllers = sheetControllers( aCursor, aBlock );
        requireFixedController( controllers, 3, SYMBOL_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 4, SYMBOL_PIECE_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 5, SYMBOL_VERTEX_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 6, SYMBOL_ARC_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 7, USED_DECAL_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 8, TERMINAL_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 9, PART_TYPE_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 10, GATE_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 11, PIN_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 12, SIGNAL_PIN_BYTES, aSourceName, aModel.version, aSheetIndex );

        const size_t   symbolBase = controllers.offsets[2];
        const size_t   pieceBase = controllers.offsets[3];
        const size_t   vertexBase = controllers.offsets[4];
        const size_t   arcBase = controllers.offsets[5];
        const size_t   usedDecalBase = controllers.offsets[6];
        const size_t   terminalBase = controllers.offsets[7];
        const size_t   partBase = controllers.offsets[8];
        const size_t   gateBase = controllers.offsets[9];
        const size_t   pinBase = controllers.offsets[10];
        const size_t   signalPinBase = controllers.offsets[11];
        const size_t   textBase = controllers.offsets[0];
        const size_t   textHeapBase = controllers.offsets[1];
        const size_t   pinNameBase = controllers.offsets[13];
        const uint32_t textCount = controllers.pools[0].count;
        const uint32_t textHeapBytes = controllers.pools[1].usedBytes;
        const uint32_t pinNameBytes = controllers.pools[13].usedBytes;
        const uint32_t definitionIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 1 );
        const uint32_t pinIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x10000 );
        const uint32_t partIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x20000 );
        const uint32_t gateIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x30000 );

        // These are prefix sums over file-supplied per-record counts. A 32-bit accumulator wraps on
        // a crafted file and the totals below then compare small against their pools while the
        // stored offsets are already nonsense, so accumulate in 64 bits
        std::vector<uint32_t> pieceVertexStart;
        uint64_t              vertexCursor = 0;
        uint64_t              pieceCursor = 0;
        std::vector<uint32_t> definitionPieceStart;

        for( size_t definition = 0; definition < controllers.pools[2].count; ++definition )
        {
            definitionPieceStart.push_back( static_cast<uint32_t>( pieceCursor ) );
            pieceCursor += aCursor.U16At( symbolBase + definition * SYMBOL_RECORD_BYTES + 0x2A );
        }

        if( pieceCursor != controllers.pools[3].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "symbol definition" ), 3, 0, symbolBase,
                              controllers.pools[2].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "symbol graphic-piece counts leave controller 4" ) );
        }

        for( size_t piece = 0; piece < controllers.pools[3].count; ++piece )
        {
            pieceVertexStart.push_back( static_cast<uint32_t>( vertexCursor ) );
            vertexCursor += aCursor.U16At( pieceBase + piece * SYMBOL_PIECE_BYTES + 2 );
        }

        if( vertexCursor != controllers.pools[4].count )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "symbol piece" ), 4, 0, pieceBase,
                                                 controllers.pools[3].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "symbol piece vertex counts do not consume controller 5" ) );
        }

        for( size_t definition = 0; definition < controllers.pools[2].count; ++definition )
        {
            const size_t   definitionOffset = symbolBase + definition * SYMBOL_RECORD_BYTES;
            const uint32_t firstPiece = definitionPieceStart[definition];
            const uint32_t pieceEnd = definition + 1 < definitionPieceStart.size()
                                              ? definitionPieceStart[definition + 1]
                                              : controllers.pools[3].count;
            const uint32_t firstVertex = aCursor.U32At( definitionOffset + 0x34 );
            const uint32_t vertexEnd = definition + 1 < controllers.pools[2].count
                                               ? aCursor.U32At( definitionOffset + SYMBOL_RECORD_BYTES + 0x34 )
                                               : controllers.pools[4].count;
            const bool     emptyMatches = firstPiece == pieceEnd && firstVertex == vertexEnd;
            const bool     ownedMatches =
                    firstPiece < pieceEnd && pieceVertexStart[firstPiece] == firstVertex
                    && pieceVertexStart[pieceEnd - 1]
                                       + aCursor.U16At( pieceBase + ( pieceEnd - 1 ) * SYMBOL_PIECE_BYTES + 2 )
                               == vertexEnd;

            if( !emptyMatches && !ownedMatches )
            {
                SOURCE_PROVENANCE source =
                        sourceAt( aSourceName, aModel.version, wxS( "symbol definition" ), 3, definition,
                                  definitionOffset, SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
                throwDecodeError( source, wxS( "symbol piece/vertex ownership mismatch" ) );
            }
        }

        std::vector<std::vector<size_t>> pieceArcRecords( controllers.pools[3].count );
        size_t                           discoveredArcCount = 0;

        for( size_t piece = 0; piece < controllers.pools[3].count; ++piece )
        {
            const size_t   pieceOffset = pieceBase + piece * SYMBOL_PIECE_BYTES;
            const uint16_t pointCount = aCursor.U16At( pieceOffset + 2 );

            for( size_t point = 0; point < pointCount; ++point )
            {
                const size_t vertexOffset = vertexBase + ( pieceVertexStart[piece] + point ) * SYMBOL_VERTEX_BYTES;

                if( static_cast<int16_t>( aCursor.U16At( vertexOffset + 4 ) ) >= 0 )
                    pieceArcRecords[piece].push_back( discoveredArcCount++ );
            }
        }

        if( discoveredArcCount != controllers.pools[5].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "symbol arc" ), 6, discoveredArcCount, arcBase,
                              controllers.pools[5].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "arc markers do not consume controller 6" ) );
        }

        std::vector<MODEL_SYMBOL_DEFINITION*> definitionsByRecord( controllers.pools[2].count );
        std::vector<size_t>                   pageGraphicRecords;
        aModel.definitions.reserve( aModel.definitions.size() + controllers.pools[2].count );

        for( size_t record = 0; record < controllers.pools[2].count; ++record )
        {
            const size_t      offset = symbolBase + record * SYMBOL_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "symbol definition" ), 3, record,
                                                 offset, SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 38;
            SOURCE_STRING  name = decodeFixedString( aBytes, offset, 38, nameSource, aModel.diagnostics );
            const uint8_t  objectClass = aCursor.U8At( offset + 0x29 );
            const uint32_t vertexStart = aCursor.U32At( offset + 0x34 );
            const uint32_t vertexEnd = record + 1 < controllers.pools[2].count
                                               ? aCursor.U32At( offset + SYMBOL_RECORD_BYTES + 0x34 )
                                               : controllers.pools[4].count;

            if( vertexStart > vertexEnd || vertexEnd > controllers.pools[4].count )
                throwDecodeError( source, wxS( "symbol definition vertex slice leaves controller 5" ) );

            if( objectClass == 0 )
            {
                const uint32_t firstPiece = definitionPieceStart[record];
                const uint32_t pieceEnd = record + 1 < definitionPieceStart.size() ? definitionPieceStart[record + 1]
                                                                                   : controllers.pools[3].count;
                const uint16_t groupTextCount = aCursor.U16At( offset + 64 );
                const uint16_t groupLastText = aCursor.U16At( offset + 66 );
                const bool     hasContiguousTextOwnership =
                        groupTextCount == 0
                        || ( groupLastText < textCount && groupTextCount <= static_cast<size_t>( groupLastText ) + 1 );

                if( !hasContiguousTextOwnership )
                {
                    aModel.diagnostics.push_back(
                            { RPT_SEVERITY_WARNING, source,
                              wxS( "unsupported class-zero drawing text relationship preserved" ) } );
                }

                if( aCursor.U16At( offset + 42 ) != pieceEnd - firstPiece )
                    throwDecodeError( source, wxS( "page-graphic piece count does not match controller 4 slice" ) );

                for( size_t piece = firstPiece; piece < pieceEnd; ++piece )
                {
                    const size_t      pieceOffset = pieceBase + piece * SYMBOL_PIECE_BYTES;
                    const uint8_t     pieceKind = aCursor.U8At( pieceOffset );
                    const uint8_t     lineStyle = aCursor.U8At( pieceOffset + 1 );
                    const uint16_t    pointCount = aCursor.U16At( pieceOffset + 2 );
                    const uint32_t    firstVertex = pieceVertexStart[piece];
                    SOURCE_PROVENANCE graphicSource =
                            sourceAt( aSourceName, aModel.version, wxS( "page graphic" ), 4, piece, pieceOffset,
                                      SYMBOL_PIECE_BYTES, static_cast<int>( aSheetIndex ) );

                    if( firstVertex + pointCount > vertexEnd )
                        throwDecodeError( graphicSource, wxS( "page graphic crosses its controller-5 slice" ) );

                    MODEL_GRAPHIC graphic;
                    graphic.source = graphicSource;
                    graphic.strokeWidth = static_cast<int64_t>( aCursor.U16At( pieceOffset + 4 ) ) * 2;

                    switch( lineStyle )
                    {
                    case 0xFF: graphic.lineStyle = MODEL_LINE_STYLE::SOLID; break;
                    case 0: graphic.lineStyle = MODEL_LINE_STYLE::DASH; break;
                    case 1: graphic.lineStyle = MODEL_LINE_STYLE::DOT; break;
                    case 2: graphic.lineStyle = MODEL_LINE_STYLE::DASH_DOT; break;
                    default:
                        PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "page graphic line style" ), lineStyle,
                                                                   graphicSource, aModel.diagnostics );
                        graphic.lineStyle = MODEL_LINE_STYLE::DEFAULT;
                        break;
                    }

                    switch( pieceKind )
                    {
                    case 0:
                        graphic.kind = pointCount == 2 ? MODEL_GRAPHIC_KIND::LINE : MODEL_GRAPHIC_KIND::POLYLINE;
                        break;
                    case 1: graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE; break;
                    case 2: graphic.kind = MODEL_GRAPHIC_KIND::CIRCLE; break;
                    case 4:
                        graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE;
                        graphic.fill = MODEL_FILL_STYLE::FILLED;
                        break;
                    default:
                        PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "page graphic kind" ), pieceKind, graphicSource,
                                                                   aModel.diagnostics );
                        graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE;
                        break;
                    }

                    for( size_t point = 0; point < pointCount; ++point )
                    {
                        const size_t      pointOffset = vertexBase + ( firstVertex + point ) * SYMBOL_VERTEX_BYTES;
                        SOURCE_PROVENANCE pointSource = sourceAt(
                                aSourceName, aModel.version, wxS( "page graphic vertex" ), 5, firstVertex + point,
                                pointOffset, SYMBOL_VERTEX_BYTES, static_cast<int>( aSheetIndex ) );
                        graphic.points.push_back( { decodeLocalCoordinate( aCursor.U16At( pointOffset ) ),
                                                    decodeLocalCoordinate( aCursor.U16At( pointOffset + 2 ) ),
                                                    pointSource } );
                    }

                    const int16_t arcMarker =
                            static_cast<int16_t>( aCursor.U16At( vertexBase + firstVertex * SYMBOL_VERTEX_BYTES + 4 ) );

                    if( pieceKind == 0 && arcMarker >= 0 )
                    {
                        if( pieceArcRecords[piece].empty() )
                            throwDecodeError( graphicSource, wxS( "page arc has no controller-6 record" ) );

                        const size_t      arcRecord = pieceArcRecords[piece].front();
                        const size_t      arcOffset = arcBase + arcRecord * SYMBOL_ARC_BYTES;
                        SOURCE_PROVENANCE arcSource =
                                sourceAt( aSourceName, aModel.version, wxS( "page arc" ), 6, arcRecord, arcOffset,
                                          SYMBOL_ARC_BYTES, static_cast<int>( aSheetIndex ) );
                        graphic.kind = MODEL_GRAPHIC_KIND::ARC;
                        graphic.arcSweepAngle = aCursor.U16At( arcOffset );
                        graphic.arcClockwise = static_cast<int16_t>( aCursor.U16At( arcOffset + 2 ) ) < 0;
                        graphic.arcBoundsStart = { decodeLocalCoordinate( aCursor.U16At( arcOffset + 6 ) ),
                                                   decodeLocalCoordinate( aCursor.U16At( arcOffset + 8 ) ), arcSource };
                        graphic.arcBoundsEnd = { decodeLocalCoordinate( aCursor.U16At( arcOffset + 10 ) ),
                                                 decodeLocalCoordinate( aCursor.U16At( arcOffset + 12 ) ), arcSource };
                        graphic.arcCenter = { ( graphic.arcBoundsStart.x + graphic.arcBoundsEnd.x ) / 2,
                                              ( graphic.arcBoundsStart.y + graphic.arcBoundsEnd.y ) / 2, arcSource };
                    }

                    graphic.properties.push_back( sourceProperty( wxS( "page_graphic_group" ), name.text, source ) );

                    if( !hasContiguousTextOwnership )
                    {
                        SOURCE_PROVENANCE relationshipSource = source;
                        relationshipSource.absoluteOffset += 64;
                        relationshipSource.length = 4;
                        SOURCE_PROPERTY relationship = sourceProperty(
                                wxS( "preserved_drawing_text_relationship" ),
                                wxString::Format( wxS( "%u,%u" ), groupTextCount, groupLastText ), relationshipSource );
                        relationship.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                        graphic.properties.push_back( std::move( relationship ) );
                    }
                    aModel.graphics.push_back(
                            { graphicSource, { aModel.sheets[aSheetIndex].id, graphicSource }, std::move( graphic ) } );
                }

                if( hasContiguousTextOwnership )
                    pageGraphicRecords.push_back( record );
                continue;
            }

            if( objectClass != 0x06 )
                continue;

            MODEL_SYMBOL_DEFINITION definition;
            definition.id = DEFINITION_ID( definitionIdBase + record );
            definition.source = source;
            definition.name = std::move( name );
            const uint32_t firstPiece = definitionPieceStart[record];
            const uint32_t pieceEnd = record + 1 < definitionPieceStart.size() ? definitionPieceStart[record + 1]
                                                                               : controllers.pools[3].count;

            for( size_t piece = firstPiece; piece < pieceEnd; ++piece )
            {
                const size_t   pieceOffset = pieceBase + piece * SYMBOL_PIECE_BYTES;
                const uint8_t  pieceKind = aCursor.U8At( pieceOffset );
                const uint16_t pointCount = aCursor.U16At( pieceOffset + 2 );
                const uint32_t firstVertex = pieceVertexStart[piece];

                if( firstVertex + pointCount > vertexEnd )
                    throwDecodeError( source, wxS( "symbol piece crosses its definition vertex slice" ) );

                SOURCE_PROVENANCE graphicSource =
                        sourceAt( aSourceName, aModel.version, wxS( "symbol graphic" ), 4, piece, pieceOffset,
                                  SYMBOL_PIECE_BYTES, static_cast<int>( aSheetIndex ) );
                MODEL_GRAPHIC graphic;
                graphic.source = graphicSource;
                graphic.strokeWidth = static_cast<int64_t>( aCursor.U16At( pieceOffset + 4 ) ) * 2;
                graphic.lineStyle = MODEL_LINE_STYLE::SOLID;

                switch( pieceKind )
                {
                case 0: graphic.kind = pointCount == 2 ? MODEL_GRAPHIC_KIND::LINE : MODEL_GRAPHIC_KIND::POLYLINE; break;
                case 1: graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE; break;
                case 2: graphic.kind = MODEL_GRAPHIC_KIND::CIRCLE; break;
                case 4:
                    graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE;
                    graphic.fill = MODEL_FILL_STYLE::FILLED;
                    break;
                default:
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "symbol graphic kind" ), pieceKind, graphicSource,
                                                               aModel.diagnostics );
                    graphic.kind = MODEL_GRAPHIC_KIND::POLYLINE;
                    break;
                }

                for( size_t point = 0; point < pointCount; ++point )
                {
                    const size_t      pointOffset = vertexBase + ( firstVertex + point ) * SYMBOL_VERTEX_BYTES;
                    SOURCE_PROVENANCE pointSource =
                            sourceAt( aSourceName, aModel.version, wxS( "symbol vertex" ), 5, firstVertex + point,
                                      pointOffset, SYMBOL_VERTEX_BYTES, static_cast<int>( aSheetIndex ) );
                    graphic.points.push_back( { decodeLocalCoordinate( aCursor.U16At( pointOffset ) ),
                                                decodeLocalCoordinate( aCursor.U16At( pointOffset + 2 ) ),
                                                pointSource } );
                }

                if( pieceKind == 1 && graphic.points.size() == 5 && graphic.points.front().x == graphic.points.back().x
                    && graphic.points.front().y == graphic.points.back().y )
                {
                    const auto [minX, maxX] = std::ranges::minmax( graphic.points, {}, &SOURCE_POINT::x );
                    const auto [minY, maxY] = std::ranges::minmax( graphic.points, {}, &SOURCE_POINT::y );
                    const bool cornersOnly =
                            std::ranges::all_of( graphic.points,
                                                 [&]( const SOURCE_POINT& aPoint )
                                                 {
                                                     return ( aPoint.x == minX.x || aPoint.x == maxX.x )
                                                            && ( aPoint.y == minY.y || aPoint.y == maxY.y );
                                                 } );

                    if( cornersOnly )
                    {
                        graphic.kind = MODEL_GRAPHIC_KIND::RECTANGLE;
                        graphic.points = { { minX.x, minY.y, graphicSource }, { maxX.x, maxY.y, graphicSource } };
                    }
                }

                const int16_t arcMarker =
                        static_cast<int16_t>( aCursor.U16At( vertexBase + firstVertex * SYMBOL_VERTEX_BYTES + 4 ) );

                if( pieceKind == 0 && arcMarker >= 0 )
                {
                    if( pieceArcRecords[piece].empty() )
                        throwDecodeError( graphicSource, wxS( "symbol arc has no controller-6 record" ) );

                    const size_t arcRecord = pieceArcRecords[piece].front();
                    const size_t arcOffset = arcBase + arcRecord * SYMBOL_ARC_BYTES;
                    graphic.kind = MODEL_GRAPHIC_KIND::ARC;
                    SOURCE_PROVENANCE arcSource =
                            sourceAt( aSourceName, aModel.version, wxS( "symbol arc" ), 6, arcRecord, arcOffset,
                                      SYMBOL_ARC_BYTES, static_cast<int>( aSheetIndex ) );
                    graphic.arcSweepAngle = aCursor.U16At( arcOffset );
                    graphic.arcClockwise = static_cast<int16_t>( aCursor.U16At( arcOffset + 2 ) ) < 0;
                    graphic.arcBoundsStart = { decodeLocalCoordinate( aCursor.U16At( arcOffset + 6 ) ),
                                               decodeLocalCoordinate( aCursor.U16At( arcOffset + 8 ) ), arcSource };
                    graphic.arcBoundsEnd = { decodeLocalCoordinate( aCursor.U16At( arcOffset + 10 ) ),
                                             decodeLocalCoordinate( aCursor.U16At( arcOffset + 12 ) ), arcSource };
                    graphic.arcCenter = { ( graphic.arcBoundsStart.x + graphic.arcBoundsEnd.x ) / 2,
                                          ( graphic.arcBoundsStart.y + graphic.arcBoundsEnd.y ) / 2, arcSource };
                    graphic.properties.push_back( sourceProperty(
                            wxS( "arc_direction" ),
                            graphic.arcClockwise ? wxS( "clockwise" ) : wxS( "counterclockwise" ), arcSource ) );
                    graphic.properties.push_back( sourceProperty(
                            wxS( "arc_marker" ),
                            wxString::Format( wxS( "%d" ), static_cast<int16_t>( aCursor.U16At( arcOffset + 4 ) ) ),
                            arcSource ) );
                }

                definition.graphics.push_back( std::move( graphic ) );
            }

            aModel.definitions.push_back( std::move( definition ) );
            definitionsByRecord[record] = &aModel.definitions.back();
        }

        struct USED_DECAL
        {
            size_t                   record = 0;
            uint32_t                 definitionRecord = 0;
            uint16_t                 terminalStart = 0;
            uint8_t                  terminalCount = 0;
            uint32_t                 fieldStart = 0;
            MODEL_SYMBOL_DEFINITION* definition = nullptr;
        };

        std::vector<USED_DECAL>  usedDecals( controllers.pools[6].count );
        std::vector<USED_DECAL*> semanticDecals;

        for( size_t record = 0; record < usedDecals.size(); ++record )
        {
            const size_t      offset = usedDecalBase + record * USED_DECAL_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "used decal" ), 7, record, offset,
                                                 USED_DECAL_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 40;
            SOURCE_STRING name = decodeFixedString( aBytes, offset, 40, nameSource, aModel.diagnostics );
            USED_DECAL&   decal = usedDecals[record];
            decal.record = record;
            decal.terminalCount = aCursor.U8At( offset + 42 );
            decal.terminalStart = aCursor.U16At( offset + 44 );
            decal.definitionRecord = aCursor.U32At( offset + 48 );
            decal.fieldStart = aCursor.U32At( offset + 52 );

            if( name.text.empty() || decal.definitionRecord == 0xFFFFFFFF )
                continue;

            if( decal.definitionRecord >= definitionsByRecord.size() )
                throwDecodeError( source, wxS( "unresolved symbol definition reference" ) );

            const size_t      definitionOffset = symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES;
            SOURCE_PROVENANCE targetNameSource =
                    sourceAt( aSourceName, aModel.version, wxS( "symbol definition" ), 3, decal.definitionRecord,
                              definitionOffset, 38, static_cast<int>( aSheetIndex ) );
            SOURCE_STRING targetName =
                    decodeFixedString( aBytes, definitionOffset, 38, targetNameSource, aModel.diagnostics );

            if( targetName.text != name.text )
            {
                const uint8_t targetClass = aCursor.U8At( definitionOffset + 0x29 );
                throwDecodeError( source, targetClass == 0x06 ? wxS( "used-decal handle name mismatch" )
                                                              : wxS( "used-decal handle targets wrong object class" ) );
            }

            decal.definition = definitionsByRecord[decal.definitionRecord];

            if( !decal.definition )
                continue;

            const uint16_t embeddedTextCount =
                    aCursor.U16At( symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES + 0x40 );

            if( embeddedTextCount != 0 && decal.fieldStart > textCount && decal.fieldStart < 0x80000000 )
                throwDecodeError( source, wxS( "embedded definition text handle leaves controller 1" ) );

            if( static_cast<uint32_t>( decal.terminalStart ) + decal.terminalCount > controllers.pools[7].count )
                throwDecodeError( source, wxS( "used-decal terminal slice leaves controller 8" ) );

            semanticDecals.push_back( &decal );

            for( size_t pin = 0; pin < decal.terminalCount; ++pin )
            {
                const size_t      terminalRecord = decal.terminalStart + pin;
                const size_t      terminalOffset = terminalBase + terminalRecord * TERMINAL_BYTES;
                SOURCE_PROVENANCE pinSource =
                        sourceAt( aSourceName, aModel.version, wxS( "symbol pin" ), 8, terminalRecord, terminalOffset,
                                  TERMINAL_BYTES, static_cast<int>( aSheetIndex ) );
                const uint16_t       pinDecalHandle = aCursor.U16At( terminalOffset );
                MODEL_PIN_DEFINITION definitionPin;
                definitionPin.id = PIN_ID( pinIdBase + terminalRecord );
                definitionPin.source = pinSource;
                definitionPin.position = { decodeLocalCoordinate( aCursor.U16At( terminalOffset + 2 ) ),
                                           decodeLocalCoordinate( aCursor.U16At( terminalOffset + 4 ) ), pinSource };
                definitionPin.presentation.source = pinSource;
                definitionPin.presentation.height =
                        static_cast<int64_t>( static_cast<int16_t>( aCursor.U16At( terminalOffset + 6 ) ) ) * 2;
                definitionPin.presentation.width =
                        static_cast<int64_t>( static_cast<int16_t>( aCursor.U16At( terminalOffset + 8 ) ) ) * 2;
                definitionPin.presentation.visible = ( aCursor.U16At( terminalOffset + 24 ) & 0x8000 ) == 0;
                definitionPin.namePresentation = definitionPin.presentation;
                definitionPin.numberPresentation.source = pinSource;
                definitionPin.numberPresentation.height =
                        static_cast<int64_t>( static_cast<int16_t>( aCursor.U16At( terminalOffset + 10 ) ) ) * 2;
                definitionPin.numberPresentation.width =
                        static_cast<int64_t>( static_cast<int16_t>( aCursor.U16At( terminalOffset + 12 ) ) ) * 2;
                definitionPin.namePresentation.visible = definitionPin.namePresentation.height != 0
                                                         && ( aCursor.U16At( terminalOffset + 24 ) & 0x8000 ) == 0;
                definitionPin.numberPresentation.visible = definitionPin.numberPresentation.height != 0;
                definitionPin.presentation = definitionPin.namePresentation;
                definitionPin.nameOffset = { decodeLocalCoordinate( aCursor.U16At( terminalOffset + 14 ) ),
                                             decodeLocalCoordinate( aCursor.U16At( terminalOffset + 16 ) ), pinSource };
                definitionPin.numberOffset = { decodeLocalCoordinate( aCursor.U16At( terminalOffset + 18 ) ),
                                               decodeLocalCoordinate( aCursor.U16At( terminalOffset + 20 ) ),
                                               pinSource };
                const uint16_t presentationFlags = aCursor.U16At( terminalOffset + 22 );
                const uint16_t visibilityFlags = aCursor.U16At( terminalOffset + 24 );
                const uint16_t side = presentationFlags & 0x0006;
                definitionPin.side = side / 2;

                if( ( presentationFlags & ~0x2107 ) != 0 )
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "terminal side" ), presentationFlags, pinSource,
                                                               aModel.diagnostics );

                definitionPin.angle = ( presentationFlags & 0x2000 ) != 0 ? 900 : 0;
                definitionPin.nameAngle = ( presentationFlags & 0x2000 ) != 0 ? 900 : 0;
                definitionPin.numberAngle = 0;
                definitionPin.nameJustification = ( presentationFlags & 0x0100 ) != 0 ? 2 : 0;

                switch( visibilityFlags & 0x00E0 )
                {
                case 0x0000: definitionPin.numberJustification = 0; break;
                case 0x0040: definitionPin.numberJustification = 8; break;
                case 0x0060: definitionPin.numberJustification = 9; break;
                case 0x0080: definitionPin.numberJustification = 2; break;
                default:
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "terminal number justification" ),
                                                               visibilityFlags & 0x00E0, pinSource,
                                                               aModel.diagnostics );
                    break;
                }
                definitionPin.nameOffsetAngle = definitionPin.nameAngle;
                definitionPin.numberOffsetAngle = 0;
                definitionPin.nameOffsetJustification = ( presentationFlags & 0x0100 ) != 0 ? 2
                                                        : ( visibilityFlags & 0x0400 ) != 0 ? 1
                                                                                            : 0;
                definitionPin.numberOffsetJustification = 0;
                definitionPin.visibilityFlags = ( visibilityFlags & 0x8000 ) != 0 ? 128 : 0;

                if( pinDecalHandle == 0xFFFF )
                {
                    definitionPin.length = 0;
                }
                else if( pinDecalHandle >= usedDecals.size() )
                    throwDecodeError( pinSource, wxS( "unresolved pin-decal handle" ) );

                if( pinDecalHandle != 0xFFFF )
                {
                    MODEL_SYMBOL_DEFINITION* pinDecal = usedDecals[pinDecalHandle].definition;

                    if( !pinDecal )
                    {
                        const size_t   handleOffset = usedDecalBase + pinDecalHandle * USED_DECAL_BYTES;
                        const uint32_t definitionRecord = aCursor.U32At( handleOffset + 48 );

                        if( definitionRecord >= definitionsByRecord.size() || !definitionsByRecord[definitionRecord] )
                            throwDecodeError( pinSource, wxS( "unresolved pin-decal handle" ) );

                        pinDecal = definitionsByRecord[definitionRecord];
                    }

                    const wxString pinDecalName = pinDecal->name.text;

                    int64_t provenLength = 0;

                    for( const MODEL_GRAPHIC& pinGraphic : pinDecal->graphics )
                    {
                        const bool containsOrigin = std::ranges::any_of( pinGraphic.points,
                                                                         []( const SOURCE_POINT& aPoint )
                                                                         {
                                                                             return aPoint.x == 0 && aPoint.y == 0;
                                                                         } );

                        if( !containsOrigin )
                            continue;

                        for( const SOURCE_POINT& point : pinGraphic.points )
                            provenLength =
                                    std::max( provenLength, std::max( std::abs( point.x ), std::abs( point.y ) ) );
                    }

                    if( provenLength != 0 )
                        definitionPin.length = provenLength * 2;
                    else
                    {
                        SOURCE_PROPERTY lengthProperty =
                                sourceProperty( wxS( "pin_length" ), wxS( "unknown" ), pinSource );
                        lengthProperty.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                        definitionPin.properties.push_back( std::move( lengthProperty ) );
                    }

                    const bool inverted = pinDecalName == wxS( "PINB" ) || pinDecalName == wxS( "PINORB" )
                                          || pinDecalName == wxS( "PCLKB" ) || pinDecalName == wxS( "PINIEB" );
                    const bool clock = pinDecalName == wxS( "PCLK" ) || pinDecalName == wxS( "PCLKB" );
                    definitionPin.graphicStyle = ( inverted ? 1U : 0U ) | ( clock ? 2U : 0U );
                }

                definitionPin.properties.push_back( sourceProperty(
                        wxS( "pin_name_height_half_mils" ),
                        wxString::Format( wxS( "%lld" ), definitionPin.presentation.height ), pinSource ) );
                definitionPin.properties.push_back( sourceProperty(
                        wxS( "pin_number_height_half_mils" ),
                        wxString::Format( wxS( "%lld" ), definitionPin.numberPresentation.height ), pinSource ) );
                definitionPin.properties.push_back( sourceProperty(
                        wxS( "pin_name_width_half_mils" ),
                        wxString::Format( wxS( "%lld" ), definitionPin.namePresentation.width ), pinSource ) );
                definitionPin.properties.push_back( sourceProperty(
                        wxS( "pin_number_width_half_mils" ),
                        wxString::Format( wxS( "%lld" ), definitionPin.numberPresentation.width ), pinSource ) );
                definitionPin.properties.push_back(
                        sourceProperty( wxS( "terminal_side" ), wxString::Format( wxS( "%u" ), side ), pinSource ) );
                decal.definition->pins.push_back( std::move( definitionPin ) );
            }
        }

        std::vector<USED_DECAL*> fieldDecals;

        std::ranges::copy_if( semanticDecals, std::back_inserter( fieldDecals ),
                              [&]( const USED_DECAL* aDecal )
                              {
                                  return aDecal->fieldStart < textCount;
                              } );
        std::ranges::sort( fieldDecals,
                           []( const USED_DECAL* aLeft, const USED_DECAL* aRight )
                           {
                               return aLeft->fieldStart < aRight->fieldStart;
                           } );

        for( size_t i = 1; i < fieldDecals.size(); ++i )
        {
            if( fieldDecals[i - 1]->fieldStart != fieldDecals[i]->fieldStart )
                continue;

            SOURCE_PROVENANCE first =
                    sourceAt( aSourceName, aModel.version, wxS( "used decal" ), 7, fieldDecals[i - 1]->record,
                              usedDecalBase + fieldDecals[i - 1]->record * USED_DECAL_BYTES, USED_DECAL_BYTES,
                              static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE duplicate =
                    sourceAt( aSourceName, aModel.version, wxS( "used decal" ), 7, fieldDecals[i]->record,
                              usedDecalBase + fieldDecals[i]->record * USED_DECAL_BYTES, USED_DECAL_BYTES,
                              static_cast<int>( aSheetIndex ) );
            throwDecodeError(
                    duplicate,
                    wxString::Format( wxS( "duplicate field ID; first at v0x%04X %s controller %d record %llu "
                                           "sheet %d offset 0x%llX" ),
                                      first.version, first.objectClass, first.controller,
                                      static_cast<unsigned long long>( first.recordIndex ), first.sheet,
                                      static_cast<unsigned long long>( first.absoluteOffset ) ) );
        }

        auto decodeTextRecord =
                [&]( size_t aRecord, bool aEmbedded, bool aPageText, MODEL_SYMBOL_DEFINITION& aDefinition )
        {
            if( aRecord >= textCount )
                throwDecodeError( aDefinition.source, wxS( "definition field handle leaves controller 1" ) );

            const size_t      offset = textBase + aRecord * TEXT_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt(
                    aSourceName, aModel.version, aEmbedded ? wxS( "embedded symbol text" ) : wxS( "definition field" ),
                    1, aRecord, offset, TEXT_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint32_t stringOffset = aCursor.U32At( offset + 8 );
            const uint16_t stringBytes = aCursor.U16At( offset + 20 );

            if( stringBytes == 0 || stringOffset > textHeapBytes || stringBytes > textHeapBytes - stringOffset
                || aBytes[textHeapBase + stringOffset + stringBytes - 1] != 0 )
            {
                throwDecodeError( source, wxS( "definition field string leaves controller 2" ) );
            }

            SOURCE_PROVENANCE stringSource =
                    sourceAt( aSourceName, aModel.version, source.objectClass + wxS( " string" ), 2, aRecord,
                              textHeapBase + stringOffset, stringBytes - 1, static_cast<int>( aSheetIndex ) );
            SOURCE_STRING string = PADS_SCH_BINARY_PARSER::DecodeString(
                    { aBytes.begin() + stringSource.absoluteOffset,
                      aBytes.begin() + stringSource.absoluteOffset + stringSource.length },
                    DEFAULT_CODE_PAGE, stringSource, aModel.diagnostics );
            if( aEmbedded )
            {
                MODEL_GRAPHIC graphic;
                graphic.source = source;
                graphic.kind = MODEL_GRAPHIC_KIND::TEXT;
                graphic.text = std::move( string );
                graphic.points.push_back( { decodeLocalCoordinate( aCursor.U16At( offset + 12 ) ),
                                            decodeLocalCoordinate( aCursor.U16At( offset + 14 ) ), source } );
                graphic.presentation.source = source;
                graphic.presentation.height = static_cast<int64_t>( aCursor.U16At( offset + 22 ) ) * 2;
                graphic.presentation.width = static_cast<int64_t>( aCursor.U8At( offset + 30 ) ) * 2;
                graphic.presentation.properties.push_back(
                        sourceProperty( wxS( "display_flags" ),
                                        wxString::Format( wxS( "%u" ), aCursor.U8At( offset + 31 ) ), source ) );
                graphic.presentation.horizontalJustification =
                        horizontalJustification( aCursor.U16At( offset + 18 ), source, aModel.diagnostics );
                graphic.presentation.verticalJustification = verticalJustification( aCursor.U16At( offset + 18 ) );
                SOURCE_PROVENANCE fontSource = source;
                fontSource.absoluteOffset += 28;
                fontSource.length = 2;
                const int16_t relationship = static_cast<int16_t>( aCursor.U16At( offset + 28 ) );
                graphic.presentation.font = decodedDefinitionFont( aPageText ? -1 : relationship, fontSource );
                graphic.presentation.properties.push_back(
                        sourceProperty( aPageText ? wxS( "successor_ordinal" ) : wxS( "font_handle" ),
                                        wxString::Format( wxS( "%d" ), relationship ), fontSource ) );
                graphic.angle = NormalizeAngle( aCursor.U16At( offset + 16 ) );
                aDefinition.graphics.push_back( std::move( graphic ) );
            }
            else
            {
                MODEL_FIELD field;
                field.source = source;
                field.name = std::move( string );
                field.position = { decodeLocalCoordinate( aCursor.U16At( offset + 12 ) ),
                                   decodeLocalCoordinate( aCursor.U16At( offset + 14 ) ), source };
                field.angle = NormalizeAngle( aCursor.U16At( offset + 16 ) );
                field.presentation.source = source;
                field.presentation.height = static_cast<int64_t>( aCursor.U16At( offset + 22 ) ) * 2;
                field.presentation.width = static_cast<int64_t>( aCursor.U8At( offset + 30 ) ) * 2;
                field.presentation.properties.push_back(
                        sourceProperty( wxS( "display_flags" ),
                                        wxString::Format( wxS( "%u" ), aCursor.U8At( offset + 31 ) ), source ) );
                field.presentation.horizontalJustification =
                        horizontalJustification( aCursor.U16At( offset + 18 ), source, aModel.diagnostics );
                field.presentation.verticalJustification = verticalJustification( aCursor.U16At( offset + 18 ) );
                SOURCE_PROVENANCE fontSource = source;
                fontSource.absoluteOffset += 28;
                fontSource.length = 2;
                const int16_t fontHandle = static_cast<int16_t>( aCursor.U16At( offset + 28 ) );
                field.presentation.font = decodedDefinitionFont( fontHandle, fontSource );
                field.presentation.properties.push_back( sourceProperty(
                        wxS( "font_handle" ), wxString::Format( wxS( "%d" ), fontHandle ), fontSource ) );
                aDefinition.fields.push_back( std::move( field ) );
            }
        };

        for( size_t record : pageGraphicRecords )
        {
            const size_t      offset = symbolBase + record * SYMBOL_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "page graphic group" ), 3, record,
                                                 offset, SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 38;
            SOURCE_STRING  groupName = decodeFixedString( aBytes, offset, 38, nameSource, aModel.diagnostics );
            const uint16_t textCountForGroup = aCursor.U16At( offset + 64 );
            const uint16_t lastTextRecord = aCursor.U16At( offset + 66 );

            if( textCountForGroup != 0
                && ( lastTextRecord >= textCount || textCountForGroup > static_cast<size_t>( lastTextRecord ) + 1 ) )
                throwDecodeError( source, wxS( "page-text ownership leaves controller 1" ) );

            MODEL_SYMBOL_DEFINITION textOwner;
            textOwner.source = source;

            if( textCountForGroup != 0 )
            {
                for( size_t textRecord = static_cast<size_t>( lastTextRecord ) + 1 - textCountForGroup;
                     textRecord <= lastTextRecord; ++textRecord )
                {
                    decodeTextRecord( textRecord, true, true, textOwner );
                }
            }

            if( textOwner.graphics.size() != textCountForGroup )
                throwDecodeError( source, wxS( "page-text records do not exactly match declared count" ) );

            for( MODEL_GRAPHIC& graphic : textOwner.graphics )
            {
                graphic.source.objectClass = wxS( "page text" );
                graphic.properties.push_back( sourceProperty( wxS( "page_graphic_group" ), groupName.text, source ) );
                aModel.graphics.push_back(
                        { graphic.source, { aModel.sheets[aSheetIndex].id, graphic.source }, std::move( graphic ) } );
            }
        }

        for( size_t i = 0; i < fieldDecals.size(); ++i )
        {
            USED_DECAL&    decal = *fieldDecals[i];
            const size_t   definitionOffset = symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES;
            const uint16_t embeddedCount = aCursor.U16At( definitionOffset + 0x40 );
            const uint32_t fieldEnd = i + 1 < fieldDecals.size() ? fieldDecals[i + 1]->fieldStart : textCount;

            if( decal.fieldStart > fieldEnd || embeddedCount > decal.fieldStart )
                throwDecodeError( decal.definition->source, wxS( "definition field slice is not monotone" ) );

            if( decal.definition->fields.empty() )
            {
                for( size_t standard = 0; standard < 2; ++standard )
                {
                    const size_t      usedOffset = usedDecalBase + decal.record * USED_DECAL_BYTES;
                    SOURCE_PROVENANCE fieldSource =
                            sourceAt( aSourceName, aModel.version, wxS( "standard definition field" ), 7, decal.record,
                                      usedOffset + 60 + standard * 8, 8, static_cast<int>( aSheetIndex ) );
                    MODEL_FIELD field;
                    field.source = fieldSource;
                    field.name = { {},
                                   standard == 0 ? wxS( "REF-DES" ) : wxS( "PART-TYPE" ),
                                   STRING_ENCODING_STATUS::CODE_PAGE,
                                   fieldSource };
                    field.position = { decodeLocalCoordinate( aCursor.U16At( usedOffset + 60 + standard * 8 ) ),
                                       decodeLocalCoordinate( aCursor.U16At( usedOffset + 62 + standard * 8 ) ),
                                       fieldSource };
                    field.angle = NormalizeAngle( aCursor.U16At( usedOffset + 64 + standard * 8 ) );
                    field.presentation.source = fieldSource;
                    field.presentation.height =
                            static_cast<int64_t>( aCursor.U16At( usedOffset + 88 + standard * 2 ) ) * 2;
                    field.presentation.width = static_cast<int64_t>( aCursor.U8At( usedOffset + 96 + standard ) ) * 2;
                    SOURCE_PROVENANCE fontSource = fieldSource;
                    fontSource.absoluteOffset = usedOffset + 100 + standard * 2;
                    fontSource.length = 2;
                    const int16_t fontHandle = static_cast<int16_t>( aCursor.U16At( fontSource.absoluteOffset ) );
                    field.presentation.font = decodedDefinitionFont( fontHandle, fontSource );
                    field.presentation.properties.push_back( sourceProperty(
                            wxS( "font_handle" ), wxString::Format( wxS( "%d" ), fontHandle ), fontSource ) );
                    const uint16_t justification = aCursor.U16At( usedOffset + 66 + standard * 8 );
                    field.presentation.horizontalJustification =
                            horizontalJustification( justification, fieldSource, aModel.diagnostics );
                    field.presentation.verticalJustification = verticalJustification( justification );
                    decal.definition->fields.push_back( std::move( field ) );
                }
            }

            for( size_t record = decal.fieldStart - embeddedCount; record < decal.fieldStart; ++record )
                decodeTextRecord( record, true, false, *decal.definition );

            for( size_t record = decal.fieldStart; record < fieldEnd; ++record )
                decodeTextRecord( record, false, false, *decal.definition );
        }

        size_t signalPinCursor = 0;

        for( size_t record = 0; record < controllers.pools[8].count; ++record )
        {
            const size_t      offset = partBase + record * PART_TYPE_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "part type" ), 9, record, offset,
                                                 PART_TYPE_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 44;
            MODEL_PART_TYPE part;
            part.id = PART_TYPE_ID( partIdBase + record );
            part.source = source;
            part.name = decodeFixedString( aBytes, offset, 44, nameSource, aModel.diagnostics );
            const uint32_t gateStart = aCursor.U32At( offset + 44 );
            const uint32_t pinStart = aCursor.U32At( offset + 48 );
            const uint32_t gateEnd = record + 1 < controllers.pools[8].count
                                             ? aCursor.U32At( offset + PART_TYPE_BYTES + 44 )
                                             : controllers.pools[9].count;
            const uint32_t pinEnd = record + 1 < controllers.pools[8].count
                                            ? aCursor.U32At( offset + PART_TYPE_BYTES + 48 )
                                            : controllers.pools[10].count;

            if( gateStart > gateEnd || gateEnd > controllers.pools[9].count || pinStart > pinEnd
                || pinEnd > controllers.pools[10].count )
            {
                throwDecodeError( source, wxS( "part-type gate or pin slice leaves its controller" ) );
            }

            if( aCursor.U16At( offset + 68 ) != gateEnd - gateStart )
                throwDecodeError( source, wxS( "stored gate count does not match controller-10 slice" ) );

            size_t   partPinCursor = pinStart;
            uint32_t unitCursor = 0;

            auto decodePartPin = [&]( MODEL_PIN_DEFINITION& aDefinitionPin, size_t aPinRecord, MODEL_GATE& aGate )
            {
                const size_t      pinOffset = pinBase + aPinRecord * PIN_BYTES;
                SOURCE_PROVENANCE pinSource = sourceAt( aSourceName, aModel.version, wxS( "part pin" ), 11, aPinRecord,
                                                        pinOffset, PIN_BYTES, static_cast<int>( aSheetIndex ) );
                SOURCE_PROVENANCE numberSource = pinSource;
                numberSource.absoluteOffset += 4;
                numberSource.length = 16;
                aDefinitionPin.number =
                        decodeFixedString( aBytes, pinOffset + 4, 16, numberSource, aModel.diagnostics );
                const uint32_t nameOffset = aCursor.U32At( pinOffset );

                if( nameOffset != 0xFFFFFFFF )
                {
                    if( nameOffset >= pinNameBytes )
                        throwDecodeError( pinSource, wxS( "pin-name offset leaves controller 14" ) );

                    SOURCE_PROVENANCE pinNameSource = sourceAt(
                            aSourceName, aModel.version, wxS( "pin name" ), 14, aPinRecord, pinNameBase + nameOffset,
                            pinNameBytes - nameOffset, static_cast<int>( aSheetIndex ) );
                    aDefinitionPin.name =
                            decodeFixedString( aBytes, pinNameBase + nameOffset, pinNameBytes - nameOffset,
                                               pinNameSource, aModel.diagnostics );
                }

                aDefinitionPin.electricalType =
                        pinElectricalType( aCursor.U8At( pinOffset + 21 ), pinSource, aModel.diagnostics );
                aDefinitionPin.properties.push_back(
                        sourceProperty( wxS( "swap_group" ),
                                        wxString::Format( wxS( "%u" ), aCursor.U8At( pinOffset + 20 ) ), pinSource ) );
                aGate.pins.push_back( { aDefinitionPin.id, pinSource } );
            };

            auto decodeConnectorPin = [&]( size_t aPinRecord )
            {
                const size_t      pinOffset = pinBase + aPinRecord * PIN_BYTES;
                SOURCE_PROVENANCE pinSource =
                        sourceAt( aSourceName, aModel.version, wxS( "connector logical pin" ), 11, aPinRecord,
                                  pinOffset, PIN_BYTES, static_cast<int>( aSheetIndex ) );
                SOURCE_PROVENANCE numberSource = pinSource;
                numberSource.absoluteOffset += 4;
                numberSource.length = 16;
                MODEL_CONNECTOR_PIN pin;
                pin.source = pinSource;
                pin.number = decodeFixedString( aBytes, pinOffset + 4, 16, numberSource, aModel.diagnostics );
                const uint32_t nameOffset = aCursor.U32At( pinOffset );

                if( nameOffset != 0xFFFFFFFF )
                {
                    if( nameOffset >= pinNameBytes )
                        throwDecodeError( pinSource, wxS( "pin-name offset leaves controller 14" ) );

                    SOURCE_PROVENANCE pinNameSource = sourceAt(
                            aSourceName, aModel.version, wxS( "connector pin name" ), 14, aPinRecord,
                            pinNameBase + nameOffset, pinNameBytes - nameOffset, static_cast<int>( aSheetIndex ) );
                    pin.name = decodeFixedString( aBytes, pinNameBase + nameOffset, pinNameBytes - nameOffset,
                                                  pinNameSource, aModel.diagnostics );
                }

                pin.swapGroup = aCursor.U8At( pinOffset + 20 );
                pin.electricalType = pinElectricalType( aCursor.U8At( pinOffset + 21 ), pinSource, aModel.diagnostics );
                pin.flags = aCursor.U16At( pinOffset + 22 );
                return pin;
            };

            for( size_t gateRecord = gateStart; gateRecord < gateEnd; ++gateRecord )
            {
                const size_t      gateOffset = gateBase + gateRecord * GATE_BYTES;
                SOURCE_PROVENANCE gateSource = sourceAt( aSourceName, aModel.version, wxS( "gate" ), 10, gateRecord,
                                                         gateOffset, GATE_BYTES, static_cast<int>( aSheetIndex ) );
                MODEL_GATE        gate;
                gate.id = GATE_ID( gateIdBase + gateRecord );
                gate.source = gateSource;
                const uint16_t pinCount = aCursor.U16At( gateOffset + 8 );
                const uint16_t swapGroup = aCursor.U16At( gateOffset + 10 );
                const uint16_t primaryHandle = aCursor.U16At( gateOffset );

                if( pinCount == 0 )
                    continue;

                gate.unit = ++unitCursor;

                if( primaryHandle == 0xFFFF || primaryHandle >= usedDecals.size()
                    || !usedDecals[primaryHandle].definition )
                {
                    bool pinDecalGroup = primaryHandle == 0xFFFF && gateRecord + 1 < gateEnd;

                    for( size_t member = gateRecord + 1; pinDecalGroup && member < gateEnd; ++member )
                    {
                        const size_t   memberOffset = gateBase + member * GATE_BYTES;
                        const uint16_t memberHandle = aCursor.U16At( memberOffset );
                        pinDecalGroup = aCursor.U16At( memberOffset + 8 ) == 0 && memberHandle < usedDecals.size()
                                        && usedDecals[memberHandle].definition;
                    }

                    if( !pinDecalGroup || partPinCursor + pinCount > pinEnd )
                        throwDecodeError( gateSource, wxS( "unresolved symbol definition reference" ) );

                    part.properties.push_back( sourceProperty(
                            wxString::Format( wxS( "pin_decal_group_%llu" ),
                                              static_cast<unsigned long long>( gateRecord - gateStart + 1 ) ),
                            wxString::Format( wxS( "%u" ), pinCount ), gateSource ) );

                    for( size_t member = gateRecord + 1; member < gateEnd; ++member )
                    {
                        const size_t   memberOffset = gateBase + member * GATE_BYTES;
                        const uint16_t memberHandle = aCursor.U16At( memberOffset );

                        if( aCursor.U16At( memberOffset + 8 ) != 0 )
                            break;

                        SOURCE_PROVENANCE memberSource =
                                sourceAt( aSourceName, aModel.version, wxS( "pin-decal group member" ), 10, member,
                                          memberOffset, GATE_BYTES, static_cast<int>( aSheetIndex ) );

                        if( memberHandle >= usedDecals.size() || !usedDecals[memberHandle].definition )
                            throwDecodeError( memberSource, wxS( "unresolved pin-decal group member" ) );

                        gate.decalGroupMembers.push_back( { usedDecals[memberHandle].definition->id, memberSource } );
                    }

                    for( size_t pin = 0; pin < pinCount; ++pin, ++partPinCursor )
                        gate.connectorPins.push_back( decodeConnectorPin( partPinCursor ) );

                    part.gates.push_back( std::move( gate ) );
                    continue;
                }

                MODEL_SYMBOL_DEFINITION* definition = usedDecals[primaryHandle].definition;
                gate.definition = { definition->id, gateSource };
                gate.properties.push_back(
                        sourceProperty( wxS( "swap_group" ), wxString::Format( wxS( "%u" ), swapGroup ), gateSource ) );

                for( size_t alternate = 1; alternate < 4; ++alternate )
                {
                    const uint16_t handle = aCursor.U16At( gateOffset + alternate * 2 );

                    if( handle == 0xFFFF )
                        continue;

                    if( handle >= usedDecals.size() || !usedDecals[handle].definition )
                        throwDecodeError( gateSource, wxS( "unresolved alternate symbol definition reference" ) );

                    gate.alternateDefinitions.push_back( { usedDecals[handle].definition->id, gateSource } );
                }

                if( partPinCursor + pinCount > pinEnd || pinCount > definition->pins.size() )
                    throwDecodeError( gateSource, wxS( "gate pin slice leaves part type or symbol definition" ) );

                for( size_t pin = 0; pin < pinCount; ++pin, ++partPinCursor )
                {
                    MODEL_PIN_DEFINITION& definitionPin = definition->pins[pin];
                    decodePartPin( definitionPin, partPinCursor, gate );
                }

                part.gates.push_back( std::move( gate ) );
            }

            if( partPinCursor != pinEnd )
                throwDecodeError( source, wxS( "part-type gates do not consume its pin slice" ) );

            if( !part.gates.empty() && part.gates.front().definition.id.IsValid() )
            {
                auto defaultDefinition =
                        std::ranges::find_if( aModel.definitions,
                                              [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                              {
                                                  return aDefinition.id == part.gates.front().definition.id;
                                              } );

                if( defaultDefinition == aModel.definitions.end() )
                    throwDecodeError( source, wxS( "unresolved part-type default definition" ) );

                part.fields = defaultDefinition->fields;
            }

            const uint16_t signalPinCount = aCursor.U16At( offset + 70 );

            if( signalPinCursor + signalPinCount > controllers.pools[11].count )
                throwDecodeError( source, wxS( "part-type signal-pin slice leaves controller 12" ) );

            for( size_t signal = 0; signal < signalPinCount; ++signal, ++signalPinCursor )
            {
                const size_t      signalOffset = signalPinBase + signalPinCursor * SIGNAL_PIN_BYTES;
                SOURCE_PROVENANCE signalSource =
                        sourceAt( aSourceName, aModel.version, wxS( "signal pin" ), 12, signalPinCursor, signalOffset,
                                  SIGNAL_PIN_BYTES, static_cast<int>( aSheetIndex ) );
                SOURCE_PROVENANCE numberSource = signalSource;
                numberSource.length = 16;
                SOURCE_PROVENANCE nameSource2 = signalSource;
                nameSource2.absoluteOffset += 16;
                nameSource2.length = 48;
                SOURCE_STRING number = decodeFixedString( aBytes, signalOffset, 16, numberSource, aModel.diagnostics );
                SOURCE_STRING signalName =
                        decodeFixedString( aBytes, signalOffset + 16, 48, nameSource2, aModel.diagnostics );
                part.signalPins.push_back( { signalSource, number, signalName } );
                part.properties.push_back(
                        sourceProperty( wxS( "signal_pin_" ) + number.text, signalName.text, signalSource ) );
            }

            aModel.partTypes.push_back( std::move( part ) );
        }

        if( signalPinCursor != controllers.pools[11].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "signal pin" ), 12, signalPinCursor, signalPinBase,
                              controllers.pools[11].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned signal-pin record" ) );
        }
    }


    void decodePlacements( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                           const SCH_SDB_BLOCK& aBlock, size_t aSheetIndex, const wxString& aSourceName,
                           const PLACEMENT_GLOBALS& aGlobals, PADS_SCH_MODEL& aModel )
    {
        const PLACEMENT_LAYOUT& layout = placementLayout( aModel.version );

        if( !layout.decoded )
            throwDecodeError( aModel.source, wxS( "placement decoder selected for raw-preserved version" ) );

        const SHEET_CONTROLLERS controllers = sheetControllers( aCursor, aBlock );
        requireFixedController( controllers, 15, layout.placementBytes, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 16, layout.placedPinBytes, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 17, layout.fieldBytes, aSourceName, aModel.version, aSheetIndex );

        const size_t placementBase = controllers.offsets[14];
        const size_t placedPinBase = controllers.offsets[15];
        const size_t fieldBase = controllers.offsets[16];
        uint32_t     expectedPinStart = 0;
        uint32_t     fieldCursor = 0;

        auto font = [&]( int16_t aHandle, const SOURCE_PROVENANCE& aHandleSource,
                         MODEL_TEXT_PRESENTATION& aPresentation, bool aStrictHandle )
        {
            if( aHandle == -1 || aHandle == -4 )
            {
                aPresentation.font = decodedDefinitionFont( aHandle, aHandleSource );
                return;
            }

            if( aHandle < 0 || static_cast<uint32_t>( aHandle ) >= aGlobals.fontCount )
            {
                if( aStrictHandle )
                    throwDecodeError( aHandleSource, wxS( "placement font handle leaves outer controller 19" ) );

                SOURCE_PROPERTY property =
                        sourceProperty( wxS( "inline_font_payload" ),
                                        wxString::Format( wxS( "%u" ), uint16_t( aHandle ) ), aHandleSource );
                property.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                aPresentation.properties.push_back( std::move( property ) );
                aModel.diagnostics.push_back( { RPT_SEVERITY_WARNING, aHandleSource,
                                                wxS( "unsupported inline placement font payload preserved" ) } );
                return;
            }

            const size_t      fontOffset = aGlobals.fontBase + static_cast<size_t>( aHandle ) * FONT_RECORD_BYTES;
            SOURCE_PROVENANCE fontSource = sourceAt( aSourceName, aModel.version, wxS( "placement font" ), 19, aHandle,
                                                     fontOffset, FONT_RECORD_BYTES, -1 );
            const uint32_t    style = aCursor.U32At( fontOffset );

            SOURCE_PROVENANCE nameSource = fontSource;
            nameSource.absoluteOffset += 4;
            nameSource.length = 32;
            SOURCE_STRING name = decodeFixedString( aBytes, fontOffset + 4, 32, nameSource, aModel.diagnostics );
            aPresentation.bold = ( style & 2 ) != 0;
            aPresentation.italic = ( style & 1 ) != 0;
            aPresentation.font = name;

            if( aPresentation.bold )
                aPresentation.font.text.Prepend( wxS( "Bold " ) );

            if( aPresentation.italic )
                aPresentation.font.text.Prepend( wxS( "Italic " ) );

            aPresentation.properties.push_back(
                    sourceProperty( wxS( "font_handle" ), wxString::Format( wxS( "%d" ), aHandle ), fontSource ) );

            if( ( style & ~uint32_t{ 3 } ) != 0 )
            {
                SOURCE_PROPERTY property = sourceProperty( wxS( "unsupported_font_style_flags" ),
                                                           wxString::Format( wxS( "%u" ), style & ~3U ), fontSource );
                property.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                aPresentation.properties.push_back( std::move( property ) );
                aModel.diagnostics.push_back( { RPT_SEVERITY_WARNING, fontSource,
                                                wxS( "unsupported placement font style flags preserved" ) } );
            }
        };

        auto attributeString = [&]( uint32_t aOffsetIndex, bool aRequireValue )
        {
            if( aOffsetIndex >= aGlobals.attributeOffsetCount )
            {
                SOURCE_PROVENANCE source =
                        sourceAt( aSourceName, aModel.version, wxS( "attribute offset" ), 7, aOffsetIndex,
                                  aGlobals.attributeOffsetBase, ATTRIBUTE_OFFSET_BYTES, -1 );
                throwDecodeError( source, wxS( "attribute offset index leaves outer controller 7" ) );
            }

            const size_t      offsetRecord = aGlobals.attributeOffsetBase + aOffsetIndex * ATTRIBUTE_OFFSET_BYTES;
            const uint32_t    heapOffset = aCursor.U32At( offsetRecord );
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "placement attribute" ), 2,
                                                 aOffsetIndex, aGlobals.attributeHeapBase + heapOffset, 0, -1 );

            if( heapOffset >= aGlobals.attributeHeapBytes )
                throwDecodeError( source, wxS( "attribute string offset leaves outer controller 2" ) );

            size_t       end = source.absoluteOffset;
            const size_t heapEnd = aGlobals.attributeHeapBase + aGlobals.attributeHeapBytes;

            while( end < heapEnd && aBytes[end] != 0 )
                ++end;

            if( end == heapEnd )
                throwDecodeError( source, wxS( "placement attribute is not NUL terminated" ) );

            source.length = end - source.absoluteOffset;
            size_t separator = source.absoluteOffset;

            while( separator < end && aBytes[separator] != 1 )
                ++separator;

            if( aRequireValue && separator == end )
                throwDecodeError( source, wxS( "placement attribute lacks key/value separator" ) );

            SOURCE_STRING name = PADS_SCH_BINARY_PARSER::DecodeString(
                    { aBytes.begin() + source.absoluteOffset, aBytes.begin() + separator }, DEFAULT_CODE_PAGE, source,
                    aModel.diagnostics );
            SOURCE_STRING value;

            if( separator != end )
            {
                SOURCE_PROVENANCE valueSource = source;
                valueSource.absoluteOffset = separator + 1;
                valueSource.length = end - separator - 1;
                value = PADS_SCH_BINARY_PARSER::DecodeString(
                        { aBytes.begin() + valueSource.absoluteOffset, aBytes.begin() + end }, DEFAULT_CODE_PAGE,
                        valueSource, aModel.diagnostics );
            }

            return std::pair<SOURCE_STRING, SOURCE_STRING>{ std::move( name ), std::move( value ) };
        };

        for( size_t record = 0; record < controllers.pools[14].count; ++record )
        {
            const size_t      offset = placementBase + record * layout.placementBytes;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "placement" ), 15, record, offset,
                                                 layout.placementBytes, static_cast<int>( aSheetIndex ) );
            const uint32_t    componentIdentity = aCursor.U32At( offset + layout.componentIdentity );

            const uint16_t partHandle = aCursor.U16At( offset + layout.partType );
            auto           part = std::ranges::find_if( aModel.partTypes,
                                                        [&]( const MODEL_PART_TYPE& aPart )
                                                        {
                                                  return aPart.source.sheet == static_cast<int>( aSheetIndex )
                                                         && aPart.source.recordIndex == partHandle;
                                              } );

            if( part == aModel.partTypes.end() )
            {
                const bool definitionClass =
                        std::ranges::any_of( aModel.definitions,
                                             [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                             {
                                                 return aDefinition.source.sheet == static_cast<int>( aSheetIndex )
                                                        && aDefinition.source.recordIndex == partHandle;
                                             } );
                throwDecodeError( source, definitionClass
                                                  ? wxS( "placement part-type handle targets definition object class" )
                                                  : wxS( "unresolved placement part-type reference" ) );
            }

            const uint32_t groupHandle = aCursor.U32At( offset + layout.componentGroup );

            if( groupHandle >= aGlobals.groupCount )
                throwDecodeError( source, wxS( "placement component-group handle leaves outer controller 6" ) );

            const size_t   groupOffset = aGlobals.groupBase + groupHandle * PLACEMENT_GROUP_BYTES;
            const uint32_t attributeStart = aCursor.U32At( groupOffset );
            const uint16_t attributeCount = aCursor.U16At( groupOffset + 20 );

            if( attributeCount < 2 || attributeStart > aGlobals.attributeOffsetCount
                || attributeCount > aGlobals.attributeOffsetCount - attributeStart )
            {
                throwDecodeError( source, wxS( "placement component-group attribute slice leaves controller 7" ) );
            }

            auto [groupPartName, unusedGroupPartValue] = attributeString( attributeStart + 1, false );

            if( groupPartName.text != part->name.text )
                throwDecodeError( source, wxS( "placement component-group targets wrong part-type object class" ) );

            const uint16_t decalHandle = aCursor.U16At( offset + layout.decal );

            if( decalHandle >= controllers.pools[6].count )
                throwDecodeError( source, wxS( "unresolved placement decal reference" ) );

            const size_t   decalOffset = controllers.offsets[6] + decalHandle * USED_DECAL_BYTES;
            const uint32_t definitionHandle = aCursor.U32At( decalOffset + 48 );
            auto           definition =
                    std::ranges::find_if( aModel.definitions,
                                          [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                          {
                                              return aDefinition.source.sheet == static_cast<int>( aSheetIndex )
                                                     && aDefinition.source.recordIndex == definitionHandle;
                                          } );

            if( definition == aModel.definitions.end() )
                throwDecodeError( source, wxS( "placement decal targets wrong definition object class" ) );

            const uint16_t unitIndex = aCursor.U16At( offset + layout.gate );

            const MODEL_GATE* gate = nullptr;

            if( unitIndex < part->gates.size() )
                gate = &part->gates[unitIndex];
            else if( part->gates.size() == 1 && !part->gates.front().decalGroupMembers.empty() )
                gate = &part->gates.front();
            else if( part->gates.empty() && unitIndex == 0 && definition->pins.empty() )
                gate = nullptr;
            else
            {
                const bool definitionClass =
                        std::ranges::any_of( aModel.definitions,
                                             [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                             {
                                                 return aDefinition.source.sheet == static_cast<int>( aSheetIndex )
                                                        && aDefinition.source.recordIndex == unitIndex;
                                             } );
                throwDecodeError( source, definitionClass
                                                  ? wxS( "placement gate handle targets definition object class" )
                                                  : wxS( "unresolved placement gate reference" ) );
            }

            auto gateHasDefinition = [&]( const DEFINITION_REFERENCE& aReference )
            {
                return aReference.id == definition->id;
            };

            if( gate && !gateHasDefinition( gate->definition )
                && std::ranges::none_of( gate->alternateDefinitions, gateHasDefinition )
                && std::ranges::none_of( gate->decalGroupMembers, gateHasDefinition ) )
            {
                throwDecodeError( source, wxS( "placement decal and gate reference target different object classes" ) );
            }

            const uint32_t pinStart = aCursor.U32At( offset + layout.pinStart );
            const uint16_t pinCount = aCursor.U16At( offset + layout.pinCount );

            if( pinStart != expectedPinStart || pinStart > controllers.pools[15].count
                || pinCount > controllers.pools[15].count - pinStart || pinCount != definition->pins.size() )
            {
                throwDecodeError( source, wxS( "placement pin ownership does not match controller 16" ) );
            }

            MODEL_PLACEMENT placement;

            if( aSheetIndex >= 0x0FFF || record >= 0x100000 )
                throwDecodeError( source, wxS( "placement identity exceeds sheet/controller namespace" ) );

            placement.id = PLACEMENT_ID( static_cast<uint32_t>( aSheetIndex * 0x100000 + record + 1 ) );
            placement.source = source;
            placement.sheet = { aModel.sheets[aSheetIndex].id, source };
            placement.partType = { part->id, source };
            if( gate )
                placement.gate = GATE_REFERENCE{ gate->id, source };
            placement.definition = { definition->id, source };
            placement.unit = unitIndex + 1;
            placement.position = { decodeCoordinate( aCursor.U16At( offset + layout.x ) ),
                                   decodeCoordinate( aCursor.U16At( offset + layout.y ) ), source };
            const uint16_t rawAngle = aCursor.U16At( offset + layout.angle );
            placement.angle = NormalizeAngle( rawAngle );
            placement.mirrorFlags = aCursor.U16At( offset + layout.mirror );

            auto recordTransformEnum = [&]( size_t aFieldOffset )
            {
                SOURCE_PROVENANCE enumSource = source;
                enumSource.absoluteOffset += aFieldOffset;
                enumSource.length = 2;
                aModel.diagnostics.push_back(
                        { RPT_SEVERITY_WARNING, enumSource, wxS( "unknown placement transform enum preserved" ) } );
            };

            const bool knownAngle = rawAngle == 0 || rawAngle == 900 || rawAngle == 1800 || rawAngle == 2700;

            if( !knownAngle )
                recordTransformEnum( layout.angle );

            const bool knownMirror =
                    placement.mirrorFlags == 0 || placement.mirrorFlags == 2 || placement.mirrorFlags == 3;

            if( !knownMirror )
                recordTransformEnum( layout.mirror );

            placement.mirrored = placement.mirrorFlags != 0;
            SOURCE_PROVENANCE referenceSource = source;
            referenceSource.absoluteOffset += layout.reference;
            referenceSource.length = 40;
            placement.reference =
                    decodeFixedString( aBytes, offset + layout.reference, 40, referenceSource, aModel.diagnostics );

            for( size_t pin = 0; pin < pinCount; ++pin )
            {
                const size_t      pinOffset = placedPinBase + ( pinStart + pin ) * layout.placedPinBytes;
                SOURCE_PROVENANCE pinSource =
                        sourceAt( aSourceName, aModel.version, wxS( "placed pin" ), 16, pinStart + pin, pinOffset,
                                  layout.placedPinBytes, static_cast<int>( aSheetIndex ) );
                const uint16_t pinOrdinal = aCursor.U16At( pinOffset + layout.placedPinOrdinal );

                if( pinOrdinal >= definition->pins.size() || pinOrdinal != pin )
                    throwDecodeError( pinSource, wxS( "placed-pin handle leaves placement definition" ) );

                placement.pins.push_back( { definition->pins[pinOrdinal].id, pinSource } );
            }

            expectedPinStart += pinCount;

            auto addInlineField = [&]( const wxString& aName, const SOURCE_STRING& aValue, size_t aXOffset,
                                       size_t aAngleOffset, size_t aFontOffset, size_t aHeightOffset,
                                       size_t aWidthOffset )
            {
                SOURCE_PROVENANCE fieldSource = source;
                fieldSource.objectClass = wxS( "placement field" );
                fieldSource.absoluteOffset += aXOffset;
                fieldSource.length = 8;
                MODEL_FIELD field;
                field.source = fieldSource;
                field.name.text = aName;
                field.name.source = fieldSource;
                field.value = aValue;
                field.position = { decodeLocalCoordinate( aCursor.U16At( offset + aXOffset ) ),
                                   decodeLocalCoordinate( aCursor.U16At( offset + aXOffset + 2 ) ), fieldSource };
                field.angle = NormalizeAngle( aCursor.U16At( offset + aAngleOffset ) );
                field.presentation.source = fieldSource;
                field.presentation.height = static_cast<int64_t>( aCursor.U16At( offset + aHeightOffset ) ) * 2;
                field.presentation.width = static_cast<int64_t>( aCursor.U8At( offset + aWidthOffset ) ) * 2;
                const uint16_t justification = aCursor.U16At( offset + aAngleOffset + 2 );
                field.presentation.horizontalJustification =
                        horizontalJustification( justification, fieldSource, aModel.diagnostics );
                field.presentation.verticalJustification = verticalJustification( justification );
                SOURCE_PROVENANCE fontSource = fieldSource;
                fontSource.absoluteOffset = offset + aFontOffset;
                fontSource.length = 2;
                font( static_cast<int16_t>( aCursor.U16At( offset + aFontOffset ) ), fontSource, field.presentation,
                      false );
                placement.fields.push_back( std::move( field ) );
            };

            SOURCE_STRING referenceValue = placement.reference;
            addInlineField( wxS( "REF-DES" ), referenceValue, layout.referenceField, layout.referenceFieldAngle,
                            layout.referenceFont, layout.referenceHeight, layout.referenceWidth );
            addInlineField( wxS( "PART-TYPE" ), part->name, layout.partTypeField, layout.partTypeFieldAngle,
                            layout.partTypeFont, layout.partTypeHeight, layout.partTypeWidth );

            const uint16_t customFieldCount = aCursor.U16At( offset + layout.fieldCount );

            if( fieldCursor > controllers.pools[16].count
                || customFieldCount > controllers.pools[16].count - fieldCursor )
            {
                throwDecodeError( source, wxS( "placement field ownership does not match controller 17" ) );
            }

            uint16_t namedFieldCount = 0;

            for( size_t fieldOrdinal = 0; fieldOrdinal < customFieldCount; ++fieldOrdinal )
            {
                const size_t candidateOffset = fieldBase + ( fieldCursor + fieldOrdinal ) * layout.fieldBytes;

                if( ( aCursor.U8At( candidateOffset + layout.customDisplayFlags ) & 7 ) != 0 )
                    ++namedFieldCount;
            }

            if( namedFieldCount > attributeCount - 2 )
                throwDecodeError( source, wxS( "named placement fields leave component attribute slice" ) );

            const uint32_t customAttributeStart = attributeStart + attributeCount - namedFieldCount;
            uint16_t       namedFieldOrdinal = 0;

            for( size_t fieldOrdinal = 0; fieldOrdinal < customFieldCount; ++fieldOrdinal )
            {
                const size_t      fieldOffset = fieldBase + fieldCursor * layout.fieldBytes;
                SOURCE_PROVENANCE fieldSource =
                        sourceAt( aSourceName, aModel.version, wxS( "placement field" ), 17, fieldCursor, fieldOffset,
                                  layout.fieldBytes, static_cast<int>( aSheetIndex ) );
                SOURCE_STRING name;
                SOURCE_STRING value;

                const uint8_t displayFlags = aCursor.U8At( fieldOffset + layout.customDisplayFlags );

                if( ( displayFlags & 7 ) == 0 )
                {
                    name.text = wxS( "*" );
                    name.source = fieldSource;
                    value.source = fieldSource;
                }
                else
                {
                    std::tie( name, value ) = attributeString( customAttributeStart + namedFieldOrdinal, true );
                    ++namedFieldOrdinal;
                }
                MODEL_FIELD field;
                field.source = fieldSource;
                field.name = std::move( name );
                field.value = std::move( value );
                field.position = { decodeLocalCoordinate( aCursor.U16At( fieldOffset + layout.customX ) ),
                                   decodeLocalCoordinate( aCursor.U16At( fieldOffset + layout.customX + 2 ) ),
                                   fieldSource };
                field.angle = NormalizeAngle( aCursor.U16At( fieldOffset + layout.customAngle ) );

                if( field.angle % 900 != 0 )
                    throwDecodeError( fieldSource, wxS( "unsupported placement-field rotation" ) );

                const uint8_t justification = aCursor.U8At( fieldOffset + layout.customJustification );
                field.presentation.source = fieldSource;
                field.presentation.horizontalJustification =
                        horizontalJustification( justification, fieldSource, aModel.diagnostics );
                field.presentation.verticalJustification = verticalJustification( justification );
                field.presentation.height =
                        static_cast<int64_t>( aCursor.U16At( fieldOffset + layout.customHeight ) ) * 2;
                field.presentation.width = static_cast<int64_t>( aCursor.U8At( fieldOffset + layout.customWidth ) ) * 2;
                field.presentation.visible = ( displayFlags & 8 ) == 0;
                field.visible = field.presentation.visible;

                SOURCE_PROVENANCE fontSource = fieldSource;
                fontSource.length = 2;
                font( static_cast<int16_t>( aCursor.U16At( fieldOffset + layout.customFont ) ), fontSource,
                      field.presentation, true );
                field.properties.push_back( sourceProperty(
                        wxS( "display_flags" ), wxString::Format( wxS( "%u" ), displayFlags ), fieldSource ) );
                SOURCE_PROVENANCE lineWidthSource = fieldSource;
                lineWidthSource.absoluteOffset += layout.customLineWidth;
                lineWidthSource.length = 2;
                field.properties.push_back( sourceProperty(
                        wxS( "line_width_half_mil" ),
                        wxString::Format( wxS( "%u" ), aCursor.U16At( fieldOffset + layout.customLineWidth ) ),
                        lineWidthSource ) );
                SOURCE_PROPERTY preservedTail = sourceProperty(
                        wxS( "preserved_field_tail" ),
                        wxString::Format( wxS( "%u" ), aCursor.U16At( fieldOffset + layout.customTail ) ),
                        fieldSource );
                preservedTail.disposition = PROPERTY_DISPOSITION::PRESERVED;
                field.properties.push_back( std::move( preservedTail ) );
                placement.fields.push_back( std::move( field ) );
                ++fieldCursor;
            }

            SOURCE_PROVENANCE rawAngleSource = source;
            rawAngleSource.absoluteOffset += layout.angle;
            rawAngleSource.length = 2;
            placement.properties.push_back(
                    sourceProperty( wxS( "raw_angle" ), wxString::Format( wxS( "%u" ), rawAngle ), rawAngleSource ) );
            placement.properties.push_back( sourceProperty(
                    wxS( "component_identity" ), wxString::Format( wxS( "%u" ), componentIdentity ), source ) );
            placement.properties.push_back( sourceProperty( wxS( "component_group_handle" ),
                                                            wxString::Format( wxS( "%u" ), groupHandle ), source ) );
            placement.properties.push_back(
                    sourceProperty( wxS( "decal_handle" ), wxString::Format( wxS( "%u" ), decalHandle ), source ) );
            SOURCE_PROVENANCE rawMirrorSource = source;
            rawMirrorSource.absoluteOffset += layout.mirror;
            rawMirrorSource.length = 2;
            placement.properties.push_back( sourceProperty(
                    wxS( "raw_mirror" ), wxString::Format( wxS( "%u" ), placement.mirrorFlags ), rawMirrorSource ) );
            aModel.placements.push_back( std::move( placement ) );
        }

        if( expectedPinStart != controllers.pools[15].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "placed pin" ), 16, expectedPinStart, placedPinBase,
                              controllers.pools[15].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned placed-pin record" ) );
        }

        if( fieldCursor != controllers.pools[16].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "placement field" ), 17, fieldCursor, fieldBase,
                              controllers.pools[16].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned placement-field record" ) );
        }
    }


    void decodeConnectivity( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                             const SCH_SDB_BLOCK& aBlock, size_t aSheetIndex, const wxString& aSourceName,
                             const CONNECTIVITY_GLOBALS& aGlobals, PADS_SCH_MODEL& aModel )
    {
        if( aModel.version != 0x000D )
            throwDecodeError( aModel.source, wxS( "connectivity decoder selected for raw-preserved version" ) );

        const SHEET_CONTROLLERS controllers = sheetControllers( aCursor, aBlock );
        requireFixedController( controllers, 18, BUS_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 19, JUNCTION_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 20, OFFPAGE_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 21, CONNECTION_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );
        requireFixedController( controllers, 22, CONNECTION_VERTEX_BYTES, aSourceName, aModel.version, aSheetIndex );

        const size_t                 busBase = controllers.offsets[17];
        const size_t                 junctionBase = controllers.offsets[18];
        const size_t                 offpageBase = controllers.offsets[19];
        const size_t                 connectionBase = controllers.offsets[20];
        const size_t                 vertexBase = controllers.offsets[21];
        std::unordered_set<uint32_t> busGlobalRecords;

        for( size_t record = 0; record < controllers.pools[17].count; ++record )
        {
            const uint32_t globalRecord = aCursor.U32At( busBase + record * BUS_RECORD_BYTES + 8 );
            const uint8_t  netKind =
                    globalRecord < aGlobals.nets.size() ? aGlobals.nets[globalRecord].kindFlags & 0xFF : 0;

            if( globalRecord >= aGlobals.nets.size() || aGlobals.nets[globalRecord].tombstone
                || ( netKind != 1 && netKind != 5 ) )
            {
                SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "bus" ), 18, record,
                                                     busBase + record * BUS_RECORD_BYTES, BUS_RECORD_BYTES,
                                                     static_cast<int>( aSheetIndex ) );
                throwDecodeError( source, wxS( "bus global-net handle targets wrong or unresolved object class" ) );
            }

            busGlobalRecords.insert( globalRecord );
        }

        std::vector<MODEL_NET*> sheetNets( aGlobals.nets.size(), nullptr );
        aModel.nets.reserve( aModel.nets.size() + aGlobals.nets.size() );

        for( size_t globalRecord = 0; globalRecord < aGlobals.nets.size(); ++globalRecord )
        {
            const GLOBAL_NET_RECORD& global = aGlobals.nets[globalRecord];

            if( global.tombstone )
                continue;

            for( uint32_t membership = global.membershipStart;
                 membership < global.membershipStart + global.membershipCount; ++membership )
            {
                if( aGlobals.membershipSheets[membership] != aSheetIndex )
                    continue;

                if( global.aliasCount != 0 || busGlobalRecords.contains( globalRecord ) )
                    continue;

                MODEL_NET net;
                net.id = NET_ID( membership );
                net.source = global.source;
                net.source.sheet = static_cast<int>( aSheetIndex );
                SOURCE_PROVENANCE membershipSource =
                        sourceAt( aSourceName, aModel.version, wxS( "net sheet membership" ), 4, membership,
                                  aGlobals.membershipBase + membership * NET_MEMBERSHIP_BYTES, NET_MEMBERSHIP_BYTES,
                                  static_cast<int>( aSheetIndex ) );
                net.sheet = { aModel.sheets[aSheetIndex].id, membershipSource };
                net.name = global.name;
                net.properties.push_back( sourceProperty(
                        wxS( "global_net_record" ), wxString::Format( wxS( "%llu" ), globalRecord ), global.source ) );
                SOURCE_PROPERTY identity =
                        sourceProperty( wxS( "preserved_net_identity" ),
                                        wxString::Format( wxS( "%u" ), global.preservedIdentity ), global.source );
                identity.disposition = PROPERTY_DISPOSITION::PRESERVED;
                net.properties.push_back( std::move( identity ) );
                SOURCE_PROVENANCE relationshipSource = global.source;
                relationshipSource.absoluteOffset += 84;
                relationshipSource.length = 4;
                SOURCE_PROPERTY relationship = sourceProperty(
                        wxS( "preserved_net_relationship" ),
                        wxString::Format( wxS( "%u" ), global.preservedRelationship ), relationshipSource );
                relationship.disposition = PROPERTY_DISPOSITION::PRESERVED;
                net.properties.push_back( std::move( relationship ) );
                aModel.nets.push_back( std::move( net ) );
                sheetNets[globalRecord] = &aModel.nets.back();
            }
        }

        struct PLACED_PIN_JOIN
        {
            MODEL_PLACEMENT* placement = nullptr;
            PIN_ID           pin;
        };

        std::vector<PLACED_PIN_JOIN> placedPins( controllers.pools[15].count );
        const PLACEMENT_LAYOUT&      placementSchema = placementLayout( aModel.version );

        for( MODEL_PLACEMENT& placement : aModel.placements )
        {
            if( placement.source.sheet != static_cast<int>( aSheetIndex ) )
                continue;

            const size_t placementOffset =
                    controllers.offsets[14] + placement.source.recordIndex * placementSchema.placementBytes;
            const uint32_t pinStart = aCursor.U32At( placementOffset + placementSchema.pinStart );
            const uint16_t pinCount = aCursor.U16At( placementOffset + placementSchema.pinCount );

            if( pinStart > placedPins.size() || pinCount > placedPins.size() - pinStart
                || pinCount != placement.pins.size() )
            {
                throwDecodeError( placement.source, wxS( "placement pin join leaves controller 16" ) );
            }

            for( size_t pin = 0; pin < pinCount; ++pin )
                placedPins[pinStart + pin] = { &placement, placement.pins[pin].id };
        }

        auto offpagePosition = [&]( size_t aRecord )
        {
            const size_t      offset = offpageBase + aRecord * OFFPAGE_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "off-page reference" ), 20, aRecord,
                                                 offset, OFFPAGE_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            return SOURCE_POINT{ decodeCoordinate( aCursor.U16At( offset + 22 ) ),
                                 decodeCoordinate( aCursor.U16At( offset + 24 ) ), source };
        };

        auto junctionPosition = [&]( size_t aRecord )
        {
            const size_t      offset = junctionBase + aRecord * JUNCTION_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "junction" ), 19, aRecord, offset,
                                                 JUNCTION_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            return SOURCE_POINT{ decodeCoordinate( aCursor.U16At( offset + 4 ) ),
                                 decodeCoordinate( aCursor.U16At( offset + 6 ) ), source };
        };

        for( size_t record = 0; record < controllers.pools[18].count; ++record )
        {
            const size_t      offset = junctionBase + record * JUNCTION_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "junction" ), 19, record, offset,
                                                 JUNCTION_RECORD_BYTES, static_cast<int>( aSheetIndex ) );

            const uint16_t status = aCursor.U16At( offset + 10 );

            if( status != 0 && status != 0x00FC && status != 0x00FD )
                throwDecodeError( source, wxS( "invalid junction object-class marker" ) );

            MODEL_JUNCTION junction;
            junction.source = source;
            junction.sheet = { aModel.sheets[aSheetIndex].id, source };
            junction.position = junctionPosition( record );
            SOURCE_PROVENANCE connectionSource = source;
            connectionSource.absoluteOffset += 8;
            connectionSource.length = 2;
            junction.properties.push_back( sourceProperty( wxS( "connection_record" ),
                                                           wxString::Format( wxS( "%u" ), aCursor.U16At( offset + 8 ) ),
                                                           connectionSource ) );
            aModel.junctions.push_back( std::move( junction ) );
        }

        std::vector<uint32_t> objectVertexStarts;
        objectVertexStarts.reserve( controllers.pools[17].count + controllers.pools[20].count );

        for( size_t record = 0; record < controllers.pools[17].count; ++record )
            objectVertexStarts.push_back( aCursor.U32At( busBase + record * BUS_RECORD_BYTES + 4 ) );

        for( size_t record = 0; record < controllers.pools[20].count; ++record )
            objectVertexStarts.push_back( aCursor.U32At( connectionBase + record * CONNECTION_RECORD_BYTES + 4 ) );

        std::vector<size_t> objectOrder( objectVertexStarts.size() );

        for( size_t object = 0; object < objectOrder.size(); ++object )
            objectOrder[object] = object;

        std::ranges::sort( objectOrder,
                           [&]( size_t aLeft, size_t aRight )
                           {
                               return objectVertexStarts[aLeft] < objectVertexStarts[aRight];
                           } );
        std::vector<uint32_t> objectVertexEnds( objectVertexStarts.size() );

        for( size_t ordinal = 0; ordinal < objectOrder.size(); ++ordinal )
        {
            const size_t   object = objectOrder[ordinal];
            const uint32_t end = ordinal + 1 < objectOrder.size() ? objectVertexStarts[objectOrder[ordinal + 1]]
                                                                  : controllers.pools[21].count;

            if( ( ordinal == 0 && objectVertexStarts[object] != 0 ) || objectVertexStarts[object] >= end
                || end > controllers.pools[21].count )
            {
                SOURCE_PROVENANCE source =
                        sourceAt( aSourceName, aModel.version, wxS( "connectivity vertex ownership" ),
                                  object < controllers.pools[17].count ? 18 : 21, object, vertexBase,
                                  controllers.pools[21].usedBytes, static_cast<int>( aSheetIndex ) );
                throwDecodeError( source, wxS( "connectivity vertex slices do not exactly tile controller 22" ) );
            }

            objectVertexEnds[object] = end;
        }

        if( objectVertexStarts.empty() && controllers.pools[21].count != 0 )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "connection vertex" ), 22, 0, vertexBase,
                              controllers.pools[21].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unclaimed connection vertices" ) );
        }

        auto appendVertices = [&]( std::vector<SOURCE_POINT>& aVertices, size_t aObject )
        {
            const uint32_t start = objectVertexStarts[aObject];
            const uint32_t end = objectVertexEnds[aObject];

            for( uint32_t vertex = start; vertex < end; ++vertex )
            {
                const size_t      offset = vertexBase + vertex * CONNECTION_VERTEX_BYTES;
                SOURCE_PROVENANCE source =
                        sourceAt( aSourceName, aModel.version, wxS( "connection vertex" ), 22, vertex, offset,
                                  CONNECTION_VERTEX_BYTES, static_cast<int>( aSheetIndex ) );

                if( aCursor.U32At( offset ) != 0 )
                    throwDecodeError( source, wxS( "nonzero connection-vertex padding" ) );

                aVertices.push_back( { decodeCoordinate( aCursor.U16At( offset + 4 ) ),
                                       decodeCoordinate( aCursor.U16At( offset + 6 ) ), source } );
            }
        };

        std::vector<MODEL_CONNECTION*>       connections( controllers.pools[20].count, nullptr );
        std::vector<MODEL_NET*>              connectionNets( controllers.pools[20].count, nullptr );
        std::vector<std::array<uint16_t, 2>> connectionEndpointHandles( controllers.pools[20].count );
        std::vector<std::vector<size_t>>     junctionBacklinks( controllers.pools[18].count );
        std::vector<std::vector<size_t>>     offpageBacklinks( controllers.pools[19].count );
        std::vector<size_t>                  connectionCounts( sheetNets.size(), 0 );

        for( size_t record = 0; record < controllers.pools[20].count; ++record )
        {
            const uint32_t netHandle = aCursor.U32At( connectionBase + record * CONNECTION_RECORD_BYTES + 8 );

            if( netHandle < connectionCounts.size() )
                ++connectionCounts[netHandle];
        }

        for( size_t net = 0; net < sheetNets.size(); ++net )
        {
            if( sheetNets[net] )
                sheetNets[net]->connections.reserve( connectionCounts[net] );
        }

        for( size_t record = 0; record < controllers.pools[20].count; ++record )
        {
            const size_t      offset = connectionBase + record * CONNECTION_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "connection" ), 21, record, offset,
                                                 CONNECTION_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint32_t    netHandle = aCursor.U32At( offset + 8 );

            if( netHandle >= sheetNets.size() || !sheetNets[netHandle] )
                throwDecodeError( source, wxS( "connection net handle targets wrong or unresolved object class" ) );

            const uint16_t marker = aCursor.U16At( offset + 34 );

            const uint8_t markerStatus = marker & 0xFF;

            if( marker >> 8 < 2 || marker >> 8 > 6
                || ( markerStatus != 0 && markerStatus != 0xFC && markerStatus != 0xFD ) )
                throwDecodeError( source, wxS( "invalid connection object-class marker" ) );

            MODEL_CONNECTION connection;
            connection.source = source;

            auto endpoint = [&]( size_t aFieldOffset )
            {
                SOURCE_PROVENANCE endpointSource = source;
                endpointSource.objectClass = wxS( "connection endpoint" );
                endpointSource.absoluteOffset += aFieldOffset;
                endpointSource.length = 2;
                const uint16_t            raw = aCursor.U16At( endpointSource.absoluteOffset );
                const uint16_t            objectClass = raw >> 12;
                const uint16_t            objectRecord = raw & 0x0FFF;
                MODEL_CONNECTION_ENDPOINT result;
                result.source = endpointSource;

                switch( objectClass )
                {
                case 0:
                    if( objectRecord >= placedPins.size() || !placedPins[objectRecord].placement
                        || !placedPins[objectRecord].pin.IsValid() )
                    {
                        throwDecodeError( endpointSource, wxS( "unresolved placed-pin endpoint" ) );
                    }

                    result.kind = MODEL_ENDPOINT_KIND::PIN;
                    result.placement = PLACEMENT_REFERENCE{ placedPins[objectRecord].placement->id, endpointSource };
                    result.pin = PIN_REFERENCE{ placedPins[objectRecord].pin, endpointSource };
                    result.point = { placedPins[objectRecord].placement->position.x,
                                     placedPins[objectRecord].placement->position.y, endpointSource };
                    break;

                case 2:
                    if( objectRecord >= controllers.pools[19].count )
                        throwDecodeError( endpointSource, wxS( "unresolved off-page endpoint" ) );

                    result.kind = MODEL_ENDPOINT_KIND::POINT;
                    result.point = offpagePosition( objectRecord );
                    break;

                case 3:
                    if( objectRecord >= controllers.pools[18].count )
                        throwDecodeError( endpointSource, wxS( "unresolved junction endpoint" ) );

                    result.kind = MODEL_ENDPOINT_KIND::POINT;
                    result.point = junctionPosition( objectRecord );
                    break;

                default: throwDecodeError( endpointSource, wxS( "wrong endpoint object class" ) );
                }

                result.properties.push_back( sourceProperty( wxS( "raw_endpoint_handle" ),
                                                             wxString::Format( wxS( "%u" ), raw ), endpointSource ) );
                return result;
            };

            connection.endpoints.push_back( endpoint( 12 ) );
            connection.endpoints.push_back( endpoint( 14 ) );
            appendVertices( connection.vertices, controllers.pools[17].count + record );

            if( connection.vertices.size() < 2 )
                throwDecodeError( source, wxS( "connection lacks explicit endpoint vertices" ) );

            for( size_t endpointIndex = 0; endpointIndex < connection.endpoints.size(); ++endpointIndex )
            {
                MODEL_CONNECTION_ENDPOINT& decodedEndpoint = connection.endpoints[endpointIndex];
                const SOURCE_POINT&        wirePoint =
                        endpointIndex == 0 ? connection.vertices.front() : connection.vertices.back();

                if( decodedEndpoint.kind == MODEL_ENDPOINT_KIND::POINT
                    && ( decodedEndpoint.point.x != wirePoint.x || decodedEndpoint.point.y != wirePoint.y ) )
                {
                    throwDecodeError( decodedEndpoint.source,
                                      wxS( "typed endpoint position does not match wire endpoint vertex" ) );
                }

                decodedEndpoint.point = { wirePoint.x, wirePoint.y, wirePoint.source };
            }

            connectionEndpointHandles[record] = { aCursor.U16At( offset + 12 ), aCursor.U16At( offset + 14 ) };

            for( size_t endpointIndex = 0; endpointIndex < connectionEndpointHandles[record].size(); ++endpointIndex )
            {
                const uint16_t       raw = connectionEndpointHandles[record][endpointIndex];
                const size_t         objectClass = raw >> 12;
                const size_t         objectRecord = raw & 0x0FFF;
                std::vector<size_t>* backlinks = nullptr;

                if( objectClass == 2 )
                    backlinks = &offpageBacklinks[objectRecord];
                else if( objectClass == 3 )
                    backlinks = &junctionBacklinks[objectRecord];

                if( backlinks )
                {
                    if( endpointIndex == 1 && connectionEndpointHandles[record][0] == raw )
                    {
                        SOURCE_PROVENANCE endpointSource = source;
                        endpointSource.objectClass = wxS( "connection endpoint" );
                        endpointSource.absoluteOffset += endpointIndex == 0 ? 12 : 14;
                        endpointSource.length = 2;
                        throwDecodeError( endpointSource, wxS( "duplicate typed endpoint backlink" ) );
                    }

                    backlinks->push_back( record );
                }
            }

            connection.properties.push_back(
                    sourceProperty( wxS( "raw_connection_marker" ), wxString::Format( wxS( "%u" ), marker ), source ) );
            sheetNets[netHandle]->connections.push_back( std::move( connection ) );
            connections[record] = &sheetNets[netHandle]->connections.back();
            connectionNets[record] = sheetNets[netHandle];
        }

        for( size_t record = 0; record < controllers.pools[18].count; ++record )
        {
            const size_t      offset = junctionBase + record * JUNCTION_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "junction" ), 19, record, offset,
                                                 JUNCTION_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint16_t    owner = aCursor.U16At( offset + 8 );
            SOURCE_PROVENANCE ownerSource = source;
            ownerSource.absoluteOffset += 8;
            ownerSource.length = 2;

            if( owner >= connections.size() || !connections[owner] )
                throwDecodeError( ownerSource, wxS( "junction connection handle leaves controller 21" ) );

            if( std::ranges::find( junctionBacklinks[record], owner ) == junctionBacklinks[record].end() )
                throwDecodeError( ownerSource, wxS( "junction connection handle does not point back" ) );

            for( size_t connection : junctionBacklinks[record] )
            {
                if( connectionNets[connection] != connectionNets[owner] )
                {
                    throwDecodeError( ownerSource, wxS( "junction is shared across different nets" ) );
                }
            }
        }

        for( size_t record = 0; record < controllers.pools[19].count; ++record )
        {
            const size_t      offset = offpageBase + record * OFFPAGE_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "off-page reference" ), 20, record,
                                                 offset, OFFPAGE_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint16_t    owner = aCursor.U16At( offset + 8 );
            SOURCE_PROVENANCE ownerSource = source;
            ownerSource.absoluteOffset += 8;
            ownerSource.length = 2;

            if( owner >= connections.size() || !connections[owner] )
                throwDecodeError( ownerSource, wxS( "off-page net handle leaves controller 21" ) );

            if( std::ranges::find( offpageBacklinks[record], owner ) == offpageBacklinks[record].end() )
                throwDecodeError( ownerSource, wxS( "off-page connection handle does not point back" ) );

            for( size_t connection : offpageBacklinks[record] )
            {
                if( connectionNets[connection] != connectionNets[owner] )
                    throwDecodeError( ownerSource, wxS( "off-page reference is shared across different nets" ) );
            }
        }

        std::vector<bool> claimedBusEntries( controllers.pools[19].count, false );

        for( size_t record = 0; record < controllers.pools[17].count; ++record )
        {
            const size_t      offset = busBase + record * BUS_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "bus" ), 18, record, offset,
                                                 BUS_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint16_t    marker = aCursor.U16At( offset + 38 );

            const uint8_t markerStatus = marker & 0xFF;

            if( marker >> 8 < 2 || marker >> 8 > 4 || ( markerStatus != 0 && markerStatus != 0xFD ) )
                throwDecodeError( source, wxS( "invalid bus object-class marker" ) );

            const size_t globalRecord = aCursor.U32At( offset + 8 );

            if( globalRecord >= aGlobals.nets.size() || aGlobals.nets[globalRecord].tombstone
                || !busGlobalRecords.contains( globalRecord ) )
            {
                throwDecodeError( source, wxS( "bus global-net handle targets wrong or unresolved object class" ) );
            }

            const GLOBAL_NET_RECORD& global = aGlobals.nets[globalRecord];
            uint32_t                 membership = global.membershipStart;

            while( membership < global.membershipStart + global.membershipCount
                   && aGlobals.membershipSheets[membership] != aSheetIndex )
            {
                ++membership;
            }

            if( membership == global.membershipStart + global.membershipCount )
                throwDecodeError( source, wxS( "bus global-net handle does not belong to this sheet" ) );

            MODEL_BUS bus;
            bus.id = BUS_ID( static_cast<uint32_t>( aSheetIndex * 0x100000 + record + 1 ) );
            bus.source = source;
            bus.sheet = { aModel.sheets[aSheetIndex].id, source };
            bus.name = global.name;
            bus.aliases.push_back( global.name );
            SOURCE_PROPERTY identity =
                    sourceProperty( wxS( "preserved_net_identity" ),
                                    wxString::Format( wxS( "%u" ), global.preservedIdentity ), global.source );
            identity.disposition = PROPERTY_DISPOSITION::PRESERVED;
            bus.properties.push_back( std::move( identity ) );
            SOURCE_PROVENANCE relationshipSource = global.source;
            relationshipSource.absoluteOffset += 84;
            relationshipSource.length = 4;
            SOURCE_PROPERTY relationship =
                    sourceProperty( wxS( "preserved_net_relationship" ),
                                    wxString::Format( wxS( "%u" ), global.preservedRelationship ), relationshipSource );
            relationship.disposition = PROPERTY_DISPOSITION::PRESERVED;
            bus.properties.push_back( std::move( relationship ) );
            appendVertices( bus.vertices, record );

            std::vector<size_t>        entryRecords;
            uint16_t                   entryHandle = aCursor.U16At( offset + 24 );
            std::unordered_set<size_t> chain;

            while( ( entryHandle >> 12 ) == 2 )
            {
                const size_t entryRecord = entryHandle & 0x0FFF;

                if( entryRecord >= controllers.pools[19].count )
                    throwDecodeError( source, wxS( "unresolved bus-entry handle" ) );

                if( !chain.insert( entryRecord ).second )
                    throwDecodeError( source, wxS( "cyclic bus-entry handle chain" ) );

                entryRecords.push_back( entryRecord );
                entryHandle = aCursor.U16At( offpageBase + entryRecord * OFFPAGE_RECORD_BYTES + 4 );
            }

            if( record > 0x0FFF || entryHandle != 0xBFFF - record )
                throwDecodeError( source, wxS( "bus-entry chain terminates in wrong object class" ) );

            std::ranges::reverse( entryRecords );

            const bool exactAliasMapping = entryRecords.size() == global.aliasMembers.size();

            if( !exactAliasMapping )
            {
                wxString preservedMembers;

                for( const SOURCE_STRING& member : global.aliasMembers )
                {
                    if( !preservedMembers.empty() )
                        preservedMembers += wxS( "\n" );

                    preservedMembers += member.text;
                }

                SOURCE_PROPERTY members =
                        sourceProperty( wxS( "preserved_bus_alias_members" ), preservedMembers, global.source );
                members.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                bus.properties.push_back( std::move( members ) );
                aModel.diagnostics.push_back( { RPT_SEVERITY_WARNING, global.source,
                                                wxS( "PADS property 'preserved_bus_alias_members' retained with "
                                                     "unsupported disposition" ) } );
            }

            for( size_t entry = 0; entry < entryRecords.size(); ++entry )
            {
                const size_t      entryRecord = entryRecords[entry];
                const size_t      entryOffset = offpageBase + entryRecord * OFFPAGE_RECORD_BYTES;
                SOURCE_PROVENANCE entrySource =
                        sourceAt( aSourceName, aModel.version, wxS( "bus entry" ), 20, entryRecord, entryOffset,
                                  OFFPAGE_RECORD_BYTES, static_cast<int>( aSheetIndex ) );

                if( aCursor.U8At( entryOffset + 30 ) != 0xFF )
                    throwDecodeError( entrySource, wxS( "bus-entry handle targets wrong off-page object class" ) );

                if( claimedBusEntries[entryRecord] )
                    throwDecodeError( entrySource, wxS( "duplicate bus-entry membership" ) );

                const uint16_t connectionHandle = aCursor.U16At( entryOffset + 8 );

                if( connectionHandle >= connections.size() || !connections[connectionHandle] )
                    throwDecodeError( entrySource, wxS( "unresolved bus-entry connection reference" ) );

                MODEL_CONNECTION* entryConnection = connections[connectionHandle];
                MODEL_NET*        memberNet = connectionNets[connectionHandle];

                if( !memberNet )
                    throwDecodeError( entrySource, wxS( "bus-entry connection targets wrong member-net class" ) );

                if( exactAliasMapping && memberNet->name.text != global.aliasMembers[entry].text )
                    throwDecodeError( entrySource, wxS( "bus alias member does not match connected net" ) );

                const bool ownsConnection = std::ranges::any_of( memberNet->connections,
                                                                 [&]( const MODEL_CONNECTION& aConnection )
                                                                 {
                                                                     return &aConnection == entryConnection;
                                                                 } );

                if( !ownsConnection )
                    throwDecodeError( entrySource, wxS( "bus-entry connection targets wrong member-net class" ) );

                claimedBusEntries[entryRecord] = true;
                MODEL_BUS_ENTRY busEntry;
                busEntry.source = entrySource;
                busEntry.position = offpagePosition( entryRecord );
                busEntry.memberNet = { memberNet->id, entrySource };
                bus.entries.push_back( std::move( busEntry ) );
                bus.memberNets.push_back( { memberNet->id, entrySource } );
            }

            aModel.buses.push_back( std::move( bus ) );
        }

        for( size_t record = 0; record < controllers.pools[19].count; ++record )
        {
            const size_t      offset = offpageBase + record * OFFPAGE_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "off-page reference" ), 20, record,
                                                 offset, OFFPAGE_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            const uint8_t     rawKind = aCursor.U8At( offset + 30 );

            if( rawKind == 0xFF )
            {
                if( !claimedBusEntries[record] )
                    throwDecodeError( source, wxS( "unclaimed bus-entry record" ) );

                continue;
            }

            const uint16_t connectionHandle = aCursor.U16At( offset + 8 );

            if( connectionHandle >= connections.size() || !connections[connectionHandle] )
                throwDecodeError( source, wxS( "off-page net handle leaves controller 21" ) );

            MODEL_NET* ownerNet = connectionNets[connectionHandle];

            if( !ownerNet )
                throwDecodeError( source, wxS( "off-page record targets wrong net object class" ) );

            MODEL_LABEL label;
            label.source = source;
            label.sheet = { aModel.sheets[aSheetIndex].id, source };
            label.text = ownerNet->name;
            label.position = offpagePosition( record );
            label.angle = NormalizeAngle( aCursor.U16At( offset + 26 ) );

            switch( rawKind )
            {
            case 0xFE: label.kind = MODEL_LABEL_KIND::LOCAL; break;
            case 0:
            case 1:
            {
                label.kind = rawKind == 0 ? MODEL_LABEL_KIND::GROUND : MODEL_LABEL_KIND::POWER;
                const uint16_t decalHandle = aCursor.U16At( offset + 4 );

                if( decalHandle >= controllers.pools[6].count )
                    throwDecodeError( source, wxS( "power-label decal handle leaves controller 7" ) );
                break;
            }
            case 3:
                label.kind = MODEL_LABEL_KIND::GLOBAL;

                for( uint32_t membership =
                             aGlobals.nets[aCursor.U32At( connectionBase + connectionHandle * CONNECTION_RECORD_BYTES
                                                          + 8 )]
                                     .membershipStart;
                     membership
                     < aGlobals.nets[aCursor.U32At( connectionBase + connectionHandle * CONNECTION_RECORD_BYTES + 8 )]
                                       .membershipStart
                               + aGlobals.nets[aCursor.U32At( connectionBase
                                                              + connectionHandle * CONNECTION_RECORD_BYTES + 8 )]
                                         .membershipCount;
                     ++membership )
                {
                    const uint16_t peerSheet = aGlobals.membershipSheets[membership];

                    if( peerSheet != aSheetIndex )
                        label.linkedSheets.push_back( { aModel.sheets[peerSheet].id, source } );
                }
                break;
            case 2:
            case 4:
            case 5:
                label.kind = MODEL_LABEL_KIND::UNSUPPORTED;
                aModel.diagnostics.push_back(
                        { RPT_SEVERITY_WARNING, source, wxS( "unsupported off-page label kind preserved" ) } );
                break;
            default: throwDecodeError( source, wxS( "unknown off-page label kind" ) );
            }

            label.properties.push_back(
                    sourceProperty( wxS( "raw_label_kind" ), wxString::Format( wxS( "%u" ), rawKind ), source ) );
            aModel.labels.push_back( std::move( label ) );
        }
    }


    void assignFieldIds( PADS_SCH_MODEL& aModel )
    {
        auto assign = []( MODEL_FIELD& aField, FIELD_ID_DOMAIN aDomain, uint32_t aOwner, size_t aOrdinal )
        {
            if( aOrdinal > FIELD_ID_MAX_ORDINAL )
                throwDecodeError( aField.source, wxS( "field ordinal exceeds identity capacity" ) );

            aField.id = MakeFieldId( aDomain, aOwner, static_cast<uint32_t>( aOrdinal ) );
        };

        for( MODEL_SHEET& sheet : aModel.sheets )
        {
            for( size_t ordinal = 0; ordinal < sheet.titleBlockFields.size(); ++ordinal )
            {
                MODEL_FIELD& field = sheet.titleBlockFields[ordinal];
                assign( field, FIELD_ID_DOMAIN::SHEET, sheet.id.Value(), ordinal );
            }
        }

        for( MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
        {
            for( size_t ordinal = 0; ordinal < definition.fields.size(); ++ordinal )
            {
                MODEL_FIELD& field = definition.fields[ordinal];
                assign( field, FIELD_ID_DOMAIN::DEFINITION, definition.id.Value(), ordinal );
            }
        }

        for( MODEL_PART_TYPE& partType : aModel.partTypes )
        {
            for( size_t ordinal = 0; ordinal < partType.fields.size(); ++ordinal )
            {
                MODEL_FIELD& field = partType.fields[ordinal];
                assign( field, FIELD_ID_DOMAIN::PART_TYPE, partType.id.Value(), ordinal );
            }
        }

        for( MODEL_PLACEMENT& placement : aModel.placements )
        {
            for( size_t ordinal = 0; ordinal < placement.fields.size(); ++ordinal )
            {
                MODEL_FIELD& field = placement.fields[ordinal];
                assign( field, FIELD_ID_DOMAIN::PLACEMENT, placement.id.Value(), ordinal );
            }
        }
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
    std::unordered_set<uint64_t> fieldIds;

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

        for( const MODEL_FIELD& field : definition.fields )
        {
            if( !field.id.IsValid() || !fieldIds.insert( field.id.Value() ).second )
                return false;
        }
    }

    auto fieldsAreUnique = [&]( const std::vector<MODEL_FIELD>& aFields )
    {
        return std::ranges::all_of( aFields,
                                    [&]( const MODEL_FIELD& aField )
                                    {
                                        return aField.id.IsValid() && fieldIds.insert( aField.id.Value() ).second;
                                    } );
    };

    for( const MODEL_SHEET& sheet : sheets )
    {
        if( !fieldsAreUnique( sheet.titleBlockFields ) )
            return false;
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        if( !fieldsAreUnique( partType.fields ) )
            return false;
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !fieldsAreUnique( placement.fields ) )
            return false;
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

            if( definition == index.definitions.end() && gate.decalGroupMembers.empty() )
                return false;

            for( const DEFINITION_REFERENCE& alternate : gate.alternateDefinitions )
            {
                if( !index.definitions.contains( alternate.id.Value() ) )
                    return false;
            }

            for( const DEFINITION_REFERENCE& member : gate.decalGroupMembers )
            {
                if( !index.definitions.contains( member.id.Value() ) )
                    return false;
            }

            if( definition == index.definitions.end() )
                continue;

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

        auto placementDefinition = index.definitions.find( placement.definition.id.Value() );

        if( placementDefinition == index.definitions.end() )
            return false;

        for( const PIN_REFERENCE& pin : placement.pins )
        {
            auto owner = index.pinOwners.find( pin.id.Value() );

            if( owner == index.pinOwners.end() || owner->second != placementDefinition->second )
                return false;
        }

        if( placement.gate )
        {
            auto partType = index.partTypes.find( placement.partType.id.Value() );

            if( partType == index.partTypes.end() )
                return false;

            auto gate = index.gates.find( placement.gate->id.Value() );
            auto gateOwner = index.gateOwners.find( placement.gate->id.Value() );

            if( gate == index.gates.end() || gateOwner == index.gateOwners.end()
                || gateOwner->second != partType->second
                || ( gate->second->unit != placement.unit && gate->second->decalGroupMembers.empty() ) )
                return false;

            const MODEL_GATE& selectedGate = *gate->second;
            const bool        definitionMatches = selectedGate.definition.id == placement.definition.id
                                           || std::ranges::any_of( selectedGate.alternateDefinitions,
                                                                   [&]( const DEFINITION_REFERENCE& aDefinition )
                                                                   {
                                                                       return aDefinition.id == placement.definition.id;
                                                                   } )
                                           || std::ranges::any_of( selectedGate.decalGroupMembers,
                                                                   [&]( const DEFINITION_REFERENCE& aDefinition )
                                                                   {
                                                                       return aDefinition.id == placement.definition.id;
                                                                   } );

            if( !definitionMatches )
                return false;

            auto definition = index.definitions.find( placement.definition.id.Value() );

            if( definition == index.definitions.end() )
                return false;

            for( const PIN_REFERENCE& pin : placement.pins )
            {
                auto owner = index.pinOwners.find( pin.id.Value() );

                if( owner == index.pinOwners.end() || owner->second != definition->second )
                    return false;
            }
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

        if( std::ranges::any_of( label.linkedSheets,
                                 [&]( const SHEET_REFERENCE& aSheet )
                                 {
                                     return !index.sheets.contains( aSheet.id.Value() );
                                 } ) )
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
    std::unordered_map<uint64_t, SOURCE_PROVENANCE> fieldDeclarations;

    auto validateNestedId = [&]( const auto& aItem, const wxString& aObjectClass, auto& aDeclarations )
    {
        if( !aItem.id.IsValid() )
            throwValidationError( aItem.source, wxString::Format( wxS( "invalid %s ID" ), aObjectClass ) );

        auto [first, inserted] = aDeclarations.emplace( aItem.id.Value(), aItem.source );

        if( !inserted )
        {
            throwValidationError(
                    aItem.source,
                    wxString::Format( wxS( "duplicate %s ID %llu; first at v0x%04X %s controller %d record %llu "
                                           "sheet %d offset 0x%llX" ),
                                      aObjectClass, static_cast<unsigned long long>( aItem.id.Value() ),
                                      first->second.version, first->second.objectClass, first->second.controller,
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

        for( const MODEL_FIELD& field : definition.fields )
            validateNestedId( field, wxS( "field" ), fieldDeclarations );
    }

    for( const MODEL_SHEET& sheet : sheets )
    {
        for( const MODEL_FIELD& field : sheet.titleBlockFields )
            validateNestedId( field, wxS( "field" ), fieldDeclarations );
    }

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_FIELD& field : partType.fields )
            validateNestedId( field, wxS( "field" ), fieldDeclarations );
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        for( const MODEL_FIELD& field : placement.fields )
            validateNestedId( field, wxS( "field" ), fieldDeclarations );
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

            if( definition == index.definitions.end() && gate.decalGroupMembers.empty() )
                throwValidationError( gate.definition.source, wxS( "unresolved symbol definition reference" ) );

            for( const DEFINITION_REFERENCE& alternate : gate.alternateDefinitions )
            {
                if( !index.definitions.contains( alternate.id.Value() ) )
                    throwValidationError( alternate.source, wxS( "unresolved alternate definition reference" ) );
            }

            for( const DEFINITION_REFERENCE& member : gate.decalGroupMembers )
            {
                if( !index.definitions.contains( member.id.Value() ) )
                    throwValidationError( member.source, wxS( "unresolved pin-decal group member" ) );
            }

            if( definition == index.definitions.end() )
                continue;

            for( const PIN_REFERENCE& pin : gate.pins )
            {
                auto pinOwner = index.pinOwners.find( pin.id.Value() );

                if( pinOwner == index.pinOwners.end() || pinOwner->second != definition->second )
                    throwValidationError( pin.source, wxS( "pin does not belong to gate definition" ) );
            }
        }
    }

    struct DEFINITION_EDGE
    {
        uint32_t          target;
        SOURCE_PROVENANCE source;
    };

    std::unordered_map<uint32_t, std::vector<DEFINITION_EDGE>> definitionEdges;

    for( const MODEL_PART_TYPE& partType : partTypes )
    {
        for( const MODEL_GATE& gate : partType.gates )
        {
            if( !gate.definition.id.IsValid() )
                continue;

            for( const DEFINITION_REFERENCE& alternate : gate.alternateDefinitions )
                definitionEdges[gate.definition.id.Value()].push_back( { alternate.id.Value(), alternate.source } );
        }
    }

    std::unordered_map<uint32_t, uint8_t> definitionColors;
    std::function<void( uint32_t )>       visitDefinition = [&]( uint32_t aDefinition )
    {
        definitionColors[aDefinition] = 1;

        for( const DEFINITION_EDGE& edge : definitionEdges[aDefinition] )
        {
            if( definitionColors[edge.target] == 1 )
                throwValidationError( edge.source, wxS( "cyclic symbol definition reference" ) );

            if( definitionColors[edge.target] == 0 )
                visitDefinition( edge.target );
        }

        definitionColors[aDefinition] = 2;
    };

    for( const auto& [definition, edges] : definitionEdges )
    {
        if( definitionColors[definition] == 0 )
            visitDefinition( definition );
    }

    for( const MODEL_PLACEMENT& placement : placements )
    {
        if( !index.sheets.contains( placement.sheet.id.Value() ) )
            throwValidationError( placement.sheet.source, wxS( "unresolved placement sheet reference" ) );

        auto partType = index.partTypes.find( placement.partType.id.Value() );

        if( partType == index.partTypes.end() )
            throwValidationError( placement.partType.source, wxS( "unresolved placement part-type reference" ) );

        auto placementDefinition = index.definitions.find( placement.definition.id.Value() );

        if( placementDefinition == index.definitions.end() )
            throwValidationError( placement.definition.source, wxS( "unresolved placement definition" ) );

        for( const PIN_REFERENCE& pin : placement.pins )
        {
            auto owner = index.pinOwners.find( pin.id.Value() );

            if( owner == index.pinOwners.end() || owner->second != placementDefinition->second )
                throwValidationError( pin.source, wxS( "placement pin does not belong to selected definition" ) );
        }

        if( placement.gate )
        {
            auto gate = index.gates.find( placement.gate->id.Value() );
            auto gateOwner = index.gateOwners.find( placement.gate->id.Value() );

            if( gate == index.gates.end() || gateOwner == index.gateOwners.end()
                || gateOwner->second != partType->second
                || ( gate->second->unit != placement.unit && gate->second->decalGroupMembers.empty() ) )
                throwValidationError( placement.gate->source, wxS( "placement gate or unit mismatch" ) );

            const MODEL_GATE& selectedGate = *gate->second;
            const bool        definitionMatches = selectedGate.definition.id == placement.definition.id
                                           || std::ranges::any_of( selectedGate.alternateDefinitions,
                                                                   [&]( const DEFINITION_REFERENCE& aDefinition )
                                                                   {
                                                                       return aDefinition.id == placement.definition.id;
                                                                   } )
                                           || std::ranges::any_of( selectedGate.decalGroupMembers,
                                                                   [&]( const DEFINITION_REFERENCE& aDefinition )
                                                                   {
                                                                       return aDefinition.id == placement.definition.id;
                                                                   } );

            if( !definitionMatches )
                throwValidationError( placement.definition.source,
                                      wxS( "placement definition does not belong to selected gate" ) );

            auto definition = index.definitions.find( placement.definition.id.Value() );

            if( definition == index.definitions.end() )
                throwValidationError( placement.definition.source, wxS( "unresolved placement definition" ) );

            for( const PIN_REFERENCE& pin : placement.pins )
            {
                auto owner = index.pinOwners.find( pin.id.Value() );

                if( owner == index.pinOwners.end() || owner->second != definition->second )
                    throwValidationError( pin.source, wxS( "placement pin does not belong to selected definition" ) );
            }
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

        for( const SHEET_REFERENCE& linkedSheet : label.linkedSheets )
        {
            if( !index.sheets.contains( linkedSheet.id.Value() ) )
                throwValidationError( linkedSheet.source, wxS( "unresolved cross-sheet label reference" ) );
        }
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

    enum class VISIT_STATE : uint8_t
    {
        VISITING,
        COMPLETE
    };
    std::unordered_map<uint32_t, VISIT_STATE> visitStates;

    for( const MODEL_SHEET& sheet : sheets )
    {
        const MODEL_SHEET*              current = &sheet;
        std::vector<const MODEL_SHEET*> path;

        while( current && !visitStates.contains( current->id.Value() ) )
        {
            visitStates.emplace( current->id.Value(), VISIT_STATE::VISITING );
            path.push_back( current );

            if( !current->parent )
            {
                current = nullptr;
                break;
            }

            auto parent = index.sheets.find( current->parent->id.Value() );

            if( parent == index.sheets.end() )
                break;

            current = parent->second;
        }

        if( current && visitStates.at( current->id.Value() ) == VISIT_STATE::VISITING )
        {
            throwValidationError( current->source, wxS( "cyclic sheet hierarchy" ) );
        }

        for( const MODEL_SHEET* visited : path )
            visitStates[visited->id.Value()] = VISIT_STATE::COMPLETE;
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

        model.sheets.push_back( std::move( sheet ) );
    }

    size_t                              sheetIndex = 0;
    std::optional<PLACEMENT_GLOBALS>    placementData;
    std::optional<CONNECTIVITY_GLOBALS> connectivityData;
    const PLACEMENT_LAYOUT&             placementSchema = placementLayout( model.version );

    if( placementSchema.decoded )
    {
        placementData = placementGlobals( sdb, aSourceName );
        connectivityData = connectivityGlobals( aBytes, cursor, sdb, aSourceName, model );
    }

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

        if( !placementSchema.decoded )
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
            decodeDefinitionsAndParts( aBytes, cursor, block, sheetIndex, aSourceName, model );
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

            if( stringOffset > heapBytes )
            {
                SOURCE_PROVENANCE offsetSource = textSource;
                offsetSource.absoluteOffset += 8;
                offsetSource.length = 4;
                throwDecodeError( offsetSource, wxS( "free-text string offset leaves controller 2" ) );
            }

            if( stringBytes == 0 || stringBytes > heapBytes - stringOffset )
            {
                SOURCE_PROVENANCE lengthSource = textSource;
                lengthSource.absoluteOffset += 20;
                lengthSource.length = 2;
                throwDecodeError( lengthSource, wxS( "free-text string length leaves controller 2" ) );
            }

            if( aBytes[heapOffset + stringOffset + stringBytes - 1] != 0 )
            {
                SOURCE_PROVENANCE terminatorSource =
                        sourceAt( aSourceName, model.version, wxS( "free text string terminator" ), 2, record,
                                  heapOffset + stringOffset + stringBytes - 1, 1, static_cast<int>( sheetIndex ) );
                throwDecodeError( terminatorSource, wxS( "free-text string is not NUL terminated" ) );
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

        decodeDefinitionsAndParts( aBytes, cursor, block, sheetIndex, aSourceName, model );
        decodePlacements( aBytes, cursor, block, sheetIndex, aSourceName, *placementData, model );
        decodeConnectivity( aBytes, cursor, block, sheetIndex, aSourceName, *connectivityData, model );
        ++sheetIndex;
    }

    assignFieldIds( model );
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

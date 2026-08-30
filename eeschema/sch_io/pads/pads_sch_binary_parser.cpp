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

#include <sch_io/ole_image.h>

#include <algorithm>
#include <array>
#include <functional>
#include <iterator>
#include <map>
#include <ranges>
#include <set>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include <ki_exception.h>
#include <wx/strconv.h>

namespace PADS_SCH_BINARY
{

namespace
{
    static constexpr std::array<uint16_t, 16> JUSTIFICATION_BY_NIBBLE_0 = { 0, 4, 1, 5, 8,  12, 9,  13,
                                                                            2, 6, 3, 7, 10, 14, 11, 15 };
    static constexpr std::array<uint16_t, 16> JUSTIFICATION_BY_NIBBLE_90 = { 0, 4, 2, 6, 8, 12, 10, 14,
                                                                             1, 5, 3, 7, 9, 13, 11, 15 };


    uint16_t terminalJustification( uint16_t aNibble, bool aRotated )
    {
        return aRotated ? JUSTIFICATION_BY_NIBBLE_90[aNibble] : JUSTIFICATION_BY_NIBBLE_0[aNibble];
    }


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
    constexpr size_t   NET_NAME_RECORD_BYTES = 48;
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
        size_t                         fontBase = 0;
        uint32_t                       fontCount = 0;
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
        size_t   itemVisibility;
        size_t   placedPinOrdinal;
        size_t   customFont;
        size_t   customX;
        size_t   customAngle;
        size_t   customJustification;
        size_t   customAttributeIndex;
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
                                                         .itemVisibility = 0x87,
                                                         .placedPinOrdinal = 4,
                                                         .customFont = 0,
                                                         .customX = 8,
                                                         .customAngle = 12,
                                                         .customJustification = 14,
                                                         .customAttributeIndex = 16,
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


    int64_t decodeDatabaseCoordinate( int32_t aRaw, const SOURCE_PROVENANCE& aSource )
    {
        if( aRaw < std::numeric_limits<int16_t>::min() || aRaw > std::numeric_limits<int16_t>::max() )
            throwDecodeError( aSource, wxS( "embedded OLE database coordinate is not sign-extended 16-bit" ) );

        return decodeCoordinate( static_cast<uint16_t>( static_cast<int16_t>( aRaw ) ) );
    }


    int64_t decodeLocalCoordinate( uint16_t aRaw )
    {
        return static_cast<int64_t>( static_cast<int16_t>( aRaw ) ) * 4;
    }


    int64_t decodeTerminalCoordinate( uint16_t aRaw )
    {
        return static_cast<int64_t>( static_cast<int16_t>( aRaw ) ) * 4;
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


    bool samePointValue( const SOURCE_POINT& aLeft, const SOURCE_POINT& aRight )
    {
        return aLeft.x == aRight.x && aLeft.y == aRight.y;
    }


    bool samePresentationValue( const MODEL_TEXT_PRESENTATION& aLeft, const MODEL_TEXT_PRESENTATION& aRight )
    {
        return aLeft.height == aRight.height && aLeft.width == aRight.width && aLeft.font.text == aRight.font.text
               && aLeft.horizontalJustification == aRight.horizontalJustification
               && aLeft.verticalJustification == aRight.verticalJustification && aLeft.bold == aRight.bold
               && aLeft.italic == aRight.italic && aLeft.underline == aRight.underline
               && aLeft.visible == aRight.visible;
    }


    bool sameGraphicValue( const MODEL_GRAPHIC& aLeft, const MODEL_GRAPHIC& aRight )
    {
        return aLeft.kind == aRight.kind && aLeft.text.text == aRight.text.text && aLeft.lineStyle == aRight.lineStyle
               && aLeft.strokeWidth == aRight.strokeWidth && aLeft.fill == aRight.fill
               && samePresentationValue( aLeft.presentation, aRight.presentation ) && aLeft.angle == aRight.angle
               && aLeft.arcSweepAngle == aRight.arcSweepAngle && aLeft.arcClockwise == aRight.arcClockwise
               && samePointValue( aLeft.arcCenter, aRight.arcCenter )
               && samePointValue( aLeft.arcBoundsStart, aRight.arcBoundsStart )
               && samePointValue( aLeft.arcBoundsEnd, aRight.arcBoundsEnd )
               && std::ranges::equal( aLeft.points, aRight.points, samePointValue );
    }


    bool sameWorksheetValue( const MODEL_WORKSHEET& aLeft, const MODEL_WORKSHEET& aRight )
    {
        std::vector<const MODEL_GRAPHIC*> leftDrawing;
        std::vector<const MODEL_GRAPHIC*> rightDrawing;
        std::vector<const MODEL_GRAPHIC*> leftText;
        std::vector<const MODEL_GRAPHIC*> rightText;

        for( const MODEL_GRAPHIC& graphic : aLeft.graphics )
            ( graphic.kind == MODEL_GRAPHIC_KIND::TEXT ? leftText : leftDrawing ).push_back( &graphic );

        for( const MODEL_GRAPHIC& graphic : aRight.graphics )
            ( graphic.kind == MODEL_GRAPHIC_KIND::TEXT ? rightText : rightDrawing ).push_back( &graphic );

        const auto equalPointers = []( const MODEL_GRAPHIC* aLeftGraphic, const MODEL_GRAPHIC* aRightGraphic )
        {
            return sameGraphicValue( *aLeftGraphic, *aRightGraphic );
        };

        return std::ranges::equal( leftDrawing, rightDrawing, equalPointers )
               && std::ranges::is_permutation( leftText, rightText, equalPointers );
    }


    bool isProvenGraphicStrokeWidth( uint8_t aWidth )
    {
        constexpr std::array<uint16_t, 13> widths{ 1, 2, 5, 7, 8, 10, 11, 15, 20, 25, 30, 31, 40 };
        return std::ranges::binary_search( widths, aWidth );
    }


    void decodeGraphicStrokeWidth( uint8_t aRawWidth, uint8_t aPackedPresentation,
                                   const SOURCE_PROVENANCE& aGraphicSource, MODEL_GRAPHIC& aGraphic,
                                   std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
    {
        SOURCE_PROVENANCE packedSource = aGraphicSource;
        packedSource.absoluteOffset += 5;
        packedSource.length = 1;
        SOURCE_PROPERTY packed = sourceProperty( wxS( "preserved_graphic_presentation" ),
                                                 wxString::Format( wxS( "%u" ), aPackedPresentation ), packedSource );
        packed.disposition = PROPERTY_DISPOSITION::PRESERVED;
        aGraphic.properties.push_back( std::move( packed ) );

        if( isProvenGraphicStrokeWidth( aRawWidth ) )
        {
            aGraphic.strokeWidth = static_cast<int64_t>( aRawWidth ) * 2;
            return;
        }

        SOURCE_PROVENANCE strokeSource = aGraphicSource;
        strokeSource.absoluteOffset += 4;
        strokeSource.length = 1;
        SOURCE_PROPERTY stroke = sourceProperty( wxS( "unsupported_graphic_stroke_width" ),
                                                 wxString::Format( wxS( "%u" ), aRawWidth ), strokeSource );
        stroke.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
        aDiagnostics.push_back( MakePropertyDiagnostic(
                RPT_SEVERITY_WARNING, stroke, wxS( "unproved graphic stroke width preserved; using hairline" ) ) );
        aGraphic.properties.push_back( std::move( stroke ) );
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


    MODEL_JUSTIFICATION horizontalJustification( uint16_t aValue )
    {
        const uint16_t justification = aValue & 0x00FF;
        const uint16_t horizontal = justification >= 8   ? justification - 8
                                    : justification >= 2 ? justification - 2
                                                         : justification;

        switch( horizontal )
        {
        default:
        case 0: return MODEL_JUSTIFICATION::LEFT;
        case 1: return MODEL_JUSTIFICATION::RIGHT;
        case 4: return MODEL_JUSTIFICATION::CENTER;
        }
    }


    MODEL_JUSTIFICATION verticalJustification( uint16_t aValue )
    {
        const uint16_t justification = aValue & 0x00FF;

        if( justification >= 8 )
            return MODEL_JUSTIFICATION::CENTER;

        if( justification >= 2 )
            return MODEL_JUSTIFICATION::LEFT;

        return MODEL_JUSTIFICATION::RIGHT;
    }


    MODEL_JUSTIFICATION freeTextHorizontalJustification( uint16_t aValue )
    {
        switch( aValue & 0x000F )
        {
        case 0:
        case 2:
        case 8: return MODEL_JUSTIFICATION::LEFT;

        case 4:
        case 6:
        case 10:
        case 12:
        case 14: return MODEL_JUSTIFICATION::CENTER;

        default: return MODEL_JUSTIFICATION::RIGHT;
        }
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


    constexpr auto itemId = []( const auto& aItem ) -> const auto&
    {
        return aItem.id;
    };


    constexpr auto itemProvenance = []( const auto& aItem ) -> const SOURCE_PROVENANCE&
    {
        return aItem.source;
    };


    template <typename Item, typename Declarations>
    void validateNestedId( const Item& aItem, const wxString& aObjectClass, Declarations& aDeclarations )
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
    }


    struct MODEL_REFERENCE_INDEX
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

        explicit MODEL_REFERENCE_INDEX( const PADS_SCH_MODEL& aModel )
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


    bool endpointIsValid( const MODEL_REFERENCE_INDEX& aIndex, const MODEL_CONNECTION_ENDPOINT& aEndpoint )
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


    void decodeGlobalFont( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                           const PLACEMENT_GLOBALS& aGlobals, const wxString& aSourceName, uint16_t aVersion,
                           int16_t aHandle, const SOURCE_PROVENANCE& aHandleSource,
                           MODEL_TEXT_PRESENTATION& aPresentation, bool aStrictHandle,
                           std::vector<PARSER_DIAGNOSTIC>& aDiagnostics )
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

            SOURCE_PROPERTY property = sourceProperty(
                    wxS( "inline_font_payload" ), wxString::Format( wxS( "%u" ), uint16_t( aHandle ) ), aHandleSource );
            property.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, property, wxS( "unsupported inline placement font payload preserved" ) ) );
            aPresentation.properties.push_back( std::move( property ) );
            return;
        }

        const size_t      fontOffset = aGlobals.fontBase + static_cast<size_t>( aHandle ) * FONT_RECORD_BYTES;
        SOURCE_PROVENANCE fontSource = sourceAt( aSourceName, aVersion, wxS( "placement font" ), 19, aHandle,
                                                 fontOffset, FONT_RECORD_BYTES, -1 );
        const uint32_t    style = aCursor.U32At( fontOffset );

        SOURCE_PROVENANCE nameSource = fontSource;
        nameSource.absoluteOffset += 4;
        nameSource.length = 32;
        SOURCE_STRING name = decodeFixedString( aBytes, fontOffset + 4, 32, nameSource, aDiagnostics );
        aPresentation.bold = ( style & 2 ) != 0;
        aPresentation.italic = ( style & 1 ) != 0;
        aPresentation.underline = ( style & 4 ) != 0;
        aPresentation.font = name;

        if( aPresentation.bold )
            aPresentation.font.text.Prepend( wxS( "Bold " ) );

        if( aPresentation.italic )
            aPresentation.font.text.Prepend( wxS( "Italic " ) );

        aPresentation.properties.push_back(
                sourceProperty( wxS( "font_handle" ), wxString::Format( wxS( "%d" ), aHandle ), fontSource ) );

        if( ( style & ~uint32_t{ 7 } ) != 0 )
        {
            SOURCE_PROPERTY property = sourceProperty( wxS( "unsupported_font_style_flags" ),
                                                       wxString::Format( wxS( "%u" ), style & ~7U ), fontSource );
            property.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
            aDiagnostics.push_back( MakePropertyDiagnostic(
                    RPT_SEVERITY_WARNING, property, wxS( "unsupported placement font style flags preserved" ) ) );
            aPresentation.properties.push_back( std::move( property ) );
        }
    }


    CONNECTIVITY_GLOBALS connectivityGlobals( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                              const PADS_SCH_SDB& aSdb, const wxString& aSourceName,
                                              PADS_SCH_MODEL& aModel )
    {
        CONNECTIVITY_GLOBALS result;
        const SCH_SDB_POOL&  membershipPool = aSdb.Pools()[4];
        const SCH_SDB_POOL&  netPool = aSdb.Pools()[8];
        const SCH_SDB_POOL&  fontPool = aSdb.Pools()[19];

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

        if( fontPool.usedBytes != fontPool.count * FONT_RECORD_BYTES )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "font directory" ), 19, 0,
                                                 outerControllerOffset( aSdb, 19 ), fontPool.usedBytes, -1 );
            throwDecodeError( source, wxS( "controller byte count does not match 36-byte records" ) );
        }

        result.membershipBase = outerControllerOffset( aSdb, 4 );
        result.membershipCount = membershipPool.count;
        result.fontBase = outerControllerOffset( aSdb, 19 );
        result.fontCount = fontPool.count;
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


    enum class TEXT_ROLE
    {
        DEFINITION_FIELD,
        EMBEDDED_SYMBOL_TEXT,
        PAGE_TEXT
    };


    struct DEFINITION_TEXT_HEAP
    {
        size_t   recordBase = 0;
        uint32_t recordCount = 0;
        size_t   stringBase = 0;
        uint32_t stringBytes = 0;
    };


    MODEL_TEXT_PRESENTATION decodeTextPresentation( const PADS_IO::BINARY_CURSOR& aCursor, size_t aOffset,
                                                    const SOURCE_PROVENANCE& aSource, TEXT_ROLE aRole,
                                                    int16_t& aRelationship )
    {
        MODEL_TEXT_PRESENTATION presentation;
        presentation.source = aSource;
        presentation.height = aCursor.U16At( aOffset + 22 );
        presentation.width = aCursor.U8At( aOffset + 30 );
        presentation.properties.push_back( sourceProperty(
                wxS( "display_flags" ), wxString::Format( wxS( "%u" ), aCursor.U8At( aOffset + 31 ) ), aSource ) );
        presentation.horizontalJustification = horizontalJustification( aCursor.U16At( aOffset + 18 ) );
        presentation.verticalJustification = verticalJustification( aCursor.U16At( aOffset + 18 ) );

        SOURCE_PROVENANCE fontSource = aSource;
        fontSource.absoluteOffset += 28;
        fontSource.length = 2;
        aRelationship = static_cast<int16_t>( aCursor.U16At( aOffset + 28 ) );

        // Page text spends word 28 on the successor ordinal, so it always falls back to the default font.
        const bool pageText = aRole == TEXT_ROLE::PAGE_TEXT;

        presentation.font = decodedDefinitionFont( pageText ? int16_t( -1 ) : aRelationship, fontSource );
        presentation.properties.push_back( sourceProperty( pageText ? wxS( "successor_ordinal" ) : wxS( "font_handle" ),
                                                           wxString::Format( wxS( "%d" ), aRelationship ),
                                                           fontSource ) );
        return presentation;
    }


    void decodeDefinitionTextRecord( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                     const wxString& aSourceName, size_t aSheetIndex, const DEFINITION_TEXT_HEAP& aHeap,
                                     size_t aRecord, TEXT_ROLE aRole, MODEL_SYMBOL_DEFINITION& aDefinition,
                                     PADS_SCH_MODEL& aModel )
    {
        if( aRecord >= aHeap.recordCount )
            throwDecodeError( aDefinition.source, wxS( "definition field handle leaves controller 1" ) );

        const bool        isField = aRole == TEXT_ROLE::DEFINITION_FIELD;
        const size_t      offset = aHeap.recordBase + aRecord * TEXT_RECORD_BYTES;
        SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version,
                                             isField ? wxS( "definition field" ) : wxS( "embedded symbol text" ), 1,
                                             aRecord, offset, TEXT_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
        const uint32_t    stringOffset = aCursor.U32At( offset + 8 );
        const uint16_t    stringBytes = aCursor.U16At( offset + 20 );

        if( stringBytes == 0 || stringOffset > aHeap.stringBytes || stringBytes > aHeap.stringBytes - stringOffset
            || aBytes[aHeap.stringBase + stringOffset + stringBytes - 1] != 0 )
        {
            throwDecodeError( source, wxS( "definition field string leaves controller 2" ) );
        }

        SOURCE_PROVENANCE stringSource =
                sourceAt( aSourceName, aModel.version, source.objectClass + wxS( " string" ), 2, aRecord,
                          aHeap.stringBase + stringOffset, stringBytes - 1, static_cast<int>( aSheetIndex ) );
        SOURCE_STRING string = PADS_SCH_BINARY_PARSER::DecodeString(
                { aBytes.begin() + stringSource.absoluteOffset,
                  aBytes.begin() + stringSource.absoluteOffset + stringSource.length },
                DEFAULT_CODE_PAGE, stringSource, aModel.diagnostics );

        int16_t                 relationship = 0;
        MODEL_TEXT_PRESENTATION presentation = decodeTextPresentation( aCursor, offset, source, aRole, relationship );
        const SOURCE_POINT      position{ decodeLocalCoordinate( aCursor.U16At( offset + 12 ) ),
                                     decodeLocalCoordinate( aCursor.U16At( offset + 14 ) ), source };
        const int               angle = NormalizeAngle( aCursor.U16At( offset + 16 ) );

        if( isField )
        {
            MODEL_FIELD field;
            field.source = source;
            field.name = std::move( string );
            field.position = position;
            field.angle = angle;
            field.presentation = std::move( presentation );
            aDefinition.fields.push_back( std::move( field ) );
            return;
        }

        MODEL_GRAPHIC graphic;
        graphic.source = source;
        graphic.kind = MODEL_GRAPHIC_KIND::TEXT;
        graphic.text = std::move( string );
        graphic.points.push_back( position );
        graphic.presentation = std::move( presentation );
        graphic.angle = angle;
        aDefinition.graphics.push_back( std::move( graphic ) );
    }


    struct DEFINITION_LAYOUT
    {
        size_t   symbolBase = 0;
        size_t   pieceBase = 0;
        size_t   vertexBase = 0;
        size_t   arcBase = 0;
        size_t   usedDecalBase = 0;
        size_t   terminalBase = 0;
        size_t   partBase = 0;
        size_t   gateBase = 0;
        size_t   pinBase = 0;
        size_t   signalPinBase = 0;
        size_t   pinNameBase = 0;
        uint32_t pinNameBytes = 0;
        uint32_t definitionIdBase = 0;
        uint32_t pinIdBase = 0;
        uint32_t partIdBase = 0;
        uint32_t gateIdBase = 0;

        DEFINITION_TEXT_HEAP             textHeap;
        std::vector<uint32_t>            definitionPieceStart;
        std::vector<uint32_t>            pieceVertexStart;
        std::vector<std::vector<size_t>> pieceArcRecords;
    };


    struct USED_DECAL
    {
        size_t                   record = 0;
        uint32_t                 definitionRecord = 0;
        uint16_t                 terminalStart = 0;
        uint8_t                  terminalCount = 0;
        uint32_t                 fieldStart = 0;
        MODEL_SYMBOL_DEFINITION* definition = nullptr;
    };


    void preserveRawDefinitionControllers( const std::vector<uint8_t>& aBytes, const SHEET_CONTROLLERS& aControllers,
                                           size_t aSheetIndex, const wxString& aSourceName, PADS_SCH_MODEL& aModel )
    {
        for( size_t controller = 3; controller <= 23; ++controller )
        {
            const SCH_SDB_POOL& pool = aControllers.pools[controller - 1];

            if( pool.usedBytes == 0 )
                continue;

            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "definition controller" ), controller, 0,
                              aControllers.offsets[controller - 1], pool.usedBytes, static_cast<int>( aSheetIndex ) );
            aModel.preservedControllerPayloads.push_back(
                    { source,
                      PROPERTY_DISPOSITION::PRESERVED,
                      { aBytes.begin() + source.absoluteOffset,
                        aBytes.begin() + source.absoluteOffset + source.length } } );
        }
    }


    DEFINITION_LAYOUT definitionLayout( const PADS_IO::BINARY_CURSOR& aCursor, const SHEET_CONTROLLERS& aControllers,
                                        size_t aSheetIndex, const wxString& aSourceName, uint16_t aVersion )
    {
        requireFixedController( aControllers, 3, SYMBOL_RECORD_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 4, SYMBOL_PIECE_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 5, SYMBOL_VERTEX_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 6, SYMBOL_ARC_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 7, USED_DECAL_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 8, TERMINAL_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 9, PART_TYPE_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 10, GATE_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 11, PIN_BYTES, aSourceName, aVersion, aSheetIndex );
        requireFixedController( aControllers, 12, SIGNAL_PIN_BYTES, aSourceName, aVersion, aSheetIndex );

        DEFINITION_LAYOUT layout;
        layout.symbolBase = aControllers.offsets[2];
        layout.pieceBase = aControllers.offsets[3];
        layout.vertexBase = aControllers.offsets[4];
        layout.arcBase = aControllers.offsets[5];
        layout.usedDecalBase = aControllers.offsets[6];
        layout.terminalBase = aControllers.offsets[7];
        layout.partBase = aControllers.offsets[8];
        layout.gateBase = aControllers.offsets[9];
        layout.pinBase = aControllers.offsets[10];
        layout.signalPinBase = aControllers.offsets[11];
        layout.pinNameBase = aControllers.offsets[13];
        layout.pinNameBytes = aControllers.pools[13].usedBytes;
        layout.definitionIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 1 );
        layout.pinIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x10000 );
        layout.partIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x20000 );
        layout.gateIdBase = static_cast<uint32_t>( aSheetIndex * 0x100000 + 0x30000 );
        layout.textHeap = { aControllers.offsets[0], aControllers.pools[0].count, aControllers.offsets[1],
                            aControllers.pools[1].usedBytes };

        // Prefix sums over file-supplied per-record counts. A 32-bit accumulator wraps on a
        // crafted file and the exact-total checks below then compare small against their pools
        // while the stored offsets are already nonsense, so accumulate in 64 bits
        uint64_t vertexCursor = 0;
        uint64_t pieceCursor = 0;

        for( size_t definition = 0; definition < aControllers.pools[2].count; ++definition )
        {
            layout.definitionPieceStart.push_back( static_cast<uint32_t>( pieceCursor ) );
            pieceCursor += aCursor.U16At( layout.symbolBase + definition * SYMBOL_RECORD_BYTES + 0x2A );
        }

        if( pieceCursor != aControllers.pools[3].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aVersion, wxS( "symbol definition" ), 3, 0, layout.symbolBase,
                              aControllers.pools[2].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "symbol graphic-piece counts leave controller 4" ) );
        }

        for( size_t piece = 0; piece < aControllers.pools[3].count; ++piece )
        {
            layout.pieceVertexStart.push_back( static_cast<uint32_t>( vertexCursor ) );
            vertexCursor += aCursor.U16At( layout.pieceBase + piece * SYMBOL_PIECE_BYTES + 2 );
        }

        if( vertexCursor != aControllers.pools[4].count )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aVersion, wxS( "symbol piece" ), 4, 0, layout.pieceBase,
                                                 aControllers.pools[3].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "symbol piece vertex counts do not consume controller 5" ) );
        }

        for( size_t definition = 0; definition < aControllers.pools[2].count; ++definition )
        {
            const size_t   definitionOffset = layout.symbolBase + definition * SYMBOL_RECORD_BYTES;
            const uint32_t firstPiece = layout.definitionPieceStart[definition];
            const uint32_t pieceEnd = definition + 1 < layout.definitionPieceStart.size()
                                              ? layout.definitionPieceStart[definition + 1]
                                              : aControllers.pools[3].count;
            const uint32_t firstVertex = aCursor.U32At( definitionOffset + 0x34 );
            const uint32_t vertexEnd = definition + 1 < aControllers.pools[2].count
                                               ? aCursor.U32At( definitionOffset + SYMBOL_RECORD_BYTES + 0x34 )
                                               : aControllers.pools[4].count;
            const bool     emptyMatches = firstPiece == pieceEnd && firstVertex == vertexEnd;
            const bool     ownedMatches =
                    firstPiece < pieceEnd && layout.pieceVertexStart[firstPiece] == firstVertex
                    && layout.pieceVertexStart[pieceEnd - 1]
                                       + aCursor.U16At( layout.pieceBase + ( pieceEnd - 1 ) * SYMBOL_PIECE_BYTES + 2 )
                               == vertexEnd;

            if( !emptyMatches && !ownedMatches )
            {
                SOURCE_PROVENANCE source =
                        sourceAt( aSourceName, aVersion, wxS( "symbol definition" ), 3, definition, definitionOffset,
                                  SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
                throwDecodeError( source, wxS( "symbol piece/vertex ownership mismatch" ) );
            }
        }

        layout.pieceArcRecords.resize( aControllers.pools[3].count );
        size_t discoveredArcCount = 0;

        for( size_t piece = 0; piece < aControllers.pools[3].count; ++piece )
        {
            const size_t   pieceOffset = layout.pieceBase + piece * SYMBOL_PIECE_BYTES;
            const uint16_t pointCount = aCursor.U16At( pieceOffset + 2 );

            for( size_t point = 0; point < pointCount; ++point )
            {
                const size_t vertexOffset =
                        layout.vertexBase + ( layout.pieceVertexStart[piece] + point ) * SYMBOL_VERTEX_BYTES;

                if( static_cast<int16_t>( aCursor.U16At( vertexOffset + 4 ) ) >= 0 )
                    layout.pieceArcRecords[piece].push_back( discoveredArcCount++ );
            }
        }

        if( discoveredArcCount != aControllers.pools[5].count )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aVersion, wxS( "symbol arc" ), 6, discoveredArcCount, layout.arcBase,
                              aControllers.pools[5].usedBytes, static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "arc markers do not consume controller 6" ) );
        }

        return layout;
    }


    void decodeSymbolDefinitions( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                  const SHEET_CONTROLLERS& aControllers, const DEFINITION_LAYOUT& aLayout,
                                  size_t aSheetIndex, const wxString& aSourceName,
                                  std::vector<MODEL_SYMBOL_DEFINITION*>& aDefinitionsByRecord,
                                  std::vector<size_t>& aPageGraphicRecords, PADS_SCH_MODEL& aModel )
    {
        // Definitions must not reallocate; aDefinitionsByRecord holds pointers into them
        aModel.definitions.reserve( aModel.definitions.size() + aControllers.pools[2].count );

        for( size_t record = 0; record < aControllers.pools[2].count; ++record )
        {
            const size_t      offset = aLayout.symbolBase + record * SYMBOL_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "symbol definition" ), 3, record,
                                                 offset, SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 38;
            SOURCE_STRING  name = decodeFixedString( aBytes, offset, 38, nameSource, aModel.diagnostics );
            const uint8_t  objectClass = aCursor.U8At( offset + 0x29 );
            const uint32_t vertexStart = aCursor.U32At( offset + 0x34 );
            const uint32_t vertexEnd = record + 1 < aControllers.pools[2].count
                                               ? aCursor.U32At( offset + SYMBOL_RECORD_BYTES + 0x34 )
                                               : aControllers.pools[4].count;

            if( vertexStart > vertexEnd || vertexEnd > aControllers.pools[4].count )
                throwDecodeError( source, wxS( "symbol definition vertex slice leaves controller 5" ) );

            if( objectClass == 0 )
            {
                const uint32_t firstPiece = aLayout.definitionPieceStart[record];
                const uint32_t pieceEnd = record + 1 < aLayout.definitionPieceStart.size()
                                                  ? aLayout.definitionPieceStart[record + 1]
                                                  : aControllers.pools[3].count;
                const uint16_t groupTextCount = aCursor.U16At( offset + 64 );
                const uint16_t groupLastText = aCursor.U16At( offset + 66 );
                const bool     worksheetCandidate = pieceEnd - firstPiece == 69 && groupTextCount == 58;
                const bool     hasCircularTextOwnership = groupTextCount == 0
                                                      || ( groupTextCount <= aLayout.textHeap.recordCount
                                                           && groupLastText < aLayout.textHeap.recordCount );
                SOURCE_PROVENANCE originSource = source;
                originSource.objectClass = wxS( "page graphic group origin" );
                originSource.absoluteOffset += 60;
                originSource.length = 4;
                const SOURCE_POINT groupOrigin{ decodeCoordinate( aCursor.U16At( offset + 60 ) ),
                                                decodeCoordinate( aCursor.U16At( offset + 62 ) ), originSource };

                if( aCursor.U16At( offset + 42 ) != pieceEnd - firstPiece )
                    throwDecodeError( source, wxS( "page-graphic piece count does not match controller 4 slice" ) );

                for( size_t piece = firstPiece; piece < pieceEnd; ++piece )
                {
                    const size_t      pieceOffset = aLayout.pieceBase + piece * SYMBOL_PIECE_BYTES;
                    const uint8_t     pieceKind = aCursor.U8At( pieceOffset );
                    const uint8_t     lineStyle = aCursor.U8At( pieceOffset + 1 );
                    const uint16_t    pointCount = aCursor.U16At( pieceOffset + 2 );
                    const uint32_t    firstVertex = aLayout.pieceVertexStart[piece];
                    SOURCE_PROVENANCE graphicSource =
                            sourceAt( aSourceName, aModel.version, wxS( "page graphic" ), 4, piece, pieceOffset,
                                      SYMBOL_PIECE_BYTES, static_cast<int>( aSheetIndex ) );

                    if( firstVertex + pointCount > vertexEnd )
                        throwDecodeError( graphicSource, wxS( "page graphic crosses its controller-5 slice" ) );

                    MODEL_GRAPHIC graphic;
                    graphic.source = graphicSource;
                    decodeGraphicStrokeWidth( aCursor.U8At( pieceOffset + 4 ), aCursor.U8At( pieceOffset + 5 ),
                                              graphicSource, graphic, aModel.diagnostics );

                    switch( lineStyle )
                    {
                    case 0xFF: graphic.lineStyle = MODEL_LINE_STYLE::SOLID; break;
                    case 0: graphic.lineStyle = MODEL_LINE_STYLE::DASH; break;
                    case 1: graphic.lineStyle = MODEL_LINE_STYLE::DOT; break;
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
                        const size_t pointOffset = aLayout.vertexBase + ( firstVertex + point ) * SYMBOL_VERTEX_BYTES;
                        SOURCE_PROVENANCE pointSource = sourceAt(
                                aSourceName, aModel.version, wxS( "page graphic vertex" ), 5, firstVertex + point,
                                pointOffset, SYMBOL_VERTEX_BYTES, static_cast<int>( aSheetIndex ) );
                        graphic.points.push_back( { decodeLocalCoordinate( aCursor.U16At( pointOffset ) ),
                                                    decodeLocalCoordinate( aCursor.U16At( pointOffset + 2 ) ),
                                                    pointSource } );
                    }

                    const int16_t arcMarker = static_cast<int16_t>(
                            aCursor.U16At( aLayout.vertexBase + firstVertex * SYMBOL_VERTEX_BYTES + 4 ) );

                    if( pieceKind == 0 && arcMarker >= 0 )
                    {
                        if( aLayout.pieceArcRecords[piece].empty() )
                            throwDecodeError( graphicSource, wxS( "page arc has no controller-6 record" ) );

                        const size_t      arcRecord = aLayout.pieceArcRecords[piece].front();
                        const size_t      arcOffset = aLayout.arcBase + arcRecord * SYMBOL_ARC_BYTES;
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

                    for( SOURCE_POINT& point : graphic.points )
                    {
                        point.x += groupOrigin.x;
                        point.y += groupOrigin.y;
                    }

                    if( graphic.kind == MODEL_GRAPHIC_KIND::ARC )
                    {
                        graphic.arcBoundsStart.x += groupOrigin.x;
                        graphic.arcBoundsStart.y += groupOrigin.y;
                        graphic.arcBoundsEnd.x += groupOrigin.x;
                        graphic.arcBoundsEnd.y += groupOrigin.y;
                        graphic.arcCenter.x += groupOrigin.x;
                        graphic.arcCenter.y += groupOrigin.y;
                    }

                    graphic.properties.push_back( sourceProperty( wxS( "page_graphic_group" ), name.text, source ) );

                    if( worksheetCandidate )
                        graphic.properties.push_back( sourceProperty( wxS( "worksheet_group" ), name.text, source ) );

                    if( !hasCircularTextOwnership )
                    {
                        SOURCE_PROVENANCE relationshipSource = source;
                        relationshipSource.absoluteOffset += 64;
                        relationshipSource.length = 4;
                        SOURCE_PROPERTY relationship = sourceProperty(
                                wxS( "preserved_drawing_text_relationship" ),
                                wxString::Format( wxS( "%u,%u" ), groupTextCount, groupLastText ), relationshipSource );
                        relationship.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                        aModel.diagnostics.push_back( MakePropertyDiagnostic(
                                RPT_SEVERITY_WARNING, relationship,
                                wxS( "unsupported class-zero drawing text relationship preserved" ) ) );
                        graphic.properties.push_back( std::move( relationship ) );
                    }
                    aModel.graphics.push_back(
                            { graphicSource, { aModel.sheets[aSheetIndex].id, graphicSource }, std::move( graphic ) } );
                }

                if( hasCircularTextOwnership )
                    aPageGraphicRecords.push_back( record );
                continue;
            }

            if( objectClass != 0x06 )
                continue;

            MODEL_SYMBOL_DEFINITION definition;
            definition.id = DEFINITION_ID( aLayout.definitionIdBase + record );
            definition.source = source;
            definition.name = std::move( name );
            const uint32_t firstPiece = aLayout.definitionPieceStart[record];
            const uint32_t pieceEnd = record + 1 < aLayout.definitionPieceStart.size()
                                              ? aLayout.definitionPieceStart[record + 1]
                                              : aControllers.pools[3].count;

            for( size_t piece = firstPiece; piece < pieceEnd; ++piece )
            {
                const size_t   pieceOffset = aLayout.pieceBase + piece * SYMBOL_PIECE_BYTES;
                const uint8_t  pieceKind = aCursor.U8At( pieceOffset );
                const uint16_t pointCount = aCursor.U16At( pieceOffset + 2 );
                const uint32_t firstVertex = aLayout.pieceVertexStart[piece];

                if( firstVertex + pointCount > vertexEnd )
                    throwDecodeError( source, wxS( "symbol piece crosses its definition vertex slice" ) );

                SOURCE_PROVENANCE graphicSource =
                        sourceAt( aSourceName, aModel.version, wxS( "symbol graphic" ), 4, piece, pieceOffset,
                                  SYMBOL_PIECE_BYTES, static_cast<int>( aSheetIndex ) );
                MODEL_GRAPHIC graphic;
                graphic.source = graphicSource;
                decodeGraphicStrokeWidth( aCursor.U8At( pieceOffset + 4 ), aCursor.U8At( pieceOffset + 5 ),
                                          graphicSource, graphic, aModel.diagnostics );
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
                    const size_t      pointOffset = aLayout.vertexBase + ( firstVertex + point ) * SYMBOL_VERTEX_BYTES;
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

                const int16_t arcMarker = static_cast<int16_t>(
                        aCursor.U16At( aLayout.vertexBase + firstVertex * SYMBOL_VERTEX_BYTES + 4 ) );

                if( pieceKind == 0 && arcMarker >= 0 )
                {
                    if( aLayout.pieceArcRecords[piece].empty() )
                        throwDecodeError( graphicSource, wxS( "symbol arc has no controller-6 record" ) );

                    const size_t arcRecord = aLayout.pieceArcRecords[piece].front();
                    const size_t arcOffset = aLayout.arcBase + arcRecord * SYMBOL_ARC_BYTES;
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
            aDefinitionsByRecord[record] = &aModel.definitions.back();
        }
    }


    void decodeUsedDecals( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                           const SHEET_CONTROLLERS& aControllers, const DEFINITION_LAYOUT& aLayout, size_t aSheetIndex,
                           const wxString&                              aSourceName,
                           const std::vector<MODEL_SYMBOL_DEFINITION*>& aDefinitionsByRecord,
                           std::vector<USED_DECAL>& aUsedDecals, std::vector<USED_DECAL*>& aSemanticDecals,
                           PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aUsedDecals.size(); ++record )
        {
            const size_t      offset = aLayout.usedDecalBase + record * USED_DECAL_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "used decal" ), 7, record, offset,
                                                 USED_DECAL_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 40;
            SOURCE_STRING name = decodeFixedString( aBytes, offset, 40, nameSource, aModel.diagnostics );
            USED_DECAL&   decal = aUsedDecals[record];
            decal.record = record;
            decal.terminalCount = aCursor.U8At( offset + 42 );
            decal.terminalStart = aCursor.U16At( offset + 44 );
            decal.definitionRecord = aCursor.U32At( offset + 48 );
            decal.fieldStart = aCursor.U32At( offset + 52 );

            if( name.text.empty() || decal.definitionRecord == 0xFFFFFFFF )
                continue;

            if( decal.definitionRecord >= aDefinitionsByRecord.size() )
                throwDecodeError( source, wxS( "unresolved symbol definition reference" ) );

            const size_t      definitionOffset = aLayout.symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES;
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

            decal.definition = aDefinitionsByRecord[decal.definitionRecord];

            if( !decal.definition )
                continue;

            const uint16_t embeddedTextCount =
                    aCursor.U16At( aLayout.symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES + 0x40 );

            if( embeddedTextCount != 0 && decal.fieldStart > aLayout.textHeap.recordCount
                && decal.fieldStart < 0x80000000 )
                throwDecodeError( source, wxS( "embedded definition text handle leaves controller 1" ) );

            if( static_cast<uint32_t>( decal.terminalStart ) + decal.terminalCount > aControllers.pools[7].count )
                throwDecodeError( source, wxS( "used-decal terminal slice leaves controller 8" ) );

            aSemanticDecals.push_back( &decal );

            for( size_t pin = 0; pin < decal.terminalCount; ++pin )
            {
                const size_t      terminalRecord = decal.terminalStart + pin;
                const size_t      terminalOffset = aLayout.terminalBase + terminalRecord * TERMINAL_BYTES;
                SOURCE_PROVENANCE pinSource =
                        sourceAt( aSourceName, aModel.version, wxS( "symbol pin" ), 8, terminalRecord, terminalOffset,
                                  TERMINAL_BYTES, static_cast<int>( aSheetIndex ) );
                const uint16_t       pinDecalHandle = aCursor.U16At( terminalOffset );
                MODEL_PIN_DEFINITION definitionPin;
                definitionPin.id = PIN_ID( aLayout.pinIdBase + terminalRecord );
                definitionPin.source = pinSource;
                definitionPin.position = { decodeTerminalCoordinate( aCursor.U16At( terminalOffset + 2 ) ),
                                           decodeTerminalCoordinate( aCursor.U16At( terminalOffset + 4 ) ), pinSource };
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
                definitionPin.nameOffset = { decodeLocalCoordinate( aCursor.U16At( terminalOffset + 14 ) ),
                                             decodeLocalCoordinate( aCursor.U16At( terminalOffset + 16 ) ), pinSource };
                definitionPin.numberOffset = { decodeLocalCoordinate( aCursor.U16At( terminalOffset + 18 ) ),
                                               decodeLocalCoordinate( aCursor.U16At( terminalOffset + 20 ) ),
                                               pinSource };
                const uint16_t presentationFlags = aCursor.U16At( terminalOffset + 22 );
                const uint16_t visibilityFlags = aCursor.U16At( terminalOffset + 24 );
                const uint16_t side = presentationFlags & 0x0006;
                definitionPin.presentationFlags = presentationFlags;
                definitionPin.visibilityAndNumberPresentationFlags = visibilityFlags;
                definitionPin.side = side / 2;

                definitionPin.angle = ( presentationFlags & 0x0001 ) != 0 ? 900 : 0;
                definitionPin.nameAngle = ( presentationFlags & 0x0100 ) != 0 ? 900 : 0;
                definitionPin.numberAngle = ( visibilityFlags & 0x0001 ) != 0 ? 900 : 0;

                definitionPin.nameJustification =
                        terminalJustification( ( presentationFlags >> 12 ) & 0x0F, definitionPin.nameAngle != 0 );

                if( ( presentationFlags & 0x00F8 ) != 0 )
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "terminal side" ), presentationFlags, pinSource,
                                                               aModel.diagnostics );

                definitionPin.numberJustification =
                        terminalJustification( ( visibilityFlags >> 4 ) & 0x0F, definitionPin.numberAngle != 0 );
                const uint16_t nameOffsetFlags = visibilityFlags & 0x0F00;
                definitionPin.nameOffsetAngle = ( nameOffsetFlags & 0x0100 ) != 0 ? 900 : 0;
                definitionPin.numberOffsetAngle = ( nameOffsetFlags & 0x0200 ) != 0 ? 900 : 0;

                switch( visibilityFlags & 0x0F00 )
                {
                case 0x0000:
                case 0x0800: definitionPin.nameOffsetJustification = 0; break;
                case 0x0400:
                case 0x0C00: definitionPin.nameOffsetJustification = 1; break;
                case 0x0500:
                case 0x0D00:
                case 0x0F00: definitionPin.nameOffsetJustification = 2; break;
                default:
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "terminal name-offset presentation" ),
                                                               visibilityFlags & 0x0F00, pinSource,
                                                               aModel.diagnostics );
                    break;
                }

                switch( visibilityFlags & 0x0F00 )
                {
                case 0x0000: definitionPin.numberOffsetJustification = 0; break;
                case 0x0800:
                case 0x0C00: definitionPin.numberOffsetJustification = 1; break;
                case 0x0D00: definitionPin.numberOffsetJustification = 8; break;
                case 0x0F00: definitionPin.numberOffsetJustification = 2; break;
                default:
                    PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "terminal number-offset presentation" ),
                                                               visibilityFlags & 0x0F00, pinSource,
                                                               aModel.diagnostics );
                    break;
                }

                definitionPin.visibilityFlags = ( visibilityFlags >> 8 ) & 0x00C0;

                if( pinDecalHandle == 0xFFFF )
                {
                    definitionPin.length = 0;
                }
                else if( pinDecalHandle >= aUsedDecals.size() )
                    throwDecodeError( pinSource, wxS( "unresolved pin-decal handle" ) );

                if( pinDecalHandle != 0xFFFF )
                {
                    MODEL_SYMBOL_DEFINITION* pinDecal = aUsedDecals[pinDecalHandle].definition;

                    if( !pinDecal )
                    {
                        const size_t   handleOffset = aLayout.usedDecalBase + pinDecalHandle * USED_DECAL_BYTES;
                        const uint32_t definitionRecord = aCursor.U32At( handleOffset + 48 );

                        if( definitionRecord >= aDefinitionsByRecord.size() || !aDefinitionsByRecord[definitionRecord] )
                            throwDecodeError( pinSource, wxS( "unresolved pin-decal handle" ) );

                        pinDecal = aDefinitionsByRecord[definitionRecord];
                    }

                    const wxString pinDecalName = pinDecal->name.text;
                    definitionPin.decalName = pinDecal->name;

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
                        definitionPin.length = provenLength;
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
    }


    std::vector<USED_DECAL*> buildFieldDecals( const std::vector<USED_DECAL*>& aSemanticDecals,
                                               const DEFINITION_LAYOUT& aLayout, size_t aSheetIndex,
                                               const wxString& aSourceName, uint16_t aVersion )
    {
        std::vector<USED_DECAL*> fieldDecals;

        std::ranges::copy_if( aSemanticDecals, std::back_inserter( fieldDecals ),
                              [&]( const USED_DECAL* aDecal )
                              {
                                  return aDecal->fieldStart < aLayout.textHeap.recordCount;
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
                    sourceAt( aSourceName, aVersion, wxS( "used decal" ), 7, fieldDecals[i - 1]->record,
                              aLayout.usedDecalBase + fieldDecals[i - 1]->record * USED_DECAL_BYTES, USED_DECAL_BYTES,
                              static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE duplicate =
                    sourceAt( aSourceName, aVersion, wxS( "used decal" ), 7, fieldDecals[i]->record,
                              aLayout.usedDecalBase + fieldDecals[i]->record * USED_DECAL_BYTES, USED_DECAL_BYTES,
                              static_cast<int>( aSheetIndex ) );
            throwDecodeError(
                    duplicate,
                    wxString::Format( wxS( "duplicate field ID; first at v0x%04X %s controller %d record %llu "
                                           "sheet %d offset 0x%llX" ),
                                      first.version, first.objectClass, first.controller,
                                      static_cast<unsigned long long>( first.recordIndex ), first.sheet,
                                      static_cast<unsigned long long>( first.absoluteOffset ) ) );
        }

        return fieldDecals;
    }


    void decodePageGraphics( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                             const DEFINITION_LAYOUT& aLayout, size_t aSheetIndex, const wxString& aSourceName,
                             const std::vector<size_t>& aPageGraphicRecords, PADS_SCH_MODEL& aModel )
    {
        for( size_t record : aPageGraphicRecords )
        {
            const size_t      offset = aLayout.symbolBase + record * SYMBOL_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "page graphic group" ), 3, record,
                                                 offset, SYMBOL_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 38;
            SOURCE_STRING  groupName = decodeFixedString( aBytes, offset, 38, nameSource, aModel.diagnostics );
            const uint16_t textCountForGroup = aCursor.U16At( offset + 64 );
            const uint16_t lastTextRecord = aCursor.U16At( offset + 66 );
            const bool     worksheetCandidate = aCursor.U16At( offset + 42 ) == 69 && textCountForGroup == 58;

            if( textCountForGroup > aLayout.textHeap.recordCount
                || ( textCountForGroup != 0 && lastTextRecord >= aLayout.textHeap.recordCount ) )
                throwDecodeError( source, wxS( "page-text ownership leaves controller 1" ) );

            MODEL_SYMBOL_DEFINITION textOwner;
            textOwner.source = source;

            if( textCountForGroup != 0 )
            {
                std::vector<size_t> textRecords( textCountForGroup );
                std::vector<bool>   visitedTextRecords( aLayout.textHeap.recordCount, false );
                size_t              textRecord = lastTextRecord;

                for( size_t reverseIndex = textCountForGroup; reverseIndex != 0; --reverseIndex )
                {
                    if( textRecord >= aLayout.textHeap.recordCount )
                        throwDecodeError( source, wxS( "page-text predecessor leaves controller 1" ) );

                    if( visitedTextRecords[textRecord] )
                        throwDecodeError( source, wxS( "page-text predecessor repeats controller 1 record" ) );

                    visitedTextRecords[textRecord] = true;

                    textRecords[reverseIndex - 1] = textRecord;

                    if( reverseIndex != 1 )
                    {
                        const size_t predecessor =
                                aCursor.U16At( aLayout.textHeap.recordBase + textRecord * TEXT_RECORD_BYTES + 24 );

                        if( predecessor >= aLayout.textHeap.recordCount )
                            throwDecodeError( source, wxS( "page-text predecessor leaves controller 1" ) );

                        textRecord = predecessor;
                    }
                }

                for( size_t textRecordIndex : textRecords )
                    decodeDefinitionTextRecord( aBytes, aCursor, aSourceName, aSheetIndex, aLayout.textHeap,
                                                textRecordIndex, TEXT_ROLE::PAGE_TEXT, textOwner, aModel );
            }

            if( textOwner.graphics.size() != textCountForGroup )
                throwDecodeError( source, wxS( "page-text records do not exactly match declared count" ) );

            for( MODEL_GRAPHIC& graphic : textOwner.graphics )
            {
                graphic.source.objectClass = wxS( "page text" );
                const SOURCE_POINT groupOrigin{ decodeCoordinate( aCursor.U16At( offset + 60 ) ),
                                                decodeCoordinate( aCursor.U16At( offset + 62 ) ), source };

                for( SOURCE_POINT& point : graphic.points )
                {
                    point.x += groupOrigin.x;
                    point.y += groupOrigin.y;
                }

                graphic.properties.push_back( sourceProperty( wxS( "page_graphic_group" ), groupName.text, source ) );

                if( worksheetCandidate )
                    graphic.properties.push_back( sourceProperty( wxS( "worksheet_group" ), groupName.text, source ) );

                aModel.graphics.push_back(
                        { graphic.source, { aModel.sheets[aSheetIndex].id, graphic.source }, std::move( graphic ) } );
            }

            auto belongsToGroup = [&]( const MODEL_PAGE_GRAPHIC& aGraphic )
            {
                if( aGraphic.sheet.id != aModel.sheets[aSheetIndex].id )
                    return false;

                return std::ranges::any_of( aGraphic.graphic.properties,
                                            [&]( const SOURCE_PROPERTY& aProperty )
                                            {
                                                return aProperty.name.text == wxS( "page_graphic_group" )
                                                       && aProperty.value.text == groupName.text;
                                            } );
            };

            size_t numericEdgeMarkers = 0;
            size_t alphabeticEdgeMarkers = 0;
            size_t textGraphics = 0;
            size_t drawingGraphics = 0;
            bool   hasTitleAnchor = false;
            bool   hasSheetAnchor = false;
            bool   hasRevisionAnchor = false;

            for( const MODEL_PAGE_GRAPHIC& pageGraphic : aModel.graphics )
            {
                if( !belongsToGroup( pageGraphic ) )
                    continue;

                if( pageGraphic.graphic.kind != MODEL_GRAPHIC_KIND::TEXT )
                {
                    ++drawingGraphics;
                    continue;
                }

                ++textGraphics;
                wxString text = pageGraphic.graphic.text.text.Upper();

                if( text.length() == 1 && text[0] >= '0' && text[0] <= '9' )
                    ++numericEdgeMarkers;

                if( text.length() == 1 && text[0] >= 'A' && text[0] <= 'Z' )
                    ++alphabeticEdgeMarkers;

                hasTitleAnchor |= text == wxS( "TITLE" ) || text.StartsWith( wxS( "TITLE:" ) );
                hasSheetAnchor |= text == wxS( "SHEET NUMBER" ) || text == wxS( "NUMBER OF SHEETS" )
                                  || text.StartsWith( wxS( "SHEET:" ) );
                hasRevisionAnchor |= text == wxS( "REVISION" ) || text.StartsWith( wxS( "REV:" ) )
                                     || text.StartsWith( wxS( "REVISION " ) );
            }

            const bool worksheet = drawingGraphics >= 30 && textGraphics >= 30 && numericEdgeMarkers >= 4
                                   && alphabeticEdgeMarkers >= 4 && hasTitleAnchor && hasSheetAnchor
                                   && hasRevisionAnchor;

            if( worksheet )
            {
                MODEL_WORKSHEET modelWorksheet;
                modelWorksheet.source = source;
                modelWorksheet.sheet = { aModel.sheets[aSheetIndex].id, source };
                modelWorksheet.name = groupName;

                for( auto graphic = aModel.graphics.begin(); graphic != aModel.graphics.end(); )
                {
                    if( belongsToGroup( *graphic ) )
                    {
                        modelWorksheet.graphics.push_back( std::move( graphic->graphic ) );
                        graphic = aModel.graphics.erase( graphic );
                    }
                    else
                    {
                        ++graphic;
                    }
                }

                auto existing = std::ranges::find_if( aModel.worksheets,
                                                      [&]( const MODEL_WORKSHEET& aWorksheet )
                                                      {
                                                          return aWorksheet.sheet.id == modelWorksheet.sheet.id;
                                                      } );

                if( existing == aModel.worksheets.end() )
                {
                    aModel.worksheets.push_back( std::move( modelWorksheet ) );
                }
                else if( !sameWorksheetValue( *existing, modelWorksheet ) )
                {
                    SOURCE_PROPERTY distinct =
                            sourceProperty( wxS( "distinct_worksheet_layout" ), groupName.text, source );
                    distinct.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                    aModel.diagnostics.push_back( MakePropertyDiagnostic(
                            RPT_SEVERITY_WARNING, distinct,
                            wxS( "distinct worksheet layout preserved as schematic page graphics" ) ) );

                    for( MODEL_GRAPHIC& graphic : modelWorksheet.graphics )
                    {
                        graphic.properties.push_back( distinct );
                        aModel.graphics.push_back( { graphic.source,
                                                     { aModel.sheets[aSheetIndex].id, graphic.source },
                                                     std::move( graphic ) } );
                    }
                }
            }
        }

        auto canonicalWorksheet = std::ranges::find_if( aModel.worksheets,
                                                        []( const MODEL_WORKSHEET& aWorksheet )
                                                        {
                                                            return aWorksheet.name.text == wxS( "DRW5982" );
                                                        } );

        if( canonicalWorksheet != aModel.worksheets.end() )
        {
            using GROUP_KEY = std::pair<uint32_t, wxString>;
            std::map<GROUP_KEY, std::vector<const MODEL_GRAPHIC*>> candidates;
            std::vector<const MODEL_GRAPHIC*>                      canonicalGeometry;

            for( const MODEL_GRAPHIC& graphic : canonicalWorksheet->graphics )
            {
                if( graphic.kind != MODEL_GRAPHIC_KIND::TEXT )
                    canonicalGeometry.push_back( &graphic );
            }

            for( const MODEL_PAGE_GRAPHIC& pageGraphic : aModel.graphics )
            {
                auto groupProperty = std::ranges::find_if( pageGraphic.graphic.properties,
                                                           []( const SOURCE_PROPERTY& aProperty )
                                                           {
                                                               return aProperty.name.text == wxS( "worksheet_group" );
                                                           } );

                if( groupProperty != pageGraphic.graphic.properties.end()
                    && pageGraphic.graphic.kind != MODEL_GRAPHIC_KIND::TEXT )
                {
                    candidates[{ pageGraphic.sheet.id.Value(), groupProperty->value.text }].push_back(
                            &pageGraphic.graphic );
                }
            }

            std::set<GROUP_KEY> equivalentGroups;

            for( const auto& [key, geometry] : candidates )
            {
                if( geometry.size() == canonicalGeometry.size()
                    && std::ranges::equal( geometry, canonicalGeometry,
                                           [&]( const MODEL_GRAPHIC* aLeft, const MODEL_GRAPHIC* aRight )
                                           {
                                               return sameGraphicValue( *aLeft, *aRight );
                                           } ) )
                {
                    equivalentGroups.insert( key );
                }
            }

            std::erase_if( aModel.graphics,
                           [&]( const MODEL_PAGE_GRAPHIC& aGraphic )
                           {
                               return std::ranges::any_of(
                                       aGraphic.graphic.properties,
                                       [&]( const SOURCE_PROPERTY& aProperty )
                                       {
                                           return aProperty.name.text == wxS( "worksheet_group" )
                                                  && equivalentGroups.contains(
                                                          { aGraphic.sheet.id.Value(), aProperty.value.text } );
                                       } );
                           } );
        }
    }


    void decodeDefinitionFields( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                 const DEFINITION_LAYOUT& aLayout, size_t aSheetIndex, const wxString& aSourceName,
                                 const std::vector<USED_DECAL*>& aFieldDecals, PADS_SCH_MODEL& aModel )
    {
        for( size_t i = 0; i < aFieldDecals.size(); ++i )
        {
            USED_DECAL&    decal = *aFieldDecals[i];
            const size_t   definitionOffset = aLayout.symbolBase + decal.definitionRecord * SYMBOL_RECORD_BYTES;
            const uint16_t embeddedCount = aCursor.U16At( definitionOffset + 0x40 );
            const uint32_t fieldEnd =
                    i + 1 < aFieldDecals.size() ? aFieldDecals[i + 1]->fieldStart : aLayout.textHeap.recordCount;

            if( decal.fieldStart > fieldEnd )
                throwDecodeError( decal.definition->source, wxS( "definition field slice is not monotone" ) );

            if( decal.definition->fields.empty() )
            {
                for( size_t standard = 0; standard < 2; ++standard )
                {
                    const size_t      usedOffset = aLayout.usedDecalBase + decal.record * USED_DECAL_BYTES;
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
                    field.presentation.height = aCursor.U16At( usedOffset + 88 + standard * 2 );
                    field.presentation.width = aCursor.U8At( usedOffset + 96 + standard );
                    SOURCE_PROVENANCE fontSource = fieldSource;
                    fontSource.absoluteOffset = usedOffset + 100 + standard * 2;
                    fontSource.length = 2;
                    const int16_t fontHandle = static_cast<int16_t>( aCursor.U16At( fontSource.absoluteOffset ) );
                    field.presentation.font = decodedDefinitionFont( fontHandle, fontSource );
                    field.presentation.properties.push_back( sourceProperty(
                            wxS( "font_handle" ), wxString::Format( wxS( "%d" ), fontHandle ), fontSource ) );
                    const uint16_t justification = aCursor.U16At( usedOffset + 66 + standard * 8 );
                    field.presentation.horizontalJustification = horizontalJustification( justification );
                    field.presentation.verticalJustification = verticalJustification( justification );
                    decal.definition->fields.push_back( std::move( field ) );
                }
            }

            if( embeddedCount != 0 )
            {
                const uint16_t lastEmbeddedRecord = aCursor.U16At( definitionOffset + 0x42 );

                if( embeddedCount > aLayout.textHeap.recordCount || lastEmbeddedRecord >= aLayout.textHeap.recordCount )
                    throwDecodeError( decal.definition->source,
                                      wxS( "embedded definition text ownership leaves controller 1" ) );

                const size_t firstEmbeddedRecord =
                        ( static_cast<size_t>( lastEmbeddedRecord ) + aLayout.textHeap.recordCount + 1 - embeddedCount )
                        % aLayout.textHeap.recordCount;

                for( size_t textIndex = 0; textIndex < embeddedCount; ++textIndex )
                {
                    decodeDefinitionTextRecord( aBytes, aCursor, aSourceName, aSheetIndex, aLayout.textHeap,
                                                ( firstEmbeddedRecord + textIndex ) % aLayout.textHeap.recordCount,
                                                TEXT_ROLE::EMBEDDED_SYMBOL_TEXT, *decal.definition, aModel );
                }
            }

            for( size_t record = decal.fieldStart; record < fieldEnd; ++record )
                decodeDefinitionTextRecord( aBytes, aCursor, aSourceName, aSheetIndex, aLayout.textHeap, record,
                                            TEXT_ROLE::DEFINITION_FIELD, *decal.definition, aModel );
        }
    }


    void decodePartTypes( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                          const SHEET_CONTROLLERS& aControllers, const DEFINITION_LAYOUT& aLayout, size_t aSheetIndex,
                          const wxString& aSourceName, const std::vector<USED_DECAL>& aUsedDecals,
                          PADS_SCH_MODEL& aModel )
    {
        size_t signalPinCursor = 0;

        for( size_t record = 0; record < aControllers.pools[8].count; ++record )
        {
            const size_t      offset = aLayout.partBase + record * PART_TYPE_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "part type" ), 9, record, offset,
                                                 PART_TYPE_BYTES, static_cast<int>( aSheetIndex ) );
            SOURCE_PROVENANCE nameSource = source;
            nameSource.length = 44;
            MODEL_PART_TYPE part;
            part.id = PART_TYPE_ID( aLayout.partIdBase + record );
            part.source = source;
            part.name = decodeFixedString( aBytes, offset, 44, nameSource, aModel.diagnostics );
            const uint32_t gateStart = aCursor.U32At( offset + 44 );
            const uint32_t pinStart = aCursor.U32At( offset + 48 );
            const uint32_t gateEnd = record + 1 < aControllers.pools[8].count
                                             ? aCursor.U32At( offset + PART_TYPE_BYTES + 44 )
                                             : aControllers.pools[9].count;
            const uint32_t pinEnd = record + 1 < aControllers.pools[8].count
                                            ? aCursor.U32At( offset + PART_TYPE_BYTES + 48 )
                                            : aControllers.pools[10].count;

            if( gateStart > gateEnd || gateEnd > aControllers.pools[9].count || pinStart > pinEnd
                || pinEnd > aControllers.pools[10].count )
            {
                throwDecodeError( source, wxS( "part-type gate or pin slice leaves its controller" ) );
            }

            if( aCursor.U16At( offset + 68 ) != gateEnd - gateStart )
                throwDecodeError( source, wxS( "stored gate count does not match controller-10 slice" ) );

            size_t         partPinCursor = pinStart;
            uint32_t       unitCursor = 0;
            const uint32_t pinNameHeapBase = aCursor.U32At( offset + 60 );

            if( pinNameHeapBase > aLayout.pinNameBytes )
                throwDecodeError( source, wxS( "part-type pin-name base leaves controller 14" ) );

            auto decodePartPin = [&]( MODEL_PIN_DEFINITION& aDefinitionPin, size_t aPinRecord, MODEL_GATE& aGate )
            {
                const size_t      pinOffset = aLayout.pinBase + aPinRecord * PIN_BYTES;
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
                    if( nameOffset >= aLayout.pinNameBytes - pinNameHeapBase )
                        throwDecodeError( pinSource, wxS( "pin-name offset leaves controller 14" ) );

                    SOURCE_PROVENANCE pinNameSource = sourceAt(
                            aSourceName, aModel.version, wxS( "pin name" ), 14, aPinRecord,
                            aLayout.pinNameBase + pinNameHeapBase + nameOffset,
                            aLayout.pinNameBytes - pinNameHeapBase - nameOffset, static_cast<int>( aSheetIndex ) );
                    aDefinitionPin.name = decodeFixedString( aBytes, aLayout.pinNameBase + pinNameHeapBase + nameOffset,
                                                             aLayout.pinNameBytes - pinNameHeapBase - nameOffset,
                                                             pinNameSource, aModel.diagnostics );
                }

                aDefinitionPin.electricalType =
                        pinElectricalType( aCursor.U8At( pinOffset + 21 ), pinSource, aModel.diagnostics );
                MODEL_GATE_PIN logicalPin;
                logicalPin.source = pinSource;
                logicalPin.definitionPin = { aDefinitionPin.id, pinSource };
                logicalPin.number = aDefinitionPin.number;
                logicalPin.name = aDefinitionPin.name;
                logicalPin.electricalType = aDefinitionPin.electricalType;
                logicalPin.swapGroup = aCursor.U8At( pinOffset + 20 );
                logicalPin.flags = aCursor.U16At( pinOffset + 22 );
                aDefinitionPin.properties.push_back(
                        sourceProperty( wxS( "swap_group" ),
                                        wxString::Format( wxS( "%u" ), aCursor.U8At( pinOffset + 20 ) ), pinSource ) );
                aGate.pins.push_back( { aDefinitionPin.id, pinSource } );
                aGate.logicalPins.push_back( std::move( logicalPin ) );
            };

            auto decodeConnectorPin = [&]( size_t aPinRecord )
            {
                const size_t      pinOffset = aLayout.pinBase + aPinRecord * PIN_BYTES;
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
                    if( nameOffset >= aLayout.pinNameBytes - pinNameHeapBase )
                        throwDecodeError( pinSource, wxS( "pin-name offset leaves controller 14" ) );

                    SOURCE_PROVENANCE pinNameSource = sourceAt(
                            aSourceName, aModel.version, wxS( "connector pin name" ), 14, aPinRecord,
                            aLayout.pinNameBase + pinNameHeapBase + nameOffset,
                            aLayout.pinNameBytes - pinNameHeapBase - nameOffset, static_cast<int>( aSheetIndex ) );
                    pin.name = decodeFixedString( aBytes, aLayout.pinNameBase + pinNameHeapBase + nameOffset,
                                                  aLayout.pinNameBytes - pinNameHeapBase - nameOffset, pinNameSource,
                                                  aModel.diagnostics );
                }

                pin.swapGroup = aCursor.U8At( pinOffset + 20 );
                pin.electricalType = pinElectricalType( aCursor.U8At( pinOffset + 21 ), pinSource, aModel.diagnostics );
                pin.flags = aCursor.U16At( pinOffset + 22 );
                return pin;
            };

            for( size_t gateRecord = gateStart; gateRecord < gateEnd; ++gateRecord )
            {
                const size_t      gateOffset = aLayout.gateBase + gateRecord * GATE_BYTES;
                SOURCE_PROVENANCE gateSource = sourceAt( aSourceName, aModel.version, wxS( "gate" ), 10, gateRecord,
                                                         gateOffset, GATE_BYTES, static_cast<int>( aSheetIndex ) );
                MODEL_GATE        gate;
                gate.id = GATE_ID( aLayout.gateIdBase + gateRecord );
                gate.source = gateSource;
                const uint16_t pinCount = aCursor.U16At( gateOffset + 8 );
                const uint16_t swapGroup = aCursor.U16At( gateOffset + 10 );
                const uint16_t primaryHandle = aCursor.U16At( gateOffset );

                if( pinCount == 0 )
                    continue;

                gate.unit = ++unitCursor;

                if( primaryHandle == 0xFFFF || primaryHandle >= aUsedDecals.size()
                    || !aUsedDecals[primaryHandle].definition )
                {
                    bool pinDecalGroup = primaryHandle == 0xFFFF && gateRecord + 1 < gateEnd;

                    for( size_t member = gateRecord + 1; pinDecalGroup && member < gateEnd; ++member )
                    {
                        const size_t   memberOffset = aLayout.gateBase + member * GATE_BYTES;
                        const uint16_t memberHandle = aCursor.U16At( memberOffset );
                        pinDecalGroup = aCursor.U16At( memberOffset + 8 ) == 0 && memberHandle < aUsedDecals.size()
                                        && aUsedDecals[memberHandle].definition;
                    }

                    if( !pinDecalGroup || partPinCursor + pinCount > pinEnd )
                        throwDecodeError( gateSource, wxS( "unresolved symbol definition reference" ) );

                    part.properties.push_back( sourceProperty(
                            wxString::Format( wxS( "pin_decal_group_%llu" ),
                                              static_cast<unsigned long long>( gateRecord - gateStart + 1 ) ),
                            wxString::Format( wxS( "%u" ), pinCount ), gateSource ) );

                    for( size_t member = gateRecord + 1; member < gateEnd; ++member )
                    {
                        const size_t   memberOffset = aLayout.gateBase + member * GATE_BYTES;
                        const uint16_t memberHandle = aCursor.U16At( memberOffset );

                        if( aCursor.U16At( memberOffset + 8 ) != 0 )
                            break;

                        SOURCE_PROVENANCE memberSource =
                                sourceAt( aSourceName, aModel.version, wxS( "pin-decal group member" ), 10, member,
                                          memberOffset, GATE_BYTES, static_cast<int>( aSheetIndex ) );

                        if( memberHandle >= aUsedDecals.size() || !aUsedDecals[memberHandle].definition )
                            throwDecodeError( memberSource, wxS( "unresolved pin-decal group member" ) );

                        gate.decalGroupMembers.push_back( { aUsedDecals[memberHandle].definition->id, memberSource } );
                    }

                    for( size_t pin = 0; pin < pinCount; ++pin, ++partPinCursor )
                        gate.connectorPins.push_back( decodeConnectorPin( partPinCursor ) );

                    part.gates.push_back( std::move( gate ) );
                    continue;
                }

                MODEL_SYMBOL_DEFINITION* definition = aUsedDecals[primaryHandle].definition;
                gate.definition = { definition->id, gateSource };
                gate.properties.push_back(
                        sourceProperty( wxS( "swap_group" ), wxString::Format( wxS( "%u" ), swapGroup ), gateSource ) );

                for( size_t alternate = 1; alternate < 4; ++alternate )
                {
                    const uint16_t handle = aCursor.U16At( gateOffset + alternate * 2 );

                    if( handle == 0xFFFF )
                        continue;

                    if( handle >= aUsedDecals.size() || !aUsedDecals[handle].definition )
                        throwDecodeError( gateSource, wxS( "unresolved alternate symbol definition reference" ) );

                    gate.alternateDefinitions.push_back( { aUsedDecals[handle].definition->id, gateSource } );
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

            if( signalPinCursor + signalPinCount > aControllers.pools[11].count )
                throwDecodeError( source, wxS( "part-type signal-pin slice leaves controller 12" ) );

            for( size_t signal = 0; signal < signalPinCount; ++signal, ++signalPinCursor )
            {
                const size_t      signalOffset = aLayout.signalPinBase + signalPinCursor * SIGNAL_PIN_BYTES;
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

        if( signalPinCursor != aControllers.pools[11].count )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "signal pin" ), 12, signalPinCursor,
                                                 aLayout.signalPinBase, aControllers.pools[11].usedBytes,
                                                 static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned signal-pin record" ) );
        }
    }


    void decodeDefinitionsAndParts( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                                    const SCH_SDB_BLOCK& aBlock, size_t aSheetIndex, const wxString& aSourceName,
                                    PADS_SCH_MODEL& aModel )
    {
        const SHEET_CONTROLLERS controllers = sheetControllers( aCursor, aBlock );

        if( aModel.version == 0x000C )
        {
            preserveRawDefinitionControllers( aBytes, controllers, aSheetIndex, aSourceName, aModel );
            return;
        }

        const DEFINITION_LAYOUT layout =
                definitionLayout( aCursor, controllers, aSheetIndex, aSourceName, aModel.version );

        std::vector<MODEL_SYMBOL_DEFINITION*> definitionsByRecord( controllers.pools[2].count );
        std::vector<size_t>                   pageGraphicRecords;
        std::vector<USED_DECAL>               usedDecals( controllers.pools[6].count );
        std::vector<USED_DECAL*>              semanticDecals;

        decodeSymbolDefinitions( aBytes, aCursor, controllers, layout, aSheetIndex, aSourceName, definitionsByRecord,
                                 pageGraphicRecords, aModel );
        decodeUsedDecals( aBytes, aCursor, controllers, layout, aSheetIndex, aSourceName, definitionsByRecord,
                          usedDecals, semanticDecals, aModel );

        const std::vector<USED_DECAL*> fieldDecals =
                buildFieldDecals( semanticDecals, layout, aSheetIndex, aSourceName, aModel.version );

        decodePageGraphics( aBytes, aCursor, layout, aSheetIndex, aSourceName, pageGraphicRecords, aModel );
        decodeDefinitionFields( aBytes, aCursor, layout, aSheetIndex, aSourceName, fieldDecals, aModel );
        decodePartTypes( aBytes, aCursor, controllers, layout, aSheetIndex, aSourceName, usedDecals, aModel );
    }


    struct PLACEMENT_DECODE
    {
        const std::vector<uint8_t>&   bytes;
        const PADS_IO::BINARY_CURSOR& cursor;
        const SHEET_CONTROLLERS&      controllers;
        const PLACEMENT_LAYOUT&       layout;
        const PLACEMENT_GLOBALS&      globals;
        const wxString&               sourceName;
        size_t                        sheetIndex;
    };


    struct PLACEMENT_TARGET
    {
        const MODEL_PART_TYPE*         part = nullptr;
        const MODEL_SYMBOL_DEFINITION* definition = nullptr;
        const MODEL_GATE*              gate = nullptr;
        uint32_t                       componentIdentity = 0;
        uint32_t                       groupHandle = 0;
        uint32_t                       attributeStart = 0;
        uint16_t                       attributeCount = 0;
        uint16_t                       decalHandle = 0;
        uint16_t                       unitIndex = 0;
    };


    struct INLINE_FIELD_LAYOUT
    {
        size_t position;
        size_t angle;
        size_t font;
        size_t height;
        size_t width;
    };


    std::pair<SOURCE_STRING, SOURCE_STRING> decodePlacementAttribute( const PLACEMENT_DECODE& aDecode,
                                                                      uint32_t aOffsetIndex, bool aRequireValue,
                                                                      PADS_SCH_MODEL& aModel )
    {
        if( aOffsetIndex >= aDecode.globals.attributeOffsetCount )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "attribute offset" ), 7, aOffsetIndex,
                              aDecode.globals.attributeOffsetBase, ATTRIBUTE_OFFSET_BYTES, -1 );
            throwDecodeError( source, wxS( "attribute offset index leaves outer controller 7" ) );
        }

        const size_t      offsetRecord = aDecode.globals.attributeOffsetBase + aOffsetIndex * ATTRIBUTE_OFFSET_BYTES;
        const uint32_t    heapOffset = aDecode.cursor.U32At( offsetRecord );
        SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aModel.version, wxS( "placement attribute" ), 2,
                                             aOffsetIndex, aDecode.globals.attributeHeapBase + heapOffset, 0, -1 );

        if( heapOffset >= aDecode.globals.attributeHeapBytes )
            throwDecodeError( source, wxS( "attribute string offset leaves outer controller 2" ) );

        size_t       end = source.absoluteOffset;
        const size_t heapEnd = aDecode.globals.attributeHeapBase + aDecode.globals.attributeHeapBytes;

        while( end < heapEnd && aDecode.bytes[end] != 0 )
            ++end;

        if( end == heapEnd )
            throwDecodeError( source, wxS( "placement attribute is not NUL terminated" ) );

        source.length = end - source.absoluteOffset;
        size_t separator = source.absoluteOffset;

        while( separator < end && aDecode.bytes[separator] != 1 )
            ++separator;

        if( aRequireValue && separator == end )
            throwDecodeError( source, wxS( "placement attribute lacks key/value separator" ) );

        SOURCE_STRING name = PADS_SCH_BINARY_PARSER::DecodeString(
                { aDecode.bytes.begin() + source.absoluteOffset, aDecode.bytes.begin() + separator }, DEFAULT_CODE_PAGE,
                source, aModel.diagnostics );
        SOURCE_STRING value;

        if( separator != end )
        {
            SOURCE_PROVENANCE valueSource = source;
            valueSource.absoluteOffset = separator + 1;
            valueSource.length = end - separator - 1;
            value = PADS_SCH_BINARY_PARSER::DecodeString(
                    { aDecode.bytes.begin() + valueSource.absoluteOffset, aDecode.bytes.begin() + end },
                    DEFAULT_CODE_PAGE, valueSource, aModel.diagnostics );
        }

        return std::pair<SOURCE_STRING, SOURCE_STRING>{ std::move( name ), std::move( value ) };
    }


    PLACEMENT_TARGET resolvePlacementTarget( const PLACEMENT_DECODE& aDecode, size_t aOffset,
                                             const SOURCE_PROVENANCE& aSource, PADS_SCH_MODEL& aModel )
    {
        PLACEMENT_TARGET target;
        target.componentIdentity = aDecode.cursor.U32At( aOffset + aDecode.layout.componentIdentity );

        const uint16_t partHandle = aDecode.cursor.U16At( aOffset + aDecode.layout.partType );
        auto           part = std::ranges::find_if( aModel.partTypes,
                                                    [&]( const MODEL_PART_TYPE& aPart )
                                                    {
                                              return aPart.source.sheet == static_cast<int>( aDecode.sheetIndex )
                                                     && aPart.source.recordIndex == partHandle;
                                          } );

        if( part == aModel.partTypes.end() )
        {
            const bool definitionClass =
                    std::ranges::any_of( aModel.definitions,
                                         [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                         {
                                             return aDefinition.source.sheet == static_cast<int>( aDecode.sheetIndex )
                                                    && aDefinition.source.recordIndex == partHandle;
                                         } );
            throwDecodeError( aSource, definitionClass
                                               ? wxS( "placement part-type handle targets definition object class" )
                                               : wxS( "unresolved placement part-type reference" ) );
        }

        const uint32_t groupHandle = aDecode.cursor.U32At( aOffset + aDecode.layout.componentGroup );

        if( groupHandle >= aDecode.globals.groupCount )
            throwDecodeError( aSource, wxS( "placement component-group handle leaves outer controller 6" ) );

        const size_t   groupOffset = aDecode.globals.groupBase + groupHandle * PLACEMENT_GROUP_BYTES;
        const uint32_t attributeStart = aDecode.cursor.U32At( groupOffset );
        const uint16_t attributeCount = aDecode.cursor.U16At( groupOffset + 20 );

        if( attributeCount < 2 || attributeStart > aDecode.globals.attributeOffsetCount
            || attributeCount > aDecode.globals.attributeOffsetCount - attributeStart )
        {
            throwDecodeError( aSource, wxS( "placement component-group attribute slice leaves controller 7" ) );
        }

        auto [groupPartName, unusedGroupPartValue] =
                decodePlacementAttribute( aDecode, attributeStart + 1, false, aModel );

        if( groupPartName.text != part->name.text )
            throwDecodeError( aSource, wxS( "placement component-group targets wrong part-type object class" ) );

        const uint16_t decalHandle = aDecode.cursor.U16At( aOffset + aDecode.layout.decal );

        if( decalHandle >= aDecode.controllers.pools[6].count )
            throwDecodeError( aSource, wxS( "unresolved placement decal reference" ) );

        const size_t   decalOffset = aDecode.controllers.offsets[6] + decalHandle * USED_DECAL_BYTES;
        const uint32_t definitionHandle = aDecode.cursor.U32At( decalOffset + 48 );
        auto           definition =
                std::ranges::find_if( aModel.definitions,
                                      [&]( const MODEL_SYMBOL_DEFINITION& aDefinition )
                                      {
                                          return aDefinition.source.sheet == static_cast<int>( aDecode.sheetIndex )
                                                 && aDefinition.source.recordIndex == definitionHandle;
                                      } );

        if( definition == aModel.definitions.end() )
            throwDecodeError( aSource, wxS( "placement decal targets wrong definition object class" ) );

        const uint16_t unitIndex = aDecode.cursor.U16At( aOffset + aDecode.layout.gate );

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
                                             return aDefinition.source.sheet == static_cast<int>( aDecode.sheetIndex )
                                                    && aDefinition.source.recordIndex == unitIndex;
                                         } );
            throwDecodeError( aSource, definitionClass ? wxS( "placement gate handle targets definition object class" )
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
            throwDecodeError( aSource, wxS( "placement decal and gate reference target different object classes" ) );
        }

        target.part = &*part;
        target.definition = &*definition;
        target.gate = gate;
        target.groupHandle = groupHandle;
        target.attributeStart = attributeStart;
        target.attributeCount = attributeCount;
        target.decalHandle = decalHandle;
        target.unitIndex = unitIndex;
        return target;
    }


    void decodePlacedPins( const PLACEMENT_DECODE& aDecode, uint32_t aPinStart, uint16_t aPinCount,
                           const MODEL_SYMBOL_DEFINITION& aDefinition, MODEL_PLACEMENT& aPlacement,
                           PADS_SCH_MODEL& aModel )
    {
        const size_t placedPinBase = aDecode.controllers.offsets[15];

        for( size_t pin = 0; pin < aPinCount; ++pin )
        {
            const size_t      pinOffset = placedPinBase + ( aPinStart + pin ) * aDecode.layout.placedPinBytes;
            SOURCE_PROVENANCE pinSource =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "placed pin" ), 16, aPinStart + pin, pinOffset,
                              aDecode.layout.placedPinBytes, static_cast<int>( aDecode.sheetIndex ) );
            const uint16_t pinOrdinal = aDecode.cursor.U16At( pinOffset + aDecode.layout.placedPinOrdinal );

            if( pinOrdinal >= aDefinition.pins.size() || pinOrdinal != pin )
                throwDecodeError( pinSource, wxS( "placed-pin handle leaves placement definition" ) );

            PLACED_PIN_REFERENCE pinReference{ aDefinition.pins[pinOrdinal].id, pinSource };
            pinReference.numberOffset = {
                static_cast<int64_t>( static_cast<int16_t>( aDecode.cursor.U16At( pinOffset + 6 ) ) ) * 2,
                static_cast<int64_t>( static_cast<int16_t>( aDecode.cursor.U16At( pinOffset + 8 ) ) ) * 2, pinSource
            };
            pinReference.numberPresentationFlags = aDecode.cursor.U16At( pinOffset + 10 );
            pinReference.numberAngle = ( pinReference.numberPresentationFlags & 0x0001 ) != 0 ? 900 : 0;

            pinReference.numberJustification = terminalJustification(
                    ( pinReference.numberPresentationFlags >> 4 ) & 0x0F, pinReference.numberAngle != 0 );

            pinReference.hasNumberPlacement = true;
            aPlacement.pins.push_back( std::move( pinReference ) );
        }
    }


    void decodeInlineField( const PLACEMENT_DECODE& aDecode, size_t aOffset, const SOURCE_PROVENANCE& aSource,
                            const wxString& aName, const SOURCE_STRING& aValue, bool aVisible,
                            const INLINE_FIELD_LAYOUT& aFieldLayout, MODEL_PLACEMENT& aPlacement,
                            PADS_SCH_MODEL& aModel )
    {
        SOURCE_PROVENANCE fieldSource = aSource;
        fieldSource.objectClass = wxS( "placement field" );
        fieldSource.absoluteOffset += aFieldLayout.position;
        fieldSource.length = 8;
        MODEL_FIELD field;
        field.source = fieldSource;
        field.name.text = aName;
        field.name.source = fieldSource;
        field.value = aValue;
        field.position = { decodeLocalCoordinate( aDecode.cursor.U16At( aOffset + aFieldLayout.position ) ),
                           decodeLocalCoordinate( aDecode.cursor.U16At( aOffset + aFieldLayout.position + 2 ) ),
                           fieldSource };
        field.angle = NormalizeAngle( aDecode.cursor.U16At( aOffset + aFieldLayout.angle ) );
        field.presentation.source = fieldSource;
        field.visible = aVisible;
        field.presentation.visible = aVisible;
        field.presentation.height = aDecode.cursor.U16At( aOffset + aFieldLayout.height );
        field.presentation.width = aDecode.cursor.U8At( aOffset + aFieldLayout.width );
        const uint16_t justification = aDecode.cursor.U16At( aOffset + aFieldLayout.angle + 2 );
        field.presentation.horizontalJustification = horizontalJustification( justification );
        field.presentation.verticalJustification = verticalJustification( justification );
        SOURCE_PROVENANCE fontSource = fieldSource;
        fontSource.absoluteOffset = aOffset + aFieldLayout.font;
        fontSource.length = 2;
        decodeGlobalFont( aDecode.bytes, aDecode.cursor, aDecode.globals, aDecode.sourceName, aModel.version,
                          static_cast<int16_t>( aDecode.cursor.U16At( aOffset + aFieldLayout.font ) ), fontSource,
                          field.presentation, false, aModel.diagnostics );
        aPlacement.fields.push_back( std::move( field ) );
    }


    void decodeCustomFields( const PLACEMENT_DECODE& aDecode, size_t aOffset, const SOURCE_PROVENANCE& aSource,
                             const PLACEMENT_TARGET& aTarget, uint32_t& aFieldCursor, MODEL_PLACEMENT& aPlacement,
                             PADS_SCH_MODEL& aModel )
    {
        const size_t fieldBase = aDecode.controllers.offsets[16];

        const uint16_t customFieldCount = aDecode.cursor.U16At( aOffset + aDecode.layout.fieldCount );

        if( aFieldCursor > aDecode.controllers.pools[16].count
            || customFieldCount > aDecode.controllers.pools[16].count - aFieldCursor )
        {
            throwDecodeError( aSource, wxS( "placement field ownership does not match controller 17" ) );
        }

        for( size_t fieldOrdinal = 0; fieldOrdinal < customFieldCount; ++fieldOrdinal )
        {
            const size_t      fieldOffset = fieldBase + aFieldCursor * aDecode.layout.fieldBytes;
            SOURCE_PROVENANCE fieldSource =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "placement field" ), 17, aFieldCursor,
                              fieldOffset, aDecode.layout.fieldBytes, static_cast<int>( aDecode.sheetIndex ) );
            SOURCE_STRING name;
            SOURCE_STRING value;

            const uint8_t  displayFlags = aDecode.cursor.U8At( fieldOffset + aDecode.layout.customDisplayFlags );
            const uint16_t attributeIndex = aDecode.cursor.U16At( fieldOffset + aDecode.layout.customAttributeIndex );

            if( attributeIndex == 0xFFFF )
            {
                name.text = wxS( "*" );
                name.source = fieldSource;
                value.source = fieldSource;
            }
            else
            {
                if( attributeIndex >= aTarget.attributeCount )
                    throwDecodeError( fieldSource, wxS( "placement field attribute index leaves component group" ) );

                std::tie( name, value ) =
                        decodePlacementAttribute( aDecode, aTarget.attributeStart + attributeIndex, true, aModel );
            }
            MODEL_FIELD field;
            field.source = fieldSource;
            field.name = std::move( name );
            field.value = std::move( value );
            field.position = {
                decodeLocalCoordinate( aDecode.cursor.U16At( fieldOffset + aDecode.layout.customX ) ),
                decodeLocalCoordinate( aDecode.cursor.U16At( fieldOffset + aDecode.layout.customX + 2 ) ), fieldSource
            };
            field.angle = NormalizeAngle( aDecode.cursor.U16At( fieldOffset + aDecode.layout.customAngle ) );

            if( field.angle % 900 != 0 )
                throwDecodeError( fieldSource, wxS( "unsupported placement-field rotation" ) );

            const uint8_t justification = aDecode.cursor.U8At( fieldOffset + aDecode.layout.customJustification );
            field.presentation.source = fieldSource;
            field.presentation.horizontalJustification = horizontalJustification( justification );
            field.presentation.verticalJustification = verticalJustification( justification );
            field.presentation.height = aDecode.cursor.U16At( fieldOffset + aDecode.layout.customHeight );
            field.presentation.width = aDecode.cursor.U8At( fieldOffset + aDecode.layout.customWidth );
            field.presentation.visible = ( displayFlags & 8 ) == 0;
            field.visible = field.presentation.visible;

            SOURCE_PROVENANCE fontSource = fieldSource;
            fontSource.length = 2;
            decodeGlobalFont( aDecode.bytes, aDecode.cursor, aDecode.globals, aDecode.sourceName, aModel.version,
                              static_cast<int16_t>( aDecode.cursor.U16At( fieldOffset + aDecode.layout.customFont ) ),
                              fontSource, field.presentation, true, aModel.diagnostics );
            field.properties.push_back( sourceProperty( wxS( "display_flags" ),
                                                        wxString::Format( wxS( "%u" ), displayFlags ), fieldSource ) );
            SOURCE_PROVENANCE attributeIndexSource = fieldSource;
            attributeIndexSource.absoluteOffset += aDecode.layout.customAttributeIndex;
            attributeIndexSource.length = 2;
            field.properties.push_back( sourceProperty(
                    wxS( "component_attribute_index" ),
                    wxString::Format( wxS( "%u" ),
                                      aDecode.cursor.U16At( fieldOffset + aDecode.layout.customAttributeIndex ) ),
                    attributeIndexSource ) );
            SOURCE_PROPERTY preservedTail = sourceProperty(
                    wxS( "preserved_field_tail" ),
                    wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( fieldOffset + aDecode.layout.customTail ) ),
                    fieldSource );
            preservedTail.disposition = PROPERTY_DISPOSITION::PRESERVED;
            field.properties.push_back( std::move( preservedTail ) );
            aPlacement.fields.push_back( std::move( field ) );
            ++aFieldCursor;
        }
    }


    void decodeGroupAttributeFields( const PLACEMENT_DECODE& aDecode, const PLACEMENT_TARGET& aTarget,
                                     MODEL_PLACEMENT& aPlacement, PADS_SCH_MODEL& aModel )
    {
        for( uint16_t attributeIndex = 2; attributeIndex < aTarget.attributeCount; ++attributeIndex )
        {
            auto [name, value] =
                    decodePlacementAttribute( aDecode, aTarget.attributeStart + attributeIndex, true, aModel );

            if( name.text.CmpNoCase( wxS( "PCB DECAL" ) ) == 0
                || std::ranges::any_of( aPlacement.fields,
                                        [&]( const MODEL_FIELD& aField )
                                        {
                                            return aField.name.text.CmpNoCase( name.text ) == 0;
                                        } ) )
            {
                continue;
            }

            MODEL_FIELD field;
            field.source = name.source;
            field.name = std::move( name );
            field.value = std::move( value );
            field.visible = false;
            field.presentation.source = field.source;
            field.presentation.visible = false;
            aPlacement.fields.push_back( std::move( field ) );
        }
    }


    void recordPlacementProperties( const PLACEMENT_DECODE& aDecode, const SOURCE_PROVENANCE& aSource,
                                    const PLACEMENT_TARGET& aTarget, uint16_t aRawAngle, MODEL_PLACEMENT& aPlacement )
    {
        SOURCE_PROVENANCE rawAngleSource = aSource;
        rawAngleSource.absoluteOffset += aDecode.layout.angle;
        rawAngleSource.length = 2;
        aPlacement.properties.push_back(
                sourceProperty( wxS( "raw_angle" ), wxString::Format( wxS( "%u" ), aRawAngle ), rawAngleSource ) );
        aPlacement.properties.push_back( sourceProperty(
                wxS( "component_identity" ), wxString::Format( wxS( "%u" ), aTarget.componentIdentity ), aSource ) );
        aPlacement.properties.push_back( sourceProperty(
                wxS( "component_group_handle" ), wxString::Format( wxS( "%u" ), aTarget.groupHandle ), aSource ) );
        aPlacement.properties.push_back( sourceProperty(
                wxS( "decal_handle" ), wxString::Format( wxS( "%u" ), aTarget.decalHandle ), aSource ) );
        SOURCE_PROVENANCE rawMirrorSource = aSource;
        rawMirrorSource.absoluteOffset += aDecode.layout.mirror;
        rawMirrorSource.length = 2;
        aPlacement.properties.push_back( sourceProperty(
                wxS( "raw_mirror" ), wxString::Format( wxS( "%u" ), aPlacement.mirrorFlags ), rawMirrorSource ) );
        SOURCE_PROVENANCE visibilitySource = aSource;
        visibilitySource.absoluteOffset += aDecode.layout.itemVisibility;
        visibilitySource.length = 1;
        aPlacement.properties.push_back(
                sourceProperty( wxS( "item_visibility_flags" ),
                                wxString::Format( wxS( "%u" ), aPlacement.itemVisibilityFlags ), visibilitySource ) );
    }


    void decodePlacementRecord( const PLACEMENT_DECODE& aDecode, size_t aRecord, uint32_t& aExpectedPinStart,
                                uint32_t& aFieldCursor, PADS_SCH_MODEL& aModel )
    {
        const PLACEMENT_LAYOUT& layout = aDecode.layout;
        const size_t            offset = aDecode.controllers.offsets[14] + aRecord * layout.placementBytes;

        SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aModel.version, wxS( "placement" ), 15, aRecord,
                                             offset, layout.placementBytes, static_cast<int>( aDecode.sheetIndex ) );

        const PLACEMENT_TARGET target = resolvePlacementTarget( aDecode, offset, source, aModel );
        const uint32_t         pinStart = aDecode.cursor.U32At( offset + layout.pinStart );
        const uint16_t         pinCount = aDecode.cursor.U16At( offset + layout.pinCount );

        if( pinStart != aExpectedPinStart || pinStart > aDecode.controllers.pools[15].count
            || pinCount > aDecode.controllers.pools[15].count - pinStart || pinCount != target.definition->pins.size() )
        {
            throwDecodeError( source, wxS( "placement pin ownership does not match controller 16" ) );
        }

        MODEL_PLACEMENT placement;

        if( aDecode.sheetIndex >= 0x0FFF || aRecord >= 0x100000 )
            throwDecodeError( source, wxS( "placement identity exceeds sheet/controller namespace" ) );

        placement.id = PLACEMENT_ID( static_cast<uint32_t>( aDecode.sheetIndex * 0x100000 + aRecord + 1 ) );
        placement.source = source;
        placement.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
        placement.partType = { target.part->id, source };

        if( target.gate )
            placement.gate = GATE_REFERENCE{ target.gate->id, source };

        placement.definition = { target.definition->id, source };
        placement.unit = target.unitIndex + 1;
        placement.position = { decodeCoordinate( aDecode.cursor.U16At( offset + layout.x ) ),
                               decodeCoordinate( aDecode.cursor.U16At( offset + layout.y ) ), source };
        const uint16_t rawAngle = aDecode.cursor.U16At( offset + layout.angle );
        placement.angle = NormalizeAngle( rawAngle );
        placement.mirrorFlags = aDecode.cursor.U16At( offset + layout.mirror );

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

        const bool knownMirror = placement.mirrorFlags <= 3;

        if( !knownMirror )
            recordTransformEnum( layout.mirror );

        placement.mirrored = placement.mirrorFlags != 0;
        SOURCE_PROVENANCE referenceSource = source;
        referenceSource.absoluteOffset += layout.reference;
        referenceSource.length = 40;
        placement.reference =
                decodeFixedString( aDecode.bytes, offset + layout.reference, 40, referenceSource, aModel.diagnostics );

        decodePlacedPins( aDecode, pinStart, pinCount, *target.definition, placement, aModel );
        aExpectedPinStart += pinCount;

        placement.itemVisibilityFlags = aDecode.cursor.U8At( offset + layout.itemVisibility );
        placement.referenceVisible = ( placement.itemVisibilityFlags & 0x01 ) == 0;
        placement.partTypeVisible = ( placement.itemVisibilityFlags & 0x02 ) == 0;
        placement.pinNamesVisible = ( placement.itemVisibilityFlags & 0x08 ) == 0;
        placement.pinNumbersVisible = ( placement.itemVisibilityFlags & 0x10 ) == 0;

        decodeInlineField( aDecode, offset, source, wxS( "REF-DES" ), placement.reference, placement.referenceVisible,
                           { layout.referenceField, layout.referenceFieldAngle, layout.referenceFont,
                             layout.referenceHeight, layout.referenceWidth },
                           placement, aModel );
        decodeInlineField( aDecode, offset, source, wxS( "PART-TYPE" ), target.part->name, placement.partTypeVisible,
                           { layout.partTypeField, layout.partTypeFieldAngle, layout.partTypeFont,
                             layout.partTypeHeight, layout.partTypeWidth },
                           placement, aModel );
        decodeCustomFields( aDecode, offset, source, target, aFieldCursor, placement, aModel );
        decodeGroupAttributeFields( aDecode, target, placement, aModel );
        recordPlacementProperties( aDecode, source, target, rawAngle, placement );
        aModel.placements.push_back( std::move( placement ) );
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

        const PLACEMENT_DECODE decode{ aBytes, aCursor, controllers, layout, aGlobals, aSourceName, aSheetIndex };
        uint32_t               expectedPinStart = 0;
        uint32_t               fieldCursor = 0;

        for( size_t record = 0; record < controllers.pools[14].count; ++record )
            decodePlacementRecord( decode, record, expectedPinStart, fieldCursor, aModel );

        if( expectedPinStart != controllers.pools[15].count )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "placed pin" ), 16, expectedPinStart,
                                                 controllers.offsets[15], controllers.pools[15].usedBytes,
                                                 static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned placed-pin record" ) );
        }

        if( fieldCursor != controllers.pools[16].count )
        {
            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "placement field" ), 17, fieldCursor,
                                                 controllers.offsets[16], controllers.pools[16].usedBytes,
                                                 static_cast<int>( aSheetIndex ) );
            throwDecodeError( source, wxS( "unowned placement-field record" ) );
        }
    }


    struct CONNECTIVITY_DECODE
    {
        const std::vector<uint8_t>&   bytes;
        const PADS_IO::BINARY_CURSOR& cursor;
        const SHEET_CONTROLLERS&      controllers;
        const CONNECTIVITY_GLOBALS&   globals;
        const wxString&               sourceName;
        size_t                        sheetIndex;
        size_t                        busBase;
        size_t                        junctionBase;
        size_t                        offpageBase;
        size_t                        connectionBase;
        size_t                        vertexBase;
        size_t                        netNameBase;
    };


    struct VERTEX_TILING
    {
        std::vector<uint32_t> starts;
        std::vector<uint32_t> ends;
    };


    struct SHEET_CONNECTIVITY
    {
        std::vector<MODEL_NET*>          sheetNets;
        std::vector<MODEL_CONNECTION*>   connections;
        std::vector<MODEL_NET*>          connectionNets;
        std::vector<std::vector<size_t>> junctionBacklinks;
        std::vector<std::vector<size_t>> offpageBacklinks;
    };


    std::pair<int64_t, int64_t> transformedPinPosition( const MODEL_PLACEMENT&      aPlacement,
                                                        const MODEL_PIN_DEFINITION& aPin )
    {
        int64_t x = aPin.position.x;
        int64_t y = aPin.position.y;

        switch( NormalizeAngle( aPlacement.angle ) )
        {
        case 900: std::tie( x, y ) = std::pair{ -y, x }; break;
        case 1800: std::tie( x, y ) = std::pair{ -x, -y }; break;
        case 2700: std::tie( x, y ) = std::pair{ y, -x }; break;
        default: break;
        }

        if( aPlacement.mirrorFlags & 1 )
            x = -x;

        if( aPlacement.mirrorFlags & 2 )
            y = -y;

        return std::pair{ aPlacement.position.x + x, aPlacement.position.y + y };
    }


    SOURCE_POINT offpagePosition( const CONNECTIVITY_DECODE& aDecode, size_t aRecord, uint16_t aVersion )
    {
        const size_t      offset = aDecode.offpageBase + aRecord * OFFPAGE_RECORD_BYTES;
        SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aVersion, wxS( "off-page reference" ), 20, aRecord,
                                             offset, OFFPAGE_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
        return SOURCE_POINT{ decodeCoordinate( aDecode.cursor.U16At( offset + 22 ) ),
                             decodeCoordinate( aDecode.cursor.U16At( offset + 24 ) ), source };
    }


    SOURCE_POINT junctionPosition( const CONNECTIVITY_DECODE& aDecode, size_t aRecord, uint16_t aVersion )
    {
        const size_t      offset = aDecode.junctionBase + aRecord * JUNCTION_RECORD_BYTES;
        SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aVersion, wxS( "junction" ), 19, aRecord, offset,
                                             JUNCTION_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
        return SOURCE_POINT{ decodeCoordinate( aDecode.cursor.U16At( offset + 4 ) ),
                             decodeCoordinate( aDecode.cursor.U16At( offset + 6 ) ), source };
    }


    std::unordered_set<uint32_t> decodeBusGlobalRecords( const CONNECTIVITY_DECODE& aDecode, PADS_SCH_MODEL& aModel )
    {
        std::unordered_set<uint32_t> busGlobalRecords;

        for( size_t record = 0; record < aDecode.controllers.pools[17].count; ++record )
        {
            const uint32_t globalRecord = aDecode.cursor.U32At( aDecode.busBase + record * BUS_RECORD_BYTES + 8 );
            const uint8_t  netKind = globalRecord < aDecode.globals.nets.size()
                                             ? aDecode.globals.nets[globalRecord].kindFlags & 0xFF
                                             : 0;

            if( globalRecord >= aDecode.globals.nets.size() || aDecode.globals.nets[globalRecord].tombstone
                || ( netKind != 1 && netKind != 5 ) )
            {
                SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aModel.version, wxS( "bus" ), 18, record,
                                                     aDecode.busBase + record * BUS_RECORD_BYTES, BUS_RECORD_BYTES,
                                                     static_cast<int>( aDecode.sheetIndex ) );
                throwDecodeError( source, wxS( "bus global-net handle targets wrong or unresolved object class" ) );
            }

            busGlobalRecords.insert( globalRecord );
        }

        return busGlobalRecords;
    }


    std::vector<MODEL_NET*> materializeSheetNets( const CONNECTIVITY_DECODE&          aDecode,
                                                  const std::unordered_set<uint32_t>& aBusGlobalRecords,
                                                  PADS_SCH_MODEL&                     aModel )
    {
        std::vector<MODEL_NET*> sheetNets( aDecode.globals.nets.size(), nullptr );
        // sheetNets holds pointers into nets, and one global record can own several memberships on
        // this sheet, so bound the reserve by the disjoint membership slices
        aModel.nets.reserve( aModel.nets.size() + aDecode.globals.membershipCount );

        for( size_t globalRecord = 0; globalRecord < aDecode.globals.nets.size(); ++globalRecord )
        {
            const GLOBAL_NET_RECORD& global = aDecode.globals.nets[globalRecord];

            if( global.tombstone )
                continue;

            for( uint32_t membership = global.membershipStart;
                 membership < global.membershipStart + global.membershipCount; ++membership )
            {
                if( aDecode.globals.membershipSheets[membership] != aDecode.sheetIndex )
                    continue;

                if( global.aliasCount != 0 || aBusGlobalRecords.contains( globalRecord ) )
                    continue;

                MODEL_NET net;
                net.id = NET_ID( membership );
                net.source = global.source;
                net.source.sheet = static_cast<int>( aDecode.sheetIndex );
                SOURCE_PROVENANCE membershipSource =
                        sourceAt( aDecode.sourceName, aModel.version, wxS( "net sheet membership" ), 4, membership,
                                  aDecode.globals.membershipBase + membership * NET_MEMBERSHIP_BYTES,
                                  NET_MEMBERSHIP_BYTES, static_cast<int>( aDecode.sheetIndex ) );
                net.sheet = { aModel.sheets[aDecode.sheetIndex].id, membershipSource };
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

        return sheetNets;
    }


    std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*> indexDefinitionPins( const PADS_SCH_MODEL& aModel )
    {
        std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*> definitionPins;

        for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
        {
            for( const MODEL_PIN_DEFINITION& pin : definition.pins )
                definitionPins.emplace( pin.id.Value(), &pin );
        }

        return definitionPins;
    }


    std::vector<MODEL_PLACEMENT*> indexSheetPlacements( const CONNECTIVITY_DECODE& aDecode, PADS_SCH_MODEL& aModel )
    {
        std::vector<MODEL_PLACEMENT*> placements( aDecode.controllers.pools[14].count, nullptr );

        for( MODEL_PLACEMENT& placement : aModel.placements )
        {
            if( placement.source.sheet != static_cast<int>( aDecode.sheetIndex ) )
                continue;

            if( placement.source.recordIndex >= placements.size() || placements[placement.source.recordIndex] )
                throwDecodeError( placement.source, wxS( "placement endpoint identity leaves controller 15" ) );

            placements[placement.source.recordIndex] = &placement;
        }

        return placements;
    }


    void decodeJunctions( const CONNECTIVITY_DECODE& aDecode, PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aDecode.controllers.pools[18].count; ++record )
        {
            const size_t      offset = aDecode.junctionBase + record * JUNCTION_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "junction" ), 19, record, offset,
                              JUNCTION_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );

            const uint16_t status = aDecode.cursor.U16At( offset + 10 );

            if( status != 0 && status != 0x00FC && status != 0x00FD )
                throwDecodeError( source, wxS( "invalid junction object-class marker" ) );

            MODEL_JUNCTION junction;
            junction.source = source;
            junction.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
            junction.position = junctionPosition( aDecode, record, aModel.version );
            SOURCE_PROVENANCE connectionSource = source;
            connectionSource.absoluteOffset += 8;
            connectionSource.length = 2;
            junction.properties.push_back( sourceProperty(
                    wxS( "connection_record" ), wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 8 ) ),
                    connectionSource ) );
            aModel.junctions.push_back( std::move( junction ) );
        }
    }


    VERTEX_TILING tileConnectivityVertices( const CONNECTIVITY_DECODE& aDecode, PADS_SCH_MODEL& aModel )
    {
        VERTEX_TILING tiling;


        tiling.starts.reserve( aDecode.controllers.pools[17].count + aDecode.controllers.pools[20].count );

        for( size_t record = 0; record < aDecode.controllers.pools[17].count; ++record )
            tiling.starts.push_back( aDecode.cursor.U32At( aDecode.busBase + record * BUS_RECORD_BYTES + 4 ) );

        for( size_t record = 0; record < aDecode.controllers.pools[20].count; ++record )
            tiling.starts.push_back(
                    aDecode.cursor.U32At( aDecode.connectionBase + record * CONNECTION_RECORD_BYTES + 4 ) );

        std::vector<size_t> objectOrder( tiling.starts.size() );

        for( size_t object = 0; object < objectOrder.size(); ++object )
            objectOrder[object] = object;

        std::ranges::sort( objectOrder,
                           [&]( size_t aLeft, size_t aRight )
                           {
                               return tiling.starts[aLeft] < tiling.starts[aRight];
                           } );
        tiling.ends.resize( tiling.starts.size() );

        for( size_t ordinal = 0; ordinal < objectOrder.size(); ++ordinal )
        {
            const size_t   object = objectOrder[ordinal];
            const uint32_t end = ordinal + 1 < objectOrder.size() ? tiling.starts[objectOrder[ordinal + 1]]
                                                                  : aDecode.controllers.pools[21].count;

            if( ( ordinal == 0 && tiling.starts[object] != 0 ) || tiling.starts[object] >= end
                || end > aDecode.controllers.pools[21].count )
            {
                SOURCE_PROVENANCE source =
                        sourceAt( aDecode.sourceName, aModel.version, wxS( "connectivity vertex ownership" ),
                                  object < aDecode.controllers.pools[17].count ? 18 : 21, object, aDecode.vertexBase,
                                  aDecode.controllers.pools[21].usedBytes, static_cast<int>( aDecode.sheetIndex ) );
                throwDecodeError( source, wxS( "connectivity vertex slices do not exactly tile controller 22" ) );
            }

            tiling.ends[object] = end;
        }

        if( tiling.starts.empty() && aDecode.controllers.pools[21].count != 0 )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "connection vertex" ), 22, 0, aDecode.vertexBase,
                              aDecode.controllers.pools[21].usedBytes, static_cast<int>( aDecode.sheetIndex ) );
            throwDecodeError( source, wxS( "unclaimed connection vertices" ) );
        }

        return tiling;
    }


    void appendVertices( const CONNECTIVITY_DECODE& aDecode, const VERTEX_TILING& aTiling, size_t aObject,
                         uint16_t aVersion, std::vector<SOURCE_POINT>& aVertices )
    {
        const uint32_t start = aTiling.starts[aObject];
        const uint32_t end = aTiling.ends[aObject];

        for( uint32_t vertex = start; vertex < end; ++vertex )
        {
            const size_t      offset = aDecode.vertexBase + vertex * CONNECTION_VERTEX_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aVersion, wxS( "connection vertex" ), 22, vertex, offset,
                              CONNECTION_VERTEX_BYTES, static_cast<int>( aDecode.sheetIndex ) );

            if( aDecode.cursor.U32At( offset ) != 0 )
                throwDecodeError( source, wxS( "nonzero connection-vertex padding" ) );

            aVertices.push_back( { decodeCoordinate( aDecode.cursor.U16At( offset + 4 ) ),
                                   decodeCoordinate( aDecode.cursor.U16At( offset + 6 ) ), source } );
        }
    }


    MODEL_CONNECTION_ENDPOINT
    decodeConnectionEndpoint( const CONNECTIVITY_DECODE& aDecode, const SOURCE_PROVENANCE& aSource, size_t aFieldOffset,
                              size_t aRelationshipOffset, const SOURCE_POINT& aWirePoint,
                              const std::vector<MODEL_PLACEMENT*>&                             aPlacements,
                              const std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*>& aDefinitionPins,
                              PADS_SCH_MODEL&                                                  aModel )
    {
        SOURCE_PROVENANCE endpointSource = aSource;
        endpointSource.objectClass = wxS( "connection endpoint" );
        endpointSource.absoluteOffset += aFieldOffset;
        endpointSource.length = 2;
        const uint16_t    raw = aDecode.cursor.U16At( endpointSource.absoluteOffset );
        const uint16_t    objectClass = raw >> 12;
        const uint16_t    objectRecord = raw & 0x0FFF;
        SOURCE_PROVENANCE relationshipSource = aSource;
        relationshipSource.objectClass = wxS( "connection endpoint relationship" );
        relationshipSource.absoluteOffset += aRelationshipOffset;
        relationshipSource.length = 4;
        const uint32_t            relationship = aDecode.cursor.U32At( relationshipSource.absoluteOffset );
        MODEL_CONNECTION_ENDPOINT result;
        result.source = endpointSource;

        switch( objectClass )
        {
        case 0:
            if( objectRecord >= aPlacements.size() || !aPlacements[objectRecord] )
                throwDecodeError( endpointSource, wxS( "unresolved placement endpoint" ) );

            {
                MODEL_PLACEMENT&     placement = *aPlacements[objectRecord];
                const PIN_REFERENCE* matchedPin = nullptr;

                for( const PIN_REFERENCE& pin : placement.pins )
                {
                    auto definitionPin = aDefinitionPins.find( pin.id.Value() );

                    if( definitionPin == aDefinitionPins.end() )
                        throwDecodeError( pin.source, wxS( "placement pin definition is unresolved" ) );

                    const auto position = transformedPinPosition( placement, *definitionPin->second );

                    if( position.first != aWirePoint.x || position.second != aWirePoint.y )
                        continue;

                    if( matchedPin )
                        throwDecodeError( endpointSource,
                                          wxS( "placement has duplicate pins at connection endpoint" ) );

                    matchedPin = &pin;
                }

                if( !matchedPin )
                    throwDecodeError( endpointSource,
                                      wxString::Format( wxS( "placement has no pin at connection endpoint; "
                                                             "angle %d mirror %u" ),
                                                        placement.angle, placement.mirrorFlags ) );

                result.kind = MODEL_ENDPOINT_KIND::PIN;
                result.placement = PLACEMENT_REFERENCE{ placement.id, endpointSource };
                result.pin = PIN_REFERENCE{ matchedPin->id, endpointSource };
                result.point = aWirePoint;
            }
            break;

        case 2:
            if( objectRecord >= aDecode.controllers.pools[19].count )
                throwDecodeError( endpointSource, wxS( "unresolved off-page endpoint" ) );

            result.kind = MODEL_ENDPOINT_KIND::POINT;
            result.point = offpagePosition( aDecode, objectRecord, aModel.version );
            break;

        case 3:
            if( objectRecord >= aDecode.controllers.pools[18].count )
                throwDecodeError( endpointSource, wxS( "unresolved junction endpoint" ) );

            result.kind = MODEL_ENDPOINT_KIND::POINT;
            result.point = junctionPosition( aDecode, objectRecord, aModel.version );
            break;

        default: throwDecodeError( endpointSource, wxS( "wrong endpoint object class" ) );
        }

        result.properties.push_back(
                sourceProperty( wxS( "raw_endpoint_handle" ), wxString::Format( wxS( "%u" ), raw ), endpointSource ) );
        SOURCE_PROPERTY relationshipProperty = sourceProperty(
                wxS( "raw_endpoint_relationship" ), wxString::Format( wxS( "%u" ), relationship ), relationshipSource );
        relationshipProperty.disposition = PROPERTY_DISPOSITION::PRESERVED;
        result.properties.push_back( std::move( relationshipProperty ) );
        return result;
    }


    void decodeConnections( const CONNECTIVITY_DECODE& aDecode, const VERTEX_TILING& aTiling,
                            const std::vector<MODEL_PLACEMENT*>&                             aPlacements,
                            const std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*>& aDefinitionPins,
                            SHEET_CONNECTIVITY& aConnectivity, PADS_SCH_MODEL& aModel )
    {
        aConnectivity.connections.assign( aDecode.controllers.pools[20].count, nullptr );
        aConnectivity.connectionNets.assign( aDecode.controllers.pools[20].count, nullptr );
        aConnectivity.junctionBacklinks.assign( aDecode.controllers.pools[18].count, {} );
        aConnectivity.offpageBacklinks.assign( aDecode.controllers.pools[19].count, {} );
        std::vector<std::array<uint16_t, 2>> connectionEndpointHandles( aDecode.controllers.pools[20].count );
        std::vector<size_t>                  connectionCounts( aConnectivity.sheetNets.size(), 0 );

        for( size_t record = 0; record < aDecode.controllers.pools[20].count; ++record )
        {
            const uint32_t netHandle =
                    aDecode.cursor.U32At( aDecode.connectionBase + record * CONNECTION_RECORD_BYTES + 8 );

            if( netHandle < connectionCounts.size() )
                ++connectionCounts[netHandle];
        }

        for( size_t net = 0; net < aConnectivity.sheetNets.size(); ++net )
        {
            if( aConnectivity.sheetNets[net] )
                aConnectivity.sheetNets[net]->connections.reserve( connectionCounts[net] );
        }

        for( size_t record = 0; record < aDecode.controllers.pools[20].count; ++record )
        {
            const size_t      offset = aDecode.connectionBase + record * CONNECTION_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "connection" ), 21, record, offset,
                              CONNECTION_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint32_t netHandle = aDecode.cursor.U32At( offset + 8 );

            if( netHandle >= aConnectivity.sheetNets.size() || !aConnectivity.sheetNets[netHandle] )
                throwDecodeError( source, wxS( "connection net handle targets wrong or unresolved object class" ) );

            const uint16_t marker = aDecode.cursor.U16At( offset + 34 );

            const uint8_t markerStatus = marker & 0xFF;

            if( marker >> 8 < 2 || marker >> 8 > 6
                || ( markerStatus != 0 && markerStatus != 0xFC && markerStatus != 0xFD ) )
                throwDecodeError( source, wxS( "invalid connection object-class marker" ) );

            MODEL_CONNECTION connection;
            connection.source = source;

            appendVertices( aDecode, aTiling, aDecode.controllers.pools[17].count + record, aModel.version,
                            connection.vertices );

            if( connection.vertices.size() < 2 )
                throwDecodeError( source, wxS( "connection lacks explicit endpoint vertices" ) );

            connection.endpoints.push_back( decodeConnectionEndpoint(
                    aDecode, source, 12, 16, connection.vertices.front(), aPlacements, aDefinitionPins, aModel ) );
            connection.endpoints.push_back( decodeConnectionEndpoint(
                    aDecode, source, 14, 20, connection.vertices.back(), aPlacements, aDefinitionPins, aModel ) );

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

            connectionEndpointHandles[record] = { aDecode.cursor.U16At( offset + 12 ),
                                                  aDecode.cursor.U16At( offset + 14 ) };

            for( size_t endpointIndex = 0; endpointIndex < connectionEndpointHandles[record].size(); ++endpointIndex )
            {
                const uint16_t       raw = connectionEndpointHandles[record][endpointIndex];
                const size_t         objectClass = raw >> 12;
                const size_t         objectRecord = raw & 0x0FFF;
                std::vector<size_t>* backlinks = nullptr;

                if( objectClass == 2 )
                    backlinks = &aConnectivity.offpageBacklinks[objectRecord];
                else if( objectClass == 3 )
                    backlinks = &aConnectivity.junctionBacklinks[objectRecord];

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
            aConnectivity.sheetNets[netHandle]->connections.push_back( std::move( connection ) );
            aConnectivity.connections[record] = &aConnectivity.sheetNets[netHandle]->connections.back();
            aConnectivity.connectionNets[record] = aConnectivity.sheetNets[netHandle];
        }
    }


    void validateJunctionBacklinks( const CONNECTIVITY_DECODE& aDecode, const SHEET_CONNECTIVITY& aConnectivity,
                                    PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aDecode.controllers.pools[18].count; ++record )
        {
            const size_t      offset = aDecode.junctionBase + record * JUNCTION_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "junction" ), 19, record, offset,
                              JUNCTION_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint16_t    owner = aDecode.cursor.U16At( offset + 8 );
            SOURCE_PROVENANCE ownerSource = source;
            ownerSource.absoluteOffset += 8;
            ownerSource.length = 2;

            if( owner >= aConnectivity.connections.size() || !aConnectivity.connections[owner] )
                throwDecodeError( ownerSource, wxS( "junction connection handle leaves controller 21" ) );

            if( std::ranges::find( aConnectivity.junctionBacklinks[record], owner )
                == aConnectivity.junctionBacklinks[record].end() )
                throwDecodeError( ownerSource, wxS( "junction connection handle does not point back" ) );

            for( size_t connection : aConnectivity.junctionBacklinks[record] )
            {
                if( aConnectivity.connectionNets[connection] != aConnectivity.connectionNets[owner] )
                {
                    throwDecodeError( ownerSource, wxS( "junction is shared across different nets" ) );
                }
            }
        }
    }


    void validateOffpageBacklinks( const CONNECTIVITY_DECODE& aDecode, const SHEET_CONNECTIVITY& aConnectivity,
                                   PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aDecode.controllers.pools[19].count; ++record )
        {
            const size_t      offset = aDecode.offpageBase + record * OFFPAGE_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "off-page reference" ), 20, record, offset,
                              OFFPAGE_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint16_t    owner = aDecode.cursor.U16At( offset + 8 );
            SOURCE_PROVENANCE ownerSource = source;
            ownerSource.absoluteOffset += 8;
            ownerSource.length = 2;

            if( owner >= aConnectivity.connections.size() || !aConnectivity.connections[owner] )
                throwDecodeError( ownerSource, wxS( "off-page net handle leaves controller 21" ) );

            if( std::ranges::find( aConnectivity.offpageBacklinks[record], owner )
                == aConnectivity.offpageBacklinks[record].end() )
                throwDecodeError( ownerSource, wxS( "off-page connection handle does not point back" ) );

            for( size_t connection : aConnectivity.offpageBacklinks[record] )
            {
                if( aConnectivity.connectionNets[connection] != aConnectivity.connectionNets[owner] )
                    throwDecodeError( ownerSource, wxS( "off-page reference is shared across different nets" ) );
            }
        }
    }


    std::vector<bool> decodeBuses( const CONNECTIVITY_DECODE& aDecode, const VERTEX_TILING& aTiling,
                                   const std::unordered_set<uint32_t>& aBusGlobalRecords,
                                   const SHEET_CONNECTIVITY& aConnectivity, PADS_SCH_MODEL& aModel )
    {
        std::vector<bool> claimedBusEntries( aDecode.controllers.pools[19].count, false );

        for( size_t record = 0; record < aDecode.controllers.pools[17].count; ++record )
        {
            const size_t      offset = aDecode.busBase + record * BUS_RECORD_BYTES;
            SOURCE_PROVENANCE source = sourceAt( aDecode.sourceName, aModel.version, wxS( "bus" ), 18, record, offset,
                                                 BUS_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint16_t    marker = aDecode.cursor.U16At( offset + 38 );

            const uint8_t markerStatus = marker & 0xFF;

            if( marker >> 8 < 2 || marker >> 8 > 4 || ( markerStatus != 0 && markerStatus != 0xFD ) )
                throwDecodeError( source, wxS( "invalid bus object-class marker" ) );

            const size_t globalRecord = aDecode.cursor.U32At( offset + 8 );

            if( globalRecord >= aDecode.globals.nets.size() || aDecode.globals.nets[globalRecord].tombstone
                || !aBusGlobalRecords.contains( globalRecord ) )
            {
                throwDecodeError( source, wxS( "bus global-net handle targets wrong or unresolved object class" ) );
            }

            const GLOBAL_NET_RECORD& global = aDecode.globals.nets[globalRecord];
            uint32_t                 membership = global.membershipStart;

            while( membership < global.membershipStart + global.membershipCount
                   && aDecode.globals.membershipSheets[membership] != aDecode.sheetIndex )
            {
                ++membership;
            }

            if( membership == global.membershipStart + global.membershipCount )
                throwDecodeError( source, wxS( "bus global-net handle does not belong to this sheet" ) );

            MODEL_BUS bus;
            bus.id = BUS_ID( static_cast<uint32_t>( aDecode.sheetIndex * 0x100000 + record + 1 ) );
            bus.source = source;
            bus.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
            bus.name = global.name;
            bus.aliases.push_back( global.name );
            bus.declaredMembers = global.aliasMembers;
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
            appendVertices( aDecode, aTiling, record, aModel.version, bus.vertices );

            std::vector<size_t>        entryRecords;
            uint16_t                   entryHandle = aDecode.cursor.U16At( offset + 24 );
            std::unordered_set<size_t> chain;

            while( ( entryHandle >> 12 ) == 2 )
            {
                const size_t entryRecord = entryHandle & 0x0FFF;

                if( entryRecord >= aDecode.controllers.pools[19].count )
                    throwDecodeError( source, wxS( "unresolved bus-entry handle" ) );

                if( !chain.insert( entryRecord ).second )
                    throwDecodeError( source, wxS( "cyclic bus-entry handle chain" ) );

                entryRecords.push_back( entryRecord );
                entryHandle = aDecode.cursor.U16At( aDecode.offpageBase + entryRecord * OFFPAGE_RECORD_BYTES + 4 );
            }

            if( record > 0x0FFF || entryHandle != 0xBFFF - record )
                throwDecodeError( source, wxS( "bus-entry chain terminates in wrong object class" ) );

            std::ranges::reverse( entryRecords );

            const bool exactAliasMapping = entryRecords.size() == global.aliasMembers.size();

            for( size_t entry = 0; entry < entryRecords.size(); ++entry )
            {
                const size_t      entryRecord = entryRecords[entry];
                const size_t      entryOffset = aDecode.offpageBase + entryRecord * OFFPAGE_RECORD_BYTES;
                SOURCE_PROVENANCE entrySource =
                        sourceAt( aDecode.sourceName, aModel.version, wxS( "bus entry" ), 20, entryRecord, entryOffset,
                                  OFFPAGE_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );

                if( aDecode.cursor.U8At( entryOffset + 30 ) != 0xFF )
                    throwDecodeError( entrySource, wxS( "bus-entry handle targets wrong off-page object class" ) );

                if( claimedBusEntries[entryRecord] )
                    throwDecodeError( entrySource, wxS( "duplicate bus-entry membership" ) );

                const uint16_t connectionHandle = aDecode.cursor.U16At( entryOffset + 8 );

                if( connectionHandle >= aConnectivity.connections.size()
                    || !aConnectivity.connections[connectionHandle] )
                    throwDecodeError( entrySource, wxS( "unresolved bus-entry connection reference" ) );

                MODEL_CONNECTION* entryConnection = aConnectivity.connections[connectionHandle];
                MODEL_NET*        memberNet = aConnectivity.connectionNets[connectionHandle];

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
                busEntry.position = offpagePosition( aDecode, entryRecord, aModel.version );
                busEntry.memberNet = { memberNet->id, entrySource };
                bus.entries.push_back( std::move( busEntry ) );
                bus.memberNets.push_back( { memberNet->id, entrySource } );
            }

            aModel.buses.push_back( std::move( bus ) );
        }

        return claimedBusEntries;
    }


    void decodeOffpageLabels( const CONNECTIVITY_DECODE& aDecode, const SHEET_CONNECTIVITY& aConnectivity,
                              const std::vector<bool>& aClaimedBusEntries, PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aDecode.controllers.pools[19].count; ++record )
        {
            const size_t      offset = aDecode.offpageBase + record * OFFPAGE_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "off-page reference" ), 20, record, offset,
                              OFFPAGE_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint8_t rawKind = aDecode.cursor.U8At( offset + 30 );

            if( rawKind == 0xFF )
            {
                if( !aClaimedBusEntries[record] )
                    throwDecodeError( source, wxS( "unclaimed bus-entry record" ) );

                continue;
            }

            const uint16_t connectionHandle = aDecode.cursor.U16At( offset + 8 );

            if( connectionHandle >= aConnectivity.connections.size() || !aConnectivity.connections[connectionHandle] )
                throwDecodeError( source, wxS( "off-page net handle leaves controller 21" ) );

            MODEL_NET* ownerNet = aConnectivity.connectionNets[connectionHandle];

            if( !ownerNet )
                throwDecodeError( source, wxS( "off-page record targets wrong net object class" ) );

            MODEL_LABEL label;
            label.source = source;
            label.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
            label.text = ownerNet->name;
            label.position = offpagePosition( aDecode, record, aModel.version );
            label.angle = NormalizeAngle( aDecode.cursor.U16At( offset + 26 ) );
            label.symbolVariant = rawKind;

            if( rawKind == 0xFE )
            {
                label.kind = MODEL_LABEL_KIND::LOCAL;
            }
            else
            {
                const uint16_t decalHandle = aDecode.cursor.U16At( offset + 4 );

                if( decalHandle == 0xFFFF )
                {
                    label.kind = MODEL_LABEL_KIND::GLOBAL;
                }
                else
                {
                    if( decalHandle >= aDecode.controllers.pools[6].count )
                        throwDecodeError( source, wxS( "off-page decal handle leaves controller 7" ) );

                    SOURCE_PROVENANCE decalSource =
                            sourceAt( aDecode.sourceName, aModel.version, wxS( "used decal" ), 7, decalHandle,
                                      aDecode.controllers.offsets[6] + decalHandle * USED_DECAL_BYTES, 40,
                                      static_cast<int>( aDecode.sheetIndex ) );
                    SOURCE_STRING  decalName = decodeFixedString( aDecode.bytes, decalSource.absoluteOffset,
                                                                  decalSource.length, decalSource, aModel.diagnostics );
                    const uint32_t definitionRecord = aDecode.cursor.U32At( decalSource.absoluteOffset + 48 );

                    if( definitionRecord >= aDecode.controllers.pools[2].count )
                        throwDecodeError( decalSource, wxS( "off-page decal definition leaves controller 3" ) );

                    const DEFINITION_ID definitionId(
                            static_cast<uint32_t>( aDecode.sheetIndex * 0x100000 + 1 + definitionRecord ) );
                    auto specialPartOwnsDefinition = [&]( const wxString& aPartName )
                    {
                        return std::ranges::any_of(
                                aModel.partTypes,
                                [&]( const MODEL_PART_TYPE& aPart )
                                {
                                    if( aPart.name.text != aPartName )
                                        return false;

                                    return std::ranges::any_of(
                                            aPart.gates,
                                            [&]( const MODEL_GATE& aGate )
                                            {
                                                return aGate.definition.id == definitionId
                                                       || std::ranges::any_of( aGate.alternateDefinitions,
                                                                               [&]( const DEFINITION_REFERENCE& aRef )
                                                                               {
                                                                                   return aRef.id == definitionId;
                                                                               } );
                                            } );
                                } );
                    };

                    if( specialPartOwnsDefinition( wxS( "$OSR_SYMS" ) ) )
                        label.kind = MODEL_LABEL_KIND::GLOBAL;
                    else if( specialPartOwnsDefinition( wxS( "$GND_SYMS" ) ) )
                        label.kind = MODEL_LABEL_KIND::GROUND;
                    else if( specialPartOwnsDefinition( wxS( "$PWR_SYMS" ) ) )
                        label.kind = MODEL_LABEL_KIND::POWER;
                    else
                    {
                        label.kind = MODEL_LABEL_KIND::UNSUPPORTED;
                        SOURCE_PROPERTY unsupportedDecal =
                                sourceProperty( wxS( "unsupported_offpage_decal" ), decalName.text, decalSource );
                        unsupportedDecal.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                        aModel.diagnostics.push_back(
                                MakePropertyDiagnostic( RPT_SEVERITY_WARNING, unsupportedDecal,
                                                        wxS( "unsupported off-page decal class preserved" ) ) );
                        label.properties.push_back( std::move( unsupportedDecal ) );
                    }
                }
            }

            if( label.kind == MODEL_LABEL_KIND::GLOBAL )
            {
                for( uint32_t membership =
                             aDecode.globals
                                     .nets[aDecode.cursor.U32At( aDecode.connectionBase
                                                                 + connectionHandle * CONNECTION_RECORD_BYTES + 8 )]
                                     .membershipStart;
                     membership
                     < aDecode.globals.nets[aDecode.cursor.U32At( aDecode.connectionBase
                                                                  + connectionHandle * CONNECTION_RECORD_BYTES + 8 )]
                                       .membershipStart
                               + aDecode.globals
                                         .nets[aDecode.cursor.U32At( aDecode.connectionBase
                                                                     + connectionHandle * CONNECTION_RECORD_BYTES + 8 )]
                                         .membershipCount;
                     ++membership )
                {
                    const uint16_t peerSheet = aDecode.globals.membershipSheets[membership];

                    if( peerSheet != aDecode.sheetIndex )
                        label.linkedSheets.push_back( { aModel.sheets[peerSheet].id, source } );
                }
            }

            label.properties.push_back(
                    sourceProperty( wxS( "offpage_variant" ), wxString::Format( wxS( "%u" ), rawKind ), source ) );
            aModel.labels.push_back( std::move( label ) );
        }
    }


    MODEL_TEXT_PRESENTATION netNamePresentation( const CONNECTIVITY_DECODE& aDecode, size_t aOffset,
                                                 const SOURCE_PROVENANCE& aSource, PADS_SCH_MODEL& aModel )
    {
        MODEL_TEXT_PRESENTATION presentation;
        presentation.source = aSource;
        presentation.height = aDecode.cursor.U16At( aOffset + 2 );
        presentation.width = aDecode.cursor.U16At( aOffset + 4 );
        presentation.horizontalJustification = horizontalJustification( aDecode.cursor.U16At( aOffset + 26 ) );
        presentation.verticalJustification = verticalJustification( aDecode.cursor.U16At( aOffset + 26 ) );
        const int16_t     fontHandle = static_cast<int16_t>( aDecode.cursor.U16At( aOffset ) );
        SOURCE_PROVENANCE fontSource = aSource;
        fontSource.length = 2;

        if( fontHandle == -1 )
        {
            presentation.font = decodedDefinitionFont( -1, fontSource );
        }
        else
        {
            if( fontHandle < 0 || static_cast<uint32_t>( fontHandle ) >= aDecode.globals.fontCount )
                throwDecodeError( fontSource, wxS( "net-name font handle leaves outer controller 19" ) );

            const size_t fontOffset = aDecode.globals.fontBase + static_cast<size_t>( fontHandle ) * FONT_RECORD_BYTES;
            SOURCE_PROVENANCE nameSource = sourceAt( aDecode.sourceName, aModel.version, wxS( "net-name font" ), 19,
                                                     fontHandle, fontOffset + 4, 32, -1 );
            presentation.font = decodeFixedString( aDecode.bytes, fontOffset + 4, 32, nameSource, aModel.diagnostics );
            const uint32_t style = aDecode.cursor.U32At( fontOffset );
            presentation.bold = style & 1;
            presentation.italic = style & 2;

            if( style & ~3U )
            {
                SOURCE_PROPERTY property = sourceProperty( wxS( "unsupported_font_style_flags" ),
                                                           wxString::Format( wxS( "%u" ), style & ~3U ), fontSource );
                property.disposition = PROPERTY_DISPOSITION::UNSUPPORTED;
                presentation.properties.push_back( property );
                aModel.diagnostics.push_back( MakePropertyDiagnostic(
                        RPT_SEVERITY_WARNING, property, wxS( "unsupported net-name font style flags preserved" ) ) );
            }
        }

        presentation.properties.push_back(
                sourceProperty( wxS( "font_handle" ), wxString::Format( wxS( "%d" ), fontHandle ), fontSource ) );
        return presentation;
    }


    void decodeNetNames( const CONNECTIVITY_DECODE& aDecode, const SHEET_CONNECTIVITY& aConnectivity,
                         PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aDecode.controllers.pools[22].count; ++record )
        {
            const size_t      offset = aDecode.netNameBase + record * NET_NAME_RECORD_BYTES;
            SOURCE_PROVENANCE source =
                    sourceAt( aDecode.sourceName, aModel.version, wxS( "net-name presentation" ), 23, record, offset,
                              NET_NAME_RECORD_BYTES, static_cast<int>( aDecode.sheetIndex ) );
            const uint32_t               globalRecord = aDecode.cursor.U32At( offset + 16 );
            const uint16_t               ownerHandle = aDecode.cursor.U16At( offset + 38 );
            const uint16_t               childHandle = aDecode.cursor.U16At( offset + 40 );
            std::vector<SOURCE_PROPERTY> preservedPresentation;
            auto preserve = [&]( const wxString& aName, const wxString& aValue, size_t aRelativeOffset, size_t aLength )
            {
                SOURCE_PROVENANCE propertySource = source;
                propertySource.absoluteOffset += aRelativeOffset;
                propertySource.length = aLength;
                SOURCE_PROPERTY property = sourceProperty( aName, aValue, propertySource );
                property.disposition = PROPERTY_DISPOSITION::PRESERVED;
                preservedPresentation.push_back( std::move( property ) );
            };
            wxString presentation06;

            for( size_t index = 6; index < 16; ++index )
                presentation06 += wxString::Format( wxS( "%02x" ), aDecode.bytes[offset + index] );

            preserve( wxS( "preserved_net_name_presentation_06" ), presentation06, 6, 10 );
            preserve( wxS( "preserved_net_name_secondary_offset" ),
                      wxString::Format( wxS( "%d,%d" ), static_cast<int16_t>( aDecode.cursor.U16At( offset + 28 ) ),
                                        static_cast<int16_t>( aDecode.cursor.U16At( offset + 30 ) ) ),
                      28, 4 );
            preserve( wxS( "preserved_net_name_presentation_20" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 32 ) ), 32, 2 );
            preserve( wxS( "preserved_net_name_presentation_flags" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 34 ) ), 34, 2 );
            preserve( wxS( "preserved_net_name_predecessor_handle" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 36 ) ), 36, 2 );
            preserve( wxS( "preserved_net_name_predecessor_record" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 42 ) ), 42, 2 );
            preserve( wxS( "preserved_net_name_successor_record" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 44 ) ), 44, 2 );
            preserve( wxS( "preserved_net_name_tail" ),
                      wxString::Format( wxS( "%u" ), aDecode.cursor.U16At( offset + 46 ) ), 46, 2 );

            if( ( ownerHandle & 0xF000 ) == 0x4000 )
            {
                if( globalRecord >= aDecode.globals.nets.size() || aDecode.globals.nets[globalRecord].tombstone )
                    throwDecodeError( source, wxS( "bus net-name record targets wrong global-net object class" ) );

                const size_t busRecord = ownerHandle & 0x0FFF;
                auto         bus = std::ranges::find_if( aModel.buses,
                                                         [&]( const MODEL_BUS& aBus )
                                                         {
                                                     return aBus.sheet.id == aModel.sheets[aDecode.sheetIndex].id
                                                            && aBus.source.recordIndex == busRecord;
                                                 } );

                if( bus == aModel.buses.end() || childHandle == 0 || childHandle > bus->memberNets.size()
                    || bus->name.text != aDecode.globals.nets[globalRecord].name.text )
                {
                    throwDecodeError( source, wxS( "net-name bus owner targets wrong object class" ) );
                }

                bus->properties.push_back( sourceProperty( wxS( "net_name_presentation_record" ),
                                                           wxString::Format( wxS( "%llu" ), record ), source ) );
                bus->properties.insert( bus->properties.end(), std::make_move_iterator( preservedPresentation.begin() ),
                                        std::make_move_iterator( preservedPresentation.end() ) );
                continue;
            }

            if( globalRecord >= aConnectivity.sheetNets.size() || !aConnectivity.sheetNets[globalRecord] )
                throwDecodeError( source, wxS( "net-name record targets wrong or unresolved net object class" ) );

            MODEL_NET&              ownerNet = *aConnectivity.sheetNets[globalRecord];
            const SOURCE_POINT      textOffset{ decodeTerminalCoordinate( aDecode.cursor.U16At( offset + 20 ) ),
                                           decodeTerminalCoordinate( aDecode.cursor.U16At( offset + 22 ) ), source };
            MODEL_TEXT_PRESENTATION presentation = netNamePresentation( aDecode, offset, source, aModel );
            presentation.properties.insert( presentation.properties.end(),
                                            std::make_move_iterator( preservedPresentation.begin() ),
                                            std::make_move_iterator( preservedPresentation.end() ) );

            if( ( ownerHandle & 0xF000 ) == 0x2000 )
            {
                const size_t offpageRecord = ownerHandle & 0x0FFF;
                auto         label = std::ranges::find_if( aModel.labels,
                                                           [&]( const MODEL_LABEL& aLabel )
                                                           {
                                                       return aLabel.sheet.id == aModel.sheets[aDecode.sheetIndex].id
                                                              && aLabel.source.controller == 20
                                                              && aLabel.source.recordIndex == offpageRecord;
                                                   } );

                if( label == aModel.labels.end() )
                {
                    const MODEL_BUS_ENTRY* busEntry = nullptr;

                    for( const MODEL_BUS& bus : aModel.buses )
                    {
                        if( bus.sheet.id != aModel.sheets[aDecode.sheetIndex].id )
                            continue;

                        auto candidate = std::ranges::find_if( bus.entries,
                                                               [&]( const MODEL_BUS_ENTRY& aEntry )
                                                               {
                                                                   return aEntry.source.recordIndex == offpageRecord
                                                                          && aEntry.memberNet.id == ownerNet.id;
                                                               } );

                        if( candidate != bus.entries.end() )
                        {
                            if( busEntry )
                                throwDecodeError( source, wxS( "net-name bus-entry owner is ambiguous" ) );

                            busEntry = &*candidate;
                        }
                    }

                    if( !busEntry )
                        throwDecodeError( source, wxS( "net-name off-page owner targets wrong object class" ) );

                    MODEL_TEXT busText;
                    busText.source = source;
                    busText.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
                    busText.text = ownerNet.name;
                    busText.position = { busEntry->position.x + textOffset.x, busEntry->position.y + textOffset.y,
                                         source };
                    busText.angle = NormalizeAngle( aDecode.cursor.U16At( offset + 24 ) );
                    busText.presentation = std::move( presentation );
                    busText.properties.push_back( sourceProperty(
                            wxS( "net_name_text_offset" ),
                            wxString::Format( wxS( "%lld,%lld" ), textOffset.x, textOffset.y ), source ) );
                    aModel.texts.push_back( std::move( busText ) );
                    continue;
                }

                if( label->text.text != ownerNet.name.text )
                    throwDecodeError( source, wxS( "net-name off-page owner targets wrong net object" ) );

                label->presentation = std::move( presentation );
                label->textOffset = textOffset;
                label->properties.push_back(
                        sourceProperty( wxS( "net_name_text_offset" ),
                                        wxString::Format( wxS( "%lld,%lld" ), textOffset.x, textOffset.y ), source ) );
                continue;
            }

            auto placement =
                    std::ranges::find_if( aModel.placements,
                                          [&]( const MODEL_PLACEMENT& aPlacement )
                                          {
                                              return aPlacement.sheet.id == aModel.sheets[aDecode.sheetIndex].id
                                                     && aPlacement.source.recordIndex == ownerHandle;
                                          } );

            if( placement == aModel.placements.end() )
                throwDecodeError( source, wxS( "net-name placement owner targets wrong object class" ) );

            if( childHandle >= placement->pins.size() )
                throwDecodeError( source, wxS( "net-name placement pin ordinal leaves placement" ) );

            const PIN_ID ownerPin = placement->pins[childHandle].id;

            const MODEL_CONNECTION_ENDPOINT* endpoint = nullptr;

            for( const MODEL_CONNECTION& connection : ownerNet.connections )
            {
                for( const MODEL_CONNECTION_ENDPOINT& candidate : connection.endpoints )
                {
                    if( candidate.kind != MODEL_ENDPOINT_KIND::PIN || !candidate.placement || !candidate.pin
                        || candidate.placement->id != placement->id || candidate.pin->id != ownerPin )
                    {
                        continue;
                    }

                    if( endpoint )
                        throwDecodeError( source, wxS( "net-name placement pin owner is ambiguous" ) );

                    endpoint = &candidate;
                }
            }

            if( !endpoint )
                throwDecodeError( source, wxS( "net-name placement pin owner is unresolved" ) );

            MODEL_TEXT text;
            text.source = source;
            text.sheet = { aModel.sheets[aDecode.sheetIndex].id, source };
            text.text = ownerNet.name;
            text.position = { endpoint->point.x + textOffset.x, endpoint->point.y + textOffset.y, source };
            text.angle = NormalizeAngle( aDecode.cursor.U16At( offset + 24 ) );
            text.presentation = std::move( presentation );
            text.properties.push_back(
                    sourceProperty( wxS( "net_name_text_offset" ),
                                    wxString::Format( wxS( "%lld,%lld" ), textOffset.x, textOffset.y ), source ) );
            aModel.texts.push_back( std::move( text ) );
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
        requireFixedController( controllers, 23, NET_NAME_RECORD_BYTES, aSourceName, aModel.version, aSheetIndex );

        const CONNECTIVITY_DECODE decode{ aBytes,
                                          aCursor,
                                          controllers,
                                          aGlobals,
                                          aSourceName,
                                          aSheetIndex,
                                          controllers.offsets[17],
                                          controllers.offsets[18],
                                          controllers.offsets[19],
                                          controllers.offsets[20],
                                          controllers.offsets[21],
                                          controllers.offsets[22] };

        const std::unordered_set<uint32_t> busGlobalRecords = decodeBusGlobalRecords( decode, aModel );
        SHEET_CONNECTIVITY                 connectivity;
        connectivity.sheetNets = materializeSheetNets( decode, busGlobalRecords, aModel );

        const std::unordered_map<uint32_t, const MODEL_PIN_DEFINITION*> definitionPins = indexDefinitionPins( aModel );
        const std::vector<MODEL_PLACEMENT*> placements = indexSheetPlacements( decode, aModel );

        decodeJunctions( decode, aModel );

        const VERTEX_TILING tiling = tileConnectivityVertices( decode, aModel );

        decodeConnections( decode, tiling, placements, definitionPins, connectivity, aModel );
        validateJunctionBacklinks( decode, connectivity, aModel );
        validateOffpageBacklinks( decode, connectivity, aModel );

        const std::vector<bool> claimedBusEntries =
                decodeBuses( decode, tiling, busGlobalRecords, connectivity, aModel );

        decodeOffpageLabels( decode, connectivity, claimedBusEntries, aModel );
        decodeNetNames( decode, connectivity, aModel );
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


    template <typename Item>
    void validateSheetRef( const Item& aItem, const MODEL_REFERENCE_INDEX& aIndex, const wxString& aObjectClass )
    {
        if( !aIndex.sheets.contains( aItem.sheet.id.Value() ) )
        {
            throwValidationError( aItem.sheet.source,
                                  wxString::Format( wxS( "unresolved %s sheet reference" ), aObjectClass ) );
        }
    }


    void validateIdentity( const PADS_SCH_MODEL& aModel )
    {
        validateUniqueIds( aModel.sheets, wxS( "sheet" ), itemId, itemProvenance );
        validateUniqueIds( aModel.definitions, wxS( "definition" ), itemId, itemProvenance );
        validateUniqueIds( aModel.partTypes, wxS( "part type" ), itemId, itemProvenance );
        validateUniqueIds( aModel.placements, wxS( "placement" ), itemId, itemProvenance );
        validateUniqueIds( aModel.nets, wxS( "net" ), itemId, itemProvenance );
        validateUniqueIds( aModel.buses, wxS( "bus" ), itemId, itemProvenance );
        validateUniqueIds( aModel.images, wxS( "embedded image" ), itemId, itemProvenance );

        std::vector<bool> sheetIndexes( aModel.sheets.size() );

        for( const MODEL_SHEET& sheet : aModel.sheets )
        {
            if( sheet.index >= sheetIndexes.size() )
                throwValidationError( sheet.source, wxS( "sheet source index leaves the declared sheet range" ) );

            if( sheetIndexes[sheet.index] )
                throwValidationError( sheet.source, wxS( "duplicate sheet source index" ) );

            sheetIndexes[sheet.index] = true;
        }

        std::unordered_map<uint32_t, SOURCE_PROVENANCE> gateDeclarations;
        std::unordered_map<uint32_t, SOURCE_PROVENANCE> pinDeclarations;
        std::unordered_map<uint64_t, SOURCE_PROVENANCE> fieldDeclarations;

        for( const MODEL_PART_TYPE& partType : aModel.partTypes )
        {
            for( const MODEL_GATE& gate : partType.gates )
                validateNestedId( gate, wxS( "gate" ), gateDeclarations );
        }

        for( const MODEL_SYMBOL_DEFINITION& definition : aModel.definitions )
        {
            for( const MODEL_PIN_DEFINITION& pin : definition.pins )
                validateNestedId( pin, wxS( "pin" ), pinDeclarations );

            for( const MODEL_FIELD& field : definition.fields )
                validateNestedId( field, wxS( "field" ), fieldDeclarations );
        }

        for( const MODEL_SHEET& sheet : aModel.sheets )
        {
            for( const MODEL_FIELD& field : sheet.titleBlockFields )
                validateNestedId( field, wxS( "field" ), fieldDeclarations );
        }

        for( const MODEL_PART_TYPE& partType : aModel.partTypes )
        {
            for( const MODEL_FIELD& field : partType.fields )
                validateNestedId( field, wxS( "field" ), fieldDeclarations );
        }

        for( const MODEL_PLACEMENT& placement : aModel.placements )
        {
            for( const MODEL_FIELD& field : placement.fields )
                validateNestedId( field, wxS( "field" ), fieldDeclarations );
        }
    }


    void validateSymbolGraph( const PADS_SCH_MODEL& aModel, const MODEL_REFERENCE_INDEX& aIndex )
    {
        for( const MODEL_SHEET& sheet : aModel.sheets )
        {
            if( sheet.parent && !aIndex.sheets.contains( sheet.parent->id.Value() ) )
                throwValidationError( sheet.parent->source, wxS( "unresolved sheet reference" ) );
        }

        for( const MODEL_PART_TYPE& partType : aModel.partTypes )
        {
            for( const MODEL_GATE& gate : partType.gates )
            {
                auto definition = aIndex.definitions.find( gate.definition.id.Value() );

                if( definition == aIndex.definitions.end() && gate.decalGroupMembers.empty() )
                    throwValidationError( gate.definition.source, wxS( "unresolved symbol definition reference" ) );

                for( const DEFINITION_REFERENCE& alternate : gate.alternateDefinitions )
                {
                    if( !aIndex.definitions.contains( alternate.id.Value() ) )
                        throwValidationError( alternate.source, wxS( "unresolved alternate definition reference" ) );
                }

                for( const DEFINITION_REFERENCE& member : gate.decalGroupMembers )
                {
                    if( !aIndex.definitions.contains( member.id.Value() ) )
                        throwValidationError( member.source, wxS( "unresolved pin-decal group member" ) );
                }

                if( definition == aIndex.definitions.end() )
                    continue;

                if( !gate.logicalPins.empty() && gate.logicalPins.size() != gate.pins.size() )
                    throwValidationError( gate.source, wxS( "gate logical-pin count does not match definition pins" ) );

                for( size_t pinOrdinal = 0; pinOrdinal < gate.pins.size(); ++pinOrdinal )
                {
                    const PIN_REFERENCE& pin = gate.pins[pinOrdinal];
                    auto                 pinOwner = aIndex.pinOwners.find( pin.id.Value() );

                    if( pinOwner == aIndex.pinOwners.end() || pinOwner->second != definition->second )
                        throwValidationError( pin.source, wxS( "pin does not belong to gate definition" ) );

                    if( !gate.logicalPins.empty() && gate.logicalPins[pinOrdinal].definitionPin.id != pin.id )
                        throwValidationError( gate.logicalPins[pinOrdinal].definitionPin.source,
                                              wxS( "logical pin does not belong to gate definition pin" ) );
                }
            }
        }

        struct DEFINITION_EDGE
        {
            uint32_t          target;
            SOURCE_PROVENANCE source;
        };

        std::unordered_map<uint32_t, std::vector<DEFINITION_EDGE>> definitionEdges;

        for( const MODEL_PART_TYPE& partType : aModel.partTypes )
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
        std::function<void( uint32_t )>       visitDefinition =
                [&]( uint32_t aDefinition )
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
    }


    void validatePlacements( const PADS_SCH_MODEL& aModel, const MODEL_REFERENCE_INDEX& aIndex )
    {
        for( const MODEL_PLACEMENT& placement : aModel.placements )
        {
            if( !aIndex.sheets.contains( placement.sheet.id.Value() ) )
                throwValidationError( placement.sheet.source, wxS( "unresolved placement sheet reference" ) );

            auto partType = aIndex.partTypes.find( placement.partType.id.Value() );

            if( partType == aIndex.partTypes.end() )
                throwValidationError( placement.partType.source, wxS( "unresolved placement part-type reference" ) );

            auto placementDefinition = aIndex.definitions.find( placement.definition.id.Value() );

            if( placementDefinition == aIndex.definitions.end() )
                throwValidationError( placement.definition.source, wxS( "unresolved placement definition" ) );

            for( const PIN_REFERENCE& pin : placement.pins )
            {
                auto owner = aIndex.pinOwners.find( pin.id.Value() );

                if( owner == aIndex.pinOwners.end() || owner->second != placementDefinition->second )
                    throwValidationError( pin.source, wxS( "placement pin does not belong to selected definition" ) );
            }

            if( !placement.gate )
                continue;

            auto gate = aIndex.gates.find( placement.gate->id.Value() );
            auto gateOwner = aIndex.gateOwners.find( placement.gate->id.Value() );

            if( gate == aIndex.gates.end() || gateOwner == aIndex.gateOwners.end()
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

            auto definition = aIndex.definitions.find( placement.definition.id.Value() );

            if( definition == aIndex.definitions.end() )
                throwValidationError( placement.definition.source, wxS( "unresolved placement definition" ) );

            for( const PIN_REFERENCE& pin : placement.pins )
            {
                auto owner = aIndex.pinOwners.find( pin.id.Value() );

                if( owner == aIndex.pinOwners.end() || owner->second != definition->second )
                    throwValidationError( pin.source, wxS( "placement pin does not belong to selected definition" ) );
            }
        }
    }


    void validateSheetBoundContent( const PADS_SCH_MODEL& aModel, const MODEL_REFERENCE_INDEX& aIndex )
    {
        for( const MODEL_NET& net : aModel.nets )
        {
            validateSheetRef( net, aIndex, wxS( "net" ) );

            for( const MODEL_CONNECTION& connection : net.connections )
            {
                if( connection.endpoints.empty() )
                    throwValidationError( connection.source, wxS( "connection has no endpoints" ) );

                for( const MODEL_CONNECTION_ENDPOINT& endpoint : connection.endpoints )
                {
                    if( !endpointIsValid( aIndex, endpoint ) )
                    {
                        throwValidationError( endpoint.source,
                                              wxS( "empty, mixed, or unresolved connection endpoint" ) );
                    }

                    if( !endpoint.placement )
                        continue;

                    auto placement = aIndex.placements.find( endpoint.placement->id.Value() );

                    if( placement != aIndex.placements.end() && placement->second->sheet.id != net.sheet.id )
                    {
                        throwValidationError( endpoint.source,
                                              wxS( "connection endpoint placement sheet does not match net sheet" ) );
                    }
                }
            }
        }

        for( const MODEL_BUS& bus : aModel.buses )
        {
            validateSheetRef( bus, aIndex, wxS( "bus" ) );

            for( const NET_REFERENCE& member : bus.memberNets )
            {
                auto net = aIndex.nets.find( member.id.Value() );

                if( net == aIndex.nets.end() )
                    throwValidationError( member.source, wxS( "unresolved bus member-net reference" ) );

                if( net->second->sheet.id != bus.sheet.id )
                    throwValidationError( member.source, wxS( "bus member-net sheet does not match bus sheet" ) );
            }

            for( const MODEL_BUS_ENTRY& entry : bus.entries )
            {
                auto net = aIndex.nets.find( entry.memberNet.id.Value() );

                if( net == aIndex.nets.end() )
                    throwValidationError( entry.memberNet.source, wxS( "unresolved bus-entry net reference" ) );

                if( net->second->sheet.id != bus.sheet.id )
                    throwValidationError( entry.memberNet.source,
                                          wxS( "bus-entry net sheet does not match bus sheet" ) );

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

        for( const MODEL_LABEL& label : aModel.labels )
        {
            validateSheetRef( label, aIndex, wxS( "label" ) );

            for( const SHEET_REFERENCE& linkedSheet : label.linkedSheets )
            {
                if( !aIndex.sheets.contains( linkedSheet.id.Value() ) )
                    throwValidationError( linkedSheet.source, wxS( "unresolved cross-sheet label reference" ) );
            }
        }

        for( const MODEL_JUNCTION& junction : aModel.junctions )
            validateSheetRef( junction, aIndex, wxS( "junction" ) );

        for( const MODEL_TEXT& text : aModel.texts )
            validateSheetRef( text, aIndex, wxS( "text" ) );

        for( const MODEL_PAGE_GRAPHIC& graphic : aModel.graphics )
            validateSheetRef( graphic, aIndex, wxS( "page-graphic" ) );

        std::set<uint32_t> worksheetSheets;

        for( const MODEL_WORKSHEET& worksheet : aModel.worksheets )
        {
            validateSheetRef( worksheet, aIndex, wxS( "worksheet" ) );

            if( !worksheetSheets.insert( worksheet.sheet.id.Value() ).second )
                throwValidationError( worksheet.source, wxS( "duplicate worksheet for sheet" ) );

            if( worksheet.graphics.empty() )
                throwValidationError( worksheet.source, wxS( "worksheet has no graphics" ) );
        }

        for( const MODEL_EMBEDDED_IMAGE& image : aModel.images )
        {
            validateSheetRef( image, aIndex, wxS( "embedded-image" ) );

            if( image.type != MODEL_EMBEDDED_IMAGE_TYPE::UNSUPPORTED && image.data.empty() )
                throwValidationError( image.source, wxS( "embedded image has no decoded payload" ) );
        }
    }


    void validateSheetHierarchy( const PADS_SCH_MODEL& aModel, const MODEL_REFERENCE_INDEX& aIndex )
    {
        enum class VISIT_STATE : uint8_t
        {
            VISITING,
            COMPLETE
        };
        std::unordered_map<uint32_t, VISIT_STATE> visitStates;

        for( const MODEL_SHEET& sheet : aModel.sheets )
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

                auto parent = aIndex.sheets.find( current->parent->id.Value() );

                if( parent == aIndex.sheets.end() )
                    break;

                current = parent->second;
            }

            if( current && visitStates.at( current->id.Value() ) == VISIT_STATE::VISITING )
                throwValidationError( current->source, wxS( "cyclic sheet hierarchy" ) );

            for( const MODEL_SHEET* visited : path )
                visitStates[visited->id.Value()] = VISIT_STATE::COMPLETE;
        }
    }


    std::vector<MODEL_FIELD> decodeDesignSettings( const std::vector<uint8_t>&   aBytes,
                                                   const PADS_IO::BINARY_CURSOR& aCursor, const PADS_SCH_SDB& aSdb,
                                                   const wxString& aSourceName, PADS_SCH_MODEL& aModel )
    {
        const SCH_SDB_POOL& sheetPool = aSdb.Pools()[3];

        if( sheetPool.usedBytes != sheetPool.count * SHEET_RECORD_BYTES )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "sheet index" ), 3, 0,
                              OUTER_DIRECTORY_OFFSET + 3 * OUTER_DESCRIPTOR_BYTES + OUTER_USED_BYTES_OFFSET, 4, -1 );
            throwDecodeError( source, wxS( "sheet-index byte count does not match 48-byte record count" ) );
        }

        const SCH_SDB_POOL& settingsPool = aSdb.Pools()[5];

        if( settingsPool.count != 100 || settingsPool.usedBytes != 400 )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "design settings" ), 5, 0,
                              OUTER_DIRECTORY_OFFSET + 5 * OUTER_DESCRIPTOR_BYTES + OUTER_USED_BYTES_OFFSET, 4, -1 );
            throwDecodeError( source, wxS( "design-settings controller is not the required 400-byte record" ) );
        }

        const size_t      settingsOffset = outerControllerOffset( aSdb, 5 );
        SOURCE_PROVENANCE settingsSource = sourceAt( aSourceName, aModel.version, wxS( "design settings" ), 5, 0,
                                                     settingsOffset, settingsPool.usedBytes, -1 );
        aModel.settings.source = settingsSource;

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
        aModel.settings.pageSize = pageExtent( pageDesignator, pageSource );
        aModel.settings.defaultLineWidth = static_cast<int64_t>( aCursor.U32At( settingsOffset + 12 ) ) * 2;
        aModel.settings.defaultBusWidth = static_cast<int64_t>( aCursor.U32At( settingsOffset + 16 ) ) * 2;

        std::vector<MODEL_FIELD> titleFields;
        const SCH_SDB_POOL&      titleFieldPool = aSdb.Pools()[1];
        size_t                   titleOffset = outerControllerOffset( aSdb, 1 );
        const size_t             titleEnd = titleOffset + titleFieldPool.usedBytes;
        size_t                   titleRecord = 0;

        while( titleOffset + 6 <= titleEnd
               && std::equal( aBytes.begin() + titleOffset, aBytes.begin() + titleOffset + 6, "Field\n" ) )
        {
            size_t terminator = titleOffset;

            while( terminator < titleEnd && aBytes[terminator] != 0 )
                ++terminator;

            SOURCE_PROVENANCE source = sourceAt( aSourceName, aModel.version, wxS( "title field" ), 1, titleRecord,
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
            field.name = PADS_SCH_BINARY_PARSER::DecodeString( nameBytes, DEFAULT_CODE_PAGE, nameSource,
                                                               aModel.diagnostics );
            field.value = PADS_SCH_BINARY_PARSER::DecodeString( valueBytes, DEFAULT_CODE_PAGE, valueSource,
                                                                aModel.diagnostics );
            field.presentation.source = source;
            titleFields.push_back( std::move( field ) );
            titleOffset = terminator + 1;
            ++titleRecord;
        }

        if( titleFields.empty() )
        {
            SOURCE_PROVENANCE source =
                    sourceAt( aSourceName, aModel.version, wxS( "title field" ), 1, titleFields.size(),
                              outerControllerOffset( aSdb, 1 ), titleFieldPool.usedBytes, -1 );
            throwDecodeError( source, wxS( "title-field controller is empty" ) );
        }

        return titleFields;
    }


    void decodeSheets( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                       const PADS_SCH_SDB& aSdb, const wxString& aSourceName,
                       const std::vector<MODEL_FIELD>& aTitleFields, PADS_SCH_MODEL& aModel )
    {
        const size_t sheetIndexOffset = outerControllerOffset( aSdb, 3 );
        auto         sheetBlocks = aSdb.Blocks()
                           | std::views::filter(
                                   []( const SCH_SDB_BLOCK& aBlock )
                                   {
                                       return aBlock.kind == SCH_SDB_BLOCK_KIND::SHEET;
                                   } );
        auto sheetBlock = sheetBlocks.begin();

        for( size_t index = 0; index < aSdb.Pools()[3].count; ++index, ++sheetBlock )
        {
            size_t            recordOffset = sheetIndexOffset + index * SHEET_RECORD_BYTES;
            SOURCE_PROVENANCE provenance = sourceAt( aSourceName, aModel.version, wxS( "sheet" ), 3, index,
                                                     recordOffset, SHEET_RECORD_BYTES, static_cast<int>( index ) );

            if( sheetBlock == sheetBlocks.end() || aCursor.U32At( recordOffset ) != sheetBlock->offset
                || aCursor.U32At( recordOffset + 4 ) != sheetBlock->bytes )
            {
                throwDecodeError( provenance, wxS( "sheet-index record references the wrong SDB object class" ) );
            }

            constexpr size_t nameOffset = 14;

            if( aCursor.U16At( recordOffset + 10 ) != 0xFFFF || aCursor.U16At( recordOffset + 12 ) != 0xFFFF )
                throwDecodeError( provenance, wxS( "invalid sheet-index class marker" ) );

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
            sheet.id = SHEET_ID( aCursor.U16At( recordOffset + 8 ) );
            sheet.index = sheet.id.IsValid() ? sheet.id.Value() - 1 : std::numeric_limits<size_t>::max();
            sheet.source = provenance;
            sheet.name = PADS_SCH_BINARY_PARSER::DecodeString( nameBytes, DEFAULT_CODE_PAGE, nameSource,
                                                               aModel.diagnostics );

            if( sheet.name.text.empty() )
                sheet.name.text = wxS( "$$$NONE" );

            sheet.pageSize = aModel.settings.pageSize;
            sheet.defaultLineWidth = aModel.settings.defaultLineWidth;
            sheet.defaultBusWidth = aModel.settings.defaultBusWidth;
            sheet.titleBlockFields = aTitleFields;

            auto title = std::ranges::find_if( sheet.titleBlockFields,
                                               []( const MODEL_FIELD& aField )
                                               {
                                                   return aField.name.text == wxS( "Title" );
                                               } );

            if( title != sheet.titleBlockFields.end() )
                sheet.title = title->value;

            aModel.sheets.push_back( std::move( sheet ) );
        }
    }


    void decodeFreeText( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                         const wxString& aSourceName, size_t aSheetIndex, size_t aTextBase, uint32_t aTextCount,
                         size_t aHeapBase, uint32_t aHeapBytes, const PLACEMENT_GLOBALS& aGlobals,
                         PADS_SCH_MODEL& aModel )
    {
        for( size_t record = 0; record < aTextCount; ++record )
        {
            size_t recordOffset = aTextBase + record * TEXT_RECORD_BYTES;

            if( aCursor.U16At( recordOffset + 24 ) != aCursor.U16At( recordOffset + 26 ) )
                continue;

            SOURCE_PROVENANCE textSource = sourceAt( aSourceName, aModel.version, wxS( "free text" ), 1, record,
                                                     recordOffset, TEXT_RECORD_BYTES, static_cast<int>( aSheetIndex ) );
            uint32_t          stringOffset = aCursor.U32At( recordOffset + 8 );
            uint16_t          stringBytes = aCursor.U16At( recordOffset + 20 );

            if( stringOffset > aHeapBytes )
            {
                SOURCE_PROVENANCE offsetSource = textSource;
                offsetSource.absoluteOffset += 8;
                offsetSource.length = 4;
                throwDecodeError( offsetSource, wxS( "free-text string offset leaves controller 2" ) );
            }

            if( stringBytes == 0 || stringBytes > aHeapBytes - stringOffset )
            {
                SOURCE_PROVENANCE lengthSource = textSource;
                lengthSource.absoluteOffset += 20;
                lengthSource.length = 2;
                throwDecodeError( lengthSource, wxS( "free-text string length leaves controller 2" ) );
            }

            if( aBytes[aHeapBase + stringOffset + stringBytes - 1] != 0 )
            {
                SOURCE_PROVENANCE terminatorSource =
                        sourceAt( aSourceName, aModel.version, wxS( "free text string terminator" ), 2, record,
                                  aHeapBase + stringOffset + stringBytes - 1, 1, static_cast<int>( aSheetIndex ) );
                throwDecodeError( terminatorSource, wxS( "free-text string is not NUL terminated" ) );
            }

            SOURCE_PROVENANCE stringSource =
                    sourceAt( aSourceName, aModel.version, wxS( "free text string" ), 2, record,
                              aHeapBase + stringOffset, stringBytes - 1, static_cast<int>( aSheetIndex ) );
            std::vector<uint8_t> string( aBytes.begin() + stringSource.absoluteOffset,
                                         aBytes.begin() + stringSource.absoluteOffset + stringSource.length );
            uint16_t             justification = aCursor.U16At( recordOffset + 18 );

            MODEL_TEXT text;
            text.source = textSource;
            text.sheet = { aModel.sheets[aSheetIndex].id, textSource };
            text.text =
                    PADS_SCH_BINARY_PARSER::DecodeString( string, DEFAULT_CODE_PAGE, stringSource, aModel.diagnostics );
            text.position = { decodeCoordinate( aCursor.U16At( recordOffset + 12 ) ),
                              decodeCoordinate( aCursor.U16At( recordOffset + 14 ) ), textSource };
            text.angle = NormalizeAngle( aCursor.U16At( recordOffset + 16 ) );
            text.presentation.source = textSource;
            text.presentation.height = aCursor.U16At( recordOffset + 22 );
            text.presentation.width = aCursor.U8At( recordOffset + 30 );
            const uint8_t displayFlags = aCursor.U8At( recordOffset + 31 );
            text.presentation.visible = ( displayFlags & 1 ) == 0;
            text.presentation.horizontalJustification = freeTextHorizontalJustification( justification );
            text.presentation.verticalJustification = MODEL_JUSTIFICATION::CENTER;

            SOURCE_PROVENANCE fontHandleSource = textSource;
            fontHandleSource.length = 2;
            decodeGlobalFont( aBytes, aCursor, aGlobals, aSourceName, aModel.version,
                              static_cast<int16_t>( aCursor.U16At( recordOffset ) ), fontHandleSource,
                              text.presentation, false, aModel.diagnostics );

            SOURCE_PROVENANCE displaySource = textSource;
            displaySource.absoluteOffset += 31;
            displaySource.length = 1;
            text.presentation.properties.push_back( sourceProperty(
                    wxS( "display_flags" ), wxString::Format( wxS( "%u" ), displayFlags ), displaySource ) );

            SOURCE_PROVENANCE relationshipSource = textSource;
            relationshipSource.objectClass = wxS( "free text relationship" );
            relationshipSource.absoluteOffset += 28;
            relationshipSource.length = 2;
            uint16_t        relationship = aCursor.U16At( relationshipSource.absoluteOffset );
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
            aModel.texts.push_back( std::move( text ) );
        }
    }


    void decodeSheetBlocks( const std::vector<uint8_t>& aBytes, const PADS_IO::BINARY_CURSOR& aCursor,
                            const PADS_SCH_SDB& aSdb, const wxString& aSourceName, PADS_SCH_MODEL& aModel )
    {
        size_t                              sheetIndex = 0;
        std::optional<PLACEMENT_GLOBALS>    placementData;
        std::optional<CONNECTIVITY_GLOBALS> connectivityData;
        const PLACEMENT_LAYOUT&             placementSchema = placementLayout( aModel.version );

        if( placementSchema.decoded )
        {
            placementData = placementGlobals( aSdb, aSourceName );
            connectivityData = connectivityGlobals( aBytes, aCursor, aSdb, aSourceName, aModel );
        }

        for( const SCH_SDB_BLOCK& block : aSdb.Blocks() )
        {
            if( block.kind != SCH_SDB_BLOCK_KIND::SHEET )
                continue;

            size_t   descriptors = block.offset + SHEET_HEADER_BYTES;
            size_t   payload = descriptors + SHEET_DESCRIPTOR_COUNT * SHEET_DESCRIPTOR_BYTES;
            uint32_t textCount = aCursor.U32At( descriptors + SHEET_COUNT_OFFSET );
            uint32_t textBytes = aCursor.U32At( descriptors + SHEET_USED_BYTES_OFFSET );
            uint32_t heapBytes = aCursor.U32At( descriptors + SHEET_DESCRIPTOR_BYTES + SHEET_USED_BYTES_OFFSET );

            SOURCE_PROVENANCE controllerSource = sourceAt( aSourceName, aModel.version, wxS( "text controller" ), 1, 0,
                                                           payload, textBytes, static_cast<int>( sheetIndex ) );

            if( textBytes != textCount * TEXT_RECORD_BYTES )
                throwDecodeError( controllerSource,
                                  wxS( "text-controller byte count does not match 32-byte records" ) );

            size_t heapOffset = payload + textBytes;

            if( !placementSchema.decoded )
            {
                SOURCE_PROVENANCE heapSource = sourceAt( aSourceName, aModel.version, wxS( "text string controller" ),
                                                         2, 0, heapOffset, heapBytes, static_cast<int>( sheetIndex ) );
                aModel.preservedControllerPayloads.push_back(
                        { controllerSource,
                          PROPERTY_DISPOSITION::PRESERVED,
                          { aBytes.begin() + payload, aBytes.begin() + payload + textBytes } } );
                aModel.preservedControllerPayloads.push_back(
                        { heapSource,
                          PROPERTY_DISPOSITION::PRESERVED,
                          { aBytes.begin() + heapOffset, aBytes.begin() + heapOffset + heapBytes } } );
                decodeDefinitionsAndParts( aBytes, aCursor, block, sheetIndex, aSourceName, aModel );
                ++sheetIndex;
                continue;
            }

            decodeFreeText( aBytes, aCursor, aSourceName, sheetIndex, payload, textCount, heapOffset, heapBytes,
                            *placementData, aModel );
            decodeDefinitionsAndParts( aBytes, aCursor, block, sheetIndex, aSourceName, aModel );
            decodePlacements( aBytes, aCursor, block, sheetIndex, aSourceName, *placementData, aModel );
            decodeConnectivity( aBytes, aCursor, block, sheetIndex, aSourceName, *connectivityData, aModel );
            ++sheetIndex;
        }
    }


    void decodeOleImages( const std::vector<uint8_t>& aBytes, const PADS_SCH_SDB& aSdb, const wxString& aSourceName,
                          PADS_SCH_MODEL& aModel )
    {
        for( size_t index = 0; index < aSdb.OleItems().size(); ++index )
        {
            const SCH_SDB_OLE_ITEM& item = aSdb.OleItems()[index];
            SOURCE_PROVENANCE       source =
                    sourceAt( aSourceName, aModel.version, wxS( "embedded OLE image" ), item.cfb.controller, index,
                              item.cfb.offset, item.cfb.bytes, static_cast<int>( item.sheetPlane ) );
            OLE_IMAGE_PAYLOAD    payload = ExtractOleImage( aBytes.data() + item.cfb.offset, item.cfb.bytes );
            MODEL_EMBEDDED_IMAGE image;
            image.id = IMAGE_ID( index );
            image.source = source;
            image.sheet = { aModel.sheets[item.sheetPlane].id, source };
            image.streamName = wxString::FromUTF8( payload.streamName );
            image.extent = item.extent;
            image.databaseBox = { item.left, item.bottom, item.right, item.top };
            SOURCE_PROVENANCE boxSource = source;
            boxSource.objectClass = wxS( "embedded OLE image database box" );
            boxSource.absoluteOffset = item.boxOffset;
            boxSource.length = 16;
            int64_t left = decodeDatabaseCoordinate( item.left, boxSource );
            int64_t bottom = decodeDatabaseCoordinate( item.bottom, boxSource );
            int64_t right = decodeDatabaseCoordinate( item.right, boxSource );
            int64_t top = decodeDatabaseCoordinate( item.top, boxSource );

            image.position = { left + ( right - left ) / 2, top + ( bottom - top ) / 2, boxSource };
            image.size = { std::abs( right - left ), std::abs( bottom - top ), boxSource };
            image.mirrorHorizontal = right < left;
            image.mirrorVertical = bottom < top;
            image.flags = item.flags;
            image.data = std::move( payload.data );

            switch( payload.type )
            {
            case OLE_IMAGE_TYPE::BMP: image.type = MODEL_EMBEDDED_IMAGE_TYPE::BMP; break;
            case OLE_IMAGE_TYPE::DIB: image.type = MODEL_EMBEDDED_IMAGE_TYPE::DIB; break;
            case OLE_IMAGE_TYPE::WMF: image.type = MODEL_EMBEDDED_IMAGE_TYPE::WMF; break;
            case OLE_IMAGE_TYPE::NONE:
                image.type = MODEL_EMBEDDED_IMAGE_TYPE::UNSUPPORTED;
                aModel.diagnostics.emplace_back(
                        RPT_SEVERITY_WARNING, source,
                        wxS( "embedded OLE object has no supported BMP, DIB, or WMF stream" ) );
                break;
            }

            if( right == left || bottom == top )
            {
                image.type = MODEL_EMBEDDED_IMAGE_TYPE::UNSUPPORTED;
                aModel.diagnostics.emplace_back(
                        RPT_SEVERITY_WARNING, boxSource,
                        wxS( "embedded OLE image has a zero-size database box and was skipped" ) );
            }

            if( item.flags != 1 )
            {
                SOURCE_PROVENANCE flagsSource = source;
                flagsSource.objectClass = wxS( "embedded OLE image flags" );
                flagsSource.absoluteOffset = item.boxOffset + 20;
                flagsSource.length = 4;
                PADS_SCH_BINARY_PARSER::RecordUnknownEnum( wxS( "embedded OLE image flags" ), item.flags, flagsSource,
                                                           aModel.diagnostics );
            }

            aModel.images.push_back( std::move( image ) );
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


bool DIAGNOSTIC_PROPERTY_KEY::operator<( const DIAGNOSTIC_PROPERTY_KEY& aOther ) const
{
    return std::tie( source.file, source.version, source.objectClass, source.controller, source.recordIndex,
                     source.absoluteOffset, source.length, source.sheet, property.name, property.disposition )
           < std::tie( aOther.source.file, aOther.source.version, aOther.source.objectClass, aOther.source.controller,
                       aOther.source.recordIndex, aOther.source.absoluteOffset, aOther.source.length,
                       aOther.source.sheet, aOther.property.name, aOther.property.disposition );
}


std::optional<DIAGNOSTIC_PROPERTY_KEY> DiagnosticPropertyKey( const PARSER_DIAGNOSTIC& aDiagnostic )
{
    if( !aDiagnostic.property )
        return std::nullopt;

    return DIAGNOSTIC_PROPERTY_KEY{ aDiagnostic.source, *aDiagnostic.property };
}


PARSER_DIAGNOSTIC MakePropertyDiagnostic( SEVERITY aSeverity, const SOURCE_PROPERTY& aProperty,
                                          const wxString& aMessage )
{
    return MakePropertyDiagnostic( aSeverity, aProperty.source, aProperty.name.text, aProperty.disposition, aMessage );
}


PARSER_DIAGNOSTIC MakePropertyDiagnostic( SEVERITY aSeverity, const SOURCE_PROVENANCE& aSource, const wxString& aName,
                                          PROPERTY_DISPOSITION aDisposition, const wxString& aMessage )
{
    return { aSeverity, aSource, aMessage, DIAGNOSTIC_PROPERTY_IDENTITY{ aName, aDisposition } };
}


void PADS_SCH_MODEL::ValidateOrThrow() const
{
    validateIdentity( *this );

    const MODEL_REFERENCE_INDEX index( *this );

    validateSymbolGraph( *this, index );
    validatePlacements( *this, index );
    validateSheetBoundContent( *this, index );
    validateSheetHierarchy( *this, index );
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

    const std::vector<MODEL_FIELD> titleFields = decodeDesignSettings( aBytes, cursor, sdb, aSourceName, model );

    decodeSheets( aBytes, cursor, sdb, aSourceName, titleFields, model );
    decodeSheetBlocks( aBytes, cursor, sdb, aSourceName, model );
    decodeOleImages( aBytes, sdb, aSourceName, model );
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
    const wxString defaultName = aCodePage == 65001  ? wxString( wxS( "UTF-8" ) )
                                 : aCodePage == 1252 ? wxString( wxS( "windows-1252" ) )
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

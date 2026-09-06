/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * Based on the dsn2kicad reference implementation and on OrCAD file format
 * documentation from the OpenOrCadParser project (MIT licensed).
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */



#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/ole_image.h>

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <chrono>
#include <cmath>
#include <functional>
#include <numeric>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <wx/buffer.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/log.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>

#include <base_units.h>
#include <bitmap_base.h>
#include <connection_graph.h>
#include <ki_exception.h>
#include <kiid.h>
#include <layer_ids.h>
#include <lib_symbol.h>
#include <math/util.h>
#include <page_info.h>
#include <progress_reporter.h>
#include <project.h>
#include <reference_image.h>
#include <reporter.h>
#include <schematic.h>
#include <sch_bitmap.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_no_connect.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_sheet_pin.h>
#include <sch_symbol.h>
#include <sch_text.h>
#include <string_utils.h>
#include <stroke_params.h>
#include <template_fieldnames.h>
#include <title_block.h>

#include <sch_io/orcad/orcad_stream.h>


namespace
{

/// OrCAD DBU (10 mil) -> millimetres.
constexpr double DBU_TO_MM = 0.254;

/// Comment slots a KiCad title block exposes, ${COMMENT1}..${COMMENT9}
constexpr int COMMENT_COUNT = 9;


std::string trimmed( const std::string& aText )
{
    size_t begin = aText.find_first_not_of( " \t\r\n\f\v" );

    if( begin == std::string::npos )
        return std::string();

    size_t end = aText.find_last_not_of( " \t\r\n\f\v" );

    return aText.substr( begin, end - begin + 1 );
}


std::optional<std::chrono::sys_days> orcadCalendarDay( uint32_t aTimestamp )
{
    if( aTimestamp == 0 )
        return std::nullopt;

    using namespace std::chrono;

    // Use the reference installation's fixed UTC-8 offset for title-block dates.
    sys_days       date = floor<days>( sys_seconds( seconds( aTimestamp ) ) - hours( 8 ) );
    year_month_day ymd( date );

    if( !ymd.ok() )
        return std::nullopt;

    return date;
}


std::string orcadCalendarDate( uint32_t aTimestamp )
{
    using namespace std::chrono;

    std::optional<sys_days> date = orcadCalendarDay( aTimestamp );

    if( !date )
        return {};

    year_month_day ymd( *date );
    weekday        day( *date );

    static constexpr std::array<const char*, 7> weekdays = { "Sunday",   "Monday", "Tuesday", "Wednesday",
                                                              "Thursday", "Friday", "Saturday" };
    static constexpr std::array<const char*, 12> months = { "January",   "February", "March",    "April",
                                                             "May",       "June",     "July",     "August",
                                                             "September", "October",  "November", "December" };

    return std::string( weekdays[day.c_encoding()] ) + ", " + months[static_cast<unsigned>( ymd.month() ) - 1]
           + wxString::Format( wxS( " %02u, %d" ), static_cast<unsigned>( ymd.day() ),
                               static_cast<int>( ymd.year() ) )
                     .ToStdString();
}


std::string orcadShortCalendarDate( uint32_t aTimestamp )
{
    using namespace std::chrono;

    std::optional<sys_days> date = orcadCalendarDay( aTimestamp );

    if( !date )
        return {};

    year_month_day ymd( *date );
    static constexpr std::array<const char*, 12> months = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    return months[static_cast<unsigned>( ymd.month() ) - 1]
           + wxString::Format( wxS( " %02u, %d" ), static_cast<unsigned>( ymd.day() ),
                               static_cast<int>( ymd.year() ) )
                     .ToStdString();
}


std::string kicadBusName( std::string aName )
{
    size_t open = 0;

    while( ( open = aName.find( '[', open ) ) != std::string::npos )
    {
        size_t close = aName.find( ']', open + 1 );

        if( close == std::string::npos )
            break;

        size_t colon = aName.find( ':', open + 1 );

        if( colon < close )
        {
            aName.replace( colon, 1, ".." );
            ++close;
        }

        open = close + 1;
    }

    return aName;
}


std::string kicadElectricalNetName( std::string aName )
{
    aName = kicadBusName( std::move( aName ) );

    if( !aName.empty() && aName.front() == '/' )
    {
        size_t lastSlash = aName.find_last_of( '/' );

        if( lastSlash == 0 )
            aName.replace( 0, 1, "{slash}" );
        else
            aName.erase( 0, lastSlash + 1 );
    }

    return aName;
}


std::string kicadOccurrenceNetName( std::string aName )
{
    if( !aName.empty() && aName.front() == '/' )
        aName.erase( 0, aName.find_last_of( '/' ) + 1 );

    return kicadElectricalNetName( std::move( aName ) );
}


std::optional<uint32_t> occurrenceNetObjectId( const std::string& aName )
{
    size_t begin = std::string::npos;
    size_t separator = aName.find_last_of( '_' );

    if( separator != std::string::npos && separator + 1 < aName.size()
        && std::isdigit( static_cast<unsigned char>( aName[separator + 1] ) ) )
        begin = separator + 1;
    else if( aName.size() > 1 && aName[0] == 'N' )
        begin = 1;

    if( begin == std::string::npos )
        return std::nullopt;

    size_t end = begin;

    while( end < aName.size() && std::isdigit( static_cast<unsigned char>( aName[end] ) ) )
        ++end;

    if( end == begin || ( end != aName.size() && aName[end] != '_' ) )
        return std::nullopt;

    uint64_t objectId = 0;
    auto [next, error] = std::from_chars( aName.data() + begin, aName.data() + end, objectId );

    if( error != std::errc() || next != aName.data() + end || objectId == 0
        || objectId > std::numeric_limits<uint32_t>::max() )
    {
        return std::nullopt;
    }

    return static_cast<uint32_t>( objectId );
}


void pollProgress( PROGRESS_REPORTER* aReporter, const std::string& aPageName )
{
    if( !aReporter )
        return;

    aReporter->Report( wxString::Format( _( "Converting page '%s'..." ), FromOrcadString( aPageName ) ) );

    if( !aReporter->KeepRefreshing() )
        THROW_IO_ERROR( _( "Open canceled by user." ) );
}


SCH_SHAPE* makeSheetPoly( const std::vector<VECTOR2I>& aPoints, const ORCAD_PRIMITIVE& aPrimitive,
                          const KIGFX::COLOR4D& aColor, bool aCanFill = false,
                          bool aUseSymbolLineWidths = false )
{
    SCH_SHAPE* poly = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES );

    for( const VECTOR2I& pt : aPoints )
        poly->AddPoint( pt );

    int lineWidth = aUseSymbolLineWidths ? OrcadLineWidthIu( aPrimitive.lineWidth )
                                         : OrcadPageGraphicLineWidthIu( aPrimitive.lineWidth );
    poly->SetStroke( STROKE_PARAMS( lineWidth, OrcadLineStyle( aPrimitive.lineStyle ), aColor ) );

    // Page graphics have no symbol body color, so solid fill uses the foreground color.
    if( aCanFill && aPrimitive.fillStyle == 0 )
        poly->SetFillMode( FILL_T::FILLED_SHAPE );
    else
        poly->SetFillMode( aCanFill ? OrcadFillType( aPrimitive.fillStyle, aPrimitive.hatchStyle ) : FILL_T::NO_FILL );

    return poly;
}


VECTOR2I dbuPointToIu( double aX, double aY )
{
    return VECTOR2I( KiROUND( aX * ORCAD_IU_PER_DBU ), KiROUND( aY * ORCAD_IU_PER_DBU ) );
}

} // namespace


VECTOR2I OrcadStretchedImageSize( int aWidth, int aHeight, int aBoxWidth, int aBoxHeight )
{
    if( aWidth <= 0 || aHeight <= 0 || aBoxWidth <= 0 || aBoxHeight <= 0 )
        return VECTOR2I( aWidth, aHeight );

    int64_t imageAspect = static_cast<int64_t>( aWidth ) * aBoxHeight;
    int64_t boxAspect = static_cast<int64_t>( aBoxWidth ) * aHeight;

    if( imageAspect == boxAspect )
        return VECTOR2I( aWidth, aHeight );

    auto roundedRatio = []( int64_t aNumerator, int64_t aDenominator )
    {
        int64_t value = ( aNumerator + aDenominator / 2 ) / aDenominator;
        return static_cast<int>( std::clamp<int64_t>( value, 1, std::numeric_limits<int>::max() ) );
    };

    if( imageAspect < boxAspect )
        return VECTOR2I( roundedRatio( static_cast<int64_t>( aHeight ) * aBoxWidth, aBoxHeight ), aHeight );

    return VECTOR2I( aWidth, roundedRatio( static_cast<int64_t>( aWidth ) * aBoxHeight, aBoxWidth ) );
}


KIGFX::COLOR4D OrcadColor( int aColorIndex )
{
    static constexpr uint8_t palette[][3] = {
        { 0, 0, 0 },       { 255, 255, 128 }, { 128, 255, 128 }, { 0, 255, 128 },   { 128, 255, 255 }, { 0, 128, 255 },
        { 255, 128, 192 }, { 255, 128, 255 }, { 255, 0, 0 },     { 255, 255, 0 },   { 128, 255, 0 },   { 0, 255, 64 },
        { 0, 255, 255 },   { 0, 128, 192 },   { 128, 128, 192 }, { 255, 0, 255 },   { 128, 64, 64 },   { 255, 128, 64 },
        { 0, 255, 0 },     { 0, 128, 128 },   { 0, 64, 128 },    { 128, 128, 255 }, { 128, 0, 64 },    { 255, 0, 128 },
        { 128, 0, 0 },     { 255, 128, 0 },   { 0, 128, 0 },     { 0, 128, 64 },    { 0, 0, 255 },     { 0, 0, 160 },
        { 128, 0, 128 },   { 128, 0, 255 },   { 64, 0, 0 },      { 128, 64, 0 },    { 0, 64, 0 },      { 0, 64, 64 },
        { 0, 0, 128 },     { 0, 0, 64 },      { 64, 0, 64 },     { 64, 0, 128 },    { 0, 0, 0 },       { 128, 128, 0 },
        { 128, 128, 64 },  { 128, 128, 128 }, { 64, 128, 128 },  { 192, 192, 192 }, { 64, 0, 64 },     { 255, 255, 255 }
    };

    if( aColorIndex < 1 || aColorIndex >= static_cast<int>( std::size( palette ) ) )
        return KIGFX::COLOR4D::UNSPECIFIED;

    return KIGFX::COLOR4D( palette[aColorIndex][0] / 255.0, palette[aColorIndex][1] / 255.0,
                           palette[aColorIndex][2] / 255.0, 1.0 );
}


ORCAD_CONVERTER::ORCAD_CONVERTER( ORCAD_DESIGN& aDesign, SCHEMATIC* aSchematic, REPORTER* aReporter,
                                  PROGRESS_REPORTER* aProgressReporter ) :
        m_design( aDesign ),
        m_schematic( aSchematic ),
        m_reporter( aReporter ),
        m_progressReporter( aProgressReporter ),
        m_rootSheet( nullptr ),
        m_powerCount( 0 ),
        m_fontBaselineDbu( 0 )
{
}


ORCAD_CONVERTER::~ORCAD_CONVERTER() = default;


KIID ORCAD_CONVERTER::deterministicUuid( const std::string& aRole, size_t aOrdinal ) const
{
    return KIID::FromName( "orcad-import:" + m_design.sourceId + ":" + aRole + ":" + std::to_string( aOrdinal ) );
}


void ORCAD_CONVERTER::appendPageItem( SCH_SCREEN* aScreen, SCH_ITEM* aItem )
{
    if( aScreen == m_pageItemScreen )
        m_pageItems.push_back( aItem );

    aScreen->Append( aItem );
}


void ORCAD_CONVERTER::assignPageItemUuids( size_t aPageOrdinal )
{
    std::map<KICAD_T, size_t> ordinals;

    for( SCH_ITEM* item : m_pageItems )
    {
        if( item->Type() == SCH_SHEET_T )
            continue;

        size_t      ordinal = ordinals[item->Type()]++;
        std::string role = "page:" + std::to_string( aPageOrdinal )
                           + ":item:" + std::to_string( static_cast<int>( item->Type() ) );
        const_cast<KIID&>( item->m_Uuid ) = deterministicUuid( role, ordinal );

        if( item->Type() == SCH_SYMBOL_T )
        {
            SCH_SYMBOL*                   symbol = static_cast<SCH_SYMBOL*>( item );
            std::map<std::string, size_t> pinOrdinals;

            for( const std::unique_ptr<SCH_PIN>& ownedPin : symbol->GetRawPins() )
            {
                SCH_PIN*    pin = ownedPin.get();
                VECTOR2I    position = pin->GetPosition();
                std::string pinRole = std::string( pin->GetNumber().ToUTF8() ) + ":"
                                      + std::string( pin->GetName().ToUTF8() ) + ":" + std::to_string( position.x )
                                      + ":" + std::to_string( position.y );
                size_t pinOrdinal = pinOrdinals[pinRole]++;
                const_cast<KIID&>( pin->m_Uuid ) =
                        deterministicUuid( role + ":" + std::to_string( ordinal ) + ":pin:" + pinRole, pinOrdinal );
            }

            std::sort( symbol->GetRawPins().begin(), symbol->GetRawPins().end(),
                       []( const std::unique_ptr<SCH_PIN>& a, const std::unique_ptr<SCH_PIN>& b )
                       {
                           return a->m_Uuid < b->m_Uuid;
                       } );
        }
    }
}


void ORCAD_CONVERTER::warn( const wxString& aMsg )
{
    if( m_reporter )
        m_reporter->Report( aMsg, RPT_SEVERITY_WARNING );
}


void ORCAD_CONVERTER::note( const wxString& aMsg )
{
    if( m_reporter )
        m_reporter->Report( aMsg, RPT_SEVERITY_INFO );
}


void ORCAD_CONVERTER::prepareGlobalNetNames()
{
    auto lowerName = []( std::string aName )
    {
        std::transform( aName.begin(), aName.end(), aName.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return aName;
    };

    auto addName = [&]( const std::string& aName )
    {
        std::string name = trimmed( aName );
        std::string key = lowerName( name );

        if( !key.empty() )
            m_globalNetNames.emplace( std::move( key ), std::move( name ) );
    };

    auto addPage = [&]( const ORCAD_RAW_PAGE& aPage )
    {
        for( const ORCAD_GRAPHIC_INST& global : aPage.globals )
        {
            auto        name = global.props.find( "Name" );
            std::string displayName = name != global.props.end() ? trimmed( name->second ) : std::string();
            std::string logicalName = trimmed( global.logicalName );
            std::string powerName = !displayName.empty() ? displayName : logicalName;
            addName( powerName );

            powerName = lowerName( powerName );

            if( !powerName.empty() )
                m_powerNetNames.insert( std::move( powerName ) );
        }

        for( const ORCAD_GRAPHIC_INST& offpage : aPage.offpage )
        {
            addName( offpage.logicalName );

            std::string key = trimmed( offpage.logicalName );
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            if( !key.empty() )
                m_offpageNetNames.insert( std::move( key ) );
        }

        for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
            addName( port.logicalName.empty() ? port.name : port.logicalName );
    };

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
        addPage( page );

    for( const auto& [folder, pages] : m_design.childFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            addPage( page );
    }

}


std::string ORCAD_CONVERTER::canonicalGlobalNetName( const std::string& aName ) const
{
    std::string name = trimmed( aName );
    std::string key = name;
    std::transform( key.begin(), key.end(), key.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );

    auto alias = m_globalNetAliases.find( key );

    if( alias != m_globalNetAliases.end() )
        return kicadElectricalNetName( alias->second );

    auto canonical = m_globalNetNames.find( key );
    return kicadElectricalNetName( canonical != m_globalNetNames.end() ? canonical->second : name );
}


std::string ORCAD_CONVERTER::effectiveInterfaceNetName( const std::string& aName ) const
{
    std::string name = canonicalGlobalNetName( aName );
    std::string key = name;
    std::transform( key.begin(), key.end(), key.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );

    auto effective = m_currentInterfaceNetAliases.find( key );
    return effective != m_currentInterfaceNetAliases.end() ? effective->second : name;
}


std::string ORCAD_CONVERTER::occurrenceElectricalNetName( uint32_t aOccurrenceId,
                                                          const std::string& aName ) const
{
    auto lower = []( std::string aValue )
    {
        std::transform( aValue.begin(), aValue.end(), aValue.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return aValue;
    };

    auto baseName = [&]( const std::string& aValue )
    {
        std::string name = kicadOccurrenceNetName( aValue );
        std::string key = lower( name );
        auto occurrenceAlias = m_currentOccurrenceNetAliases.find( key );

        if( occurrenceAlias != m_currentOccurrenceNetAliases.end() )
            return occurrenceAlias->second;

        auto interfaceAlias = m_currentInterfaceNetAliases.find( key );
        return interfaceAlias != m_currentInterfaceNetAliases.end() ? interfaceAlias->second : name;
    };

    std::string electricalName = baseName( aName );
    std::string electricalKey = lower( electricalName );
    size_t      peerCount = 0;

    if( m_currentOccNetNames )
    {
        peerCount = std::count_if( m_currentOccNetNames->begin(), m_currentOccNetNames->end(),
                                   [&]( const auto& aOccurrence )
                                   {
                                       return lower( baseName( aOccurrence.second ) ) == electricalKey;
                                   } );
    }

    if( peerCount > 1 )
    {
        std::string suffix = std::to_string( aOccurrenceId );

        if( suffix.size() < 6 )
            suffix.insert( 0, 6 - suffix.size(), '0' );

        electricalName += "_" + suffix;
    }

    return electricalName;
}


bool ORCAD_CONVERTER::isOffpageNetName( const std::string& aName ) const
{
    std::string key = trimmed( aName );
    std::transform( key.begin(), key.end(), key.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );
    return m_offpageNetNames.count( key );
}


bool ORCAD_CONVERTER::isPowerNetName( const std::string& aName ) const
{
    std::string key = trimmed( aName );
    std::transform( key.begin(), key.end(), key.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );
    return m_powerNetNames.count( key );
}


/// Return a numeric page prefix (or -1); strip only the "N - title" convention.
int OrcadPageOrder( wxString& aName )
{
    size_t   digitStart = 0;
    wxString upper = aName.Upper();

    for( const wxString& prefix : { wxString( "PAGE" ), wxString( "SCH" ), wxString( "PAG" ) } )
    {
        if( upper.StartsWith( prefix )
            && ( aName.length() == prefix.length() || wxIsdigit( aName[prefix.length()] )
                 || wxIsspace( aName[prefix.length()] ) || aName[prefix.length()] == '_' ) )
        {
            digitStart = prefix.length();

            while( digitStart < aName.length() && ( wxIsspace( aName[digitStart] ) || aName[digitStart] == '_' ) )
            {
                digitStart++;
            }

            break;
        }
    }

    size_t digitEnd = digitStart;

    while( digitEnd < aName.length() && wxIsdigit( aName[digitEnd] ) )
        digitEnd++;

    long order = 0;

    if( digitEnd == digitStart || !aName.Mid( digitStart, digitEnd - digitStart ).ToLong( &order ) )
    {
        return -1;
    }

    size_t separator = digitEnd;

    while( separator < aName.length() && wxIsspace( aName[separator] ) )
        separator++;

    bool separatedBySpace = separator > digitEnd;

    if( !separatedBySpace && separator < aName.length() && aName[separator] != '-' && aName[separator] != '.'
        && aName[separator] != ':' && aName[separator] != '_' )
        return -1;

    if( separator < aName.length() && aName[separator] == '-' )
    {
        wxString rest = aName.Mid( separator + 1 );
        rest.Trim( false );
        aName = rest;
    }

    return static_cast<int>( order );
}


static std::string scopedHierBusName( const std::string& aName, uint32_t aOccurrenceId )
{
    size_t range = aName.find( '[' );

    if( range == std::string::npos || aName.find( "..", range ) == std::string::npos )
        return aName;

    return aName.substr( 0, range ) + "_ORCAD_" + std::to_string( aOccurrenceId ) + aName.substr( range );
}


static bool parseVectorBusName( const std::string& aName, std::string& aPrefix, int& aFirst, int& aLast )
{
    size_t open = aName.find( '[' );
    size_t dots = open == std::string::npos ? std::string::npos : aName.find( "..", open + 1 );
    size_t separatorSize = 2;

    if( dots == std::string::npos && open != std::string::npos )
    {
        dots = aName.find( ':', open + 1 );
        separatorSize = 1;
    }

    size_t close = dots == std::string::npos ? std::string::npos : aName.find( ']', dots + separatorSize );

    if( open == std::string::npos || dots == std::string::npos || close != aName.size() - 1 )
        return false;

    auto parseIndex = []( std::string_view aText, int& aValue )
    {
        const char* begin = aText.data();
        const char* end = begin + aText.size();
        auto [next, error] = std::from_chars( begin, end, aValue );
        return error == std::errc() && next == end;
    };

    if( !parseIndex( std::string_view( aName ).substr( open + 1, dots - open - 1 ), aFirst )
        || !parseIndex( std::string_view( aName ).substr( dots + separatorSize, close - dots - separatorSize ),
                        aLast ) )
    {
        return false;
    }

    aPrefix = aName.substr( 0, open );
    return true;
}


static std::string mappedHierBusMember( const std::string& aName, const std::map<std::string, std::string>& aBusNames,
                                        bool aReverse )
{
    for( const auto& [source, scoped] : aBusNames )
    {
        const std::string& sourceBus = aReverse ? scoped : source;
        const std::string& scopedBus = aReverse ? source : scoped;
        std::string sourcePrefix;
        std::string scopedPrefix;
        int         sourceFirst;
        int         sourceLast;
        int         scopedFirst;
        int         scopedLast;

        if( !parseVectorBusName( sourceBus, sourcePrefix, sourceFirst, sourceLast )
            || !parseVectorBusName( scopedBus, scopedPrefix, scopedFirst, scopedLast )
            || aName.compare( 0, sourcePrefix.size(), sourcePrefix ) != 0 )
        {
            continue;
        }

        int              member;
        std::string_view suffix( aName.data() + sourcePrefix.size(), aName.size() - sourcePrefix.size() );
        auto [next, error] = std::from_chars( suffix.data(), suffix.data() + suffix.size(), member );

        if( suffix.empty() || error != std::errc() || next != suffix.data() + suffix.size() )
            continue;

        int sourceStep = sourceLast >= sourceFirst ? 1 : -1;
        int scopedStep = scopedLast >= scopedFirst ? 1 : -1;
        int64_t ordinal = ( static_cast<int64_t>( member ) - sourceFirst ) * sourceStep;
        int64_t sourceCount = std::abs( static_cast<int64_t>( sourceLast ) - sourceFirst ) + 1;
        int64_t scopedCount = std::abs( static_cast<int64_t>( scopedLast ) - scopedFirst ) + 1;

        if( ordinal < 0 || ordinal >= sourceCount || sourceCount != scopedCount )
            continue;

        return scopedPrefix + std::to_string( scopedFirst + ordinal * scopedStep );
    }

    return aName;
}


static std::string scopedHierBusMember( const std::string& aName,
                                        const std::map<std::string, std::string>& aBusNames )
{
    return mappedHierBusMember( aName, aBusNames, false );
}


static std::string unscopedHierBusMember( const std::string& aName,
                                          const std::map<std::string, std::string>& aBusNames )
{
    return mappedHierBusMember( aName, aBusNames, true );
}


static std::string scopedHierBusRange( const std::string& aName, const std::map<std::string, std::string>& aBusNames )
{
    std::string memberPrefix;
    int         memberFirst;
    int         memberLast;

    if( !parseVectorBusName( aName, memberPrefix, memberFirst, memberLast ) )
        return aName;

    for( const auto& [sourceBus, scopedBus] : aBusNames )
    {
        std::string sourcePrefix;
        std::string scopedPrefix;
        int         sourceFirst;
        int         sourceLast;
        int         scopedFirst;
        int         scopedLast;

        if( !parseVectorBusName( sourceBus, sourcePrefix, sourceFirst, sourceLast )
            || !parseVectorBusName( scopedBus, scopedPrefix, scopedFirst, scopedLast ) || memberPrefix != sourcePrefix )
        {
            continue;
        }

        int sourceStep = sourceLast >= sourceFirst ? 1 : -1;
        int scopedStep = scopedLast >= scopedFirst ? 1 : -1;
        int64_t sourceCount = std::abs( static_cast<int64_t>( sourceLast ) - sourceFirst ) + 1;
        int64_t scopedCount = std::abs( static_cast<int64_t>( scopedLast ) - scopedFirst ) + 1;
        int64_t firstOrdinal = ( static_cast<int64_t>( memberFirst ) - sourceFirst ) * sourceStep;
        int64_t lastOrdinal = ( static_cast<int64_t>( memberLast ) - sourceFirst ) * sourceStep;

        if( sourceCount != scopedCount || firstOrdinal < 0 || firstOrdinal >= sourceCount || lastOrdinal < 0
            || lastOrdinal >= sourceCount )
        {
            continue;
        }

        int64_t mappedFirst = scopedFirst + firstOrdinal * scopedStep;
        int64_t mappedLast = scopedFirst + lastOrdinal * scopedStep;
        return scopedPrefix + "[" + std::to_string( mappedFirst ) + ".." + std::to_string( mappedLast ) + "]";
    }

    return aName;
}


static bool rawPointOnSegment( int aX, int aY, const ORCAD_WIRE& aWire )
{
    int64_t cross = static_cast<int64_t>( aX - aWire.x1 ) * ( aWire.y2 - aWire.y1 )
                    - static_cast<int64_t>( aY - aWire.y1 ) * ( aWire.x2 - aWire.x1 );
    return cross == 0 && aX >= std::min( aWire.x1, aWire.x2 ) && aX <= std::max( aWire.x1, aWire.x2 )
           && aY >= std::min( aWire.y1, aWire.y2 ) && aY <= std::max( aWire.y1, aWire.y2 );
}


static std::optional<VECTOR2I> rawWireIntersection( const ORCAD_WIRE& aFirst, const ORCAD_WIRE& aSecond )
{
    int64_t firstDx = static_cast<int64_t>( aFirst.x2 ) - aFirst.x1;
    int64_t firstDy = static_cast<int64_t>( aFirst.y2 ) - aFirst.y1;
    int64_t secondDx = static_cast<int64_t>( aSecond.x2 ) - aSecond.x1;
    int64_t secondDy = static_cast<int64_t>( aSecond.y2 ) - aSecond.y1;
    int64_t deltaX = static_cast<int64_t>( aSecond.x1 ) - aFirst.x1;
    int64_t deltaY = static_cast<int64_t>( aSecond.y1 ) - aFirst.y1;
    int64_t denominator = firstDx * secondDy - firstDy * secondDx;

    if( denominator == 0 )
        return std::nullopt;

    int64_t firstNumerator = deltaX * secondDy - deltaY * secondDx;
    int64_t secondNumerator = deltaX * firstDy - deltaY * firstDx;

    if( denominator < 0 )
    {
        denominator = -denominator;
        firstNumerator = -firstNumerator;
        secondNumerator = -secondNumerator;
    }

    if( firstNumerator < 0 || firstNumerator > denominator || secondNumerator < 0
        || secondNumerator > denominator )
    {
        return std::nullopt;
    }

    int64_t xNumerator = static_cast<int64_t>( aFirst.x1 ) * denominator + firstDx * firstNumerator;
    int64_t yNumerator = static_cast<int64_t>( aFirst.y1 ) * denominator + firstDy * firstNumerator;

    if( xNumerator % denominator != 0 || yNumerator % denominator != 0 )
        return std::nullopt;

    return VECTOR2I( static_cast<int>( xNumerator / denominator ), static_cast<int>( yNumerator / denominator ) );
}


static bool rawBusSegmentsTouch( const ORCAD_WIRE& aFirst, const ORCAD_WIRE& aSecond )
{
    return rawPointOnSegment( aFirst.x1, aFirst.y1, aSecond ) || rawPointOnSegment( aFirst.x2, aFirst.y2, aSecond )
           || rawPointOnSegment( aSecond.x1, aSecond.y1, aFirst )
           || rawPointOnSegment( aSecond.x2, aSecond.y2, aFirst );
}


static std::set<std::string> busPrefixTokens( const std::string& aPrefix )
{
    std::set<std::string> result;
    size_t                start = 0;

    while( start < aPrefix.size() )
    {
        while( start < aPrefix.size() && !std::isalnum( static_cast<unsigned char>( aPrefix[start] ) ) )
            ++start;

        size_t end = start;

        while( end < aPrefix.size() && std::isalnum( static_cast<unsigned char>( aPrefix[end] ) ) )
            ++end;

        if( end - start > 1 )
            result.insert( aPrefix.substr( start, end - start ) );

        start = end;
    }

    return result;
}


static std::string firstBusPrefixToken( const std::string& aPrefix )
{
    size_t start = 0;

    while( start < aPrefix.size() && !std::isalnum( static_cast<unsigned char>( aPrefix[start] ) ) )
        ++start;

    size_t end = start;

    while( end < aPrefix.size() && std::isalnum( static_cast<unsigned char>( aPrefix[end] ) ) )
        ++end;

    return end - start > 1 ? aPrefix.substr( start, end - start ) : std::string();
}


static std::string connectedBusName( const ORCAD_RAW_PAGE& aPage, const ORCAD_BLOCK_PIN& aPin,
                                     const std::string& aFallback )
{
    std::string sourcePrefix;
    int         sourceFirst;
    int         sourceLast;

    if( !parseVectorBusName( aFallback, sourcePrefix, sourceFirst, sourceLast ) )
        return aFallback;

    int                 sourceCount = std::abs( sourceLast - sourceFirst ) + 1;
    std::vector<size_t> pending;
    std::set<size_t>    seen;

    for( size_t i = 0; i < aPage.wires.size(); ++i )
    {
        if( aPage.wires[i].isBus && rawPointOnSegment( aPin.x, aPin.y, aPage.wires[i] ) )
        {
            pending.push_back( i );
            seen.insert( i );
        }
    }

    for( size_t cursor = 0; cursor < pending.size(); ++cursor )
    {
        const ORCAD_WIRE& wire = aPage.wires[pending[cursor]];

        for( size_t i = 0; i < aPage.wires.size(); ++i )
        {
            if( !seen.count( i ) && aPage.wires[i].isBus && rawBusSegmentsTouch( wire, aPage.wires[i] ) )
            {
                seen.insert( i );
                pending.push_back( i );
            }
        }
    }

    std::set<std::string> names;

    for( size_t wireIndex : pending )
    {
        const ORCAD_WIRE& wire = aPage.wires[wireIndex];

        for( const ORCAD_ALIAS& alias : wire.aliases )
            names.insert( alias.name );

        auto aliases = aPage.netAliases.find( wire.id );

        if( aliases != aPage.netAliases.end() )
            names.insert( aliases->second.begin(), aliases->second.end() );

        auto net = aPage.netmap.find( wire.id );

        if( net != aPage.netmap.end() )
            names.insert( net->second );
    }

    for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
    {
        if( std::any_of( pending.begin(), pending.end(),
                         [&]( size_t aWireIndex )
                         {
                             return rawPointOnSegment( port.x, port.y, aPage.wires[aWireIndex] );
                         } ) )
        {
            names.insert( port.name );
            names.insert( port.logicalName );
        }
    }

    for( const ORCAD_GRAPHIC_INST& connector : aPage.offpage )
    {
        if( std::any_of( pending.begin(), pending.end(),
                         [&]( size_t aWireIndex )
                         {
                             return rawPointOnSegment( connector.x, connector.y, aPage.wires[aWireIndex] );
                         } ) )
        {
            names.insert( connector.name );
            names.insert( connector.logicalName );
        }
    }

    if( pending.empty() )
    {
        for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
        {
            names.insert( port.name );
            names.insert( port.logicalName );
        }

        for( const ORCAD_GRAPHIC_INST& connector : aPage.offpage )
        {
            names.insert( connector.name );
            names.insert( connector.logicalName );
        }
    }

    if( names.count( aFallback ) )
        return aFallback;

    std::set<uint32_t> connectedNetIds;

    for( size_t wireIndex : pending )
        connectedNetIds.insert( aPage.wires[wireIndex].id );

    std::set<std::string> sourceMembers;
    int                   sourceStep = sourceLast >= sourceFirst ? 1 : -1;

    for( int member = sourceFirst;; member += sourceStep )
    {
        sourceMembers.insert( sourcePrefix + std::to_string( member ) );

        if( member == sourceLast )
            break;
    }

    for( const ORCAD_NET_GROUP& group : aPage.netGroups )
    {
        if( !connectedNetIds.count( group.id ) || static_cast<int>( group.members.size() ) != sourceCount )
            continue;

        std::set<std::string> memberNames;

        for( uint32_t memberId : group.members )
        {
            auto aliases = aPage.netAliases.find( memberId );

            if( aliases != aPage.netAliases.end() )
                memberNames.insert( aliases->second.begin(), aliases->second.end() );

            auto net = aPage.netmap.find( memberId );

            if( net != aPage.netmap.end() )
                memberNames.insert( net->second );
        }

        if( std::includes( memberNames.begin(), memberNames.end(), sourceMembers.begin(), sourceMembers.end() ) )
            return aFallback;
    }

    std::set<std::string> candidates;

    for( const std::string& name : names )
    {
        std::string targetPrefix;
        int         targetFirst;
        int         targetLast;

        if( parseVectorBusName( name, targetPrefix, targetFirst, targetLast )
            && std::abs( targetLast - targetFirst ) + 1 == sourceCount )
        {
            candidates.insert( kicadBusName( name ) );
        }
    }

    std::set<std::string> sourceTokens = busPrefixTokens( sourcePrefix );

    for( const std::string& candidate : candidates )
    {
        std::string candidatePrefix;
        int         candidateFirst;
        int         candidateLast;

        if( !parseVectorBusName( candidate, candidatePrefix, candidateFirst, candidateLast ) )
            continue;

        std::set<std::string>    candidateTokens = busPrefixTokens( candidatePrefix );
        std::vector<std::string> sharedTokens;
        std::set_intersection( sourceTokens.begin(), sourceTokens.end(), candidateTokens.begin(), candidateTokens.end(),
                               std::back_inserter( sharedTokens ) );

        // KiCad maps ranges positionally within a bus family.  Renaming such a pin
        // translates nested ranges twice; only unrelated, unambiguous families need it.
        bool leadingNamespaceOnly = sharedTokens.size() == 1 && sourceTokens.size() > 1 && candidateTokens.size() > 1
                                    && sharedTokens.front() == firstBusPrefixToken( sourcePrefix )
                                    && sharedTokens.front() == firstBusPrefixToken( candidatePrefix );

        if( !sharedTokens.empty() && !leadingNamespaceOnly )
            return aFallback;
    }

    if( candidates.size() == 1 )
        return *candidates.begin();

    return aFallback;
}


static COLOR4D invisibleElectricalTextColor()
{
    // All-zero COLOR4D is KiCad's unspecified-color sentinel and is not serialized.
    return COLOR4D( 1.0 / 255.0, 0, 0, 0 );
}


static uint32_t busNetAt( const ORCAD_RAW_PAGE& aPage, const ORCAD_BLOCK_PIN& aPin )
{
    uint32_t endpointNet = 0;
    uint32_t throughNet = 0;

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        int64_t cross = static_cast<int64_t>( aPin.x - wire.x1 ) * ( wire.y2 - wire.y1 )
                        - static_cast<int64_t>( aPin.y - wire.y1 ) * ( wire.x2 - wire.x1 );
        bool onSegment = cross == 0 && aPin.x >= std::min( wire.x1, wire.x2 ) && aPin.x <= std::max( wire.x1, wire.x2 )
                         && aPin.y >= std::min( wire.y1, wire.y2 ) && aPin.y <= std::max( wire.y1, wire.y2 );

        if( !wire.isBus || !onSegment )
            continue;

        bool      endpoint = ( aPin.x == wire.x1 && aPin.y == wire.y1 ) || ( aPin.x == wire.x2 && aPin.y == wire.y2 );
        uint32_t& candidate = endpoint ? endpointNet : throughNet;

        if( candidate == 0 || wire.id < candidate )
            candidate = wire.id;
    }

    return endpointNet ? endpointNet : throughNet;
}


static std::string flatNetSuffix( const ORCAD_DRAWN_INSTANCE& aBlock )
{
    if( !aBlock.name.empty() )
        return aBlock.name;

    auto name = aBlock.props.find( "Name" );

    return name != aBlock.props.end() ? name->second : std::string();
}


SCH_SHEET* ORCAD_CONVERTER::Convert( SCH_SHEET* aRootSheet )
{
    m_rootSheet = aRootSheet;

    prepareGlobalNetNames();
    prepareSymbols();

    struct SLASH_NAMES
    {
        bool                  leading = false;
        std::set<std::string> embedded;
    };

    std::map<std::string, SLASH_NAMES> slashNames;
    auto collectSlashName = [&]( const std::string& aName )
    {
        if( aName.empty() || aName.find( '/' ) == std::string::npos )
            return;

        std::string key = aName;
        std::erase( key, '/' );
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( aName.front() == '/' )
            slashNames[key].leading = true;
        else
            slashNames[key].embedded.insert( kicadOccurrenceNetName( aName ) );
    };
    auto collectPageSlashNames = [&]( const ORCAD_RAW_PAGE& aPage )
    {
        for( const auto& [netId, name] : aPage.netmap )
            collectSlashName( name );

        for( const auto& [netId, aliases] : aPage.netAliases )
        {
            for( const std::string& alias : aliases )
                collectSlashName( alias );
        }
    };

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
        collectPageSlashNames( page );

    for( const auto& [folder, pages] : m_design.childFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            collectPageSlashNames( page );
    }

    std::map<std::string, std::string> baseOccurrenceNetAliases;

    for( const auto& [key, names] : slashNames )
    {
        if( names.leading && names.embedded.size() == 1 )
            baseOccurrenceNetAliases[key] = *names.embedded.begin();
    }

    std::map<const std::map<uint32_t, std::string>*, std::map<std::string, std::string>> occurrenceAliasesByScope;

    auto selectOccurrenceAliases = [&]( const std::map<uint32_t, std::string>* aScope )
    {
        m_currentOccurrenceNetAliases = baseOccurrenceNetAliases;
        auto aliases = occurrenceAliasesByScope.find( aScope );

        if( aliases != occurrenceAliasesByScope.end() )
            m_currentOccurrenceNetAliases.insert( aliases->second.begin(), aliases->second.end() );
    };

    m_occurrenceNetNameScopeCounts.clear();
    m_occurrenceNetNameMinDepth.clear();
    std::function<void( const ORCAD_OCC_SCOPE&, size_t )> countOccurrenceNetNameScopes =
            [&]( const ORCAD_OCC_SCOPE& aScope, size_t aDepth )
    {
        std::set<std::string> scopeNames;

        for( const auto& occurrence : aScope.netNames )
        {
            const std::string& name = occurrence.second;
            std::string        key = kicadOccurrenceNetName( name );
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            scopeNames.insert( key );

            auto depth = m_occurrenceNetNameMinDepth.find( key );

            if( depth == m_occurrenceNetNameMinDepth.end() || aDepth < depth->second )
                m_occurrenceNetNameMinDepth[key] = aDepth;
        }

        for( const std::string& name : scopeNames )
            ++m_occurrenceNetNameScopeCounts[name];

        for( const ORCAD_OCC_BLOCK& block : aScope.blocks )
            countOccurrenceNetNameScopes( block.scope, aDepth + 1 );
    };
    countOccurrenceNetNameScopes( m_design.occurrenceRoot, 0 );

    SCH_SCREEN* rootScreen = aRootSheet->GetScreen();

    SCH_SHEET_PATH rootPath;
    rootPath.push_back( aRootSheet );
    rootPath.SetPageNumber( wxS( "1" ) );

    std::map<std::string, size_t>                 occurrenceFolderCounts;
    std::function<void( const ORCAD_OCC_SCOPE& )> countOccurrenceFolders = [&]( const ORCAD_OCC_SCOPE& aScope )
    {
        for( const ORCAD_OCC_BLOCK& block : aScope.blocks )
        {
            std::string key = block.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            ++occurrenceFolderCounts[key];
            countOccurrenceFolders( block.scope );
        }
    };
    countOccurrenceFolders( m_design.occurrenceRoot );

    std::set<std::string> rootChildFolders;
    bool                  simpleRepeatedLeafDesign = !m_design.occurrenceRoot.blocks.empty();

    for( const ORCAD_OCC_BLOCK& block : m_design.occurrenceRoot.blocks )
    {
        std::string key = block.childFolder;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        rootChildFolders.insert( key );
        auto pages = m_design.childFolderPages.find( key );

        if( pages == m_design.childFolderPages.end()
            || !std::all_of( pages->second.begin(), pages->second.end(),
                             []( const ORCAD_RAW_PAGE& aPage )
                             {
                                 return aPage.blocks.empty();
                             } ) )
        {
            simpleRepeatedLeafDesign = false;
        }
    }

    simpleRepeatedLeafDesign &= rootChildFolders.size() == 1 && m_design.occurrenceRoot.blocks.size() > 1;

    m_connectedBlockInterfaceNames.clear();
    auto collectConnectedBlockInterfaceNames = [&]( const ORCAD_RAW_PAGE& aPage )
    {
        for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
        {
            for( const ORCAD_BLOCK_PIN& pin : block.pins )
            {
                if( pin.noConnect || pin.name.empty() )
                    continue;

                std::string pinName = canonicalGlobalNetName( pin.name );
                std::transform( pinName.begin(), pinName.end(), pinName.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                for( const ORCAD_WIRE& wire : aPage.wires )
                {
                    if( wire.isBus || !rawPointOnSegment( pin.x, pin.y, wire ) )
                        continue;

                    auto netName = aPage.netmap.find( wire.id );

                    if( netName == aPage.netmap.end() )
                        continue;

                    std::string wireName = canonicalGlobalNetName( netName->second );
                    std::transform( wireName.begin(), wireName.end(), wireName.begin(),
                                    []( unsigned char c )
                                    {
                                        return static_cast<char>( std::tolower( c ) );
                                    } );

                    if( wireName == pinName )
                        m_connectedBlockInterfaceNames.insert( pinName );
                }
            }
        }
    };

    for( const ORCAD_RAW_PAGE& page : m_design.pages )
        collectConnectedBlockInterfaceNames( page );

    for( const auto& [folder, pages] : m_design.childFolderPages )
    {
        for( const ORCAD_RAW_PAGE& page : pages )
            collectConnectedBlockInterfaceNames( page );
    }

    auto unconnectedInterfaceNetNames = [&]( const ORCAD_DRAWN_INSTANCE& aDrawn, const std::string& aFlatNetSuffix )
    {
        std::map<std::string, std::string> result;

        for( const ORCAD_BLOCK_PIN& pin : aDrawn.pins )
        {
            if( !pin.noConnect || pin.name.empty() || aFlatNetSuffix.empty() )
                continue;

            std::string localName = canonicalGlobalNetName( pin.name );
            std::string localKey = localName;
            std::transform( localKey.begin(), localKey.end(), localKey.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            if( !m_connectedBlockInterfaceNames.count( localKey ) )
                continue;

            result.emplace( std::move( localKey ), localName + "_" + aFlatNetSuffix );
        }

        return result;
    };

    std::function<bool( const ORCAD_RAW_PAGE&, const ORCAD_OCC_SCOPE& )> canBuildHierarchy =
            [&]( const ORCAD_RAW_PAGE& aPage, const ORCAD_OCC_SCOPE& aScope )
    {
        if( aPage.blocks.size() != aScope.blocks.size() )
            return false;

        std::set<uint32_t> matchedBlocks;

        for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
        {
            auto drawn = std::find_if( aPage.blocks.begin(), aPage.blocks.end(),
                                       [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                                       {
                                           return aBlock.dbId == occurrence.targetDbId;
                                       } );

            std::string key = occurrence.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            auto pages = m_design.childFolderPages.find( key );

            if( !matchedBlocks.insert( occurrence.targetDbId ).second || drawn == aPage.blocks.end()
                || pages == m_design.childFolderPages.end() || pages->second.size() != 1
                || !canBuildHierarchy( pages->second.front(), occurrence.scope ) )
            {
                return false;
            }
        }

        return true;
    };

    bool nativeHierarchy = m_design.pages.size() == 1 && !m_design.occurrenceRoot.blocks.empty()
                           && canBuildHierarchy( m_design.pages.front(), m_design.occurrenceRoot );

    if( nativeHierarchy )
    {
        ORCAD_RAW_PAGE& rootPage = m_design.pages.front();

        pollProgress( m_progressReporter, rootPage.name );
        m_currentOccRefs = &m_design.occurrenceRoot.partRefs;
        m_currentOccUnitRefs = &m_design.occurrenceRoot.partUnitRefs;
        m_currentOccProps = &m_design.occurrenceRoot.partProps;
        m_currentOccNetNames = &m_design.occurrenceRoot.netNames;
        selectOccurrenceAliases( m_currentOccNetNames );
        m_currentFlatNetSuffix.clear();
        m_scopeNamedFlatNets = false;
        m_scopeGeneratedFlatNets = false;
        m_currentUnconnectedInterfaceNetNames.clear();
        applyPageSettings( rootPage, rootScreen );
        convertPage( rootPage, rootScreen, rootPath );

        int pageIndex = 1;

        std::function<void( ORCAD_RAW_PAGE&, const ORCAD_OCC_SCOPE&, SCH_SHEET*, const SCH_SHEET_PATH& )>
                placeChildren = [&]( ORCAD_RAW_PAGE& aParentPage, const ORCAD_OCC_SCOPE& aScope,
                                     SCH_SHEET* aParentSheet, const SCH_SHEET_PATH& aParentPath )
        {
            std::vector<const ORCAD_OCC_BLOCK*> occurrences;

            for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
                occurrences.push_back( &occurrence );

            std::stable_sort( occurrences.begin(), occurrences.end(),
                              []( const ORCAD_OCC_BLOCK* a, const ORCAD_OCC_BLOCK* b )
                              {
                                  wxString aName = FromOrcadString( a->childFolder );
                                  wxString bName = FromOrcadString( b->childFolder );
                                  int      aOrder = OrcadPageOrder( aName );
                                  int      bOrder = OrcadPageOrder( bName );
                                  int      aKey = aOrder >= 0 ? aOrder : std::numeric_limits<int>::max();
                                  int      bKey = bOrder >= 0 ? bOrder : std::numeric_limits<int>::max();

                                  if( aKey != bKey )
                                      return aKey < bKey;

                                  return aName.CmpNoCase( bName ) < 0;
                              } );

            for( const ORCAD_OCC_BLOCK* occurrencePtr : occurrences )
            {
                const ORCAD_OCC_BLOCK& occurrence = *occurrencePtr;
                auto                   drawn = std::find_if( aParentPage.blocks.begin(), aParentPage.blocks.end(),
                                                             [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                                                             {
                                               return aBlock.dbId == occurrence.targetDbId;
                                           } );

                std::string key = occurrence.childFolder;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                ORCAD_RAW_PAGE& childPage = m_design.childFolderPages.at( key ).front();
                SCH_SCREEN*     childScreen = new SCH_SCREEN( m_schematic );
                const_cast<KIID&>( childScreen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );
                SCH_SHEET* childSheet = new SCH_SHEET( aParentSheet, OrcadDbuToIu( drawn->x1, drawn->y1 ),
                                                       OrcadDbuToIu( drawn->w, drawn->h ) );
                wxString   sheetName = FromOrcadString( drawn->reference );

                if( sheetName.IsEmpty() )
                    sheetName = FromOrcadString( childPage.name );

                wxString base = sheetName;

                for( int suffix = 2; !m_usedSheetNames.insert( sheetName.Lower() ).second; ++suffix )
                    sheetName = wxString::Format( wxS( "%s (%d)" ), base, suffix );

                wxString fileName = MakePageFileName( ++pageIndex, childPage.name );
                const_cast<KIID&>( childSheet->m_Uuid ) = deterministicUuid( "sheet", pageIndex );
                childSheet->GetField( FIELD_T::SHEET_NAME )->SetText( sheetName );
                childSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
                placeHierarchicalBlockFields( childSheet, *drawn, occurrence.childFolder );
                childSheet->SetScreen( childScreen );
                childScreen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

                auto implementation = drawn->props.find( "Implementation" );

                if( implementation != drawn->props.end() && !implementation->second.empty()
                    && FromOrcadString( implementation->second )
                                       .CmpNoCase( FromOrcadString( occurrence.childFolder ) )
                               != 0 )
                {
                    childSheet->SetExcludedFromBoard( true );
                }

                size_t pinOrdinal = 0;

                for( const ORCAD_BLOCK_PIN& sourcePin : drawn->pins )
                {
                    VECTOR2I    position = OrcadDbuToIu( sourcePin.x, sourcePin.y );
                    std::string sourceName = canonicalGlobalNetName( sourcePin.name );
                    uint32_t    busNetId = busNetAt( aParentPage, sourcePin );
                    std::string pinName = busNetId ? connectedBusName( aParentPage, sourcePin, sourceName )
                                                   : scopedHierBusName( sourceName, occurrence.targetDbId );

                    if( busNetId && pinName != sourceName )
                    {
                        auto parentBusNames =
                                m_hierBusNamesByScreen.find( aParentSheet->GetScreen()->GetUuid().AsStdString() );

                        if( parentBusNames != m_hierBusNamesByScreen.end() )
                            pinName = scopedHierBusRange( pinName, parentBusNames->second );
                    }

                    if( pinName != sourceName )
                        m_hierBusNamesByScreen[childScreen->GetUuid().AsStdString()][sourceName] = pinName;

                    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( childSheet, position, FromOrcadString( pinName ) );
                    const_cast<KIID&>( pin->m_Uuid ) =
                            deterministicUuid( "sheet-pin:" + childScreen->GetUuid().AsStdString(), pinOrdinal++ );
                    std::array<std::pair<int, SHEET_SIDE>, 4> sides = {
                        std::pair{ std::abs( sourcePin.x - drawn->x1 ), SHEET_SIDE::LEFT },
                        std::pair{ std::abs( sourcePin.x - drawn->x1 - drawn->w ), SHEET_SIDE::RIGHT },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 ), SHEET_SIDE::TOP },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 - drawn->h ), SHEET_SIDE::BOTTOM }
                    };

                    pin->SetSide( std::min_element( sides.begin(), sides.end(),
                                                    []( const auto& a, const auto& b )
                                                    {
                                                        return a.first < b.first;
                                                    } )
                                          ->second );
                    pin->SetPosition( position );

                    switch( sourcePin.portType )
                    {
                    case ORCAD_PORT_TYPE::INPUT: pin->SetShape( LABEL_FLAG_SHAPE::L_INPUT ); break;
                    case ORCAD_PORT_TYPE::OUTPUT: pin->SetShape( LABEL_FLAG_SHAPE::L_OUTPUT ); break;
                    case ORCAD_PORT_TYPE::BIDIRECTIONAL: pin->SetShape( LABEL_FLAG_SHAPE::L_BIDI ); break;
                    case ORCAD_PORT_TYPE::TRI_STATE: pin->SetShape( LABEL_FLAG_SHAPE::L_TRISTATE ); break;
                    default: pin->SetShape( LABEL_FLAG_SHAPE::L_UNSPECIFIED ); break;
                    }

                    childSheet->AddPin( pin );
                    placeHierarchicalBlockPinFill( aParentSheet->GetScreen(), pin );
                }

                aParentSheet->GetScreen()->Append( childSheet );

                SCH_SHEET_PATH childPath = aParentPath;
                childPath.push_back( childSheet );
                childPath.SetPageNumber( wxString::Format( wxS( "%d" ), pageIndex ) );

                std::map<std::string, std::string> childInterfaceAliases;

                for( const ORCAD_BLOCK_PIN& sourcePin : drawn->pins )
                {
                    std::set<std::string> sourceNames;
                    std::set<uint32_t>    wireObjectIds;
                    std::string           pinName = canonicalGlobalNetName( sourcePin.name );
                    std::string           pinKey = pinName;
                    std::transform( pinKey.begin(), pinKey.end(), pinKey.begin(),
                                    []( unsigned char c )
                                    {
                                        return static_cast<char>( std::tolower( c ) );
                                    } );
                    sourceNames.insert( pinKey );

                    for( const ORCAD_WIRE& wire : aParentPage.wires )
                    {
                        if( wire.isBus || !rawPointOnSegment( sourcePin.x, sourcePin.y, wire ) )
                            continue;

                        wireObjectIds.insert( wire.dbId );
                        auto pageName = aParentPage.netmap.find( wire.id );

                        if( pageName != aParentPage.netmap.end() && !pageName->second.empty() )
                        {
                            std::string name = canonicalGlobalNetName( pageName->second );
                            std::transform( name.begin(), name.end(), name.begin(),
                                            []( unsigned char c )
                                            {
                                                return static_cast<char>( std::tolower( c ) );
                                            } );
                            sourceNames.insert( std::move( name ) );
                        }

                        auto aliases = aParentPage.netAliases.find( wire.id );

                        if( aliases != aParentPage.netAliases.end() )
                        {
                            for( const std::string& alias : aliases->second )
                            {
                                std::string name = canonicalGlobalNetName( alias );
                                std::transform( name.begin(), name.end(), name.begin(),
                                                []( unsigned char c )
                                                {
                                                    return static_cast<char>( std::tolower( c ) );
                                                } );
                                sourceNames.insert( std::move( name ) );
                            }
                        }
                    }

                    std::set<const std::string*> targets;

                    for( const auto& [occurrenceId, occurrenceName] : aScope.netNames )
                    {
                        std::string occurrenceKey = canonicalGlobalNetName( occurrenceName );
                        std::transform( occurrenceKey.begin(), occurrenceKey.end(), occurrenceKey.begin(),
                                        []( unsigned char c )
                                        {
                                            return static_cast<char>( std::tolower( c ) );
                                        } );
                        bool matches = sourceNames.count( occurrenceKey );

                        if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
                            matches = matches || wireObjectIds.count( *objectId );

                        if( matches )
                            targets.insert( &occurrenceName );
                    }

                    if( targets.size() == 1 )
                        childInterfaceAliases[pinKey] = canonicalGlobalNetName( **targets.begin() );
                }

                pollProgress( m_progressReporter, childPage.name );
                m_currentOccRefs = &occurrence.scope.partRefs;
                m_currentOccUnitRefs = &occurrence.scope.partUnitRefs;
                m_currentOccProps = &occurrence.scope.partProps;
                m_currentOccNetNames = &occurrence.scope.netNames;
                selectOccurrenceAliases( m_currentOccNetNames );
                m_currentFlatNetSuffix = simpleRepeatedLeafDesign ? flatNetSuffix( *drawn ) : std::string();
                m_scopeNamedFlatNets = simpleRepeatedLeafDesign;
                m_scopeGeneratedFlatNets = occurrenceFolderCounts[key] > 1;
                m_currentUnconnectedInterfaceNetNames = unconnectedInterfaceNetNames( *drawn, m_currentFlatNetSuffix );
                m_currentInterfaceNetAliases = std::move( childInterfaceAliases );
                m_currentConnectorInterfaceNetAliases.clear();
                applyPageSettings( childPage, childScreen );
                convertPage( childPage, childScreen, childPath, true, drawn->pins.empty() );
                placeChildren( childPage, occurrence.scope, childSheet, childPath );
            }
        };

        placeChildren( rootPage, m_design.occurrenceRoot, aRootSheet, rootPath );
        m_currentOccRefs = nullptr;
        m_currentOccUnitRefs = nullptr;
        m_currentOccProps = nullptr;
        m_currentOccNetNames = nullptr;
        m_currentFlatNetSuffix.clear();
        m_scopeNamedFlatNets = false;
        m_scopeGeneratedFlatNets = false;
        m_currentUnconnectedInterfaceNetNames.clear();
        promoteUniqueOccurrenceNetNames();
        convertUnreferencedPages();
        return aRootSheet;
    }

    std::function<bool( const std::vector<ORCAD_RAW_PAGE>&, const ORCAD_OCC_SCOPE& )> canBuildFolderHierarchy =
            [&]( const std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope )
    {
        size_t blockCount = 0;

        for( const ORCAD_RAW_PAGE& page : aPages )
            blockCount += page.blocks.size();

        if( blockCount != aScope.blocks.size() )
            return false;

        std::set<uint32_t> matchedBlocks;

        for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
        {
            size_t matches = 0;

            for( const ORCAD_RAW_PAGE& page : aPages )
            {
                matches += std::count_if( page.blocks.begin(), page.blocks.end(),
                                          [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                                          {
                                              return aBlock.dbId == occurrence.targetDbId;
                                          } );
            }

            std::string key = occurrence.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            auto pages = m_design.childFolderPages.find( key );

            if( matches != 1 || !matchedBlocks.insert( occurrence.targetDbId ).second
                || pages == m_design.childFolderPages.end() || pages->second.empty()
                || !canBuildFolderHierarchy( pages->second, occurrence.scope ) )
            {
                return false;
            }
        }

        return true;
    };

    bool folderHierarchy = !m_design.occurrenceRoot.blocks.empty()
                           && canBuildFolderHierarchy( m_design.pages, m_design.occurrenceRoot );

    struct POWER_ALIAS_EVIDENCE
    {
        size_t                                                  placements = 0;
        std::map<std::string, std::pair<std::string, size_t>> targets;
    };

    auto lowerPowerName = []( std::string aName )
    {
        std::transform( aName.begin(), aName.end(), aName.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return aName;
    };

    auto recordPowerAlias = [&]( std::map<std::string, POWER_ALIAS_EVIDENCE>& aCandidates,
                                 const std::string& aSourceName, const std::string& aElectricalName )
    {
        if( aSourceName.empty() || aElectricalName.empty() )
            return;

        POWER_ALIAS_EVIDENCE& evidence = aCandidates[lowerPowerName( aSourceName )];
        ++evidence.placements;

        if( !isPowerNetName( aElectricalName )
            || wxString::FromUTF8( aSourceName ).CmpNoCase( wxString::FromUTF8( aElectricalName ) ) == 0 )
        {
            return;
        }

        auto& target = evidence.targets[lowerPowerName( aElectricalName )];
        target.first = aElectricalName;
        ++target.second;
    };

    auto acceptPowerAliases = [&]( const std::map<std::string, POWER_ALIAS_EVIDENCE>& aCandidates )
    {
        for( const auto& [sourceName, evidence] : aCandidates )
        {
            if( evidence.targets.size() != 1 )
                continue;

            const auto& target = evidence.targets.begin()->second;

            if( target.second * 2 > evidence.placements )
                m_globalNetAliases[sourceName] = target.first;
        }
    };

    if( folderHierarchy )
    {
        int    pageIndex = 1;
        size_t sourcePageIndex = 0;

        std::function<size_t( const std::vector<ORCAD_RAW_PAGE>&, const ORCAD_OCC_SCOPE& )> countSourcePages =
                [&]( const std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope )
        {
            size_t count = aPages.size();

            for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
            {
                std::string key = occurrence.childFolder;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                if( auto pages = m_design.childFolderPages.find( key ); pages != m_design.childFolderPages.end() )
                    count += countSourcePages( pages->second, occurrence.scope );
            }

            return count;
        };

        size_t sourcePageCount = countSourcePages( m_design.pages, m_design.occurrenceRoot );
        auto   numberSourcePage = [&]( ORCAD_RAW_PAGE& aPage )
        {
            aPage.sourcePageNumber = ++sourcePageIndex;
            aPage.sourcePageCount = sourcePageCount;
        };

        auto uniqueSheetName = [&]( const std::string& aName )
        {
            wxString name = FromOrcadString( aName );

            if( name.IsEmpty() )
                name = wxS( "PAGE" );

            wxString base = name;

            for( int suffix = 2; !m_usedSheetNames.insert( name.Lower() ).second; ++suffix )
                name = wxString::Format( wxS( "%s (%d)" ), base, suffix );

            return name;
        };

        auto interfaceNames = [&]( const ORCAD_RAW_PAGE& aPage )
        {
            std::vector<std::string> names;
            std::set<std::string>    seen;

            auto collect = [&]( const std::vector<ORCAD_GRAPHIC_INST>& aConnectors )
            {
                for( const ORCAD_GRAPHIC_INST& connector : aConnectors )
                {
                    std::string name = canonicalGlobalNetName( connector.logicalName );

                    if( name.empty() )
                        name = canonicalGlobalNetName( connector.name );

                    std::string key = name;
                    std::transform( key.begin(), key.end(), key.begin(),
                                    []( unsigned char c )
                                    {
                                        return static_cast<char>( std::tolower( c ) );
                                    } );

                    if( !name.empty() && seen.insert( key ).second )
                        names.push_back( std::move( name ) );
                }
            };

            collect( aPage.ports );
            collect( aPage.offpage );
            return names;
        };

        auto addContainerLabel =
                [&]( SCH_SCREEN* aScreen, const wxString& aName, const VECTOR2I& aPosition, bool aHierarchical )
        {
            SCH_LABEL_BASE* label = aHierarchical
                                            ? static_cast<SCH_LABEL_BASE*>( new SCH_HIERLABEL( aPosition, aName ) )
                                            : static_cast<SCH_LABEL_BASE*>( new SCH_LABEL( aPosition, aName ) );
            label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
            appendPageItem( aScreen, label );
        };

        auto addChildOccurrenceAliases = [&]( const ORCAD_RAW_PAGE& aParentPage, const ORCAD_OCC_SCOPE& aParentScope,
                                              const ORCAD_DRAWN_INSTANCE& aDrawn, const ORCAD_OCC_SCOPE& aChildScope )
        {
            auto lower = []( std::string aName )
            {
                std::transform( aName.begin(), aName.end(), aName.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );
                return aName;
            };
            auto generated = []( const std::string& aName )
            {
                return aName.size() > 1 && aName.front() == 'N'
                       && std::all_of( aName.begin() + 1, aName.end(),
                                       []( unsigned char c )
                                       {
                                           return std::isdigit( c );
                                       } );
            };

            auto& childAliases = occurrenceAliasesByScope[&aChildScope.netNames];

            for( const ORCAD_BLOCK_PIN& pin : aDrawn.pins )
            {
                std::set<std::string> sourceNames = { lower( canonicalGlobalNetName( pin.name ) ) };
                std::set<uint32_t>    wireObjectIds;

                for( const ORCAD_WIRE& wire : aParentPage.wires )
                {
                    if( wire.isBus || !rawPointOnSegment( pin.x, pin.y, wire ) )
                        continue;

                    wireObjectIds.insert( wire.dbId );
                    auto pageName = aParentPage.netmap.find( wire.id );

                    if( pageName != aParentPage.netmap.end() && !pageName->second.empty() )
                        sourceNames.insert( lower( canonicalGlobalNetName( pageName->second ) ) );

                    auto aliases = aParentPage.netAliases.find( wire.id );

                    if( aliases != aParentPage.netAliases.end() )
                    {
                        for( const std::string& alias : aliases->second )
                            sourceNames.insert( lower( canonicalGlobalNetName( alias ) ) );
                    }
                }

                std::set<const std::string*> targets;

                for( const auto& [occurrenceId, occurrenceName] : aParentScope.netNames )
                {
                    bool matches = sourceNames.count( lower( canonicalGlobalNetName( occurrenceName ) ) );

                    if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
                        matches = matches || wireObjectIds.count( *objectId );

                    if( matches && !generated( occurrenceName ) )
                        targets.insert( &occurrenceName );
                }

                if( targets.size() == 1 )
                {
                    std::string sourceName = lower( kicadOccurrenceNetName( pin.name ) );
                    std::string targetName = kicadOccurrenceNetName( **targets.begin() );

                    if( sourceName != lower( targetName ) )
                        childAliases[std::move( sourceName )] = std::move( targetName );
                }
            }
        };

        std::function<void( std::vector<ORCAD_RAW_PAGE>&, const ORCAD_OCC_SCOPE&, SCH_SHEET*, const SCH_SHEET_PATH&,
                            const std::string&, bool, const std::map<std::string, std::string>& )>
                placeFolder;

        placeFolder = [&]( std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope, SCH_SHEET* aFolderSheet,
                           const SCH_SHEET_PATH& aFolderPath, const std::string& aOccurrenceSuffix,
                           bool                                      aRepeatedFolder,
                           const std::map<std::string, std::string>& aUnconnectedInterfaceNetNames )
        {
            struct PAGE_PLACEMENT
            {
                ORCAD_RAW_PAGE* page;
                SCH_SHEET*      sheet;
                SCH_SCREEN*     screen;
                SCH_SHEET_PATH  path;
            };

            std::vector<PAGE_PLACEMENT> placements;
            bool                        leafFolder = std::all_of( aPages.begin(), aPages.end(),
                                                                  []( const ORCAD_RAW_PAGE& aPage )
                                                                  {
                                               return aPage.blocks.empty();
                                           } );
            m_currentFlatNetSuffix = aOccurrenceSuffix;
            m_scopeNamedFlatNets = simpleRepeatedLeafDesign && leafFolder;
            m_scopeGeneratedFlatNets = aRepeatedFolder;
            m_currentUnconnectedInterfaceNetNames =
                    leafFolder ? aUnconnectedInterfaceNetNames : std::map<std::string, std::string>();

            if( aPages.size() == 1 )
            {
                ORCAD_RAW_PAGE& page = aPages.front();
                pollProgress( m_progressReporter, page.name );
                m_currentOccRefs = &aScope.partRefs;
                m_currentOccUnitRefs = &aScope.partUnitRefs;
                m_currentOccProps = &aScope.partProps;
                m_currentOccNetNames = &aScope.netNames;
                selectOccurrenceAliases( m_currentOccNetNames );

                numberSourcePage( page );
                applyPageSettings( page, aFolderSheet->GetScreen() );
                convertPage( page, aFolderSheet->GetScreen(), aFolderPath, aFolderSheet != aRootSheet,
                             aFolderSheet->GetPins().empty() );
                placements.push_back( { &page, aFolderSheet, aFolderSheet->GetScreen(), aFolderPath } );
            }
            else if( aFolderSheet == aRootSheet )
            {
                std::vector<SCH_SHEET*>  topSheets;
                std::vector<SCH_SCREEN*> topScreens;

                for( size_t i = 0; i < aPages.size(); ++i )
                {
                    ORCAD_RAW_PAGE& page = aPages[i];
                    SCH_SHEET*      pageSheet = i == 0 ? aRootSheet : new SCH_SHEET( m_schematic );
                    SCH_SCREEN*     pageScreen = i == 0 ? aRootSheet->GetScreen() : new SCH_SCREEN( m_schematic );
                    int             currentPage = static_cast<int>( i + 1 );

                    if( i != 0 )
                    {
                        const_cast<KIID&>( pageScreen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );
                        const_cast<KIID&>( pageSheet->m_Uuid ) = deterministicUuid( "sheet", currentPage );
                        pageSheet->SetScreen( pageScreen );
                    }

                    wxString fileName = MakePageFileName( currentPage, page.name );
                    pageSheet->GetField( FIELD_T::SHEET_NAME )->SetText( uniqueSheetName( page.name ) );
                    pageSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
                    pageScreen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );
                    topSheets.push_back( pageSheet );
                    topScreens.push_back( pageScreen );
                }

                m_schematic->SetTopLevelSheets( topSheets );
                pageIndex = static_cast<int>( aPages.size() );

                for( size_t i = 0; i < aPages.size(); ++i )
                {
                    ORCAD_RAW_PAGE& page = aPages[i];
                    SCH_SHEET_PATH  pagePath;

                    for( const SCH_SHEET_PATH& candidate : m_schematic->Hierarchy() )
                    {
                        if( candidate.Last() == topSheets[i] )
                        {
                            pagePath = candidate;
                            break;
                        }
                    }

                    if( pagePath.empty() )
                        pagePath.push_back( topSheets[i] );

                    pagePath.SetPageNumber( wxString::Format( wxS( "%zu" ), i + 1 ) );
                    pollProgress( m_progressReporter, page.name );
                    m_currentOccRefs = &aScope.partRefs;
                    m_currentOccUnitRefs = &aScope.partUnitRefs;
                    m_currentOccProps = &aScope.partProps;
                    m_currentOccNetNames = &aScope.netNames;
                    selectOccurrenceAliases( m_currentOccNetNames );
                    numberSourcePage( page );
                    applyPageSettings( page, topScreens[i] );
                    convertPage( page, topScreens[i], pagePath, false, true );
                    placements.push_back( { &page, topSheets[i], topScreens[i], pagePath } );
                }
            }
            else
            {
                SCH_SCREEN* container = aFolderSheet->GetScreen();
                container->SetPageSettings( PAGE_INFO( PAGE_SIZE_TYPE::A4 ) );
                size_t outerOrdinal = 0;
                auto   folderBusNames = m_hierBusNamesByScreen.find( container->GetUuid().AsStdString() );

                for( const SCH_SHEET_PIN* pin : aFolderSheet->GetPins() )
                {
                    VECTOR2I position( schIUScale.mmToIU( 20 ),
                                       schIUScale.mmToIU( 20 + 5 * static_cast<int>( outerOrdinal++ ) ) );
                    addContainerLabel( container, pin->GetText(), position, true );
                }

                for( size_t i = 0; i < aPages.size(); ++i )
                {
                    ORCAD_RAW_PAGE&          page = aPages[i];
                    int                      column = static_cast<int>( i % 3 );
                    int                      row = static_cast<int>( i / 3 );
                    std::vector<std::string> names = interfaceNames( page );

                    if( folderBusNames != m_hierBusNamesByScreen.end() )
                    {
                        for( std::string& name : names )
                        {
                            auto renamed = folderBusNames->second.find( name );

                            if( renamed != folderBusNames->second.end() )
                                name = renamed->second;
                        }
                    }

                    int         heightMm = std::max( 25, 10 + 5 * static_cast<int>( names.size() ) );
                    VECTOR2I    position( schIUScale.mmToIU( 55 + column * 70 ), schIUScale.mmToIU( 15 + row * 70 ) );
                    VECTOR2I    size( schIUScale.mmToIU( 55 ), schIUScale.mmToIU( heightMm ) );
                    SCH_SCREEN* pageScreen = new SCH_SCREEN( m_schematic );
                    const_cast<KIID&>( pageScreen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );

                    if( folderBusNames != m_hierBusNamesByScreen.end() )
                        m_hierBusNamesByScreen[pageScreen->GetUuid().AsStdString()] = folderBusNames->second;

                    SCH_SHEET* pageSheet = new SCH_SHEET( aFolderSheet, position, size );
                    int        currentPage = ++pageIndex;
                    const_cast<KIID&>( pageSheet->m_Uuid ) = deterministicUuid( "sheet", currentPage );
                    wxString fileName = MakePageFileName( currentPage, page.name );
                    pageSheet->GetField( FIELD_T::SHEET_NAME )->SetText( uniqueSheetName( page.name ) );
                    pageSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
                    pageSheet->SetScreen( pageScreen );
                    pageScreen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

                    for( size_t pinIndex = 0; pinIndex < names.size(); ++pinIndex )
                    {
                        VECTOR2I       pinPosition( position.x,
                                                    position.y + schIUScale.mmToIU( 5 + 5 * static_cast<int>( pinIndex ) ) );
                        wxString       name = FromOrcadString( names[pinIndex] );
                        SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( pageSheet, pinPosition, name );
                        const_cast<KIID&>( pin->m_Uuid ) =
                                deterministicUuid( "container-pin:" + pageScreen->GetUuid().AsStdString(), pinIndex );
                        pin->SetSide( SHEET_SIDE::LEFT );
                        pin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
                        pageSheet->AddPin( pin );
                        addContainerLabel( container, name, pinPosition, false );
                    }

                    container->Append( pageSheet );
                    SCH_SHEET_PATH pagePath = aFolderPath;
                    pagePath.push_back( pageSheet );
                    pagePath.SetPageNumber( wxString::Format( wxS( "%d" ), currentPage ) );
                    pollProgress( m_progressReporter, page.name );
                    m_currentOccRefs = &aScope.partRefs;
                    m_currentOccUnitRefs = &aScope.partUnitRefs;
                    m_currentOccProps = &aScope.partProps;
                    m_currentOccNetNames = &aScope.netNames;
                    selectOccurrenceAliases( m_currentOccNetNames );
                    numberSourcePage( page );
                    applyPageSettings( page, pageScreen );
                    convertPage( page, pageScreen, pagePath, true, true );
                    placements.push_back( { &page, pageSheet, pageScreen, pagePath } );
                }
            }

            std::vector<const ORCAD_OCC_BLOCK*> occurrences;

            for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
                occurrences.push_back( &occurrence );

            std::stable_sort( occurrences.begin(), occurrences.end(),
                              []( const ORCAD_OCC_BLOCK* a, const ORCAD_OCC_BLOCK* b )
                              {
                                  wxString aName = FromOrcadString( a->childFolder );
                                  wxString bName = FromOrcadString( b->childFolder );
                                  int      aOrder = OrcadPageOrder( aName );
                                  int      bOrder = OrcadPageOrder( bName );
                                  int      aKey = aOrder >= 0 ? aOrder : std::numeric_limits<int>::max();
                                  int      bKey = bOrder >= 0 ? bOrder : std::numeric_limits<int>::max();

                                  if( aKey != bKey )
                                      return aKey < bKey;

                                  return aName.CmpNoCase( bName ) < 0;
                              } );

            for( const ORCAD_OCC_BLOCK* occurrencePtr : occurrences )
            {
                const ORCAD_OCC_BLOCK& occurrence = *occurrencePtr;
                PAGE_PLACEMENT*        parent = nullptr;
                ORCAD_DRAWN_INSTANCE*  drawn = nullptr;

                for( PAGE_PLACEMENT& placement : placements )
                {
                    auto found = std::find_if( placement.page->blocks.begin(), placement.page->blocks.end(),
                                               [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                                               {
                                                   return aBlock.dbId == occurrence.targetDbId;
                                               } );

                    if( found != placement.page->blocks.end() )
                    {
                        parent = &placement;
                        drawn = &*found;
                        break;
                    }
                }

                if( !parent || !drawn )
                    continue;

                std::string key = occurrence.childFolder;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );
                std::vector<ORCAD_RAW_PAGE>& childPages = m_design.childFolderPages.at( key );
                SCH_SCREEN*                  childScreen = new SCH_SCREEN( m_schematic );
                const_cast<KIID&>( childScreen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );
                SCH_SHEET* childSheet = new SCH_SHEET( parent->sheet, OrcadDbuToIu( drawn->x1, drawn->y1 ),
                                                       OrcadDbuToIu( drawn->w, drawn->h ) );
                int        currentPage = ++pageIndex;
                const_cast<KIID&>( childSheet->m_Uuid ) = deterministicUuid( "sheet", currentPage );
                wxString fileName =
                        MakePageFileName( currentPage, childPages.size() == 1 ? childPages.front().name
                                                                              : occurrence.childFolder + " container" );
                childSheet->GetField( FIELD_T::SHEET_NAME )
                        ->SetText( uniqueSheetName( drawn->reference.empty() ? occurrence.childFolder
                                                                             : drawn->reference ) );
                childSheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
                placeHierarchicalBlockFields( childSheet, *drawn, occurrence.childFolder );
                childSheet->SetScreen( childScreen );
                childScreen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

                auto implementation = drawn->props.find( "Implementation" );

                if( implementation != drawn->props.end() && !implementation->second.empty()
                    && FromOrcadString( implementation->second )
                                       .CmpNoCase( FromOrcadString( occurrence.childFolder ) )
                               != 0 )
                {
                    childSheet->SetExcludedFromBoard( true );
                }

                for( size_t pinIndex = 0; pinIndex < drawn->pins.size(); ++pinIndex )
                {
                    const ORCAD_BLOCK_PIN& sourcePin = drawn->pins[pinIndex];
                    VECTOR2I               position = OrcadDbuToIu( sourcePin.x, sourcePin.y );
                    std::string            sourceName = canonicalGlobalNetName( sourcePin.name );
                    uint32_t               busNetId = busNetAt( *parent->page, sourcePin );
                    std::string            pinName = busNetId ? connectedBusName( *parent->page, sourcePin, sourceName )
                                                              : scopedHierBusName( sourceName, occurrence.targetDbId );

                    if( busNetId && pinName != sourceName )
                    {
                        auto parentBusNames = m_hierBusNamesByScreen.find( parent->screen->GetUuid().AsStdString() );

                        if( parentBusNames != m_hierBusNamesByScreen.end() )
                            pinName = scopedHierBusRange( pinName, parentBusNames->second );
                    }

                    if( pinName != sourceName )
                        m_hierBusNamesByScreen[childScreen->GetUuid().AsStdString()][sourceName] = pinName;

                    SCH_SHEET_PIN* pin = new SCH_SHEET_PIN( childSheet, position, FromOrcadString( pinName ) );
                    const_cast<KIID&>( pin->m_Uuid ) =
                            deterministicUuid( "sheet-pin:" + childScreen->GetUuid().AsStdString(), pinIndex );
                    std::array<std::pair<int, SHEET_SIDE>, 4> sides = {
                        std::pair{ std::abs( sourcePin.x - drawn->x1 ), SHEET_SIDE::LEFT },
                        std::pair{ std::abs( sourcePin.x - drawn->x1 - drawn->w ), SHEET_SIDE::RIGHT },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 ), SHEET_SIDE::TOP },
                        std::pair{ std::abs( sourcePin.y - drawn->y1 - drawn->h ), SHEET_SIDE::BOTTOM }
                    };
                    pin->SetSide( std::min_element( sides.begin(), sides.end(),
                                                    []( const auto& a, const auto& b )
                                                    {
                                                        return a.first < b.first;
                                                    } )
                                          ->second );
                    pin->SetPosition( position );
                    pin->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
                    childSheet->AddPin( pin );
                    placeHierarchicalBlockPinFill( parent->screen, pin );
                }

                parent->screen->Append( childSheet );
                SCH_SHEET_PATH childPath = parent->path;
                childPath.push_back( childSheet );
                childPath.SetPageNumber( wxString::Format( wxS( "%d" ), currentPage ) );
                std::string childSuffix = aOccurrenceSuffix;
                std::string childKey = occurrence.childFolder;
                std::transform( childKey.begin(), childKey.end(), childKey.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                if( !flatNetSuffix( *drawn ).empty() )
                {
                    if( !childSuffix.empty() )
                        childSuffix += '_';

                    childSuffix += flatNetSuffix( *drawn );
                }

                auto unconnectedNames = unconnectedInterfaceNetNames( *drawn, childSuffix );
                addChildOccurrenceAliases( *parent->page, aScope, *drawn, occurrence.scope );
                placeFolder( childPages, occurrence.scope, childSheet, childPath, childSuffix,
                             occurrenceFolderCounts[childKey] > 1, unconnectedNames );
            }
        };

        std::map<std::string, POWER_ALIAS_EVIDENCE> powerAliasCandidates;
        std::function<void( std::vector<ORCAD_RAW_PAGE>&, const ORCAD_OCC_SCOPE& )> collectPowerAliases =
                [&]( std::vector<ORCAD_RAW_PAGE>& aPages, const ORCAD_OCC_SCOPE& aScope )
        {
            m_currentOccRefs = &aScope.partRefs;
            m_currentOccUnitRefs = &aScope.partUnitRefs;
            m_currentOccProps = &aScope.partProps;
            m_currentOccNetNames = &aScope.netNames;
            selectOccurrenceAliases( m_currentOccNetNames );

            for( ORCAD_RAW_PAGE& page : aPages )
            {
                buildNetLookup( page );

                for( const ORCAD_GRAPHIC_INST& global : page.globals )
                {
                    std::string sourceName = trimmed( global.logicalName );
                    std::string electricalName = powerNet( page, global );
                    recordPowerAlias( powerAliasCandidates, sourceName, electricalName );
                }
            }

            for( const ORCAD_OCC_BLOCK& occurrence : aScope.blocks )
            {
                ORCAD_RAW_PAGE*        parentPage = nullptr;
                ORCAD_DRAWN_INSTANCE* drawn = nullptr;

                for( ORCAD_RAW_PAGE& page : aPages )
                {
                    auto found = std::find_if( page.blocks.begin(), page.blocks.end(),
                                               [&]( const ORCAD_DRAWN_INSTANCE& aBlock )
                                               {
                                                   return aBlock.dbId == occurrence.targetDbId;
                                               } );

                    if( found != page.blocks.end() )
                    {
                        parentPage = &page;
                        drawn = &*found;
                        break;
                    }
                }

                std::string key = lowerPowerName( occurrence.childFolder );
                auto        childPages = m_design.childFolderPages.find( key );

                if( parentPage && drawn && childPages != m_design.childFolderPages.end() )
                {
                    addChildOccurrenceAliases( *parentPage, aScope, *drawn, occurrence.scope );
                    collectPowerAliases( childPages->second, occurrence.scope );
                }
            }
        };

        collectPowerAliases( m_design.pages, m_design.occurrenceRoot );
        acceptPowerAliases( powerAliasCandidates );

        m_currentOccRefs = nullptr;
        m_currentOccUnitRefs = nullptr;
        m_currentOccProps = nullptr;
        m_currentOccNetNames = nullptr;
        m_currentOccurrenceNetAliases.clear();

        placeFolder( m_design.pages, m_design.occurrenceRoot, aRootSheet, rootPath, {}, false, {} );
        m_currentOccRefs = nullptr;
        m_currentOccUnitRefs = nullptr;
        m_currentOccProps = nullptr;
        m_currentOccNetNames = nullptr;
        m_currentFlatNetSuffix.clear();
        m_scopeNamedFlatNets = false;
        m_scopeGeneratedFlatNets = false;
        m_currentUnconnectedInterfaceNetNames.clear();
        m_currentOccurrenceNetAliases.clear();
        promoteUniqueOccurrenceNetNames();
        convertUnreferencedPages();
        return aRootSheet;
    }

    // Page list = root pages + each block occurrence's child pages, tagged w/ scope refs.
    // Child schematic reused N times yields N jobs, each w/ own designators.
    struct PAGE_JOB
    {
        ORCAD_RAW_PAGE*                        page;
        const std::map<uint32_t, std::string>* refs;
        const std::map<uint32_t, std::string>* unitRefs;
        const std::map<uint32_t, std::map<std::string, std::string>>* props;
        const std::map<uint32_t, std::string>* netNames;
        std::string                            flatNetSuffix;
        bool                                   scopeNamedFlatNets;
        bool                                   scopeGeneratedFlatNets;
        std::map<std::string, std::string>     unconnectedInterfaceNetNames;
    };

    std::vector<PAGE_JOB> jobs;

    for( ORCAD_RAW_PAGE& page : m_design.pages )
        jobs.push_back( { &page,
                          &m_design.occurrenceRoot.partRefs,
                          &m_design.occurrenceRoot.partUnitRefs,
                          &m_design.occurrenceRoot.partProps,
                          &m_design.occurrenceRoot.netNames,
                          {},
                          false,
                          false,
                          {} } );

    auto findBlock = [&]( uint32_t aDbId ) -> std::pair<const ORCAD_DRAWN_INSTANCE*, const ORCAD_RAW_PAGE*>
    {
        auto findInPages = [&]( const std::vector<ORCAD_RAW_PAGE>& aPages )
                -> std::pair<const ORCAD_DRAWN_INSTANCE*, const ORCAD_RAW_PAGE*>
        {
            for( const ORCAD_RAW_PAGE& page : aPages )
            {
                for( const ORCAD_DRAWN_INSTANCE& block : page.blocks )
                {
                    if( block.dbId == aDbId )
                        return { &block, &page };
                }
            }

            return {};
        };

        auto result = findInPages( m_design.pages );

        if( result.first )
            return result;

        for( const auto& [folder, pages] : m_design.childFolderPages )
        {
            result = findInPages( pages );

            if( result.first )
                return result;
        }

        return {};
    };

    std::function<void( const ORCAD_OCC_SCOPE&, const std::string& )> expand =
            [&]( const ORCAD_OCC_SCOPE& aScope, const std::string& aParentSuffix )
    {
        for( const ORCAD_OCC_BLOCK& block : aScope.blocks )
        {
            std::string key = block.childFolder;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            auto                        it = m_design.childFolderPages.find( key );
            const ORCAD_DRAWN_INSTANCE* drawn = findBlock( block.targetDbId ).first;
            std::string                 occurrenceSuffix = aParentSuffix;

            if( drawn && !flatNetSuffix( *drawn ).empty() )
            {
                if( !occurrenceSuffix.empty() )
                    occurrenceSuffix += '_';

                occurrenceSuffix += flatNetSuffix( *drawn );
            }

            if( it != m_design.childFolderPages.end() )
            {
                bool                               leafFolder = std::all_of( it->second.begin(), it->second.end(),
                                                                             []( const ORCAD_RAW_PAGE& aPage )
                                                                             {
                                                   return aPage.blocks.empty();
                                               } );
                std::map<std::string, std::string> unconnectedNames;

                if( drawn )
                    unconnectedNames = unconnectedInterfaceNetNames( *drawn, occurrenceSuffix );

                for( ORCAD_RAW_PAGE& childPage : it->second )
                {
                    jobs.push_back( { &childPage, &block.scope.partRefs, &block.scope.partUnitRefs,
                                      &block.scope.partProps, &block.scope.netNames,
                                      leafFolder ? occurrenceSuffix : std::string(),
                                      simpleRepeatedLeafDesign && leafFolder,
                                      leafFolder && occurrenceFolderCounts[key] > 1, unconnectedNames } );
                }
            }

            expand( block.scope, occurrenceSuffix );
        }
    };

    expand( m_design.occurrenceRoot, {} );

    std::map<const std::map<uint32_t, std::string>*, std::map<std::string, std::set<std::string>>>
            interfaceAliasCandidates;
    std::map<const std::map<uint32_t, std::string>*, std::map<std::string, std::set<std::string>>>
            connectorAliasCandidates;

    auto lowerName = []( std::string aName )
    {
        std::transform( aName.begin(), aName.end(), aName.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        return aName;
    };

    using NET_NAME_SCOPE = const std::map<uint32_t, std::string>*;
    std::map<NET_NAME_SCOPE, std::map<std::string, std::set<std::string>>> interfaceNameGraphs;

    for( const PAGE_JOB& job : jobs )
    {
        for( const auto& [netId, aliases] : job.page->netAliases )
        {
            std::set<std::string> names;
            auto                  primary = job.page->netmap.find( netId );
            bool                  allPowerNames = true;
            bool                  anyPowerName = false;

            if( primary != job.page->netmap.end() && !primary->second.empty() )
            {
                std::string name = lowerName( canonicalGlobalNetName( primary->second ) );
                names.insert( name );
                allPowerNames = isPowerNetName( primary->second );
                anyPowerName = allPowerNames;
            }

            for( const std::string& alias : aliases )
            {
                if( !alias.empty() )
                {
                    names.insert( lowerName( canonicalGlobalNetName( alias ) ) );
                    allPowerNames = allPowerNames && isPowerNetName( alias );
                    anyPowerName = anyPowerName || isPowerNetName( alias );
                }
            }

            if( anyPowerName && !allPowerNames )
                continue;

            for( const std::string& name : names )
            {
                auto& neighbors = interfaceNameGraphs[job.netNames][name];
                neighbors.insert( names.begin(), names.end() );
            }
        }
    }

    for( const PAGE_JOB& job : jobs )
    {
        for( const auto& [netId, aliases] : job.page->netAliases )
        {
            std::set<std::string> distinctAliases;

            for( const std::string& alias : aliases )
            {
                if( !alias.empty() )
                    distinctAliases.insert( lowerName( alias ) );
            }

            auto primary = job.page->netmap.find( netId );

            if( distinctAliases.size() < 2 || primary == job.page->netmap.end() || primary->second.empty() )
                continue;

            std::string effectiveName;
            int64_t     bestConnectorAliasDistance = std::numeric_limits<int64_t>::max();
            bool        connectorAlias = false;
            auto        firstAlias = std::find_if( aliases.begin(), aliases.end(),
                                                   []( const std::string& aName )
                                                   {
                                                return !aName.empty();
                                            } );

            for( const auto& [occurrenceId, occurrenceName] : *job.netNames )
            {
                if( lowerName( primary->second ) == lowerName( occurrenceName ) )
                    effectiveName = canonicalGlobalNetName( occurrenceName );
            }

            if( effectiveName.empty() )
            {
                for( const ORCAD_PLACED_INSTANCE& instance : job.page->instances )
                {
                    if( instance.reference.empty()
                        || std::toupper( static_cast<unsigned char>( instance.reference.front() ) ) != 'J' )
                    {
                        continue;
                    }

                    for( const ORCAD_PIN_INST& pin : instance.pins )
                    {
                        for( const ORCAD_WIRE& wire : job.page->wires )
                        {
                            if( wire.id != netId
                                || ( pin.wordA != wire.dbId && pin.wordB != wire.dbId
                                     && !rawPointOnSegment( pin.x, pin.y, wire ) ) )
                            {
                                continue;
                            }

                            size_t localAliasCount = std::count_if(
                                    wire.aliases.begin(), wire.aliases.end(),
                                    [&]( const ORCAD_ALIAS& aAlias )
                                    {
                                        if( aAlias.name.empty() || isOffpageNetName( aAlias.name ) )
                                        {
                                            return false;
                                        }

                                        std::string aliasName = lowerName( aAlias.name );

                                        return std::none_of( aliases.begin(), aliases.end(),
                                                             [&]( const std::string& aName )
                                                             {
                                                                 return isOffpageNetName( aName )
                                                                        && lowerName( aName ).find( aliasName )
                                                                                   != std::string::npos;
                                                             } );
                                    } );

                            if( localAliasCount < 2 )
                                continue;

                            for( const ORCAD_ALIAS& alias : wire.aliases )
                            {
                                if( alias.name.empty() || isOffpageNetName( alias.name ) || firstAlias == aliases.end()
                                    || lowerName( alias.name ) != lowerName( *firstAlias ) )
                                {
                                    continue;
                                }

                                int64_t dx = alias.x - pin.x;
                                int64_t dy = alias.y - pin.y;
                                int64_t distance = dx * dx + dy * dy;

                                if( distance < bestConnectorAliasDistance )
                                {
                                    bestConnectorAliasDistance = distance;
                                    effectiveName = canonicalGlobalNetName( alias.name );
                                    connectorAlias = true;
                                }
                            }
                        }
                    }
                }
            }

            if( !effectiveName.empty() && !connectorAlias )
            {
                connectorAlias = std::any_of(
                        job.page->instances.begin(), job.page->instances.end(),
                        [&]( const ORCAD_PLACED_INSTANCE& aInstance )
                        {
                            if( aInstance.reference.empty()
                                || std::toupper( static_cast<unsigned char>( aInstance.reference.front() ) ) != 'J' )
                            {
                                return false;
                            }

                            return std::any_of(
                                    aInstance.pins.begin(), aInstance.pins.end(),
                                    [&]( const ORCAD_PIN_INST& aPin )
                                    {
                                        return std::any_of(
                                                job.page->wires.begin(), job.page->wires.end(),
                                                [&]( const ORCAD_WIRE& aWire )
                                                {
                                                    if( aWire.id != netId
                                                        || ( aPin.wordA != aWire.dbId && aPin.wordB != aWire.dbId
                                                             && !rawPointOnSegment( aPin.x, aPin.y, aWire ) ) )
                                                    {
                                                        return false;
                                                    }

                                                    return std::any_of( aWire.aliases.begin(), aWire.aliases.end(),
                                                                        [&]( const ORCAD_ALIAS& aAlias )
                                                                        {
                                                                            return lowerName( canonicalGlobalNetName(
                                                                                           aAlias.name ) )
                                                                                   == lowerName( effectiveName );
                                                                        } );
                                                } );
                                    } );
                        } );
            }

            if( effectiveName.empty() )
                continue;

            for( const std::string& alias : aliases )
            {
                if( isOffpageNetName( alias ) )
                {
                    std::string sourceName = lowerName( canonicalGlobalNetName( alias ) );
                    interfaceAliasCandidates[job.netNames][sourceName].insert( effectiveName );

                    if( connectorAlias )
                        connectorAliasCandidates[job.netNames][sourceName].insert( effectiveName );
                }
            }
        }
    }

    std::map<const std::map<uint32_t, std::string>*, std::map<std::string, std::string>> interfaceAliasesByScope;
    std::map<const std::map<uint32_t, std::string>*, std::set<std::string>>              connectorAliasesByScope;

    for( const auto& [scope, aliases] : interfaceAliasCandidates )
    {
        for( const auto& [sourceName, effectiveNames] : aliases )
        {
            auto selected = std::max_element( effectiveNames.begin(), effectiveNames.end(),
                                              []( const std::string& aLeft, const std::string& aRight )
                                              {
                                                  return aLeft.size() < aRight.size();
                                              } );

            if( selected == effectiveNames.end() )
                continue;

            bool sourceIsOccurrenceName = std::any_of(
                    scope->begin(), scope->end(),
                    [&]( const auto& aOccurrenceNet )
                    {
                        return lowerName( canonicalGlobalNetName( aOccurrenceNet.second ) ) == sourceName;
                    } );
            bool selectedIsOccurrenceName = std::any_of(
                    scope->begin(), scope->end(),
                    [&]( const auto& aOccurrenceNet )
                    {
                        return lowerName( canonicalGlobalNetName( aOccurrenceNet.second ) ) == lowerName( *selected );
                    } );

            if( sourceIsOccurrenceName && selectedIsOccurrenceName && lowerName( *selected ) != sourceName )
                continue;

            bool uniqueLength = std::none_of( effectiveNames.begin(), effectiveNames.end(),
                                              [&]( const std::string& aName )
                                              {
                                                  return aName != *selected && aName.size() == selected->size();
                                              } );

            if( uniqueLength )
            {
                interfaceAliasesByScope[scope][sourceName] = *selected;

                auto connectorScope = connectorAliasCandidates.find( scope );

                if( connectorScope != connectorAliasCandidates.end() )
                {
                    auto connectorNames = connectorScope->second.find( sourceName );

                    if( connectorNames != connectorScope->second.end() && connectorNames->second.count( *selected ) )
                        connectorAliasesByScope[scope].insert( sourceName );
                }
            }
        }
    }

    for( const auto& [scope, graph] : interfaceNameGraphs )
    {
        std::set<std::string> unseen;

        for( const auto& [name, neighbors] : graph )
            unseen.insert( name );

        while( !unseen.empty() )
        {
            std::set<std::string>    component;
            std::vector<std::string> pending = { *unseen.begin() };
            unseen.erase( pending.front() );

            while( !pending.empty() )
            {
                std::string name = std::move( pending.back() );
                pending.pop_back();
                component.insert( name );

                for( const std::string& neighbor : graph.at( name ) )
                {
                    if( unseen.erase( neighbor ) )
                        pending.push_back( neighbor );
                }
            }

            std::set<std::string> authoritativeNames;

            for( const auto& [occurrenceId, occurrenceName] : *scope )
            {
                std::string key = lowerName( canonicalGlobalNetName( occurrenceName ) );

                if( component.count( key ) )
                    authoritativeNames.insert( canonicalGlobalNetName( occurrenceName ) );
            }

            if( authoritativeNames.size() != 1 )
                continue;

            for( const std::string& sourceName : component )
            {
                if( connectorAliasesByScope[scope].count( sourceName ) )
                    continue;

                interfaceAliasesByScope[scope][sourceName] = *authoritativeNames.begin();
                connectorAliasesByScope[scope].erase( sourceName );
            }
        }
    }

    std::function<void( const ORCAD_OCC_SCOPE& )> propagateInterfaceAliases =
            [&]( const ORCAD_OCC_SCOPE& aParentScope )
    {
        const auto& parentAliases = interfaceAliasesByScope[&aParentScope.netNames];

        for( const ORCAD_OCC_BLOCK& block : aParentScope.blocks )
        {
            auto& childAliases = interfaceAliasesByScope[&block.scope.netNames];
            auto [drawnBlock, parentPage] = findBlock( block.targetDbId );
            if( drawnBlock && parentPage )
            {
                for( const ORCAD_BLOCK_PIN& pin : drawnBlock->pins )
                {
                    if( pin.name.empty() )
                        continue;

                    std::set<std::string> sourceNames = { lowerName( canonicalGlobalNetName( pin.name ) ) };
                    std::set<uint32_t>    wireObjectIds;

                    for( const ORCAD_WIRE& wire : parentPage->wires )
                    {
                        if( wire.isBus || !rawPointOnSegment( pin.x, pin.y, wire ) )
                            continue;

                        wireObjectIds.insert( wire.dbId );
                        auto pageName = parentPage->netmap.find( wire.id );

                        if( pageName != parentPage->netmap.end() && !pageName->second.empty() )
                            sourceNames.insert( lowerName( canonicalGlobalNetName( pageName->second ) ) );

                        auto aliases = parentPage->netAliases.find( wire.id );

                        if( aliases != parentPage->netAliases.end() )
                        {
                            for( const std::string& alias : aliases->second )
                                sourceNames.insert( lowerName( canonicalGlobalNetName( alias ) ) );
                        }
                    }

                    std::set<const std::string*> targets;

                    for( const auto& [occurrenceId, occurrenceName] : aParentScope.netNames )
                    {
                        std::string occurrenceKey = lowerName( canonicalGlobalNetName( occurrenceName ) );
                        bool        matches = sourceNames.count( occurrenceKey );

                        if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
                            matches = matches || wireObjectIds.count( *objectId );

                        if( matches )
                            targets.insert( &occurrenceName );
                    }

                    std::string targetName;

                    if( targets.size() == 1 )
                        targetName = canonicalGlobalNetName( **targets.begin() );
                    else
                    {
                        for( const std::string& sourceName : sourceNames )
                        {
                            auto inherited = parentAliases.find( sourceName );

                            if( inherited != parentAliases.end() )
                                targetName = inherited->second;
                        }
                    }

                    if( !targetName.empty() )
                        childAliases[lowerName( canonicalGlobalNetName( pin.name ) )] = std::move( targetName );

                }
            }

            for( auto& [sourceName, targetName] : childAliases )
            {
                auto inherited = parentAliases.find( lowerName( canonicalGlobalNetName( targetName ) ) );

                if( inherited != parentAliases.end() )
                    targetName = inherited->second;
            }

            for( const auto& [occurrenceId, occurrenceName] : block.scope.netNames )
            {
                std::string sourceName = lowerName( canonicalGlobalNetName( occurrenceName ) );
                auto        inherited = parentAliases.find( sourceName );

                if( inherited != parentAliases.end() )
                    childAliases[sourceName] = inherited->second;
            }

            propagateInterfaceAliases( block.scope );
        }
    };
    propagateInterfaceAliases( m_design.occurrenceRoot );

    std::map<std::string, POWER_ALIAS_EVIDENCE> powerAliasCandidates;

    for( const PAGE_JOB& job : jobs )
    {
        m_currentOccRefs = job.refs;
        m_currentOccUnitRefs = job.unitRefs;
        m_currentOccProps = job.props;
        m_currentOccNetNames = job.netNames;
        selectOccurrenceAliases( m_currentOccNetNames );
        m_currentFlatNetSuffix = job.flatNetSuffix;
        m_scopeNamedFlatNets = job.scopeNamedFlatNets;
        m_scopeGeneratedFlatNets = job.scopeGeneratedFlatNets;
        m_currentUnconnectedInterfaceNetNames = job.unconnectedInterfaceNetNames;
        m_currentInterfaceNetAliases = interfaceAliasesByScope[job.netNames];
        m_currentConnectorInterfaceNetAliases = connectorAliasesByScope[job.netNames];
        buildNetLookup( *job.page );

        for( const ORCAD_GRAPHIC_INST& global : job.page->globals )
        {
            std::string sourceName = trimmed( global.logicalName );
            std::string electricalName = powerNet( *job.page, global );
            recordPowerAlias( powerAliasCandidates, sourceName, electricalName );
        }
    }

    acceptPowerAliases( powerAliasCandidates );

    m_currentOccRefs = nullptr;
    m_currentOccUnitRefs = nullptr;
    m_currentOccProps = nullptr;
    m_currentOccNetNames = nullptr;
    m_currentFlatNetSuffix.clear();
    m_scopeNamedFlatNets = false;
    m_scopeGeneratedFlatNets = false;
    m_currentUnconnectedInterfaceNetNames.clear();
    m_currentInterfaceNetAliases.clear();
    m_currentConnectorInterfaceNetAliases.clear();
    m_currentOccurrenceNetAliases.clear();

    if( jobs.size() == 1 )
    {
        PAGE_JOB& job = jobs[0];

        pollProgress( m_progressReporter, job.page->name );
        m_currentOccRefs = job.refs;
        m_currentOccUnitRefs = job.unitRefs;
        m_currentOccProps = job.props;
        m_currentOccNetNames = job.netNames;
        selectOccurrenceAliases( m_currentOccNetNames );
        m_currentFlatNetSuffix = job.flatNetSuffix;
        m_scopeNamedFlatNets = job.scopeNamedFlatNets;
        m_scopeGeneratedFlatNets = job.scopeGeneratedFlatNets;
        m_currentUnconnectedInterfaceNetNames = job.unconnectedInterfaceNetNames;
        m_currentInterfaceNetAliases = interfaceAliasesByScope[job.netNames];
        m_currentConnectorInterfaceNetAliases = connectorAliasesByScope[job.netNames];
        applyPageSettings( *job.page, rootScreen );
        convertPage( *job.page, rootScreen, rootPath );
        m_currentOccRefs = nullptr;
        m_currentOccUnitRefs = nullptr;
        m_currentOccProps = nullptr;
        m_currentOccNetNames = nullptr;
        m_currentFlatNetSuffix.clear();
        m_scopeNamedFlatNets = false;
        m_scopeGeneratedFlatNets = false;
        m_currentUnconnectedInterfaceNetNames.clear();
        m_currentInterfaceNetAliases.clear();
        m_currentConnectorInterfaceNetAliases.clear();
    }
    else
    {
        // Each page = flat top-level sheet (no stitching root); order root pages by
        // leading "N - " prefix, also stripped for title.
        struct SHEET_JOB
        {
            PAGE_JOB job;
            wxString name;
            int      order;
        };

        std::vector<SHEET_JOB> sheetJobs;

        for( size_t i = 0; i < jobs.size(); ++i )
        {
            wxString name = FromOrcadString( jobs[i].page->name );
            int      order = i < m_design.pages.size() ? OrcadPageOrder( name ) : -1;

            if( order < 0 && i < m_design.pages.size() )
            {
                for( const ORCAD_GRAPHIC_INST& titleBlock : jobs[i].page->titleBlocks )
                {
                    auto pageNumber = titleBlock.props.find( "Page Number" );
                    long parsed = 0;

                    if( pageNumber != titleBlock.props.end()
                        && wxString::FromUTF8( pageNumber->second ).ToLong( &parsed ) && parsed > 0 )
                    {
                        order = static_cast<int>( parsed );
                        break;
                    }
                }
            }

            if( order < 0 && i < m_design.pages.size() && jobs[i].page->sourcePageNumber != 0 )
                order = static_cast<int>( jobs[i].page->sourcePageNumber );

            if( name.IsEmpty() )
                name = wxString::Format( wxS( "PAGE%zu" ), i + 1 );

            sheetJobs.push_back( { jobs[i], name, order } );
        }

        std::stable_sort( sheetJobs.begin(), sheetJobs.end(),
                          []( const SHEET_JOB& a, const SHEET_JOB& b )
                          {
                              int ka = a.order >= 0 ? a.order : std::numeric_limits<int>::max();
                              int kb = b.order >= 0 ? b.order : std::numeric_limits<int>::max();
                              return ka < kb;
                          } );

        if( jobs.size() == m_design.pages.size() )
        {
            for( size_t i = 0; i < sheetJobs.size(); ++i )
            {
                sheetJobs[i].job.page->sourcePageNumber = i + 1;
                sheetJobs[i].job.page->sourcePageCount = sheetJobs.size();
            }
        }

        std::vector<SCH_SHEET*>  topSheets;
        std::vector<SCH_SCREEN*> topScreens;

        for( size_t i = 0; i < sheetJobs.size(); ++i )
        {
            SHEET_JOB& sj = sheetJobs[i];

            SCH_SHEET*  sheet;
            SCH_SCREEN* screen;

            if( i == 0 )
            {
                // Reuse sheet the loader created for first page
                sheet = aRootSheet;
                screen = rootScreen;
            }
            else
            {
                screen = new SCH_SCREEN( m_schematic );
                const_cast<KIID&>( screen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );
                sheet = new SCH_SHEET( m_schematic );
                sheet->SetScreen( screen );
                sheet->SyncUuidToScreen();
            }

            wxString base = sj.name;

            for( int suffix = 2; !m_usedSheetNames.insert( sj.name.Lower() ).second; ++suffix )
                sj.name = wxString::Format( wxS( "%s (%d)" ), base, suffix );

            wxString fileName = MakePageFileName( static_cast<int>( i + 1 ), sj.job.page->name );

            sheet->GetField( FIELD_T::SHEET_NAME )->SetText( sj.name );
            sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
            screen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );

            topSheets.push_back( sheet );
            topScreens.push_back( screen );
        }

        m_schematic->SetTopLevelSheets( topSheets );

        for( size_t i = 0; i < sheetJobs.size(); ++i )
        {
            SHEET_JOB&     sj = sheetJobs[i];
            SCH_SHEET_PATH pagePath;

            for( const SCH_SHEET_PATH& candidate : m_schematic->Hierarchy() )
            {
                if( candidate.Last() == topSheets[i] )
                {
                    pagePath = candidate;
                    break;
                }
            }

            if( pagePath.empty() )
                pagePath.push_back( topSheets[i] );

            pagePath.SetPageNumber( wxString::Format( wxS( "%zu" ), i + 1 ) );
            pollProgress( m_progressReporter, sj.job.page->name );
            m_currentOccRefs = sj.job.refs;
            m_currentOccUnitRefs = sj.job.unitRefs;
            m_currentOccProps = sj.job.props;
            m_currentOccNetNames = sj.job.netNames;
            selectOccurrenceAliases( m_currentOccNetNames );
            m_currentFlatNetSuffix = sj.job.flatNetSuffix;
            m_scopeNamedFlatNets = sj.job.scopeNamedFlatNets;
            m_scopeGeneratedFlatNets = sj.job.scopeGeneratedFlatNets;
            m_currentUnconnectedInterfaceNetNames = sj.job.unconnectedInterfaceNetNames;
            m_currentInterfaceNetAliases = interfaceAliasesByScope[sj.job.netNames];
            m_currentConnectorInterfaceNetAliases = connectorAliasesByScope[sj.job.netNames];
            applyPageSettings( *sj.job.page, topScreens[i] );
            convertPage( *sj.job.page, topScreens[i], pagePath );
            m_currentOccRefs = nullptr;
            m_currentOccUnitRefs = nullptr;
            m_currentOccProps = nullptr;
            m_currentOccNetNames = nullptr;
            m_currentFlatNetSuffix.clear();
            m_scopeNamedFlatNets = false;
            m_scopeGeneratedFlatNets = false;
            m_currentUnconnectedInterfaceNetNames.clear();
            m_currentInterfaceNetAliases.clear();
            m_currentConnectorInterfaceNetAliases.clear();
        }
    }

    promoteUniqueOccurrenceNetNames();
    convertUnreferencedPages();
    return aRootSheet;
}


void ORCAD_CONVERTER::convertUnreferencedPages()
{
    if( m_design.unreferencedFolderPages.empty() )
        return;

    std::vector<SCH_SHEET*> topSheets = m_schematic->GetTopLevelSheets();

    for( SCH_SHEET* sheet : topSheets )
        m_usedSheetNames.insert( sheet->GetName().Lower() );

    std::set<wxString> usedFileNames;

    for( const SCH_SHEET_PATH& path : m_schematic->Hierarchy() )
    {
        if( path.LastScreen() )
            usedFileNames.insert( wxFileName( path.LastScreen()->GetFileName() ).GetFullName().Lower() );
    }

    size_t fileIndex = topSheets.size() + 1;

    struct PAGE_SHEET
    {
        ORCAD_RAW_PAGE* page;
        SCH_SHEET*      sheet;
        SCH_SCREEN*     screen;
        size_t          pageNumber;
    };

    std::vector<PAGE_SHEET> pageSheets;

    for( auto& [folder, pages] : m_design.unreferencedFolderPages )
    {
        for( ORCAD_RAW_PAGE& page : pages )
        {
            SCH_SCREEN* screen = new SCH_SCREEN( m_schematic );
            const_cast<KIID&>( screen->GetUuid() ) = deterministicUuid( "screen", m_screenOrdinal++ );
            SCH_SHEET* sheet = new SCH_SHEET( m_schematic );
            sheet->SetScreen( screen );
            const_cast<KIID&>( sheet->m_Uuid ) = screen->GetUuid();
            sheet->SetExcludedFromBoard( true );

            wxString name = FromOrcadString( page.name );
            wxString base = name;

            for( int suffix = 2; !m_usedSheetNames.insert( name.Lower() ).second; ++suffix )
                name = wxString::Format( wxS( "%s (%d)" ), base, suffix );

            size_t   pageNumber = topSheets.size() + 1;
            wxString fileName;

            do
            {
                fileName = MakePageFileName( static_cast<int>( fileIndex++ ), page.name );
            } while( !usedFileNames.insert( fileName.Lower() ).second );

            sheet->GetField( FIELD_T::SHEET_NAME )->SetText( name );
            sheet->GetField( FIELD_T::SHEET_FILENAME )->SetText( fileName );
            screen->SetFileName( m_schematic->Project().GetProjectPath() + fileName );
            topSheets.push_back( sheet );
            pageSheets.push_back( { &page, sheet, screen, pageNumber } );
        }
    }

    m_schematic->SetTopLevelSheets( topSheets );
    m_currentOccRefs = nullptr;
    m_currentOccUnitRefs = nullptr;
    m_currentOccProps = nullptr;
    m_currentOccNetNames = nullptr;
    m_currentFlatNetSuffix.clear();
    m_scopeNamedFlatNets = false;
    m_scopeGeneratedFlatNets = false;
    m_currentUnconnectedInterfaceNetNames.clear();
    m_currentInterfaceNetAliases.clear();
    m_currentOccurrenceNetAliases.clear();
    m_currentConnectorInterfaceNetAliases.clear();

    for( PAGE_SHEET& pageSheet : pageSheets )
    {
        SCH_SHEET_PATH path;

        for( const SCH_SHEET_PATH& candidate : m_schematic->Hierarchy() )
        {
            if( candidate.Last() == pageSheet.sheet )
            {
                path = candidate;
                break;
            }
        }

        if( path.empty() )
            path.push_back( pageSheet.sheet );

        path.SetPageNumber( wxString::Format( wxS( "%zu" ), pageSheet.pageNumber ) );
        pollProgress( m_progressReporter, pageSheet.page->name );
        applyPageSettings( *pageSheet.page, pageSheet.screen );
        convertPage( *pageSheet.page, pageSheet.screen, path );

        // Top-level sheet attributes are not serialized in schematic files.
        for( SCH_ITEM* item : pageSheet.screen->Items().OfType( SCH_SYMBOL_T ) )
            static_cast<SCH_SYMBOL*>( item )->SetExcludedFromBoard( true );
    }
}


void ORCAD_CONVERTER::promoteUniqueOccurrenceNetNames()
{
    struct NET_CANDIDATE
    {
        std::string                       name;
        CONNECTION_SUBGRAPH*              subgraph;
        std::vector<CONNECTION_SUBGRAPH*> subgraphs;
        std::set<std::string>             occurrenceSuffixes;
        std::string                       sourceName;
        std::string                       sourceKey;
        uint32_t                          occurrenceId;
        bool                              hasCommonSuffix;
        bool                              sourceBusMember;
        bool                              needsPromotion;
    };

    SCH_SHEET_LIST sheets = m_schematic->BuildSheetListSortedByPageNumbers();
    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    std::vector<NET_CANDIDATE>                  candidates;
    std::map<std::string, size_t>               nameCounts;
    std::set<std::string>                       claimedBaseNames;
    std::set<std::string>                       rootOccurrenceNames;
    std::map<SCH_SCREEN*, size_t>               screenUseCounts;
    std::set<std::tuple<std::string, int, int>> sourceBusRanges;

    for( const SCH_SHEET_PATH& sheet : sheets )
        ++screenUseCounts[sheet.LastScreen()];

    for( const auto& [occurrenceId, occurrenceName] : m_design.occurrenceRoot.netNames )
    {
        std::string key = kicadOccurrenceNetName( occurrenceName );
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        rootOccurrenceNames.insert( std::move( key ) );
    }

    for( const auto& [screenId, busNames] : m_hierBusNamesByScreen )
    {
        for( const auto& [sourceBus, scopedBus] : busNames )
        {
            std::string prefix;
            int         first;
            int         last;

            if( parseVectorBusName( sourceBus, prefix, first, last ) )
                sourceBusRanges.emplace( std::move( prefix ), first, last );
        }
    }

    for( const auto& [netKey, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph || subgraph->GetNetName().StartsWith( wxS( "/" ) ) )
                continue;

            std::string key = subgraph->GetNetName().ToStdString();
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            claimedBaseNames.insert( key );
        }
    }

    for( const auto& [netKey, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        CONNECTION_SUBGRAPH* selected = nullptr;
        wxString             netName;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( subgraph && subgraph->GetNetName().StartsWith( wxS( "/" ) ) )
            {
                selected = subgraph;
                netName = subgraph->GetNetName();
                break;
            }
        }

        if( !selected )
            continue;

        std::string name = netName.AfterLast( '/' ).ToStdString();
        std::string sourceName = name;
        std::string screenId = selected->GetSheet().LastScreen()->GetUuid().AsStdString();
        auto        busNames = m_hierBusNamesByScreen.find( screenId );
        bool        busMemberRenamed = false;
        bool        sourceBusMember = false;

        for( const auto& [prefix, first, last] : sourceBusRanges )
        {
            if( name.compare( 0, prefix.size(), prefix ) != 0 )
                continue;

            int              member;
            std::string_view memberText( name.data() + prefix.size(), name.size() - prefix.size() );
            auto [next, error] = std::from_chars( memberText.data(), memberText.data() + memberText.size(), member );

            if( !memberText.empty() && error == std::errc() && next == memberText.data() + memberText.size()
                && member >= std::min( first, last ) && member <= std::max( first, last ) )
            {
                sourceBusMember = true;
                break;
            }
        }

        if( name.find( "_ORCAD_" ) != std::string::npos && busNames != m_hierBusNamesByScreen.end() )
        {
            name = unscopedHierBusMember( name, busNames->second );
            busMemberRenamed = name != sourceName;
        }

        std::string key = name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        std::string baseKey = key;

        std::set<std::string> occurrenceSuffixes;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph )
                continue;

            std::string subgraphScreenId = subgraph->GetSheet().LastScreen()->GetUuid().AsStdString();
            auto        suffix = m_occurrenceSuffixByScreen.find( subgraphScreenId );

            if( suffix != m_occurrenceSuffixByScreen.end() && !suffix->second.empty() )
                occurrenceSuffixes.insert( suffix->second );

            std::string subgraphName = subgraph->GetNetName().AfterLast( '/' ).ToStdString();
            auto        subgraphBusNames = m_hierBusNamesByScreen.find( subgraphScreenId );

            if( subgraphBusNames != m_hierBusNamesByScreen.end() )
            {
                for( const auto& [sourceBus, scopedBus] : subgraphBusNames->second )
                {
                    std::string prefix;
                    int         first;
                    int         last;

                    if( !parseVectorBusName( sourceBus, prefix, first, last )
                        || name.compare( 0, prefix.size(), prefix ) != 0 )
                    {
                        continue;
                    }

                    int              member;
                    std::string_view memberText( name.data() + prefix.size(), name.size() - prefix.size() );
                    auto [next, error] =
                            std::from_chars( memberText.data(), memberText.data() + memberText.size(), member );

                    if( !memberText.empty() && error == std::errc() && next == memberText.data() + memberText.size()
                        && member >= std::min( first, last ) && member <= std::max( first, last ) )
                    {
                        sourceBusMember = true;
                    }
                }
            }

            if( subgraphName.find( "_ORCAD_" ) != std::string::npos
                && subgraphBusNames != m_hierBusNamesByScreen.end() )
            {
                std::string unscoped = unscopedHierBusMember( subgraphName, subgraphBusNames->second );

                if( wxString::FromUTF8( unscoped ).CmpNoCase( wxString::FromUTF8( name ) ) == 0 )
                    busMemberRenamed = true;
            }
        }

        std::vector<std::string> commonSuffixParts;

        if( !occurrenceSuffixes.empty() )
        {
            auto splitSuffix = []( const std::string& aSuffix )
            {
                std::vector<std::string> parts;
                size_t                   start = 0;

                for( size_t separator = aSuffix.find( '_' );; separator = aSuffix.find( '_', start ) )
                {
                    if( separator == std::string::npos )
                    {
                        parts.push_back( aSuffix.substr( start ) );
                        break;
                    }

                    parts.push_back( aSuffix.substr( start, separator - start ) );
                    start = separator + 1;
                }

                return parts;
            };

            commonSuffixParts = splitSuffix( *occurrenceSuffixes.begin() );

            for( auto suffix = std::next( occurrenceSuffixes.begin() ); suffix != occurrenceSuffixes.end(); ++suffix )
            {
                std::vector<std::string> parts = splitSuffix( *suffix );
                size_t                   common = 0;

                while( common < commonSuffixParts.size() && common < parts.size()
                       && commonSuffixParts[common] == parts[common] )
                {
                    ++common;
                }

                commonSuffixParts.resize( common );
            }
        }

        std::string commonSuffix;

        for( const std::string& part : commonSuffixParts )
        {
            if( !commonSuffix.empty() )
                commonSuffix += '_';

            commonSuffix += part;
        }

        size_t occurrenceScopeCount = m_occurrenceNetNameScopeCounts[baseKey];
        bool   canonicalRoot = false;

        if( occurrenceScopeCount > 1 )
        {
            bool scopedBusOccurrence = false;

            for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
            {
                if( !subgraph )
                    continue;

                for( SCH_ITEM* item : subgraph->GetItems() )
                {
                    if( auto label = dynamic_cast<SCH_LABEL_BASE*>( item ) )
                    {
                        if( label->GetText().Upper().Contains( wxS( "_ORCAD_" ) ) )
                        {
                            scopedBusOccurrence = true;
                            break;
                        }
                    }
                }

                if( scopedBusOccurrence )
                    break;
            }

            canonicalRoot = rootOccurrenceNames.count( baseKey ) && !claimedBaseNames.count( baseKey )
                            && !scopedBusOccurrence && commonSuffix.empty();

            if( !canonicalRoot && !busMemberRenamed && !scopedBusOccurrence )
            {
                if( !commonSuffix.empty() )
                {
                    name += "_" + commonSuffix;
                    key = name;
                    std::transform( key.begin(), key.end(), key.begin(),
                                    []( unsigned char c )
                                    {
                                        return static_cast<char>( std::tolower( c ) );
                                    } );
                }
            }
        }

        uint32_t occurrenceId = std::numeric_limits<uint32_t>::max();

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph )
                continue;

            std::string subgraphScreenId = subgraph->GetSheet().LastScreen()->GetUuid().AsStdString();
            auto        screenIds = m_occurrenceNetIdsByScreen.find( subgraphScreenId );

            if( screenIds == m_occurrenceNetIdsByScreen.end() )
                continue;

            auto id = screenIds->second.find( key );

            if( id == screenIds->second.end() )
                id = screenIds->second.find( baseKey );

            if( id != screenIds->second.end() )
                occurrenceId = std::min( occurrenceId, id->second );
        }

        bool spellingChanged = wxString::FromUTF8( sourceName ).CmpNoCase( wxString::FromUTF8( name ) ) != 0;
        bool needsPromotion = spellingChanged || occurrenceScopeCount == 1;

        if( canonicalRoot )
            needsPromotion = false;

        candidates.push_back( { std::move( name ), selected, subgraphs, std::move( occurrenceSuffixes ), sourceName,
                                baseKey, occurrenceId, !commonSuffix.empty(), sourceBusMember, needsPromotion } );
        ++nameCounts[key];
    }

    auto terminalBearing = []( const NET_CANDIDATE& aCandidate )
    {
        return std::any_of( aCandidate.subgraphs.begin(), aCandidate.subgraphs.end(),
                            []( const CONNECTION_SUBGRAPH* aSubgraph )
                            {
                                return aSubgraph
                                       && std::any_of( aSubgraph->GetItems().begin(), aSubgraph->GetItems().end(),
                                                       []( const SCH_ITEM* aItem )
                                                       {
                                                           return aItem->Type() == SCH_PIN_T;
                                                       } );
                            } );
    };

    std::map<std::string, std::vector<size_t>> peerGroups;
    std::map<wxString, wxString>               canonicalRenames;

    for( size_t i = 0; i < candidates.size(); ++i )
        peerGroups[candidates[i].sourceKey].push_back( i );

    for( const auto& peerGroup : peerGroups )
    {
        const std::vector<size_t>& indices = peerGroup.second;

        if( indices.size() != 1 || !terminalBearing( candidates[indices.front()] ) )
            continue;

        NET_CANDIDATE& candidate = candidates[indices.front()];

        if( candidate.hasCommonSuffix && !candidate.sourceBusMember
            && !wxString::FromUTF8( candidate.sourceName ).Upper().Contains( wxS( "_ORCAD" ) ) )
        {
            candidate.name = candidate.sourceName;
            candidate.needsPromotion = true;
        }
    }

    for( const auto& [key, indices] : peerGroups )
    {
        if( !std::any_of( indices.begin(), indices.end(),
                          [&]( size_t aIndex )
                          {
                              return candidates[aIndex].sourceBusMember;
                          } ) )
        {
            continue;
        }

        std::vector<size_t> realIndices;

        for( size_t index : indices )
        {
            if( candidates[index].hasCommonSuffix && terminalBearing( candidates[index] )
                && !wxString::FromUTF8( candidates[index].sourceName ).Upper().Contains( wxS( "_ORCAD" ) ) )
            {
                realIndices.push_back( index );
            }
        }

        if( realIndices.empty() )
            continue;


        auto canonical = std::min_element( realIndices.begin(), realIndices.end(),
                                           [&]( size_t aLeft, size_t aRight )
                                           {
                                               if( candidates[aLeft].occurrenceId == candidates[aRight].occurrenceId )
                                                   return candidates[aLeft].name < candidates[aRight].name;

                                               return candidates[aLeft].occurrenceId < candidates[aRight].occurrenceId;
                                           } );

        if( canonical == realIndices.end() )
            continue;

        NET_CANDIDATE& owner = candidates[*canonical];
        wxString       oldName = wxString::FromUTF8( owner.name );
        canonicalRenames[oldName.Lower()] = wxString::FromUTF8( owner.sourceName );

        for( CONNECTION_SUBGRAPH* subgraph : owner.subgraphs )
        {
            if( !subgraph )
                continue;

            for( SCH_ITEM* item : subgraph->GetSheet().LastScreen()->Items() )
            {
                auto* label = dynamic_cast<SCH_GLOBALLABEL*>( item );

                if( label && label->GetText().CmpNoCase( oldName ) == 0 )
                    label->SetText( wxString::FromUTF8( owner.sourceName ) );
            }
        }

        owner.name = owner.sourceName;
        owner.needsPromotion = false;
    }

    std::map<std::string, std::vector<size_t>> candidateGroups;

    for( size_t i = 0; i < candidates.size(); ++i )
    {
        std::string key = candidates[i].name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        candidateGroups[key].push_back( i );
    }

    for( const auto& [key, indices] : candidateGroups )
    {
        for( size_t index : indices )
        {
            if( !terminalBearing( candidates[index] ) )
                candidates[index].name.clear();
        }
    }

    nameCounts.clear();

    for( const NET_CANDIDATE& candidate : candidates )
    {
        if( candidate.name.empty() )
            continue;

        std::string key = candidate.name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        ++nameCounts[key];
    }

    for( NET_CANDIDATE& candidate : candidates )
    {
        if( candidate.name.empty() )
            continue;

        std::string key = candidate.name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( nameCounts[key] <= 1 )
            continue;

        auto suffix =
                m_occurrenceSuffixByScreen.find( candidate.subgraph->GetSheet().LastScreen()->GetUuid().AsStdString() );

        if( suffix != m_occurrenceSuffixByScreen.end() && !suffix->second.empty() )
        {
            candidate.name += "_" + suffix->second;
            candidate.needsPromotion = true;
        }
        else if( m_occurrenceNetNameScopeCounts[key] > 1 && candidate.occurrenceSuffixes.size() == 1 )
        {
            candidate.name += "_" + *candidate.occurrenceSuffixes.begin();
            candidate.needsPromotion = true;
        }
    }

    std::map<std::string, size_t> finalNameCounts;

    for( const NET_CANDIDATE& candidate : candidates )
    {
        if( candidate.name.empty() )
            continue;

        if( !candidate.needsPromotion )
            continue;

        std::string key = candidate.name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        ++finalNameCounts[key];
    }

    for( const NET_CANDIDATE& candidate : candidates )
    {
        if( candidate.name.empty() )
            continue;

        std::string key = candidate.name;
        std::transform( key.begin(), key.end(), key.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( finalNameCounts[key] != 1 )
            continue;

        bool sharedScreen =
                std::any_of( candidate.subgraphs.begin(), candidate.subgraphs.end(),
                             [&]( const CONNECTION_SUBGRAPH* aSubgraph )
                             {
                                 return aSubgraph && screenUseCounts[aSubgraph->GetSheet().LastScreen()] != 1;
                             } );

        if( sharedScreen )
            continue;

        if( !terminalBearing( candidate ) )
            continue;

        size_t ordinal = 0;

        for( CONNECTION_SUBGRAPH* subgraph : candidate.subgraphs )
        {
            if( !subgraph )
                continue;

            SCH_ITEM* anchor = nullptr;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() == SCH_PIN_T )
                {
                    anchor = item;
                    break;
                }
            }

            if( !anchor )
                continue;

            SCH_GLOBALLABEL* label = new SCH_GLOBALLABEL( anchor->GetPosition(), FromOrcadString( candidate.name ) );
            const_cast<KIID&>( label->m_Uuid ) = deterministicUuid( "occurrence-net:" + key, ordinal++ );
            label->SetTextColor( invisibleElectricalTextColor() );
            subgraph->GetSheet().LastScreen()->Append( label );
        }
    }

    std::set<SCH_SCREEN*> renamedScreens;

    for( const SCH_SHEET_PATH& sheet : sheets )
    {
        SCH_SCREEN* screen = sheet.LastScreen();

        if( !renamedScreens.insert( screen ).second )
            continue;

        for( SCH_ITEM* item : screen->Items() )
        {
            auto* label = dynamic_cast<SCH_GLOBALLABEL*>( item );

            if( !label || label->GetTextColor().a != 0.0 )
                continue;

            auto rename = canonicalRenames.find( label->GetText().Lower() );

            if( rename != canonicalRenames.end() )
                label->SetText( rename->second );
        }
    }

    m_schematic->ConnectionGraph()->Recalculate( sheets, true );

    for( const auto& [netKey, subgraphs] : m_schematic->ConnectionGraph()->GetNetMap() )
    {
        if( netKey.Name.StartsWith( wxS( "/" ) ) )
            continue;

        std::map<std::string, wxString> occurrenceNames;

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                auto* label = dynamic_cast<SCH_GLOBALLABEL*>( item );

                if( !label || label->GetTextColor().a != 0.0 )
                    continue;

                std::string key = label->GetText().Lower().ToStdString();

                if( rootOccurrenceNames.count( key ) )
                    occurrenceNames[key] = label->GetText();
            }
        }

        if( occurrenceNames.size() < 2 )
            continue;

        wxString authoritativeName;

        for( const auto& [candidateKey, candidateName] : occurrenceNames )
        {
            bool commonBase = std::all_of(
                    occurrenceNames.begin(), occurrenceNames.end(),
                    [&]( const auto& aPeer )
                    {
                        return aPeer.first == candidateKey
                               || ( aPeer.first.size() > candidateKey.size() && aPeer.first[candidateKey.size()] == '_'
                                    && aPeer.first.compare( 0, candidateKey.size(), candidateKey ) == 0 );
                    } );

            if( commonBase )
            {
                authoritativeName = candidateName;
                break;
            }
        }

        if( authoritativeName.IsEmpty() )
        {
            auto authoritative = std::max_element( occurrenceNames.begin(), occurrenceNames.end(),
                                                   [&]( const auto& aLeft, const auto& aRight )
                                                   {
                                                       return m_occurrenceNetNameScopeCounts[aLeft.first]
                                                              < m_occurrenceNetNameScopeCounts[aRight.first];
                                                   } );

            size_t authoritativeScopeCount = m_occurrenceNetNameScopeCounts[authoritative->first];
            size_t equallyAuthoritative =
                    std::count_if( occurrenceNames.begin(), occurrenceNames.end(),
                                   [&]( const auto& aEntry )
                                   {
                                       return m_occurrenceNetNameScopeCounts[aEntry.first] == authoritativeScopeCount;
                                   } );

            if( equallyAuthoritative != 1 )
                continue;

            authoritativeName = authoritative->second;
        }

        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            if( !subgraph )
                continue;

            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                auto* label = dynamic_cast<SCH_GLOBALLABEL*>( item );

                if( label && label->GetTextColor().a == 0.0
                    && label->GetText().CmpNoCase( authoritativeName ) != 0 )
                {
                    label->SetText( authoritativeName );
                }
            }
        }
    }
}


void ORCAD_CONVERTER::convertPage( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aSheetPath,
                                   bool aContainerPage, bool aSharedFolderPage )
{
    m_pageItemScreen = aScreen;
    m_pageItems.clear();

    std::string screenId = aScreen->GetUuid().AsStdString();
    m_occurrenceSuffixByScreen[screenId] = m_currentFlatNetSuffix;

    if( m_currentOccNetNames )
    {
        auto& ids = m_occurrenceNetIdsByScreen[screenId];

        for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
        {
            std::string key = kicadOccurrenceNetName( occurrenceName );
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            auto [entry, inserted] = ids.emplace( std::move( key ), occurrenceId );

            if( !inserted )
                entry->second = std::min( entry->second, occurrenceId );
        }
    }

    placePageFrame( aPage, aScreen );
    applyTitleBlock( aPage, aScreen );
    buildNetLookup( aPage );

    placeJunctions( aPage, aScreen );
    placeBusEntries( aPage, aScreen );
    bool hierarchical = aContainerPage || !aSheetPath.Last()->IsTopLevelSheet();
    bool sharedFolderPage = aSharedFolderPage
                            || std::any_of(
                                    m_design.childFolderPages.begin(), m_design.childFolderPages.end(),
                                    [&]( const auto& aFolder )
                                    {
                                        return aFolder.second.size() > 1
                                               && std::any_of( aFolder.second.begin(), aFolder.second.end(),
                                                               [&]( const ORCAD_RAW_PAGE& aCandidate )
                                                               {
                                                                   return &aCandidate == &aPage;
                                                               } );
                                    } );
    wxString syntheticPageSuffix = aSheetPath.Last()->GetName().Mid( 5 );
    syntheticPageSuffix.Trim( true ).Trim( false );
    bool syntheticPage = aSheetPath.Last()->GetName().StartsWith( wxS( "Sheet" ) )
                         && !syntheticPageSuffix.empty()
                         && std::all_of( syntheticPageSuffix.begin(), syntheticPageSuffix.end(),
                                         []( wxUniChar c )
                                         {
                                             return wxIsdigit( c );
                                         } );
    sharedFolderPage |= syntheticPage;
    bool semanticNestedHierarchy = aSheetPath.size() > 2 && !sharedFolderPage
                                   && !aSheetPath.Last()->GetPins().empty();
    placeWires( aPage, aScreen, hierarchical, semanticNestedHierarchy, sharedFolderPage );
    placeOffpageConnectors( aPage, aScreen, aSheetPath, hierarchical );
    placePorts( aPage, aScreen, !aSheetPath.Last()->IsTopLevelSheet() );
    placeGraphics( aPage, aScreen );

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
        placeInstance( aPage, instance, aScreen, aSheetPath );

    for( const ORCAD_GRAPHIC_INST& global : aPage.globals )
        placePowerSymbol( aPage, global, powerNet( aPage, global ), aScreen, aSheetPath );

    assignPageItemUuids( m_pageOrdinal++ );
    m_pageItemScreen = nullptr;
    m_pageItems.clear();
}


void ORCAD_CONVERTER::placePageFrame( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    if( !aPage.borderPrinted && !aPage.gridRefPrinted )
        return;

    int widthDbu = KiROUND( aPage.isMetric ? aPage.width / 254.0 : aPage.width / 10.0 );
    int heightDbu = KiROUND( aPage.isMetric ? aPage.height / 254.0 : aPage.height / 10.0 );

    if( widthDbu <= 0 || heightDbu <= 0 )
        return;

    ORCAD_PRIMITIVE border;
    border.kind = ORCAD_PRIM_KIND::RECT;
    border.lineWidth = 0;
    border.lineStyle = 0;
    const KIGFX::COLOR4D color( 0.0, 0.0, 0.0, 1.0 );

    auto addLine = [&]( std::initializer_list<ORCAD_POINT> aPoints )
    {
        std::vector<VECTOR2I> points;

        for( const ORCAD_POINT& point : aPoints )
            points.push_back( OrcadDbuToIu( point.x, point.y ) );

        appendPageItem( aScreen, makeSheetPoly( points, border, color ) );
    };

    addLine( { { 0, 0 }, { widthDbu, 0 }, { widthDbu, heightDbu }, { 0, heightDbu }, { 0, 0 } } );

    if( !aPage.gridRefPrinted )
        return;

    int horizontalBand = KiROUND( aPage.isMetric ? aPage.horizontalWidth / 254.0
                                                  : aPage.horizontalWidth / 10.0 );
    int verticalBand = KiROUND( aPage.isMetric ? aPage.verticalWidth / 254.0 : aPage.verticalWidth / 10.0 );
    horizontalBand = std::clamp( horizontalBand, 0, heightDbu / 2 );
    verticalBand = std::clamp( verticalBand, 0, widthDbu / 2 );

    if( horizontalBand > 0 && verticalBand > 0 )
    {
        addLine( { { verticalBand, horizontalBand }, { widthDbu - verticalBand, horizontalBand },
                   { widthDbu - verticalBand, heightDbu - horizontalBand },
                   { verticalBand, heightDbu - horizontalBand }, { verticalBand, horizontalBand } } );
    }

    auto addLabel = [&]( const wxString& aContent, int aX, int aY )
    {
        SCH_TEXT* label = new SCH_TEXT( OrcadDbuToIu( aX, aY ), aContent, LAYER_NOTES );
        int       size = OrcadDbuToIu( 5, 5 ).x;
        label->SetTextSize( VECTOR2I( size, size ) );
        label->SetFont( KIFONT::FONT::GetFont( wxS( "Arial" ), false, false ) );
        label->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        label->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
        label->SetTextColor( color );
        appendPageItem( aScreen, label );
    };

    for( int i = 0; i < aPage.horizontalCount; ++i )
    {
        int left = KiROUND( static_cast<double>( widthDbu ) * i / aPage.horizontalCount );
        int right = KiROUND( static_cast<double>( widthDbu ) * ( i + 1 ) / aPage.horizontalCount );
        int value = aPage.horizontalAscending ? i : aPage.horizontalCount - i - 1;
        wxString label = aPage.horizontalChar ? wxString( static_cast<wxUniChar>( 'A' + value ) )
                                              : wxString::Format( wxS( "%d" ), value + 1 );

        if( i > 0 )
        {
            addLine( { { left, 0 }, { left, horizontalBand } } );
            addLine( { { left, heightDbu - horizontalBand }, { left, heightDbu } } );
        }

        addLabel( label, ( left + right ) / 2, horizontalBand / 2 );
        addLabel( label, ( left + right ) / 2, heightDbu - horizontalBand / 2 );
    }

    for( int i = 0; i < aPage.verticalCount; ++i )
    {
        int top = KiROUND( static_cast<double>( heightDbu ) * i / aPage.verticalCount );
        int bottom = KiROUND( static_cast<double>( heightDbu ) * ( i + 1 ) / aPage.verticalCount );
        int value = aPage.verticalAscending ? i : aPage.verticalCount - i - 1;
        wxString label = aPage.verticalChar ? wxString( static_cast<wxUniChar>( 'A' + value ) )
                                            : wxString::Format( wxS( "%d" ), value + 1 );

        if( i > 0 )
        {
            addLine( { { 0, top }, { verticalBand, top } } );
            addLine( { { widthDbu - verticalBand, top }, { widthDbu, top } } );
        }

        addLabel( label, verticalBand / 2, ( top + bottom ) / 2 );
        addLabel( label, widthDbu - verticalBand / 2, ( top + bottom ) / 2 );
    }
}


wxString ORCAD_CONVERTER::MakePageFileName( int aPageIndex, const std::string& aPageName )
{
    wxString fileName =
            wxString::Format( wxS( "P%02d_" ), aPageIndex ) + SanitizeFileName( aPageName ) + wxS( ".kicad_sch" );

    ReplaceIllegalFileNameChars( fileName, '_' );

    return fileName;
}


wxString ORCAD_CONVERTER::SanitizeFileName( const std::string& aName )
{
    const wxString illegal( wxS( "<>:\"/\\|?*" ) );
    wxString       in = FromOrcadString( aName );
    wxString       out;

    for( wxUniChar c : in )
    {
        if( c.GetValue() < 0x20 || illegal.Find( c ) != wxNOT_FOUND )
            out += '_';
        else
            out += c;
    }

    while( !out.IsEmpty() && ( out.Last() == ' ' || out.Last() == '.' ) )
        out.RemoveLast();

    while( !out.IsEmpty() && ( out.GetChar( 0 ) == ' ' || out.GetChar( 0 ) == '.' ) )
        out.Remove( 0, 1 );

    if( out.IsEmpty() )
        out = wxS( "unnamed" );

    return out;
}


void ORCAD_CONVERTER::applyPageSettings( ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    if( aPage.width > 0 && aPage.height > 0 )
    {
        double widthMils = aPage.isMetric ? aPage.width / 25.4 : aPage.width;
        double heightMils = aPage.isMetric ? aPage.height / 25.4 : aPage.height;

        PAGE_INFO::SetCustomWidthMils( widthMils );
        PAGE_INFO::SetCustomHeightMils( heightMils );

        PAGE_INFO pageInfo;
        pageInfo.SetType( PAGE_SIZE_TYPE::User );
        aScreen->SetPageSettings( pageInfo );
        return;
    }

    BOX2I extent = pageExtentDbu( aPage );

    // Nominal paper from stored page size; mils, or micrometres when metric.
    double k = aPage.isMetric ? 0.001 : 0.0254;
    double nominalWmm = aPage.width * k;
    double nominalHmm = aPage.height * k;

    // The clearance shift below pushes a full-page drawing past its own paper, so a
    // named size only survives while the content stays at source coordinates
    PAGE_INFO named;

    if( !aPage.pageSize.empty() && named.SetType( FromOrcadString( aPage.pageSize ), nominalHmm > nominalWmm )
        && named.GetType() != PAGE_SIZE_TYPE::User && extent.GetLeft() >= 0 && extent.GetTop() >= 0
        && extent.GetRight() * DBU_TO_MM <= named.GetWidthMM()
        && extent.GetBottom() * DBU_TO_MM <= named.GetHeightMM() )
    {
        aScreen->SetPageSettings( named );
        return;
    }

    // Shift content clear of frame, round up to 10-DBU grid to keep points on grid.
    int dx = std::max( 0, MARGIN_L_DBU - extent.GetLeft() );
    int dy = std::max( 0, MARGIN_T_DBU - extent.GetTop() );

    dx = ( dx + 9 ) / 10 * 10;
    dy = ( dy + 9 ) / 10 * 10;

    int maxX = extent.GetRight();
    int maxY = extent.GetBottom();

    if( dx || dy )
    {
        offsetPage( aPage, dx, dy );
        maxX += dx;
        maxY += dy;
    }

    // Needed paper = shifted content extent plus right/bottom margins.
    double neededWmm = ( maxX + MARGIN_R_DBU ) * DBU_TO_MM;
    double neededHmm = ( maxY + MARGIN_B_DBU ) * DBU_TO_MM;

    int paperWmm = static_cast<int>( std::ceil( std::max( nominalWmm, neededWmm ) ) );
    int paperHmm = static_cast<int>( std::ceil( std::max( nominalHmm, neededHmm ) ) );

    PAGE_INFO pageInfo;
    PAGE_INFO::SetCustomWidthMils( paperWmm * 1000.0 / 25.4 );
    PAGE_INFO::SetCustomHeightMils( paperHmm * 1000.0 / 25.4 );
    pageInfo.SetType( PAGE_SIZE_TYPE::User );

    aScreen->SetPageSettings( pageInfo );
}


BOX2I ORCAD_CONVERTER::pageExtentDbu( const ORCAD_RAW_PAGE& aPage )
{
    bool any = false;
    int  minX = 0;
    int  minY = 0;
    int  maxX = 0;
    int  maxY = 0;

    auto add = [&]( int aX, int aY )
    {
        if( !any )
        {
            minX = maxX = aX;
            minY = maxY = aY;
            any = true;
        }
        else
        {
            minX = std::min( minX, aX );
            minY = std::min( minY, aY );
            maxX = std::max( maxX, aX );
            maxY = std::max( maxY, aY );
        }
    };

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        add( wire.x1, wire.y1 );
        add( wire.x2, wire.y2 );
    }

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        add( instance.bbox.x1, instance.bbox.y1 );
        add( instance.bbox.x2, instance.bbox.y2 );

        for( const ORCAD_PIN_INST& pin : instance.pins )
            add( pin.x, pin.y );
    }

    for( const std::vector<ORCAD_GRAPHIC_INST>* list :
         { &aPage.globals, &aPage.offpage, &aPage.ports, &aPage.ercObjects } )
    {
        for( const ORCAD_GRAPHIC_INST& inst : *list )
        {
            add( inst.x, inst.y );
            add( inst.bbox.x1, inst.bbox.y1 );
            add( inst.bbox.x2, inst.bbox.y2 );
        }
    }

    // Free graphics outer bbox = anchor-relative junk; only nested primitives carry real coords.
    for( const ORCAD_GRAPHIC_INST& gfx : aPage.graphics )
    {
        if( !gfx.nested )
            continue;

        for( const ORCAD_PRIMITIVE& prim : gfx.nested->primitives )
        {
            if( !prim.points.empty() )
            {
                for( const ORCAD_POINT& pt : prim.points )
                    add( pt.x, pt.y );
            }
            else
            {
                add( prim.x1, prim.y1 );
                add( prim.x2, prim.y2 );
            }
        }
    }

    for( const ORCAD_BUS_ENTRY& entry : aPage.busEntries )
    {
        add( entry.x1, entry.y1 );
        add( entry.x2, entry.y2 );
    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        add( block.x1, block.y1 );
        add( block.x1 + block.w, block.y1 + block.h );

        for( const ORCAD_BLOCK_PIN& pin : block.pins )
            add( pin.x, pin.y );
    }

    if( !any )
        return BOX2I( VECTOR2I( 0, 0 ), VECTOR2I( 3800, 2700 ) );

    return BOX2I( VECTOR2I( minX, minY ), VECTOR2I( maxX - minX, maxY - minY ) );
}


void ORCAD_CONVERTER::offsetPage( ORCAD_RAW_PAGE& aPage, int aDx, int aDy )
{
    for( ORCAD_WIRE& wire : aPage.wires )
    {
        wire.x1 += aDx;
        wire.y1 += aDy;
        wire.x2 += aDx;
        wire.y2 += aDy;

        for( ORCAD_ALIAS& alias : wire.aliases )
        {
            alias.x += aDx;
            alias.y += aDy;
        }
    }

    for( ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        instance.x += aDx;
        instance.y += aDy;
        instance.bbox.x1 += aDx;
        instance.bbox.y1 += aDy;
        instance.bbox.x2 += aDx;
        instance.bbox.y2 += aDy;

        for( ORCAD_PIN_INST& pin : instance.pins )
        {
            pin.x += aDx;
            pin.y += aDy;
        }
    }

    auto shiftGraphic = [&]( ORCAD_GRAPHIC_INST& aInst )
    {
        aInst.x += aDx;
        aInst.y += aDy;
        aInst.bbox.x1 += aDx;
        aInst.bbox.y1 += aDy;
        aInst.bbox.x2 += aDx;
        aInst.bbox.y2 += aDy;

        if( !aInst.nested )
            return;

        for( ORCAD_PRIMITIVE& prim : aInst.nested->primitives )
        {
            prim.x1 += aDx;
            prim.y1 += aDy;
            prim.x2 += aDx;
            prim.y2 += aDy;

            for( ORCAD_POINT& pt : prim.points )
            {
                pt.x += aDx;
                pt.y += aDy;
            }

            if( prim.start )
            {
                prim.start->x += aDx;
                prim.start->y += aDy;
            }

            if( prim.end )
            {
                prim.end->x += aDx;
                prim.end->y += aDy;
            }
        }
    };

    for( std::vector<ORCAD_GRAPHIC_INST>* list :
         { &aPage.globals, &aPage.offpage, &aPage.ports, &aPage.ercObjects, &aPage.graphics } )
    {
        for( ORCAD_GRAPHIC_INST& inst : *list )
            shiftGraphic( inst );
    }

    for( ORCAD_BUS_ENTRY& entry : aPage.busEntries )
    {
        entry.x1 += aDx;
        entry.y1 += aDy;
        entry.x2 += aDx;
        entry.y2 += aDy;
    }

    for( ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        block.x1 += aDx;
        block.y1 += aDy;

        for( ORCAD_BLOCK_PIN& pin : block.pins )
        {
            pin.x += aDx;
            pin.y += aDy;
        }
    }
}


void ORCAD_CONVERTER::applyTitleBlock( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_GRAPHIC_INST& tbInst : aPage.titleBlocks )
    {
        auto symbolIt = m_design.symbols.find( tbInst.name );
        const ORCAD_SYMBOL_DEF* symbolDef = symbolIt != m_design.symbols.end() ? &symbolIt->second : nullptr;

        if( symbolDef )
        {
            auto normalizedPath = []( std::string aPath )
            {
                std::transform( aPath.begin(), aPath.end(), aPath.begin(),
                                []( unsigned char aChar )
                                {
                                    return aChar == '\\' ? '/' : static_cast<char>( std::tolower( aChar ) );
                                } );
                return aPath;
            };

            bool sourceMatched = false;

            if( auto source = tbInst.props.find( "Source Library" );
                source != tbInst.props.end() && !source->second.empty() )
            {
                std::string sourceKey = normalizedPath( source->second );

                if( normalizedPath( symbolDef->sourceLib ) == sourceKey )
                {
                    sourceMatched = true;
                }
                else
                {
                    for( const ORCAD_SYMBOL_DEF& variant : symbolIt->second.variants )
                    {
                        if( normalizedPath( variant.sourceLib ) == sourceKey )
                        {
                            symbolDef = &variant;
                            sourceMatched = true;
                            break;
                        }
                    }
                }
            }

            bool dimensionsEncoded = tbInst.bbox.x2 < tbInst.bbox.x1 || tbInst.bbox.y2 < tbInst.bbox.y1;
            int  placedWidth = std::abs( dimensionsEncoded ? tbInst.bbox.x2 : tbInst.bbox.x2 - tbInst.bbox.x1 );
            int  placedHeight = std::abs( dimensionsEncoded ? tbInst.bbox.y2 : tbInst.bbox.y2 - tbInst.bbox.y1 );

            auto matchesPlacedBounds = [&]( const ORCAD_SYMBOL_DEF& aDefinition )
            {
                if( !aDefinition.bbox )
                    return false;

                int width = std::abs( aDefinition.bbox->x2 - aDefinition.bbox->x1 );
                int height = std::abs( aDefinition.bbox->y2 - aDefinition.bbox->y1 );

                if( tbInst.rotation & 1 )
                    std::swap( width, height );

                return width == placedWidth && height == placedHeight;
            };

            if( !sourceMatched && !matchesPlacedBounds( *symbolDef ) )
            {
                for( const ORCAD_SYMBOL_DEF& variant : symbolIt->second.variants )
                {
                    if( matchesPlacedBounds( variant ) )
                    {
                        symbolDef = &variant;
                        break;
                    }
                }
            }
        }

        auto calendarDate = [&]( uint32_t aTimestamp )
        {
            return tbInst.name == "TITLEBLK/Rudy" ? orcadShortCalendarDate( aTimestamp )
                                                  : orcadCalendarDate( aTimestamp );
        };

        auto sourceValue = [&]( const std::string& aKey ) -> std::string
        {
            if( aKey == "Page Size" )
                return aPage.pageSize;

            if( aKey == "Page Number" && aPage.sourcePageNumber != 0 )
                return std::to_string( aPage.sourcePageNumber );

            if( aKey == "Page Count" && aPage.sourcePageCount != 0 )
                return std::to_string( aPage.sourcePageCount );

            if( aKey == "SIZE" || aKey == "Page Number" || aKey == "Page Count" )
            {
                if( auto value = tbInst.props.find( aKey ); value != tbInst.props.end() && !value->second.empty() )
                {
                    return value->second;
                }

                if( auto value = aPage.props.find( aKey );
                    value != aPage.props.end() && !value->second.empty() )
                {
                    return value->second;
                }

                if( symbolDef )
                {
                    if( auto value = symbolDef->props.find( aKey );
                        value != symbolDef->props.end() && !value->second.empty() )
                    {
                        return value->second;
                    }
                }
            }

            if( aKey == "SIZE" )
                return aPage.pageSize == "Custom" ? "N/A" : aPage.pageSize;

            if( ( aKey == "Page Modify Date" || aKey == "Schematic Modify Date" )
                && aPage.modifyTimestamp != 0 )
                return calendarDate( aPage.modifyTimestamp );

            if( aKey == "Design Modify Date" && m_design.library.modifyTimestamp != 0 )
                return calendarDate( m_design.library.modifyTimestamp );

            if( ( aKey == "Page Create Date" || aKey == "Schematic Create Date" )
                && aPage.createTimestamp != 0 )
                return calendarDate( aPage.createTimestamp );

            if( aKey == "Design Create Date" && m_design.library.createTimestamp != 0 )
                return calendarDate( m_design.library.createTimestamp );

            if( auto value = tbInst.props.find( aKey ); value != tbInst.props.end() && !value->second.empty() )
                return value->second;

            if( auto value = aPage.props.find( aKey ); value != aPage.props.end() && !value->second.empty() )
                return value->second;

            if( symbolDef )
            {
                if( auto value = symbolDef->props.find( aKey );
                    value != symbolDef->props.end() && !value->second.empty() )
                {
                    return value->second;
                }
            }

            if( aKey == "Page Modify Date" )
                return calendarDate( aPage.modifyTimestamp );

            return {};
        };

        if( symbolDef )
        {
            placeDefinitionVectors( *symbolDef, tbInst.bbox.x1, tbInst.bbox.y1,
                                    OrcadOrientOf( tbInst.rotation, tbInst.mirror ), aScreen, 35.0 / 32.0,
                                    6.0 / 5.0, true );
            placeDefinitionImages( *symbolDef, tbInst.bbox.x1, tbInst.bbox.y1,
                                   OrcadOrientOf( tbInst.rotation, tbInst.mirror ), aScreen );
        }

        for( const ORCAD_DISPLAY_PROP& dp : tbInst.displayProps )
        {
            std::string value = sourceValue( dp.name );

            if( !OrcadDisplayPropVisible( dp ) )
                continue;

            if( value.empty() || value == "?" )
                value = "<" + dp.name + ">";

            wxString content;

            if( OrcadDisplayPropShowsValue( dp ) )
                content = FromOrcadString( value );

            if( OrcadDisplayPropShowsName( dp ) )
            {
                content = FromOrcadString( dp.name );

                if( OrcadDisplayPropShowsValue( dp ) )
                    content += wxS( ": " ) + FromOrcadString( value );
            }

            int      fontId = displayFontId( dp );
            bool     templateFont = displayUsesTemplateFont( dp );
            int      size = textSizeIU( fontId, templateFont );
            int      baseline = textBaselineOffset( size, fontId, templateFont );
            bool     vertical = ( dp.rotation & 1 ) != 0;
            VECTOR2I position = OrcadDbuToIu( tbInst.bbox.x1 + dp.x, tbInst.bbox.y1 + dp.y )
                                + ( vertical ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline ) );
            SCH_TEXT* text = new SCH_TEXT( position, content, LAYER_NOTES );
            text->SetTextAngle( vertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
            text->SetTextSize( textSize( fontId, templateFont ) );
            applyFont( text, fontId, templateFont );
            applyMultilineSpacing( text, fontId, templateFont );
            text->SetTextColor( KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
            text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
            appendPageItem( aScreen, text );
        }

        if( tbInst.props.empty() )
            continue;

        auto get = [&]( const char* aKey ) -> wxString
        {
            std::string value = sourceValue( aKey );
            return value == "?" ? wxString() : FromOrcadString( value );
        };

        TITLE_BLOCK titleBlock;

        titleBlock.SetTitle( get( "Title" ) );

        wxString date = get( "Page Modify Date" );

        if( date.IsEmpty() )
            date = get( "Doc Date" );

        titleBlock.SetDate( date );
        titleBlock.SetRevision( get( "RevCode" ) );
        titleBlock.SetCompany( get( "OrgName" ) );

        int slot = 0;

        for( const char* stock : { "Doc", "OrgAddr1", "OrgAddr2" } )
        {
            if( wxString text = get( stock ); !text.IsEmpty() )
                titleBlock.SetComment( slot++, text );
        }

        // Custom title blocks name their own fields and KiCad has no slot for them, so
        // spill into the free comments.  Page number and count KiCad resolves itself
        static const std::set<std::string> mapped = { "Title",       "RevCode",         "OrgName",  "Doc",
                                                      "OrgAddr1",    "OrgAddr2",        "Doc Date", "Page Count",
                                                      "Page Number", "Page Modify Date" };

        for( const auto& [propName, propValue] : tbInst.props )
        {
            if( slot >= COMMENT_COUNT || propValue.empty() || mapped.count( propName ) )
                continue;

            titleBlock.SetComment( slot++, wxString::Format( wxS( "%s: %s" ), FromOrcadString( propName ),
                                                             FromOrcadString( propValue ) ) );
        }

        aScreen->SetTitleBlock( titleBlock );
    }
}


void ORCAD_CONVERTER::placeDefinitionVectors( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                              SCH_SCREEN* aScreen, double aTextScaleX, double aTextScaleY,
                                              bool aUseGenericTextBaseline, const std::string& aTextFaceOverride )
{
    ORCAD_BBOX bbox = aDefinition.bbox.value_or( ORCAD_BBOX() );
    int        width = bbox.x2 - bbox.x1;
    int        height = bbox.y2 - bbox.y1;

    auto transform = [&]( int aX, int aY )
    {
        return OrcadTransformPoint( aOrient, width, height, aBaseX, aBaseY, aX, aY );
    };

    ORCAD_GRAPHIC_INST graphic;
    graphic.rotation = aOrient & 3;
    graphic.color = aDefinition.color;
    graphic.textScaleX = aTextScaleX;
    graphic.textScaleY = aTextScaleY;
    graphic.useGenericTextBaseline = aUseGenericTextBaseline;
    graphic.useSymbolLineWidths = true;
    graphic.textFaceOverride = aTextFaceOverride;
    graphic.nested = std::make_unique<ORCAD_SYMBOL_DEF>();

    std::function<void( const std::vector<ORCAD_PRIMITIVE>&, int, int )> appendVectors =
            [&]( const std::vector<ORCAD_PRIMITIVE>& aPrimitives, int aOffsetX, int aOffsetY )
    {
        for( const ORCAD_PRIMITIVE& source : aPrimitives )
        {
            if( source.kind == ORCAD_PRIM_KIND::GROUP )
            {
                appendVectors( source.children, aOffsetX + source.x1, aOffsetY + source.y1 );
                continue;
            }

            if( source.kind == ORCAD_PRIM_KIND::IMAGE )
                continue;

            ORCAD_PRIMITIVE primitive = source;

            auto transformBox = [&]
            {
                std::array<VECTOR2I, 4> corners = { transform( aOffsetX + source.x1, aOffsetY + source.y1 ),
                                                    transform( aOffsetX + source.x2, aOffsetY + source.y1 ),
                                                    transform( aOffsetX + source.x2, aOffsetY + source.y2 ),
                                                    transform( aOffsetX + source.x1, aOffsetY + source.y2 ) };

                primitive.x1 = primitive.x2 = corners[0].x;
                primitive.y1 = primitive.y2 = corners[0].y;

                for( const VECTOR2I& corner : corners )
                {
                    primitive.x1 = std::min( primitive.x1, corner.x );
                    primitive.y1 = std::min( primitive.y1, corner.y );
                    primitive.x2 = std::max( primitive.x2, corner.x );
                    primitive.y2 = std::max( primitive.y2, corner.y );
                }
            };

            if( source.kind == ORCAD_PRIM_KIND::LINE )
            {
                VECTOR2I p1 = transform( aOffsetX + source.x1, aOffsetY + source.y1 );
                VECTOR2I p2 = transform( aOffsetX + source.x2, aOffsetY + source.y2 );
                primitive.x1 = p1.x;
                primitive.y1 = p1.y;
                primitive.x2 = p2.x;
                primitive.y2 = p2.y;
            }
            else if( source.kind == ORCAD_PRIM_KIND::TEXT )
            {
                VECTOR2I anchor = transform( aOffsetX + source.x1, aOffsetY + source.y1 );
                primitive.textBoundsStart.reset();
                primitive.x1 = primitive.x2 = anchor.x;
                primitive.y1 = primitive.y2 = anchor.y;
            }
            else
            {
                transformBox();
            }

            for( ORCAD_POINT& point : primitive.points )
            {
                VECTOR2I transformed = transform( aOffsetX + point.x, aOffsetY + point.y );
                point.x = transformed.x;
                point.y = transformed.y;
            }

            if( primitive.start )
            {
                VECTOR2I transformed = transform( aOffsetX + primitive.start->x, aOffsetY + primitive.start->y );
                primitive.start = ORCAD_POINT{ transformed.x, transformed.y };
            }

            if( primitive.end )
            {
                VECTOR2I transformed = transform( aOffsetX + primitive.end->x, aOffsetY + primitive.end->y );
                primitive.end = ORCAD_POINT{ transformed.x, transformed.y };
            }

            graphic.nested->primitives.push_back( std::move( primitive ) );
        }
    };

    appendVectors( aDefinition.primitives, 0, 0 );

    ORCAD_RAW_PAGE page;
    page.graphics.push_back( std::move( graphic ) );
    placeGraphics( page, aScreen );
}


void ORCAD_CONVERTER::placeDefinitionImages( const ORCAD_SYMBOL_DEF& aDefinition, int aBaseX, int aBaseY, int aOrient,
                                             SCH_SCREEN* aScreen )
{
    ORCAD_BBOX bbox = aDefinition.bbox.value_or( ORCAD_BBOX() );
    int        width = bbox.x2 - bbox.x1;
    int        height = bbox.y2 - bbox.y1;

    std::function<void( const std::vector<ORCAD_PRIMITIVE>&, int, int )> placeImages =
            [&]( const std::vector<ORCAD_PRIMITIVE>& aPrimitives, int aOffsetX, int aOffsetY )
    {
        for( const ORCAD_PRIMITIVE& primitive : aPrimitives )
        {
            if( primitive.kind == ORCAD_PRIM_KIND::GROUP )
            {
                placeImages( primitive.children, aOffsetX + primitive.x1, aOffsetY + primitive.y1 );
                continue;
            }

            if( primitive.kind != ORCAD_PRIM_KIND::IMAGE )
                continue;

            VECTOR2I        center = OrcadTransformPoint( aOrient, width, height, aBaseX, aBaseY,
                                                          aOffsetX + ( primitive.x1 + primitive.x2 ) / 2,
                                                          aOffsetY + ( primitive.y1 + primitive.y2 ) / 2 );
            int             imageWidth = std::abs( primitive.x2 - primitive.x1 );
            int             imageHeight = std::abs( primitive.y2 - primitive.y1 );
            ORCAD_PRIMITIVE image = primitive;
            image.x1 = center.x - imageWidth / 2;
            image.y1 = center.y - imageHeight / 2;
            image.x2 = image.x1 + imageWidth;
            image.y2 = image.y1 + imageHeight;
            placeBitmap( image, aScreen, aOrient );
        }
    };

    placeImages( aDefinition.primitives, 0, 0 );
}


void ORCAD_CONVERTER::buildNetLookup( const ORCAD_RAW_PAGE& aPage )
{
    m_wireEndpoints.clear();

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        m_wireEndpoints[{ wire.x1, wire.y1 }].push_back( &wire );
        m_wireEndpoints[{ wire.x2, wire.y2 }].push_back( &wire );
    }
}


std::string ORCAD_CONVERTER::netAt( const ORCAD_RAW_PAGE& aPage, int aX, int aY ) const
{
    auto wireName = [&aPage]( const ORCAD_WIRE& aWire )
    {
        for( const ORCAD_ALIAS& alias : aWire.aliases )
        {
            if( !trimmed( alias.name ).empty() )
                return alias.name;
        }

        auto aliases = aPage.netAliases.find( aWire.id );

        if( aliases != aPage.netAliases.end() )
        {
            std::set<std::string> names;

            for( const std::string& alias : aliases->second )
            {
                if( !trimmed( alias ).empty() )
                    names.insert( alias );
            }

            if( names.size() == 1 )
                return *names.begin();

            return std::string();
        }

        auto net = aPage.netmap.find( aWire.id );
        return net != aPage.netmap.end() ? net->second : std::string();
    };

    auto endIt = m_wireEndpoints.find( { aX, aY } );

    const std::vector<const ORCAD_WIRE*>* endWires = endIt != m_wireEndpoints.end() ? &endIt->second : nullptr;

    if( endWires )
    {
        for( const ORCAD_WIRE* wire : *endWires )
        {
            std::string name = wireName( *wire );

            if( !name.empty() )
                return name;
        }
    }

    // Also try wires passing through point.
    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        if( onSegment( aX, aY, wire ) )
        {
            std::string name = wireName( wire );

            if( !name.empty() )
                return name;
        }
    }

    return std::string();
}


std::string ORCAD_CONVERTER::powerNet( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const
{
    VECTOR2I                                         pin = powerPinPos( aPage, aInst );
    std::string                                      net = netAt( aPage, pin.x, pin.y );
    auto                                             nameIt = aInst.props.find( "Name" );
    std::string propertyName = nameIt != aInst.props.end() ? trimmed( nameIt->second ) : std::string();
    std::string logicalName = trimmed( aInst.logicalName );

    bool                                             authoritativeOccurrenceNet = false;
    std::map<uint32_t, std::set<const std::string*>> occurrenceNamesByPageNetId;
    auto occurrenceElectricalName = [&]( const std::string* aName )
    {
        return aName ? *aName : std::string();
    };

    if( m_currentOccNetNames )
    {
        std::set<const std::string*> generatedNamesAtPin;

        for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
        {
            for( size_t pinIndex = 0; pinIndex < instance.pins.size(); ++pinIndex )
            {
                const ORCAD_PIN_INST& placedPin = instance.pins[pinIndex];

                if( !placedPin.wordA && !placedPin.wordB )
                    continue;

                VECTOR2I electricalPosition = placedPinElectricalPosition( instance, pinIndex );
                bool     touchesWire = std::any_of( aPage.wires.begin(), aPage.wires.end(),
                                                    [&]( const ORCAD_WIRE& aWire )
                                                    {
                                                    return onSegment( placedPin.x, placedPin.y, aWire );
                                                } );

                if( electricalPosition != pin
                    || ( electricalPosition == VECTOR2I( placedPin.x, placedPin.y ) && touchesWire ) )
                {
                    continue;
                }

                std::string generatedName = "N" + std::to_string( instance.dbId ) + std::to_string( pinIndex );

                for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
                {
                    if( occurrenceName == generatedName )
                        generatedNamesAtPin.insert( &occurrenceName );
                }
            }
        }

        if( generatedNamesAtPin.size() == 1 )
            return occurrenceElectricalName( *generatedNamesAtPin.begin() );

        if( net.empty() && !logicalName.empty() )
        {
            std::set<const std::string*> matchingNames;

            for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
            {
                if( wxString::FromUTF8( kicadOccurrenceNetName( occurrenceName ) )
                            .CmpNoCase( wxString::FromUTF8( logicalName ) )
                    == 0 )
                {
                    matchingNames.insert( &occurrenceName );
                }
            }

            if( matchingNames.size() == 1 )
            {
                auto occurrence = std::find_if( m_currentOccNetNames->begin(), m_currentOccNetNames->end(),
                                                [&]( const auto& aEntry )
                                                {
                                                    return &aEntry.second == *matchingNames.begin();
                                                } );

                return occurrenceElectricalNetName( occurrence->first, occurrence->second );
            }
        }

        for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
        {
            if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
            {
                auto sourceWire = std::find_if( aPage.wires.begin(), aPage.wires.end(),
                                                [&]( const ORCAD_WIRE& aWire )
                                                {
                                                    return aWire.dbId == *objectId;
                                                } );

                if( sourceWire != aPage.wires.end() )
                {
                    occurrenceNamesByPageNetId[sourceWire->id].insert( &occurrenceName );
                    continue;
                }

                auto sourceInstance = std::find_if( aPage.instances.begin(), aPage.instances.end(),
                                                    [&]( const ORCAD_PLACED_INSTANCE& aInstance )
                                                    {
                                                        return aInstance.dbId == *objectId;
                                                    } );

                if( sourceInstance == aPage.instances.end() )
                    continue;

                std::string occurrenceKey = occurrenceName;
                std::transform( occurrenceKey.begin(), occurrenceKey.end(), occurrenceKey.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                auto matchesOccurrenceName = [&]( uint32_t aNetId )
                {
                    std::set<std::string> localNames;
                    auto                  pageName = aPage.netmap.find( aNetId );

                    if( pageName != aPage.netmap.end() )
                        localNames.insert( pageName->second );

                    auto aliases = aPage.netAliases.find( aNetId );

                    if( aliases != aPage.netAliases.end() )
                        localNames.insert( aliases->second.begin(), aliases->second.end() );

                    return std::any_of( localNames.begin(), localNames.end(),
                                        [&]( std::string aName )
                                        {
                                            std::transform( aName.begin(), aName.end(), aName.begin(),
                                                            []( unsigned char c )
                                                            {
                                                                return static_cast<char>( std::tolower( c ) );
                                                            } );
                                            return occurrenceKey == aName
                                                   || ( occurrenceKey.size() > aName.size()
                                                        && occurrenceKey[aName.size()] == '_'
                                                        && occurrenceKey.compare( 0, aName.size(), aName ) == 0 );
                                        } );
                };

                for( const ORCAD_PIN_INST& sourcePin : sourceInstance->pins )
                {
                    if( sourcePin.wordB && matchesOccurrenceName( sourcePin.wordB ) )
                        occurrenceNamesByPageNetId[sourcePin.wordB].insert( &occurrenceName );

                    for( const ORCAD_WIRE& wire : aPage.wires )
                    {
                        if( matchesOccurrenceName( wire.id )
                            && ( wire.id == sourcePin.wordB || wire.dbId == sourcePin.wordA
                                 || rawPointOnSegment( sourcePin.x, sourcePin.y, wire ) ) )
                        {
                            occurrenceNamesByPageNetId[wire.id].insert( &occurrenceName );
                        }
                    }
                }
            }
        }

        std::set<const std::string*> objectNets;
        std::set<const std::string*> occurrenceNets;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            bool endpoint = ( pin.x == wire.x1 && pin.y == wire.y1 ) || ( pin.x == wire.x2 && pin.y == wire.y2 );

            if( !endpoint && !onSegment( pin.x, pin.y, wire ) )
                continue;

            auto objectNames = occurrenceNamesByPageNetId.find( wire.id );

            if( objectNames != occurrenceNamesByPageNetId.end() && objectNames->second.size() == 1 )
                objectNets.insert( *objectNames->second.begin() );

            auto pageNet = aPage.netmap.find( wire.id );

            if( pageNet == aPage.netmap.end() || pageNet->second.empty() )
                continue;

            for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
            {
                if( wxString::FromUTF8( pageNet->second ).CmpNoCase( wxString::FromUTF8( occurrenceName ) ) == 0 )
                    occurrenceNets.insert( &occurrenceName );
            }
        }

        if( objectNets.size() == 1 )
        {
            net = occurrenceElectricalName( *objectNets.begin() );
            authoritativeOccurrenceNet = true;
        }
        else if( occurrenceNets.size() == 1 )
        {
            net = occurrenceElectricalName( *occurrenceNets.begin() );
            authoritativeOccurrenceNet = true;
        }
    }

    if( m_currentOccNetNames && !authoritativeOccurrenceNet )
    {
        std::set<const std::string*> directPinNets;

        for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
        {
            for( const ORCAD_PIN_INST& componentPin : instance.pins )
            {
                if( componentPin.x != pin.x || componentPin.y != pin.y || componentPin.IsNoConnect() )
                    continue;

                for( const ORCAD_WIRE& wire : aPage.wires )
                {
                    if( wire.dbId != componentPin.wordA && wire.id != componentPin.wordB )
                        continue;

                    auto names = occurrenceNamesByPageNetId.find( wire.id );

                    if( names != occurrenceNamesByPageNetId.end() && names->second.size() == 1 )
                        directPinNets.insert( *names->second.begin() );
                }
            }
        }

        if( directPinNets.size() == 1 )
        {
            net = occurrenceElectricalName( *directPinNets.begin() );
            authoritativeOccurrenceNet = true;
        }
    }

    if( m_currentOccNetNames && !authoritativeOccurrenceNet )
    {
        std::string        sourceName = trimmed( aInst.logicalName );
        std::set<uint32_t> matchingNetIds;

        for( const auto& [netId, pageName] : aPage.netmap )
        {
            bool matches = wxString::FromUTF8( pageName ).CmpNoCase( wxString::FromUTF8( sourceName ) ) == 0;
            auto aliases = aPage.netAliases.find( netId );

            if( aliases != aPage.netAliases.end() )
            {
                matches = matches
                          || std::any_of( aliases->second.begin(), aliases->second.end(),
                                          [&]( const std::string& aAlias )
                                          {
                                              return wxString::FromUTF8( aAlias ).CmpNoCase(
                                                             wxString::FromUTF8( sourceName ) )
                                                     == 0;
                                          } );
            }

            if( matches )
                matchingNetIds.insert( netId );
        }

        if( matchingNetIds.size() == 1 )
        {
            auto names = occurrenceNamesByPageNetId.find( *matchingNetIds.begin() );

            if( names != occurrenceNamesByPageNetId.end() && names->second.size() == 1 )
            {
                net = occurrenceElectricalName( *names->second.begin() );
                authoritativeOccurrenceNet = true;
            }
        }
    }

    if( net.empty() )
    {
        std::set<std::string> pinNetNames;

        for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
        {
            for( const ORCAD_PIN_INST& componentPin : instance.pins )
            {
                if( componentPin.x != pin.x || componentPin.y != pin.y || componentPin.IsNoConnect() )
                    continue;

                for( uint32_t netId : { componentPin.wordB, componentPin.wordA } )
                {
                    std::set<std::string> sourceNames;
                    auto                  netName = aPage.netmap.find( netId );

                    if( netName != aPage.netmap.end() && !netName->second.empty() )
                        sourceNames.insert( netName->second );

                    auto aliases = aPage.netAliases.find( netId );

                    if( aliases != aPage.netAliases.end() )
                        sourceNames.insert( aliases->second.begin(), aliases->second.end() );

                    std::set<std::string> occurrenceNames;

                    if( m_currentOccNetNames )
                    {
                        for( const std::string& sourceName : sourceNames )
                        {
                            for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
                            {
                                if( wxString::FromUTF8( sourceName ).CmpNoCase( wxString::FromUTF8( occurrenceName ) )
                                    == 0 )
                                {
                                    occurrenceNames.insert( occurrenceName );
                                }
                            }
                        }
                    }

                    if( occurrenceNames.size() == 1 )
                        pinNetNames.insert( *occurrenceNames.begin() );
                    else if( sourceNames.size() == 1 )
                        pinNetNames.insert( *sourceNames.begin() );
                }
            }
        }

        if( pinNetNames.size() == 1 )
            net = *pinNetNames.begin();
    }

    auto        namesEqual = []( const std::string& aLeft, const std::string& aRight )
    {
        return wxString::FromUTF8( aLeft ).CmpNoCase( wxString::FromUTF8( aRight ) ) == 0;
    };

    std::string physicalName = !propertyName.empty() ? propertyName : logicalName;

    if( !physicalName.empty() )
    {
        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            bool endpoint = ( pin.x == wire.x1 && pin.y == wire.y1 ) || ( pin.x == wire.x2 && pin.y == wire.y2 );

            if( !endpoint && !onSegment( pin.x, pin.y, wire ) )
                continue;

            std::vector<std::string> names;
            auto                     pageName = aPage.netmap.find( wire.id );

            if( pageName != aPage.netmap.end() )
                names.push_back( pageName->second );

            auto aliases = aPage.netAliases.find( wire.id );

            if( aliases != aPage.netAliases.end() )
                names.insert( names.end(), aliases->second.begin(), aliases->second.end() );

            std::set<std::string> distinctNames;

            for( std::string name : names )
            {
                std::transform( name.begin(), name.end(), name.begin(),
                                []( unsigned char aChar ) { return static_cast<char>( std::tolower( aChar ) ); } );
                distinctNames.insert( std::move( name ) );
            }

            bool hasPhysicalName = std::any_of( names.begin(), names.end(),
                                                [&]( const std::string& aName )
                                                { return namesEqual( aName, physicalName ); } );

            if( ( !m_currentOccNetNames || m_currentOccNetNames->empty() ) && distinctNames.size() > 1
                && hasPhysicalName )
                return canonicalGlobalNetName( physicalName );
        }
    }

    if( m_currentOccNetNames && m_currentOccNetNames->empty() && !net.empty() && !isPowerNetName( net ) )
    {
        std::string sourcePowerName = !logicalName.empty() ? logicalName : propertyName;

        if( isPowerNetName( sourcePowerName ) )
            net = std::move( sourcePowerName );
    }

    if( m_currentOccNetNames && !authoritativeOccurrenceNet )
    {
        std::set<const std::string*> occurrenceAliases;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            bool endpoint = ( pin.x == wire.x1 && pin.y == wire.y1 ) || ( pin.x == wire.x2 && pin.y == wire.y2 );

            if( !endpoint && !onSegment( pin.x, pin.y, wire ) )
                continue;

            auto aliases = aPage.netAliases.find( wire.id );

            if( aliases == aPage.netAliases.end() )
                continue;

            for( const std::string& alias : aliases->second )
            {
                for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
                {
                    if( wxString::FromUTF8( alias ).CmpNoCase( wxString::FromUTF8( occurrenceName ) ) == 0 )
                        occurrenceAliases.insert( &occurrenceName );
                }
            }
        }

        if( occurrenceAliases.size() == 1 )
            net = occurrenceElectricalName( *occurrenceAliases.begin() );
    }

    bool implicitGeneratedName = net.size() > 1 && net[0] == 'N'
                                 && std::all_of( net.begin() + 1, net.end(),
                                                 []( unsigned char c )
                                                 {
                                                     return std::isdigit( c );
                                                 } );

    if( net.empty() || ( implicitGeneratedName && !authoritativeOccurrenceNet ) )
    {
        if( !logicalName.empty() )
            net = logicalName;
        else if( !propertyName.empty() )
            net = propertyName;
    }

    if( net.empty() )
        net = aInst.name;

    return effectiveInterfaceNetName( net );
}


VECTOR2I ORCAD_CONVERTER::graphicPinPos( const ORCAD_GRAPHIC_INST& aInst ) const
{
    auto symIt = m_design.symbols.find( aInst.name );

    if( symIt == m_design.symbols.end() || symIt->second.pins.empty() )
        return VECTOR2I( aInst.x, aInst.y );

    const ORCAD_SYMBOL_DEF& sym = symIt->second;
    int                     baseX = std::min( aInst.bbox.x1, aInst.bbox.x2 );
    int                     baseY = std::min( aInst.bbox.y1, aInst.bbox.y2 );

    ORCAD_BBOX symBox = sym.bbox.value_or( ORCAD_BBOX() );
    int        width = symBox.x2 - symBox.x1;
    int        height = symBox.y2 - symBox.y1;
    int        bboxWidth = std::abs( aInst.bbox.x2 - aInst.bbox.x1 );
    int        bboxHeight = std::abs( aInst.bbox.y2 - aInst.bbox.y1 );
    bool       storedExtent = std::abs( aInst.bbox.x2 ) == width && std::abs( aInst.bbox.y2 ) == height;
    bool legacyExtent = width > 0 && height > 0 && storedExtent && ( bboxWidth > 4 * width || bboxHeight > 4 * height );

    if( legacyExtent )
    {
        baseX = std::max( aInst.bbox.x1, aInst.bbox.x2 ) - std::abs( aInst.x );
        baseY = std::max( aInst.bbox.y1, aInst.bbox.y2 ) - std::abs( aInst.y );
    }

    int                     orient = OrcadOrientOf( aInst.rotation, aInst.mirror );
    const ORCAD_SYMBOL_PIN& pin = sym.pins[0];

    return OrcadTransformPoint( orient, width, height, baseX, baseY, pin.hotptX, pin.hotptY );
}


VECTOR2I ORCAD_CONVERTER::namedGraphicPinPos( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const
{
    VECTOR2I      candidate = graphicPinPos( aInst );
    std::string   logicalName = trimmed( aInst.logicalName );
    constexpr int MAX_PIN_SNAP_DISTANCE = 20;

    std::transform( logicalName.begin(), logicalName.end(), logicalName.begin(),
                    []( unsigned char c )
                    {
                        return static_cast<char>( std::tolower( c ) );
                    } );

    if( logicalName.empty() )
        return candidate;

    size_t matchingConnectors = 0;
    auto   countMatchingConnectors = [&]( const std::vector<ORCAD_GRAPHIC_INST>& aConnectors )
    {
        matchingConnectors += std::count_if( aConnectors.begin(), aConnectors.end(),
                                             [&]( const ORCAD_GRAPHIC_INST& aConnector )
                                             {
                                                 std::string name = trimmed( aConnector.logicalName );
                                                 std::transform( name.begin(), name.end(), name.begin(),
                                                                 []( unsigned char c )
                                                                 {
                                                                     return static_cast<char>( std::tolower( c ) );
                                                                 } );
                                                 return name == logicalName;
                                             } );
    };

    countMatchingConnectors( aPage.globals );
    countMatchingConnectors( aPage.offpage );
    countMatchingConnectors( aPage.ports );

    int64_t  bestDistanceSquared = matchingConnectors > 1 ? MAX_PIN_SNAP_DISTANCE * MAX_PIN_SNAP_DISTANCE + 1
                                                          : std::numeric_limits<int64_t>::max();
    VECTOR2I best = candidate;

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        auto netIt = aPage.netmap.find( wire.id );

        if( netIt == aPage.netmap.end() )
            continue;

        std::string wireName = trimmed( netIt->second );
        std::transform( wireName.begin(), wireName.end(), wireName.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( wireName != logicalName )
            continue;

        for( const VECTOR2I& endpoint : { VECTOR2I( wire.x1, wire.y1 ), VECTOR2I( wire.x2, wire.y2 ) } )
        {
            int64_t dx = endpoint.x - candidate.x;
            int64_t dy = endpoint.y - candidate.y;
            int64_t distanceSquared = dx * dx + dy * dy;

            if( distanceSquared < bestDistanceSquared )
            {
                bestDistanceSquared = distanceSquared;
                best = endpoint;
            }
        }
    }

    return best;
}


VECTOR2I ORCAD_CONVERTER::powerPinPos( const ORCAD_RAW_PAGE& aPage, const ORCAD_GRAPHIC_INST& aInst ) const
{
    VECTOR2I      sourceAnchor( aInst.x, aInst.y );
    constexpr int MAX_PIN_SNAP_DISTANCE = 20;
    int64_t       bestDistanceSquared = MAX_PIN_SNAP_DISTANCE * MAX_PIN_SNAP_DISTANCE + 1;
    VECTOR2I      best = sourceAnchor;
    std::string   powerName = trimmed( aInst.logicalName );

    if( powerName.empty() )
    {
        auto property = aInst.props.find( "Name" );

        if( property != aInst.props.end() )
            powerName = trimmed( property->second );
    }

    auto namesEqual = []( const std::string& aLeft, const std::string& aRight )
    {
        return wxString::FromUTF8( aLeft ).CmpNoCase( wxString::FromUTF8( aRight ) ) == 0;
    };

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        for( const ORCAD_PIN_INST& pin : instance.pins )
        {
            if( pin.IsNoConnect() || pin.wordA != std::numeric_limits<uint32_t>::max() || !pin.wordB )
                continue;

            std::vector<std::string> pinNetNames;
            auto                     pageName = aPage.netmap.find( pin.wordB );

            if( pageName != aPage.netmap.end() && !trimmed( pageName->second ).empty() )
                pinNetNames.push_back( trimmed( pageName->second ) );

            auto aliases = aPage.netAliases.find( pin.wordB );

            if( aliases != aPage.netAliases.end() )
                pinNetNames.insert( pinNetNames.end(), aliases->second.begin(), aliases->second.end() );

            if( !powerName.empty() && !pinNetNames.empty()
                && std::none_of( pinNetNames.begin(), pinNetNames.end(),
                                 [&]( const std::string& aName )
                                 {
                                     return namesEqual( aName, powerName );
                                 } ) )
            {
                continue;
            }

            int64_t dx = pin.x - sourceAnchor.x;
            int64_t dy = pin.y - sourceAnchor.y;
            int64_t distanceSquared = dx * dx + dy * dy;

            if( distanceSquared < bestDistanceSquared )
            {
                bestDistanceSquared = distanceSquared;
                best = VECTOR2I( pin.x, pin.y );
            }
        }
    }

    if( bestDistanceSquared <= MAX_PIN_SNAP_DISTANCE * MAX_PIN_SNAP_DISTANCE )
        return best;

    return namedGraphicPinPos( aPage, aInst );
}


std::vector<int> ORCAD_CONVERTER::placedStackedPinOffsets( const ORCAD_PLACED_INSTANCE& aInstance ) const
{
    std::vector<int> offsets( aInstance.pins.size() );

    auto shareNet = []( const ORCAD_PIN_INST& aLeft, const ORCAD_PIN_INST& aRight )
    {
        if( ( !aLeft.wordA && !aLeft.wordB ) || ( !aRight.wordA && !aRight.wordB ) )
            return true;

        return ( aLeft.wordA && aLeft.wordA == aRight.wordA ) || ( aLeft.wordB && aLeft.wordB == aRight.wordB );
    };

    for( size_t i = 0; i < aInstance.pins.size(); ++i )
    {
        const ORCAD_PIN_INST& pin = aInstance.pins[i];
        int                   nextOffset = 0;

        for( size_t j = 0; j < i; ++j )
        {
            const ORCAD_PIN_INST& peer = aInstance.pins[j];

            if( peer.x != pin.x || peer.y != pin.y )
                continue;

            if( shareNet( pin, peer ) )
            {
                offsets[i] = offsets[j];
                nextOffset = -1;
                break;
            }

            nextOffset = std::max( nextOffset, offsets[j] + 1 );
        }

        if( nextOffset >= 0 )
            offsets[i] = nextOffset;
    }

    return offsets;
}


VECTOR2I ORCAD_CONVERTER::placedPinElectricalPosition( const ORCAD_PLACED_INSTANCE& aInstance, size_t aPinIndex ) const
{
    const ORCAD_PIN_INST& pin = aInstance.pins[aPinIndex];
    std::vector<int>      offsets = placedStackedPinOffsets( aInstance );
    int                   orient = OrcadOrientOf( aInstance.rotation, aInstance.mirror );
    const ORCAD_ORIENT_ENTRY& transform = ORCAD_ORIENT_TABLE[orient];

    return VECTOR2I( pin.x + transform.a * offsets[aPinIndex], pin.y + transform.c * offsets[aPinIndex] );
}


std::vector<ORCAD_CONVERTER::OFFPAGE_NET> ORCAD_CONVERTER::offpageNets( const ORCAD_RAW_PAGE& aPage ) const
{
    std::vector<OFFPAGE_NET>                         out;
    std::map<uint32_t, std::set<const std::string*>> occurrenceNamesByPageNetId;

    if( m_currentOccNetNames )
    {
        for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
        {
            if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
            {
                auto sourceWire = std::find_if( aPage.wires.begin(), aPage.wires.end(),
                                                [&]( const ORCAD_WIRE& aWire )
                                                {
                                                    return aWire.dbId == *objectId;
                                                } );

                if( sourceWire != aPage.wires.end() )
                {
                    occurrenceNamesByPageNetId[sourceWire->id].insert( &occurrenceName );
                    continue;
                }

                auto sourceInstance = std::find_if( aPage.instances.begin(), aPage.instances.end(),
                                                    [&]( const ORCAD_PLACED_INSTANCE& aInstance )
                                                    {
                                                        return aInstance.dbId == *objectId;
                                                    } );

                if( sourceInstance == aPage.instances.end() )
                    continue;

                std::string occurrenceKey = occurrenceName;
                std::transform( occurrenceKey.begin(), occurrenceKey.end(), occurrenceKey.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                auto matchesOccurrenceName = [&]( uint32_t aNetId )
                {
                    std::set<std::string> localNames;
                    auto                  pageName = aPage.netmap.find( aNetId );

                    if( pageName != aPage.netmap.end() )
                        localNames.insert( pageName->second );

                    auto aliases = aPage.netAliases.find( aNetId );

                    if( aliases != aPage.netAliases.end() )
                        localNames.insert( aliases->second.begin(), aliases->second.end() );

                    return std::any_of( localNames.begin(), localNames.end(),
                                        [&]( std::string aName )
                                        {
                                            std::transform( aName.begin(), aName.end(), aName.begin(),
                                                            []( unsigned char c )
                                                            {
                                                                return static_cast<char>( std::tolower( c ) );
                                                            } );
                                            return occurrenceKey == aName
                                                   || ( occurrenceKey.size() > aName.size()
                                                        && occurrenceKey[aName.size()] == '_'
                                                        && occurrenceKey.compare( 0, aName.size(), aName ) == 0 );
                                        } );
                };

                for( const ORCAD_PIN_INST& pin : sourceInstance->pins )
                {
                    if( pin.wordB && matchesOccurrenceName( pin.wordB ) )
                        occurrenceNamesByPageNetId[pin.wordB].insert( &occurrenceName );

                    for( const ORCAD_WIRE& wire : aPage.wires )
                    {
                        if( matchesOccurrenceName( wire.id )
                            && ( wire.id == pin.wordB || wire.dbId == pin.wordA
                                 || rawPointOnSegment( pin.x, pin.y, wire ) ) )
                        {
                            occurrenceNamesByPageNetId[wire.id].insert( &occurrenceName );
                        }
                    }
                }
            }
        }
    }

    for( size_t i = 0; i < aPage.offpage.size(); ++i )
    {
        const ORCAD_GRAPHIC_INST& conn = aPage.offpage[i];
        VECTOR2I                  pin = namedGraphicPinPos( aPage, conn );

        OFFPAGE_NET entry;
        entry.index = static_cast<int>( i );

        std::set<const std::string*> occurrenceNets;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            if( wire.isBus || !onSegment( pin.x, pin.y, wire ) )
                continue;

            auto occurrenceNames = occurrenceNamesByPageNetId.find( wire.id );

            if( occurrenceNames != occurrenceNamesByPageNetId.end() && occurrenceNames->second.size() == 1 )
                occurrenceNets.insert( *occurrenceNames->second.begin() );
        }

        std::set<uint32_t> matchingNetIds;

        for( const auto& [netId, pageName] : aPage.netmap )
        {
            bool matches = wxString::FromUTF8( pageName ).CmpNoCase( wxString::FromUTF8( conn.logicalName ) ) == 0;
            auto aliases = aPage.netAliases.find( netId );

            if( aliases != aPage.netAliases.end() )
            {
                matches = matches
                          || std::any_of( aliases->second.begin(), aliases->second.end(),
                                          [&]( const std::string& aAlias )
                                          {
                                              return wxString::FromUTF8( aAlias ).CmpNoCase(
                                                             wxString::FromUTF8( conn.logicalName ) )
                                                     == 0;
                                          } );
            }

            if( matches )
                matchingNetIds.insert( netId );
        }

        if( occurrenceNets.empty() )
        {
            if( matchingNetIds.size() == 1 )
            {
                auto names = occurrenceNamesByPageNetId.find( *matchingNetIds.begin() );

                if( names != occurrenceNamesByPageNetId.end() && names->second.size() == 1 )
                    occurrenceNets.insert( *names->second.begin() );
            }
        }

        if( occurrenceNets.size() == 1 )
        {
            entry.net = canonicalGlobalNetName( **occurrenceNets.begin() );
        }
        else
        {
            entry.net = matchingNetIds.size() > 1 ? canonicalGlobalNetName( conn.logicalName )
                                                  : effectiveInterfaceNetName( conn.logicalName );
        }

        entry.x = pin.x;
        entry.y = pin.y;

        out.push_back( entry );
    }

    return out;
}


std::vector<VECTOR2I> ORCAD_CONVERTER::computeJunctions( const ORCAD_RAW_PAGE& aPage ) const
{
    std::set<std::pair<int, int>> pinPts;

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        for( const ORCAD_PIN_INST& pin : instance.pins )
            pinPts.insert( { pin.x, pin.y } );
    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        for( const ORCAD_BLOCK_PIN& pin : block.pins )
            pinPts.insert( { pin.x, pin.y } );
    }

    std::map<std::pair<int, int>, int> ends;

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        ends[{ wire.x1, wire.y1 }]++;
        ends[{ wire.x2, wire.y2 }]++;
    }

    std::set<std::pair<int, int>> candidates = pinPts;
    std::set<std::pair<int, int>> interiorCrossings;

    for( const std::pair<const std::pair<int, int>, int>& end : ends )
        candidates.insert( end.first );

    for( size_t firstIndex = 0; firstIndex < aPage.wires.size(); ++firstIndex )
    {
        const ORCAD_WIRE& first = aPage.wires[firstIndex];

        if( first.isBus )
            continue;

        for( size_t secondIndex = firstIndex + 1; secondIndex < aPage.wires.size(); ++secondIndex )
        {
            const ORCAD_WIRE& second = aPage.wires[secondIndex];

            if( second.isBus || first.id != second.id )
                continue;

            std::optional<VECTOR2I> intersection = rawWireIntersection( first, second );

            if( intersection && onSegment( intersection->x, intersection->y, first )
                && onSegment( intersection->x, intersection->y, second ) )
            {
                interiorCrossings.insert( { intersection->x, intersection->y } );
                candidates.insert( { intersection->x, intersection->y } );
            }
        }
    }

    std::vector<VECTOR2I> out;

    for( const std::pair<int, int>& pt : candidates )
    {
        auto               endIt = ends.find( pt );
        int                endCount = endIt != ends.end() ? endIt->second : 0;
        int                through = 0;
        std::set<uint32_t> netIds;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            if( onSegment( pt.first, pt.second, wire ) )
            {
                through++;
                netIds.insert( wire.id );
            }
        }

        int pinCount = pinPts.count( pt ) ? 1 : 0;
        int score = endCount + 2 * through + pinCount;

        // Junction needed when 3+ contributions meet and at least one terminates there.
        bool interiorCrossing = interiorCrossings.count( pt ) != 0;

        if( netIds.size() <= 1 && score >= 3 && ( endCount + pinCount >= 1 || interiorCrossing )
            && endCount + through >= 2 )
        {
            out.emplace_back( pt.first, pt.second );
        }
    }

    return out;
}


void ORCAD_CONVERTER::placeJunctions( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const VECTOR2I& pt : computeJunctions( aPage ) )
        appendPageItem( aScreen, new SCH_JUNCTION( OrcadDbuToIu( pt.x, pt.y ) ) );
}


void ORCAD_CONVERTER::placeWires( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, bool aHierarchical,
                                  bool aNestedHierarchy, bool aSharedFolderPage )
{
    bool sharedFlatFolder = aSharedFolderPage && !m_scopeNamedFlatNets && !m_scopeGeneratedFlatNets;
    std::map<uint32_t, const std::string*>               generatedNamesByObjectId;
    std::map<uint32_t, const std::string*>               occurrenceNamesByNetId;
    std::set<uint32_t>                                   localizedOwnedOccurrenceNetIds;
    std::map<uint32_t, std::string>                      connectorNamesByObjectId;
    std::set<uint32_t>                                   globalConnectorObjectIds;
    std::map<uint32_t, std::string>                      netNamesByObjectId;
    std::map<uint32_t, const std::string*>               netNamesByWireObjectId;
    std::map<uint32_t, std::set<std::string>>            blockPinNamesByNetId;
    std::map<std::pair<int, int>, std::set<std::string>> connectorNamesByPosition;
    std::set<std::pair<int, int>>                        globalConnectorPositions;
    std::vector<size_t>                                  wireParents( aPage.wires.size() );
    std::map<size_t, std::set<std::string>>              powerAliasesByWireGroup;
    std::map<size_t, std::set<std::string>>              powerGlobalNamesByWireGroup;
    std::map<size_t, std::set<std::string>>              resolvedPowerNamesByWireGroup;
    std::map<uint32_t, std::set<std::string>>            resolvedPowerNamesByNetId;
    std::map<uint32_t, std::string>                      flattenedNamesByNetId;
    std::set<uint32_t>                                   renamedNetIds;
    std::set<std::string>                                occurrenceNames;
    std::map<const std::string*, std::string>            occurrenceElectricalNames;
    const std::map<std::string, std::string>*            hierBusNames = nullptr;
    std::iota( wireParents.begin(), wireParents.end(), 0 );

    if( m_currentOccNetNames )
    {
        for( const auto& [occurrenceId, name] : *m_currentOccNetNames )
            occurrenceElectricalNames[&name] = occurrenceElectricalNetName( occurrenceId, name );
    }

    auto occurrenceElectricalName = [&]( const std::string* aName ) -> const std::string&
    {
        return occurrenceElectricalNames.at( aName );
    };

    auto wireRoot = [&]( size_t aIndex )
    {
        while( wireParents[aIndex] != aIndex )
        {
            wireParents[aIndex] = wireParents[wireParents[aIndex]];
            aIndex = wireParents[aIndex];
        }

        return aIndex;
    };

    auto joinWires = [&]( size_t aLeft, size_t aRight )
    {
        size_t left = wireRoot( aLeft );
        size_t right = wireRoot( aRight );

        if( left != right )
            wireParents[right] = left;
    };

    auto pointOnWire = []( const VECTOR2I& aPoint, const ORCAD_WIRE& aWire )
    {
        return ( aPoint.x == aWire.x1 && aPoint.y == aWire.y1 ) || ( aPoint.x == aWire.x2 && aPoint.y == aWire.y2 )
               || onSegment( aPoint.x, aPoint.y, aWire );
    };

    std::map<uint32_t, std::vector<size_t>> powerWireIndicesByNetId;

    for( size_t i = 0; i < aPage.wires.size(); ++i )
    {
        if( !aPage.wires[i].isBus )
            powerWireIndicesByNetId[aPage.wires[i].id].push_back( i );
    }

    std::vector<VECTOR2I> junctions = computeJunctions( aPage );

    for( const auto& [netId, indices] : powerWireIndicesByNetId )
    {
        for( size_t left = 0; left < indices.size(); ++left )
        {
            const ORCAD_WIRE& leftWire = aPage.wires[indices[left]];

            for( size_t right = left + 1; right < indices.size(); ++right )
            {
                const ORCAD_WIRE& rightWire = aPage.wires[indices[right]];
                bool              connected = pointOnWire( VECTOR2I( leftWire.x1, leftWire.y1 ), rightWire )
                                 || pointOnWire( VECTOR2I( leftWire.x2, leftWire.y2 ), rightWire )
                                 || pointOnWire( VECTOR2I( rightWire.x1, rightWire.y1 ), leftWire )
                                 || pointOnWire( VECTOR2I( rightWire.x2, rightWire.y2 ), leftWire );

                if( !connected )
                {
                    connected = std::any_of( junctions.begin(), junctions.end(),
                                             [&]( const VECTOR2I& aJunction )
                                             {
                                                 return pointOnWire( aJunction, leftWire )
                                                        && pointOnWire( aJunction, rightWire );
                                             } );
                }

                if( connected )
                    joinWires( indices[left], indices[right] );
            }
        }
    }

    for( const ORCAD_NET_GROUP& net : aPage.netGroups )
    {
        if( !net.name.empty() )
            netNamesByObjectId[net.id] = canonicalGlobalNetName( net.name );
    }

    for( const ORCAD_GRAPHIC_INST& global : aPage.globals )
    {
        std::string name = powerNet( aPage, global );
        connectorNamesByObjectId[global.dbId] = name;
        globalConnectorObjectIds.insert( global.dbId );
        VECTOR2I position = powerPinPos( aPage, global );
        connectorNamesByPosition[{ position.x, position.y }].insert( name );
        globalConnectorPositions.insert( { position.x, position.y } );

        std::set<std::string> sourceNames;
        std::string           logicalName = effectiveInterfaceNetName( trimmed( global.logicalName ) );

        if( !logicalName.empty() )
            sourceNames.insert( logicalName );

        auto property = global.props.find( "Name" );

        if( property != global.props.end() )
        {
            std::string propertyName = effectiveInterfaceNetName( trimmed( property->second ) );

            if( !propertyName.empty() )
                sourceNames.insert( propertyName );
        }

        VECTOR2I pin = powerPinPos( aPage, global );

        for( size_t wireIndex = 0; wireIndex < aPage.wires.size(); ++wireIndex )
        {
            const ORCAD_WIRE& wire = aPage.wires[wireIndex];
            bool endpoint = ( pin.x == wire.x1 && pin.y == wire.y1 ) || ( pin.x == wire.x2 && pin.y == wire.y2 );

            if( !wire.isBus && ( endpoint || onSegment( pin.x, pin.y, wire ) ) )
            {
                size_t root = wireRoot( wireIndex );
                powerAliasesByWireGroup[root].insert( sourceNames.begin(), sourceNames.end() );
                powerGlobalNamesByWireGroup[root].insert( sourceNames.begin(), sourceNames.end() );
                resolvedPowerNamesByWireGroup[root].insert( name );
                resolvedPowerNamesByNetId[wire.id].insert( name );
            }
        }
    }

    for( const auto& [root, globalNames] : powerGlobalNamesByWireGroup )
    {
        if( globalNames.size() < 2 )
            continue;

        for( size_t wireIndex = 0; wireIndex < aPage.wires.size(); ++wireIndex )
        {
            if( aPage.wires[wireIndex].isBus || wireRoot( wireIndex ) != root )
                continue;

            for( const ORCAD_ALIAS& alias : aPage.wires[wireIndex].aliases )
            {
                std::string name = effectiveInterfaceNetName( alias.name );

                if( !name.empty() )
                    powerAliasesByWireGroup[root].insert( std::move( name ) );
            }
        }
    }

    for( const ORCAD_GRAPHIC_INST& offpage : aPage.offpage )
    {
        std::string name =
                effectiveInterfaceNetName( offpage.logicalName.empty() ? offpage.name : offpage.logicalName );
        connectorNamesByObjectId[offpage.dbId] = name;
        if( !aHierarchical )
            globalConnectorObjectIds.insert( offpage.dbId );
        VECTOR2I position = namedGraphicPinPos( aPage, offpage );
        connectorNamesByPosition[{ position.x, position.y }].insert( std::move( name ) );
        if( !aHierarchical )
            globalConnectorPositions.insert( { position.x, position.y } );
    }

    for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
    {
        std::string name = canonicalGlobalNetName( port.logicalName.empty() ? port.name : port.logicalName );
        connectorNamesByObjectId[port.dbId] = name;
        if( !aHierarchical )
            globalConnectorObjectIds.insert( port.dbId );
        VECTOR2I position = namedGraphicPinPos( aPage, port );
        connectorNamesByPosition[{ position.x, position.y }].insert( std::move( name ) );
        if( !aHierarchical )
            globalConnectorPositions.insert( { position.x, position.y } );
    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        for( const ORCAD_BLOCK_PIN& pin : block.pins )
        {
            std::string name = canonicalGlobalNetName( pin.name );

            if( name.empty() )
                continue;

            connectorNamesByPosition[{ pin.x, pin.y }].insert( name );

            for( const ORCAD_WIRE& wire : aPage.wires )
            {
                if( !wire.isBus && pointOnWire( VECTOR2I( pin.x, pin.y ), wire ) )
                    blockPinNamesByNetId[wire.id].insert( name );
            }
        }
    }

    auto isImplicitGeneratedName = []( const std::string& aName )
    {
        if( aName.size() < 2 || aName[0] != 'N' || !std::isdigit( static_cast<unsigned char>( aName[1] ) ) )
            return false;

        return std::all_of( aName.begin() + 1, aName.end(),
                            []( unsigned char c )
                            {
                                return std::isdigit( c );
                            } );
    };

    auto busNames = m_hierBusNamesByScreen.find( aScreen->GetUuid().AsStdString() );

    if( busNames != m_hierBusNamesByScreen.end() )
        hierBusNames = &busNames->second;

    if( m_currentOccNetNames )
    {
        std::map<uint32_t, uint32_t>                     netIdByWireObjectId;
        std::map<uint32_t, std::set<const std::string*>> objectNamesByNetId;

        for( const ORCAD_WIRE& wire : aPage.wires )
            netIdByWireObjectId[wire.dbId] = wire.id;

        auto lower = []( const std::string& aName )
        {
            std::string result = aName;
            std::transform( result.begin(), result.end(), result.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            return result;
        };
        for( const auto& [netId, localName] : aPage.netmap )
        {
            std::set<std::string>        localNames = { localName };
            std::set<const std::string*> candidates;
            std::set<const std::string*> ownedCandidates;
            bool                         localizedOwnedCandidate = false;

            auto aliases = aPage.netAliases.find( netId );

            if( aliases != aPage.netAliases.end() )
                localNames.insert( aliases->second.begin(), aliases->second.end() );

            bool directOffpage = std::any_of(
                    aPage.offpage.begin(), aPage.offpage.end(),
                    [&]( const ORCAD_GRAPHIC_INST& aConnector )
                    {
                        std::string connectorName = aConnector.logicalName.empty() ? aConnector.name
                                                                                   : aConnector.logicalName;
                        std::string connectorKey = lower( connectorName );
                        return std::any_of( localNames.begin(), localNames.end(),
                                            [&]( const std::string& aName )
                                            {
                                                return lower( aName ) == connectorKey;
                                            } );
                    } );

            for( const std::string& candidateLocalName : localNames )
            {
                bool        implicitGenerated = isImplicitGeneratedName( candidateLocalName );
                std::string localKey = lower( candidateLocalName );

                for( const auto& [occurrenceId, occurrenceName] : *m_currentOccNetNames )
                {
                    std::string occurrenceKey = lower( occurrenceName );
                    bool        occurrenceOwnedByNet = false;

                    if( std::optional<uint32_t> objectId = occurrenceNetObjectId( occurrenceName ) )
                    {
                        occurrenceOwnedByNet = std::any_of(
                                aPage.instances.begin(), aPage.instances.end(),
                                [&]( const ORCAD_PLACED_INSTANCE& aInstance )
                                {
                                    if( aInstance.dbId != *objectId )
                                        return false;

                                    return std::any_of(
                                            aInstance.pins.begin(), aInstance.pins.end(),
                                            [&]( const ORCAD_PIN_INST& aPin )
                                            {
                                                if( aPin.wordB == netId )
                                                    return true;

                                                return std::any_of(
                                                        aPage.wires.begin(), aPage.wires.end(),
                                                        [&]( const ORCAD_WIRE& aWire )
                                                        {
                                                            return aWire.id == netId
                                                                   && ( aPin.wordA == aWire.dbId
                                                                        || rawPointOnSegment( aPin.x, aPin.y,
                                                                                              aWire ) );
                                                        } );
                                            } );
                                } );
                    }

                    if( occurrenceKey == localKey
                        || ( !implicitGenerated
                             && ( !isOffpageNetName( candidateLocalName ) || occurrenceOwnedByNet )
                             && !isPowerNetName( candidateLocalName ) && occurrenceKey.size() > localKey.size()
                             && occurrenceKey[localKey.size()] == '_'
                             && occurrenceKey.compare( 0, localKey.size(), localKey ) == 0 ) )
                    {
                        candidates.insert( &occurrenceName );

                        if( occurrenceOwnedByNet && !directOffpage )
                        {
                            ownedCandidates.insert( &occurrenceName );
                            localizedOwnedCandidate |= !sharedFlatFolder && isOffpageNetName( candidateLocalName );
                        }
                    }
                }
            }

            if( ownedCandidates.size() == 1 )
            {
                occurrenceNamesByNetId[netId] = *ownedCandidates.begin();
                if( localizedOwnedCandidate )
                    localizedOwnedOccurrenceNetIds.insert( netId );
            }
            else if( candidates.size() == 1 )
            {
                occurrenceNamesByNetId[netId] = *candidates.begin();

                const std::string* selected = *candidates.begin();
                std::string        selectedKey = lower( kicadOccurrenceNetName( *selected ) );
                bool localOffpage = std::any_of( localNames.begin(), localNames.end(),
                                                 [&]( const std::string& aName )
                                                 {
                                                     return isOffpageNetName( aName )
                                                            && lower( aName ) == selectedKey;
                                                 } );
                bool competingSuffixedOccurrence = std::any_of(
                        m_currentOccNetNames->begin(), m_currentOccNetNames->end(),
                        [&]( const auto& aOccurrence )
                        {
                            std::string key = lower( kicadOccurrenceNetName( aOccurrence.second ) );
                            return &aOccurrence.second != selected && key.size() > selectedKey.size()
                                   && key[selectedKey.size()] == '_'
                                   && key.compare( 0, selectedKey.size(), selectedKey ) == 0
                                   && occurrenceNetObjectId( aOccurrence.second ).has_value();
                        } );

                if( localOffpage && !directOffpage && competingSuffixedOccurrence )
                {
                    std::optional<uint32_t> componentId;

                    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
                    {
                        bool connected = std::any_of(
                                instance.pins.begin(), instance.pins.end(),
                                [&]( const ORCAD_PIN_INST& aPin )
                                {
                                    if( aPin.wordB == netId )
                                        return true;

                                    return std::any_of( aPage.wires.begin(), aPage.wires.end(),
                                                        [&]( const ORCAD_WIRE& aWire )
                                                        {
                                                            return aWire.id == netId
                                                                   && ( aPin.wordA == aWire.dbId
                                                                        || rawPointOnSegment( aPin.x, aPin.y,
                                                                                              aWire ) );
                                                        } );
                                } );

                        if( connected && ( !componentId || instance.dbId < *componentId ) )
                            componentId = instance.dbId;
                    }

                    if( componentId )
                    {
                        occurrenceElectricalNames[selected] = kicadOccurrenceNetName( *selected ) + "_"
                                                              + std::to_string( *componentId );

                        if( !sharedFlatFolder )
                            localizedOwnedOccurrenceNetIds.insert( netId );
                    }
                }
            }
        }

        for( const auto& [occurrenceId, name] : *m_currentOccNetNames )
        {
            occurrenceNames.insert( name );

            if( std::optional<uint32_t> objectId = occurrenceNetObjectId( name ) )
            {
                auto netId = netIdByWireObjectId.find( *objectId );

                if( netId != netIdByWireObjectId.end() )
                    objectNamesByNetId[netId->second].insert( &name );
            }

            if( name.size() < 2 || name[0] != 'N' )
                continue;

            uint64_t objectId = 0;

            auto it = name.begin() + 1;

            for( ; it != name.end() && std::isdigit( static_cast<unsigned char>( *it ) ); ++it )
            {
                objectId = objectId * 10 + static_cast<unsigned>( *it - '0' );
            }

            if( objectId && objectId <= std::numeric_limits<uint32_t>::max() && ( it == name.end() || *it == '_' ) )
            {
                generatedNamesByObjectId[static_cast<uint32_t>( objectId )] = &name;
            }
        }

        for( const auto& [netId, names] : objectNamesByNetId )
        {
            if( names.size() == 1 )
                occurrenceNamesByNetId[netId] = *names.begin();
        }

        for( const auto& [netId, occurrenceName] : occurrenceNamesByNetId )
        {
            std::string electricalName = occurrenceElectricalNames.at( occurrenceName );
            bool        collides = std::any_of( aPage.netmap.begin(), aPage.netmap.end(),
                                                [&]( const auto& aPageNet )
                                                {
                                                    return aPageNet.first != netId
                                                           && wxString::FromUTF8(
                                                                      kicadElectricalNetName( aPageNet.second ) )
                                                                      .CmpNoCase( wxString::FromUTF8( electricalName ) )
                                                                      == 0;
                                                } );

            if( !collides )
                continue;

            auto occurrence = std::find_if( m_currentOccNetNames->begin(), m_currentOccNetNames->end(),
                                            [&]( const auto& aEntry )
                                            {
                                                return &aEntry.second == occurrenceName;
                                            } );

            if( occurrence == m_currentOccNetNames->end() )
                continue;

            std::string suffix = std::to_string( occurrence->first );

            if( suffix.size() < 6 )
                suffix.insert( 0, 6 - suffix.size(), '0' );

            occurrenceElectricalNames[occurrenceName] += "_" + suffix;
        }
    }

    std::map<uint32_t, const std::string*>  netNamesById;
    std::map<uint32_t, std::vector<size_t>> wiresByNetId;
    std::set<uint32_t>                      ambiguousNetIds;

    for( const auto& [netId, name] : aPage.netmap )
    {
        auto                  aliases = aPage.netAliases.find( netId );
        std::set<std::string> distinctNames;

        if( aliases != aPage.netAliases.end() )
        {
            for( const std::string& alias : aliases->second )
            {
                if( !alias.empty() )
                    distinctNames.insert( alias );
            }
        }

        if( distinctNames.size() > 1 )
        {
            ambiguousNetIds.insert( netId );

            if( isPowerNetName( name ) )
                netNamesById[netId] = &name;

            continue;
        }

        bool explicitAlias =
                aliases != aPage.netAliases.end()
                && std::find( aliases->second.begin(), aliases->second.end(), name ) != aliases->second.end();

        if( !name.empty() && ( !isImplicitGeneratedName( name ) || explicitAlias ) )
            netNamesById[netId] = &name;
    }

    for( const auto& [netId, name] : occurrenceNamesByNetId )
        netNamesById[netId] = name;

    for( const auto& [netId, occurrenceName] : occurrenceNamesByNetId )
    {
        std::string occurrenceKey = kicadOccurrenceNetName( *occurrenceName );
        std::transform( occurrenceKey.begin(), occurrenceKey.end(), occurrenceKey.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );
        auto occurrenceDepth = m_occurrenceNetNameMinDepth.find( occurrenceKey );

        if( occurrenceDepth == m_occurrenceNetNameMinDepth.end() )
            continue;

        std::map<size_t, std::set<std::string>> candidatesByDepth;
        std::set<std::string>                   sourceNames;
        auto                                    pageName = aPage.netmap.find( netId );

        if( pageName != aPage.netmap.end() && !pageName->second.empty() )
            sourceNames.insert( canonicalGlobalNetName( pageName->second ) );

        auto aliases = aPage.netAliases.find( netId );

        if( aliases != aPage.netAliases.end() )
        {
            for( const std::string& alias : aliases->second )
                sourceNames.insert( canonicalGlobalNetName( alias ) );
        }

        for( const std::string& sourceName : sourceNames )
        {
            std::string sourceKey = sourceName;
            std::transform( sourceKey.begin(), sourceKey.end(), sourceKey.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );
            auto sourceDepth = m_occurrenceNetNameMinDepth.find( sourceKey );

            if( sourceDepth != m_occurrenceNetNameMinDepth.end() && sourceDepth->second < occurrenceDepth->second )
                candidatesByDepth[sourceDepth->second].insert( sourceName );
        }

        if( !candidatesByDepth.empty() && candidatesByDepth.begin()->second.size() == 1 )
        {
            std::string& flattened = flattenedNamesByNetId[netId];
            flattened = *candidatesByDepth.begin()->second.begin();
            netNamesById[netId] = &flattened;
            renamedNetIds.insert( netId );
        }
    }

    std::map<std::string, std::set<uint32_t>> blockNetIdsByName;

    for( const auto& [netId, name] : aPage.netmap )
    {
        if( !name.empty() && !isImplicitGeneratedName( name ) )
            blockNetIdsByName[canonicalGlobalNetName( name )].insert( netId );
    }

    for( const auto& [netId, names] : blockPinNamesByNetId )
    {
        if( !names.empty() )
            blockNetIdsByName[*names.begin()].insert( netId );
    }

    std::map<uint32_t, std::string> blockElectricalNames;
    std::set<uint32_t>              blockElectricalNetIds;

    for( const auto& [netId, names] : blockPinNamesByNetId )
    {
        auto currentName = netNamesById.find( netId );

        if( !names.empty() && ( currentName == netNamesById.end() || isImplicitGeneratedName( *currentName->second ) ) )
        {
            std::string name = *names.begin();

            if( blockNetIdsByName.at( name ).size() > 1 )
            {
                std::string suffix = std::to_string( netId );

                if( suffix.size() < 6 )
                    suffix.insert( 0, 6 - suffix.size(), '0' );

                name += "_" + suffix;
            }

            auto inserted = blockElectricalNames.emplace( netId, std::move( name ) ).first;
            netNamesById[netId] = &inserted->second;
            blockElectricalNetIds.insert( netId );
        }
    }

    if( !m_currentUnconnectedInterfaceNetNames.empty() )
    {
        for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
        {
            std::string name = canonicalGlobalNetName( port.logicalName.empty() ? port.name : port.logicalName );
            std::string key = name;
            std::transform( key.begin(), key.end(), key.begin(),
                            []( unsigned char c )
                            {
                                return static_cast<char>( std::tolower( c ) );
                            } );

            auto targetName = m_currentUnconnectedInterfaceNetNames.find( key );

            if( targetName == m_currentUnconnectedInterfaceNetNames.end() )
                continue;

            VECTOR2I pin = namedGraphicPinPos( aPage, port );

            for( const ORCAD_WIRE& wire : aPage.wires )
            {
                if( wire.isBus || !pointOnWire( pin, wire ) )
                    continue;

                std::string& flattened = flattenedNamesByNetId[wire.id];
                flattened = targetName->second;
                netNamesById[wire.id] = &flattened;
                renamedNetIds.insert( wire.id );
            }
        }
    }

    for( const ORCAD_WIRE& wire : aPage.wires )
    {
        auto name = netNamesById.find( wire.id );

        if( name != netNamesById.end() )
            netNamesByWireObjectId[wire.dbId] = name->second;
    }

    std::set<std::string>                     interfaceNames;
    std::set<uint32_t>                        interfaceNetIds;
    std::map<uint32_t, std::set<std::string>> globalInterfaceNamesByNetId;

    auto addInterfacePoint = [&]( const VECTOR2I& aPin, const std::string* aGlobalName )
    {
        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            bool endpoint = ( aPin.x == wire.x1 && aPin.y == wire.y1 ) || ( aPin.x == wire.x2 && aPin.y == wire.y2 );

            if( !wire.isBus && ( endpoint || onSegment( aPin.x, aPin.y, wire ) ) )
            {
                interfaceNetIds.insert( wire.id );

                if( aGlobalName && !aGlobalName->empty() )
                    globalInterfaceNamesByNetId[wire.id].insert( *aGlobalName );
            }
        }
    };

    auto addInterfaceName = [&]( const ORCAD_GRAPHIC_INST& aInstance, bool aGlobal, bool aPower )
    {
        std::string name = aInstance.logicalName.empty() ? aInstance.name : aInstance.logicalName;

        if( aPower )
            name = powerNet( aPage, aInstance );
        else if( aGlobal )
            name = effectiveInterfaceNetName( name );

        if( !name.empty() )
            interfaceNames.insert( name );

        VECTOR2I pin = aPower ? powerPinPos( aPage, aInstance ) : namedGraphicPinPos( aPage, aInstance );
        addInterfacePoint( pin, aGlobal ? &name : nullptr );
    };

    for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
        addInterfaceName( port, false, false );

    for( const ORCAD_GRAPHIC_INST& connector : aPage.offpage )
        addInterfaceName( connector, !aHierarchical, false );

    for( const ORCAD_GRAPHIC_INST& global : aPage.globals )
        addInterfaceName( global, true, true );

    for( const auto& [netId, name] : occurrenceNamesByNetId )
    {
        auto selectedName = netNamesById.find( netId );

        if( selectedName == netNamesById.end() || selectedName->second != name )
            continue;

        std::string sourceName = kicadOccurrenceNetName( *name );

        std::transform( sourceName.begin(), sourceName.end(), sourceName.begin(),
                        []( unsigned char c )
                        {
                            return static_cast<char>( std::tolower( c ) );
                        } );

        if( !sharedFlatFolder && ( m_scopeNamedFlatNets || m_scopeGeneratedFlatNets || aNestedHierarchy )
            && !m_currentInterfaceNetAliases.count( sourceName )
            && m_occurrenceNetNameScopeCounts[sourceName] > 1 )
        {
            continue;
        }

        if( !aNestedHierarchy
            && ( localizedOwnedOccurrenceNetIds.count( netId )
                 || !m_currentConnectorInterfaceNetAliases.count( sourceName ) ) )
        {
            globalInterfaceNamesByNetId[netId].clear();
            globalInterfaceNamesByNetId[netId].insert( occurrenceElectricalName( name ) );
        }

    }

    for( const ORCAD_DRAWN_INSTANCE& block : aPage.blocks )
    {
        for( const ORCAD_BLOCK_PIN& pin : block.pins )
            addInterfacePoint( VECTOR2I( pin.x, pin.y ), nullptr );
    }

    if( m_scopeNamedFlatNets && !m_currentFlatNetSuffix.empty() )
    {
        for( const auto& [netId, name] : aPage.netmap )
        {
            if( name.empty() || interfaceNetIds.count( netId ) || interfaceNames.count( name )
                || isOffpageNetName( name ) )
                continue;

            auto selected = netNamesById.find( netId );

            if( selected == netNamesById.end() || *selected->second != name )
                continue;

            std::string& flattened = flattenedNamesByNetId[netId];
            flattened = name + "_" + m_currentFlatNetSuffix;
            netNamesById[netId] = &flattened;
            renamedNetIds.insert( netId );
        }
    }

    for( size_t i = 0; i < aPage.wires.size(); ++i )
    {
        const ORCAD_WIRE& wire = aPage.wires[i];

        if( wire.isBus )
            continue;

        wiresByNetId[wire.id].push_back( i );

        auto generatedName = generatedNamesByObjectId.find( wire.dbId );

        if( generatedName != generatedNamesByObjectId.end() )
        {
            if( m_scopeGeneratedFlatNets && !m_currentFlatNetSuffix.empty() && !interfaceNetIds.count( wire.id ) )
            {
                std::string& flattened = flattenedNamesByNetId[wire.id];
                flattened = *generatedName->second + "_" + m_currentFlatNetSuffix;
                netNamesById[wire.id] = &flattened;
                renamedNetIds.insert( wire.id );
            }
        }
    }

    std::map<int, std::vector<size_t>>      horizontalWires;
    std::map<int, std::vector<size_t>>      verticalWires;
    std::map<size_t, std::vector<VECTOR2I>> cutsByWire;

    for( size_t i = 0; i < aPage.wires.size(); ++i )
    {
        const ORCAD_WIRE& wire = aPage.wires[i];

        if( wire.isBus )
            continue;

        if( wire.y1 == wire.y2 )
            horizontalWires[wire.y1].push_back( i );
        else if( wire.x1 == wire.x2 )
            verticalWires[wire.x1].push_back( i );
    }

    for( const auto& [y, horizontalIndices] : horizontalWires )
    {
        for( size_t horizontalIndex : horizontalIndices )
        {
            const ORCAD_WIRE& horizontal = aPage.wires[horizontalIndex];
            int               minX = std::min( horizontal.x1, horizontal.x2 );
            int               maxX = std::max( horizontal.x1, horizontal.x2 );

            for( auto verticalIt = verticalWires.lower_bound( minX );
                 verticalIt != verticalWires.end() && verticalIt->first <= maxX; ++verticalIt )
            {
                for( size_t verticalIndex : verticalIt->second )
                {
                    const ORCAD_WIRE& vertical = aPage.wires[verticalIndex];

                    if( vertical.id == horizontal.id || !onSegment( vertical.x1, y, horizontal )
                        || !onSegment( vertical.x1, y, vertical ) )
                    {
                        continue;
                    }

                    // Keep nets separate at ambiguous crossings. A graphic fills the gap; labels reconnect each net.
                    size_t cutIndex = netNamesById.count( vertical.id ) ? verticalIndex : horizontalIndex;

                    if( netNamesById.count( aPage.wires[cutIndex].id ) )
                        cutsByWire[cutIndex].emplace_back( vertical.x1, y );
                }
            }
        }
    }

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        for( const ORCAD_PIN_INST& pin : instance.pins )
        {
            if( pin.wordA || pin.wordB )
                continue;

            for( size_t wireIndex = 0; wireIndex < aPage.wires.size(); ++wireIndex )
            {
                const ORCAD_WIRE& wire = aPage.wires[wireIndex];

                if( !wire.isBus && netNamesById.count( wire.id ) && onSegment( pin.x, pin.y, wire ) )
                    cutsByWire[wireIndex].emplace_back( pin.x, pin.y );
            }
        }
    }

    std::set<size_t> labelWireIndices;

    for( const auto& [netId, wireIndices] : wiresByNetId )
    {
        if( !netNamesById.count( netId ) && globalInterfaceNamesByNetId[netId].size() != 1 )
            continue;

        std::set<size_t> unseen( wireIndices.begin(), wireIndices.end() );

        while( !unseen.empty() )
        {
            size_t              representative = *unseen.begin();
            std::vector<size_t> pending = { representative };
            unseen.erase( representative );

            while( !pending.empty() )
            {
                const ORCAD_WIRE& current = aPage.wires[pending.back()];
                pending.pop_back();

                for( auto it = unseen.begin(); it != unseen.end(); )
                {
                    const ORCAD_WIRE& candidate = aPage.wires[*it];
                    bool              touches = onSegment( current.x1, current.y1, candidate )
                                   || onSegment( current.x2, current.y2, candidate )
                                   || onSegment( candidate.x1, candidate.y1, current )
                                   || onSegment( candidate.x2, candidate.y2, current );

                    if( touches )
                    {
                        pending.push_back( *it );
                        it = unseen.erase( it );
                    }
                    else
                    {
                        ++it;
                    }
                }
            }

            labelWireIndices.insert( representative );
        }
    }

    for( size_t wireIndex = 0; wireIndex < aPage.wires.size(); ++wireIndex )
    {
        const ORCAD_WIRE& wire = aPage.wires[wireIndex];
        auto              appendSegment = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd, int aLayer )
        {
            if( aStart == aEnd )
                return;

            SCH_LINE* line = new SCH_LINE( OrcadDbuToIu( aStart.x, aStart.y ), aLayer );
            line->SetEndPoint( OrcadDbuToIu( aEnd.x, aEnd.y ) );
            line->SetLineWidth( OrcadLineWidthIu( wire.lineWidth ) );
            line->SetLineStyle( OrcadLineStyle( wire.lineStyle ) );
            line->SetLineColor( OrcadColor( wire.color ) );
            appendPageItem( aScreen, line );
        };

        std::vector<VECTOR2I> cuts = cutsByWire[wireIndex];

        int64_t dx = static_cast<int64_t>( wire.x2 ) - wire.x1;
        int64_t dy = static_cast<int64_t>( wire.y2 ) - wire.y1;

        std::sort( cuts.begin(), cuts.end(),
                   [&]( const VECTOR2I& a, const VECTOR2I& b )
                   {
                       int64_t ta = ( static_cast<int64_t>( a.x ) - wire.x1 ) * dx
                                    + ( static_cast<int64_t>( a.y ) - wire.y1 ) * dy;
                       int64_t tb = ( static_cast<int64_t>( b.x ) - wire.x1 ) * dx
                                    + ( static_cast<int64_t>( b.y ) - wire.y1 ) * dy;
                       return ta < tb;
                   } );
        cuts.erase( std::unique( cuts.begin(), cuts.end() ), cuts.end() );

        VECTOR2I step( dx == 0 ? 0 : ( dx > 0 ? 1 : -1 ), dy == 0 ? 0 : ( dy > 0 ? 1 : -1 ) );
        VECTOR2I cursor( wire.x1, wire.y1 );

        auto netName = netNamesById.find( wire.id );

        auto segmentMidpoint = []( const VECTOR2I& aStart, const VECTOR2I& aEnd )
        {
            return VECTOR2I( aStart.x + ( aEnd.x - aStart.x ) / 2, aStart.y + ( aEnd.y - aStart.y ) / 2 );
        };
        auto transparentLabelPosition = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd )
        {
            VECTOR2I midpoint = segmentMidpoint( aStart, aEnd );
            VECTOR2I direction( aEnd.x == aStart.x ? 0 : ( aEnd.x > aStart.x ? 1 : -1 ),
                                aEnd.y == aStart.y ? 0 : ( aEnd.y > aStart.y ? 1 : -1 ) );
            std::vector<VECTOR2I> candidates = { midpoint, aStart + direction, aEnd - direction };

            for( int numerator : { 1, 3, 2, 6 } )
            {
                constexpr int denominator = 8;
                candidates.emplace_back( aStart.x + ( aEnd.x - aStart.x ) * numerator / denominator,
                                         aStart.y + ( aEnd.y - aStart.y ) * numerator / denominator );
            }

            for( const VECTOR2I& candidate : candidates )
            {
                if( candidate == aStart || candidate == aEnd || !onSegment( candidate.x, candidate.y, wire ) )
                    continue;

                bool crossesAnotherNet =
                        std::any_of( aPage.wires.begin(), aPage.wires.end(),
                                     [&]( const ORCAD_WIRE& aOther )
                                     {
                                         return &aOther != &wire
                                                && ( aOther.isBus != wire.isBus || aOther.id != wire.id )
                                                && onSegment( candidate.x, candidate.y, aOther );
                                     } );

                if( !crossesAnotherNet )
                    return candidate;
            }

            return midpoint;
        };
        auto appendTransparentLabels = [&]( const VECTOR2I& aPosition )
        {
            std::set<std::string> names;
            size_t                wireGroup = wireRoot( wireIndex );
            bool                  ambiguousPhysicalNet = ambiguousNetIds.count( wire.id );
            auto                  resolvedPower = resolvedPowerNamesByWireGroup.find( wireGroup );

            if( ambiguousPhysicalNet && resolvedPower != resolvedPowerNamesByWireGroup.end() )
                names.insert( resolvedPower->second.begin(), resolvedPower->second.end() );
            else if( ambiguousPhysicalNet )
            {
                auto netPower = resolvedPowerNamesByNetId.find( wire.id );

                if( netPower != resolvedPowerNamesByNetId.end() && netPower->second.size() == 1 )
                    names.insert( *netPower->second.begin() );
            }

            auto aliases = aPage.netAliases.find( wire.id );

            if( aliases != aPage.netAliases.end() && !ambiguousPhysicalNet && !occurrenceNamesByNetId.count( wire.id ) )
            {
                for( const std::string& alias : aliases->second )
                    names.insert( alias );
            }

            auto powerAliases = powerAliasesByWireGroup.find( wireGroup );

            bool joinedPowerAliases = !ambiguousPhysicalNet && powerAliases != powerAliasesByWireGroup.end()
                                      && powerAliases->second.size() > 1
                                      && powerGlobalNamesByWireGroup[wireGroup].size() > 1;

            if( joinedPowerAliases )
                names.insert( powerAliases->second.begin(), powerAliases->second.end() );

            if( ambiguousPhysicalNet && occurrenceNamesByNetId.count( wire.id ) )
            {
                names.insert( *occurrenceNamesByNetId.at( wire.id ) );
            }
            else if( !ambiguousPhysicalNet && netName != netNamesById.end() )
            {
                std::string key = *netName->second;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );

                if( !m_currentConnectorInterfaceNetAliases.count( key ) )
                    names.insert( *netName->second );
            }

            if( !ambiguousPhysicalNet )
                names.insert( globalInterfaceNamesByNetId[wire.id].begin(), globalInterfaceNamesByNetId[wire.id].end() );
            else if( globalInterfaceNamesByNetId[wire.id].size() == 1 )
                names.insert( *globalInterfaceNamesByNetId[wire.id].begin() );

            if( hierBusNames )
            {
                std::vector<std::string> sourceNames( names.begin(), names.end() );

                for( const std::string& name : sourceNames )
                    names.insert( scopedHierBusMember( name, *hierBusNames ) );
            }

            for( const std::string& name : names )
            {
                if( name.empty() )
                    continue;

                SCH_LABEL_BASE* label;
                bool occurrenceName = occurrenceNamesByNetId.count( wire.id )
                                      && name == *occurrenceNamesByNetId.at( wire.id );
                bool renamedBusMember = hierBusNames && scopedHierBusMember( name, *hierBusNames ) != name;
                std::string sourceInterfaceKey = aPage.netmap.count( wire.id ) ? aPage.netmap.at( wire.id )
                                                                               : std::string();
                std::transform( sourceInterfaceKey.begin(), sourceInterfaceKey.end(), sourceInterfaceKey.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );
                auto interfaceAlias = m_currentInterfaceNetAliases.find( sourceInterfaceKey );
                bool scopedHierarchicalInterface =
                        aHierarchical && interfaceNetIds.count( wire.id )
                        && interfaceAlias != m_currentInterfaceNetAliases.end()
                        && wxString::FromUTF8( interfaceAlias->second ).CmpNoCase( wxString::FromUTF8( sourceInterfaceKey ) )
                                   != 0;
                wxString electricalName = FromOrcadString(
                        occurrenceName ? occurrenceElectricalName( occurrenceNamesByNetId.at( wire.id ) )
                                       : kicadElectricalNetName( name ) );

                if( blockElectricalNetIds.count( wire.id ) && netName != netNamesById.end()
                    && name == *netName->second )
                {
                    label = new SCH_LABEL( OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName );
                }
                else if( joinedPowerAliases && powerAliases->second.count( name )
                         && !scopedHierarchicalInterface )
                {
                    label = new SCH_GLOBALLABEL( OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName );
                }
                else if( renamedNetIds.count( wire.id ) && netName != netNamesById.end() && name == *netName->second )
                {
                    label = scopedHierarchicalInterface
                                    ? static_cast<SCH_LABEL_BASE*>(
                                              new SCH_LABEL( OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName ) )
                                    : static_cast<SCH_LABEL_BASE*>( new SCH_GLOBALLABEL(
                                              OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName ) );
                }
                else if( globalInterfaceNamesByNetId[wire.id].count( name ) && !renamedBusMember
                         && !scopedHierarchicalInterface )
                {
                    label = new SCH_GLOBALLABEL( OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName );
                }
                else
                    label = new SCH_LABEL( OrcadDbuToIu( aPosition.x, aPosition.y ), electricalName );

                label->SetTextColor( invisibleElectricalTextColor() );
                appendPageItem( aScreen, label );
            }
        };

        for( const VECTOR2I& cut : cuts )
        {
            VECTOR2I before = cut - step;
            VECTOR2I after = cut + step;

            if( !onSegment( before.x, before.y, wire ) )
                before = cut;

            if( !onSegment( after.x, after.y, wire ) )
                after = cut;

            appendSegment( cursor, before, wire.isBus ? LAYER_BUS : LAYER_WIRE );

            if( cursor != before )
                appendTransparentLabels( transparentLabelPosition( cursor, before ) );

            appendSegment( before, after, LAYER_NOTES );
            cursor = after;
        }

        VECTOR2I end( wire.x2, wire.y2 );
        appendSegment( cursor, end, wire.isBus ? LAYER_BUS : LAYER_WIRE );

        if( !cuts.empty() && cursor != end )
            appendTransparentLabels( transparentLabelPosition( cursor, end ) );

        for( const ORCAD_ALIAS& alias : wire.aliases )
        {
            wxString text = FromOrcadString( kicadBusName( trimmed( alias.name ) ) );
            wxString electricalText = FromOrcadString( kicadElectricalNetName( trimmed( alias.name ) ) );

            if( text.IsEmpty() )
                continue;

            VECTOR2I anchor = snapToWire( alias.x, alias.y, wire );
            // Quadrants 2/3 fold to 0/90 so text reads upright.
            int  quadrant = alias.rotation & 3;
            int  fontId = wireAliasFontId( alias );
            bool electrical = !( ambiguousNetIds.count( wire.id ) && netName != netNamesById.end()
                                 && isPowerNetName( *netName->second )
                                 && canonicalGlobalNetName( alias.name ) != *netName->second );

            if( occurrenceNamesByNetId.count( wire.id ) && netName != netNamesById.end()
                && canonicalGlobalNetName( alias.name ) != canonicalGlobalNetName( *netName->second ) )
            {
                electrical = false;
            }

            VECTOR2I electricalAnchor = anchor;
            bool     interiorCrossing = anchor != VECTOR2I( wire.x1, wire.y1 ) && anchor != VECTOR2I( wire.x2, wire.y2 )
                                    && std::any_of( aPage.wires.begin(), aPage.wires.end(),
                                                    [&]( const ORCAD_WIRE& aOther )
                                                    {
                                                        return !aOther.isBus && aOther.id != wire.id
                                                               && anchor != VECTOR2I( aOther.x1, aOther.y1 )
                                                               && anchor != VECTOR2I( aOther.x2, aOther.y2 )
                                                               && onSegment( anchor.x, anchor.y, aOther );
                                                    } );

            if( interiorCrossing && std::find( cuts.begin(), cuts.end(), anchor ) != cuts.end() )
            {
                VECTOR2I candidate = anchor - step;

                if( candidate == anchor || !onSegment( candidate.x, candidate.y, wire ) )
                    candidate = anchor + step;

                electricalAnchor = candidate;
            }

            if( electrical && ( electricalText != text || electricalAnchor != anchor ) )
            {
                SCH_LABEL* label =
                        new SCH_LABEL( OrcadDbuToIu( electricalAnchor.x, electricalAnchor.y ), electricalText );
                label->SetTextColor( invisibleElectricalTextColor() );
                appendPageItem( aScreen, label );
                electrical = false;
            }

            if( electrical )
            {
                SCH_LABEL* label = new SCH_LABEL( OrcadDbuToIu( anchor.x, anchor.y ), electricalText );
                label->SetSpinStyle( ( quadrant == 1 || quadrant == 3 ) ? SPIN_STYLE::UP : SPIN_STYLE::RIGHT );
                label->SetTextSize( textSize( fontId ) );
                applyFont( label, fontId );
                label->SetTextColor( OrcadColor( alias.color ) );
                appendPageItem( aScreen, label );
            }
            else
            {
                SCH_TEXT* note = new SCH_TEXT( OrcadDbuToIu( anchor.x, anchor.y ), text, LAYER_NOTES );
                note->SetTextAngle( ( quadrant == 1 || quadrant == 3 ) ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
                note->SetTextSize( textSize( fontId ) );
                note->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
                note->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
                applyFont( note, fontId );
                note->SetTextColor( OrcadColor( alias.color ) );
                appendPageItem( aScreen, note );
            }
        }

        size_t wireGroup = wireRoot( wireIndex );
        bool   hasPowerAliases = powerAliasesByWireGroup.count( wireGroup )
                               && powerAliasesByWireGroup.at( wireGroup ).size() > 1
                               && powerGlobalNamesByWireGroup[wireGroup].size() > 1;

        bool authoritativeInterface = globalInterfaceNamesByNetId[wire.id].size() == 1;

        if( cuts.empty()
            && ( hasPowerAliases
                 || ( labelWireIndices.count( wireIndex )
                      && ( netName != netNamesById.end() || authoritativeInterface ) ) ) )
        {
            appendTransparentLabels( transparentLabelPosition( VECTOR2I( wire.x1, wire.y1 ), end ) );
        }

    }

    auto pinTouchesWire = [&aPage]( const ORCAD_PIN_INST& aPin )
    {
        return std::any_of( aPage.wires.begin(), aPage.wires.end(),
                            [&aPin]( const ORCAD_WIRE& aWire )
                            {
                                return ( aPin.x == aWire.x1 && aPin.y == aWire.y1 )
                                       || ( aPin.x == aWire.x2 && aPin.y == aWire.y2 )
                                       || onSegment( aPin.x, aPin.y, aWire );
                            } );
    };

    std::map<std::pair<int, int>, std::string> generatedWirelessNets;

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        for( size_t pinIndex = 0; pinIndex < instance.pins.size(); ++pinIndex )
        {
            const ORCAD_PIN_INST& pin = instance.pins[pinIndex];
            VECTOR2I              electricalPosition = placedPinElectricalPosition( instance, pinIndex );

            if( ( pin.wordA || pin.wordB )
                && ( electricalPosition != VECTOR2I( pin.x, pin.y ) || !pinTouchesWire( pin ) ) )
            {
                std::string generatedName = "N" + std::to_string( instance.dbId ) + std::to_string( pinIndex );

                if( occurrenceNames.count( generatedName ) )
                    generatedWirelessNets[{ electricalPosition.x, electricalPosition.y }] = std::move( generatedName );
            }
        }
    }

    for( const ORCAD_PLACED_INSTANCE& instance : aPage.instances )
    {
        const ORCAD_SYMBOL_DEF* definition = pickVariant( instance ).first;

        for( size_t pinIndex = 0; pinIndex < instance.pins.size(); ++pinIndex )
        {
            const ORCAD_PIN_INST& pin = instance.pins[pinIndex];
            VECTOR2I              electricalPosition = placedPinElectricalPosition( instance, pinIndex );

            if( !pin.wordA && !pin.wordB )
                continue;

            std::string generatedName = "N" + std::to_string( instance.dbId ) + std::to_string( pinIndex );
            std::string name;
            bool        globalName = false;

            if( electricalPosition == VECTOR2I( pin.x, pin.y ) && pinTouchesWire( pin ) )
                continue;

            if( occurrenceNames.count( generatedName ) )
                name = std::move( generatedName );
            else if( generatedWirelessNets.count( { electricalPosition.x, electricalPosition.y } ) )
                name = generatedWirelessNets.at( { electricalPosition.x, electricalPosition.y } );
            else if( pin.wordA && netNamesByWireObjectId.count( pin.wordA ) )
                name = *netNamesByWireObjectId.at( pin.wordA );
            else if( pin.wordA && netNamesByObjectId.count( pin.wordA ) )
                name = netNamesByObjectId.at( pin.wordA );
            else if( pin.wordA && connectorNamesByObjectId.count( pin.wordA ) )
            {
                name = connectorNamesByObjectId.at( pin.wordA );
                globalName = globalConnectorObjectIds.count( pin.wordA );
            }
            else if( pin.wordB )
            {
                auto occurrenceName = occurrenceNamesByNetId.find( pin.wordB );
                auto pageName = aPage.netmap.find( pin.wordB );

                if( occurrenceName != occurrenceNamesByNetId.end() )
                {
                    name = occurrenceElectricalName( occurrenceName->second );
                    std::string sourceName = kicadOccurrenceNetName( *occurrenceName->second );
                    std::string key = sourceName;
                    std::transform( key.begin(), key.end(), key.begin(),
                                    []( unsigned char c )
                                    {
                                        return static_cast<char>( std::tolower( c ) );
                                    } );
                    globalName = !localizedOwnedOccurrenceNetIds.count( pin.wordB )
                                 && ( sharedFlatFolder
                                      || !( m_scopeNamedFlatNets || m_scopeGeneratedFlatNets || aNestedHierarchy )
                                      || m_currentInterfaceNetAliases.count( key )
                                      || m_occurrenceNetNameScopeCounts[key] == 1 );

                }
                else if( !ambiguousNetIds.count( pin.wordB ) && pageName != aPage.netmap.end() )
                    name = pageName->second;
            }

            if( name.empty() && connectorNamesByPosition[{ pin.x, pin.y }].size() == 1 )
            {
                name = *connectorNamesByPosition.at( { pin.x, pin.y } ).begin();
                globalName = globalConnectorPositions.count( { pin.x, pin.y } );
            }

            bool stackedPeer = std::any_of( instance.pins.begin(), instance.pins.end(),
                                            [&]( const ORCAD_PIN_INST& aPeer )
                                            {
                                                return &aPeer != &pin && aPeer.x == pin.x && aPeer.y == pin.y;
                                            } );
            bool sameNetPeer = std::any_of( instance.pins.begin(), instance.pins.end(),
                                            [&]( const ORCAD_PIN_INST& aPeer )
                                            {
                                                return &aPeer != &pin && aPeer.x == pin.x && aPeer.y == pin.y
                                                       && ( ( pin.wordA && pin.wordA == aPeer.wordA )
                                                            || ( pin.wordB && pin.wordB == aPeer.wordB ) );
                                            } );

            if( name.empty() && ( !stackedPeer || sameNetPeer ) && electricalPosition != VECTOR2I( pin.x, pin.y )
                && pinTouchesWire( pin ) )
            {
                name = netAt( aPage, pin.x, pin.y );
            }

            if( name.empty() && sameNetPeer && electricalPosition != VECTOR2I( pin.x, pin.y ) )
            {
                auto sourceNet = generatedWirelessNets.find( { pin.x, pin.y } );

                if( sourceNet != generatedWirelessNets.end() )
                    name = sourceNet->second;
            }

            if( name.empty() && !generatedWirelessNets.count( { electricalPosition.x, electricalPosition.y } )
                && definition && pin.pinIndex > 0 && static_cast<size_t>( pin.pinIndex ) <= definition->pins.size() )
            {
                std::string sourceName = trimmed( definition->pins[pin.pinIndex - 1].name );

                if( occurrenceNames.count( sourceName ) )
                    name = std::move( sourceName );
            }

            if( name.empty() )
                continue;

            std::string scopedName = name;

            if( hierBusNames )
                scopedName = scopedHierBusMember( name, *hierBusNames );

            SCH_LABEL_BASE* label;

            if( scopedName == name
                && ( globalName || name.rfind( "$$$", 0 ) == 0 || ( !aHierarchical && isOffpageNetName( name ) )
                     || isPowerNetName( name ) ) )
            {
                label = new SCH_GLOBALLABEL( OrcadDbuToIu( electricalPosition.x, electricalPosition.y ),
                                             FromOrcadString( name ) );
            }
            else
                label = new SCH_LABEL( OrcadDbuToIu( electricalPosition.x, electricalPosition.y ),
                                       FromOrcadString( name ) );

            label->SetTextColor( invisibleElectricalTextColor() );
            appendPageItem( aScreen, label );

            if( scopedName != name )
            {
                SCH_LABEL* scopedLabel = new SCH_LABEL( OrcadDbuToIu( electricalPosition.x, electricalPosition.y ),
                                                        FromOrcadString( scopedName ) );
                scopedLabel->SetTextColor( invisibleElectricalTextColor() );
                appendPageItem( aScreen, scopedLabel );
            }
        }
    }
}


void ORCAD_CONVERTER::placeHierarchicalBlockFields( SCH_SHEET* aSheet, const ORCAD_DRAWN_INSTANCE& aBlock,
                                                    const std::string& aChildFolder )
{
    SCH_FIELD* sheetName = aSheet->GetField( FIELD_T::SHEET_NAME );
    SCH_FIELD* sheetFile = aSheet->GetField( FIELD_T::SHEET_FILENAME );
    sheetName->SetVisible( false );
    sheetFile->SetVisible( false );

    const ORCAD_DISPLAY_PROP* referenceDisplay = nullptr;
    const ORCAD_DISPLAY_PROP* valueDisplay = nullptr;
    auto namesEqual = []( const std::string& aLeft, std::string_view aRight )
    {
        return aLeft.size() == aRight.size()
               && std::equal( aLeft.begin(), aLeft.end(), aRight.begin(),
                              []( unsigned char a, unsigned char b )
                              {
                                  return std::tolower( a ) == std::tolower( b );
                              } );
    };

    for( const ORCAD_DISPLAY_PROP& display : aBlock.displayProps )
    {
        if( namesEqual( display.name, "Part Reference" ) )
            referenceDisplay = &display;
        else if( namesEqual( display.name, "Reference" ) && !referenceDisplay )
            referenceDisplay = &display;
        else if( namesEqual( display.name, "Value" ) )
            valueDisplay = &display;
    }

    auto applyDisplay = [&]( SCH_FIELD* aField, const ORCAD_DISPLAY_PROP& aDisplay )
    {
        int      fontId = displayFontId( aDisplay );
        bool     templateFont = displayUsesTemplateFont( aDisplay );
        int      size = textSizeIU( fontId, templateFont );
        int      baseline = textBaselineOffset( size, fontId, templateFont );
        bool     vertical = ( aDisplay.rotation & 1 ) != 0;
        VECTOR2I position = OrcadDbuToIu( aBlock.x1 + aDisplay.x, aBlock.y1 + aDisplay.y )
                            + ( vertical ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline ) );

        aField->SetPosition( position );
        aField->SetTextAngle( vertical ? ANGLE_VERTICAL : ANGLE_HORIZONTAL );
        aField->SetTextSize( textSize( fontId, templateFont ) );
        applyFont( aField, fontId, templateFont );
        aField->SetTextColor( OrcadColor( aDisplay.color ) );
        aField->SetEffectiveHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        aField->SetEffectiveVertJustify( GR_TEXT_V_ALIGN_TOP );
        aField->SetVisible( OrcadDisplayPropVisible( aDisplay ) );
        aField->SetNameShown( OrcadDisplayPropShowsName( aDisplay ) );
    };

    if( referenceDisplay )
        applyDisplay( sheetName, *referenceDisplay );

    if( valueDisplay )
    {
        auto value = std::find_if( aBlock.props.begin(), aBlock.props.end(),
                                   [&]( const auto& aProperty )
                                   {
                                       return namesEqual( aProperty.first, "Value" );
                                   } );
        std::string valueText;

        if( value != aBlock.props.end() )
            valueText = value->second;

        if( valueText.empty() )
            valueText = aBlock.childName;

        if( valueText.empty() )
            valueText = aChildFolder;

        if( !valueText.empty() )
        {
            SCH_FIELD implementation( aSheet, FIELD_T::USER, wxS( "Implementation" ) );
            implementation.SetText( FromOrcadString( valueText ) );
            applyDisplay( &implementation, *valueDisplay );
            aSheet->AddField( implementation );
        }
    }
}


void ORCAD_CONVERTER::placeHierarchicalBlockPinFill( SCH_SCREEN* aScreen, SCH_SHEET_PIN* aPin )
{
    std::vector<VECTOR2I> points;
    aPin->CreateGraphicShape( nullptr, points, aPin->GetPosition() );

    if( points.empty() )
        return;

    KIGFX::COLOR4D color = OrcadColor( 26 );
    SCH_SHAPE*     fill = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES );

    for( const VECTOR2I& point : points )
        fill->AddPoint( point );

    fill->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID, color ) );
    fill->SetFillMode( FILL_T::FILLED_WITH_COLOR );
    fill->SetFillColor( color );
    appendPageItem( aScreen, fill );
}


void ORCAD_CONVERTER::placeBusEntries( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_BUS_ENTRY& busEntry : aPage.busEntries )
    {
        SCH_BUS_WIRE_ENTRY* entry = new SCH_BUS_WIRE_ENTRY( OrcadDbuToIu( busEntry.x1, busEntry.y1 ) );
        entry->SetSize( VECTOR2I( ( busEntry.x2 - busEntry.x1 ) * ORCAD_IU_PER_DBU,
                                  ( busEntry.y2 - busEntry.y1 ) * ORCAD_IU_PER_DBU ) );
        entry->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::DEFAULT, OrcadColor( busEntry.color ) ) );
        appendPageItem( aScreen, entry );
    }
}


void ORCAD_CONVERTER::placeGraphicDisplayText( const ORCAD_GRAPHIC_INST& aGraphic,
                                               const ORCAD_DISPLAY_PROP& aDisplay,
                                               const std::string& aText, SCH_SCREEN* aScreen )
{
    int      fontId = displayFontId( aDisplay );
    int      size = textSizeIU( fontId );
    int      baseline = textBaselineOffset( size, fontId ) + KiROUND( size * 7.0 / 21.0 );
    int      textRotation = aDisplay.rotation & 3;
    bool     vertical = ( textRotation & 1 ) != 0;
    int      baseX = std::min( aGraphic.bbox.x1, aGraphic.bbox.x2 );
    int      baseY = std::min( aGraphic.bbox.y1, aGraphic.bbox.y2 );
    VECTOR2I textPosition = OrcadDbuToIu( baseX + aDisplay.x, baseY + aDisplay.y )
                            + ( vertical ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline ) );
    SCH_TEXT* text = new SCH_TEXT( textPosition, FromOrcadString( aText ), LAYER_NOTES );

    switch( textRotation )
    {
    case 1:  text->SetTextAngle( ANGLE_270 ); break;
    case 2:  text->SetTextAngle( ANGLE_180 ); break;
    case 3:  text->SetTextAngle( ANGLE_90 ); break;
    default: text->SetTextAngle( ANGLE_0 ); break;
    }

    text->SetTextSize( textSize( fontId ) );
    applyFont( text, fontId );
    applyMultilineSpacing( text, fontId );
    KIGFX::COLOR4D color = OrcadColor( aGraphic.color );

    if( color == KIGFX::COLOR4D::UNSPECIFIED )
        color = OrcadColor( 8 );

    text->SetTextColor( color );
    text->SetHorizJustify( textRotation == 1 || textRotation == 2 ? GR_TEXT_H_ALIGN_RIGHT
                                                                 : GR_TEXT_H_ALIGN_LEFT );
    text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );
    text->SetVisible( OrcadDisplayPropVisible( aDisplay ) );
    appendPageItem( aScreen, text );
}


void ORCAD_CONVERTER::placeOffpageConnectors( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen,
                                              const SCH_SHEET_PATH& aSheetPath, bool aHierarchical )
{
    for( const OFFPAGE_NET& offpage : offpageNets( aPage ) )
    {
        std::string net = offpage.net;
        auto        busNames = m_hierBusNamesByScreen.find( aScreen->GetUuid().AsStdString() );

        if( busNames != m_hierBusNamesByScreen.end() )
        {
            auto renamed = busNames->second.find( canonicalGlobalNetName( net ) );

            if( renamed != busNames->second.end() )
                net = renamed->second;
        }

        if( net.empty() )
        {
            note( wxString::Format( _( "Page '%s': the off-page connector at (%d, %d) is "
                                       "unconnected in the source design; skipped." ),
                                    FromOrcadString( aPage.name ), offpage.x, offpage.y ) );
            continue;
        }

        SCH_LABEL_BASE* label;

        if( aHierarchical )
            label = new SCH_HIERLABEL( OrcadDbuToIu( offpage.x, offpage.y ), FromOrcadString( net ) );
        else
            label = new SCH_GLOBALLABEL( OrcadDbuToIu( offpage.x, offpage.y ), FromOrcadString( net ) );
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        label->SetTextColor( OrcadColor( aPage.offpage[offpage.index].color ) );

        // Point label away from attached wire; vertical wire gives up/down, horizontal left/right.
        SPIN_STYLE spin = SPIN_STYLE::RIGHT;
        auto       endIt = m_wireEndpoints.find( { offpage.x, offpage.y } );

        if( endIt != m_wireEndpoints.end() && !endIt->second.empty() )
        {
            const ORCAD_WIRE* wire = endIt->second.front();
            int64_t           dx = (int64_t) wire->x1 + wire->x2 - 2LL * offpage.x;
            int64_t           dy = (int64_t) wire->y1 + wire->y2 - 2LL * offpage.y;

            if( std::abs( dx ) >= std::abs( dy ) )
                spin = dx > 0 ? SPIN_STYLE::LEFT : SPIN_STYLE::RIGHT;
            else
                spin = dy > 0 ? SPIN_STYLE::UP : SPIN_STYLE::BOTTOM;
        }

        label->SetSpinStyle( spin );

        if( SCH_GLOBALLABEL* global = dynamic_cast<SCH_GLOBALLABEL*>( label ) )
        {
            if( SCH_FIELD* refs = global->GetField( FIELD_T::INTERSHEET_REFS ) )
                refs->SetVisible( false );
        }

        const ORCAD_GRAPHIC_INST& connector = aPage.offpage[offpage.index];
        auto                      definition = m_design.symbols.find( connector.name );
        auto displayedName = std::find_if( connector.displayProps.begin(), connector.displayProps.end(),
                                           []( const ORCAD_DISPLAY_PROP& aProp )
                                           {
                                               return aProp.name == "Name";
                                           } );
        auto displayedIref = std::find_if( connector.displayProps.begin(), connector.displayProps.end(),
                                           []( const ORCAD_DISPLAY_PROP& aProp )
                                           {
                                               return aProp.name == "IREF";
                                           } );
        auto storedIref = connector.props.find( "IREF" );

        bool exactGraphics = definition != m_design.symbols.end() && displayedName != connector.displayProps.end();

        if( exactGraphics )
        {
            ORCAD_SYMBOL_DEF vectors = definition->second;
            vectors.color = connector.color;

            if( OrcadColor( vectors.color ) == KIGFX::COLOR4D::UNSPECIFIED )
                vectors.color = 8;

            int baseX = std::min( connector.bbox.x1, connector.bbox.x2 );
            int baseY = std::min( connector.bbox.y1, connector.bbox.y2 );
            int orient = OrcadOrientOf( connector.rotation, connector.mirror );
            placeDefinitionVectors( vectors, baseX, baseY, orient, aScreen );

            std::string displayText = connector.logicalName.empty() ? net : connector.logicalName;
            placeGraphicDisplayText( connector, *displayedName, displayText, aScreen );
        }

        if( storedIref != connector.props.end() && displayedIref != connector.displayProps.end()
            && OrcadDisplayPropVisible( *displayedIref ) )
        {
            placeGraphicDisplayText( connector, *displayedIref, storedIref->second, aScreen );
        }

        if( !exactGraphics || aHierarchical )
        {
            appendPageItem( aScreen, label );
            continue;
        }

        label->SetTextColor( invisibleElectricalTextColor() );
        appendPageItem( aScreen, label );
    }
}


void ORCAD_CONVERTER::placePorts( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen, bool aHierarchical )
{
    for( const ORCAD_GRAPHIC_INST& port : aPage.ports )
    {
        std::string net = effectiveInterfaceNetName( port.logicalName.empty() ? port.name : port.logicalName );
        auto        busNames = m_hierBusNamesByScreen.find( aScreen->GetUuid().AsStdString() );
        VECTOR2I    pos = namedGraphicPinPos( aPage, port );
        std::map<size_t, std::set<std::string>> occurrenceNamesByDepth;

        for( const ORCAD_WIRE& wire : aPage.wires )
        {
            if( wire.isBus || !rawPointOnSegment( pos.x, pos.y, wire ) )
                continue;

            std::set<std::string> sourceNames;
            auto                  pageName = aPage.netmap.find( wire.id );

            if( pageName != aPage.netmap.end() && !pageName->second.empty() )
                sourceNames.insert( canonicalGlobalNetName( pageName->second ) );

            auto aliases = aPage.netAliases.find( wire.id );

            if( aliases != aPage.netAliases.end() )
            {
                for( const std::string& alias : aliases->second )
                    sourceNames.insert( canonicalGlobalNetName( alias ) );
            }

            for( const std::string& sourceName : sourceNames )
            {
                std::string key = sourceName;
                std::transform( key.begin(), key.end(), key.begin(),
                                []( unsigned char c )
                                {
                                    return static_cast<char>( std::tolower( c ) );
                                } );
                auto depth = m_occurrenceNetNameMinDepth.find( key );

                if( depth != m_occurrenceNetNameMinDepth.end() )
                    occurrenceNamesByDepth[depth->second].insert( sourceName );
            }
        }

        if( !occurrenceNamesByDepth.empty() && occurrenceNamesByDepth.begin()->second.size() == 1 )
            net = *occurrenceNamesByDepth.begin()->second.begin();

        if( busNames != m_hierBusNamesByScreen.end() )
        {
            auto renamed = busNames->second.find( net );

            if( renamed != busNames->second.end() )
                net = renamed->second;
        }

        if( net.empty() )
            continue;

        // Symbol name encodes arrow direction, e.g. "PORTLEFT-L".
        wxString         symbolName = FromOrcadString( port.name ).Upper();
        LABEL_FLAG_SHAPE shape = LABEL_FLAG_SHAPE::L_BIDI;

        if( symbolName.Contains( wxS( "LEFT" ) ) )
            shape = LABEL_FLAG_SHAPE::L_INPUT;
        else if( symbolName.Contains( wxS( "RIGHT" ) ) )
            shape = LABEL_FLAG_SHAPE::L_OUTPUT;

        SCH_LABEL_BASE* label;

        if( aHierarchical )
            label = new SCH_HIERLABEL( OrcadDbuToIu( pos.x, pos.y ), FromOrcadString( net ) );
        else
            label = new SCH_GLOBALLABEL( OrcadDbuToIu( pos.x, pos.y ), FromOrcadString( net ) );

        label->SetShape( shape );
        label->SetSpinStyle( SPIN_STYLE::RIGHT );
        label->SetTextColor( OrcadColor( port.color ) );

        if( SCH_GLOBALLABEL* global = dynamic_cast<SCH_GLOBALLABEL*>( label ) )
        {
            if( SCH_FIELD* refs = global->GetField( FIELD_T::INTERSHEET_REFS ) )
                refs->SetVisible( false );
        }

        auto definition = m_design.symbols.find( port.name );
        auto displayedName = std::find_if( port.displayProps.begin(), port.displayProps.end(),
                                           []( const ORCAD_DISPLAY_PROP& aProp )
                                           {
                                               return aProp.name == "Name";
                                           } );
        bool exactGraphics = definition != m_design.symbols.end() && displayedName != port.displayProps.end();

        if( exactGraphics )
        {
            ORCAD_SYMBOL_DEF vectors = definition->second;
            vectors.color = port.color;

            if( OrcadColor( vectors.color ) == KIGFX::COLOR4D::UNSPECIFIED )
                vectors.color = 8;

            int baseX = std::min( port.bbox.x1, port.bbox.x2 );
            int baseY = std::min( port.bbox.y1, port.bbox.y2 );
            int orient = OrcadOrientOf( port.rotation, port.mirror );
            placeDefinitionVectors( vectors, baseX, baseY, orient, aScreen );
            placeGraphicDisplayText( port, *displayedName, port.logicalName.empty() ? net : port.logicalName,
                                     aScreen );
        }

        auto storedIref = port.props.find( "IREF" );
        auto displayedIref = std::find_if( port.displayProps.begin(), port.displayProps.end(),
                                           []( const ORCAD_DISPLAY_PROP& aProp )
                                           {
                                               return aProp.name == "IREF";
                                           } );

        if( storedIref != port.props.end() && displayedIref != port.displayProps.end()
            && OrcadDisplayPropVisible( *displayedIref ) )
        {
            placeGraphicDisplayText( port, *displayedIref, storedIref->second, aScreen );
        }

        if( exactGraphics )
            label->SetTextColor( invisibleElectricalTextColor() );

        appendPageItem( aScreen, label );
    }
}


void ORCAD_CONVERTER::placeGraphics( const ORCAD_RAW_PAGE& aPage, SCH_SCREEN* aScreen )
{
    for( const ORCAD_GRAPHIC_INST& gfx : aPage.graphics )
    {
        if( !gfx.nested )
            continue;

        KIGFX::COLOR4D                                          graphicColor = OrcadColor( gfx.color );

        if( graphicColor == KIGFX::COLOR4D::UNSPECIFIED
            || graphicColor == KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) )
        {
            graphicColor = KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 );
        }

        std::function<void( const ORCAD_PRIMITIVE&, int, int )> placePrimitive =
                [&]( const ORCAD_PRIMITIVE& aSource, int aOffsetX, int aOffsetY )
        {
            if( aSource.kind == ORCAD_PRIM_KIND::GROUP )
            {
                for( const ORCAD_PRIMITIVE& child : aSource.children )
                    placePrimitive( child, aOffsetX + aSource.x1, aOffsetY + aSource.y1 );

                return;
            }

            ORCAD_PRIMITIVE prim = aSource;
            prim.x1 += aOffsetX;
            prim.y1 += aOffsetY;
            prim.x2 += aOffsetX;
            prim.y2 += aOffsetY;

            for( ORCAD_POINT& point : prim.points )
            {
                point.x += aOffsetX;
                point.y += aOffsetY;
            }

            if( prim.start )
            {
                prim.start->x += aOffsetX;
                prim.start->y += aOffsetY;
            }

            if( prim.end )
            {
                prim.end->x += aOffsetX;
                prim.end->y += aOffsetY;
            }

            switch( prim.kind )
            {
            case ORCAD_PRIM_KIND::GROUP: break;

            case ORCAD_PRIM_KIND::IMAGE:
            {
                if( gfx.typeId == ORCAD_ST_GRAPHIC_OLE_INST )
                {
                    prim.x1 += 10;
                    prim.y1 += 10;
                }

                bool usedEmbeddedEmf = false;
                placeBitmap( prim, aScreen, 0, &usedEmbeddedEmf );

                if( gfx.typeId == ORCAD_ST_GRAPHIC_OLE_INST )
                {
                    ORCAD_PRIMITIVE frame = prim;
                    KIGFX::COLOR4D  frameColor = OrcadColor( gfx.color );

                    if( frameColor == KIGFX::COLOR4D::UNSPECIFIED
                        || frameColor == KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) )
                    {
                        frameColor = usedEmbeddedEmf ? OrcadColor( 8 )
                                                     : KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 );
                    }

                    frame.kind = ORCAD_PRIM_KIND::RECT;
                    frame.lineStyle = 0;
                    frame.lineWidth = 0;
                    frame.fillStyle = 1;
                    appendPageItem( aScreen,
                                    makeSheetPoly( { OrcadDbuToIu( prim.x1, prim.y1 ),
                                                     OrcadDbuToIu( prim.x2, prim.y1 ),
                                                     OrcadDbuToIu( prim.x2, prim.y2 ),
                                                     OrcadDbuToIu( prim.x1, prim.y2 ),
                                                     OrcadDbuToIu( prim.x1, prim.y1 ) },
                                                   frame, frameColor, true, false ) );
                }

                break;
            }

            case ORCAD_PRIM_KIND::TEXT:
            {
                wxString content = FromOrcadString( prim.text );
                std::string sourceFace;
                const ORCAD_FONT* sourceFont = nullptr;

                if( prim.fontIdx > 0 && prim.fontIdx <= static_cast<int>( m_design.library.fonts.size() ) )
                {
                    sourceFont = &m_design.library.fonts[prim.fontIdx - 1];
                    sourceFace = sourceFont->face;
                    std::transform( sourceFace.begin(), sourceFace.end(), sourceFace.begin(),
                                    []( unsigned char aChar ) { return std::tolower( aChar ); } );
                }

                if( sourceFace == "greekc" )
                    content.Replace( wxS( "m" ), wxString::FromUTF8( "µ" ) );
                else if( sourceFace == "commercialpi bt" )
                    content.Replace( wxS( "b" ), wxString::FromUTF8( "®" ) );

                if( content.IsEmpty() )
                    break;

                VECTOR2I  scaledTextSize = textSize( prim.fontIdx, false );

                if( !gfx.textFaceOverride.empty() && prim.fontIdx > 0
                    && prim.fontIdx <= static_cast<int>( m_design.library.fonts.size() ) )
                {
                    const ORCAD_FONT& sourceFontDef = m_design.library.fonts[prim.fontIdx - 1];

                    if( sourceFace == "arial narrow" )
                    {
                        double widthScale = sourceFontDef.width == 0 ? 35.0 / 32.0 : 8.0 / 9.0;
                        scaledTextSize.x = KiROUND( scaledTextSize.x * widthScale );
                    }
                }

                scaledTextSize.x = KiROUND( scaledTextSize.x * gfx.textScaleX );
                scaledTextSize.y = KiROUND( scaledTextSize.y * gfx.textScaleY );

                int       size = scaledTextSize.y;
                int       baseline = gfx.useGenericTextBaseline ? OrcadTextBaselineOffset( size )
                                                                : textBaselineOffset( size, prim.fontIdx, false );
                bool      vertical = ( gfx.rotation & 3 ) == 1;
                bool      hasBox = prim.x2 > prim.x1 && prim.y2 > prim.y1 && !content.Contains( '\n' );
                bool      hasSourceAnchor = prim.textBoundsStart.has_value();
                bool      wrappedToSourceBounds = false;
                EDA_TEXT* text;
                SCH_ITEM* item;
                SCH_TEXT* multilineText = nullptr;

                int anchorX = hasSourceAnchor || vertical || !hasBox ? prim.x1 : ( prim.x1 + prim.x2 ) / 2;
                int anchorY = hasSourceAnchor || !vertical || !hasBox ? prim.y1 : ( prim.y1 + prim.y2 ) / 2;
                VECTOR2I position = OrcadDbuToIu( anchorX, anchorY )
                                    + ( vertical ? VECTOR2I( baseline, 0 ) : VECTOR2I( 0, baseline ) );
                SCH_TEXT* sheetText = new SCH_TEXT( position, content );
                sheetText->SetMultilineAllowed( true );
                multilineText = sheetText;
                text = sheetText;
                item = sheetText;

                text->SetHorizJustify( hasBox && !hasSourceAnchor ? GR_TEXT_H_ALIGN_CENTER
                                                                  : GR_TEXT_H_ALIGN_LEFT );
                text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

                text->SetTextSize( scaledTextSize );
                applyFont( text, prim.fontIdx, false );

                if( sourceFace == "greekc" || sourceFace == "commercialpi bt" )
                    text->SetFont( KIFONT::FONT::GetFont( wxS( "Arial" ), text->IsBold(), text->IsItalic() ) );

                if( !gfx.textFaceOverride.empty() )
                {
                    text->SetFont( KIFONT::FONT::GetFont( FromOrcadString( gfx.textFaceOverride ), text->IsBold(),
                                                         text->IsItalic() ) );
                }

                auto sourceCellWidthDbu = [&]()
                {
                    if( !sourceFont )
                        return 0;

                    int sourceCellWidth = std::abs( sourceFont->width );

                    if( sourceFace == "arial narrow" && sourceFont->bold )
                    {
                        sourceCellWidth = std::max( sourceCellWidth,
                                                    ( std::abs( sourceFont->height ) + 1 ) / 2 );
                    }

                    return sourceCellWidth;
                };

                if( hasBox && hasSourceAnchor && !vertical && !text->GetText().Contains( '\n' ) )
                {
                    int sourceWidthDbu = prim.x2 - prim.x1;
                    int sourceHeightDbu = prim.y2 - prim.y1;
                    int sourceCellWidth = sourceCellWidthDbu();

                    if( sourceWidthDbu > 0 && sourceCellWidth > 0
                        && sourceHeightDbu <= std::abs( sourceFont->height )
                        && static_cast<int>( text->GetText().length() ) * sourceCellWidth > sourceWidthDbu )
                    {
                        wxString original = text->GetText();
                        wxString wrapped;
                        size_t   maxCharacters = sourceWidthDbu / sourceCellWidth;
                        size_t   bestWidth = std::numeric_limits<size_t>::max();

                        for( size_t pos = original.find( ' ' ); pos != wxString::npos;
                             pos = original.find( ' ', pos + 1 ) )
                        {
                            wxString candidate = original.Left( pos ) + wxS( "\n" ) + original.Mid( pos + 1 );
                            size_t   candidateWidth = std::max( pos, original.length() - pos - 1 );

                            if( candidateWidth <= maxCharacters && candidateWidth < bestWidth )
                            {
                                wrapped = candidate;
                                bestWidth = candidateWidth;
                            }
                        }

                        if( !wrapped.IsEmpty() )
                        {
                            text->SetText( wrapped );
                            wrappedToSourceBounds = true;
                        }
                        else
                        {
                            text->SetText( original );
                        }
                    }
                }

                if( sourceFace == "elephant" )
                {
                    VECTOR2I plotOffset = sheetText->GetSchematicTextOffset( nullptr )
                                           + sheetText->GetOffsetToMatchSCH_FIELD( nullptr );
                    sheetText->SetPosition( sheetText->GetPosition() - plotOffset );
                }

                if( hasBox && hasSourceAnchor )
                {
                    int sourceLengthDbu = vertical ? prim.y2 - prim.y1 : prim.x2 - prim.x1;

                    if( !wrappedToSourceBounds && !vertical && text->GetText().Contains( '\n' )
                        && sourceCellWidthDbu() > 0 )
                    {
                        size_t            maxCharacters = 0;
                        wxStringTokenizer lines( text->GetText(), wxS( "\n" ), wxTOKEN_RET_EMPTY_ALL );

                        while( lines.HasMoreTokens() )
                            maxCharacters = std::max( maxCharacters, lines.GetNextToken().length() );

                        sourceLengthDbu = std::min( sourceLengthDbu,
                                                    static_cast<int>( maxCharacters ) * sourceCellWidthDbu() );
                    }

                    int sourceLength = OrcadDbuToIu( sourceLengthDbu, 0 ).x;
                    int renderedLength = text->GetTextBox( nullptr ).GetWidth();

                    if( sourceLength > 0 && renderedLength > 0 )
                    {
                        scaledTextSize.x = KiROUND( static_cast<double>( scaledTextSize.x ) * sourceLength
                                                   / renderedLength );
                        text->SetTextSize( scaledTextSize );
                    }
                }

                if( multilineText )
                    applyMultilineSpacing( multilineText, prim.fontIdx, false );

                text->SetTextColor( graphicColor );

                // Quadrants 2/3 fold to horizontal so text reads upright.
                if( vertical )
                    text->SetTextAngle( ANGLE_VERTICAL );

                appendPageItem( aScreen, item );
                break;
            }

            case ORCAD_PRIM_KIND::LINE:
                appendPageItem( aScreen,
                                makeSheetPoly( { OrcadDbuToIu( prim.x1, prim.y1 ),
                                                 OrcadDbuToIu( prim.x2, prim.y2 ) },
                                               prim, graphicColor, false, gfx.useSymbolLineWidths ) );
                break;

            case ORCAD_PRIM_KIND::RECT:
                appendPageItem( aScreen,
                                makeSheetPoly( { OrcadDbuToIu( prim.x1, prim.y1 ),
                                                 OrcadDbuToIu( prim.x2, prim.y1 ),
                                                 OrcadDbuToIu( prim.x2, prim.y2 ),
                                                 OrcadDbuToIu( prim.x1, prim.y2 ),
                                                 OrcadDbuToIu( prim.x1, prim.y1 ) },
                                               prim, graphicColor, true, gfx.useSymbolLineWidths ) );
                break;

            case ORCAD_PRIM_KIND::POLYLINE:
            case ORCAD_PRIM_KIND::POLYGON:
            {
                if( prim.points.size() < 2 )
                    break;

                std::vector<VECTOR2I> pts;

                for( const ORCAD_POINT& pt : prim.points )
                    pts.push_back( OrcadDbuToIu( pt.x, pt.y ) );

                if( prim.kind == ORCAD_PRIM_KIND::POLYGON )
                    pts.push_back( pts.front() );

                appendPageItem( aScreen,
                                makeSheetPoly( pts, prim, graphicColor,
                                               prim.kind == ORCAD_PRIM_KIND::POLYGON,
                                               gfx.useSymbolLineWidths ) );
                break;
            }

            case ORCAD_PRIM_KIND::BEZIER:
            {
                if( prim.points.size() < 4 || ( prim.points.size() - 1 ) % 3 != 0 )
                {
                    if( prim.points.size() >= 2 )
                    {
                        std::vector<VECTOR2I> pts;

                        for( const ORCAD_POINT& pt : prim.points )
                            pts.push_back( OrcadDbuToIu( pt.x, pt.y ) );

                        appendPageItem( aScreen,
                                        makeSheetPoly( pts, prim, graphicColor, false,
                                                       gfx.useSymbolLineWidths ) );
                    }

                    break;
                }

                for( size_t i = 0; i + 3 < prim.points.size(); i += 3 )
                {
                    SCH_SHAPE* shape = new SCH_SHAPE( SHAPE_T::BEZIER, LAYER_NOTES );
                    shape->SetPosition( OrcadDbuToIu( prim.points[i].x, prim.points[i].y ) );
                    shape->SetBezierC1( OrcadDbuToIu( prim.points[i + 1].x, prim.points[i + 1].y ) );
                    shape->SetBezierC2( OrcadDbuToIu( prim.points[i + 2].x, prim.points[i + 2].y ) );
                    shape->SetEnd( OrcadDbuToIu( prim.points[i + 3].x, prim.points[i + 3].y ) );

                    int lineWidth = gfx.useSymbolLineWidths ? OrcadLineWidthIu( prim.lineWidth )
                                                            : OrcadPageGraphicLineWidthIu( prim.lineWidth );
                    shape->SetStroke( STROKE_PARAMS( lineWidth,
                                                     OrcadLineStyle( prim.lineStyle ), graphicColor ) );
                    shape->SetFillMode( FILL_T::NO_FILL );
                    appendPageItem( aScreen, shape );
                }

                break;
            }

            case ORCAD_PRIM_KIND::ARC:
            case ORCAD_PRIM_KIND::ELLIPSE:
            {
                if( prim.kind == ORCAD_PRIM_KIND::ARC && prim.start == prim.end )
                    break;

                double cx = ( prim.x1 + prim.x2 ) / 2.0;
                double cy = ( prim.y1 + prim.y2 ) / 2.0;
                double rx = std::abs( prim.x2 - prim.x1 ) / 2.0;
                double ry = std::abs( prim.y2 - prim.y1 ) / 2.0;

                if( rx == 0.0 )
                    rx = 0.01;

                if( ry == 0.0 )
                    ry = 0.01;

                double a0 = 0.0;
                double a1 = 2.0 * M_PI;

                if( prim.kind == ORCAD_PRIM_KIND::ARC && prim.start && prim.end )
                {
                    a0 = std::atan2( ( prim.start->y - cy ) / ry, ( prim.start->x - cx ) / rx );
                    a1 = std::atan2( ( prim.end->y - cy ) / ry, ( prim.end->x - cx ) / rx );

                    // Arcs run CCW in screen (Y-down) coords.
                    if( a1 >= a0 )
                        a1 -= 2.0 * M_PI;
                }

                int steps = std::max( 8, static_cast<int>( std::abs( a1 - a0 ) / ( M_PI / 16.0 ) ) );

                std::vector<VECTOR2I> pts;

                for( int k = 0; k <= steps; ++k )
                {
                    double a = a0 + ( a1 - a0 ) * k / steps;
                    pts.push_back( dbuPointToIu( cx + rx * std::cos( a ), cy + ry * std::sin( a ) ) );
                }

                appendPageItem( aScreen,
                                makeSheetPoly( pts, prim, graphicColor,
                                               prim.kind == ORCAD_PRIM_KIND::ELLIPSE,
                                               gfx.useSymbolLineWidths ) );
                break;
            }
            }
        };

        for( const ORCAD_PRIMITIVE& sourcePrimitive : gfx.nested->primitives )
            placePrimitive( sourcePrimitive, 0, 0 );
    }
}


void ORCAD_CONVERTER::placeBitmap( const ORCAD_PRIMITIVE& aPrim, SCH_SCREEN* aScreen, int aOrient,
                                   bool* aUsedEmbeddedEmf )
{
    if( aUsedEmbeddedEmf )
        *aUsedEmbeddedEmf = false;

    int widthDbu = std::abs( aPrim.x2 - aPrim.x1 );
    int heightDbu = std::abs( aPrim.y2 - aPrim.y1 );

    if( widthDbu < 2 || heightDbu < 2 || aPrim.data.empty() )
        return;

    wxMemoryBuffer bmpData;

    VECTOR2I center = dbuPointToIu( ( aPrim.x1 + aPrim.x2 ) / 2.0, ( aPrim.y1 + aPrim.y2 ) / 2.0 );

    std::unique_ptr<SCH_BITMAP> bitmap = std::make_unique<SCH_BITMAP>( center );
    REFERENCE_IMAGE&            refImage = bitmap->GetReferenceImage();

    bool readOk = false;

    std::vector<uint8_t> ciImage = OleExtractCiImage( aPrim.data );

    if( !ciImage.empty() )
    {
        bmpData.AppendData( ciImage.data(), ciImage.size() );
        wxLogNull noLog;
        readOk = refImage.ReadImageFile( bmpData );
    }

    if( !readOk )
        bmpData.Clear();

    if( !readOk && OleMakeBmpFromDib( aPrim.data, bmpData ) )
    {
        wxLogNull noLog;
        readOk = refImage.ReadImageFile( bmpData );
    }
    if( !readOk )
    {
        bmpData.Clear();
        OLE_IMAGE_PAYLOAD preview = ExtractOleImageFromPayload( aPrim.data );

        if( preview.type == OLE_IMAGE_TYPE::BMP )
        {
            bmpData.AppendData( preview.data.data(), preview.data.size() );
            wxLogNull noLog;
            readOk = refImage.ReadImageFile( bmpData );
        }
        else if( preview.type == OLE_IMAGE_TYPE::DIB && OleMakeBmpFromDib( preview.data, bmpData ) )
        {
            wxLogNull noLog;
            readOk = refImage.ReadImageFile( bmpData );
        }
        else if( preview.type == OLE_IMAGE_TYPE::WMF )
        {
            wxImage image;
            int     maxWidth = static_cast<int>( std::clamp<int64_t>( static_cast<int64_t>( widthDbu ) * 4, 1, 4096 ) );
            int maxHeight = static_cast<int>( std::clamp<int64_t>( static_cast<int64_t>( heightDbu ) * 4, 1, 4096 ) );

            if( OleRenderMetafilePreview( preview.data, maxWidth, maxHeight, image,
                                            static_cast<double>( widthDbu ) / heightDbu, aUsedEmbeddedEmf ) )
                readOk = refImage.SetImage( image );
        }
    }

    if( !readOk )
    {
        warn( wxString::Format( _( "An embedded picture could not be decoded and was skipped "
                                   "(%s)." ),
                                OleDescribeImagePayload( aPrim.data ) ) );
        return;
    }

    if( const wxImage* sourceImage = refImage.GetImage().GetImageData() )
    {
        VECTOR2I stretchedSize = OrcadStretchedImageSize( sourceImage->GetWidth(), sourceImage->GetHeight(),
                                                         widthDbu, heightDbu );

        if( stretchedSize.x != sourceImage->GetWidth() || stretchedSize.y != sourceImage->GetHeight() )
        {
            wxImage stretched = sourceImage->Scale( stretchedSize.x, stretchedSize.y, wxIMAGE_QUALITY_HIGH );

            if( stretched.IsOk() )
                refImage.SetImage( stretched );
        }
    }

    VECTOR2I nativeSize = refImage.GetSize();

    if( nativeSize.x > 0 && nativeSize.y > 0 )
    {
        double scaleX = static_cast<double>( widthDbu * ORCAD_IU_PER_DBU ) / nativeSize.x;
        double scaleY = static_cast<double>( heightDbu * ORCAD_IU_PER_DBU ) / nativeSize.y;

        refImage.SetImageScale( std::min( scaleX, scaleY ) );
    }

    const ORCAD_ORIENT_ENTRY& orientation = ORCAD_ORIENT_TABLE[aOrient & 7];

    if( orientation.mirror == 'x' )
        bitmap->MirrorVertically( center.y );
    else if( orientation.mirror == 'y' )
        bitmap->MirrorHorizontally( center.x );

    for( int angle = 0; angle < orientation.angle; angle += 90 )
        bitmap->Rotate( center, true );

    appendPageItem( aScreen, bitmap.release() );
}


VECTOR2I ORCAD_CONVERTER::snapToWire( int aX, int aY, const ORCAD_WIRE& aWire )
{
    if( aWire.x1 == aWire.x2 ) // vertical
    {
        return VECTOR2I( aWire.x1, std::clamp( aY, std::min( aWire.y1, aWire.y2 ), std::max( aWire.y1, aWire.y2 ) ) );
    }

    if( aWire.y1 == aWire.y2 ) // horizontal
    {
        return VECTOR2I( std::clamp( aX, std::min( aWire.x1, aWire.x2 ), std::max( aWire.x1, aWire.x2 ) ), aWire.y1 );
    }

    // diagonal, project onto segment
    double dx = aWire.x2 - aWire.x1;
    double dy = aWire.y2 - aWire.y1;
    double t = ( ( aX - aWire.x1 ) * dx + ( aY - aWire.y1 ) * dy ) / ( dx * dx + dy * dy );

    t = std::clamp( t, 0.0, 1.0 );

    return VECTOR2I( KiROUND( aWire.x1 + t * dx ), KiROUND( aWire.y1 + t * dy ) );
}


bool ORCAD_CONVERTER::onSegment( int aX, int aY, const ORCAD_WIRE& aWire )
{
    if( ( aX == aWire.x1 && aY == aWire.y1 ) || ( aX == aWire.x2 && aY == aWire.y2 ) )
        return false;

    if( aWire.x1 == aWire.x2 && aWire.x1 == aX )
        return std::min( aWire.y1, aWire.y2 ) < aY && aY < std::max( aWire.y1, aWire.y2 );

    if( aWire.y1 == aWire.y2 && aWire.y1 == aY )
        return std::min( aWire.x1, aWire.x2 ) < aX && aX < std::max( aWire.x1, aWire.x2 );

    return false;
}

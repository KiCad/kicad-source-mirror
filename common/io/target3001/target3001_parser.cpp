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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "target3001_parser.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <set>

#include <wx/ffile.h>
#include <wx/intl.h>
#include <wx/filename.h>
#include <wx/strconv.h>

#include <core/base64.h>
#include <ki_exception.h>


namespace TARGET3001
{

namespace
{

// Version thresholds at which the settings block gained fields
constexpr int32_t V_SETTINGS_A = 0x232a;
constexpr int32_t V_SETTINGS_B = 0x232b;
constexpr int32_t V_TRACK_WIDTH = 0x232c;
constexpr int32_t V_LOGBOOK = 0x232d;
constexpr int32_t V_SETTINGS_C = 0x232e;
constexpr int32_t V_SETTINGS_D = 0x232f;
constexpr int32_t V_SETTINGS_E = 0x2330;
constexpr int32_t V_COMPONENT_EXTRA = 0x2711;
constexpr int32_t V_SETTINGS_F = 0x2712;
constexpr int32_t V_SAVE_TIME = 0x271b;
constexpr int32_t V_SETTINGS_G = 0x271c;
constexpr int32_t V_SETTINGS_H = 0x271d;
constexpr int32_t V_SETTINGS_I = 0x271e;
constexpr int32_t V_SETTINGS_J = 0x271f;
constexpr int32_t V_SETTINGS_K = 0x2720;
constexpr int32_t V_BLOBS = 0x272e;
constexpr int32_t V_SETTINGS_L = 0x272f;
constexpr int32_t V_CHECKSUM = 0x2739;
constexpr int32_t V_SETTINGS_M = 0x2743;

constexpr int MAX_DEPTH = 32;

// A component element storing this signal holds a pad number in its component field instead
constexpr uint16_t LINKED_SIGNAL = 65503;


/// Give linked elements the signal of the pad they name and return them to their owner
void resolveLinkedSignals( COMPONENT& aComponent, uint16_t aOwner )
{
    std::map<uint16_t, uint16_t> padSignals;

    for( const ELEMENT& e : aComponent.packageItems )
    {
        if( e.type == ELEMENT_TYPE::PAD && e.pad.number != 0 )
            padSignals[e.pad.number] = e.base.signal;
    }

    auto resolve = [&]( ELEMENT& aElem )
    {
        if( aElem.base.signal != LINKED_SIGNAL )
            return;

        auto it = padSignals.find( aElem.base.component );
        aElem.base.signal = ( it != padSignals.end() && it->second != LINKED_SIGNAL ) ? it->second : 0;
        aElem.base.component = aOwner;
    };

    for( ELEMENT& e : aComponent.packageItems )
        resolve( e );

    for( ELEMENT& sym : aComponent.symbols )
    {
        if( sym.symbol )
        {
            for( ELEMENT& e : sym.symbol->children )
                resolve( e );
        }
    }
}

const std::string PASSWORD_MARKER_A = "$%&\xa7";
const std::string PASSWORD_MARKER_B = "$%&\xa7!";


class READER
{
public:
    explicit READER( const std::vector<uint8_t>& aData ) :
            m_data( aData ),
            m_pos( 0 )
    {
    }

    size_t Pos() const { return m_pos; }
    size_t Remaining() const { return m_data.size() - m_pos; }
    bool   AtEnd() const { return m_pos >= m_data.size(); }

    void Require( size_t aBytes ) const
    {
        if( aBytes > Remaining() )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unexpected end of TARGET 3001! file at offset %zu." ),
                                              m_pos ) );
        }
    }

    uint8_t U8()
    {
        Require( 1 );
        return m_data[m_pos++];
    }

    uint16_t U16()
    {
        Require( 2 );
        uint16_t v = m_data[m_pos] | ( m_data[m_pos + 1] << 8 );
        m_pos += 2;
        return v;
    }

    uint32_t U32()
    {
        Require( 4 );
        uint32_t v = uint32_t( m_data[m_pos] ) | ( uint32_t( m_data[m_pos + 1] ) << 8 )
                     | ( uint32_t( m_data[m_pos + 2] ) << 16 ) | ( uint32_t( m_data[m_pos + 3] ) << 24 );
        m_pos += 4;
        return v;
    }

    int32_t S32() { return static_cast<int32_t>( U32() ); }

    float F32()
    {
        uint32_t bits = U32();
        float    v;
        std::memcpy( &v, &bits, sizeof( v ) );
        return v;
    }

    void Skip( size_t aBytes )
    {
        Require( aBytes );
        m_pos += aBytes;
    }

    std::vector<uint8_t> Bytes( size_t aBytes )
    {
        Require( aBytes );
        std::vector<uint8_t> out( m_data.begin() + m_pos, m_data.begin() + m_pos + aBytes );
        m_pos += aBytes;
        return out;
    }

    /// Read a signed count and check that @a aMinItemSize bytes per item can still follow
    size_t Count( size_t aMinItemSize )
    {
        int32_t n = S32();

        if( n < 0 )
            Fail( "negative count" );

        if( aMinItemSize && size_t( n ) > Remaining() / aMinItemSize )
            Fail( "count exceeds file size" );

        return size_t( n );
    }

    std::string LStr()
    {
        size_t      len = Count( 1 );
        std::string s( reinterpret_cast<const char*>( m_data.data() + m_pos ), len );
        m_pos += len;
        return s;
    }

    [[noreturn]] void Fail( const char* aWhat ) const
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid TARGET 3001! file at offset %zu (%s)." ), m_pos,
                                          aWhat ) );
    }

private:
    const std::vector<uint8_t>& m_data;
    size_t                      m_pos;
};


class PARSER
{
public:
    explicit PARSER( const std::vector<uint8_t>& aData ) :
            m_r( aData )
    {
    }

    PROJECT Parse()
    {
        PROJECT p;

        p.version = m_r.S32();
        m_version = p.version;

        parseHeader( p );

        p.signals = parseTopList( nullptr );
        p.components = parseTopList( &m_componentSlots );

        // The two trees are known by their records' tags; a file may store them in the other order
        if( holdsOnly( p.signals, ELEMENT_TYPE::COMPONENT ) && holdsOnly( p.components, ELEMENT_TYPE::SIGNAL ) )
            std::swap( p.signals, p.components );

        appendFlattened( p.schematicItems, 0 );
        appendFlattened( p.boardItems, 0 );

        parseSettings( p.settings );

        if( !m_r.AtEnd() )
            m_r.Fail( "trailing data" );

        p.ApplyTextLanguage();

        return p;
    }

private:
    void parseHeader( PROJECT& aProject )
    {
        aProject.schematicTextFont = m_r.LStr();

        if( aProject.schematicTextFont == PASSWORD_MARKER_A || aProject.schematicTextFont == PASSWORD_MARKER_B )
            THROW_IO_ERROR( _( "Password-protected TARGET 3001! projects are not supported." ) );

        aProject.boardTextFont = m_r.LStr();
        aProject.signalNameFont = m_r.LStr();

        m_r.Skip( 70 );

        m_r.Skip( 94 );

        std::vector<uint8_t> functions = m_r.Bytes( BOARD_LAYER_COUNT );
        std::vector<uint8_t> associations = m_r.Bytes( BOARD_LAYER_COUNT );
        std::copy( functions.begin(), functions.end(), aProject.layerFunctions.begin() );
        std::copy( associations.begin(), associations.end(), aProject.layerAssociations.begin() );

        m_r.Skip( 119 );
        uint8_t prefixCount = m_r.U8();

        for( int i = 0; i < prefixCount; i++ )
        {
            m_r.U16();
            m_r.LStr();
        }
    }

    /**
     * Read a top-level list.  Null slots are kept as NONE placeholders so the slot count is preserved.
     *
     * @param aSlots receives the stored item count
     */
    /// True when @a aList has records and all of them are of @a aType, empty slots aside
    static bool holdsOnly( const std::vector<ELEMENT>& aList, ELEMENT_TYPE aType )
    {
        bool any = false;

        for( const ELEMENT& e : aList )
        {
            if( e.type == ELEMENT_TYPE::NONE )
                continue;

            if( e.type != aType )
                return false;

            any = true;
        }

        return any;
    }

    std::vector<ELEMENT> parseTopList( size_t* aSlots )
    {
        if( m_r.U8() != uint8_t( ELEMENT_TYPE::LIST ) )
            m_r.Fail( "element list expected" );

        size_t               n = m_r.Count( 1 );
        std::vector<ELEMENT> out;
        out.reserve( n );

        for( size_t i = 0; i < n; i++ )
        {
            size_t before = out.size();
            appendFlattened( out, 1 );

            if( out.size() == before )
                out.emplace_back();
        }

        if( aSlots )
            *aSlots = n;

        return out;
    }

    size_t parseListBody( std::vector<ELEMENT>& aOut, int aDepth )
    {
        size_t n = m_r.Count( 1 );
        aOut.reserve( aOut.size() + n );

        for( size_t i = 0; i < n; i++ )
            appendFlattened( aOut, aDepth + 1 );

        return n;
    }

    /// Read one element; lists are spliced into @a aOut and the null element adds nothing
    void appendFlattened( std::vector<ELEMENT>& aOut, int aDepth )
    {
        if( aDepth > MAX_DEPTH )
            m_r.Fail( "element nesting too deep" );

        uint8_t tag = m_r.U8();

        switch( tag )
        {
        case uint8_t( ELEMENT_TYPE::NONE ):
            return;

        case uint8_t( ELEMENT_TYPE::LIST ):
            parseListBody( aOut, aDepth );
            return;

        case uint8_t( ELEMENT_TYPE::LAYER_FUNCTIONS ):
        {
            size_t n = m_r.Count( 4 );

            for( size_t i = 0; i < n; i++ )
                m_r.LStr();

            return;
        }

        default:
            break;
        }

        if( tag < uint8_t( ELEMENT_TYPE::LINE ) || tag > uint8_t( ELEMENT_TYPE::DRC_MARKER ) )
            m_r.Fail( "unknown element class" );

        ELEMENT& e = aOut.emplace_back();
        e.type = ELEMENT_TYPE( tag );
        parseBase( e.base );
        parseBody( e, aDepth );
    }

    std::vector<ELEMENT> parseChild( int aDepth )
    {
        std::vector<ELEMENT> out;
        appendFlattened( out, aDepth + 1 );
        return out;
    }

    void parseBase( ELEMENT_BASE& aBase )
    {
        aBase.component = m_r.U16();
        aBase.signal = m_r.U16();
        aBase.x = m_r.S32();
        aBase.y = m_r.S32();
        aBase.flags = m_r.U8();
        aBase.status = m_r.U8();
        aBase.layer = m_r.U8();
        aBase.classTag = m_r.U8();

        if( ( aBase.flags & 0xfe ) == 2 )
            aBase.extra = m_r.U32();

        size_t n = m_r.Count( 4 );

        if( n % 2 )
            m_r.Fail( "odd property count" );

        aBase.props.reserve( n / 2 );

        for( size_t i = 0; i < n; i += 2 )
        {
            std::string key = m_r.LStr();
            std::string value = m_r.LStr();
            aBase.props.emplace_back( std::move( key ), std::move( value ) );
        }
    }

    void parseBody( ELEMENT& aElem, int aDepth )
    {
        switch( aElem.type )
        {
        case ELEMENT_TYPE::LINE:
        {
            LINE& l = aElem.line;
            l.x2 = m_r.S32();
            l.y2 = m_r.S32();
            l.width = m_r.S32();
            l.clearance = m_r.S32();
            l.dashStyle = m_r.U8();
            l.roundEnds = m_r.U8();
            break;
        }

        case ELEMENT_TYPE::PIN:
        {
            PIN& p = aElem.pin;
            p.direction = m_r.F32();
            p.length = m_r.S32();
            p.width = m_r.S32();
            p.electricalType = m_r.U8();
            p.swapLevel = m_r.U16();
            p.inverted = m_r.U8();
            p.clock = m_r.U8();
            p.showNumber = m_r.U8();
            p.supplyPin = m_r.U8();
            p.pinNumber = m_r.U16();
            aElem.children = parseChild( aDepth );
            break;
        }

        case ELEMENT_TYPE::PAD:
        {
            PAD& p = aElem.pad;
            p.direction = m_r.F32();
            p.shape = m_r.U8();
            p.width = m_r.S32();
            p.height = m_r.S32();
            p.clearance = m_r.S32();
            p.drill = m_r.S32();
            p.reserved = m_r.S32();
            p.number = m_r.U16();
            break;
        }

        case ELEMENT_TYPE::TRIANGLE:
        {
            TRIANGLE& t = aElem.triangle;
            t.x2 = m_r.S32();
            t.y2 = m_r.S32();
            t.x3 = m_r.S32();
            t.y3 = m_r.S32();
            break;
        }

        case ELEMENT_TYPE::RECTANGLE:
        {
            RECTANGLE& r = aElem.rectangle;
            r.height = m_r.S32();
            r.width = m_r.S32();
            r.direction = m_r.F32();
            break;
        }

        case ELEMENT_TYPE::ARC:
        {
            ARC& a = aElem.arc;
            a.endX = m_r.S32();
            a.endY = m_r.S32();
            a.centerX = m_r.S32();
            a.centerY = m_r.S32();
            a.radius = m_r.S32();
            a.width = m_r.S32();
            a.clearance = m_r.S32();
            a.startAngle = m_r.F32();
            a.endAngle = m_r.F32();

            if( !std::isfinite( a.startAngle ) || !std::isfinite( a.endAngle ) )
                m_r.Fail( "arc angle is not a number" );

            break;
        }

        case ELEMENT_TYPE::DISC:
            aElem.disc.radius = m_r.S32();
            break;

        case ELEMENT_TYPE::TEXT:
        {
            TEXT& t = aElem.text;
            t.text = m_r.LStr();
            t.height = m_r.S32();
            t.width = m_r.S32();
            t.direction = m_r.F32();
            t.weight = m_r.U8();
            t.noAutoUpright = m_r.U8();
            t.mirrored = m_r.U8();
            t.function = m_r.U8();
            t.font = m_r.LStr();
            break;
        }

        case ELEMENT_TYPE::FIDUCIAL:
        {
            FIDUCIAL& f = aElem.fiducial;
            f.kind = m_r.U8();
            f.radius = m_r.S32();
            f.width = m_r.S32();
            f.direction = m_r.F32();
            break;
        }

        case ELEMENT_TYPE::COMPONENT:
        {
            auto c = std::make_shared<COMPONENT>();
            c->name = m_r.LStr();
            c->value = m_r.LStr();
            c->prefix = m_r.LStr();
            c->library = m_r.LStr();
            c->package = m_r.LStr();
            c->packageSource = m_r.LStr();
            c->referenceNumber = m_r.U16();

            if( c->referenceNumber >= 65000 && c->referenceNumber < 65535 )
                c->referenceExtension = m_r.U16();

            c->connectionCount = m_r.U16();
            c->gateState = m_r.U8();
            c->packageItems = parseChild( aDepth );
            c->symbols = parseChild( aDepth );

            for( uint8_t& b : c->setBitmap )
                b = m_r.U8();

            resolveLinkedSignals( *c, aElem.base.component );

            aElem.component = std::move( c );
            break;
        }

        case ELEMENT_TYPE::SYMBOL:
        {
            auto s = std::make_shared<SYMBOL>();
            s->name = m_r.LStr();
            s->suffix = m_r.LStr();
            s->gateOrdinal = m_r.U16();
            s->insertRequired = m_r.U8();
            s->swapLevel = m_r.U16();
            s->children = parseChild( aDepth );
            aElem.symbol = std::move( s );
            break;
        }

        case ELEMENT_TYPE::PACKAGE:
        {
            auto p = std::make_shared<PACKAGE>();
            p->package = m_r.LStr();
            p->library = m_r.LStr();
            p->padCount = m_r.U16();
            p->bottomSide = m_r.U8();
            p->direction = m_r.F32();
            aElem.package = std::move( p );
            break;
        }

        case ELEMENT_TYPE::SIGNAL:
        {
            auto s = std::make_shared<SIGNAL>();
            s->name = m_r.LStr();
            s->connections = parseChild( aDepth );
            s->tracks = parseChild( aDepth );
            s->minWidth = m_r.S32();
            s->minSpacing = m_r.S32();
            s->string3 = m_r.LStr();
            s->string4 = m_r.LStr();
            s->record = m_r.Bytes( 48 );
            s->string5 = m_r.LStr();
            aElem.signal = std::move( s );
            break;
        }

        case ELEMENT_TYPE::SPLINE:
        {
            SPLINE& s = aElem.spline;
            s.endX = m_r.S32();
            s.endY = m_r.S32();
            s.controlX = m_r.S32();
            s.controlY = m_r.S32();
            s.width = m_r.S32();
            s.clearance = m_r.S32();

            break;
        }

        case ELEMENT_TYPE::DRC_MARKER:
            aElem.marker.radius = m_r.S32();
            aElem.marker.text = m_r.LStr();
            break;

        default:
            m_r.Fail( "unknown element class" );
        }
    }

    void skipStrings( size_t aCount )
    {
        for( size_t i = 0; i < aCount; i++ )
            m_r.LStr();
    }

    void skipCountedStrings() { skipStrings( m_r.Count( 4 ) ); }

    void parseSettings( PROJECT_SETTINGS& aSettings )
    {
        m_r.Skip( 16 + 16 + 52 );
        m_r.Skip( 3 * 4 );
        m_r.Skip( 40 + 40 );
        m_r.Skip( 5 * 4 );
        m_r.Skip( 29 + 29 );

        size_t records = m_r.Count( 16 );
        m_r.Skip( records * 16 );

        skipCountedStrings();       // functions
        m_r.Skip( 30 );
        m_r.LStr();

        uint16_t routeLayers = m_r.U16();
        m_r.Skip( size_t( routeLayers ) * 4 );

        size_t propertyStrings = m_r.Count( 4 );

        for( size_t i = 0; i + 1 < propertyStrings; i += 2 )
        {
            std::string key = m_r.LStr();
            aSettings.properties.emplace_back( std::move( key ), m_r.LStr() );
        }

        if( propertyStrings % 2 )
            m_r.LStr();

        size_t padstacks = m_r.Count( 4 + 5 + 500 );

        for( size_t i = 0; i < padstacks; i++ )
        {
            PADSTACK_DEF& def = aSettings.padstacks.emplace_back();
            def.name = m_r.LStr();
            m_r.Skip( 5 );     // router cost and cached use count

            for( bool& on : def.onLayer )
                on = m_r.U8() != 0;

            for( int32_t& grow : def.oversize )
                grow = int32_t( m_r.U32() );
        }

        m_r.Skip( size_t( routeLayers ) * 5 );
        m_r.Skip( padstacks * 8 );

        m_r.U32();
        skipCountedStrings();
        m_r.Skip( 100 + 13 + 208 );
        skipStrings( 16 );

        m_r.U8();
        m_r.LStr();
        m_r.LStr();
        m_r.Skip( 22 + 15 );
        m_r.LStr();
        aSettings.savedBy = m_r.LStr();

        size_t records9 = m_r.Count( 4 + 9 );

        for( size_t i = 0; i < records9; i++ )
        {
            m_r.LStr();
            m_r.Skip( 9 );
        }

        m_r.U8();

        aSettings.sheetNames.reserve( 100 );

        for( int i = 0; i < 100; i++ )
            aSettings.sheetNames.push_back( m_r.LStr() );

        skipStrings( 32 );
        m_r.U8();

        if( m_version >= V_SETTINGS_A )
        {
            m_r.LStr();
            m_r.LStr();
            m_r.Skip( 49 );
        }

        if( m_version >= V_SETTINGS_B )
            m_r.LStr();

        if( m_version >= V_TRACK_WIDTH )
            m_r.Skip( 8 );

        if( m_version >= V_LOGBOOK )
            skipStrings( 3 );

        if( m_version >= V_SETTINGS_C )
            m_r.LStr();

        if( m_version >= V_SETTINGS_D )
            m_r.U16();

        if( m_version >= V_SETTINGS_E )
            m_r.LStr();

        // The reader only takes these tables when the stream has not ended
        if( m_version >= V_SETTINGS_E && !m_r.AtEnd() )
        {
            for( int i = 0; i < 12; i++ )
            {
                m_r.LStr();
                m_r.Skip( 10 );
            }

            for( int i = 0; i < 12; i++ )
            {
                m_r.LStr();
                m_r.Skip( 18 );
            }
        }

        if( m_version >= V_COMPONENT_EXTRA )
        {
            for( size_t i = 0; i < m_componentSlots; i++ )
            {
                if( m_r.U8() )
                    m_r.Skip( 28 );
            }

            skipStrings( 223 );
        }

        if( m_version >= V_SETTINGS_F )
            skipStrings( 2 );

        if( m_version >= V_SAVE_TIME )
            m_r.Skip( 8 );

        if( m_version >= V_SETTINGS_G )
            m_r.Skip( 40 + 40 );

        if( m_version >= V_SETTINGS_H )
            m_r.Skip( 2 );

        if( m_version >= V_SETTINGS_I )
        {
            uint8_t language = m_r.U8();

            if( language <= uint8_t( TEXT_LANGUAGE::FRENCH ) )
                aSettings.textLanguage = TEXT_LANGUAGE( language );
        }

        if( m_version >= V_SETTINGS_J )
            m_r.Skip( 32 );

        if( m_version >= V_SETTINGS_K )
            m_r.Skip( 1 );

        if( m_version >= V_BLOBS )
        {
            size_t blobs = m_r.Count( 8 );

            for( size_t i = 0; i < blobs; i++ )
            {
                m_r.LStr();
                m_r.Skip( m_r.Count( 1 ) );
            }
        }

        if( m_version >= V_SETTINGS_L )
            skipStrings( 12 );

        if( m_version >= V_CHECKSUM )
            m_r.Skip( 8 );

        if( m_version >= V_SETTINGS_M )
            m_r.Skip( 16 + 16 );
    }

    READER  m_r;
    int32_t m_version = 0;
    size_t  m_componentSlots = 0;
};

} // namespace


const std::string* ELEMENT_BASE::FindProp( const std::string& aKey ) const
{
    for( const auto& [key, value] : props )
    {
        if( key == aKey )
            return &value;
    }

    return nullptr;
}


std::vector<uint8_t> ELEMENT_BASE::Picture() const
{
    std::vector<uint8_t> data;
    const std::string*   encoded = FindProp( "JPEG_PICTURE" );

    if( !encoded )
        return data;

    // Base64 with '_' standing in for '+'
    std::vector<uint8_t> text( encoded->begin(), encoded->end() );
    std::replace( text.begin(), text.end(), uint8_t( '_' ), uint8_t( '+' ) );
    base64::decode( text, data );
    return data;
}


bool ELEMENT_BASE::FlagProp( const std::string& aKey ) const
{
    const std::string* value = FindProp( aKey );

    if( !value )
        return false;

    wxString word = ToWx( *value ).Trim().Trim( false ).Upper();

    return word == wxS( "TRUE" ) || word == wxS( "YES" ) || word == wxS( "1" ) || word == wxS( "JA" )
           || word == wxS( "OUI" ) || word == wxS( "OK" );
}


std::optional<std::pair<int32_t, int32_t>> ELEMENT_BASE::TextRightEnd() const
{
    const std::string* arxy = FindProp( "ARXY" );

    if( !arxy )
        return std::nullopt;

    wxString value( *arxy );
    long     endX = 0;
    long     endY = 0;

    if( !value.Contains( wxS( "|" ) ) || !value.BeforeFirst( '|' ).ToLong( &endX )
        || !value.AfterFirst( '|' ).ToLong( &endY ) )
    {
        return std::nullopt;
    }

    return std::make_pair( int32_t( endX ), int32_t( endY ) );
}


const std::string* PROJECT::FindProperty( const std::string& aKey ) const
{
    for( const auto& [key, value] : settings.properties )
    {
        if( ToWx( key ).Trim().Trim( false ).IsSameAs( ToWx( aKey ), false ) )
            return &value;
    }

    return nullptr;
}


std::optional<wxString> PROJECT::FunctionText( const TEXT& aText, bool aSchematic ) const
{
    // The change timestamp is the later of the file time and the previous save time in property 1218
    auto changed = [&]()
    {
        wxDateTime when = fileModified;
        double     days = 0.0;

        if( const std::string* saved = FindProperty( "1218" ) )
        {
            wxString value = ToWx( *saved );
            value.Replace( wxS( "," ), wxS( "." ) );

            // TDateTime counts days from 1899-12-30, which is Julian day 2415018.5
            if( value.ToCDouble( &days ) && days > 0.0 )
            {
                wxDateTime previous( days + 2415018.5 );

                if( !when.IsValid() || previous.IsLaterThan( when ) )
                    when = previous;
            }
        }

        return when;
    };

    switch( TEXT_FUNCTION( aText.function ) )
    {
    case TEXT_FUNCTION::CHANGE_DATE:
    {
        wxDateTime when = changed();
        return when.IsValid() ? when.Format( wxS( "%d.%m.%Y" ) ) : wxString();
    }

    case TEXT_FUNCTION::CHANGE_TIME:
        return fileModified.IsValid() ? fileModified.Format( wxS( "%H:%M" ) ) : wxString();

    // Output stamps stay live; KiCad formats them as ISO date and HH:MM:SS
    case TEXT_FUNCTION::OUTPUT_DATE: return wxString( wxS( "${CURRENT_DATE}" ) );
    case TEXT_FUNCTION::OUTPUT_TIME: return wxString( wxS( "${CURRENT_TIME_HH_MM_SS}" ) );

    // Vector output always shows the unscaled drawing; printing formats it with two decimals
    case TEXT_FUNCTION::SCALE: return wxString( aSchematic ? wxS( "100.00%" ) : wxS( "100%" ) );

    case TEXT_FUNCTION::PROJECT_NAME: return wxFileName( fileName ).GetName();
    case TEXT_FUNCTION::FILE_NAME: return fileName;

    case TEXT_FUNCTION::SHEET_NUMBER:
        return aSchematic ? std::optional<wxString>( wxS( "${#}" ) ) : std::nullopt;

    case TEXT_FUNCTION::PAGE_NAME:
        return aSchematic ? std::optional<wxString>( wxS( "${SHEETNAME}" ) ) : std::nullopt;

    default: return std::nullopt;
    }
}


const ELEMENT* PROJECT::FindSignal( uint16_t aId ) const
{
    if( aId == 0 )
        return nullptr;

    for( const ELEMENT& e : signals )
    {
        if( e.type == ELEMENT_TYPE::SIGNAL && e.signal && e.base.signal == aId )
            return &e;
    }

    return nullptr;
}


namespace
{

struct LANGUAGE_INFO
{
    TEXT_LANGUAGE language;
    const char*   key;
};


const LANGUAGE_INFO LANGUAGES[] = { { TEXT_LANGUAGE::GERMAN, "$%&DEUTSCH" },
                                    { TEXT_LANGUAGE::ENGLISH, "$%&ENGLISH" },
                                    { TEXT_LANGUAGE::FRENCH, "$%&FRANCAIS" } };


template <typename LIST, typename FN>
void forEachText( LIST& aList, FN&& aFn )
{
    for( auto& e : aList )
    {
        if( e.type == ELEMENT_TYPE::TEXT )
            aFn( e );

        forEachText( e.children, aFn );

        if( e.symbol )
            forEachText( e.symbol->children, aFn );

        if( e.component )
        {
            forEachText( e.component->packageItems, aFn );
            forEachText( e.component->symbols, aFn );
        }

        if( e.signal )
        {
            forEachText( e.signal->connections, aFn );
            forEachText( e.signal->tracks, aFn );
        }
    }
}


template <typename PROJ, typename FN>
void forEachProjectText( PROJ& aProject, FN&& aFn )
{
    forEachText( aProject.signals, aFn );
    forEachText( aProject.components, aFn );
    forEachText( aProject.schematicItems, aFn );
    forEachText( aProject.boardItems, aFn );
}


// Variants only ever replace literal text; other functions are computed from their owner
std::optional<std::string> variant( const ELEMENT& aText, const LANGUAGE_INFO& aLanguage )
{
    if( aText.text.function != uint8_t( TEXT_FUNCTION::LITERAL ) )
        return std::nullopt;

    const std::string* value = aText.base.FindProp( aLanguage.key );

    if( !value || value->empty() )
        return std::nullopt;

    // Spaces are stored as 0x7f inside variants
    std::string text = *value;
    std::replace( text.begin(), text.end(), '\x7f', ' ' );
    return text;
}

} // namespace


void PROJECT::ApplyTextLanguage()
{
    bool automatic = false;

    forEachProjectText( *this,
                        [&]( const ELEMENT& aText )
                        {
                            wxString text = ToWx( aText.text.text ).Trim().Trim( false ).Upper();
                            automatic |= text == wxS( "AUTOLANGUAGE=TRUE" );
                        } );

    // Without the switch the source program shows stored text whatever language the file names
    if( !automatic || !settings.textLanguage )
        return;

    for( const LANGUAGE_INFO& lang : LANGUAGES )
    {
        if( lang.language != *settings.textLanguage )
            continue;

        forEachProjectText( *this,
                            [&]( ELEMENT& aText )
                            {
                                if( std::optional<std::string> text = variant( aText, lang ) )
                                    aText.text.text = *text;
                            } );
    }
}


wxString PadNumber( const ELEMENT& aPad )
{
    if( const std::string* name = aPad.base.FindProp( "PADNAME" ) )
        return ToWx( *name );

    if( aPad.pad.number )
        return wxString::Format( wxS( "%u" ), aPad.pad.number );

    return wxEmptyString;
}


PROJECT ParseBuffer( const std::vector<uint8_t>& aData )
{
    PARSER parser( aData );
    return parser.Parse();
}


PROJECT ParseFile( const wxString& aFileName )
{
    wxFFile file( aFileName, wxS( "rb" ) );

    if( !file.IsOpened() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'." ), aFileName ) );

    std::vector<uint8_t> data( file.Length() );

    if( !data.empty() && file.Read( data.data(), data.size() ) != data.size() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot read file '%s'." ), aFileName ) );

    PROJECT    project = ParseBuffer( data );
    wxFileName fn( aFileName );

    project.fileName = fn.GetFullName();
    project.fileModified = fn.GetModificationTime();
    return project;
}


bool LooksLikeProject( const wxString& aFileName )
{
    wxFileName fn( aFileName );

    if( fn.GetExt().CmpNoCase( wxS( "T3001" ) ) != 0 )
        return false;

    wxFFile file( aFileName, wxS( "rb" ) );

    if( !file.IsOpened() )
        return false;

    uint8_t head[8];

    if( file.Read( head, sizeof( head ) ) != sizeof( head ) )
        return false;

    int32_t version = int32_t( head[0] | ( head[1] << 8 ) | ( head[2] << 16 ) | ( uint32_t( head[3] ) << 24 ) );
    int32_t fontLen = int32_t( head[4] | ( head[5] << 8 ) | ( head[6] << 16 ) | ( uint32_t( head[7] ) << 24 ) );

    return version > 0 && version < 0x10000 && fontLen >= 0 && fontLen < 256;
}


wxString ToWx( const std::string& aRaw )
{
    static wxCSConv conv( wxFONTENCODING_CP1252 );
    return wxString( aRaw.data(), conv, aRaw.size() );
}

} // namespace TARGET3001

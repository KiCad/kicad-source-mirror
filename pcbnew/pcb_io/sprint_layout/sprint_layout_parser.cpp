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
 *
 * Binary format knowledge derived from:
 *   https://github.com/sergey-raevskiy/xlay (lay6.h)
 *   https://github.com/OpenBoardView/OpenBoardView (LAYFile.cpp)
 */

#include "sprint_layout_parser.h"

#include <board.h>
#include <board_item_container.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <math/util.h>
#include <math/box2.h>
#include <font/fontconfig.h>

#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/log.h>
#include <wx/strconv.h>
#include <wx/fontenc.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

// All multi-byte reads below decode little-endian explicitly,
// so this parser works correctly on any host byte order.

static constexpr uint32_t MAX_OBJECTS    = 1000000;
static constexpr uint32_t MAX_GROUPS     = 100000;
static constexpr uint32_t MAX_CHILDREN   = 10000;
static constexpr uint32_t MAX_POINTS     = 1000000;


SPRINT_LAYOUT_PARSER::SPRINT_LAYOUT_PARSER() = default;


SPRINT_LAYOUT_PARSER::~SPRINT_LAYOUT_PARSER() = default;


// ============================================================================
// Binary reading helpers
// ============================================================================

uint8_t SPRINT_LAYOUT_PARSER::readUint8()
{
    if( m_pos + 1 > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    return *m_pos++;
}


uint16_t SPRINT_LAYOUT_PARSER::readUint16()
{
    if( m_pos + 2 > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    uint16_t v = static_cast<uint16_t>( m_pos[0] )
                 | ( static_cast<uint16_t>( m_pos[1] ) << 8 );
    m_pos += 2;
    return v;
}


int16_t SPRINT_LAYOUT_PARSER::readInt16()
{
    return static_cast<int16_t>( readUint16() );
}


uint32_t SPRINT_LAYOUT_PARSER::readUnsigned()
{
    if( m_fileData.version >= 3 )
        return readUint32();
    else
        return readUint16();
}


int32_t SPRINT_LAYOUT_PARSER::readSigned()
{
    if( m_fileData.version >= 3 )
        return readInt32();
    else
        return readInt16();
}


uint32_t SPRINT_LAYOUT_PARSER::readUint32()
{
    if( m_pos + 4 > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    uint32_t v = static_cast<uint32_t>( m_pos[0] )
                 | ( static_cast<uint32_t>( m_pos[1] ) << 8 )
                 | ( static_cast<uint32_t>( m_pos[2] ) << 16 )
                 | ( static_cast<uint32_t>( m_pos[3] ) << 24 );
    m_pos += 4;
    return v;
}


int32_t SPRINT_LAYOUT_PARSER::readInt32()
{
    return static_cast<int32_t>( readUint32() );
}


float SPRINT_LAYOUT_PARSER::readFloat()
{
    uint32_t bits = readUint32();
    float v;
    std::memcpy( &v, &bits, sizeof( v ) );
    return v;
}


double SPRINT_LAYOUT_PARSER::readDouble()
{
    if( m_pos + 8 > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    uint64_t bits = static_cast<uint64_t>( m_pos[0] )
                    | ( static_cast<uint64_t>( m_pos[1] ) << 8 )
                    | ( static_cast<uint64_t>( m_pos[2] ) << 16 )
                    | ( static_cast<uint64_t>( m_pos[3] ) << 24 )
                    | ( static_cast<uint64_t>( m_pos[4] ) << 32 )
                    | ( static_cast<uint64_t>( m_pos[5] ) << 40 )
                    | ( static_cast<uint64_t>( m_pos[6] ) << 48 )
                    | ( static_cast<uint64_t>( m_pos[7] ) << 56 );
    m_pos += 8;

    double v;
    std::memcpy( &v, &bits, sizeof( v ) );
    return v;
}


float SPRINT_LAYOUT_PARSER::readCoord()
{
    if( m_fileData.version >= 5 )
        return readFloat();
    else if( m_fileData.version >= 3 )
        return static_cast<float>( readInt32() );
    else
        return static_cast<float>( readInt16() );
}


std::string SPRINT_LAYOUT_PARSER::readFixedString( size_t aMaxLen )
{
    size_t rawLen = readUint8();
    size_t len = std::min( rawLen, aMaxLen );

    if( m_pos + aMaxLen > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    std::string s( reinterpret_cast<const char*>( m_pos ), len );
    m_pos += aMaxLen;
    return s;
}


std::string SPRINT_LAYOUT_PARSER::readVarString()
{
    uint32_t len = readUint32();

    if( len > 100000 )
        THROW_IO_ERROR( _( "Invalid string length in Sprint Layout file" ) );

    if( m_pos + len > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    std::string s( reinterpret_cast<const char*>( m_pos ), len );
    m_pos += len;
    return s;
}


void SPRINT_LAYOUT_PARSER::skip( size_t aBytes )
{
    if( m_pos + aBytes > m_end )
        THROW_IO_ERROR( _( "Unexpected end of Sprint Layout file" ) );

    m_pos += aBytes;
}


void SPRINT_LAYOUT_PARSER::seek( int aBytes )
{
    const uint8_t* seekTo = m_pos + aBytes;

    if( seekTo > m_end || seekTo < m_start )
        THROW_IO_ERROR( _( "Unexpected seek in Sprint Layout file" ) );

    m_pos = seekTo;
}


// ============================================================================
// Parsing
// ============================================================================


bool SPRINT_LAYOUT_PARSER::ParseBoard( const wxString& aFileName )
{
    m_parsingMacro = false;
    parseFileStart( aFileName );

    if( m_fileData.version >= 3 )
    {
        uint32_t numBoards = readUnsigned();

        if( numBoards == 0 || numBoards > 100 )
            THROW_IO_ERROR( _( "Invalid board count in Sprint Layout file" ) );

        m_fileData.boards.resize( numBoards );
    }
    else
    {
        m_fileData.boards.resize( 1 );
    }

    for( uint32_t b = 0; b < m_fileData.boards.size(); b++ )
    {
        SPRINT_LAYOUT::BOARD_DATA& boardData = m_fileData.boards[b];
        parseBoardHeader( boardData );

        uint32_t numObjects = readUnsigned();

        if( numObjects > MAX_OBJECTS )
            THROW_IO_ERROR( _( "Too many objects in Sprint Layout board" ) );

        boardData.objects.resize( numObjects );

        for( uint32_t i = 0; i < numObjects; i++ )
            parseObject( boardData.objects[i] );

        if( m_fileData.version >= 3 )
        {
            uint32_t numConnections = 0;

            for( auto& obj : boardData.objects )
            {
                if( obj.type == SPRINT_LAYOUT::OBJ_THT_PAD || obj.type == SPRINT_LAYOUT::OBJ_SMD_PAD )
                    numConnections++;
            }

            // Read connection records (one per pad object)
            for( uint32_t c = 0; c < numConnections; c++ )
            {
                uint32_t connCount = readUnsigned();

                // Skip the connection data for now
                for( uint32_t i = 0; i < connCount; i++ )
                    (void) readUnsigned();
            }
        }
    }

    parseTrailer();

    return true;
}


bool SPRINT_LAYOUT_PARSER::ParseMacroFile( const wxString& aFileName )
{
    // Parse the macro data into BOARD_DATA
    SPRINT_LAYOUT::BOARD_DATA data;
    data.name = wxFileNameFromPath( aFileName ).BeforeLast( '.' );

    m_parsingMacro = true;
    parseFileStart( aFileName );
    parseObjectsList( data );

    m_fileData.boards = { data };

    return true;
}


void SPRINT_LAYOUT_PARSER::parseFileStart( const wxString& aFileName )
{
    wxFFileInputStream stream( aFileName );

    if( !stream.IsOk() )
        THROW_IO_ERROR( wxString::Format( _( "Cannot open file '%s'" ), aFileName ) );

    size_t fileSize = stream.GetLength();

    if( fileSize < 8 )
        THROW_IO_ERROR( wxString::Format( _( "File '%s' is too small to be a Sprint Layout file" ), aFileName ) );

    m_buffer.resize( fileSize );
    stream.Read( m_buffer.data(), fileSize );

    if( stream.LastRead() != fileSize )
        THROW_IO_ERROR( wxString::Format( _( "Failed to read file '%s'" ), aFileName ) );

    m_start = m_buffer.data();
    m_pos = m_start;
    m_end = m_start + fileSize;

    // File header: version + magic bytes (0x33, 0xAA, 0xFF)
    m_fileData.version = readUint8();
    uint8_t magic1 = readUint8();
    uint8_t magic2 = readUint8();
    uint8_t magic3 = readUint8();

    if( m_fileData.version > 6 || magic1 != 0x33 || magic2 != 0xAA || magic3 != 0xFF )
        THROW_IO_ERROR( _( "Invalid Sprint Layout file header" ) );
}


void SPRINT_LAYOUT_PARSER::parseBoardHeader( SPRINT_LAYOUT::BOARD_DATA& aBoard )
{
    if( m_fileData.version >= 3 )
    {
        // Board name (Pascal string, 30 bytes max)
        aBoard.name = readFixedString( 30 );

        // Unknown padding
        skip( 4 );

        aBoard.size_x = readUint32();
        aBoard.size_y = readUint32();

        // Ground plane enabled flag per layer (C1, S1, C2, S2, I1, I2, O)
        for( int i = 0; i < 7; i++ )
            aBoard.ground_plane[i] = readUint8();

        if( m_fileData.version >= 5 )
        {
            // Grid and viewport (not needed for import)
            readDouble(); // active_grid_val
            readDouble(); // zoom
            readUint32(); // viewport_offset_x
            readUint32(); // viewport_offset_y

            // Active layer + padding
            skip( 4 );

            // Layer visibility + scanned copy flags
            skip( 7 ); // layer_visible[7]
            skip( 1 ); // show_scanned_copy_top
            skip( 1 ); // show_scanned_copy_bottom

            // Scanned copy paths
            readFixedString( 200 );
            readFixedString( 200 );

            // DPI and shift values for scanned copies
            skip( 4 * 6 ); // dpi_top, dpi_bottom, shiftx/y_top, shiftx/y_bottom

            // Unknown fields
            skip( 4 * 2 );

            aBoard.center_x = readInt32();
            aBoard.center_y = readInt32();

            aBoard.is_multilayer = readUint8();
        }
        else if( m_fileData.version >= 4 )
        {
            skip( 19 );
            readUint32(); // active_layer
            skip( 7 );    // layer_visible
            skip( 400 );  // unknown_list: 100 * 4 bytes
            skip( 33 );

            aBoard.center_x = readInt32();
            aBoard.center_y = readInt32();
        }
        else if( m_fileData.version >= 3 )
        {
            skip( 19 );
            readUint32(); // active_layer
            skip( 7 );    // layer_visible
            skip( 400 );  // unknown_list: 100 * 4 bytes
            skip( 33 );
        }
    }
    else // Version 2 and older
    {
        aBoard.size_x = readUint32();
        aBoard.size_y = readUint32();
    }
}


void SPRINT_LAYOUT_PARSER::parseObjectsList( SPRINT_LAYOUT::BOARD_DATA& aBoard )
{
    uint32_t numObjects = readUnsigned();

    if( numObjects > MAX_OBJECTS )
        THROW_IO_ERROR( _( "Too many objects in Sprint Layout file" ) );

    aBoard.objects.resize( numObjects );

    for( uint32_t i = 0; i < numObjects; i++ )
        parseObject( aBoard.objects[i], false );
}


void SPRINT_LAYOUT_PARSER::parseGroups( SPRINT_LAYOUT::OBJECT& aObj )
{
    uint32_t groupCount = readUnsigned();

    if( groupCount > MAX_GROUPS )
        THROW_IO_ERROR( _( "Too many groups in Sprint Layout object" ) );

    aObj.groups.resize( groupCount );

    for( uint32_t i = 0; i < groupCount; i++ )
        aObj.groups[i] = readUnsigned();
}


void SPRINT_LAYOUT_PARSER::parsePoints( SPRINT_LAYOUT::OBJECT& aObj )
{
    uint32_t pointCount = readUnsigned();

    if( pointCount > MAX_POINTS )
        THROW_IO_ERROR( _( "Too many points in Sprint Layout object" ) );

    for( uint32_t i = 0; i < pointCount; i++ )
    {
        SPRINT_LAYOUT::POINT pt;
        pt.x = readCoord();
        pt.y = readCoord();

        if( pt.x == 0.0f || std::isnormal( pt.x ) )
        {
            aObj.points.emplace_back( pt );
        }
        else
        {
            seek( -8 );
        }
    }
}


void SPRINT_LAYOUT_PARSER::parseObject( SPRINT_LAYOUT::OBJECT& aObj, bool aIsTextChild )
{
    aObj.type = readUint8();

    if( aObj.type != SPRINT_LAYOUT::OBJ_SEGMENT && aObj.type != SPRINT_LAYOUT::OBJ_THT_PAD
        && aObj.type != SPRINT_LAYOUT::OBJ_OUTLINE_TEXT && aObj.type != SPRINT_LAYOUT::OBJ_POLY
        && aObj.type != SPRINT_LAYOUT::OBJ_CIRCLE && aObj.type != SPRINT_LAYOUT::OBJ_LINE
        && aObj.type != SPRINT_LAYOUT::OBJ_STROKE_TEXT && aObj.type != SPRINT_LAYOUT::OBJ_SMD_PAD )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unknown object type %d in Sprint Layout file" ), aObj.type ) );
    }

    aObj.x = readCoord();
    aObj.y = readCoord();
    aObj.outer = readCoord();
    aObj.inner = readCoord();
    aObj.line_width = readSigned();
    skip( 1 ); // padding
    aObj.layer = readUint8();
    aObj.tht_shape = readUint8();

    if( m_fileData.version >= 5 )
    {
        skip( 4 ); // padding
        aObj.component_id = readUint16();
        skip( 1 );                      // selected
        aObj.start_angle = readInt32(); // also th_style[4]
        skip( 5 );                      // unknown
        aObj.filled = readUint8();
        aObj.clearance = readInt32();
        skip( 5 ); // padding
        aObj.mirror_h = readUint8();
        aObj.mirror_v = readUint8();
        aObj.keepout = readUint8();
        aObj.rotation = readInt32();
        aObj.plated = readUint8();
        aObj.soldermask = readUint8();
        skip( 18 );

        if( !aIsTextChild )
        {
            aObj.text = readVarString();
            aObj.identifier = readVarString();
        }
    }
    else if( m_fileData.version >= 4 )
    {
        skip( 4 ); // padding
        skip( 3 );
        aObj.start_angle = readInt32();
        skip( 5 );
        aObj.filled = readUint8();
        aObj.clearance = readInt32();
        skip( 9 ); // padding
        aObj.mirror_h = readUint8(); // text H mirror
        aObj.mirror_v = readUint8(); // text V mirror
        aObj.keepout = readUint8();
        skip( 18 );

        if( !aIsTextChild )
        {
            aObj.text = readVarString();
        }
    }
    else if( m_fileData.version >= 3 )
    {
        // 50 bytes of data
        if( aObj.type == SPRINT_LAYOUT::OBJ_OUTLINE_TEXT )
        {
            aObj.text = readFixedString( 15 );
            skip( 7 );
            aObj.rotation = readInt16();
            skip( 25 );
        }
        else
        {
            skip( 23 );
            aObj.start_angle = readInt32();
            skip( 3 );
            aObj.mirror_h = readUint8();
            aObj.mirror_v = readUint8();
            skip( 1 );
            aObj.clearance = readInt32();
            skip( 13 );
        }
    }
    else // Versions 1 and 2
    {
        // 35 bytes of data
        if( aObj.type == SPRINT_LAYOUT::OBJ_OUTLINE_TEXT )
        {
            aObj.text = readFixedString( 15 );
            skip( 7 );
            aObj.rotation = readInt16();
            skip( 10 );
        }
        else
        {
            skip( 35 );
        }
    }

    if( m_fileData.version >= 2 && !aIsTextChild )
        parseGroups( aObj );

    switch( aObj.type )
    {
    case SPRINT_LAYOUT::OBJ_OUTLINE_TEXT:
    case SPRINT_LAYOUT::OBJ_SEGMENT:
    case SPRINT_LAYOUT::OBJ_CIRCLE:
    {
        return;
    }

    case SPRINT_LAYOUT::OBJ_POLY:
    case SPRINT_LAYOUT::OBJ_LINE:
    {
        parsePoints( aObj );
        return;
    }

    case SPRINT_LAYOUT::OBJ_THT_PAD:
    {
        if( m_fileData.version >= 5 )
            parsePoints( aObj );
        else if( m_fileData.version >= 3 )
            skip( 4 ); // Usually 0xFFFFFFFF

        return;
    }

    case SPRINT_LAYOUT::OBJ_SMD_PAD:
    {
        if( m_fileData.version >= 5 )
            parsePoints( aObj );

        return; // No points in older versions
    }

    case SPRINT_LAYOUT::OBJ_STROKE_TEXT:
    {
        // Only present since version 3
        uint32_t childCount = readUint32();

        if( childCount > MAX_CHILDREN )
            THROW_IO_ERROR( _( "Too many text children in Sprint Layout object" ) );

        aObj.text_children.resize( childCount );

        for( uint32_t i = 0; i < childCount; i++ )
            parseObject( aObj.text_children[i], true );

        // In v6, component data follows for text objects that define a component
        if( m_fileData.version >= 6 && aObj.tht_shape == 1 )
        {
            aObj.component.valid = true;
            aObj.component.off_x = readCoord();
            aObj.component.off_y = readCoord();
            aObj.component.center_mode = readUint8();
            aObj.component.rotation = readDouble();
            aObj.component.package = readVarString();
            aObj.component.comment = readVarString();
            aObj.component.use = readUint8();
        }

        return;
    }

    default:
    {
        THROW_IO_ERROR( wxString::Format( _( "Unknown object type %d in Sprint Layout file" ), aObj.type ) );
    }
    }
}


void SPRINT_LAYOUT_PARSER::parseTrailer()
{
    if( m_fileData.version >= 4 )
    {
        readUint32(); // active_board_tab
        m_fileData.project_name = readFixedString( 100 );
        m_fileData.project_author = readFixedString( 100 );
        m_fileData.project_company = readFixedString( 100 );
        m_fileData.project_comment = readVarString();
    }
}


// ============================================================================
// Board construction
// ============================================================================

PCB_LAYER_ID SPRINT_LAYOUT_PARSER::mapLayer( uint8_t aSprintLayer ) const
{
    if( m_fileData.version >= 4 )
    {
        switch( aSprintLayer )
        {
        case SPRINT_LAYOUT::LAYER_C1: return F_Cu;
        case SPRINT_LAYOUT::LAYER_S1: return F_SilkS;
        case SPRINT_LAYOUT::LAYER_C2: return B_Cu;
        case SPRINT_LAYOUT::LAYER_S2: return B_SilkS;
        case SPRINT_LAYOUT::LAYER_I1: return In1_Cu;
        case SPRINT_LAYOUT::LAYER_I2: return In2_Cu;
        case SPRINT_LAYOUT::LAYER_O: return Edge_Cuts;
        default: return F_Cu;
        }
    }
    else
    {
        // In older Sprint Layout versions the meaning of C1/C2 is flipped
        switch( aSprintLayer )
        {
        case SPRINT_LAYOUT::LAYER_C1: return m_fileData.version >= 3 ? F_Cu : B_Cu;
        case SPRINT_LAYOUT::LAYER_S1: return F_SilkS;
        case SPRINT_LAYOUT::LAYER_C2: return m_fileData.version >= 3 ? B_Cu : F_Cu;
        case SPRINT_LAYOUT::LAYER_S2: return B_SilkS;
        case SPRINT_LAYOUT::LAYER_O: return Edge_Cuts;

        case SPRINT_LAYOUT::LAYER_I1: return B_Cu;    // used for PTH pads and tracks inside macros
        case SPRINT_LAYOUT::LAYER_I2: return F_SilkS; // used for graphics inside macros
        default: return F_Cu;
        }
    }
}


int SPRINT_LAYOUT_PARSER::sprintToKicadCoord( float aValue ) const
{
    // Sprint Layout 6 uses 1/10000 mm
    // Older versions seem to use 1/100 mm
    // KiCad uses nanometers (1 nm = 1e-6 mm)
    double nm;

    if( m_fileData.version >= 6 )
        nm = static_cast<double>( aValue ) * 100.0; // 100 nm
    else
        nm = static_cast<double>( aValue ) * 10000.0; // 10 um

    nm = std::clamp( nm, static_cast<double>( -pcbIUScale.mmToIU( 500 ) ),
                     static_cast<double>( pcbIUScale.mmToIU( 500 ) ) );

    return KiROUND( nm );
}


VECTOR2I SPRINT_LAYOUT_PARSER::sprintToKicadPos( float aX, float aY ) const
{
    // Sprint Layout uses Y-up (mathematical), KiCad uses Y-down (screen)
    return VECTOR2I( sprintToKicadCoord( aX ), sprintToKicadCoord( -aY ) );
}


wxString SPRINT_LAYOUT_PARSER::convertString( const std::string& aStr ) const
{
    static wxCSConv convCP1251( wxFONTENCODING_CP1251 );
    static wxCSConv convCP1252( wxFONTENCODING_CP1252 );

    if( aStr.empty() )
        return wxEmptyString;

    wxString ret = wxString::FromUTF8( aStr );

    if( ret.empty() && convCP1251.IsOk() && convCP1252.IsOk() )
    {
        // Statistically determine if the string is more likely to be CP1251 (Cyrillic) or CP1252 (Western European)
        size_t extNonGermanCount = 0;

        for( unsigned char c : aStr )
        {
            // Extended-range German characters in CP1252
            switch( c )
            {
            case 0xC4: // Ä
            case 0xD6: // Ö
            case 0xDC: // Ü
            case 0xE4: // ä
            case 0xF6: // ö
            case 0xFC: // ü
            case 0xDF: // ß
                break;

            default:
                if( c >= 0x80 )
                    extNonGermanCount++;
                break;
            }
        }

        if( extNonGermanCount > 0 )
            ret = wxString( aStr.c_str(), convCP1251 );
        else
            ret = wxString( aStr.c_str(), convCP1252 );
    }

    return ret;
}


bool SPRINT_LAYOUT_PARSER::layerHasGroundPlane( PCB_LAYER_ID aLayer, const uint8_t aGroundPlane[7] ) const
{
    // Ground plane index map mirrors CreateBoard()'s groundPlaneMap
    if( m_fileData.version >= 5 )
    {
        switch( aLayer )
        {
        case F_Cu: return aGroundPlane[0] != 0;
        case B_Cu: return aGroundPlane[2] != 0;
        case In1_Cu: return aGroundPlane[4] != 0;
        case In2_Cu: return aGroundPlane[5] != 0;
        default: return false;
        }
    }
    else
    {
        switch( aLayer )
        {
        case F_Cu: return aGroundPlane[0] != 0;
        case B_Cu: return aGroundPlane[1] != 0;
        default: return false;
        }
    }
}


NETINFO_ITEM* SPRINT_LAYOUT_PARSER::resolveItemNet( BOARD* aBoard, const SPRINT_LAYOUT::OBJECT& aObj,
                                                    PCB_LAYER_ID aLayer, const uint8_t aGroundPlane[7],
                                                    NETINFO_ITEM* aGndPlaneNet ) const
{
    if( !aBoard )
        return nullptr;

    bool isPad = aObj.type == SPRINT_LAYOUT::OBJ_THT_PAD || aObj.type == SPRINT_LAYOUT::OBJ_SMD_PAD;

    // Override the net for ground plane connection. Note that the identifier string
    // could specify anything (e.g. component value), not just the net name
    if( aGndPlaneNet != nullptr && layerHasGroundPlane( aLayer, aGroundPlane ) )
    {
        if( aObj.clearance == 0 )
            return aGndPlaneNet;

        // If pad thermal reliefs are enabled, connect to the plane
        if( m_fileData.version >= 5 && isPad && aObj.mirror_h != 0 )
            return aGndPlaneNet;
    }

    // TODO: if a pad is connected through lines to the GND_PLANE, we don't want to set the pad's
    // netname as this would update the nets of the lines, disconnecting them from the plane.
    //
    //if( !aObj.identifier.empty() )
    //{
    //    wxString      netName = convertString( aObj.identifier );
    //    NETINFO_ITEM* net = aBoard->FindNet( netName );

    //    if( !net )
    //    {
    //        net = new NETINFO_ITEM( aBoard, netName );
    //        aBoard->Add( net );
    //    }

    //    return net;
    //}

    return nullptr;
}


BOARD* SPRINT_LAYOUT_PARSER::CreateBoard( std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap,
                                          size_t                                          aBoardIndex )
{
    if( aBoardIndex >= m_fileData.boards.size() )
        return nullptr;

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    // Set up copper layers based on whether inner layers are used
    const SPRINT_LAYOUT::BOARD_DATA& boardData = m_fileData.boards[aBoardIndex];
    bool hasInnerLayers = false;

    for( const SPRINT_LAYOUT::OBJECT& obj : boardData.objects )
    {
        if( obj.layer == SPRINT_LAYOUT::LAYER_I1 || obj.layer == SPRINT_LAYOUT::LAYER_I2 )
        {
            hasInnerLayers = true;
            break;
        }
    }

    if( hasInnerLayers || boardData.is_multilayer )
        board->SetCopperLayerCount( 4 );
    else
        board->SetCopperLayerCount( 2 );

    // Create ground plane zones for layers where ground plane is enabled.
    // Sprint Layout stores a per-layer flag in the board header.
    const wxString              gndPlaneNetName( "GND_PLANE" );
    std::map<int, PCB_LAYER_ID> groundPlaneMap;
    LSET                        groundPlaneLayerSet;
    NETINFO_ITEM*               gndPlaneNet = nullptr;

    if( m_fileData.version >= 5 )
    {
        groundPlaneMap = {
            { 0, F_Cu },
            { 2, B_Cu },
            { 4, In1_Cu },
            { 5, In2_Cu },
        };
    }
    else
    {
        groundPlaneMap = {
            { 0, F_Cu },
            { 1, B_Cu },
        };
    }

    for( const auto& [index, layer] : groundPlaneMap )
    {
        if( boardData.ground_plane[index] != 0 )
            groundPlaneLayerSet.set( layer );
    }

    if( !groundPlaneLayerSet.empty() )
    {
        int w = sprintToKicadCoord( static_cast<float>( boardData.size_x ) );
        int h = sprintToKicadCoord( static_cast<float>( boardData.size_y ) );

        gndPlaneNet = new NETINFO_ITEM( board.get(), gndPlaneNetName );
        board->Add( gndPlaneNet );

        ZONE* zone = new ZONE( board.get() );
        zone->SetLayerSet( groundPlaneLayerSet );
        zone->SetIsRuleArea( false );
        zone->SetZoneName( wxS( "GND_PLANE" ) );
        zone->SetLocalClearance( std::optional<int>( pcbIUScale.mmToIU( 0.3 ) ) );
        zone->SetThermalReliefGap( pcbIUScale.mmToIU( 0.5 ) );
        zone->SetThermalReliefSpokeWidth( pcbIUScale.mmToIU( 0.5 ) );
        zone->SetAssignedPriority( 0 );
        zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
        zone->SetNet( gndPlaneNet );

        SHAPE_POLY_SET outline( BOX2D( VECTOR2D( 0, 0 ), VECTOR2D( w, h ) ) );
        zone->AddPolygon( outline.COutline( 0 ) );
        zone->SetBorderDisplayStyle( ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE, ZONE::GetDefaultHatchPitch(), true );

        board->Add( zone );
    }

    // Maps component_id to FOOTPRINT for grouping component-owned objects
    std::map<uint16_t, FOOTPRINT*>     componentMap;
    std::vector<std::vector<VECTOR2I>> outlineSegments;

    auto getOrCreateComponentFootprint = [&]( const SPRINT_LAYOUT::OBJECT& aObj ) -> FOOTPRINT*
    {
        if( aObj.component_id == 0 )
            return nullptr;

        auto it = componentMap.find( aObj.component_id );

        if( it != componentMap.end() )
            return it->second;

        FOOTPRINT* fp = new FOOTPRINT( board.get() );

        if( aObj.type == SPRINT_LAYOUT::OBJ_STROKE_TEXT && !aObj.text.empty() )
        {
            fp->SetReference( convertString( aObj.text ) );
        }
        else
        {
            fp->SetReference( wxString::Format( wxS( "U%d" ), aObj.component_id ) );

            for( PCB_FIELD* fd : fp->GetFields() )
                fd->SetVisible( false );
        }

        if( aObj.type == SPRINT_LAYOUT::OBJ_STROKE_TEXT && aObj.component.valid )
        {
            if( !aObj.component.comment.empty() )
            {
                wxString comment = convertString( aObj.component.comment );
                fp->GetField( FIELD_T::DESCRIPTION )->SetText( comment );
                fp->SetValue( comment );
            }
            else if( !aObj.identifier.empty() )
            {
                fp->SetValue( convertString( aObj.identifier ) );
            }

            if( !aObj.component.package.empty() )
                fp->SetLibDescription( convertString( aObj.component.package ) );

            fp->SetOrientationDegrees( aObj.component.rotation );
        }

        PCB_LAYER_ID layer = mapLayer( aObj.layer );
        fp->SetLayer( ( layer == B_Cu || layer == B_SilkS ) ? B_Cu : F_Cu );

        componentMap[aObj.component_id] = fp;
        board->Add( fp );
        return fp;
    };

    // First pass: create footprints from component text records where available
    for( const SPRINT_LAYOUT::OBJECT& obj : boardData.objects )
    {
        if( obj.type == SPRINT_LAYOUT::OBJ_STROKE_TEXT && obj.component_id > 0 && obj.component.valid )
            getOrCreateComponentFootprint( obj );
    }

    std::map<uint32_t, std::set<BOARD_ITEM*>> gidToItems;

    // Second pass: process all objects in board/footprint context
    for( const SPRINT_LAYOUT::OBJECT& obj : boardData.objects )
    {
        BOARD_ITEM_CONTAINER* container = board.get();

        if( FOOTPRINT* fp = getOrCreateComponentFootprint( obj ) )
            container = fp;

        // clang-format off
        switch( obj.type )
        {
        case SPRINT_LAYOUT::OBJ_THT_PAD:
        case SPRINT_LAYOUT::OBJ_SMD_PAD:
            processPad( container, obj, boardData.ground_plane, gndPlaneNet, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_SEGMENT:
            processSegment( container, obj, outlineSegments, boardData.ground_plane, gndPlaneNet, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_LINE:
            processLine( container, obj, outlineSegments, boardData.ground_plane, gndPlaneNet, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_POLY:
            processPoly( container, obj, outlineSegments, boardData.ground_plane, gndPlaneNet, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_CIRCLE:
            processCircle( container, obj, outlineSegments, boardData.ground_plane, gndPlaneNet, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_OUTLINE_TEXT:
        case SPRINT_LAYOUT::OBJ_STROKE_TEXT:
            processText( container, obj, gidToItems );
            break;

        default:
            break;
        }
        // clang-format on
    }

    resolveGroups( board.get(), gidToItems );

    // Re-anchor footprints after all elements are added.
    for( FOOTPRINT* fp : board->Footprints() )
    {
        BOX2I fpBbox = fp->GetBoundingHull().BBox();

        VECTOR2I anchor = fpBbox.GetCenter();
        fp->SetPosition( anchor );

        VECTOR2I anchorShift( -anchor.x, -anchor.y );
        RotatePoint( anchorShift, fp->GetOrientation() );
        fp->MoveAnchorPosition( anchorShift );
    }

    // Fill the footprint map
    for( const auto& [componentId, fp] : componentMap )
    {
        wxString   fpKey = wxString::Format( wxS( "SprintLayout_%s" ), fp->GetReference() );
        FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( fp->Clone() );
        fpCopy->SetParent( nullptr );
        aFootprintMap[fpKey] = std::unique_ptr<FOOTPRINT>( fpCopy );
    }

    buildOutline( board.get(), outlineSegments, boardData );

    // Center the board content on the page
    BOX2I bbox = board->ComputeBoundingBox( true );

    if( bbox.GetWidth() > 0 && bbox.GetHeight() > 0 )
    {
        VECTOR2I pageSize = board->GetPageSettings().GetSizeIU( pcbIUScale.IU_PER_MILS );
        VECTOR2I centerOffset = VECTOR2I( pageSize.x / 2, pageSize.y / 2 ) - bbox.GetCenter();

        for( FOOTPRINT* fp : board->Footprints() )
            fp->Move( centerOffset );

        for( ZONE* zone : board->Zones() )
            zone->Move( centerOffset );

        for( BOARD_ITEM* item : board->Drawings() )
            item->Move( centerOffset );
    }

    return board.release();
}


FOOTPRINT* SPRINT_LAYOUT_PARSER::CreateFootprint()
{
    if( m_fileData.boards.empty() )
        return nullptr;

    const SPRINT_LAYOUT::BOARD_DATA& boardData = m_fileData.boards[0];

    std::unique_ptr<FOOTPRINT> fp = std::make_unique<FOOTPRINT>( nullptr );

    wxString fpName = convertString( boardData.name );

    fp->SetFPID( LIB_ID( wxEmptyString, fpName ) );
    fp->SetReference( wxT( "REF**" ) );
    fp->SetValue( fpName );
    fp->Reference().SetVisible( true );
    fp->Value().SetVisible( true );

    std::vector<std::vector<VECTOR2I>>        outlineSegments;
    uint8_t                                   groundPlane[7] = {};
    std::map<uint32_t, std::set<BOARD_ITEM*>> gidToItems;

    for( const SPRINT_LAYOUT::OBJECT& obj : boardData.objects )
    {
        BOARD_ITEM_CONTAINER* container = fp.get();
        
        // clang-format off
        switch( obj.type )
        {
        case SPRINT_LAYOUT::OBJ_THT_PAD:
        case SPRINT_LAYOUT::OBJ_SMD_PAD:
            processPad( container, obj, groundPlane, nullptr, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_SEGMENT:
            processSegment( container, obj, outlineSegments, groundPlane, nullptr, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_LINE:
            processLine( container, obj, outlineSegments, groundPlane, nullptr, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_POLY:
            processPoly( container, obj, outlineSegments, groundPlane, nullptr, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_CIRCLE:
            processCircle( container, obj, outlineSegments, groundPlane, nullptr, gidToItems );
            break;

        case SPRINT_LAYOUT::OBJ_OUTLINE_TEXT:
        case SPRINT_LAYOUT::OBJ_STROKE_TEXT:
            processText( container, obj, gidToItems );
            break;

        default:
            break;
        }
        // clang-format on
    }

    resolveGroups( fp.get(), gidToItems );

    fp->AutoPositionFields();

    // Generate basic courtyard rectangle
    BOX2I bbox = fp->GetBoundingHull().BBox();
    bbox.Inflate( pcbIUScale.mmToIU( 0.25 ) ); // Default courtyard clearance

    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( fp.get(), SHAPE_T::RECTANGLE );
    shape->SetWidth( pcbIUScale.mmToIU( DEFAULT_COURTYARD_WIDTH ) );
    shape->SetLayer( F_CrtYd );
    shape->SetStart( bbox.GetOrigin() );
    shape->SetEnd( bbox.GetEnd() );

    fp->Add( shape.release(), ADD_MODE::APPEND );

    return fp.release();
}


void SPRINT_LAYOUT_PARSER::processPad( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                       const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet,
                                       std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    BOARD*     board = aContainer ? aContainer->GetBoard() : nullptr;
    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aContainer );
    bool       standaloneFp = false;

    if( !fp )
    {
        // Standalone pad without a component gets its own footprint
        standaloneFp = true;
        fp = new FOOTPRINT( board );
        fp->SetReference( wxString::Format( wxS( "PAD%d" ), static_cast<int>( board->Footprints().size() ) ) );
        fp->Reference().SetVisible( false );
        fp->SetLayer( F_Cu );
        aContainer->Add( fp );
    }

    PAD* pad = new PAD( fp );

    // SMD pad x,y may be a component-relative offset rather than an absolute
    // position (depends on the Sprint Layout version that created the file).
    // The points array always stores absolute coordinates, so derive the pad
    // center from the points when available.
    // The rotation field for pads (both TH and SMD) is unknown, so detect
    // the pad angle from the points when possible.
    VECTOR2I  ptsCenter;
    EDA_ANGLE ptsAngle;

    if( !aObj.points.empty() )
    {
        double cx = 0, cy = 0;

        for( const auto& pt : aObj.points )
        {
            cx += pt.x;
            cy += pt.y;
        }

        cx /= static_cast<double>( aObj.points.size() );
        cy /= static_cast<double>( aObj.points.size() );
        ptsCenter = sprintToKicadPos( static_cast<float>( cx ), static_cast<float>( cy ) );

        std::vector<VECTOR2I> pts;

        for( const SPRINT_LAYOUT::POINT& pt : aObj.points )
            pts.emplace_back( sprintToKicadPos( pt.x, pt.y ) );

        if( pts.size() == 2 ) // Oval or circle
        {
            ptsAngle = EDA_ANGLE( pts[1] - pts[0] );

            if( aObj.type == SPRINT_LAYOUT::OBJ_THT_PAD && aObj.tht_shape == SPRINT_LAYOUT::THT_SHAPE_V_ROUND )
                ptsAngle -= ANGLE_90;
        }
        else if( pts.size() == 4 ) // Rectangular
        {
            ptsAngle = EDA_ANGLE( pts[1] - pts[0] );
        }
        else if( pts.size() == 8 ) // Octagonal
        {
            ptsAngle = EDA_ANGLE( pts[2] - pts[1] );
        }
        else
        {
            wxFAIL_MSG( wxString::Format( "Unknown pad type %d shape %d with %zu points", int( aObj.type ),
                                          int( aObj.tht_shape ), aObj.points.size() ) );
        }

        ptsAngle = ptsAngle.Round( 2 );
    }

    PCB_LAYER_ID padLayer = mapLayer( aObj.layer );
    VECTOR2I     padPos = sprintToKicadPos( aObj.x, aObj.y );

    if( aObj.type == SPRINT_LAYOUT::OBJ_THT_PAD )
    {
        int outerDia = sprintToKicadCoord( aObj.outer * 2.0f );
        int drillDia = sprintToKicadCoord( aObj.inner * 2.0f );

        bool isPTH = aObj.plated != 0 || ( m_fileData.version <= 3 && aObj.layer == 5 );

        if( isPTH )
        {
            pad->SetAttribute( PAD_ATTRIB::PTH );
            pad->SetLayerSet( PAD::PTHMask() );
        }
        else
        {
            pad->SetAttribute( drillDia > 0 ? PAD_ATTRIB ::NPTH : PAD_ATTRIB::SMD );

            if( padLayer == F_Cu )
                pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
            else if( padLayer == B_Cu )
                pad->SetLayerSet( LSET( { B_Cu, B_Mask } ) );
            else
                pad->SetLayerSet( LSET( { padLayer } ) );

            if( standaloneFp && IsBackLayer( padLayer ) )
                fp->SetLayer( B_Cu );
        }

        VECTOR2I padSize( outerDia, outerDia );
        VECTOR2I drillSize( drillDia, drillDia );

        switch( aObj.tht_shape )
        {
        case SPRINT_LAYOUT::THT_SHAPE_H_ROUND:
        case SPRINT_LAYOUT::THT_SHAPE_H_CHAMFER:
        case SPRINT_LAYOUT::THT_SHAPE_H_RECT: 
            padSize.x *= 2;
            break;

        case SPRINT_LAYOUT::THT_SHAPE_V_ROUND:
        case SPRINT_LAYOUT::THT_SHAPE_V_CHAMFER:
        case SPRINT_LAYOUT::THT_SHAPE_V_RECT: 
            padSize.y *= 2;
            break;

        default: break;
        }

        pad->SetSize( PADSTACK::ALL_LAYERS, padSize );
        pad->SetDrillSize( drillSize );

        switch( aObj.tht_shape )
        {
        case SPRINT_LAYOUT::THT_SHAPE_CIRCLE:
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            break;

        case SPRINT_LAYOUT::THT_SHAPE_H_ROUND:
        case SPRINT_LAYOUT::THT_SHAPE_V_ROUND:
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::OVAL );
            break;

        case SPRINT_LAYOUT::THT_SHAPE_OCT:
        case SPRINT_LAYOUT::THT_SHAPE_H_CHAMFER:
        case SPRINT_LAYOUT::THT_SHAPE_V_CHAMFER:
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CHAMFERED_RECT );
            pad->SetChamferRectRatio( PADSTACK::ALL_LAYERS, 0.25 );
            pad->SetChamferPositions( PADSTACK::ALL_LAYERS,
                                      RECT_CHAMFER_ALL );
            break;

        case SPRINT_LAYOUT::THT_SHAPE_SQUARE:
        case SPRINT_LAYOUT::THT_SHAPE_H_RECT:
        case SPRINT_LAYOUT::THT_SHAPE_V_RECT:
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
            break;

        default:
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
            break;
        }

        pad->SetPosition( padPos );
        pad->Rotate( padPos, -ptsAngle );
    }
    else
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );

        if( padLayer == F_Cu )
            pad->SetLayerSet( LSET( { F_Cu, F_Paste, F_Mask } ) );
        else if( padLayer == B_Cu )
            pad->SetLayerSet( LSET( { B_Cu, B_Paste, B_Mask } ) );
        else
            pad->SetLayerSet( LSET( { padLayer } ) );

        if( standaloneFp && IsBackLayer( padLayer ) )
            fp->SetLayer( B_Cu );

        pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );

        int width = sprintToKicadCoord( aObj.outer );
        int height = sprintToKicadCoord( aObj.inner );

        if( height <= 0 )
            height = width;

        if( !aObj.points.empty() )
            padPos = ptsCenter;

        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( width, height ) );
        pad->SetPosition( padPos );
        pad->Rotate( padPos, -ptsAngle );
    }

    // Solder mask: soldermask==0 means no mask opening (pad is tented/covered)
    if( aObj.soldermask == 0 )
    {
        pad->Padstack().FrontOuterLayers().has_solder_mask = false;
        pad->Padstack().BackOuterLayers().has_solder_mask = false;
    }

    // Per-element ground plane clearance
    if( aObj.clearance > 0 )
    {
        int clearance = sprintToKicadCoord( static_cast<float>( aObj.clearance ) );
        pad->SetLocalClearance( std::optional<int>( clearance ) );
        pad->SetLocalThermalGapOverride( std::optional<int>( clearance ) );
    }

    // Thermal reliefs
    if( m_fileData.version >= 5 && aObj.mirror_h != 0 )
    {
        int spokeWidth = aObj.rotation * 10000 / 2;
        pad->SetLocalThermalSpokeWidthOverride( spokeWidth );
        
        // Each byte is the spoke directions for one copper layer (C1, C2, I1, I2).
        // 0x55 matches H/V directions, 0xAA matches diagonal directions
        uint32_t spokeMask = static_cast<uint32_t>( aObj.start_angle );

        if( spokeMask != 0 )
        {
            pad->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );

            if( spokeMask & 0x55555555 )
                pad->SetThermalSpokeAngle( ANGLE_90 );
            else if( spokeMask & 0xAAAAAAAA )
                pad->SetThermalSpokeAngle( ANGLE_45 );
        }
        else
        {
            pad->SetLocalZoneConnection( ZONE_CONNECTION::NONE );
        }
    }
    else
    {
        pad->SetLocalZoneConnection( ZONE_CONNECTION::FULL );
        pad->SetThermalSpokeAngle( ANGLE_90 );
    }

    // Set net name. Plated THT pads span all copper layers, so accept the GND
    // default when any configured ground-plane layer is enabled; SMD and NPTH
    // pads only qualify on their own layer.
    PCB_LAYER_ID netLayer = padLayer;

    if( aObj.type == SPRINT_LAYOUT::OBJ_THT_PAD && aObj.plated != 0
        && !layerHasGroundPlane( padLayer, aGroundPlane ) )
    {
        if( aGroundPlane[0] )
            netLayer = F_Cu;
        else if( aGroundPlane[2] )
            netLayer = B_Cu;
        else if( aGroundPlane[4] )
            netLayer = In1_Cu;
        else if( aGroundPlane[5] )
            netLayer = In2_Cu;
    }

    if( NETINFO_ITEM* net = resolveItemNet( board, aObj, netLayer, aGroundPlane, aGndPlaneNet ) )
        pad->SetNet( net );

    pad->SetNumber( wxString::Format( wxS( "%d" ), static_cast<int>( fp->Pads().size() + 1 ) ) );

    fp->Add( pad );

    if( standaloneFp )
    {
        for( PCB_FIELD* fd : fp->GetFields() )
            fd->SetTextPos( pad->GetPosition() );

        processItemGroups( fp, aObj, aGidToItems );
    }
    else
    {
        processItemGroups( pad, aObj, aGidToItems );
    }
}


void SPRINT_LAYOUT_PARSER::processLine( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                        std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                                        const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet,
                                        std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    if( aObj.points.size() < 2 )
        return;

    BOARD* board = aContainer ? aContainer->GetBoard() : nullptr;
    PCB_LAYER_ID layer = mapLayer( aObj.layer );

    if( layer == Edge_Cuts )
    {
        std::vector<VECTOR2I> segment;

        for( const auto& pt : aObj.points )
            segment.push_back( sprintToKicadPos( pt.x, pt.y ) );

        aOutlineSegments.push_back( std::move( segment ) );
        return;
    }

    int width = sprintToKicadCoord( static_cast<float>( aObj.line_width ) );

    if( width <= 0 )
        width = pcbIUScale.mmToIU( 0.25 );

    for( size_t i = 0; i + 1 < aObj.points.size(); i++ )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( aContainer );
        shape->SetShape( SHAPE_T::SEGMENT );
        shape->SetLayer( layer );
        shape->SetWidth( width );
        shape->SetStart( sprintToKicadPos( aObj.points[i].x, aObj.points[i].y ) );
        shape->SetEnd( sprintToKicadPos( aObj.points[i + 1].x, aObj.points[i + 1].y ) );

        if( IsCopperLayer( layer ) )
        {
            if( NETINFO_ITEM* net = resolveItemNet( board, aObj, layer, aGroundPlane, aGndPlaneNet ) )
                shape->SetNet( net );
        }

        aContainer->Add( shape );
        processItemGroups( shape, aObj, aGidToItems );
    }
}


void SPRINT_LAYOUT_PARSER::processSegment( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                           std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                                           const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet,
                                           std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    PCB_LAYER_ID layer = mapLayer( aObj.layer );

    if( layer == Edge_Cuts )
    {
        std::vector<VECTOR2I> seg;
        seg.push_back( sprintToKicadPos( aObj.x, aObj.y ) );
        seg.push_back( sprintToKicadPos( aObj.outer, aObj.inner ) );
        aOutlineSegments.push_back( std::move( seg ) );
        return;
    }

    VECTOR2I start = sprintToKicadPos( aObj.x, aObj.y );
    VECTOR2I end = sprintToKicadPos( aObj.outer, aObj.inner );
    int      width = sprintToKicadCoord( static_cast<float>( aObj.line_width ) );

    // Skip the dummy segment at 0,0 in version 1 files
    if( aObj.line_width == 0 && start == end )
        return;

    PCB_SHAPE* shape = new PCB_SHAPE( aContainer );
    shape->SetShape( SHAPE_T::SEGMENT );
    shape->SetLayer( layer );
    shape->SetWidth( width );
    shape->SetStart( start );
    shape->SetEnd( end );

    aContainer->Add( shape );
    processItemGroups( shape, aObj, aGidToItems );
}


void SPRINT_LAYOUT_PARSER::processPoly( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                        std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                                        const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet,
                                        std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    if( aObj.points.size() < 2 )
        return;

    BOARD*       board = aContainer ? aContainer->GetBoard() : nullptr;
    PCB_LAYER_ID layer = mapLayer( aObj.layer );

    if( layer == Edge_Cuts )
    {
        std::vector<VECTOR2I> points;

        for( const auto& pt : aObj.points )
            points.push_back( sprintToKicadPos( pt.x, pt.y ) );

        points.push_back( points[0] );

        aOutlineSegments.push_back( std::move( points ) );
        return;
    }

    //bool isFilled = ( aObj.filled != 0 );
    bool isCutout = ( aObj.keepout != 0 );

    int width = sprintToKicadCoord( static_cast<float>( aObj.line_width ) );

    if( width < 0 )
        width = pcbIUScale.mmToIU( 0.25 );

    SHAPE_LINE_CHAIN outline;

    for( const auto& pt : aObj.points )
    {
        VECTOR2I pos = sprintToKicadPos( pt.x, pt.y );
        outline.Append( pos.x, pos.y );
    }

    outline.SetClosed( true ); // Deduplicate the last point properly

    if( isCutout && IsCopperLayer( layer ) && aObj.points.size() >= 3 )
    {
        // Cutout area for ground plane exclusion -> rule area (keepout zone)
        ZONE* zone = new ZONE( aContainer );
        zone->SetLayer( layer );
        zone->SetIsRuleArea( true );
        zone->SetDoNotAllowZoneFills( true );
        zone->SetDoNotAllowTracks( false );
        zone->SetDoNotAllowVias( false );
        zone->SetDoNotAllowPads( false );
        zone->SetDoNotAllowFootprints( false );

        zone->AddPolygon( outline );
        aContainer->Add( zone );
        processItemGroups( zone, aObj, aGidToItems );
    }
    else if( aObj.points.size() >= 3 )
    {
        // Filled polygon on non-copper layer -> filled PCB_SHAPE
        PCB_SHAPE* shape = new PCB_SHAPE( aContainer );
        shape->SetShape( SHAPE_T::POLY );
        shape->SetFilled( true );
        shape->SetLayer( layer );
        shape->SetWidth( width );

        shape->SetPolyShape( SHAPE_POLY_SET( outline ) );

        if( NETINFO_ITEM* net = resolveItemNet( board, aObj, layer, aGroundPlane, aGndPlaneNet ) )
            shape->SetNet( net );

        aContainer->Add( shape );
        processItemGroups( shape, aObj, aGidToItems );
    }
}


void SPRINT_LAYOUT_PARSER::processCircle( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                          std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                                          const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet,
                                          std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    BOARD* board = aContainer ? aContainer->GetBoard() : nullptr;
    PCB_LAYER_ID layer = mapLayer( aObj.layer );
    VECTOR2I center = sprintToKicadPos( aObj.x, aObj.y );
    float radius = ( aObj.outer + aObj.inner ) / 2.0f;
    int kiRadius = sprintToKicadCoord( radius );
    int width = sprintToKicadCoord( aObj.outer - aObj.inner );

    if( width <= 0 )
        width = pcbIUScale.mmToIU( 0.25 );

    bool   isFullCircle = true;
    double startAngleDeg = 0, endAngleDeg = 0;

    if( m_fileData.version >= 3 )
    {
        startAngleDeg = aObj.start_angle;
        endAngleDeg = aObj.line_width;

        if( m_fileData.version >= 6 )
        {
            // There's nothing else in the format to specify the angle scale
            // It's either in 1 degree or 0.001 degree units
            if( startAngleDeg > 1000 || startAngleDeg < -1000 || endAngleDeg > 1000 || endAngleDeg < -1000 )
            {
                startAngleDeg /= 1000;
                endAngleDeg /= 1000;
            }
        }
        // Older versions always use 1 degree units

        isFullCircle = ( startAngleDeg == 0 && endAngleDeg == 0 )
                       || ( endAngleDeg - startAngleDeg >= 360 )
                       || ( startAngleDeg == endAngleDeg );
    }
    // Older versions do not have arcs

    if( layer == Edge_Cuts )
    {
        // Approximate arcs as line segments for outline reconstruction
        std::vector<VECTOR2I> segment;

        if( isFullCircle )
        {
            for( int i = 0; i <= 24; i++ )
            {
                double angle = ( static_cast<double>( i ) / 24.0 ) * 2.0 * M_PI;
                int px = center.x + static_cast<int>( std::cos( angle ) * kiRadius );
                int py = center.y - static_cast<int>( std::sin( angle ) * kiRadius );
                segment.emplace_back( px, py );
            }
        }
        else
        {
            int32_t sa = startAngleDeg * 1000;
            int32_t ea = endAngleDeg * 1000;

            if( ea <= sa )
                ea += 360000;

            for( int32_t a = sa; a <= ea; a += 15000 )
            {
                double rad = ( static_cast<double>( a ) / 1000.0 ) * M_PI / 180.0;
                int px = center.x + static_cast<int>( std::cos( rad ) * kiRadius );
                int py = center.y - static_cast<int>( std::sin( rad ) * kiRadius );
                segment.emplace_back( px, py );
            }

            double endRad = ( static_cast<double>( ea ) / 1000.0 ) * M_PI / 180.0;
            int epx = center.x + static_cast<int>( std::cos( endRad ) * kiRadius );
            int epy = center.y - static_cast<int>( std::sin( endRad ) * kiRadius );
            segment.emplace_back( epx, epy );
        }

        aOutlineSegments.push_back( std::move( segment ) );
        return;
    }

    PCB_SHAPE* shape = new PCB_SHAPE( aContainer );
    shape->SetLayer( layer );
    shape->SetWidth( width );

    if( isFullCircle )
    {
        shape->SetShape( SHAPE_T::CIRCLE );
        shape->SetCenter( center );
        shape->SetEnd( VECTOR2I( center.x + kiRadius, center.y ) );
    }
    else
    {
        shape->SetShape( SHAPE_T::ARC );
        shape->SetCenter( center );

        // Y-flip reverses angular direction, so negate start angle
        double startRad = startAngleDeg * M_PI / 180.0;
        int    sx = center.x + static_cast<int>( std::cos( startRad ) * kiRadius );
        int    sy = center.y - static_cast<int>( std::sin( startRad ) * kiRadius );
        shape->SetStart( VECTOR2I( sx, sy ) );

        double newEndAngle = endAngleDeg;

        if( newEndAngle < startAngleDeg )
            newEndAngle += 360;

        // Negate arc angle for Y-flip (reverses sweep direction)
        double arcAngle = startAngleDeg - newEndAngle;
        shape->SetArcAngleAndEnd( EDA_ANGLE( arcAngle, DEGREES_T ), true );
    }

    if( IsCopperLayer( layer ) )
    {
        if( NETINFO_ITEM* net = resolveItemNet( board, aObj, layer, aGroundPlane, aGndPlaneNet ) )
            shape->SetNet( net );
    }

    aContainer->Add( shape );
    processItemGroups( shape, aObj, aGidToItems );
}


void SPRINT_LAYOUT_PARSER::processText( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                        std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aContainer );

    // Skip component reference/value text only when it is not attached to a footprint.
    if( aObj.component_id > 0 && !fp )
        return;

    PCB_LAYER_ID layer = mapLayer( aObj.layer );

    if( layer == Edge_Cuts )
        return;

    PCB_TEXT* text = nullptr;
    bool      add = false;

    if( aObj.component_id > 0 && fp && aObj.type == SPRINT_LAYOUT::OBJ_STROKE_TEXT && aObj.tht_shape > 0
        && aObj.tht_shape <= 2 )
    {
        if( aObj.tht_shape == 1 )
            text = &fp->Reference();
        else if( aObj.tht_shape == 2 )
            text = &fp->Value();
    }
    else
    {
        if( aObj.text.empty() )
            return;

        text = new PCB_TEXT( aContainer );
        add = true;
    }

    if( !text )
        return;

    // When inside a group, the rotation center seems to be at the group center.
    // Just so we don't have to do a complex fixup later, use points to detect 
    // text center instead, they are always in absolute coordinates.
    VECTOR2I ptsCenter;

    if( !aObj.text_children.empty() )
    {
        double cx = 0, cy = 0;
        size_t ptsCount = 0;

        for( const SPRINT_LAYOUT::OBJECT& child : aObj.text_children )
        {
            if( child.type == SPRINT_LAYOUT::OBJ_LINE )
            {
                for( const SPRINT_LAYOUT::POINT& pt : child.points )
                {
                    cx += pt.x;
                    cy += pt.y;
                    ptsCount += 1;
                }
            }
            else if( child.type == SPRINT_LAYOUT::OBJ_SEGMENT )
            {
                cx += child.x;
                cy += child.y;
                cx += child.outer;
                cy += child.inner;
                ptsCount += 2;
            }
        }

        cx /= static_cast<double>( ptsCount );
        cy /= static_cast<double>( ptsCount );
        ptsCenter = sprintToKicadPos( static_cast<float>( cx ), static_cast<float>( cy ) );
    }

    text->SetLayer( layer );
    text->SetText( convertString( aObj.text ) );
    text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    text->SetKeepUpright( false );
    text->SetVisible( aObj.component_id == 0 || aObj.filled != 0 );

    if( aObj.type == SPRINT_LAYOUT::OBJ_STROKE_TEXT )
    {
        int height = sprintToKicadCoord( aObj.outer ) * 0.75;

        if( height <= 0 )
            height = pcbIUScale.mmToIU( 1.0 );

        double widthScale = 0.8 + 0.2 * aObj.line_width;
        text->SetTextSize( VECTOR2I( height * widthScale, height ) );

        double thicknessScale = 0.06 + 0.05 * aObj.inner;
        int    thickness = height * thicknessScale;

        if( thickness <= 0 )
            thickness = std::max( 1, height / 8 );

        text->SetTextThickness( thickness );
    }
    else
    {
        // -133 maps to 1 mm height
        int normalized = std::abs( aObj.line_width ) * 100 / 133;
        int height = sprintToKicadCoord( normalized ) * 0.75;

        if( aObj.line_width < 0 ) // Seems to be always
            text->SetVertJustify( GR_TEXT_V_ALIGN_TOP );

        text->SetTextSize( VECTOR2I( height, height ) );
        text->SetTextThickness( height / 8 );
    }

    VECTOR2I untransformedPos = sprintToKicadPos( aObj.x, aObj.y );
    text->SetTextPos( untransformedPos );
    VECTOR2I untransformedCenter = text->GetCenter();

    VECTOR2I newCenter = !aObj.text_children.empty() ? ptsCenter : untransformedCenter;
    text->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
    text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
    text->SetTextPos( newCenter );

    int rotation = 0;

    if( m_fileData.version == 4 )
        rotation = aObj.start_angle * 90;
    else
        rotation = aObj.rotation;

    bool mirrorH = aObj.mirror_h != 0;
    bool mirrorV = aObj.mirror_v != 0;

    if( mirrorH ^ mirrorV )
    {
        text->SetMirrored( true );
        text->SetHorizJustify( (GR_TEXT_H_ALIGN_T) -text->GetHorizJustify() );
        rotation = -rotation;
    }

    if( mirrorV )
        text->Rotate( newCenter, ANGLE_180 );

    text->Rotate( newCenter, EDA_ANGLE( -rotation, DEGREES_T ) );

    if( add )
    {
        aContainer->Add( text );
        processItemGroups( text, aObj, aGidToItems );
    }
}


void SPRINT_LAYOUT_PARSER::processItemGroups( BOARD_ITEM* aItem, const SPRINT_LAYOUT::OBJECT& aObj,
                                              std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    for( uint32_t gid : aObj.groups )
        aGidToItems[gid].insert( aItem );
}


void SPRINT_LAYOUT_PARSER::resolveGroups( BOARD_ITEM_CONTAINER*                      aContainer,
                                          std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems )
{
    std::map<uint32_t, PCB_GROUP*> gidGroupMap;
    std::vector<uint32_t>          gidAscBySize;

    for( const auto& [gid, _] : aGidToItems )
    {
        PCB_GROUP* group = new PCB_GROUP( aContainer );

        gidGroupMap[gid] = group;
        gidAscBySize.push_back( gid );

        if( aContainer )
            aContainer->Add( group );
    }

    // Process groups in ascending member-count order (tie-break by group id) so smaller
    // groups attach first and larger groups can adopt them, producing stable nesting.
    std::sort( gidAscBySize.begin(), gidAscBySize.end(),
               [&]( uint32_t gidA, uint32_t gidB )
               {
                   size_t sa = aGidToItems.at( gidA ).size();
                   size_t sb = aGidToItems.at( gidB ).size();

                   if( sa != sb )
                       return sa < sb;

                   return gidA < gidB;
               } );

    for( uint32_t gid : gidAscBySize )
    {
        PCB_GROUP* grp = gidGroupMap[gid];

        for( BOARD_ITEM* item : aGidToItems[gid] )
        {
            if( PCB_GROUP* itemGroup = static_cast<PCB_GROUP*>( item->GetParentGroup() ) )
            {
                if( itemGroup != grp )
                    grp->AddItem( itemGroup );
            }
            else
            {
                // Only add if we don't cross board-footprint boundaries
                if( item->GetParent() == grp->GetParent() )
                    grp->AddItem( item );
            }
        }
    }
}


void SPRINT_LAYOUT_PARSER::buildOutline( BOARD* aBoard, std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                                         const SPRINT_LAYOUT::BOARD_DATA& aBoardData )
{
    // Try to join outline segments into closed polygons
    // Similar to OpenBoardView's outline_order_segments algorithm
    static const int PROXIMITY_DELTA = 100; // 100 nm tolerance

    auto closeEnough = []( const VECTOR2I& a, const VECTOR2I& b, int delta ) -> bool
    {
        return std::abs( a.x - b.x ) < delta && std::abs( a.y - b.y ) < delta;
    };

    // Try to join segments end-to-end. After each successful join, restart
    // the inner scan because seg.back() has changed.
    for( size_t iterations = 0; iterations < aOutlineSegments.size(); iterations++ )
    {
        bool joined = false;

        for( auto& seg : aOutlineSegments )
        {
            if( seg.size() < 2 )
                continue;

            for( auto& other : aOutlineSegments )
            {
                if( &seg == &other || other.empty() )
                    continue;

                bool frontMatch = closeEnough( seg.back(), other.front(), PROXIMITY_DELTA );
                bool backMatch = !frontMatch
                                 && closeEnough( seg.back(), other.back(), PROXIMITY_DELTA );

                if( backMatch )
                {
                    std::reverse( other.begin(), other.end() );
                    frontMatch = true;
                }

                if( !frontMatch )
                    continue;

                if( seg.back() == other.front() )
                    seg.insert( seg.end(), other.begin() + 1, other.end() );
                else
                    seg.insert( seg.end(), other.begin(), other.end() );

                other.clear();
                joined = true;
                break;
            }
        }

        if( !joined )
            break;
    }

    bool hasOutline = false;

    for( const auto& seg : aOutlineSegments )
    {
        if( seg.size() < 2 )
            continue;

        hasOutline = true;

        for( size_t i = 0; i + 1 < seg.size(); i++ )
        {
            PCB_SHAPE* shape = new PCB_SHAPE( aBoard );
            shape->SetShape( SHAPE_T::SEGMENT );
            shape->SetLayer( Edge_Cuts );
            shape->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
            shape->SetStart( seg[i] );
            shape->SetEnd( seg[i + 1] );
            aBoard->Add( shape );
        }
    }

    // Fallback: create rectangular outline from board dimensions
    if( !hasOutline )
    {
        int w = sprintToKicadCoord( static_cast<float>( aBoardData.size_x ) );
        int h = sprintToKicadCoord( static_cast<float>( aBoardData.size_y ) );

        if( w > 0 && h > 0 )
        {
            VECTOR2I corners[4] = {
                { 0, 0 },
                { w, 0 },
                { w, h },
                { 0, h }
            };

            for( int i = 0; i < 4; i++ )
            {
                PCB_SHAPE* shape = new PCB_SHAPE( aBoard );
                shape->SetShape( SHAPE_T::SEGMENT );
                shape->SetLayer( Edge_Cuts );
                shape->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
                shape->SetStart( corners[i] );
                shape->SetEnd( corners[( i + 1 ) % 4] );
                aBoard->Add( shape );
            }
        }
    }
}

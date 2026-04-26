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
#include <pcb_shape.h>
#include <pcb_text.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>
#include <math/util.h>
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

bool SPRINT_LAYOUT_PARSER::Parse( const wxString& aFileName )
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

    uint32_t numBoards = readUint32();

    if( numBoards == 0 || numBoards > 100 )
        THROW_IO_ERROR( _( "Invalid board count in Sprint Layout file" ) );

    m_fileData.boards.resize( numBoards );

    for( uint32_t b = 0; b < numBoards; b++ )
    {
        parseBoardHeader( m_fileData.boards[b] );

        uint32_t numConnections = 0;

        for( auto& obj : m_fileData.boards[b].objects )
        {
            if( obj.type == SPRINT_LAYOUT::OBJ_THT_PAD || obj.type == SPRINT_LAYOUT::OBJ_SMD_PAD )
                numConnections++;
        }

        // Read connection records (one per pad object)
        for( uint32_t c = 0; c < numConnections; c++ )
        {
            uint32_t connCount = readUint32();

            // Skip the connection data for now
            skip( connCount * sizeof( uint32_t ) );
        }
    }

    parseTrailer();

    return true;
}


void SPRINT_LAYOUT_PARSER::parseBoardHeader( SPRINT_LAYOUT::BOARD_DATA& aBoard )
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

    uint32_t numObjects = readUint32();

    if( numObjects > MAX_OBJECTS )
        THROW_IO_ERROR( _( "Too many objects in Sprint Layout board" ) );

    aBoard.objects.resize( numObjects );

    for( uint32_t i = 0; i < numObjects; i++ )
        parseObject( aBoard.objects[i] );
}


void SPRINT_LAYOUT_PARSER::parseObject( SPRINT_LAYOUT::OBJECT& aObj, bool aIsTextChild )
{
    aObj.type = readUint8();

    if( aObj.type != SPRINT_LAYOUT::OBJ_THT_PAD && aObj.type != SPRINT_LAYOUT::OBJ_POLY
        && aObj.type != SPRINT_LAYOUT::OBJ_CIRCLE && aObj.type != SPRINT_LAYOUT::OBJ_LINE
        && aObj.type != SPRINT_LAYOUT::OBJ_TEXT && aObj.type != SPRINT_LAYOUT::OBJ_SMD_PAD )
    {
        THROW_IO_ERROR( wxString::Format( _( "Unknown object type %d in Sprint Layout file" ),
                                          aObj.type ) );
    }

    aObj.x = readFloat();
    aObj.y = readFloat();
    aObj.outer = readFloat();
    aObj.inner = readFloat();
    aObj.line_width = readUint32();
    skip( 1 );  // padding
    aObj.layer = readUint8();
    aObj.tht_shape = readUint8();
    skip( 4 );  // padding
    aObj.component_id = readUint16();
    skip( 1 );  // selected
    aObj.start_angle = readInt32(); // also th_style[4]
    skip( 5 );  // unknown
    aObj.filled = readUint8();
    aObj.clearance = readInt32();
    skip( 5 );  // unknown
    aObj.thermal_width = readUint8();
    aObj.mirror = readUint8();
    aObj.keepout = readUint8();
    aObj.rotation = readInt32(); // thzise
    aObj.plated = readUint8();
    aObj.soldermask = readUint8();
    skip( 18 ); // unknown padding

    // Variable-length data after header
    if( !aIsTextChild )
    {
        aObj.text = readVarString();
        aObj.net_name = readVarString();

        uint32_t groupCount = readUint32();

        if( groupCount > MAX_GROUPS )
            THROW_IO_ERROR( _( "Too many groups in Sprint Layout object" ) );

        aObj.groups.resize( groupCount );

        for( uint32_t i = 0; i < groupCount; i++ )
            aObj.groups[i] = readUint32();
    }

    switch( aObj.type )
    {
    case SPRINT_LAYOUT::OBJ_CIRCLE:
        // Circles have no points list
        return;

    case SPRINT_LAYOUT::OBJ_TEXT:
    {
        uint32_t childCount = readUint32();

        if( childCount > MAX_CHILDREN )
            THROW_IO_ERROR( _( "Too many text children in Sprint Layout object" ) );

        aObj.text_children.resize( childCount );

        for( uint32_t i = 0; i < childCount; i++ )
            parseObject( aObj.text_children[i], true );

        // Component data follows for text objects that define a component
        if( aObj.tht_shape == 1 )
        {
            aObj.component.valid = true;
            aObj.component.off_x = readFloat();
            aObj.component.off_y = readFloat();
            aObj.component.center_mode = readUint8();
            aObj.component.rotation = readDouble();
            aObj.component.package = readVarString();
            aObj.component.comment = readVarString();
            aObj.component.use = readUint8();
        }

        return;
    }

    default:
        break;
    }

    // Points list for pads, lines, and polygons
    uint32_t pointCount = readUint32();

    if( pointCount > MAX_POINTS )
        THROW_IO_ERROR( _( "Too many points in Sprint Layout object" ) );

    for( uint32_t i = 0; i < pointCount; i++ )
    {
        SPRINT_LAYOUT::POINT pt;

        pt.x = readFloat();
        pt.y = readFloat();

        if( pt.x == 0.0f || std::isnormal( pt.x ) )
        {
            aObj.points.emplace_back( pt );
        }
        else
        {
            // ku14194revb.lay6 has polygons where pointCount is 5 but only 4 points are valid.
            // seek back so that the next object type is read correctly, and ignore invalid points.
            seek( -8 );
        }
    }
}


void SPRINT_LAYOUT_PARSER::parseTrailer()
{
    readUint32(); // active_board_tab
    m_fileData.project_name = readFixedString( 100 );
    m_fileData.project_author = readFixedString( 100 );
    m_fileData.project_company = readFixedString( 100 );
    m_fileData.project_comment = readVarString();
}


// ============================================================================
// Board construction
// ============================================================================

PCB_LAYER_ID SPRINT_LAYOUT_PARSER::mapLayer( uint8_t aSprintLayer ) const
{
    switch( aSprintLayer )
    {
    case SPRINT_LAYOUT::LAYER_C1: return F_Cu;
    case SPRINT_LAYOUT::LAYER_S1: return F_SilkS;
    case SPRINT_LAYOUT::LAYER_C2: return B_Cu;
    case SPRINT_LAYOUT::LAYER_S2: return B_SilkS;
    case SPRINT_LAYOUT::LAYER_I1: return In1_Cu;
    case SPRINT_LAYOUT::LAYER_I2: return In2_Cu;
    case SPRINT_LAYOUT::LAYER_O:  return Edge_Cuts;
    default:                      return F_Cu;
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

    if( nm > std::numeric_limits<int>::max() || nm < std::numeric_limits<int>::min() )
        THROW_IO_ERROR( _( "Coordinate value out of range in Sprint Layout file" ) );

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

    if( aStr.empty() )
        return wxEmptyString;

    wxString ret = wxString::FromUTF8( aStr );

    if( ret.empty() && convCP1251.IsOk() )
        ret = wxString( aStr.c_str(), convCP1251 );

    return ret;
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

    for( const auto& obj : boardData.objects )
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

    // Maps component_id to FOOTPRINT for grouping component-owned objects
    std::map<uint16_t, FOOTPRINT*> componentMap;
    std::vector<std::vector<VECTOR2I>> outlineSegments;

    auto getOrCreateComponentFootprint = [&]( const SPRINT_LAYOUT::OBJECT& aObj ) -> FOOTPRINT*
    {
        if( aObj.component_id == 0 )
            return nullptr;

        auto it = componentMap.find( aObj.component_id );

        if( it != componentMap.end() )
            return it->second;

        FOOTPRINT* fp = new FOOTPRINT( board.get() );

        if( aObj.type == SPRINT_LAYOUT::OBJ_TEXT && !aObj.text.empty() )
        {
            fp->SetReference( convertString( aObj.text ) );
        }
        else
        {
            fp->SetReference( wxString::Format( wxS( "U%d" ), aObj.component_id ) );

            for( PCB_FIELD* fd : fp->GetFields() )
                fd->SetVisible( false );
        }

        if( aObj.type == SPRINT_LAYOUT::OBJ_TEXT && aObj.component.valid )
        {
            if( !aObj.component.comment.empty() )
                fp->GetField( FIELD_T::DESCRIPTION )->SetText( convertString( aObj.component.comment ) );

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
    for( const auto& obj : boardData.objects )
    {
        if( obj.type == SPRINT_LAYOUT::OBJ_TEXT && obj.component_id > 0 && obj.component.valid )
            getOrCreateComponentFootprint( obj );
    }

    // Second pass: process all objects in board/footprint context
    for( const auto& obj : boardData.objects )
    {
        BOARD_ITEM_CONTAINER* container = board.get();

        if( FOOTPRINT* fp = getOrCreateComponentFootprint( obj ) )
            container = fp;

        switch( obj.type )
        {
        case SPRINT_LAYOUT::OBJ_THT_PAD:
        case SPRINT_LAYOUT::OBJ_SMD_PAD:
            processPad( container, obj );
            break;

        case SPRINT_LAYOUT::OBJ_LINE:
            processLine( container, obj, outlineSegments );
            break;

        case SPRINT_LAYOUT::OBJ_POLY:
            processPoly( container, obj, outlineSegments );
            break;

        case SPRINT_LAYOUT::OBJ_CIRCLE:
            processCircle( container, obj, outlineSegments );
            break;

        case SPRINT_LAYOUT::OBJ_TEXT:
            processText( container, obj );
            break;

        default:
            break;
        }
    }

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

    for( const auto& [ componentId, fp ] : componentMap )
    {
        wxString fpKey = wxString::Format( wxS( "SprintLayout_%s" ), fp->GetReference() );
        FOOTPRINT* fpCopy = static_cast<FOOTPRINT*>( fp->Clone() );
        fpCopy->SetParent( nullptr );
        aFootprintMap[fpKey] = std::unique_ptr<FOOTPRINT>( fpCopy );
    }

    buildOutline( board.get(), outlineSegments, boardData );

    // Create ground plane zones for layers where ground plane is enabled.
    // Sprint Layout stores a per-layer flag in the board header.
    // Indices: 0=C1(F.Cu), 2=C2(B.Cu), 4=I1(In1.Cu), 5=I2(In2.Cu)
    static const struct
    {
        int          index;
        PCB_LAYER_ID layer;
    } groundPlaneMap[] = {
        { 0, F_Cu },
        { 2, B_Cu },
        { 4, In1_Cu },
        { 5, In2_Cu },
    };

    for( const auto& gp : groundPlaneMap )
    {
        if( boardData.ground_plane[gp.index] == 0 )
            continue;

        int w = sprintToKicadCoord( static_cast<float>( boardData.size_x ) );
        int h = sprintToKicadCoord( static_cast<float>( boardData.size_y ) );

        if( w <= 0 || h <= 0 )
            continue;

        ZONE* zone = new ZONE( board.get() );
        zone->SetLayer( gp.layer );
        zone->SetIsRuleArea( false );
        zone->SetZoneName( wxString::Format( wxS( "GND_PLANE_%s" ), board->GetLayerName( gp.layer ) ) );
        zone->SetLocalClearance( std::optional<int>( pcbIUScale.mmToIU( 0.3 ) ) );
        zone->SetThermalReliefGap( pcbIUScale.mmToIU( 0.5 ) );
        zone->SetThermalReliefSpokeWidth( pcbIUScale.mmToIU( 0.5 ) );
        zone->SetAssignedPriority( 0 );
        zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );

        SHAPE_POLY_SET outline;
        outline.NewOutline();
        outline.Append( 0, 0 );
        outline.Append( w, 0 );
        outline.Append( w, h );
        outline.Append( 0, h );

        zone->AddPolygon( outline.COutline( 0 ) );
        board->Add( zone );
    }

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


void SPRINT_LAYOUT_PARSER::processPad( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj )
{
    BOARD* board = aContainer ? aContainer->GetBoard() : nullptr;
    FOOTPRINT* fp = dynamic_cast<FOOTPRINT*>( aContainer );

    if( !fp )
    {
        // Standalone pad without a component gets its own footprint
        fp = new FOOTPRINT( board );
        fp->SetReference( wxString::Format( wxS( "PAD%d" ),
                          static_cast<int>( board->Footprints().size() ) ) );
        fp->Reference().SetVisible( false );
        fp->SetLayer( F_Cu );
        aContainer->Add( fp );
    }

    PAD* pad = new PAD( fp );

    // SMD pad x,y may be a component-relative offset rather than an absolute
    // position (depends on the Sprint Layout version that created the file).
    // The points array always stores absolute coordinates, so derive the pad
    // center from the points when available.
    VECTOR2I pos;

    if( aObj.type == SPRINT_LAYOUT::OBJ_SMD_PAD && !aObj.points.empty() )
    {
        double cx = 0, cy = 0;

        for( const auto& pt : aObj.points )
        {
            cx += pt.x;
            cy += pt.y;
        }

        cx /= static_cast<double>( aObj.points.size() );
        cy /= static_cast<double>( aObj.points.size() );
        pos = sprintToKicadPos( static_cast<float>( cx ), static_cast<float>( cy ) );
    }
    else
    {
        pos = sprintToKicadPos( aObj.x, aObj.y );
    }

    pad->SetPosition( pos );

    if( aObj.type == SPRINT_LAYOUT::OBJ_THT_PAD )
    {
        PCB_LAYER_ID padLayer = mapLayer( aObj.layer );

        if( aObj.plated == 0 )
        {
            pad->SetAttribute( PAD_ATTRIB::NPTH );
            
            if( padLayer == B_Cu || padLayer == B_SilkS )
            {
                pad->SetLayerSet( LSET( { B_Cu, B_Mask } ) );
                fp->SetLayer( B_Cu );
            }
            else
            {
                pad->SetLayerSet( LSET( { F_Cu, F_Mask } ) );
            }
        }
        else
        {
            pad->SetAttribute( PAD_ATTRIB::PTH );
            pad->SetLayerSet( PAD::PTHMask() );
        }

        int outerDia = sprintToKicadCoord( aObj.outer * 2.0f );
        int drillDia = sprintToKicadCoord( aObj.inner * 2.0f );

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

        // The start_angle field is a union with th_style[4] for pad objects.
        // Each byte is the thermal style for one copper layer (C1, C2, I1, I2).
        // 0=direct/full, 1=thermal relief, 2=no connection
        uint8_t thStyle = aObj.start_angle & 0xFF;

        if( thStyle == 0 )
            pad->SetLocalZoneConnection( ZONE_CONNECTION::FULL );
        else if( thStyle == 2 )
            pad->SetLocalZoneConnection( ZONE_CONNECTION::NONE );
        else
            pad->SetLocalZoneConnection( ZONE_CONNECTION::THERMAL );
    }
    else
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );

        PCB_LAYER_ID padLayer = mapLayer( aObj.layer );

        if( padLayer == B_Cu || padLayer == B_SilkS )
        {
            pad->SetLayerSet( LSET( { B_Cu, B_Paste, B_Mask } ) );
            fp->SetLayer( B_Cu );
        }
        else
        {
            pad->SetLayerSet( PAD::SMDMask() );
        }

        int width = sprintToKicadCoord( aObj.outer * 2.0f );
        int height = sprintToKicadCoord( aObj.inner * 2.0f );

        if( height <= 0 )
            height = width;

        pad->SetSize( PADSTACK::ALL_LAYERS, VECTOR2I( width, height ) );

        if( aObj.tht_shape == SPRINT_LAYOUT::THT_SHAPE_CIRCLE )
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::CIRCLE );
        else
            pad->SetShape( PADSTACK::ALL_LAYERS, PAD_SHAPE::RECTANGLE );
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
    }

    // Thermal spoke width
    if( aObj.thermal_width > 0 )
    {
        int spokeWidth = sprintToKicadCoord( static_cast<float>( aObj.thermal_width ) );
        pad->SetLocalThermalSpokeWidthOverride( std::optional<int>( spokeWidth ) );
    }

    // Set net name
    if( !aObj.net_name.empty() )
    {
        wxString netName = convertString( aObj.net_name );
        NETINFO_ITEM* net = board->FindNet( netName );

        if( !net )
        {
            net = new NETINFO_ITEM( board, netName );
            board->Add( net );
        }

        pad->SetNet( net );
    }

    pad->SetNumber( wxString::Format( wxS( "%d" ),
                                       static_cast<int>( fp->Pads().size() + 1 ) ) );

    fp->Add( pad );
}


void SPRINT_LAYOUT_PARSER::processLine( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                        std::vector<std::vector<VECTOR2I>>& aOutlineSegments )
{
    if( aObj.points.size() < 2 )
        return;

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
        aContainer->Add( shape );
    }
}


void SPRINT_LAYOUT_PARSER::processPoly( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                        std::vector<std::vector<VECTOR2I>>& aOutlineSegments )
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

    bool isFilled = ( aObj.filled != 0 );
    bool isCutout = ( aObj.keepout != 0 );

    if( isCutout && LSET::AllCuMask().Contains( layer ) && aObj.points.size() >= 3 )
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

        SHAPE_POLY_SET outline;
        outline.NewOutline();

        for( const auto& pt : aObj.points )
        {
            VECTOR2I pos = sprintToKicadPos( pt.x, pt.y );
            outline.Append( pos.x, pos.y );
        }

        zone->AddPolygon( outline.COutline( 0 ) );
        aContainer->Add( zone );
    }
    else if( isFilled && LSET::AllCuMask().Contains( layer ) && aObj.points.size() >= 3 )
    {
        // Filled polygon on copper -> ZONE
        ZONE* zone = new ZONE( aContainer );
        zone->SetLayer( layer );
        zone->SetIsRuleArea( false );
        zone->SetDoNotAllowZoneFills( false );

        SHAPE_POLY_SET outline;
        outline.NewOutline();

        for( const auto& pt : aObj.points )
        {
            VECTOR2I pos = sprintToKicadPos( pt.x, pt.y );
            outline.Append( pos.x, pos.y );
        }

        zone->AddPolygon( outline.COutline( 0 ) );

        if( !aObj.net_name.empty() )
        {
            wxString netName = convertString( aObj.net_name );
            NETINFO_ITEM* net = board->FindNet( netName );

            if( !net )
            {
                net = new NETINFO_ITEM( board, netName );
                board->Add( net );
            }

            zone->SetNet( net );
        }

        aContainer->Add( zone );
    }
    else if( isFilled && aObj.points.size() >= 3 )
    {
        // Filled polygon on non-copper layer -> filled PCB_SHAPE
        PCB_SHAPE* shape = new PCB_SHAPE( aContainer );
        shape->SetShape( SHAPE_T::POLY );
        shape->SetFilled( true );
        shape->SetLayer( layer );
        shape->SetWidth( 0 );

        SHAPE_POLY_SET polySet;
        polySet.NewOutline();

        for( const auto& pt : aObj.points )
        {
            VECTOR2I pos = sprintToKicadPos( pt.x, pt.y );
            polySet.Append( pos.x, pos.y );
        }

        shape->SetPolyShape( polySet );
        aContainer->Add( shape );
    }
    else
    {
        // Unfilled polygon -> PCB_SHAPE polyline segments
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
            aContainer->Add( shape );
        }
    }
}


void SPRINT_LAYOUT_PARSER::processCircle( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                                          std::vector<std::vector<VECTOR2I>>& aOutlineSegments )
{
    PCB_LAYER_ID layer = mapLayer( aObj.layer );
    VECTOR2I center = sprintToKicadPos( aObj.x, aObj.y );
    float radius = ( aObj.outer + aObj.inner ) / 2.0f;
    int kiRadius = sprintToKicadCoord( radius );
    int width = sprintToKicadCoord( aObj.outer - aObj.inner );

    if( width <= 0 )
        width = pcbIUScale.mmToIU( 0.25 );

    int32_t startAngle = aObj.start_angle;

    // line_width is uint32_t but stores end_angle (signed) for circle objects.
    // Two's complement reinterpretation is correct here.
    int32_t endAngle = static_cast<int32_t>( aObj.line_width );

    bool isFullCircle = ( startAngle == 0 && endAngle == 0 )
                        || ( endAngle - startAngle >= 360000 )
                        || ( startAngle == endAngle );

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
            int32_t sa = startAngle;
            int32_t ea = endAngle;

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
        double startRad = ( static_cast<double>( startAngle ) / 1000.0 ) * M_PI / 180.0;
        int sx = center.x + static_cast<int>( std::cos( startRad ) * kiRadius );
        int sy = center.y - static_cast<int>( std::sin( startRad ) * kiRadius );
        shape->SetStart( VECTOR2I( sx, sy ) );

        int32_t ea = endAngle;

        if( ea <= startAngle )
            ea += 360000;

        // Negate arc angle for Y-flip (reverses sweep direction)
        double arcAngle = -static_cast<double>( ea - startAngle ) / 1000.0;
        shape->SetArcAngleAndEnd( EDA_ANGLE( arcAngle, DEGREES_T ), true );
    }

    aContainer->Add( shape );
}


void SPRINT_LAYOUT_PARSER::processText( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj )
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

    if( fp && aObj.tht_shape > 0 && aObj.tht_shape <= 2 )
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

    text->SetLayer( layer );
    text->SetText( convertString( aObj.text ) );
    text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
    text->SetVertJustify( GR_TEXT_V_ALIGN_BOTTOM );
    text->SetKeepUpright( false );
    text->SetVisible( aObj.filled != 0 );

    VECTOR2I pos = sprintToKicadPos( aObj.x, aObj.y );
    text->SetTextPos( pos );

    int height = sprintToKicadCoord( aObj.outer ) * 0.8;

    if( height <= 0 )
        height = pcbIUScale.mmToIU( 1.0 );

    double widthScale = 0.5 + 0.5 * aObj.line_width;
    text->SetTextSize( VECTOR2I( height * widthScale, height ) );

    double thicknessScale = 0.06 + 0.05 * aObj.inner;
    int    thickness = height * thicknessScale;

    if( thickness <= 0 )
        thickness = std::max( 1, height / 8 );

    text->SetTextThickness( thickness );
    text->SetTextAngle( EDA_ANGLE( -aObj.rotation, DEGREES_T ) );

    if( aObj.mirror != 0 || aObj.thermal_width != 0 )
        text->SetMirrored( true );

    if( add )
        aContainer->Add( text );
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

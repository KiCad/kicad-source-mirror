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

#ifndef SPRINT_LAYOUT_PARSER_H_
#define SPRINT_LAYOUT_PARSER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <set>

#include <layer_ids.h>
#include <math/vector2d.h>

class BOARD;
class BOARD_ITEM_CONTAINER;
class FOOTPRINT;
class NETINFO_ITEM;
class PAD;
class PCB_SHAPE;
class PCB_TEXT;
class BOARD_ITEM;

namespace SPRINT_LAYOUT
{

enum OBJECT_TYPE
{
    OBJ_SEGMENT      = 1,
    OBJ_THT_PAD      = 2,
    OBJ_OUTLINE_TEXT = 3,
    OBJ_POLY         = 4,
    OBJ_CIRCLE       = 5,
    OBJ_LINE         = 6,
    OBJ_STROKE_TEXT  = 7,
    OBJ_SMD_PAD      = 8,
};

enum LAYER_ID
{
    LAYER_C1 = 1,   // top copper
    LAYER_S1 = 2,   // top silkscreen
    LAYER_C2 = 3,   // bottom copper
    LAYER_S2 = 4,   // bottom silkscreen
    LAYER_I1 = 5,   // inner layer 1
    LAYER_I2 = 6,   // inner layer 2
    LAYER_O  = 7,   // board outline
};

enum THT_SHAPE
{
    THT_SHAPE_CIRCLE = 1,
    THT_SHAPE_OCT    = 2,
    THT_SHAPE_SQUARE = 3,

    THT_SHAPE_H_ROUND = 4,
    THT_SHAPE_H_CHAMFER = 5,
    THT_SHAPE_H_RECT = 6,

    THT_SHAPE_V_ROUND = 7,
    THT_SHAPE_V_CHAMFER = 8,
    THT_SHAPE_V_RECT = 9,
};

struct POINT
{
    float x;
    float y;
};

struct COMPONENT_DATA
{
    bool        valid = false;
    float       off_x = 0;
    float       off_y = 0;
    uint8_t     center_mode = 0;
    double      rotation = 0;
    std::string package;
    std::string comment;
    uint8_t     use = 0;
};

struct OBJECT
{
    uint8_t                type = 0;           // OBJECT_TYPE enum value
    float                  x = 0;
    float                  y = 0;
    float                  outer = 0;          // THT/circle outer radius, SMD half-width, text height
    float                  inner = 0;          // THT drill radius, SMD half-height, text stroke width
    int32_t                line_width = 0;     // line/poly stroke width, circle end_angle
    uint8_t                layer = 0;          // LAYER_ID enum value
    uint8_t                tht_shape = 0;      // THT_SHAPE for pads, 1=component-ref for text
    uint16_t               component_id = 0;
    int32_t                start_angle = 0;    // circle start angle, pad thermal style bytes
    uint8_t                filled = 0;         // nonzero = filled polygon
    int32_t                clearance = 0;      // ground plane clearance
    uint8_t                mirror_v = 0;
    uint8_t                mirror_h = 0;       // or thermal relief spoke enable
    uint8_t                keepout = 0;        // nonzero = ground plane exclusion area
    int32_t                rotation = 0;       // rotation in degree units
    uint8_t                plated = 0;         // 0 = NPTH, nonzero = plated
    uint8_t                soldermask = 0;     // 0 = no mask opening (tented)

    std::string            text;
    std::string            identifier;
    std::vector<uint32_t>  groups;
    std::vector<POINT>     points;
    std::vector<OBJECT>    text_children;
    COMPONENT_DATA         component;
};

struct BOARD_DATA
{
    std::string            name;
    uint32_t               size_x = 0;     // 1/10000 mm
    uint32_t               size_y = 0;     // 1/10000 mm
    int32_t                center_x = 0;   // 1/10000 mm
    int32_t                center_y = 0;   // 1/10000 mm
    uint8_t                ground_plane[7] = {};
    uint8_t                is_multilayer = 0;
    std::vector<OBJECT>    objects;
};

struct FILE_DATA
{
    uint8_t                version = 0;
    std::vector<BOARD_DATA> boards;
    std::string            project_name;
    std::string            project_author;
    std::string            project_company;
    std::string            project_comment;
};

} // namespace SPRINT_LAYOUT


class NETINFO_ITEM;

class SPRINT_LAYOUT_PARSER
{
public:
    SPRINT_LAYOUT_PARSER();
    ~SPRINT_LAYOUT_PARSER();

    // Parse full Sprint Layout board files (.lay6 / .lay) into internal file data
    bool ParseBoard( const wxString& aFileName );

    // Parse a macro (LMK) file with objects in BOARD_DATA with index 0
    bool ParseMacroFile( const wxString& aFileName );

    // Create a BOARD from BOARD_DATA at the given index, and populate the provided footprint map with any footprints found in the file.
    BOARD* CreateBoard( std::map<wxString, std::unique_ptr<FOOTPRINT>>& aFootprintMap, size_t aBoardIndex = 0 );

    // Create a single FOOTPRINT from the board at index 0
    FOOTPRINT* CreateFootprint();

    const SPRINT_LAYOUT::FILE_DATA& GetFileData() const { return m_fileData; }

private:
    // Binary reading helpers
    uint8_t     readUint8();
    uint16_t    readUint16();
    int16_t     readInt16();
    uint32_t    readUint32();
    uint32_t    readUnsigned();
    int32_t     readInt32();
    int32_t     readSigned();
    float       readFloat();
    double      readDouble();
    float       readCoord();
    std::string readFixedString( size_t aMaxLen );
    std::string readVarString();
    void        skip( size_t aBytes );
    void        seek( int aBytes );

    void parseFileStart( const wxString& aFileName );
    void parseBoardHeader( SPRINT_LAYOUT::BOARD_DATA& aBoard );
    void parseObjectsList( SPRINT_LAYOUT::BOARD_DATA& aBoard );
    void parseGroups( SPRINT_LAYOUT::OBJECT& aObj );
    void parsePoints( SPRINT_LAYOUT::OBJECT& aObj );
    void parseObject( SPRINT_LAYOUT::OBJECT& aObject, bool aIsTextChild = false );
    void parseTrailer();

    // Board construction helpers
    PCB_LAYER_ID mapLayer( uint8_t aSprintLayer ) const;
    int          sprintToKicadCoord( float aValue ) const;
    VECTOR2I     sprintToKicadPos( float aX, float aY ) const;
    wxString     convertString( const std::string& aStr ) const;

    bool          layerHasGroundPlane( PCB_LAYER_ID aLayer, const uint8_t aGroundPlane[7] ) const;

    NETINFO_ITEM* resolveItemNet( BOARD* aBoard, const SPRINT_LAYOUT::OBJECT& aObj, PCB_LAYER_ID aLayer,
                                  const uint8_t aGroundPlane[7], NETINFO_ITEM* aGndPlaneNet ) const;

    void processPad( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj, const uint8_t aGroundPlane[7],
                     NETINFO_ITEM* aGndPlaneNet, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processCircle( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                        std::vector<std::vector<VECTOR2I>>& aOutlineSegments, const uint8_t aGroundPlane[7],
                        NETINFO_ITEM* aGndPlaneNet, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processLine( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                      std::vector<std::vector<VECTOR2I>>& aOutlineSegments, const uint8_t aGroundPlane[7],
                      NETINFO_ITEM* aGndPlaneNet, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processSegment( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                         std::vector<std::vector<VECTOR2I>>& aOutlineSegments, const uint8_t aGroundPlane[7],
                         NETINFO_ITEM* aGndPlaneNet, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processPoly( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                      std::vector<std::vector<VECTOR2I>>& aOutlineSegments, const uint8_t aGroundPlane[7],
                      NETINFO_ITEM* aGndPlaneNet, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processText( BOARD_ITEM_CONTAINER* aContainer, const SPRINT_LAYOUT::OBJECT& aObj,
                      std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void processItemGroups( BOARD_ITEM* aItem, const SPRINT_LAYOUT::OBJECT& aObj,
                            std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    void buildOutline( BOARD* aBoard, std::vector<std::vector<VECTOR2I>>& aOutlineSegments,
                       const SPRINT_LAYOUT::BOARD_DATA& aBoardData );

    void resolveGroups( BOARD_ITEM_CONTAINER* aContainer, std::map<uint32_t, std::set<BOARD_ITEM*>>& aGidToItems );

    SPRINT_LAYOUT::FILE_DATA    m_fileData;
    const uint8_t*              m_pos = nullptr;
    const uint8_t*              m_start = nullptr;
    const uint8_t*              m_end = nullptr;
    std::vector<uint8_t>        m_buffer;
    bool                        m_parsingMacro = false;
};

#endif // SPRINT_LAYOUT_PARSER_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>

#include <wx/string.h>

#include "pads_parser.h"

namespace PADS_IO
{

/**
 * Parser for PADS binary PCB file format (.pcb).
 *
 * Reads the binary PADS file and populates the same intermediate structs as the
 * ASCII PARSER class, allowing the existing struct-to-KiCad conversion code to
 * be shared between both importers.
 *
 * Binary file structure:
 *   [Header]           10 bytes: magic(2) + version(2) + reserved(6)
 *   [Section Directory] N*16 bytes per entry
 *   [Section Data]     Concatenated section payloads
 *   [Metadata Region]  Part types, components, nets, config, attributes
 *   [Footer]           46 bytes: padding(4) + GUID(38) + size_check(4)
 *
 * Supported versions: 0x2021, 0x2025, 0x2026, 0x2027
 */
class BINARY_PARSER
{
public:
    BINARY_PARSER();
    ~BINARY_PARSER();

    void Parse( const wxString& aFileName );

    /**
     * Check if a file appears to be a PADS binary PCB file.
     * Checks the 2-byte magic (0x00FF) and version field.
     */
    static bool IsBinaryPadsFile( const wxString& aFileName );

    const PARAMETERS& GetParameters() const { return m_parameters; }
    const std::vector<PART>& GetParts() const { return m_parts; }
    const std::vector<NET>& GetNets() const { return m_nets; }
    const std::vector<ROUTE>& GetRoutes() const { return m_routes; }
    const std::vector<TEXT>& GetTexts() const { return m_texts; }
    const std::vector<POUR>& GetPours() const { return m_pours; }
    const std::vector<POLYLINE>& GetBoardOutlines() const { return m_boardOutlines; }
    const std::map<std::string, PART_DECAL>& GetPartDecals() const { return m_decals; }
    int GetLayerCount() const { return m_parameters.layer_count; }
    bool IsBasicUnits() const { return true; }

    std::vector<LAYER_INFO> GetLayerInfos() const;

private:
    static constexpr uint16_t MAGIC           = 0xFF00;
    static constexpr int      HEADER_SIZE     = 10;
    static constexpr int      FOOTER_SIZE     = 46;
    static constexpr int      DIR_ENTRY_SIZE  = 16;
    static constexpr int32_t  ANGLE_SCALE     = 1800000;

    struct DirEntry
    {
        int      index      = 0;
        uint32_t count      = 0;
        uint32_t totalBytes = 0;
        uint32_t dataOffset = 0;
        uint32_t perItem    = 0;
    };

    // Version helpers
    bool isOldFormat() const { return m_version == 0x2021; }
    int  dirEntryCount() const;

    // Low-level readers
    uint8_t  readU8( size_t aOffset ) const;
    uint16_t readU16( size_t aOffset ) const;
    uint32_t readU32( size_t aOffset ) const;
    int32_t  readI32( size_t aOffset ) const;
    std::string readFixedString( size_t aOffset, size_t aMaxLen ) const;

    // File structure parsing
    void parseHeader();
    void parseFooter();
    void parseDirectory();

    // Section data accessors
    const uint8_t* sectionData( int aIndex ) const;
    uint32_t       sectionSize( int aIndex ) const;
    const DirEntry* getSection( int aIndex ) const;

    // Section parsers
    void parseBoardSetup();
    void parseStringPool();
    void parsePartPlacements();
    void parseSection19Parts();
    void parsePadStacks();
    void parsePartDecals();
    void parseFootprintDefs();
    void parseLineVertices();
    void parseBoardOutline();
    void parseNetNames();
    void parseMetadataRegion();
    void parseDftConfig( size_t aStart, size_t aEnd );
    void parseRouteVertices();
    void parseTextRecords();
    void parseCopperPours();

    // DFT format parsers
    std::map<std::string, std::string> parseDftDotPadded( size_t aPos, size_t aEnd ) const;
    std::map<std::string, std::string> parseDftNullSeparated( size_t aPos, size_t aEnd ) const;

    // Net name helpers
    std::string extractNetName( const uint8_t* aData, size_t aOffset ) const;
    bool isValidNetName( const std::string& aName ) const;

    // String pool resolver
    std::string resolveString( uint32_t aByteOffset ) const;

    // Coordinate conversion from binary absolute to PADS_IO design-relative units
    double toBasicCoordX( int32_t aRawValue ) const;
    double toBasicCoordY( int32_t aRawValue ) const;
    double toBasicAngle( int32_t aRawAngle ) const;

    // File data
    std::vector<uint8_t> m_data;
    uint16_t             m_version = 0;
    int                  m_numDirEntries = 0;

    // Directory
    std::vector<DirEntry> m_dirEntries;

    // String pool (section 57)
    std::vector<uint8_t> m_stringPoolBytes;

    // Line vertex pool (section 12)
    struct LineVertex
    {
        int32_t  x     = 0;
        int32_t  y     = 0;
        uint32_t extra = 0;
    };
    std::vector<LineVertex> m_lineVertices;

    // Coordinate origin from DFT_CONFIGURATION
    int32_t m_originX = 0;
    int32_t m_originY = 0;
    bool    m_originFound = false;

    // Pad stack cache indexed by section 4 record number
    std::map<int, std::vector<PAD_STACK_LAYER>> m_padStackCache;

    // Footprint type name -> decal name mapping from section 17
    std::map<std::string, std::string> m_fpTypeToDecal;

    // Route segment from section 24 linking to section 60 vertices
    struct RouteSegment
    {
        int32_t x1    = 0;
        int32_t y1    = 0;
        int32_t x2    = 0;
        int32_t y2    = 0;
        int32_t width = 0;
    };
    std::vector<RouteSegment> m_routeSegments;

    // Via locations from section 59 (pin connection endpoints)
    struct ViaLocation
    {
        int32_t x = 0;
        int32_t y = 0;
    };
    std::vector<ViaLocation> m_viaLocations;

    // Output data (same structs as ASCII parser)
    PARAMETERS                          m_parameters;
    std::vector<PART>                   m_parts;
    std::vector<NET>                    m_nets;
    std::vector<ROUTE>                  m_routes;
    std::vector<TEXT>                   m_texts;
    std::vector<POUR>                   m_pours;
    std::vector<POLYLINE>               m_boardOutlines;
    std::map<std::string, PART_DECAL>   m_decals;
};

} // namespace PADS_IO

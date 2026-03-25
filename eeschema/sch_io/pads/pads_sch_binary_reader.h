/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_SCH_BINARY_READER_H_
#define PADS_SCH_BINARY_READER_H_

#include <cstdint>
#include <string>
#include <vector>

#include <wx/string.h>

class SCHEMATIC;
class SCH_SHEET;

namespace PADS_SCH_BINARY
{

/// One placed schematic symbol recovered from the binary .sch part array.
struct PLACEMENT
{
    std::string reference;   ///< Reference designator (e.g. "U1", "J6-1").
    int         x_mils = 0;  ///< Page X in mils.
    int         y_mils = 0;  ///< Page Y in mils (PADS is Y-up).
    int         rotation = 0;///< 0 or 90 degrees.
};

/// One wire vertex recovered from the binary 8-byte vertex pool.
struct WIRE_VERTEX
{
    int x_mils = 0;
    int y_mils = 0;
};

/// One junction (PADS tie-dot) recovered from the binary 12-byte tie-dot pool.
struct JUNCTION
{
    int x_mils = 0;
    int y_mils = 0;
};

/// One free-text item recovered from the binary 32-byte text record pool.
struct TEXT_ITEM
{
    int         x_mils = 0;
    int         y_mils = 0;
    int         orientation_deg = 0; ///< 0 or 90.
    int         justification = 0;   ///< PADS JUST column verbatim.
    int         height_mils = 0;
    int         linewidth_mils = 0;
    std::string text;                ///< Recovered string content (may be empty).
};

/**
 * Reader for the proprietary PADS Logic binary .sch format (magic 00 FE,
 * version 0x000D).
 *
 * eeschema's primary PADS path is the PADS-LOGIC ASCII export; this reader
 * adds a path for the binary .sch.  The decode is driven entirely by the
 * serialized record structure:
 *
 *   - SYMBOLS  the stride-136 part-instance records, one run per sheet, framed
 *              by the MFC class tag and the text-style trailer; recovered as a
 *              generic placeholder symbol at the stored page position and
 *              orientation (the placement->parttype->graphic link is a runtime
 *              heap pointer and is not in the file, so the real symbol graphic
 *              cannot be recovered)
 *   - WIRES    the 8-byte vertex pools tiled by the stride-40 split-header
 *              cumulative-index chain, emitted as SCH_LINE wires; the explicit
 *              gap slices between cumulative jumps are bus polylines
 *   - TEXT     the 32-byte free-text records (position, orientation,
 *              justification, height, linewidth), with string content recovered
 *              by an ordered length-matched walk of the shared string pool
 *   - JUNCTIONS the 12-byte tie-dot records (one run per sheet, marker 0xfc),
 *              emitted as SCH_JUNCTION
 *
 * The placement->parttype->graphic link (the real symbol body) is recovered
 * through the part-type and used-decal pools rather than the 136-byte record;
 * that binding is decoded separately.
 */
class PADS_SCH_BINARY_READER
{
public:
    PADS_SCH_BINARY_READER() = default;

    /// Return true if @p aData is a PADS Logic binary schematic.
    static bool IsBinarySch( const std::vector<uint8_t>& aData );

    /// Read the file at @p aFileName into @p aData. Returns false on I/O error.
    static bool ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData );

    /// Parse @p aData. Returns false if the container header is invalid.
    bool Parse( const std::vector<uint8_t>& aData );

    const std::vector<PLACEMENT>&                   GetPlacements() const { return m_placements; }
    const std::vector<WIRE_VERTEX>&                 GetWireVertices() const { return m_wireVertices; }
    const std::vector<std::vector<WIRE_VERTEX>>&    GetWirePolylines() const { return m_wirePolylines; }
    const std::vector<std::vector<WIRE_VERTEX>>&    GetBusPolylines() const { return m_busPolylines; }
    const std::vector<TEXT_ITEM>&                   GetTexts() const { return m_texts; }
    const std::vector<JUNCTION>&                    GetJunctions() const { return m_junctions; }

    /**
     * Build the recovered symbols and wires onto @p aRootSheet's screen.
     *
     * @return the number of objects (symbols + wires) appended.
     */
    int BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet ) const;

private:
    void decodePlacements( const std::vector<uint8_t>& aData );
    void decodeWires( const std::vector<uint8_t>& aData );
    void decodeTexts( const std::vector<uint8_t>& aData );
    void decodeJunctions( const std::vector<uint8_t>& aData );

    std::vector<PLACEMENT>               m_placements;
    std::vector<WIRE_VERTEX>             m_wireVertices;   ///< Flat pool, file order.
    std::vector<std::vector<WIRE_VERTEX>> m_wirePolylines; ///< Per-connection polylines.
    std::vector<std::vector<WIRE_VERTEX>> m_busPolylines;  ///< Bus polylines (split-run gaps).
    std::vector<TEXT_ITEM>               m_texts;          ///< Free-text items, file order.
    std::vector<JUNCTION>                m_junctions;      ///< Tie-dot junctions, all sheets.
};

} // namespace PADS_SCH_BINARY

#endif // PADS_SCH_BINARY_READER_H_

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

/**
 * @file orcad_page.h
 *
 * Parsers for the per-page streams 'Views/<folder>/Pages/<page>', the per-folder
 * page-order stream 'Views/<folder>/Schematic', and — for hierarchy DETECTION
 * only — the 'Views/<folder>/Hierarchy/Hierarchy' stream.  Implemented in
 * orcad_page.cpp.
 */

#ifndef ORCAD_PAGE_H_
#define ORCAD_PAGE_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sch_io/orcad/orcad_records.h>

/**
 * Parse one 'Views/<folder>/Pages/<page>' stream into raw structure lists.
 *
 * Stream layout: one framed structure of type 10 (Page) whose body starts with
 * lzt page name, lzt page-size name, then the 156-byte PageSettings block
 * (OrcadParsePageSettings — supplies width/height/isMetric), followed by these
 * u16-counted lists in order:
 *
 *   1. title blocks           framed structures (type 65)
 *   2. T0x34 records          raw, NOT prefix-framed (OrcadReadT0x34Raw)
 *   3. T0x35 records          raw, NOT prefix-framed (OrcadReadT0x35Raw)
 *   4. net table              per entry: lzt net name + u32 net db id
 *   5. wires                  framed (types 20/21)
 *   6. placed parts           framed; type 13 -> instances, type 12 -> blocks
 *   7. ports                  framed (type 23)
 *   8. globals                framed (type 37), each entry followed by 5 bytes
 *   9. off-page connectors    framed (type 38), each entry followed by 5 bytes
 *  10. ERC objects            framed (type 77)
 *  11. bus entries            framed (type 29)
 *  12. graphic instances      framed (types 55..62, 88, 89)
 *
 * The net table is authoritative for net names and junction computation: wires
 * carry ids that key it.  Structures that fail to parse are skipped with a
 * warning via aWarn (never aborting the page); only a structurally unreadable
 * Page structure itself throws IO_ERROR.
 */
ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData,
                               const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn );

/**
 * Parse the 'Views/<folder>/Schematic' stream and return the folder's page names
 * in display order.
 *
 * Layout: one prefix chain, lzt folder name, 4 bytes, u16 page count, then the
 * lzt page names stored LAST-FIRST (the returned vector is already reversed into
 * display order).
 *
 * The caller (plugin shell) must intersect the result with the page streams that
 * actually exist under 'Views/<folder>/Pages/', append any pages missing from the
 * order stream, and fall back to name-sorted order with a warning when this
 * parser throws IO_ERROR.
 */
std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData );

/**
 * Parse a 'Views/<folder>/Hierarchy/Hierarchy' stream into block-instance links:
 * block db id -> child folder name.  Used only to name skipped child folders in
 * hierarchy warnings (hierarchical designs are converted flat).
 *
 * The stream header is version-fragile, so the reader scans for preambles and
 * backtracks prefix chains (OrcadFindStructureStart), keeping only type-66
 * structures.  Type-66 body: u32, u32 instance db id, u8 inner-frame marker that
 * must equal 0x42, u32, u32, lzt child folder name.  Bad candidates are skipped
 * silently; the scan never throws.
 */
std::map<uint32_t, std::string> OrcadReadHierarchyLinks( const std::vector<char>& aData,
                                                         const std::vector<std::string>& aStrings,
                                                         const ORCAD_WARN_FN& aWarn );

/// True when the page contains hierarchical block instances (DrawnInstance, type 12).
inline bool OrcadPageHasHierarchyBlocks( const ORCAD_RAW_PAGE& aPage )
{
    return !aPage.blocks.empty();
}

#endif // ORCAD_PAGE_H_

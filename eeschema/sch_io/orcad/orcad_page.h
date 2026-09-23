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


#ifndef ORCAD_PAGE_H_
#define ORCAD_PAGE_H_

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sch_io/orcad/orcad_records.h>
#include <sch_io/orcad/orcad_structures.h>

/** A modern body failure skips that structure. Invalid page framing, and any legacy body failure,
 * throws IO_ERROR. */
ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn, ORCAD_DIALECT aDialect = {} );

/** Returns display order. The caller must reconcile these names with available page streams. */
std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData, ORCAD_DIALECT aDialect = {} );

/** Unlisted Views storages can be stale. Import them only if a hierarchy occurrence refers to them. */
std::vector<std::string> OrcadParseSchematicFolderOrder( const std::vector<char>& aData );

/** Returns occurrence references and child scopes. Modern parse errors return an empty scope; legacy
 * ones throw IO_ERROR. */
ORCAD_OCC_SCOPE OrcadReadOccurrenceTree( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                         const ORCAD_WARN_FN& aWarn, ORCAD_DIALECT aDialect = {} );

/** True when the page contains hierarchical block instances (DrawnInstance, type 12). */
inline bool OrcadPageHasHierarchyBlocks( const ORCAD_RAW_PAGE& aPage )
{
    return !aPage.blocks.empty();
}

#endif // ORCAD_PAGE_H_

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

/** A body failure skips that structure. Invalid page framing throws IO_ERROR. */
ORCAD_RAW_PAGE OrcadParsePage( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                               const ORCAD_WARN_FN& aWarn );

/** Returns display order. The caller must reconcile these names with available page streams. */
std::vector<std::string> OrcadParsePageOrder( const std::vector<char>& aData );

/** Returns display order; throws IO_ERROR for invalid legacy framing. */
std::vector<std::string> OrcadParsePageOrderV2( const std::vector<char>&        aData,
                                                const std::vector<std::string>& aStrings );

/** Unlisted Views storages can be stale. Import them only if a hierarchy occurrence refers to them. */
std::vector<std::string> OrcadParseSchematicFolderOrder( const std::vector<char>& aData );

/** Returns occurrence references and child scopes. Parse errors return an empty scope. */
ORCAD_OCC_SCOPE OrcadReadOccurrenceTree( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                         const ORCAD_WARN_FN& aWarn );

/** Parse a short-prefix-only v2.0 Hierarchy stream without scan recovery. */
ORCAD_OCC_SCOPE OrcadReadOccurrenceTreeV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings );

/** Legacy records have no stop offsets. A framing error throws IO_ERROR and discards the page. */
ORCAD_RAW_PAGE OrcadParsePageV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                 const ORCAD_WARN_FN& aWarn, bool aShortDisplayProp = false );

/** Keep decoded entries if a framing error ends the legacy cache. */
void OrcadParseCacheV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                        const ORCAD_WARN_FN& aWarn, std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                        std::map<std::string, ORCAD_PACKAGE>& aPackages );

/** aShortDisplayProp selects the version 1 display-property layout. */
void OrcadParseOlbSymbolStreamV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                  std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols, bool aShortDisplayProp = false );

/** aShortDisplayProp selects the version 1 display-property layout. */
void OrcadParseOlbPackageStreamV2( const std::vector<char>& aData, const std::vector<std::string>& aStrings,
                                   std::map<std::string, ORCAD_SYMBOL_DEF>& aSymbols,
                                   std::map<std::string, ORCAD_PACKAGE>& aPackages, bool aShortDisplayProp = false );

/** True when the page contains hierarchical block instances (DrawnInstance, type 12). */
inline bool OrcadPageHasHierarchyBlocks( const ORCAD_RAW_PAGE& aPage )
{
    return !aPage.blocks.empty();
}

#endif // ORCAD_PAGE_H_

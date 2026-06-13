/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef KICAD_SCH_GEOMETRY_EXTRACTOR_H
#define KICAD_SCH_GEOMETRY_EXTRACTOR_H

#include <diff_merge/diff_scene.h>
#include <kiid.h>

#include <map>


class SCHEMATIC;
class LIB_SYMBOL;


namespace KICAD_DIFF
{

/**
 * Extract a coarse outline of a SCHEMATIC into a DOCUMENT_GEOMETRY for use as
 * background context in DIFF_SCENE rendering. The extractor walks every
 * sheet's screen and pulls the most informative shapes:
 *
 *   - SCH_LINE wires / busses → segments
 *   - SCH_JUNCTION → filled circles
 *   - SCH_SHEET → outline polygons
 *   - SCH_SYMBOL → bounding-box outline (cheap; renders without painter)
 *
 * Color is taken from the supplied palette (typically theme.reference or
 * theme.comparison depending on which side the schematic represents).
 */
DOCUMENT_GEOMETRY ExtractSchematicGeometry( const SCHEMATIC& aSchematic, const KIGFX::COLOR4D& aColor,
                                            const std::map<KIID, KIGFX::COLOR4D>& aOverrides = {},
                                            bool                                  aOnlyOverrides = false );

/**
 * Extract coarse drawable context from a library symbol for visual symbol
 * diffs. The optional unit/body style filters mirror LIB_SYMBOL display
 * selection; zero means include all.
 */
DOCUMENT_GEOMETRY ExtractSymbolGeometry( const LIB_SYMBOL& aSymbol, const KIGFX::COLOR4D& aColor, int aUnit = 0,
                                         int aBodyStyle = 0 );

} // namespace KICAD_DIFF

#endif // KICAD_SCH_GEOMETRY_EXTRACTOR_H

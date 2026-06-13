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

#ifndef KICAD_PCB_GEOMETRY_EXTRACTOR_H
#define KICAD_PCB_GEOMETRY_EXTRACTOR_H

#include <diff_merge/diff_scene.h>


class BOARD;
class FOOTPRINT;


namespace KICAD_DIFF
{

/**
 * Extract a coarse outline of a BOARD into a DOCUMENT_GEOMETRY for use as
 * background context in DIFF_SCENE rendering. The extractor pulls the most
 * informative shapes without redrawing the whole board:
 *
 *   - Edge.Cuts segments / arcs / circles / polys → segments + circles
 *   - Tracks / vias → segments + circles
 *   - Pads / zones → bounding-box outlines
 *   - Footprint bounding boxes → outline polygons
 *   - User drawings on doc layers → segments
 *
 * Color is taken from the supplied palette (typically theme.reference or
 * theme.comparison depending on which side the board represents). Each
 * emitted primitive carries its PCB layer set so the diff canvas can filter
 * board context by selected layer.
 */
DOCUMENT_GEOMETRY ExtractBoardGeometry( const BOARD& aBoard, const KIGFX::COLOR4D& aColor );

/**
 * Extract drawable context geometry from a single FOOTPRINT. Used by
 * footprint-library and .kicad_mod visual diffs where there is no BOARD
 * wrapper to walk.
 */
DOCUMENT_GEOMETRY ExtractFootprintGeometry( const FOOTPRINT& aFootprint, const KIGFX::COLOR4D& aColor );

} // namespace KICAD_DIFF

#endif // KICAD_PCB_GEOMETRY_EXTRACTOR_H

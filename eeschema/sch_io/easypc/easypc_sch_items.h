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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file easypc_sch_items.h
 * @brief Schematic graphics, texts and symbols shared by the sheet builder and the symbol libraries, so a part
 *        placed on a sheet and the same part in a library are converted by one code path.
 *
 * Design units and schematic IU are both 100 nm; a SCH_FRAME only shifts the origin and turns Y down.
 */

#ifndef EASYPC_SCH_ITEMS_H
#define EASYPC_SCH_ITEMS_H

#include <memory>
#include <vector>

#include <layer_ids.h>
#include <math/vector2d.h>
#include <wx/arrstr.h>
#include <wx/string.h>

class EDA_TEXT;
class LIB_ID;
class LIB_SYMBOL;
class SCH_SHAPE;
class SCH_TEXT;

namespace EASYPC
{

struct SOURCE_FREE_TEXT;
struct SOURCE_PCB_COMPONENT;
struct SOURCE_SCM_COMPONENT;
struct SOURCE_SEGMENT;
struct SOURCE_SHAPE;
struct SOURCE_SHAPE_ITEM;
struct SOURCE_SYMBOL;
struct SOURCE_TEXT_POSITION;
struct SOURCE_TEXT_STYLE;


/// Design to KiCad mapping: X shifted, Y negated about the origin
struct SCH_FRAME
{
    int32_t OriginX = 0;
    int32_t OriginY = 0;

    /// @throw IO_ERROR when the point lies outside KiCad's coordinate range
    VECTOR2I Map( int64_t aX, int64_t aY ) const;
};


/// A shape's vertices in drawing order, following next references for formats 1 and 2
std::vector<const SOURCE_SEGMENT*> ShapeVertices( const SOURCE_SHAPE& aShape );

/// Outline of a shape: polylines, arcs and circles; a filled outline with arcs is one polygon
std::vector<std::unique_ptr<SCH_SHAPE>> ConvertShape( const SOURCE_SHAPE& aShape, const SCH_FRAME& aFrame,
                                                      SCH_LAYER_ID aLayer, int aStrokeWidth, bool aFilled,
                                                      int aUnit = 0 );

/// A shape item with its own line style and fill
std::vector<std::unique_ptr<SCH_SHAPE>> ConvertShapeItem( const SOURCE_SHAPE_ITEM& aItem, const SCH_FRAME& aFrame,
                                                          SCH_LAYER_ID aLayer, int aUnit = 0 );

/// KiCad text size for a source text style: its height as the width, its capital height as the height
VECTOR2I TextSize( const SOURCE_TEXT_STYLE& aStyle );

/// True when a design that keeps text upright turns aPos about the far end of its first line
bool DrawsUpright( const SOURCE_TEXT_POSITION& aPos, bool aKeepUpright );

/**
 * Place any KiCad text as the source position says: anchor, angle, mirror, justification and style. aMeasured is
 * the whole stored text when aText holds only one of its lines.
 */
void ApplyTextPosition( EDA_TEXT& aText, const SOURCE_TEXT_POSITION& aPos, const SCH_FRAME& aFrame,
                        bool aKeepUpright = false, const wxString& aMeasured = wxEmptyString );

std::unique_ptr<SCH_TEXT> ConvertFreeText( const SOURCE_FREE_TEXT& aText, const SCH_FRAME& aFrame, SCH_LAYER_ID aLayer,
                                           int aUnit = 0, bool aKeepUpright = false );


/// Everything one component LIB_SYMBOL is made from
struct SCH_COMPONENT_SOURCE
{
    const SOURCE_SCM_COMPONENT*       Schematic = nullptr;
    const SOURCE_PCB_COMPONENT*       Board = nullptr; ///< the package whose gate map numbers the pins
    std::vector<const SOURCE_SYMBOL*> Gates;           ///< symbol per gate; null leaves that unit empty
    bool                              PinTexts = true; ///< a placed part draws its pin texts from its pad instances
    wxString                          Footprint;
    wxArrayString                     FootprintFilters;
};

/**
 * One unit per gate in symbol-local coordinates, zero-length passive pins at the pads, pin texts where the symbol
 * stores them.  A gate pin naming several pads becomes one stacked pin.
 */
std::unique_ptr<LIB_SYMBOL> ConvertComponentSymbol( const SCH_COMPONENT_SOURCE& aSrc, const LIB_ID& aId );

/// A .ssl symbol alone: one unit, pins numbered by pad number
std::unique_ptr<LIB_SYMBOL> ConvertSymbol( const SOURCE_SYMBOL& aSymbol, const LIB_ID& aId );


/// What a package's gate map says about one gate terminal
struct GATE_TERMINAL
{
    bool     Mapped = false;
    wxString Number;  ///< KiCad pin number, stacked notation for a pad list
    wxString Display; ///< the number shown beside the pin, the part before '=' when there is one
    bool     NoConnect = false;
};

/// Terminal aTerminal (0-based) of gate aGate; formats before 9000 map a bare pad number, -1 unmapped
GATE_TERMINAL ResolveTerminal( const SOURCE_PCB_COMPONENT* aPcb, int aGate, int aTerminal );

} // namespace EASYPC

#endif // EASYPC_SCH_ITEMS_H

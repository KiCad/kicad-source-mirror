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
 * @file easypc_pcb_items.h
 * @brief Easy-PC / DesignSpark items to KiCad board items, shared by the board builder and the .psl library so a
 * part placed on a board and the same part read from a library become identical footprints.
 *
 * Native units are 100 nm with Y up and angles in thousandths of a degree anticlockwise.
 */

#ifndef EASYPC_PCB_ITEMS_H_
#define EASYPC_PCB_ITEMS_H_

#include <map>
#include <memory>
#include <vector>

#include <geometry/eda_angle.h>
#include <geometry/shape_line_chain.h>
#include <geometry/shape_poly_set.h>
#include <layer_ids.h>
#include <lset.h>
#include <math/vector2d.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <wx/string.h>

#include <io/easypc/easypc_classes_geometry.h>
#include <io/easypc/easypc_classes_geometry_pad.h>
#include <io/easypc/easypc_classes_styles.h>
#include <io/easypc/easypc_units.h>

class BOARD_ITEM_CONTAINER;
class FOOTPRINT;
class LIB_ID;
class PAD;
class PCB_SHAPE;
class PCB_TEXT;

namespace EASYPC
{
struct SOURCE_PCB_COMPONENT;
struct SOURCE_SYMBOL;
} // namespace EASYPC


namespace EASYPC_PCB
{

/// Native to KiCad coordinates: scale by 100 nm, flip Y and move the native origin to the KiCad offset
struct FRAME
{
    EASYPC::POINT32 Origin;
    VECTOR2I        Offset{};

    /// @throw IO_ERROR when the point lies outside the coordinate range KiCad computes in
    VECTOR2I ToKiCad( int32_t aX, int32_t aY ) const { return ToKiCad( VECTOR2D( aX, aY ) ); }
    VECTOR2I ToKiCad( const EASYPC::POINT32& aPoint ) const { return ToKiCad( aPoint.X, aPoint.Y ); }
    VECTOR2I ToKiCad( const VECTOR2D& aPoint ) const;

    /// @throw IO_ERROR when the length lies outside the coordinate range KiCad computes in
    int Length( int64_t aLength ) const;

    static EDA_ANGLE Angle( int32_t aMilliDegrees );
};


/// Layer mapping for one layer table, copper in stack order and other layers by their type
class LAYER_MAPPER
{
public:
    explicit LAYER_MAPPER( const std::vector<const EASYPC::SOURCE_LAYER*>& aLayers );

    std::vector<INPUT_LAYER_DESC> Describe() const;

    void ApplyUserMapping( const std::map<wxString, PCB_LAYER_ID>& aMapping );

    /// UNDEFINED_LAYER when the native layer has no KiCad layer
    PCB_LAYER_ID Map( const EASYPC::OBJECT* aLayer ) const;

    /// The copper aLayer (a layer or a set) covers, with the mask and paste layers whose type plots such pads
    LSET PadLayers( const EASYPC::OBJECT* aLayer, bool aComponentPad ) const;

    const EASYPC::SOURCE_LAYER* Native( PCB_LAYER_ID aLayer ) const;

    /// aSize grown on each side by aLayer's type oversize, fixed or a percentage of the size
    int32_t GrowByLayer( const EASYPC::SOURCE_LAYER* aLayer, int32_t aSize ) const;

    int CopperCount() const { return m_copperCount; }

private:
    std::vector<const EASYPC::SOURCE_LAYER*>            m_layers;
    std::map<const EASYPC::SOURCE_LAYER*, PCB_LAYER_ID> m_map;
    int                                                 m_copperCount = 2;
};


/**
 * A span as KiCad can hold it: a true arc, or a polyline that is the two ends of a straight span or points on an
 * arc whose centre lies beyond KiCad's range.
 */
struct ARC_FORM
{
    VECTOR2I              Start;
    VECTOR2I              Mid;
    VECTOR2I              End;
    std::vector<VECTOR2I> Points;

    bool IsArc() const { return Points.empty(); }
};

ARC_FORM ArcForm( const EASYPC::POINT32& aStart, const EASYPC::POINT32& aEnd, int32_t aArcAngle, bool aAnticlockwise,
                  const FRAME& aFrame );

/// A closed shape and its cutouts as an outline with holes
SHAPE_POLY_SET ShapeToPolySet( const EASYPC::SOURCE_DESIGN_SHAPE& aShape, const FRAME& aFrame );

/// The copper a shape item plots: its filled region and every span stroked at the line width with round ends
SHAPE_POLY_SET ShapeItemArea( const EASYPC::SOURCE_SHAPE_ITEM& aItem, const FRAME& aFrame, int aMaxError );

/// Add a shape item as one filled polygon, a circle, or one shape per span; aLayer overrides the item's layer
std::vector<PCB_SHAPE*> AddShapeItem( BOARD_ITEM_CONTAINER& aContainer, const EASYPC::SOURCE_SHAPE_ITEM& aItem,
                                      const FRAME& aFrame, const LAYER_MAPPER& aLayers,
                                      PCB_LAYER_ID aLayer = UNDEFINED_LAYER );

/// A pad with its style's per-layer exceptions; a mask or paste exception becomes a separate opening on aParent
std::unique_ptr<PAD> CreatePad( FOOTPRINT* aParent, const EASYPC::SOURCE_FREE_PAD& aPad,
                                const EASYPC::SOURCE_PAD_STYLE& aStyle, const FRAME& aFrame,
                                const LAYER_MAPPER& aLayers );

/// Place and size a text from its stored position and style; false when its layer has no KiCad layer
bool ApplyTextPosition( PCB_TEXT& aText, const EASYPC::SOURCE_TEXT_POSITION& aPosition, const FRAME& aFrame,
                        const LAYER_MAPPER& aLayers );

PCB_TEXT* AddText( BOARD_ITEM_CONTAINER& aContainer, const EASYPC::SOURCE_TEXT_POSITION& aPosition,
                   const wxString& aText, const FRAME& aFrame, const LAYER_MAPPER& aLayers );

/**
 * A footprint in symbol coordinates.  Pads take aComponent's pin numbers when given, else their own numbers.
 * aPadMap receives the PAD each source free pad became.
 */
std::unique_ptr<FOOTPRINT> ConvertFootprint( const EASYPC::SOURCE_SYMBOL& aSymbol, const LAYER_MAPPER& aLayers,
                                             const LIB_ID&                                   aId,
                                             const EASYPC::SOURCE_PCB_COMPONENT*             aComponent = nullptr,
                                             std::map<const EASYPC::SOURCE_FREE_PAD*, PAD*>* aPadMap = nullptr );

} // namespace EASYPC_PCB

#endif // EASYPC_PCB_ITEMS_H_

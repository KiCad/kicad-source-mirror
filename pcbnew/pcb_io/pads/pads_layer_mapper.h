/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_LAYER_MAPPER_H
#define PADS_LAYER_MAPPER_H

#include <string>
#include <map>
#include <vector>

#include <layer_ids.h>
#include <lset.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>


/**
 * PADS layer types. These represent the functional categories of layers
 * in PADS designs, independent of specific layer numbers.
 */
enum class PADS_LAYER_TYPE
{
    UNKNOWN,
    COPPER_TOP,
    COPPER_BOTTOM,
    COPPER_INNER,
    SILKSCREEN_TOP,
    SILKSCREEN_BOTTOM,
    SOLDERMASK_TOP,
    SOLDERMASK_BOTTOM,
    PASTE_TOP,
    PASTE_BOTTOM,
    ASSEMBLY_TOP,
    ASSEMBLY_BOTTOM,
    DOCUMENTATION,
    BOARD_OUTLINE,
    DRILL_DRAWING
};


/**
 * Information about a single PADS layer.
 */
struct PADS_LAYER_INFO
{
    int             padsLayerNum;   ///< PADS layer number
    std::string     name;           ///< Layer name as it appears in PADS file
    PADS_LAYER_TYPE type;           ///< Categorized layer type
    bool            required;       ///< Whether this layer must be mapped
};


/**
 * Maps PADS layer numbers and names to KiCad layer IDs.
 *
 * PADS uses a different layer numbering scheme than KiCad:
 * - Layer 1 is Top copper
 * - Layer N (where N is layer count) is Bottom copper
 * - Layers 2 through N-1 are inner copper layers
 * - Negative layer numbers (-2, -1) are used in pad stacks for Top/Bottom
 * - Higher positive numbers (20+) represent non-copper layers
 *
 * This class handles the translation between these systems and provides
 * auto-mapping suggestions for the layer mapping dialog.
 */
class PADS_LAYER_MAPPER
{
public:
    PADS_LAYER_MAPPER();

    /**
     * Set the total number of copper layers in the PADS design.
     *
     * @param aLayerCount Total copper layers (e.g., 2 for a 2-layer board)
     */
    void SetCopperLayerCount( int aLayerCount );

    /**
     * Get the current copper layer count.
     */
    int GetCopperLayerCount() const { return m_copperLayerCount; }

    /**
     * Parse a PADS layer number and return its type.
     *
     * @param aPadsLayer The PADS layer number
     * @return The layer type (COPPER_TOP, COPPER_INNER, etc.)
     */
    PADS_LAYER_TYPE GetLayerType( int aPadsLayer ) const;

    /**
     * Parse a PADS layer name and return its type.
     *
     * Recognizes common PADS layer names like "Silkscreen Top",
     * "Solder Mask Top", etc.
     *
     * @param aLayerName The PADS layer name
     * @return The layer type
     */
    PADS_LAYER_TYPE ParseLayerName( const std::string& aLayerName ) const;

    /**
     * Get the suggested KiCad layer for a PADS layer.
     *
     * @param aPadsLayer The PADS layer number
     * @param aType Optional override for the layer type (if known)
     * @return The suggested KiCad layer ID
     */
    PCB_LAYER_ID GetAutoMapLayer( int aPadsLayer,
                                   PADS_LAYER_TYPE aType = PADS_LAYER_TYPE::UNKNOWN ) const;

    /**
     * Get the permitted KiCad layers for a given PADS layer type.
     *
     * @param aType The PADS layer type
     * @return Set of permitted KiCad layers
     */
    LSET GetPermittedLayers( PADS_LAYER_TYPE aType ) const;

    /**
     * Build a vector of INPUT_LAYER_DESC from parsed PADS layer information.
     *
     * This is used to populate the layer mapping dialog.
     *
     * @param aLayerInfos Vector of parsed layer information
     * @return Vector of INPUT_LAYER_DESC for layer mapping dialog
     */
    std::vector<INPUT_LAYER_DESC> BuildInputLayerDescriptions(
            const std::vector<PADS_LAYER_INFO>& aLayerInfos ) const;

    /**
     * Add or update a layer name to type mapping.
     *
     * @param aName The PADS layer name
     * @param aType The layer type it represents
     */
    void AddLayerNameMapping( const std::string& aName, PADS_LAYER_TYPE aType );

    /**
     * Convert a layer type to a human-readable string.
     */
    static std::string LayerTypeToString( PADS_LAYER_TYPE aType );

    // Well-known PADS layer numbers
    static constexpr int LAYER_PAD_STACK_TOP = -2;     ///< Pad stack: Top copper
    static constexpr int LAYER_PAD_STACK_BOTTOM = -1;  ///< Pad stack: Bottom copper
    static constexpr int LAYER_PAD_STACK_INNER = 0;    ///< Pad stack: Inner copper

    // Common PADS layer number ranges
    static constexpr int LAYER_DRILL_DRAWING = 18;
    static constexpr int LAYER_DIMENSIONS = 19;
    static constexpr int LAYER_PLACEMENT_OUTLINE = 20;
    static constexpr int LAYER_ASSEMBLY_TOP = 21;
    static constexpr int LAYER_ASSEMBLY_BOTTOM = 22;
    static constexpr int LAYER_SOLDERMASK_TOP = 25;
    static constexpr int LAYER_SILKSCREEN_TOP = 26;
    static constexpr int LAYER_SILKSCREEN_BOTTOM = 27;
    static constexpr int LAYER_SOLDERMASK_BOTTOM = 28;
    static constexpr int LAYER_PASTE_TOP = 29;
    static constexpr int LAYER_PASTE_BOTTOM = 30;
    static constexpr int LAYER_BOARD_OUTLINE = 1;

private:
    int m_copperLayerCount;
    std::map<std::string, PADS_LAYER_TYPE> m_layerNameMap;

    PCB_LAYER_ID mapInnerCopperLayer( int aPadsLayer ) const;
    std::string normalizeLayerName( const std::string& aName ) const;
};

#endif // PADS_LAYER_MAPPER_H

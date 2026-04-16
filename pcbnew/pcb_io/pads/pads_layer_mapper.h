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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
 * Functional categories of PADS layers, independent of specific layer numbers.
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


struct PADS_LAYER_INFO
{
    int             padsLayerNum;
    std::string     name;
    PADS_LAYER_TYPE type;
    bool            required;
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

    void SetCopperLayerCount( int aLayerCount );

    int GetCopperLayerCount() const { return m_copperLayerCount; }

    PADS_LAYER_TYPE GetLayerType( int aPadsLayer ) const;

    PADS_LAYER_TYPE ParseLayerName( const std::string& aLayerName ) const;

    /**
     * Suggested KiCad layer for a PADS layer. @p aType overrides the type derived from
     * the layer number when known.
     */
    PCB_LAYER_ID GetAutoMapLayer( int aPadsLayer,
                                   PADS_LAYER_TYPE aType = PADS_LAYER_TYPE::UNKNOWN ) const;

    LSET GetPermittedLayers( PADS_LAYER_TYPE aType ) const;

    std::vector<INPUT_LAYER_DESC> BuildInputLayerDescriptions(
            const std::vector<PADS_LAYER_INFO>& aLayerInfos ) const;

    void AddLayerNameMapping( const std::string& aName, PADS_LAYER_TYPE aType );

    static std::string LayerTypeToString( PADS_LAYER_TYPE aType );

    static constexpr int LAYER_PAD_STACK_TOP = -2;
    static constexpr int LAYER_PAD_STACK_BOTTOM = -1;
    static constexpr int LAYER_PAD_STACK_INNER = 0;

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

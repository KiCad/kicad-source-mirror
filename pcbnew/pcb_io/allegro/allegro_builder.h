/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <functional>
#include <memory>
#include <unordered_map>

#include <convert/allegro_pcb_structs.h>
#include <convert/allegro_db.h>

#include <board.h>
#include <geometry/shape_line_chain.h>
#include <reporter.h>
#include <progress_reporter.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>


class BOARD;
class FOOTPRINT;
class NETINFO_ITEM;
class PCB_TEXT;

namespace ALLEGRO
{

class LAYER_MAPPER;

/**
 * Class that builds a KiCad board from a BRD_DB
 * (= FILE_HEADER + STRINGS + OBJECTS + bookkeeping)
 */
class BOARD_BUILDER
{
public:
    BOARD_BUILDER( const BRD_DB& aBrdDb, BOARD& aBoard, REPORTER& aReporter,
                   PROGRESS_REPORTER* aProgressReporter,
                   const LAYER_MAPPING_HANDLER& aLayerMappingHandler );
    ~BOARD_BUILDER();

    bool BuildBoard();

private:
    VECTOR2I scale( const VECTOR2I& aVector ) const;
    VECTOR2I scaleSize( const VECTOR2I& aSize ) const;
    int scale( int aVal ) const;

    template <typename T>
    const T* expectBlockByKey( uint32_t aKey, uint8_t aType ) const
    {
        if( aKey == 0 )
            return nullptr;

        const BLOCK_BASE* block = m_brdDb.GetObjectByKey( aKey );

        if( !block )
        {
            reportMissingBlock( aKey, aType );
            return nullptr;
        }

        if( block->GetBlockType() != aType )
        {
            reportUnexpectedBlockType( block->GetBlockType(), aType, aKey, block->GetOffset() );
            return nullptr;
        }

        return &static_cast<const BLOCK<T>&>( *block ).GetData();
    }

    void reportMissingBlock( uint32_t aKey, uint8_t aType ) const;
    void reportUnexpectedBlockType( uint8_t aGot, uint8_t aExpected, uint32_t aKey = 0, size_t aOffset = 0,
                                    const wxString& aName = wxEmptyString ) const;

    PCB_LAYER_ID getLayer( const LAYER_INFO& aLayerInfo ) const;

    /**
     * Get just the string value from a 0x31 STRING WRAPPER -> 0x30 STRING GRAPHIC pair
     *
     * Throws away all the other string data like pos/size/etc.
     */
    wxString get0x30StringValue( uint32_t a0x30Key ) const;

    /**
     * Build the shapes from an 0x14 shape list
     */
    std::vector<std::unique_ptr<PCB_SHAPE>> buildShapes( const BLK_0x14_GRAPHIC& aGraphicList, BOARD_ITEM_CONTAINER& aParent );
    std::unique_ptr<PCB_TEXT>  buildPcbText( const BLK_0x30_STR_WRAPPER& aStrWrapper, BOARD_ITEM_CONTAINER& aParent );

    /**
     * Build a rectangular shape from a 0x24 RECT block.
     */
    std::unique_ptr<PCB_SHAPE> buildRect( const BLK_0x24_RECT& aRect, BOARD_ITEM_CONTAINER& aParent );

    /**
     * Build a graphic polygon from a 0x0E RECT block.
     */
    std::unique_ptr<PCB_SHAPE> buildRect( const BLK_0x0E_RECT& aShape, BOARD_ITEM_CONTAINER& aParent );


    /**
     * Build a graphic polygon from a 0x28 SHAPE block.
     */
    std::unique_ptr<PCB_SHAPE> buildPolygon( const BLK_0x28_SHAPE& aShape, BOARD_ITEM_CONTAINER& aParent );

    /**
     * Construct "pad" items for a given 0x1C PADSTACK block.
     */
    std::vector<std::unique_ptr<BOARD_ITEM>> buildPadItems( const BLK_0x1C_PADSTACK& aPadstack, FOOTPRINT& aFp,
                                                            const wxString& aPadName, int aNetcode );
    std::unique_ptr<FOOTPRINT>               buildFootprint( const BLK_0x2D_FOOTPRINT_INST& aFpInstance );
    std::vector<std::unique_ptr<BOARD_ITEM>> buildTrack( const BLK_0x05_TRACK& aBlock, int aNetcode );
    std::unique_ptr<BOARD_ITEM>              buildVia( const BLK_0x33_VIA& aBlock, int aNetcode );
    std::unique_ptr<ZONE>                    buildZone( const BLK_0x28_SHAPE& aShape, int aNetcode );
    const SHAPE_LINE_CHAIN&                  buildOutline( const BLK_0x28_SHAPE& aShape ) const;

    /**
     * Walk a geometry chain (0x01 arcs and 0x15-17 segments) starting from the given key,
     * following m_Next links. Used for building hole outlines from 0x34 KEEPOUT blocks.
     */
    const SHAPE_LINE_CHAIN& buildSegmentChain( uint32_t aStartKey ) const;

    /**
     * Resolve the net code for a BOUNDARY shape by following the pointer chain:
     * BOUNDARY.Ptr7 -> 0x2C TABLE -> Ptr1 -> 0x37 array -> first entry -> 0x1B NET
     */
    int resolveShapeNet( const BLK_0x28_SHAPE& aShape ) const;

    /**
     * Follow m_MatchGroupPtr through the 0x26/0x2C pointer chain to get
     * the match group name for a NET.
     *
     * V172+: NET.m_MatchGroupPtr -> 0x26 -> m_GroupPtr -> 0x2C TABLE -> string
     * Pre-V172: NET.m_MatchGroupPtr -> 0x2C TABLE -> string
     *
     * @return the group name, or empty string if none
     */
    wxString resolveMatchGroupName( const BLK_0x1B_NET& aNet ) const;

    /**
     * Extract constraint set name from a 0x03 FIELD block pointer.
     *
     * Some boards store the constraint set name in a FIELD block referenced by m_FieldPtr in
     * the 0x1D block instead of the main string table. The FIELD string is a schematic
     * cross-reference like "@lib.xxx(view):\CONSTRAINT_SET_NAME\".
     */
    wxString resolveConstraintSetNameFromField( uint32_t aFieldKey ) const;

    void cacheFontDefs();
    void setupLayers();
    void createNets();
    void createTracks();
    void createBoardOutline();
    void createBoardText();
    void createZones();
    void applyZoneFills();
    void enablePadTeardrops();
    void applyConstraintSets();
    void applyNetConstraints();
    void applyMatchGroups();

    /**
     * Get the font definition for a given index in a 0x30, etc.
     *
     * @return the definition if it exists, else nullptr (which is probably an error in the
     * importer logic)
     */
    const BLK_0x36_DEF_TABLE::FontDef_X08* getFontDef( unsigned aIndex ) const;

    /**
     * Look up 0x07 FP instance data (0x07) for a given 0x2D FP instance
     */
    const BLK_0x07_COMPONENT_INST* getFpInstRef( const BLK_0x2D_FOOTPRINT_INST& aFpInstance ) const;

    const BRD_DB&         m_brdDb;
    BOARD&                m_board;
    REPORTER&             m_reporter;
    PROGRESS_REPORTER*    m_progressReporter;
    LAYER_MAPPING_HANDLER m_layerMappingHandler;

    // Cached list of font defs in the 0x36 node
    std::vector<const BLK_0x36_DEF_TABLE::FontDef_X08*> m_fontDefList;

    // Cached list of KiCad nets corresponding to Allegro 0x1B NET keys
    std::unordered_map<uint32_t, NETINFO_ITEM*> m_netCache;

    struct ZoneFillEntry
    {
        const BLK_0x28_SHAPE* shape;
        int                   netCode;
        PCB_LAYER_ID          layer;
    };

    std::vector<ZoneFillEntry> m_zoneFillShapes;

    std::unique_ptr<LAYER_MAPPER> m_layerMapper;

    // Cache of computed outlines keyed by shape block key, avoiding redundant geometry rebuilds
    mutable std::unordered_map<uint32_t, SHAPE_LINE_CHAIN> m_outlineCache;

    // Cache of segment chains keyed by start key, avoiding redundant hole geometry rebuilds
    mutable std::unordered_map<uint32_t, SHAPE_LINE_CHAIN> m_segChainCache;

    // Conversion factor from internal units to nanometers.
    // Internal coordinates are in mils / divisor, so scale = 25400 / divisor.
    double m_scale;
};

} // namespace ALLEGRO

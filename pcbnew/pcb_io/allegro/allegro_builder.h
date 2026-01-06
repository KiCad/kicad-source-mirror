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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <functional>
#include <memory>

#include <convert/allegro_pcb_structs.h>
#include <convert/allegro_db.h>

#include <board.h>
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
    std::vector<std::unique_ptr<PCB_SHAPE>> buildShapes( const BLK_0x14& aGraphicList, BOARD_ITEM_CONTAINER& aParent );
    std::unique_ptr<PCB_TEXT>  buildPcbText( const BLK_0x30_STR_WRAPPER& aStrWrapper, BOARD_ITEM_CONTAINER& aParent );

    /**
     * Construct "pad" items for a given 0x1C PADSTACK block.
     */
    std::vector<std::unique_ptr<BOARD_ITEM>> buildPadItems( const BLK_0x1C_PADSTACK& aPadstack, FOOTPRINT& aFp,
                                                            const wxString& aPadName, int aNetcode );
    std::unique_ptr<FOOTPRINT>               buildFootprint( const BLK_0x2D& aFpInstance );
    std::vector<std::unique_ptr<BOARD_ITEM>> buildTrack( const BLK_0x05_TRACK& aBlock, int aNetcode );
    std::unique_ptr<BOARD_ITEM>              buildVia( const BLK_0x33_VIA& aBlock, int aNetcode );
    std::unique_ptr<ZONE>                    buildZone( const BLK_0x28_SHAPE& aShape, int aNetcode );

    /**
     * Resolve the net code for a BOUNDARY shape by following the pointer chain:
     * BOUNDARY.Ptr7 -> 0x2C TABLE -> Ptr1 -> 0x37 array -> first entry -> 0x1B NET
     */
    int resolveShapeNet( const BLK_0x28_SHAPE& aShape ) const;

    void cacheFontDefs();
    void setupLayers();
    void createNets();
    void createTracks();
    void createBoardOutline();
    void createZones();
    /**
     * Get the font definition for a given index in a 0x30, etc.
     *
     * @return the definition if it exists, else nullptr (which is probably an error in the
     * importer logic)
     */
    const BLK_0x36::FontDef_X08* getFontDef( unsigned aIndex ) const;

    /**
     * Look up 0x07 FP instance data (0x07) for a given 0x2D FP instance
     */
    const BLK_0x07* getFpInstRef( const BLK_0x2D& aFpInstance ) const;

    const BRD_DB&         m_brdDb;
    BOARD&                m_board;
    REPORTER&             m_reporter;
    PROGRESS_REPORTER*    m_progressReporter;
    LAYER_MAPPING_HANDLER m_layerMappingHandler;

    // Cached list of font defs in the 0x36 node
    std::vector<const BLK_0x36::FontDef_X08*> m_fontDefList;

    // Cached list of KiCad nets corresponding to Allegro 0x1B NET keys
    std::unordered_map<uint32_t, NETINFO_ITEM*> m_netCache;

    std::unique_ptr<LAYER_MAPPER> m_layerMapper;

    // Conversion factor from internal units to nanometers.
    // Internal coordinates are in mils / divisor, so scale = 25400 / divisor.
    double m_scale;
};

} // namespace ALLEGRO

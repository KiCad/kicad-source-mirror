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

#include "allegro_builder.h"


#include "allegro_pcb_structs.h"
#include "allegro_pcb_structs.h"

#include <wx/log.h>

#include <allegro_builder.h>

#include <footprint.h>
#include <pcb_text.h>
#include <pcb_shape.h>


using namespace ALLEGRO;


/**
 * Flag to enable debug output of Allegro board construction.
 *
 * Use "KICAD_ALLEGRO_BUILDER" to enable debug output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceAllegroBuilder = wxT( "KICAD_ALLEGRO_BUILDER" );


#define BLK_FIELD( BLK_T, FIELD ) static_cast<const BLOCK<BLK_T>&>( aBlock ).GetData().FIELD


/**
 * Gets the next block in the linked list. Exactly which member does this depends on the block type.
 *
 * It's not yet clear if any blocks can be in multiple linked lists at once - for now just follow the "main"
 * one.
 * This is done as dispatch like this to avoid forcing all the blocks into an inheritance hierarchy.
 *
 * @param aBlock The block to get the next block from.
 * @return The next block in the linked list, or 0 if there is no next block.
 */
static uint32_t GetPrimaryNext( const BLOCK_BASE& aBlock )
{
    const uint8_t type = aBlock.GetBlockType();

    switch( type )
    {
    case 0x04: return BLK_FIELD( BLK_0x04_NET_ASSIGNMENT, m_Next );
    case 0x14: return BLK_FIELD( BLK_0x14, m_Next );
    case 0x15:
    case 0x16:
    case 0x17: return BLK_FIELD( BLK_0x15_16_17_SEGMENT, m_Next );
    case 0x1B: return BLK_FIELD( BLK_0x1B_NET, m_Next );
    case 0x2B: return BLK_FIELD( BLK_0x2B, m_Next );
    case 0x2D: return BLK_FIELD( BLK_0x2D, m_Next );
    case 0x30: return BLK_FIELD( BLK_0x30_STR_WRAPPER, m_Next );
    case 0x36: return BLK_FIELD( BLK_0x36, m_Next );
    default: return 0;
    }
}


class LL_WALKER
{
public:
    class iterator
    {
    public:
        iterator( uint32_t aCurrent, uint32_t aTail, const RAW_BOARD& aBoard ) :
                m_current( aCurrent ), m_tail( aTail ), m_board( aBoard )
        {
            m_currBlock = m_board.GetObjectByKey( m_current );
        }

        const BLOCK_BASE* operator*() const { return m_currBlock; }

        iterator& operator++()
        {
            if( m_current == m_tail || !m_currBlock )
            {
                m_current = 0;
            }
            else
            {
                m_current = ::GetPrimaryNext( *m_currBlock );

                // Reached the tail - this isn't actually a node we should return
                if( m_current == m_tail )
                {
                    m_current = 0;
                }
                else
                {
                    // Look up the next block. If it exists, advance.
                    // REVIEW: This may be a place we want to throw in some cases as it implies a corrupt list
                    // but I'm not 100% sure there aren't lists that end in 0x0.
                    m_currBlock = m_board.GetObjectByKey( m_current );

                    if( m_currBlock == nullptr )
                    {
                        m_current = 0;
                    }
                }
            }
            return *this;
        }

        bool operator!=( const iterator& other ) const { return m_current != other.m_current; }

    private:
        uint32_t          m_current;
        const BLOCK_BASE* m_currBlock;
        uint32_t          m_tail;
        const RAW_BOARD&  m_board;
    };

    LL_WALKER( uint32_t aHead, uint32_t aTail, const RAW_BOARD& aBoard ) :
            m_head( aHead ), m_tail( aTail ), m_board( aBoard )
    {
    }

    LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const RAW_BOARD& aBoard ) :
            LL_WALKER( aList.m_Head, aList.m_Tail, aBoard )
    {
    }

    iterator begin() const { return iterator( m_head, m_tail, m_board ); }
    iterator end() const { return iterator( 0, m_tail, m_board ); }

private:
    uint32_t         m_head;
    uint32_t         m_tail;
    const RAW_BOARD& m_board;
};


template <>
struct std::hash<LAYER_INFO>
{
    size_t operator()( const LAYER_INFO& aLayerInfo ) const noexcept
    {
        return ( aLayerInfo.m_Class << 8 ) + aLayerInfo.m_Subclass;
    }
};


/**
 * Map of the pre-set class:subclass pairs to standard layers.
 *
 * Allegro doesn't really have a neat mapping onto KiCad layers. In theory, we could use the Films to
 * map things that actually end up on the silkscreen layer (films can pick things out by class:subclass),
 * but that would be quite fiddly and would fail if the films weren't configured right.
 */
// clang-format off
static const std::unordered_map<LAYER_INFO, PCB_LAYER_ID> s_LayerKiMap = {

    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::ASSEMBLY_BOTTOM},            B_Fab},
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::ASSEMBLY_TOP},               F_Fab},

    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::ASSEMBLY_BOTTOM},            B_Fab},
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::ASSEMBLY_TOP},               F_Fab},

    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_SILKSCREEN_BOTTOM},    B_SilkS},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_SILKSCREEN_TOP},       F_SilkS},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_ASSEMBLY_BOTTOM},      B_Fab},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_ASSEMBLY_TOP},         F_Fab},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_PLACE_BOUND_BOTTOM},   B_CrtYd},
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_PLACE_BOUND_TOP},      F_CrtYd},

    { { LAYER_INFO::CLASS::REF_DES,          LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          B_SilkS},
    { { LAYER_INFO::CLASS::REF_DES,          LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             F_SilkS},
    { { LAYER_INFO::CLASS::REF_DES,          LAYER_INFO::SUBCLASS::ASSEMBLY_BOTTOM},            B_Fab},
    { { LAYER_INFO::CLASS::REF_DES,          LAYER_INFO::SUBCLASS::ASSEMBLY_TOP},               F_Fab},
};

/**
 * Names for custom KiCad layers that correspond to pre-defined Allegro layers.
 *
 * Multiple class:subclasses can share a layer name, in which case, they will share a layer.
 *
 * This is a balance between running out of layers and dumping too much unrelated stuff on the same layer.
 */
static const std::unordered_map<LAYER_INFO, wxString> s_OptionalFixedMappings = {
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::DFA_BOUND_TOP},              "DFA_BOUND_TOP" },
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_DISPLAY_TOP},          "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_DISPLAY_BOTTOM},       "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::PACKAGE_GEOMETRY, LAYER_INFO::SUBCLASS::PGEOM_BODY_CENTER},          "BODY_CENTER" },
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_DIMENSION},            "DIMENSION" },
    { { LAYER_INFO::CLASS::DRAWING_FORMAT,   LAYER_INFO::SUBCLASS::DFMT_OUTLINE},               "PAGE_OUTLINE" },

    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "COMPVAL_TYPE_BOTTOM"},
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "COMPVAL_TYPE_TOP"},

    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "DEVICE_TYPE_BOTTOM"},
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "DEVICE_TYPE_TOP"},

    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "TOLERANCE_BOTTOM"},
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "TOLERANCE_TOP"},

    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "USER_PART_NUM_BOTTOM"},
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "USER_PART_NUM_TOP"},

    { { LAYER_INFO::CLASS::MANUFACTURING,    LAYER_INFO::SUBCLASS::MFR_AUTOSILK_BOTTOM},        "AUTOSILK_BOTTOM"},
    { { LAYER_INFO::CLASS::MANUFACTURING,    LAYER_INFO::SUBCLASS::MFR_AUTOSILK_TOP},           "AUTOSILK_TOP"},
};

// clang-format on


/**
 * Class to handle the mapping for Allero CLASS/SUBCLASS idiom to KiCad layers.
 */
class ALLEGRO::LAYER_MAPPER
{
    /**
     * Represents the information found in a single entry of a layer list.
     *
     * Will eventually become a KiCad layer.
     */
    struct CUSTOM_LAYER
    {
        wxString m_Name;
        // LAYER_ARTWORK: POSITIVE/NEGATIVE
        // LAYER_USE: empty, EMBEDDED_PLANE, ...?
        // bool m_IsConductor;
    };

public:
    LAYER_MAPPER( const RAW_BOARD& aRawBoard, BOARD& aBoard ):
        m_rawBoard( aRawBoard ), m_board( aBoard )
    {}

    void ProcessLayerList( uint8_t aClass, const BLK_0x2A_LAYER_LIST& aList )
    {
        // If we haven't seen this list yet, create and store the CUSTOM_LAYER list
        if( m_Lists.count( &aList ) == 0 )
        {
            std::vector<CUSTOM_LAYER>& classLayers = m_Lists[&aList];

            if( aList.m_RefEntries.has_value() )
            {
                for( const BLK_0x2A_LAYER_LIST::REF_ENTRY& entry : aList.m_RefEntries.value() )
                {
                    const wxString& layerName = m_rawBoard.GetString( entry.mLayerNameId );

                    classLayers.emplace_back( CUSTOM_LAYER( layerName ) );
                }
            }
            else if( aList.m_NonRefEntries.has_value() )
            {
                for( const BLK_0x2A_LAYER_LIST::NONREF_ENTRY& entry : aList.m_NonRefEntries.value() )
                {
                    classLayers.emplace_back( CUSTOM_LAYER( entry.m_Name ) );
                }
            }
            else
            {
                // Presumably a parsing error.
                THROW_IO_ERROR( "No ETCH layer list found." );
            }

            wxLogTrace( traceAllegroBuilder, "Added %lu layers for class %#04x, from 0x2A key %#010x",
                        classLayers.size(), aClass, aList.m_Key );
        }

        // Store the class ID -> 0x2A mapping
        m_ClassCustomLayerLists[aClass] = &m_Lists[&aList];
    }

    /**
     * Called after all the custom layers are loaded.
     *
     * Finalises things like layer counts and stores into the board
     */
    void FinalizeLayers()
    {
        std::unordered_map<const CUSTOM_LAYER*, PCB_LAYER_ID> layerToId;

        // Process all the Etch layers
        const std::vector<CUSTOM_LAYER>& etchLayers = *m_ClassCustomLayerLists[LAYER_INFO::CLASS::ETCH];

        const size_t numCuLayers = etchLayers.size();
        m_board.GetDesignSettings().SetCopperLayerCount( numCuLayers );
        for( size_t li = 0; li < numCuLayers; ++li )
        {
            const CUSTOM_LAYER& cLayer = etchLayers[li];
            const PCB_LAYER_ID  lId = getNthCopperLayer( li, numCuLayers );
            layerToId[&cLayer] = lId;

            m_board.SetLayerName( lId, cLayer.m_Name );
        }

        // At this point, we don't actually create all the custom layers, because in many designs, there
        // are loads of them, and if we do that, we run out of User layers (for now...).
        //
        // We'll assign dynamically as we discover usage of the layers.
    }

    PCB_LAYER_ID GetLayer( const LAYER_INFO& aLayerInfo )
    {
        // We already mapped and created the layer
        if( m_customLayerToKiMap.count( aLayerInfo ) )
            return m_customLayerToKiMap.at( aLayerInfo );

        // It's a static mapping
        if( s_LayerKiMap.count( aLayerInfo ) )
            return s_LayerKiMap.at( aLayerInfo );

        // Next, have a look and see if the class:subclass was recorded as a custom layer
        if( m_ClassCustomLayerLists.count( aLayerInfo.m_Class ) )
        {
            const std::vector<CUSTOM_LAYER>* cLayerList = m_ClassCustomLayerLists.at( aLayerInfo.m_Class );

            if( aLayerInfo.m_Subclass < cLayerList->size() )
            {
                // This subclass maps to a custom layer in this class
                const CUSTOM_LAYER& cLayer = cLayerList->at( aLayerInfo.m_Subclass );
                return mapCustomLayer( aLayerInfo, cLayer.m_Name );
            }
        }

        // Now, there may be layers that map to custom layers in KiCad, but are fixed in Allegro
        // (perhaps, DFA_BOUND_TOP), which means we won't find them in the layer lists.
        // We add them if we encounter them, with the names defined.
        if( s_OptionalFixedMappings.count( aLayerInfo ) )
        {
            const wxString& layerName = s_OptionalFixedMappings.at( aLayerInfo );
            return mapCustomLayer( aLayerInfo, layerName );
        }

        // Keep a record of what we failed to map
        static std::unordered_map<LAYER_INFO, int> s_UnknownLayers;

        if( s_UnknownLayers.count( aLayerInfo ) == 0 )
        {
            wxLogTrace( traceAllegroBuilder, "Failed to map class:subclass to layer: %#04x:%#04x", aLayerInfo.m_Class,
                        aLayerInfo.m_Subclass );
            s_UnknownLayers[aLayerInfo] = 1;
        }
        s_UnknownLayers[aLayerInfo]++;

        // Dump everything else here
        return Cmts_User;
    }

    /**
     * Allegro puts more graphics than just the polygon on PBT/B.
     *
     * Use this function to choose a user layer instead.
     */
    PCB_LAYER_ID GetPlaceBounds( bool aTop )
    {
        const wxString name = aTop ? "PLACE_BOUND_TOP" : "PLACE_BOUND_BOTTOM";

        // If it's been added already, use it
        if( m_MappedOptionalLayers.count( name ) )
        {
            return m_MappedOptionalLayers.at( name );
        }

        const PCB_LAYER_ID newLId = addUserLayer( name );
        m_MappedOptionalLayers[name] = newLId;
        return newLId;
    }

private:
    static PCB_LAYER_ID getNthCopperLayer( int aNum, int aTotal )
    {
        if( aNum == 0 )
            return F_Cu;
        if( aNum == aTotal - 1 )
            return B_Cu;
        return ToLAYER_ID( 2 * ( aNum + 1 ) );
    }

    static PCB_LAYER_ID getNthUserLayer( int aNum )
    {
        aNum = std::min( aNum, MAX_USER_DEFINED_LAYERS - 1 );
        return ToLAYER_ID( static_cast<int>( User_1 ) + 2 * aNum );
    }

    PCB_LAYER_ID mapCustomLayer( const LAYER_INFO& aLayerInfo, const wxString& aLayerName )
    {
        // See if we have mapped this layer name under a different class:subclass
        if( m_MappedOptionalLayers.count( aLayerName ) )
        {
            const PCB_LAYER_ID existingLId = m_MappedOptionalLayers.at( aLayerName );
            // Record the reuse
            m_customLayerToKiMap[aLayerInfo] = existingLId;
            return existingLId;
        }

        // First time we needed this name:
        // Add as a user layer and store for next time
        const PCB_LAYER_ID lId = addUserLayer( aLayerName );
        m_customLayerToKiMap[aLayerInfo] = lId;

        wxLogTrace( traceAllegroBuilder, "Adding mapping for %#04x:%#04x to %s", aLayerInfo.m_Class,
                    aLayerInfo.m_Subclass, aLayerName );
        return lId;
    }

    PCB_LAYER_ID addUserLayer( const wxString& aName )
    {
        const PCB_LAYER_ID lId = getNthUserLayer( m_numUserLayersUsed++ );
        m_board.GetDesignSettings().SetUserDefinedLayerCount( m_numUserLayersUsed );
        m_board.SetLayerName( lId, aName );
        wxLogTrace( traceAllegroBuilder, "Adding user layer %s: %s", LayerName( lId ), aName );
        return lId;
    }

    // Map of original layer list - we use this to store the CUSTOM_LAYERs, as well
    // as check that we only handle each one once
    std::unordered_map<const BLK_0x2A_LAYER_LIST*, std::vector<CUSTOM_LAYER>> m_Lists;

    // Which classes point to which layer lists (more than one class can point to one list.
    std::unordered_map<uint8_t, std::vector<CUSTOM_LAYER>*> m_ClassCustomLayerLists;

    /**
     * The main map from CLASS:SUBCLASS custom mappings to KiCadLayers
     *
     * This doesn't cover all the fixed layers, just created custom ones.
     */
    std::unordered_map<LAYER_INFO, PCB_LAYER_ID> m_customLayerToKiMap;

    /**
     * This is a map of named, but optional, Allegro layers that we have mapped to KiCad layers.
     *
     * This is done by name, because multiple class:subclass pairs may share the same name.
     */
    std::unordered_map<wxString, PCB_LAYER_ID> m_MappedOptionalLayers;

    int m_numUserLayersUsed = 0;

    // for looking up strings
    const RAW_BOARD& m_rawBoard;
    BOARD&           m_board;
};


BOARD_BUILDER::BOARD_BUILDER( const RAW_BOARD& aRawBoard, BOARD& aBoard, REPORTER& aReporter,
                              PROGRESS_REPORTER* aProgressReporter ) :
        m_rawBoard( aRawBoard ), m_board( aBoard ), m_reporter( aReporter ), m_progressReporter( aProgressReporter ),
        m_layerMapper( std::make_unique<LAYER_MAPPER>( m_rawBoard, m_board ) )
{
    double scale = 1;
    switch( m_rawBoard.m_Header->m_BoardUnits )
    {
    case BOARD_UNITS::IMPERIAL:
        // 1 mil = 25400 nm
        scale = 25400;
        break;
    case BOARD_UNITS::METRIC:
        // TODO: is the metric scale um?
        scale = 1000;
        break;
    default: THROW_IO_ERROR( "Unknown board units" ); break;
    }

    if( m_rawBoard.m_Header->m_UnitsDivisor == 0 )
        THROW_IO_ERROR( "Board units divisor is 0" );

    m_scale = scale / m_rawBoard.m_Header->m_UnitsDivisor;
}


BOARD_BUILDER::~BOARD_BUILDER()
{
}


VECTOR2I BOARD_BUILDER::scale( const VECTOR2I& aVector ) const
{
    const VECTOR2I vec{ aVector.x * m_scale, -aVector.y * m_scale };
    return vec;
}

int BOARD_BUILDER::scale( int aValue ) const
{
    return aValue * m_scale;
}


void BOARD_BUILDER::reportMissingBlock( uint8_t aKey, uint8_t aType ) const
{
    m_reporter.Report( wxString::Format( "Could not find expected block with key %#010x and type %#04x", aKey, aType ),
                       RPT_SEVERITY_WARNING );
}


void BOARD_BUILDER::reportUnexpectedBlockType( uint8_t aGot, uint8_t aExpected, uint32_t aKey ) const
{
    m_reporter.Report( wxString::Format( "Object with key {#010x} has unexpected type %#04x (expected %#04x)", aKey,
                                         aGot, aExpected ),
                       RPT_SEVERITY_WARNING );
}


void BOARD_BUILDER::cacheFontDefs()
{
    LL_WALKER x36_walker{ m_rawBoard.m_Header->m_LL_0x36.m_Head, m_rawBoard.m_Header->m_LL_0x36.m_Tail, m_rawBoard };

    bool encountered = false;

    for( const BLOCK_BASE* block : x36_walker )
    {
        if( block->GetBlockType() != 0x36 )
            continue;

        const BLK_0x36& blk0x36 = static_cast<const BLOCK<BLK_0x36>&>( *block ).GetData();

        if( blk0x36.m_Code != 0x08 )
            continue;

        if( encountered )
        {
            // This would be bad, because we won't get the indexes into the list right if there
            // it's made up of entries from more than one list of entries.
            m_reporter.Report( "Found more than one font definition lists in the 0x36 list.", RPT_SEVERITY_WARNING );
            break;
        }

        for( const auto& item : blk0x36.m_Items )
        {
            const auto& fontDef = std::get<BLK_0x36::FontDef_X08>( item );
            m_fontDefList.push_back( &fontDef );
        }

        encountered = true;
    }
}


void BOARD_BUILDER::createNets()
{
    // Incrementing netcode. We could also choose to, say, use the 0x1B key if we wanted
    int netCode = 1;

    std::vector<BOARD_ITEM*> bulkAdded;

    LL_WALKER netWalker{ m_rawBoard.m_Header->m_LL_0x1B_Nets, m_rawBoard };
    for( const BLOCK_BASE* block : netWalker )
    {
        const uint8_t type = block->GetBlockType();
        if( type != BLOCK_TYPE::x1B_NET )
        {
            m_reporter.Report( wxString::Format( "Found unexpected net object of type %#04x at offset %lu", type,
                                                 block->GetOffset() ),
                               RPT_SEVERITY_WARNING );
            continue;
        }

        const auto& netBlk = static_cast<const BLOCK<BLK_0x1B_NET>&>( *block ).GetData();

        // All Allegro nets seem to have names even if just 'N1234', but even if they didn't,
        // we can handle that in KiCad, as we're assigning new netcodes.
        const wxString& netName = m_rawBoard.GetString( netBlk.m_NetName );

        auto kiNetInfo = std::make_unique<NETINFO_ITEM>( &m_board, netName, netCode );
        netCode++;

        m_netCache[netBlk.m_Key] = kiNetInfo.get();
        bulkAdded.push_back( kiNetInfo.get() );
        m_board.Add( kiNetInfo.release(), ADD_MODE::BULK_APPEND );
    }

    m_board.FinalizeBulkAdd( bulkAdded );

    wxLogTrace( traceAllegroBuilder, "Added %lu nets", m_netCache.size() );
}


void BOARD_BUILDER::setupLayers()
{
    const auto& layerMap = m_rawBoard.m_Header->m_LayerMap;

    for( size_t i = 0; i < layerMap.size(); ++i )
    {
        const uint8_t classNum = static_cast<uint8_t>( i );

        const uint32_t x2aKey = layerMap[i].m_LayerList0x2A;

        if( x2aKey == 0 )
            continue;

        const BLK_0x2A_LAYER_LIST* layerList = expectBlockByKey<BLK_0x2A_LAYER_LIST>( x2aKey, 0x2A );

        // Probably an error
        if( !layerList )
            continue;

        m_layerMapper->ProcessLayerList( classNum, *layerList );
    }

    m_layerMapper->FinalizeLayers();
}


const BLK_0x36::FontDef_X08* BOARD_BUILDER::getFontDef( unsigned aIndex ) const
{
    if( aIndex == 0 || aIndex > m_fontDefList.size() )
    {
        m_reporter.Report(
                wxString::Format( "Font def index %u requested, have %lu entries", aIndex, m_fontDefList.size() ),
                RPT_SEVERITY_WARNING );
        return nullptr;
    }

    // The index appears to be 1-indexed (maybe 0 means something special?)
    aIndex -= 1;

    return m_fontDefList[aIndex];
}


std::unique_ptr<PCB_TEXT> BOARD_BUILDER::buildPcbText( const BLK_0x30_STR_WRAPPER& aStrWrapper,
                                                       BOARD_ITEM_CONTAINER&       aParent )
{
    std::unique_ptr<PCB_TEXT> text = std::make_unique<PCB_TEXT>( &aParent );

    VECTOR2I     textPos = scale( VECTOR2I{ aStrWrapper.m_CoordsX, aStrWrapper.m_CoordsY } );
    PCB_LAYER_ID layer = getLayer( aStrWrapper.m_Layer );

    text->SetPosition( textPos );
    text->SetLayer( layer );

    const BLK_0x31_SGRAPHIC* strGraphic = expectBlockByKey<BLK_0x31_SGRAPHIC>( aStrWrapper.m_StrGraphicPtr, 0x31 );

    if( !strGraphic )
    {
        m_reporter.Report( wxString::Format( "Failed to find string graphic (0x31) with key %#010x "
                                             "in string wrapper (0x30) with key %#010x",
                                             aStrWrapper.m_StrGraphicPtr, aStrWrapper.m_Key ),
                           RPT_SEVERITY_WARNING );
        return nullptr;
    }

    const BLK_0x30_STR_WRAPPER::TEXT_PROPERTIES* props = nullptr;

    if( aStrWrapper.m_Font.has_value() )
        props = &aStrWrapper.m_Font.value();

    if( !props && aStrWrapper.m_Font16x.has_value() )
        props = &aStrWrapper.m_Font16x.value();

    if( !props )
    {
        m_reporter.Report(
                wxString::Format( "Expected one of the font properties fields in 0x30 object (key %#010x) to be set.",
                                  aStrWrapper.m_Key ),
                RPT_SEVERITY_WARNING );
        return nullptr;
    }

    const BLK_0x36::FontDef_X08* fontDef = getFontDef( props->m_Key );

    if( !fontDef )
        return nullptr;

    text->SetText( strGraphic->m_Value );
    text->SetTextWidth( m_scale * fontDef->m_CharWidth );
    text->SetTextHeight( m_scale * fontDef->m_CharHeight );

    return text;
}


PCB_LAYER_ID BOARD_BUILDER::getLayer( const LAYER_INFO& aLayerInfo ) const
{
    return m_layerMapper->GetLayer( aLayerInfo );
}


std::vector<std::unique_ptr<PCB_SHAPE>> BOARD_BUILDER::buildShapes( const BLK_0x14&       aGraphic,
                                                                    BOARD_ITEM_CONTAINER& aParent )
{
    std::vector<std::unique_ptr<PCB_SHAPE>> shapes;

    PCB_LAYER_ID layer = getLayer( aGraphic.m_Layer );

    // Within the graphics list, we can get various lines and arcs on PLACE_BOUND_TOP, which
    // aren't actually the courtyard, which is a polygon in the 0x28 list. So, if we see such items,
    // remap them now to a specific other layer
    if( layer == F_CrtYd )
        layer = m_layerMapper->GetPlaceBounds( true );
    else if( layer == B_CrtYd )
        layer = m_layerMapper->GetPlaceBounds( false );
    const LL_WALKER segWalker{ aGraphic.m_SegmentPtr, aGraphic.m_Key, m_rawBoard };

    for( const BLOCK_BASE* segBlock : segWalker )
    {
        std::unique_ptr<PCB_SHAPE>& shape = shapes.emplace_back( std::make_unique<PCB_SHAPE>( &aParent ) );
        shape->SetLayer( layer );

        switch( segBlock->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *segBlock ).GetData();

            VECTOR2I start{ arc.m_StartX, arc.m_StartY };
            VECTOR2I end{ arc.m_EndX, arc.m_EndY };

            if( layer == Cmts_User )
            {
                wxLogTrace( traceAllegroBuilder, "Unmapped Arc: %#04x %#04x %s, %s", aGraphic.m_Layer.m_Class,
                            aGraphic.m_Layer.m_Subclass, start.Format(), end.Format() );
            }

            start = scale( start );
            end = scale( end );

            VECTOR2I c = scale( KiROUND( VECTOR2D{ arc.m_CenterX, arc.m_CenterY } ) );

            int radius = static_cast<int>( arc.m_Radius * m_scale );

            bool clockwise = false; // TODO - flag?
            // Probably follow fabmaster here for flipping, as I guess it's identical.

            shape->SetWidth( m_scale * arc.m_Width );

            if( start == end )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( c );
                shape->SetRadius( radius );
            }
            else
            {
                shape->SetShape( SHAPE_T::ARC );
                EDA_ANGLE startangle( start - c );
                EDA_ANGLE endangle( end - c );

                startangle.Normalize();
                endangle.Normalize();

                EDA_ANGLE angle = endangle - startangle;

                if( clockwise && angle < ANGLE_0 )
                    angle += ANGLE_360;
                if( !clockwise && angle > ANGLE_0 )
                    angle -= ANGLE_360;

                if( start == end )
                    angle = -ANGLE_360;

                VECTOR2I mid = start;
                RotatePoint( mid, c, -angle / 2.0 );

                shape->SetArcGeometry( start, mid, end );
            }
            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            shape->SetShape( SHAPE_T::SEGMENT );

            const auto& seg = static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *segBlock ).GetData();
            VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
            VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );
            const int   width = static_cast<int>( seg.m_Width );

            if( layer == Cmts_User )
            {
                wxLogTrace( traceAllegroBuilder, "Unmapped Seg: %#04x %#04x %s, %s", aGraphic.m_Layer.m_Class,
                            aGraphic.m_Layer.m_Subclass, start.Format(), end.Format() );
            }

            shape->SetStart( start );
            shape->SetEnd( end );
            shape->SetWidth( width * m_scale );
            break;
        }
        default:
        {
            break;
        }
        }
    }

    return shapes;
}


const BLK_0x07* BOARD_BUILDER::getFpInstRef( const BLK_0x2D& aFpInstance ) const
{
    uint32_t refKey = 0x00;

    if( aFpInstance.m_InstRef.has_value() )
        refKey = aFpInstance.m_InstRef.value();

    if( !refKey && aFpInstance.m_InstRef16x.has_value() )
        refKey = aFpInstance.m_InstRef16x.value();

    // This can happen, for example for dimension "symbols".
    if( refKey == 0 )
        return nullptr;

    const BLK_0x07* blk07 = expectBlockByKey<BLK_0x07>( refKey, 0x07 );
    return blk07;
}


std::unique_ptr<FOOTPRINT> BOARD_BUILDER::buildFootprint( const BLK_0x2D& aFpInstance )
{
    auto fp = std::make_unique<FOOTPRINT>( &m_board );

    const BLK_0x07* fpInstData = getFpInstRef( aFpInstance );

    wxString refDesStr;
    if( fpInstData )
    {
        refDesStr = m_rawBoard.GetString( fpInstData->m_RefDesRef );

        if( refDesStr.IsEmpty() )
        {
            // Does this happen even when there's an 0x07 block?
            m_reporter.Report( wxString::Format( "Empty ref des for 0x2D key %#010x", aFpInstance.m_Key ),
                               RPT_SEVERITY_WARNING );
        }
    }

    // We may update the PCB_FIELD layer if it's specified explicitly (e.g. with font size and so on),
    // but if not, set the refdes at least, but make it invisible
    fp->SetReference( refDesStr );
    fp->GetField( FIELD_T::REFERENCE )->SetVisible( false );

    const VECTOR2I  fpPos = scale( VECTOR2I{ aFpInstance.m_CoordX, aFpInstance.m_CoordY } );
    const EDA_ANGLE rotation{ aFpInstance.m_Rotation / 1000. };

    fp->SetPosition( fpPos );
    fp->SetOrientation( rotation );

    const LL_WALKER graphicsWalker{ aFpInstance.m_GraphicPtr, aFpInstance.m_Key, m_rawBoard };
    for( const BLOCK_BASE* graphicsBlock : graphicsWalker )
    {
        const uint8_t type = graphicsBlock->GetBlockType();
        if( type == 0x14 )
        {
            const auto& graphics = static_cast<const BLOCK<BLK_0x14>&>( *graphicsBlock ).GetData();

            std::vector<std::unique_ptr<PCB_SHAPE>> shapes = buildShapes( graphics, *fp );

            for( std::unique_ptr<PCB_SHAPE>& shape : shapes )
            {
                // shape->Rotate( fpPos, rotation );
                fp->Add( shape.release() );
            }
        }
        else
        {
            // This probably means we're dropping something on the floor
            // But as so far, only 0x14s occur in this list.
            m_reporter.Report( wxString::Format( "Unexpected type in graphics list: %#04x", type ),
                               RPT_SEVERITY_WARNING );
        }
    }

    const LL_WALKER textWalker{ aFpInstance.m_TextPtr, aFpInstance.m_Key, m_rawBoard };
    for( const BLOCK_BASE* textBlock : textWalker )
    {
        const uint8_t type = textBlock->GetBlockType();
        if( type == 0x30 )
        {
            const auto& strWrapper = static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( *textBlock ).GetData();

            std::unique_ptr<PCB_TEXT> text = buildPcbText( strWrapper, *fp );

            if( text )
            {
                if( strWrapper.m_Layer.m_Class == LAYER_INFO::CLASS::REF_DES
                    && ( strWrapper.m_Layer.m_Subclass == LAYER_INFO::SUBCLASS::SILKSCREEN_TOP ) )
                {
                    // This is a visible silkscreen refdes - use it to update the PCB_FIELD refdes.
                    PCB_FIELD* const refDes = fp->GetField( FIELD_T::REFERENCE );

                    // KiCad netlisting requires parts to have non-digit + digit annotation.
                    // If the reference begins with a number, we prepend 'UNK' (unknown) for the source
                    // designator
                    if( !std::isalpha( text->GetText()[0] ) )
                        text->SetText( wxString( "UNK" ) + text->GetText() );

                    // Update all ref-des parameters in-place
                    *refDes = PCB_FIELD( *text, FIELD_T::REFERENCE );
                }
                else
                {
                    // Add it as a text item
                    // In Allegro, the REF_DES::ASSEMBLY_x text is also in the REF_DES class, but
                    // in KiCad, it's just a Fab layer text item with a special value.
                    if( strWrapper.m_Layer.m_Class == LAYER_INFO::CLASS::REF_DES
                        && ( strWrapper.m_Layer.m_Subclass == LAYER_INFO::SUBCLASS::ASSEMBLY_TOP ) )
                    {
                        text->SetText( "${REFERENCE}" );
                    }

                    // text->Rotate( fpPos, rotation );
                    fp->Add( text.release() );
                }
            }
        }
    }

    // Find the pads

    return fp;
}


bool BOARD_BUILDER::BuildBoard()
{
    if( m_progressReporter )
    {
        m_progressReporter->AddPhases( 3 );
        m_progressReporter->AdvancePhase( _( "Constructing caches" ) );
    }

    cacheFontDefs();
    setupLayers();

    if( m_progressReporter )
        m_progressReporter->AdvancePhase( _( "Creating nets" ) );

    createNets();

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Converting footprints" ) );
    }

    const LL_WALKER          fpWalker( m_rawBoard.m_Header->m_LL_0x2B, m_rawBoard );
    std::vector<BOARD_ITEM*> bulkAddedItems;

    for( const BLOCK_BASE* fpContainer : fpWalker )
    {
        if( fpContainer->GetBlockType() == 0x2B )
        {
            const BLK_0x2B& fpBlock = static_cast<const BLOCK<BLK_0x2B>&>( *fpContainer ).GetData();

            const LL_WALKER instWalker( fpBlock.m_FirstInstPtr, fpBlock.m_Key, m_rawBoard );

            unsigned numInsts = 0;
            for( const BLOCK_BASE* instBlock : instWalker )
            {
                if( instBlock->GetBlockType() != 0x2D )
                {
                    m_reporter.Report( wxString::Format( "Unexpected object of type %#04x found in footprint %#010x",
                                                         instBlock->GetBlockType(), fpBlock.m_Key ),
                                       RPT_SEVERITY_ERROR );
                }
                else
                {
                    const auto& inst = static_cast<const BLOCK<BLK_0x2D>&>( *instBlock ).GetData();

                    std::unique_ptr<FOOTPRINT> fp = buildFootprint( inst );

                    if( fp )
                    {
                        bulkAddedItems.push_back( fp.get() );
                        m_board.Add( fp.release(), ADD_MODE::BULK_APPEND, true );
                    }
                    else
                    {
                        m_reporter.Report(
                                wxString::Format( "Failed to construct footprint for 0x2D key %#010x", inst.m_Key ),
                                RPT_SEVERITY_ERROR );
                    }
                }
                numInsts++;
            }
        }
    }

    if( !bulkAddedItems.empty() )
        m_board.FinalizeBulkAdd( bulkAddedItems );

    return false;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
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

#include "allegro_builder.h"

#include <chrono>
#include <cmath>
#include <limits>
#include <set>
#include <tuple>
#include <unordered_set>

#include <convert/allegro_pcb_structs.h>

#include <wx/log.h>

#include <core/profile.h>

#include <base_units.h>
#include <board_design_settings.h>
#include <geometry/shape_utils.h>
#include <project/net_settings.h>
#include <footprint.h>
#include <netclass.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <convert_basic_shapes_to_polygon.h>


using namespace ALLEGRO;


/**
 * Flag to enable debug output of Allegro board construction.
 *
 * Use "KICAD_ALLEGRO_BUILDER" to enable debug output.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceAllegroBuilder = wxT( "KICAD_ALLEGRO_BUILDER" );
static const wxChar* const traceAllegroPerf = wxT( "KICAD_ALLEGRO_PERF" );


template <typename BLK_T>
const BLK_T& BlockDataAs( const BLOCK_BASE& aBlock )
{
    return static_cast<const BLOCK<BLK_T>&>( aBlock ).GetData();
}


#define BLK_FIELD( BLK_T, FIELD ) BlockDataAs<BLK_T>( aBlock ).FIELD


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
    case 0x01: return BLK_FIELD( BLK_0x01_ARC, m_Next );
    case 0x03: return BLK_FIELD( BLK_0x03_FIELD, m_Next );
    case 0x04: return BLK_FIELD( BLK_0x04_NET_ASSIGNMENT, m_Next );
    case 0x05: return BLK_FIELD( BLK_0x05_TRACK, m_Next );
    case 0x0E: return BLK_FIELD( BLK_0x0E_RECT, m_Next );
    case 0x14: return BLK_FIELD( BLK_0x14_GRAPHIC, m_Next );
    case 0x15:
    case 0x16:
    case 0x17: return BLK_FIELD( BLK_0x15_16_17_SEGMENT, m_Next );
    case 0x1B: return BLK_FIELD( BLK_0x1B_NET, m_Next );
    case 0x1D: return BLK_FIELD( BLK_0x1D_CONSTRAINT_SET, m_Next );
    case 0x1E: return BLK_FIELD( BLK_0x1E_SI_MODEL, m_Next );
    case 0x1F: return BLK_FIELD( BLK_0x1F_PADSTACK_DIM, m_Next );
    case 0x2B: return BLK_FIELD( BLK_0x2B_FOOTPRINT_DEF, m_Next );
    case 0x2D: return BLK_FIELD( BLK_0x2D_FOOTPRINT_INST, m_Next );
    case 0x2E: return BLK_FIELD( BLK_0x2E_CONNECTION, m_Next );
    case 0x30: return BLK_FIELD( BLK_0x30_STR_WRAPPER, m_Next );
    case 0x31: return 0; // Doesn't exist
    case 0x32: return BLK_FIELD( BLK_0x32_PLACED_PAD, m_Next );
    case 0x24: return BLK_FIELD( BLK_0x24_RECT, m_Next );
    case 0x28: return BLK_FIELD( BLK_0x28_SHAPE, m_Next );
    case 0x2C: return BLK_FIELD( BLK_0x2C_TABLE, m_Next );
    case 0x33: return BLK_FIELD( BLK_0x33_VIA, m_Next );
    case 0x36: return BLK_FIELD( BLK_0x36_DEF_TABLE, m_Next );
    case 0x37: return BLK_FIELD( BLK_0x37_PTR_ARRAY, m_Next );
    default: return 0;
    }
}


/**
 * "Get Next" function for the pad list in a footprint's 0x32 list.
 */
static uint32_t PadGetNextInFootprint( const BLOCK_BASE& aBlock )
{
    const uint8_t type = aBlock.GetBlockType();

    if( type != 0x32 )
    {
        THROW_IO_ERROR(
                wxString::Format( "Unexpected next item in 0x32 pad list: block type %#04x, offset %#lx, key %#010x",
                                  type, aBlock.GetOffset(), aBlock.GetKey() ) );
    }

    // When iterating in a footprint use this field, not m_Next.
    return BLK_FIELD( BLK_0x32_PLACED_PAD, m_NextInFp );
}


class LL_WALKER
{
public:

    using NEXT_FUNC_T = std::function<uint32_t( const BLOCK_BASE& )>;

    class iterator
    {
    public:
        iterator( uint32_t aCurrent, uint32_t aTail, const BRD_DB& aBoard, NEXT_FUNC_T aNextFunc ) :
                m_current( aCurrent ), m_tail( aTail ), m_board( aBoard ), m_NextFunc( aNextFunc )
        {
            m_currBlock = m_board.GetObjectByKey( m_current );

            if( !m_currBlock )
                m_current = 0;
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
                m_current = m_NextFunc( *m_currBlock );

                if( m_current == m_tail || m_board.IsSentinel( m_current ) )
                {
                    m_current = 0;
                }
                else
                {
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
        const BRD_DB&  m_board;
        NEXT_FUNC_T       m_NextFunc;
    };

    LL_WALKER( uint32_t aHead, uint32_t aTail, const BRD_DB& aBoard ) :
            m_head( aHead ), m_tail( aTail ), m_board( aBoard )
    {
        // The default next function
        m_nextFunction = GetPrimaryNext;
    }

    LL_WALKER( const FILE_HEADER::LINKED_LIST& aList, const BRD_DB& aBoard ) :
            LL_WALKER( aList.m_Head, aList.m_Tail, aBoard )
    {
    }

    iterator begin() const { return iterator( m_head, m_tail, m_board, m_nextFunction ); }
    iterator end() const { return iterator( 0, m_tail, m_board, m_nextFunction ); }

    void SetNextFunc( NEXT_FUNC_T aNextFunc ) { m_nextFunction = aNextFunc; }

private:
    uint32_t         m_head;
    uint32_t         m_tail;
    const BRD_DB& m_board;

    // This is the function that can get the next item in a list. By default
    NEXT_FUNC_T m_nextFunction;
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

    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_OUTLINE},              Edge_Cuts},
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_DESIGN_OUTLINE},       Edge_Cuts},
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_SILKSCREEN_TOP},       F_SilkS},
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_SILKSCREEN_BOTTOM},    B_SilkS},
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_SOLDERMASK_TOP},       F_Mask},
    { { LAYER_INFO::CLASS::BOARD_GEOMETRY,   LAYER_INFO::SUBCLASS::BGEOM_SOLDERMASK_BOTTOM},    B_Mask},

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

    { { LAYER_INFO::CLASS::MANUFACTURING,    LAYER_INFO::SUBCLASS::MFR_AUTOSILK_BOTTOM},        B_SilkS},
    { { LAYER_INFO::CLASS::MANUFACTURING,    LAYER_INFO::SUBCLASS::MFR_AUTOSILK_TOP},           F_SilkS},
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
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::DISPLAY_TOP},                "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "COMPVAL_TYPE_BOTTOM"},
    { { LAYER_INFO::CLASS::COMPONENT_VALUE,  LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "COMPVAL_TYPE_TOP"},

    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::DISPLAY_TOP},                "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "DEVICE_TYPE_BOTTOM"},
    { { LAYER_INFO::CLASS::DEVICE_TYPE,      LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "DEVICE_TYPE_TOP"},

    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::DISPLAY_TOP},                "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "TOLERANCE_BOTTOM"},
    { { LAYER_INFO::CLASS::TOLERANCE,        LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "TOLERANCE_TOP"},

    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM},             "DISPLAY_BOTTOM" },
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::DISPLAY_TOP},                "DISPLAY_TOP" },
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM},          "USER_PART_NUM_BOTTOM"},
    { { LAYER_INFO::CLASS::USER_PART_NUMBER, LAYER_INFO::SUBCLASS::SILKSCREEN_TOP},             "USER_PART_NUM_TOP"},

    { { LAYER_INFO::CLASS::MANUFACTURING,    LAYER_INFO::SUBCLASS::MFR_XSECTION_CHART},         "XSECTION_CHART" },
};

// clang-format on


/**
 * Build a unique display name for a LAYER_INFO entry from the static maps above,
 * suitable for presentation in the layer mapping dialog.
 *
 * Refer to https://www.artwork.com/all2dxf/alleggeo.htm for layer orders.
 */
static wxString layerInfoDisplayName( const LAYER_INFO& aLayerInfo )
{
    // clang-format off
    static const std::unordered_map<uint8_t, wxString> s_ClassNames = {
        { LAYER_INFO::CLASS::BOARD_GEOMETRY,        wxS( "Board Geometry" ) },
        { LAYER_INFO::CLASS::COMPONENT_VALUE,       wxS( "Component Value" ) },
        { LAYER_INFO::CLASS::DEVICE_TYPE,           wxS( "Device Type" ) },
        { LAYER_INFO::CLASS::DRAWING_FORMAT,        wxS( "Drawing Format" ) },
        { LAYER_INFO::CLASS::ETCH,                  wxS( "Etch" ) },
        { LAYER_INFO::CLASS::MANUFACTURING,         wxS( "Manufacturing" ) },
        { LAYER_INFO::CLASS::PACKAGE_GEOMETRY,      wxS( "Package Geometry" ) },
        { LAYER_INFO::CLASS::PACKAGE_KEEPIN,        wxS( "Package Keepin" ) },
        { LAYER_INFO::CLASS::PACKAGE_KEEPOUT,       wxS( "Package Keepout" ) },
        { LAYER_INFO::CLASS::PIN,                   wxS( "Pin" ) },
        { LAYER_INFO::CLASS::REF_DES,               wxS( "Ref Des" ) },
        { LAYER_INFO::CLASS::ROUTE_KEEPIN,          wxS( "Route Keepin" ) },
        { LAYER_INFO::CLASS::ROUTE_KEEPOUT,         wxS( "Route Keepout" ) },
        { LAYER_INFO::CLASS::TOLERANCE,             wxS( "Tolerance" ) },
        { LAYER_INFO::CLASS::USER_PART_NUMBER,      wxS( "User Part Number" ) },
        { LAYER_INFO::CLASS::VIA_CLASS,             wxS( "Via Class" ) },
        { LAYER_INFO::CLASS::VIA_KEEPOUT,           wxS( "Via Keepout" ) },
        { LAYER_INFO::CLASS::ANTI_ETCH,             wxS( "Anti Etch" ) },
        { LAYER_INFO::CLASS::BOUNDARY,              wxS( "Boundary" ) },
        { LAYER_INFO::CLASS::CONSTRAINTS_REGION,    wxS( "Constraints Region" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_BoardGeomSubclassNames = {
        { LAYER_INFO::SUBCLASS::BGEOM_OUTLINE,              wxS( "Outline" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_CONSTRAINT_AREA,      wxS( "Constraint Area" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_OFF_GRID_AREA,        wxS( "Off Grid Area" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SOLDERMASK_BOTTOM,    wxS( "Soldermask Bottom" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SOLDERMASK_TOP,       wxS( "Soldermask Top" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_ASSEMBLY_DETAIL,      wxS( "Assembly Detail" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SILKSCREEN_BOTTOM,    wxS( "Silkscreen Bottom" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SILKSCREEN_TOP,       wxS( "Silkscreen Top" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SWITCH_AREA_BOTTOM,   wxS( "Switch Area Bottom" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_SWITCH_AREA_TOP,      wxS( "Switch Area Top" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_BOTH_ROOMS,           wxS( "Both Rooms" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_BOTTOM_ROOM,          wxS( "Bottom Room" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_TOP_ROOM,             wxS( "Top Room" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_PLACE_GRID_BOTTOM,    wxS( "Place Grid Bottom" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_PLACE_GRID_TOP,       wxS( "Place Grid Top" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_DIMENSION,            wxS( "Dimension" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_TOOLING_CORNERS,      wxS( "Tooling Corners" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_ASSEMBLY_NOTES,       wxS( "Assembly Notes" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_PLATING_BAR,          wxS( "Plating Bar" ) },
        { LAYER_INFO::SUBCLASS::BGEOM_DESIGN_OUTLINE,       wxS( "Design Outline" ) },

    };

    static const std::unordered_map<uint8_t, wxString> s_ComponentValueSubclassNames = {
        { LAYER_INFO::SUBCLASS::DISPLAY_BOTTOM,             wxS( "Display Bottom" ) },
        { LAYER_INFO::SUBCLASS::DISPLAY_TOP,                wxS( "Display Top" ) },
        { LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM,          wxS( "Silkscreen Bottom" ) },
        { LAYER_INFO::SUBCLASS::SILKSCREEN_TOP,             wxS( "Silkscreen Top" ) },
        { LAYER_INFO::SUBCLASS::ASSEMBLY_BOTTOM,            wxS( "Assembly Bottom" ) },
        { LAYER_INFO::SUBCLASS::ASSEMBLY_TOP,               wxS( "Assembly Top" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_DrawingFormatSubclassNames = {
        { LAYER_INFO::SUBCLASS::DFMT_REVISION_DATA,         wxS( "Revision Data" ) },
        { LAYER_INFO::SUBCLASS::DFMT_REVISION_BLOCK,        wxS( "Revision Block" ) },
        { LAYER_INFO::SUBCLASS::DFMT_TITLE_DATA,            wxS( "Title Data" ) },
        { LAYER_INFO::SUBCLASS::DFMT_TITLE_BLOCK,           wxS( "Title Block" ) },
        { LAYER_INFO::SUBCLASS::DFMT_OUTLINE,               wxS( "Outline" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_PackageGeometrySubclassNames = {
        { LAYER_INFO::SUBCLASS::DFA_BOUND_BOTTOM,           wxS( "DFA Bound Bottom" ) },
        { LAYER_INFO::SUBCLASS::DFA_BOUND_TOP,              wxS( "DFA Bound Top" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_DISPLAY_BOTTOM,       wxS( "Display Bottom" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_DISPLAY_TOP,          wxS( "Display Top" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_SOLDERMASK_BOTTOM,    wxS( "Soldermask Bottom" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_SOLDERMASK_TOP,       wxS( "Soldermask Top" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_BODY_CENTER,          wxS( "Body Center" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_SILKSCREEN_BOTTOM,    wxS( "Silkscreen Bottom" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_SILKSCREEN_TOP,       wxS( "Silkscreen Top" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_PAD_STACK_NAME,       wxS( "Pad Stack Name" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_PIN_NUMBER,           wxS( "Pin Number" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_PLACE_BOUND_BOTTOM,   wxS( "Place Bound Bottom" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_PLACE_BOUND_TOP,      wxS( "Place Bound Top" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_ASSEMBLY_BOTTOM,      wxS( "Assembly Bottom" ) },
        { LAYER_INFO::SUBCLASS::PGEOM_ASSEMBLY_TOP,         wxS( "Assembly Top" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_ManufacturingSubclassNames = {
        { LAYER_INFO::SUBCLASS::MFR_XSECTION_CHART,           wxS( "X-Section Chart" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_PROBE_BOTTOM,          wxS( "No Probe Bottom" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_PROBE_TOP,             wxS( "No Probe Top" ) },
        { LAYER_INFO::SUBCLASS::MFR_AUTOSILK_BOTTOM,          wxS( "AutoSilk Bottom" ) },
        { LAYER_INFO::SUBCLASS::MFR_AUTOSILK_TOP,             wxS( "AutoSilk Top" ) },
        { LAYER_INFO::SUBCLASS::MFR_PROBE_BOTTOM,             wxS( "Probe Bottom" ) },
        { LAYER_INFO::SUBCLASS::MFR_PROBE_TOP,                wxS( "Probe Top" ) },
        { LAYER_INFO::SUBCLASS::MFR_NCDRILL_FIGURE,           wxS( "NC Drill Figure" ) },
        { LAYER_INFO::SUBCLASS::MFR_NCDRILL_LEGEND,           wxS( "NC Drill Legend" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_GLOSS_INTERNAL,        wxS( "No Gloss Internal" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_GLOSS_BOTTOM,          wxS( "No Gloss Bottom" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_GLOSS_TOP,             wxS( "No Gloss Top" ) },
        { LAYER_INFO::SUBCLASS::MFR_NO_GLOSS_ALL,             wxS( "No Gloss All" ) },
        { LAYER_INFO::SUBCLASS::MFR_PHOTOPLOT_OUTLINE,        wxS( "Photoplot Outline" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_AnalysisSubclassNames = {
        { LAYER_INFO::SUBCLASS::ANALYSIS_PCB_TEMPERATURE,     wxS( "PCB Temperature" ) },
        { LAYER_INFO::SUBCLASS::ANALYSIS_HIGH_ISOCONTOUR,     wxS( "High IsoContour" ) },
        { LAYER_INFO::SUBCLASS::ANALYSIS_MEDIUM3_ISOCONTOUR,  wxS( "Medium3 IsoContour" ) },
        { LAYER_INFO::SUBCLASS::ANALYSIS_MEDIUM2_ISOCONTOUR,  wxS( "Medium2 IsoContour" ) },
        { LAYER_INFO::SUBCLASS::ANALYSIS_MEDIUM1_ISOCONTOUR,  wxS( "Medium1 IsoContour" ) },
        { LAYER_INFO::SUBCLASS::ANALYSIS_LOW_ISOCONTOUR,      wxS( "Low IsoContour" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_ConstraintSubclassNames = {
        { LAYER_INFO::SUBCLASS::CREG_ALL,                     wxS( "All" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_KeepinSubclassNames = {
        { LAYER_INFO::SUBCLASS::KEEPIN_ALL,                   wxS( "All" ) },
    };

    static const std::unordered_map<uint8_t, wxString> s_KeepoutSubclassNames = {
        { LAYER_INFO::SUBCLASS::KEEPOUT_ALL,                  wxS( "All" ) },
        { LAYER_INFO::SUBCLASS::KEEPOUT_TOP,                  wxS( "Top" ) },
        { LAYER_INFO::SUBCLASS::KEEPOUT_BOTTOM,               wxS( "Bottom" ) },
    };

    static const std::unordered_map<uint8_t, const std::unordered_map<uint8_t, wxString>&> s_SubclassNameMaps = {
        { LAYER_INFO::CLASS::BOARD_GEOMETRY,        s_BoardGeomSubclassNames },

        // These classes all share the same subclass names
        { LAYER_INFO::CLASS::COMPONENT_VALUE,       s_ComponentValueSubclassNames },
        { LAYER_INFO::CLASS::DEVICE_TYPE,           s_ComponentValueSubclassNames },
        { LAYER_INFO::CLASS::REF_DES,               s_ComponentValueSubclassNames },
        { LAYER_INFO::CLASS::TOLERANCE,             s_ComponentValueSubclassNames },
        { LAYER_INFO::CLASS::USER_PART_NUMBER,      s_ComponentValueSubclassNames },

        { LAYER_INFO::CLASS::DRAWING_FORMAT,        s_DrawingFormatSubclassNames },
        { LAYER_INFO::CLASS::PACKAGE_GEOMETRY,      s_PackageGeometrySubclassNames },
        { LAYER_INFO::CLASS::MANUFACTURING,         s_ManufacturingSubclassNames },
        { LAYER_INFO::CLASS::ANALYSIS,              s_AnalysisSubclassNames },
        { LAYER_INFO::CLASS::CONSTRAINTS_REGION,    s_ConstraintSubclassNames },
        { LAYER_INFO::CLASS::PACKAGE_KEEPIN,        s_KeepinSubclassNames },
        { LAYER_INFO::CLASS::PACKAGE_KEEPOUT,       s_KeepoutSubclassNames },
        { LAYER_INFO::CLASS::ROUTE_KEEPIN,          s_KeepinSubclassNames },
        { LAYER_INFO::CLASS::ROUTE_KEEPOUT,         s_KeepoutSubclassNames },
        { LAYER_INFO::CLASS::VIA_KEEPOUT,           s_KeepoutSubclassNames },
    };
    // clang-format on

    wxString   className;
    const auto classIt = s_ClassNames.find( aLayerInfo.m_Class );

    if( classIt != s_ClassNames.end() )
        className = classIt->second;
    else
        className = wxString::Format( wxS( "Class_%02X" ), aLayerInfo.m_Class );

    wxString subclassName;

    // Find the right subclass name map for this class
    auto classMapIt = s_SubclassNameMaps.find( aLayerInfo.m_Class );

    if( classMapIt != s_SubclassNameMaps.end() )
    {
        const std::unordered_map<uint8_t, wxString>& subclassMap = classMapIt->second;

        const auto subIt = subclassMap.find( aLayerInfo.m_Subclass );

        if( subIt != subclassMap.end() )
            subclassName = subIt->second;
        else
        {
            // This subclass seems not to have a known name
            subclassName = wxString::Format( wxS( "Subclass_%02X" ), aLayerInfo.m_Subclass );
        }
    }
    else
    {
        // Don't have a specific map for this class, just do a generic one.
        subclassName = wxString::Format( wxS( "Subclass_%02X" ), aLayerInfo.m_Subclass );
    }

    return className + wxS( "/" ) + subclassName;
}


/**
 * Some layers map to KiCad rule areas (zones) - for example a package keepout
 * on ALL maps to a rule area in KiCad.
 *
 * Keepins are bit trickier, but they're still rule areas and might need
 * custom DRC rules.
 */
static bool layerIsZone( const LAYER_INFO& aLayerInfo )
{
    if ( aLayerInfo.m_Class == LAYER_INFO::CLASS::PACKAGE_KEEPIN ||
         aLayerInfo.m_Class == LAYER_INFO::CLASS::ROUTE_KEEPIN ||
         aLayerInfo.m_Class == LAYER_INFO::CLASS::PACKAGE_KEEPOUT ||
         aLayerInfo.m_Class == LAYER_INFO::CLASS::ROUTE_KEEPOUT ||
         aLayerInfo.m_Class == LAYER_INFO::CLASS::VIA_KEEPOUT )
        return true;

    return false;
}


/**
 * Some blocks report layer info - if they do, return it else std::nullopt
 */
static std::optional<LAYER_INFO> tryLayerFromBlock( const BLOCK_BASE& aBlock )
{
    switch( aBlock.GetBlockType() )
    {
    case 0x0e:
    {
        const auto& net = BlockDataAs<BLK_0x0E_RECT>( aBlock );
        return net.m_Layer;
    }
    case 0x14:
    {
        const auto& trace = BlockDataAs<BLK_0x14_GRAPHIC>( aBlock );
        return trace.m_Layer;
    }
    case 0x24:
    {
        const auto& rect = BlockDataAs<BLK_0x24_RECT>( aBlock );
        return rect.m_Layer;
    }
    case 0x28:
    {
        const auto& shape = BlockDataAs<BLK_0x28_SHAPE>( aBlock );
        return shape.m_Layer;
    }
    }

    return std::nullopt;
}


/**
 * Get a layer from a block that has layer info.

 * It's an error to request this from a block that doesn't support it.
 */
LAYER_INFO expectLayerFromBlock( const BLOCK_BASE& aBlock )
{
    std::optional<LAYER_INFO> layerInfo = tryLayerFromBlock( aBlock );

    // Programming error - should only call this function if we're sure the block has layer info
    wxCHECK( layerInfo.has_value(), LAYER_INFO() );

    return layerInfo.value();
}


/**
 * Class to handle the mapping for Allegro CLASS/SUBCLASS idiom to KiCad layers.
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
    LAYER_MAPPER( const BRD_DB& aRawBoard, BOARD& aBoard, const LAYER_MAPPING_HANDLER& aLayerMappingHandler ) :
            m_layerMappingHandler( aLayerMappingHandler ),
            m_brdDb( aRawBoard ),
            m_board( aBoard )
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
                    const wxString& layerName = m_brdDb.GetString( entry.mLayerNameId );

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

            wxLogTrace( traceAllegroBuilder, "Added %zu layers for class %#04x, from 0x2A key %#010x",
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
        auto customLayerIt = m_ClassCustomLayerLists.find( LAYER_INFO::CLASS::ETCH );

        if( customLayerIt == m_ClassCustomLayerLists.end() || !customLayerIt->second )
        {
            wxLogTrace( traceAllegroBuilder, "No ETCH layer class found; cannot finalize layers" );
            return;
        }

        const std::vector<CUSTOM_LAYER>& etchLayers = *customLayerIt->second;
        const size_t                     numCuLayers = etchLayers.size();

        m_board.GetDesignSettings().SetCopperLayerCount( numCuLayers );

        std::vector<INPUT_LAYER_DESC> inputLayers;

        for( size_t li = 0; li < numCuLayers; ++li )
        {
            INPUT_LAYER_DESC desc;
            desc.Name = etchLayers[li].m_Name;
            desc.AutoMapLayer = getNthCopperLayer( li, numCuLayers );
            desc.PermittedLayers = LSET::AllCuMask();
            desc.Required = true;
            inputLayers.push_back( desc );
        }

        // Add non-ETCH custom layers so they appear in the layer mapping dialog
        const std::vector<CUSTOM_LAYER>* etchList = m_ClassCustomLayerLists[LAYER_INFO::CLASS::ETCH];
        int nextAutoUser = 0;

        for( const auto& [classId, layerList] : m_ClassCustomLayerLists )
        {
            if( classId == LAYER_INFO::CLASS::ETCH || layerList == etchList )
                continue;

            for( size_t si = 0; si < layerList->size(); ++si )
            {
                const LAYER_INFO li{ classId, static_cast<uint8_t>( si ) };

                // Skip entries already covered by s_LayerKiMap
                if( s_LayerKiMap.count( li ) )
                    continue;

                INPUT_LAYER_DESC desc;
                desc.Name = layerInfoDisplayName( li );

                if( layerList->at( si ).m_Name.length() > 0 )
                    desc.Name = layerList->at( si ).m_Name;

                desc.AutoMapLayer = getNthUserLayer( nextAutoUser++ );
                desc.PermittedLayers = LSET::AllLayersMask();
                desc.Required = false;
                inputLayers.push_back( desc );

                m_customLayerDialogNames[li] = desc.Name;
            }
        }

        // The layers that maybe lump together multiple Allegro class:subclasses
        // into a single, named, KiCad layer
        for( const auto& [layerName, kiLayer] : m_MappedOptionalLayers )
        {
            INPUT_LAYER_DESC desc;
            desc.Name = layerName;
            desc.AutoMapLayer = kiLayer;
            desc.PermittedLayers = LSET::AllLayersMask();
            desc.Required = false;
            inputLayers.push_back( desc );
        }

        for( const auto& [layerInfo, kiLayer] : s_LayerKiMap )
        {
            INPUT_LAYER_DESC desc;
            desc.Name = layerInfoDisplayName( layerInfo );
            desc.AutoMapLayer = kiLayer;
            desc.PermittedLayers = LSET::AllLayersMask();
            desc.Required = false;
            inputLayers.push_back( desc );
        }

        std::map<wxString, PCB_LAYER_ID> resolvedMapping = m_layerMappingHandler( inputLayers );

        // Apply copper layer mapping
        for( size_t li = 0; li < numCuLayers; ++li )
        {
            const LAYER_INFO layerInfo{ LAYER_INFO::CLASS::ETCH, static_cast<uint8_t>( li ) };
            const wxString&  layerName = etchLayers[li].m_Name;

            auto it = resolvedMapping.find( layerName );
            PCB_LAYER_ID lId = ( it != resolvedMapping.end() ) ? it->second
                                                               : getNthCopperLayer( li, numCuLayers );

            m_customLayerToKiMap[layerInfo] = lId;
            m_board.SetLayerName( lId, layerName );
        }

        // Apply non-copper static layer mapping from the handler result
        for( const auto& [layerInfo, defaultKiLayer] : s_LayerKiMap )
        {
            const wxString displayName = layerInfoDisplayName( layerInfo );

            auto rmIt = resolvedMapping.find( displayName );

            if( rmIt != resolvedMapping.end() && rmIt->second != PCB_LAYER_ID::UNDEFINED_LAYER )
            {
                m_staticLayerOverrides[layerInfo] = rmIt->second;
            }
        }

        // Apply custom layer mapping from the handler result
        for( const auto& [layerInfo, dialogName] : m_customLayerDialogNames )
        {
            auto rmIt = resolvedMapping.find( dialogName );

            if( rmIt != resolvedMapping.end() && rmIt->second != PCB_LAYER_ID::UNDEFINED_LAYER )
            {
                m_customLayerToKiMap[layerInfo] = rmIt->second;
                m_board.SetLayerName( rmIt->second, dialogName );
            }
        }

        // Enable all the layers we ended up mapping to
        LSET enabledLayersMask = m_board.GetEnabledLayers();
        int userLayers = 0;
        for( const auto& [name, layerId] : resolvedMapping )
        {
            if( layerId != PCB_LAYER_ID::UNDEFINED_LAYER )
                enabledLayersMask |= LSET{ layerId };

            if( IsUserLayer( layerId ) )
                userLayers++;

            wxLogTrace( traceAllegroBuilder, "Mapping Allegro layer '%s' to KiCad layer '%s' (%d)", name,
                        m_board.GetLayerName( layerId ), layerId );

            m_board.SetLayerName( layerId, name );
        }
        m_board.SetEnabledLayers( enabledLayersMask );
        wxLogTrace( traceAllegroBuilder, "After mapping, there are %d user layers", userLayers );
        m_board.GetDesignSettings().SetUserDefinedLayerCount( userLayers );
    }

    PCB_LAYER_ID GetLayer( const LAYER_INFO& aLayerInfo )
    {
        // We already mapped and created the layer
        if( m_customLayerToKiMap.count( aLayerInfo ) )
            return m_customLayerToKiMap.at( aLayerInfo );

        // Check for user-remapped static layers first
        if( m_staticLayerOverrides.count( aLayerInfo ) )
            return m_staticLayerOverrides.at( aLayerInfo );

        // Next, have a look and see if the class:subclass was recorded as a custom layer
        if( m_ClassCustomLayerLists.count( aLayerInfo.m_Class ) )
        {
            const std::vector<CUSTOM_LAYER>* cLayerList = m_ClassCustomLayerLists.at( aLayerInfo.m_Class );

            // If it is using the copper layer list and the subclass is within the
            // copper layer range, return the mapped copper layer. Non-ETCH classes
            // can share the same layer list pointer, but their subclass values may
            // exceed the copper layer count and must fall through to the custom
            // layer mapping below.
            const auto etchIt = m_ClassCustomLayerLists.find( LAYER_INFO::CLASS::ETCH );
            if( etchIt != m_ClassCustomLayerLists.end()
                && cLayerList == etchIt->second
                && aLayerInfo.m_Subclass < cLayerList->size() )
            {
                const PCB_LAYER_ID cuLayer = getNthCopperLayer( aLayerInfo.m_Subclass, cLayerList->size() );
                // Remember this mapping
                m_customLayerToKiMap[aLayerInfo] = cuLayer;
                return cuLayer;
            }

            if( aLayerInfo.m_Subclass < cLayerList->size() )
            {
                // This subclass maps to a custom layer in this class
                const CUSTOM_LAYER& cLayer = cLayerList->at( aLayerInfo.m_Subclass );
                return MapCustomLayer( aLayerInfo, cLayer.m_Name );
            }
        }

        // Now, there may be layers that map to custom layers in KiCad, but are fixed in Allegro
        // (perhaps, DFA_BOUND_TOP), which means we won't find them in the layer lists.
        // We add them if we encounter them, with the names defined.
        if( s_OptionalFixedMappings.count( aLayerInfo ) )
        {
            const wxString& layerName = s_OptionalFixedMappings.at( aLayerInfo );
            return MapCustomLayer( aLayerInfo, layerName );
        }

        // Finally, fallback to the static mapping for any layers we haven't got a custom map for
        // We do this last so that it can be overridden for example if we want to remap
        // OUTLINE and DESIGN_OUTLINE to different layers.
        if( s_LayerKiMap.count( aLayerInfo ) )
            return s_LayerKiMap.at( aLayerInfo );

        // Keep a record of what we failed to map
        if( m_unknownLayers.count( aLayerInfo ) == 0 )
        {
            wxLogTrace( traceAllegroBuilder, "Failed to map class:subclass to layer: %#04x:%#04x", aLayerInfo.m_Class,
                        aLayerInfo.m_Subclass );
            m_unknownLayers[aLayerInfo] = 1;
        }
        m_unknownLayers[aLayerInfo]++;

        // Dump everything else here
        return m_unmappedLayer;
    }

    /**
     * Return whether this layer ID is something we mapped to, or the catch-all unmapped layer.
     */
    bool IsLayerMapped( PCB_LAYER_ID aLayerId ) const
    {
        return aLayerId != m_unmappedLayer;
    }

    /**
     * Allegro puts more graphics than just the polygon on PBT/B, but we don't want to always make a static
     * mapping, because some things on PBT/B _do_ belong to the courtyard layer in KiCad (polygons).
     *
     * Use this function to create/choose a user layer instead.
     */
    PCB_LAYER_ID GetPlaceBounds( bool aTop )
    {
        const wxString name = aTop ? "PLACE_BOUND_TOP" : "PLACE_BOUND_BOTTOM";
        return mapCustomLayerByName( name );
    }

    /**
     * Resolve the subclass name for a given class:subclass pair using the
     * per-class custom layer list. Returns empty string if not found.
     */
    bool IsOutlineLayer( const LAYER_INFO& aLayerInfo ) const
    {
        if( aLayerInfo.m_Class != LAYER_INFO::CLASS::BOARD_GEOMETRY
            && aLayerInfo.m_Class != LAYER_INFO::CLASS::DRAWING_FORMAT )
        {
            return false;
        }

        return aLayerInfo.m_Subclass == LAYER_INFO::SUBCLASS::DFMT_OUTLINE
               || aLayerInfo.m_Subclass == LAYER_INFO::SUBCLASS::BGEOM_OUTLINE;
    }

    /**
     * Record a specific class:subclass layer as mapping to some KiCad user layer, with a given name
     *
     * Usually, you don't need this as they are registered as needed based on layers found in the board,
     * but sometimes you need to override the default mapping, say when you detect that the
     * "lumped" layers need to be split.
     */
    PCB_LAYER_ID MapCustomLayer( const LAYER_INFO& aLayerInfo, const wxString& aLayerName )
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
        m_MappedOptionalLayers[aLayerName] = lId;

        wxLogTrace( traceAllegroBuilder, "Adding mapping for %#04x:%#04x to %s", aLayerInfo.m_Class,
                    aLayerInfo.m_Subclass, aLayerName );
        return lId;
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

    /**
     * Create or find a mapped layer with a given name, but not specifically bound to a specific class:subclass.
     *
     * This is useful when some items on a class:subclass need to be placed on a KiCad layer other than the usual
     * mapping (non-polygon PLACE_BOUND_TOP items, for example)
     */
    PCB_LAYER_ID mapCustomLayerByName( const wxString& aLayerName )
    {
        // If it's been added already, use it
        if( m_MappedOptionalLayers.count( aLayerName ) )
        {
            return m_MappedOptionalLayers.at( aLayerName );
        }

        const PCB_LAYER_ID newLId = addUserLayer( aLayerName );
        m_MappedOptionalLayers[aLayerName] = newLId;
        return newLId;
    }

    PCB_LAYER_ID addUserLayer( const wxString& aName )
    {
        const PCB_LAYER_ID lId = getNthUserLayer( m_numUserLayersUsed++ );
        m_board.GetDesignSettings().SetUserDefinedLayerCount( m_board.GetDesignSettings().GetUserDefinedLayerCount() + 1 );
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
     * This doesn't cover all the fixed layers, just created custom ones,
     * including the copper layers.
     */
    std::unordered_map<LAYER_INFO, PCB_LAYER_ID> m_customLayerToKiMap;

    /**
     * This is a map of optional, Allegro layers that we have mapped to KiCad layers with given names.
     *
     * This is done by name, because multiple class:subclass pairs may share the same name.
     */
    std::unordered_map<wxString, PCB_LAYER_ID> m_MappedOptionalLayers;

    /**
     * Overrides for the static s_LayerKiMap entries, populated by the layer mapping handler.
     */
    std::unordered_map<LAYER_INFO, PCB_LAYER_ID> m_staticLayerOverrides;

    /**
     * Names used in the layer mapping dialog for custom (non-ETCH, non-static) layers.
     * Populated during FinalizeLayers(), consumed when applying the handler result.
     */
    std::unordered_map<LAYER_INFO, wxString> m_customLayerDialogNames;

    /**
     * A record of what we _failed_ to map.
     */
    std::unordered_map<LAYER_INFO, int> m_unknownLayers;

    int m_numUserLayersUsed = 0;

    // The layer to use for mapping failures;
    PCB_LAYER_ID m_unmappedLayer = Cmts_User;

    const LAYER_MAPPING_HANDLER& m_layerMappingHandler;

    const BRD_DB& m_brdDb;
    BOARD&        m_board;
};


BOARD_BUILDER::BOARD_BUILDER( const BRD_DB& aRawBoard, BOARD& aBoard, REPORTER& aReporter,
                              PROGRESS_REPORTER* aProgressReporter,
                              const LAYER_MAPPING_HANDLER& aLayerMappingHandler ) :
        m_brdDb( aRawBoard ), m_board( aBoard ), m_reporter( aReporter ), m_progressReporter( aProgressReporter ),
        m_layerMappingHandler( aLayerMappingHandler ),
        m_layerMapper( std::make_unique<LAYER_MAPPER>( m_brdDb, m_board, m_layerMappingHandler ) )
{
    // Internal coordinates are stored in <base> / <divisor> units.

    const std::map<BOARD_UNITS, int> c_baseScales = { { BOARD_UNITS::MILS, pcbIUScale.MilsToIU( 1 ) },
                                                      { BOARD_UNITS::INCHES, pcbIUScale.MilsToIU( 1000 ) },
                                                      { BOARD_UNITS::MILLIMETERS, pcbIUScale.mmToIU( 1 ) },
                                                      { BOARD_UNITS::CENTIMETERS, pcbIUScale.mmToIU( 10 ) },
                                                      { BOARD_UNITS::MICROMETERS, pcbIUScale.mmToIU( 0.001 ) } };

    if( m_brdDb.m_Header->m_UnitsDivisor == 0 )
        THROW_IO_ERROR( "Board units divisor is 0" );

    if( !c_baseScales.contains( m_brdDb.m_Header->m_BoardUnits ) )
        THROW_IO_ERROR( "Unknown board units" );

    double baseScale( c_baseScales.at( m_brdDb.m_Header->m_BoardUnits ) );

    m_scale = baseScale / m_brdDb.m_Header->m_UnitsDivisor;
}


BOARD_BUILDER::~BOARD_BUILDER()
{
}


static int clampForScale( double aValue )
{
    double result = std::round( aValue );

    if( result > std::numeric_limits<int>::max() )
        return std::numeric_limits<int>::max();

    if( result < std::numeric_limits<int>::min() )
        return std::numeric_limits<int>::min();

    return static_cast<int>( result );
}


VECTOR2I BOARD_BUILDER::scale( const VECTOR2I& aVector ) const
{
    return VECTOR2I{
        clampForScale( aVector.x * m_scale ),
        clampForScale( -aVector.y * m_scale ),
    };
}

int BOARD_BUILDER::scale( int aValue ) const
{
    return clampForScale( aValue * m_scale );
}


VECTOR2I BOARD_BUILDER::scaleSize( const VECTOR2I& aSize ) const
{
    return VECTOR2I{
        clampForScale( std::abs( aSize.x ) * m_scale ),
        clampForScale( std::abs( aSize.y ) * m_scale ),
    };
}


void BOARD_BUILDER::reportMissingBlock( uint32_t aKey, uint8_t aType ) const
{
    m_reporter.Report( wxString::Format( "Could not find expected block with key %#010x and type %#04x", aKey, aType ),
                       RPT_SEVERITY_WARNING );
}


void BOARD_BUILDER::reportUnexpectedBlockType( uint8_t aGot, uint8_t aExpected, uint32_t aKey, size_t aOffset,
                                               const wxString& aName ) const
{
    wxString name = aName.IsEmpty() ? wxString( "Object" ) : aName;
    wxString withKey = ( aKey == 0 ) ? wxString( "" ) : wxString::Format( ", with key %#010x ", aKey );
    wxString withOffset = ( aOffset == 0 ) ? wxString( "" ) : wxString::Format( ", at offset %#lx ", aOffset );

    wxString s = wxString::Format( "%s has unexpected type %#04x (expected %#04x)%s%s", name, aGot, aExpected, withKey,
                                   withOffset );

    m_reporter.Report( s, RPT_SEVERITY_WARNING );
}


wxString BOARD_BUILDER::get0x30StringValue( uint32_t a0x30Key ) const
{
    const BLK_0x30_STR_WRAPPER* blk0x30 = expectBlockByKey<BLK_0x30_STR_WRAPPER>( a0x30Key, 0x30 );

    if( blk0x30 == nullptr )
        THROW_IO_ERROR( "Failed to get 0x30 for string lookup" );

    const BLK_0x31_SGRAPHIC* blk0x31 = expectBlockByKey<BLK_0x31_SGRAPHIC>( blk0x30->m_StrGraphicPtr, 0x31 );

    if( blk0x31 == nullptr )
        THROW_IO_ERROR( "Failed to get 0x31 for string lookup" );

    return blk0x31->m_Value;
}


void BOARD_BUILDER::cacheFontDefs()
{
    LL_WALKER x36_walker{ m_brdDb.m_Header->m_LL_0x36.m_Head, m_brdDb.m_Header->m_LL_0x36.m_Tail, m_brdDb };

    bool encountered = false;

    for( const BLOCK_BASE* block : x36_walker )
    {
        if( block->GetBlockType() != 0x36 )
            continue;

        const BLK_0x36_DEF_TABLE& blk0x36 = static_cast<const BLOCK<BLK_0x36_DEF_TABLE>&>( *block ).GetData();

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
            const auto& fontDef = std::get<BLK_0x36_DEF_TABLE::FontDef_X08>( item );
            m_fontDefList.push_back( &fontDef );
        }

        encountered = true;
    }
}


void BOARD_BUILDER::createNets()
{
    wxLogTrace( traceAllegroBuilder, "Creating nets from Allegro data" );

    // Incrementing netcode. We could also choose to, say, use the 0x1B key if we wanted
    int netCode = 1;

    std::vector<BOARD_ITEM*> bulkAdded;

    LL_WALKER netWalker{ m_brdDb.m_Header->m_LL_0x1B_Nets, m_brdDb };

    for( const BLOCK_BASE* block : netWalker )
    {
        const uint8_t type = block->GetBlockType();

        if( type != BLOCK_TYPE::x1B_NET )
        {
            reportUnexpectedBlockType( type, BLOCK_TYPE::x1B_NET, 0, block->GetOffset(), "Net" );
            continue;
        }

        const auto& netBlk = static_cast<const BLOCK<BLK_0x1B_NET>&>( *block ).GetData();

        wxString netName = m_brdDb.GetString( netBlk.m_NetName );

        // Allegro allows unnamed nets. KiCad's NETINFO_LIST matches nets by name, and all
        // empty-named nets would collapse to the unconnected net (code 0). Generate a unique
        // name so each Allegro net gets its own KiCad net code.
        if( netName.IsEmpty() )
            netName = wxString::Format( wxS( "Net_%d" ), netCode );

        auto kiNetInfo = std::make_unique<NETINFO_ITEM>( &m_board, netName, netCode );
        netCode++;

        m_netCache[netBlk.m_Key] = kiNetInfo.get();
        bulkAdded.push_back( kiNetInfo.get() );
        m_board.Add( kiNetInfo.release(), ADD_MODE::BULK_APPEND );
    }

    m_board.FinalizeBulkAdd( bulkAdded );

    wxLogTrace( traceAllegroBuilder, "Added %zu nets", m_netCache.size() );
}


wxString BOARD_BUILDER::resolveConstraintSetNameFromField( uint32_t aFieldKey ) const
{
    const BLOCK_BASE* fieldBlock = m_brdDb.GetObjectByKey( aFieldKey );

    if( !fieldBlock || fieldBlock->GetBlockType() != 0x03 )
        return wxEmptyString;

    const BLK_0x03_FIELD& field = static_cast<const BLOCK<BLK_0x03_FIELD>&>( *fieldBlock ).GetData();
    const std::string* str = std::get_if<std::string>( &field.m_Substruct );

    if( !str )
        return wxEmptyString;

    // Extract name from schematic cross-reference format: @lib.xxx(view):\NAME\.
    // Find the last colon-backslash separator in the raw std::string and extract from there.
    size_t sep = str->find( ":\\" );

    if( sep == std::string::npos )
        return wxEmptyString;

    std::string extracted = str->substr( sep + 2 );

    if( !extracted.empty() && extracted.back() == '\\' )
        extracted.pop_back();

    return wxString( extracted );
}


void BOARD_BUILDER::applyConstraintSets()
{
    wxLogTrace( traceAllegroBuilder, "Importing physical constraint sets from 0x1D blocks" );

    BOARD_DESIGN_SETTINGS& bds = m_board.GetDesignSettings();
    std::shared_ptr<NET_SETTINGS> netSettings = bds.m_NetSettings;

    bool isV172Plus = ( m_brdDb.m_FmtVer >= FMT_VER::V_172 );

    struct CS_DEF
    {
        wxString name;
        int      lineWidth = 0;
        int      clearance = 0;
        int      diffPairGap = 0;
    };

    // Map from constraint set name to its definition
    std::map<wxString, CS_DEF> constraintSets;

    // Also map string table keys to set names for net lookup
    std::map<uint32_t, wxString> keyToSetName;

    int csIndex = 0;
    const LL_WALKER csWalker( m_brdDb.m_Header->m_LL_0x1D_0x1E_0x1F, m_brdDb );

    for( const BLOCK_BASE* block : csWalker )
    {
        if( block->GetBlockType() != 0x1D )
            continue;

        const BLK_0x1D_CONSTRAINT_SET& csBlock = static_cast<const BLOCK<BLK_0x1D_CONSTRAINT_SET>&>( *block ).GetData();

        wxString setName;
        const wxString* resolved = m_brdDb.ResolveString( csBlock.m_NameStrKey );

        if( resolved && !resolved->IsEmpty() )
        {
            setName = *resolved;
        }
        else if( csBlock.m_FieldPtr != 0 )
        {
            // Some boards store the name in a 0x03 FIELD block as a schematic cross-reference
            setName = resolveConstraintSetNameFromField( csBlock.m_FieldPtr );
        }

        if( setName.IsEmpty() )
            setName = wxString::Format( wxS( "CS_%d" ), csIndex );

        csIndex++;

        if( csBlock.m_DataB.empty() )
        {
            wxLogTrace( traceAllegroBuilder, "Constraint set '%s' has no DataB records, skipping", setName );
            continue;
        }

        // Parse first DataB record (first copper layer) as 14 x int32
        const auto& record = csBlock.m_DataB[0];
        int32_t fields[14];
        static_assert( sizeof( fields ) == std::tuple_size_v<std::decay_t<decltype( record )>> );
        memcpy( fields, record.data(), sizeof( fields ) );

        CS_DEF def;
        def.name = setName;

        if( isV172Plus )
        {
            def.lineWidth = scale( fields[1] );
            def.clearance = scale( fields[4] );
        }
        else
        {
            // Pre-V172: f[0] is preferred line width, f[1] is line spacing (used as clearance).
            // f[4] is sometimes also clearance when non-zero, but f[1] is the primary source.
            def.lineWidth = scale( fields[0] );
            def.clearance = scale( fields[1] );
        }

        def.diffPairGap = scale( fields[7] );

        constraintSets[setName] = def;
        keyToSetName[csBlock.m_NameStrKey] = setName;

        wxLogTrace( traceAllegroBuilder,
                    "Constraint set '%s': line_width=%d nm, clearance=%d nm, dp_gap=%d nm",
                    setName, def.lineWidth, def.clearance, def.diffPairGap );
    }

    if( constraintSets.empty() )
    {
        wxLogTrace( traceAllegroBuilder, "No physical constraint sets found" );
        return;
    }

    // Create a netclass for each constraint set that has nonzero values
    for( const auto& [name, def] : constraintSets )
    {
        wxString ncName = name;

        if( ncName.CmpNoCase( NETCLASS::Default ) == 0 )
            ncName = wxS( "Allegro_Default" );

        if( netSettings->HasNetclass( ncName ) )
            continue;

        auto nc = std::make_shared<NETCLASS>( ncName );

        if( def.lineWidth > 0 )
            nc->SetTrackWidth( def.lineWidth );

        if( def.clearance > 0 )
            nc->SetClearance( def.clearance );

        if( def.diffPairGap > 0 )
        {
            nc->SetDiffPairGap( def.diffPairGap );

            // Diff pair width is the same as track width for the pair's netclass
            if( def.lineWidth > 0 )
                nc->SetDiffPairWidth( def.lineWidth );
        }

        netSettings->SetNetclass( ncName, nc );

        wxLogTrace( traceAllegroBuilder, "Created netclass '%s' from constraint set '%s'", ncName, name );
    }

    // Walk all NETs and assign them to constraint set netclasses via field 0x1a0
    wxString defaultSetName;

    for( const auto& [name, def] : constraintSets )
    {
        if( name.CmpNoCase( wxS( "DEFAULT" ) ) == 0 )
        {
            defaultSetName = name;
            break;
        }
    }

    m_brdDb.VisitNets( [&]( const VIEW_OBJS& aView )
    {
        if( !aView.m_Net )
            return;

        const NET& net = *aView.m_Net;

        // Field 0x1a0 references the constraint set. It can be an integer (string table key
        // that matches 0x1D.m_NameStrKey) or a direct string (the constraint set name).
        auto csField = net.m_Fields.GetOptField( FIELD_KEYS::PHYS_CONSTRAINT_SET );

        wxString assignedSetName;

        if( csField.has_value() )
        {
            if( auto* intVal = std::get_if<uint32_t>( &csField.value() ) )
            {
                auto it = keyToSetName.find( *intVal );

                if( it != keyToSetName.end() )
                    assignedSetName = it->second;
            }
            else if( auto* strVal = std::get_if<wxString>( &csField.value() ) )
            {
                if( constraintSets.count( *strVal ) )
                    assignedSetName = *strVal;
            }
        }

        // Nets without field 0x1a0 use the DEFAULT constraint set
        if( assignedSetName.IsEmpty() && !defaultSetName.IsEmpty() )
            assignedSetName = defaultSetName;

        if( assignedSetName.IsEmpty() )
            return;

        wxString ncName = assignedSetName;

        if( ncName.CmpNoCase( NETCLASS::Default ) == 0 )
            ncName = wxS( "Allegro_Default" );

        if( !netSettings->HasNetclass( ncName ) )
            return;

        auto netIt = m_netCache.find( net.GetKey() );

        if( netIt == m_netCache.end() )
            return;

        NETINFO_ITEM* kiNet = netIt->second;
        netSettings->SetNetclassPatternAssignment( kiNet->GetNetname(), ncName );
        kiNet->SetNetClass( netSettings->GetNetClassByName( ncName ) );
    } );

    wxLogTrace( traceAllegroBuilder, "Applied %zu physical constraint sets", constraintSets.size() );
}


void BOARD_BUILDER::applyNetConstraints()
{
    wxLogTrace( traceAllegroBuilder, "Applying per-net trace width constraints" );

    BOARD_DESIGN_SETTINGS& bds = m_board.GetDesignSettings();
    std::shared_ptr<NET_SETTINGS> netSettings = bds.m_NetSettings;

    // Group nets by their minimum trace width to create netclasses.
    // Allegro stores per-net min/max trace width in FIELD blocks attached to each NET.
    std::map<int, std::vector<uint32_t>> widthToNetKeys;

    m_brdDb.VisitNets( [&]( const VIEW_OBJS& aView )
    {
        if( !aView.m_Net )
            return;

        std::optional<int> minWidth = aView.m_Net->GetNetMinLineWidth();

        if( !minWidth.has_value() || minWidth.value() <= 0 )
            return;

        int widthNm = scale( minWidth.value() );
        widthToNetKeys[widthNm].push_back( aView.m_Net->GetKey() );
    } );

    if( widthToNetKeys.empty() )
    {
        wxLogTrace( traceAllegroBuilder, "No per-net trace width constraints found" );
        return;
    }

    for( const auto& [widthNm, netKeys] : widthToNetKeys )
    {
        int widthMils = ( widthNm + 12700 ) / 25400;
        wxString ncName = wxString::Format( wxS( "W%dmil" ), widthMils );

        if( netSettings->HasNetclass( ncName ) )
            continue;

        auto nc = std::make_shared<NETCLASS>( ncName );
        nc->SetTrackWidth( widthNm );
        netSettings->SetNetclass( ncName, nc );

        for( uint32_t netKey : netKeys )
        {
            auto it = m_netCache.find( netKey );

            if( it == m_netCache.end() )
                continue;

            NETINFO_ITEM* kiNet = it->second;
            netSettings->SetNetclassPatternAssignment( kiNet->GetNetname(), ncName );
            kiNet->SetNetClass( nc );
        }

        wxLogTrace( traceAllegroBuilder, "Created netclass '%s' (track width %d nm) with %zu nets",
                    ncName, widthNm, netKeys.size() );
    }

    wxLogTrace( traceAllegroBuilder, "Applied trace width constraints from %zu unique width groups",
                widthToNetKeys.size() );
}


wxString BOARD_BUILDER::resolveMatchGroupName( const BLK_0x1B_NET& aNet ) const
{
    if( aNet.m_MatchGroupPtr == 0 )
        return wxEmptyString;

    const BLOCK_BASE* block = m_brdDb.GetObjectByKey( aNet.m_MatchGroupPtr );

    if( !block )
        return wxEmptyString;

    uint32_t tableKey = 0;

    if( block->GetBlockType() == 0x26 )
    {
        // V172+ path: NET -> 0x26 -> m_GroupPtr -> 0x2C TABLE
        const auto& x26 = static_cast<const BLOCK<BLK_0x26_MATCH_GROUP>&>( *block ).GetData();
        tableKey = x26.m_GroupPtr;

        // Some boards have chained 0x26 blocks (m_GroupPtr -> another 0x26 -> 0x2C)
        if( tableKey != 0 )
        {
            const BLOCK_BASE* next = m_brdDb.GetObjectByKey( tableKey );

            if( next && next->GetBlockType() == 0x26 )
            {
                const auto& x26b = static_cast<const BLOCK<BLK_0x26_MATCH_GROUP>&>( *next ).GetData();
                tableKey = x26b.m_GroupPtr;
            }
        }
    }
    else if( block->GetBlockType() == 0x2C )
    {
        // Pre-V172 path: NET -> 0x2C TABLE directly
        tableKey = aNet.m_MatchGroupPtr;
    }
    else
    {
        return wxEmptyString;
    }

    if( tableKey == 0 )
        return wxEmptyString;

    // Verify the target is actually a 0x2C TABLE before calling expectBlockByKey to
    // avoid noisy warnings on boards with unexpected pointer chain configurations.
    const BLOCK_BASE* tableBlock = m_brdDb.GetObjectByKey( tableKey );

    if( !tableBlock || tableBlock->GetBlockType() != 0x2C )
        return wxEmptyString;

    const BLK_0x2C_TABLE* tbl = expectBlockByKey<BLK_0x2C_TABLE>( tableKey, 0x2C );

    if( !tbl || tbl->m_StringPtr == 0 )
        return wxEmptyString;

    const wxString& name = m_brdDb.GetString( tbl->m_StringPtr );

    wxLogTrace( traceAllegroBuilder, "Resolving match group name for NET '%s': found table at key %#010x, subtype %#x, name '%s'",
                m_brdDb.GetString( aNet.m_NetName ), tableKey, tbl->m_SubType, name );

    return name;
}


void BOARD_BUILDER::applyMatchGroups()
{
    wxLogTrace( traceAllegroBuilder, "Applying match group / differential pair assignments" );

    BOARD_DESIGN_SETTINGS& bds = m_board.GetDesignSettings();
    std::shared_ptr<NET_SETTINGS> netSettings = bds.m_NetSettings;

    // Group NET keys by their match group name
    std::map<wxString, std::vector<uint32_t>> groupToNetKeys;

    LL_WALKER netWalker{ m_brdDb.m_Header->m_LL_0x1B_Nets, m_brdDb };

    for( const BLOCK_BASE* block : netWalker )
    {
        if( block->GetBlockType() != BLOCK_TYPE::x1B_NET )
            continue;

        const auto& netBlk = static_cast<const BLOCK<BLK_0x1B_NET>&>( *block ).GetData();
        wxString groupName = resolveMatchGroupName( netBlk );

        if( groupName.empty() )
            continue;

        groupToNetKeys[groupName].push_back( netBlk.m_Key );
    }

    if( groupToNetKeys.empty() )
    {
        wxLogTrace( traceAllegroBuilder, "No match groups found" );
        return;
    }

    int dpCount = 0;
    int mgCount = 0;

    for( const auto& [groupName, netKeys] : groupToNetKeys )
    {
        // A diff pair has exactly 2 nets. We don't check P/N naming because Allegro doesn't
        // require any naming convention for paired nets.
        bool isDiffPair = ( netKeys.size() == 2 );
        wxString ncPrefix = isDiffPair ? wxS( "DP_" ) : wxS( "MG_" );
        wxString ncName = ncPrefix + groupName;

        if( netSettings->HasNetclass( ncName ) )
            continue;

        auto nc = std::make_shared<NETCLASS>( ncName );

        // Inherit constraint set values from the first net's current netclass so that
        // clearance and track width from the underlying constraint set are not lost.
        for( uint32_t netKey : netKeys )
        {
            auto it = m_netCache.find( netKey );

            if( it == m_netCache.end() )
                continue;

            NETCLASS* existing = it->second->GetNetClass();

            if( existing && existing->GetName() != NETCLASS::Default )
            {
                if( existing->HasClearance() )
                    nc->SetClearance( existing->GetClearance() );

                if( existing->HasTrackWidth() )
                    nc->SetTrackWidth( existing->GetTrackWidth() );

                if( existing->HasDiffPairGap() )
                    nc->SetDiffPairGap( existing->GetDiffPairGap() );

                if( existing->HasDiffPairWidth() )
                    nc->SetDiffPairWidth( existing->GetDiffPairWidth() );

                break;
            }
        }

        netSettings->SetNetclass( ncName, nc );

        for( uint32_t netKey : netKeys )
        {
            auto it = m_netCache.find( netKey );

            if( it == m_netCache.end() )
                continue;

            NETINFO_ITEM* kiNet = it->second;
            netSettings->SetNetclassPatternAssignment( kiNet->GetNetname(), ncName );
            kiNet->SetNetClass( nc );
        }

        if( isDiffPair )
            dpCount++;
        else
            mgCount++;

        wxLogTrace( traceAllegroBuilder, "%s group '%s' -> netclass '%s' with %zu nets",
                    isDiffPair ? wxS( "Diff pair" ) : wxS( "Match" ),
                    groupName, ncName, netKeys.size() );
    }

    wxLogTrace( traceAllegroBuilder,
                "Applied match groups: %d diff pairs, %d match groups (%zu total groups)",
                dpCount, mgCount, groupToNetKeys.size() );
}


/**
 * Look through some lists for a list of layers used.
 *
 * This isn't yet exhaustive, not sure if it needs to be. We could scan every single
 * block if we wanted, but that would be a lot of blocks without layers. So walking
 * lists seems more efficient.
 *
 * The primary goal is to look for colliding layers like OUTLINE/DESIGN_OUTLINE
 * so that we can remap one of them to something else.
 *
 * We could also use this to find all the used layers and present them in the
 * mapping dialog rather than auto-creating them layer during buildout.
 */
static std::unordered_set<LAYER_INFO> ScanForLayers( const BRD_DB& aDb )
{
    std::unordered_set<LAYER_INFO> layersFound;

    const auto& addLayer = [&]( std::optional<LAYER_INFO>& info )
    {
        if( info.has_value() )
        {
            layersFound.insert( std::move( info.value() ) );
        }
    };

    const auto& simpleWalker = [&]( const FILE_HEADER::LINKED_LIST& aLL )
    {
        LL_WALKER walker{ aLL, aDb };
        for( const BLOCK_BASE* block : walker )
        {
            std::optional<LAYER_INFO> info = tryLayerFromBlock( *block );
            addLayer( info );
        }
    };

    simpleWalker( aDb.m_Header->m_LL_Shapes );
    simpleWalker( aDb.m_Header->m_LL_0x24_0x28 );
    simpleWalker( aDb.m_Header->m_LL_0x14 );

    return layersFound;
}


void BOARD_BUILDER::setupLayers()
{
    wxLogTrace( traceAllegroBuilder, "Setting up layer mapping from Allegro to KiCad" );

    const auto& layerMap = m_brdDb.m_Header->m_LayerMap;

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

    std::unordered_set<LAYER_INFO> layersFound = ScanForLayers( m_brdDb );

    wxLogTrace( traceAllegroBuilder, "Scanned %zu layers", layersFound.size() );
    for( const LAYER_INFO& info : layersFound )
    {
        wxLogTrace( traceAllegroBuilder, " - %#02x:%#02x (%s)", info.m_Class, info.m_Subclass,
                    layerInfoDisplayName( info ) );
    }

    // The outline is sometimes on OUTLINE and sometimes on DESIGN_OUTLINE, and sometimes
    // on both. In the first two cases, whichever it is goes to Edge.Cuts, but in the both case,
    // we send one to a User layer
    const LAYER_INFO outlineInfo{ LAYER_INFO::CLASS::BOARD_GEOMETRY, LAYER_INFO::SUBCLASS::BGEOM_OUTLINE };
    const LAYER_INFO designOutlineInfo{ LAYER_INFO::CLASS::BOARD_GEOMETRY, LAYER_INFO::SUBCLASS::BGEOM_DESIGN_OUTLINE };

    if( layersFound.count( outlineInfo ) && layersFound.count( designOutlineInfo ) )
    {
        // Both layers found, remap DESIGN_OUTLINE to a user layer
        wxLogTrace( traceAllegroBuilder,
                    "Both OUTLINE and DESIGN_OUTLINE layers found, remapping DESIGN_OUTLINE to a user layer" );
        m_layerMapper->MapCustomLayer( designOutlineInfo, layerInfoDisplayName( designOutlineInfo ) );
    }

    m_layerMapper->FinalizeLayers();
}


const BLK_0x36_DEF_TABLE::FontDef_X08* BOARD_BUILDER::getFontDef( unsigned aIndex ) const
{
    if( aIndex == 0 || aIndex > m_fontDefList.size() )
    {
        m_reporter.Report(
                wxString::Format( "Font def index %u requested, have %zu entries", aIndex, m_fontDefList.size() ),
                RPT_SEVERITY_WARNING );
        return nullptr;
    }

    // The index appears to be 1-indexed (maybe 0 means something special?)
    aIndex -= 1;

    return m_fontDefList[aIndex];
}


std::unique_ptr<PCB_SHAPE> BOARD_BUILDER::buildLineSegment( const BLK_0x15_16_17_SEGMENT& aSegment,
                                                            const LAYER_INFO& aLayerInfo, PCB_LAYER_ID aLayer,
                                                            BOARD_ITEM_CONTAINER& aParent )
{
    VECTOR2I  start = scale( { aSegment.m_StartX, aSegment.m_StartY } );
    VECTOR2I  end = scale( { aSegment.m_EndX, aSegment.m_EndY } );
    const int width = scale( aSegment.m_Width );

    if( !m_layerMapper->IsLayerMapped( aLayer ) )
    {
        wxLogTrace( traceAllegroBuilder, "Unmapped Seg: %#04x %#04x %s, %s", aLayerInfo.m_Class, aLayerInfo.m_Subclass,
                    start.Format(), end.Format() );
    }

    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::SEGMENT );
    shape->SetLayer( aLayer );
    shape->SetStart( start );
    shape->SetEnd( end );

    {
        int adjustedWidth = width;

        if( adjustedWidth <= 0 )
            adjustedWidth = m_board.GetDesignSettings().GetLineThickness( aLayer );

        shape->SetWidth( adjustedWidth );
    }

    return shape;
}


std::unique_ptr<PCB_SHAPE> BOARD_BUILDER::buildArc( const BLK_0x01_ARC& aArc, const LAYER_INFO& aLayerInfo,
                                                    PCB_LAYER_ID aLayer, BOARD_ITEM_CONTAINER& aParent )
{
    VECTOR2I start{ aArc.m_StartX, aArc.m_StartY };
    VECTOR2I end{ aArc.m_EndX, aArc.m_EndY };

    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::ARC );

    shape->SetLayer( aLayer );

    if( !m_layerMapper->IsLayerMapped( aLayer ) )
    {
        wxLogTrace( traceAllegroBuilder, "Unmapped Arc: %#04x %#04x %s, %s", aLayerInfo.m_Class, aLayerInfo.m_Subclass,
                    start.Format(), end.Format() );
    }

    start = scale( start );
    end = scale( end );

    VECTOR2I c = scale( KiROUND( VECTOR2D{ aArc.m_CenterX, aArc.m_CenterY } ) );

    int radius = scale( KiROUND( aArc.m_Radius ) );

    bool clockwise = ( aArc.m_SubType & 0x40 ) != 0;

    {
        int arcWidth = scale( aArc.m_Width );

        if( arcWidth <= 0 )
            arcWidth = m_board.GetDesignSettings().GetLineThickness( aLayer );

        shape->SetWidth( arcWidth );
    }

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

    return shape;
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

    const BLK_0x36_DEF_TABLE::FontDef_X08* fontDef = getFontDef( props->m_Key );

    if( !fontDef )
        return nullptr;

    text->SetText( strGraphic->m_Value );
    text->SetTextWidth( scale( fontDef->m_CharWidth ) );
    text->SetTextHeight( scale( fontDef->m_CharHeight ) );
    text->SetTextThickness( std::max( 1, scale( fontDef->m_CharHeight ) / 8 ) );

    const EDA_ANGLE textAngle{ static_cast<double>( aStrWrapper.m_Rotation ) / 1000.0, DEGREES_T };
    text->SetTextAngle( textAngle );

    if( props->m_Reversal == BLK_0x30_STR_WRAPPER::TEXT_REVERSAL::REVERSED )
        text->SetMirrored( true );

    switch( props->m_Alignment )
    {
    case BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::LEFT:
        text->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        break;
    case BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::CENTER:
        text->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
        break;
    case BLK_0x30_STR_WRAPPER::TEXT_ALIGNMENT::RIGHT:
        text->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        break;
    default:
        break;
    }

    return text;
}


std::vector<std::unique_ptr<BOARD_ITEM>> BOARD_BUILDER::buildDrillMarker( const BLK_0x0C_PIN_DEF& aPinDef,
                                                                          BOARD_ITEM_CONTAINER&   aParent )
{
    using MS = BLK_0x0C_PIN_DEF::MARKER_SHAPE;
    std::vector<std::unique_ptr<PCB_SHAPE>> shapes;

    const uint32_t markerShape = aPinDef.GetShape();

    PCB_LAYER_ID   layer = getLayer( aPinDef.m_Layer );
    const VECTOR2I center = scale( VECTOR2I{ aPinDef.m_Coords[0], aPinDef.m_Coords[1] } );
    const VECTOR2I size = scaleSize( VECTOR2I{ aPinDef.m_Size[0], aPinDef.m_Size[1] } );

    const auto addLine = [&]( const SEG& aSeg )
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::SEGMENT );
        shape->SetStart( aSeg.A );
        shape->SetEnd( aSeg.B );
        shapes.push_back( std::move( shape ) );
    };

    const auto addPolyPts = [&]( const std::vector<VECTOR2I>& aPts )
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::POLY );
        shape->SetPolyPoints( aPts );
        shapes.push_back( std::move( shape ) );
    };

    switch( markerShape )
    {
    case MS::CIRCLE:
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::CIRCLE );
        shape->SetCenter( center );
        shape->SetRadius( size.x / 2 );
        shapes.push_back( std::move( shape ) );
        break;
    }
    case MS::SQUARE:
    case MS::RECTANGLE:
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::RECTANGLE );
        shape->SetStart( center - size / 2 );
        shape->SetEnd( center + size / 2 );
        shapes.push_back( std::move( shape ) );
        break;
    }
    case MS::CROSS:
    {
        std::unique_ptr<PCB_SHAPE> shape;

        std::vector<SEG> segs = KIGEOM::MakeCrossSegments( center, size, ANGLE_0 );

        for( const SEG& seg : segs )
        {
            addLine( seg );
        }
        break;
    }
    case MS::OBLONG_X:
    case MS::OBLONG_Y:
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent, SHAPE_T::RECTANGLE );
        shape->SetStart( center - size / 2 );
        shape->SetEnd( center + size / 2 );

        int minSize = std::min( size.x, size.y );
        shape->SetCornerRadius( minSize / 2 );
        shapes.push_back( std::move( shape ) );
        break;
    }
    case MS::TRIANGLE:
    {
        // This triangle is point-up
        // Size follows fabmaster - the circumscribed circle of the triangle
        std::vector<VECTOR2I> pts = KIGEOM::MakeRegularPolygonPoints( center, 3, size.x / 2, true, ANGLE_90 );
        addPolyPts( pts );
        break;
    }
    case MS::DIAMOND:
    {
        std::vector<VECTOR2I> pts = KIGEOM::MakeRegularPolygonPoints( center, 4, size.x / 2, true, ANGLE_90 );
        addPolyPts( pts );
        break;
    }
    case MS::PENTAGON:
    {
        // Not 100% sure which way this should point
        std::vector<VECTOR2I> pts = KIGEOM::MakeRegularPolygonPoints( center, 5, size.x / 2, true, ANGLE_90 );
        addPolyPts( pts );
        break;
    }
    case MS::HEXAGON_X:
    case MS::HEXAGON_Y:
    {
        EDA_ANGLE             startAngle = ( markerShape == MS::HEXAGON_X ) ? ANGLE_0 : ANGLE_90;
        std::vector<VECTOR2I> pts = KIGEOM::MakeRegularPolygonPoints( center, 6, size.x / 2, true, startAngle );
        addPolyPts( pts );
        break;
    }
    case MS::OCTAGON:
    {
        EDA_ANGLE startAngle = FULL_CIRCLE / 16; // Start at 22.5 degrees to align flat sides with axes
        // Octagons are measured across flats
        std::vector<VECTOR2I> pts = KIGEOM::MakeRegularPolygonPoints( center, 8, size.x / 2, false, startAngle );
        addPolyPts( pts );
        break;
    }
    default:
    {
        wxLogTrace( traceAllegroBuilder, "Unsupported drill marker shape type %#04x for pin definition with key %#010x",
                    markerShape, aPinDef.m_Key );
        break;
    }
    }

    std::vector<std::unique_ptr<BOARD_ITEM>> items;
    for( std::unique_ptr<PCB_SHAPE>& shape : shapes )
    {
        shape->SetLayer( layer );
        shape->SetWidth( 0 );

        items.push_back( std::move( shape ) );
    }

    return items;
}


std::vector<std::unique_ptr<BOARD_ITEM>> BOARD_BUILDER::buildGraphicItems( const BLOCK_BASE&     aBlock,
                                                                           BOARD_ITEM_CONTAINER& aParent )
{
    std::vector<std::unique_ptr<BOARD_ITEM>> newItems;

    switch( aBlock.GetBlockType() )
    {
    case 0x0c:
    {
        const auto& pinDef = static_cast<const BLOCK<BLK_0x0C_PIN_DEF>&>( aBlock ).GetData();
        newItems = buildDrillMarker( pinDef, aParent );
        break;
    }
    case 0x0e:
    {
        const auto&                rect = static_cast<const BLOCK<BLK_0x0E_RECT>&>( aBlock ).GetData();
        std::unique_ptr<PCB_SHAPE> shape = buildRect( rect, aParent );
        if( shape )
            newItems.push_back( std::move( shape ) );
        break;
    }
    case 0x14:
    {
        const auto& graphicContainer = static_cast<const BLOCK<BLK_0x14_GRAPHIC>&>( aBlock ).GetData();
        std::vector<std::unique_ptr<PCB_SHAPE>> shapes = buildShapes( graphicContainer, aParent );
        for( std::unique_ptr<PCB_SHAPE>& shape : shapes )
            newItems.push_back( std::move( shape ) );
        break;
    }
    case 0x24:
    {
        const auto&                rect = static_cast<const BLOCK<BLK_0x24_RECT>&>( aBlock ).GetData();
        std::unique_ptr<PCB_SHAPE> shape = buildRect( rect, aParent );
        if( shape )
            newItems.push_back( std::move( shape ) );
        break;
    }
    case 0x28:
    {
        const auto&                shapeData = static_cast<const BLOCK<BLK_0x28_SHAPE>&>( aBlock ).GetData();
        std::unique_ptr<PCB_SHAPE> shape = buildPolygon( shapeData, aParent );
        if( shape )
            newItems.push_back( std::move( shape ) );
        break;
    }
    case 0x30:
    {
        const auto& strWrapper = static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( aBlock ).GetData();

        std::unique_ptr<BOARD_ITEM> newItem = buildPcbText( strWrapper, aParent );
        if( newItem )
            newItems.push_back( std::move( newItem ) );
        break;
    }
    default:
    {
        wxLogTrace( traceAllegroBuilder, "    Unhandled block type for buildItems: %#04x", aBlock.GetBlockType() );
        break;
    }
    }

    return newItems;
};


PCB_LAYER_ID BOARD_BUILDER::getLayer( const LAYER_INFO& aLayerInfo ) const
{
    return m_layerMapper->GetLayer( aLayerInfo );
}


std::vector<std::unique_ptr<PCB_SHAPE>> BOARD_BUILDER::buildShapes( const BLK_0x14_GRAPHIC&       aGraphic,
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

    const LL_WALKER segWalker{ aGraphic.m_SegmentPtr, aGraphic.m_Key, m_brdDb };

    for( const BLOCK_BASE* segBlock : segWalker )
    {
        std::unique_ptr<PCB_SHAPE> shape;

        switch( segBlock->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *segBlock ).GetData();
            shape = buildArc( arc, aGraphic.m_Layer, layer, aParent );
            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            const auto& seg = static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *segBlock ).GetData();
            shape = buildLineSegment( seg, aGraphic.m_Layer, layer, aParent );
            break;
        }
        default:
        {
            wxLogTrace( traceAllegroBuilder, "    Unhandled block type in BLK_0x14_GRAPHIC: %#04x",
                        segBlock->GetBlockType() );
            break;
        }
        }

        if( shape )
            shapes.push_back( std::move( shape ) );
    }

    return shapes;
}


std::unique_ptr<PCB_SHAPE> BOARD_BUILDER::buildRect( const BLK_0x24_RECT& aRect, BOARD_ITEM_CONTAINER& aParent )
{
    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent );

    PCB_LAYER_ID layer = getLayer( aRect.m_Layer );
    shape->SetLayer( layer );

    shape->SetShape( SHAPE_T::RECTANGLE );

    const VECTOR2I cornerA = scale( VECTOR2I{ aRect.m_Coords[0], aRect.m_Coords[1] } );
    const VECTOR2I cornerB = scale( VECTOR2I{ aRect.m_Coords[2], aRect.m_Coords[3] } );

    shape->SetStart( cornerA );
    shape->SetEnd( cornerB );

    const EDA_ANGLE angle{ static_cast<double>( aRect.m_Rotation ) / 1000.0, DEGREES_T };
    shape->Rotate( cornerA, angle );

    const int lineWidth = 0;
    shape->SetWidth( lineWidth );

    if( aRect.m_Coords[0] == 1334202 )
    {
        wxLogTrace( traceAllegroBuilder, "Debug rect with coords [%d, %d, %d, %d], rotation %u, layer %#02x:%#02x",
                    aRect.m_Coords[0], aRect.m_Coords[1], aRect.m_Coords[2], aRect.m_Coords[3],
                    aRect.m_Rotation, aRect.m_Layer.m_Class, aRect.m_Layer.m_Subclass );
    }

    return shape;
}


std::unique_ptr<PCB_SHAPE> BOARD_BUILDER::buildRect( const BLK_0x0E_RECT& aRect, BOARD_ITEM_CONTAINER& aParent )
{
    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent );

    PCB_LAYER_ID layer = getLayer( aRect.m_Layer );
    shape->SetLayer( layer );

    shape->SetShape( SHAPE_T::RECTANGLE );

    const VECTOR2I cornerA = scale( VECTOR2I{ aRect.m_Coords[0], aRect.m_Coords[1] } );
    const VECTOR2I cornerB = scale( VECTOR2I{ aRect.m_Coords[2], aRect.m_Coords[3] } );

    shape->SetStart( cornerA );
    shape->SetEnd( cornerB );

    const EDA_ANGLE angle{ static_cast<double>( aRect.m_Rotation ) / 1000.0, DEGREES_T };
    shape->Rotate( cornerA, angle );

    const int lineWidth = 0;
    shape->SetWidth( lineWidth );

    return shape;
}


std::unique_ptr<PCB_SHAPE> BOARD_BUILDER::buildPolygon( const BLK_0x28_SHAPE& aPolygon, BOARD_ITEM_CONTAINER& aParent )
{
    std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &aParent );

    PCB_LAYER_ID layer = getLayer( aPolygon.m_Layer );
    shape->SetLayer( layer );

    shape->SetShape( SHAPE_T::POLY );

    SHAPE_LINE_CHAIN chain = buildOutline( aPolygon );

    if( chain.PointCount() < 3 )
    {
        wxLogTrace( traceAllegroBuilder, "Polygon (0x28) with key %#010x has fewer than 3 points, skipping",
                    aPolygon.m_Key );
        return nullptr;
    }

    chain.SetClosed( true );
    shape->SetPolyShape( chain );

    const int lineWidth = 0;
    shape->SetWidth( lineWidth );

    return shape;
}


std::vector<std::unique_ptr<PCB_SHAPE>> BOARD_BUILDER::buildPolygonShapes( const BLK_0x28_SHAPE& aShapeData,
                                                                           BOARD_ITEM_CONTAINER& aParent )
{
    std::vector<std::unique_ptr<PCB_SHAPE>> shapes;

    PCB_LAYER_ID layer = getLayer( aShapeData.m_Layer );

    // Walk the segments in this shape and create PCB_SHAPE objects on Edge_Cuts
    const LL_WALKER segWalker{ aShapeData.m_FirstSegmentPtr, aShapeData.m_Key, m_brdDb };

    for( const BLOCK_BASE* segBlock : segWalker )
    {
        std::unique_ptr<PCB_SHAPE> shape = std::make_unique<PCB_SHAPE>( &m_board );
        shape->SetLayer( layer );
        shape->SetWidth( m_board.GetDesignSettings().GetLineThickness( layer ) );

        switch( segBlock->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *segBlock ).GetData();

            VECTOR2I start = scale( { arc.m_StartX, arc.m_StartY } );
            VECTOR2I end = scale( { arc.m_EndX, arc.m_EndY } );
            VECTOR2I c = scale( KiROUND( VECTOR2D{ arc.m_CenterX, arc.m_CenterY } ) );

            int radius = scale( arc.m_Radius );
            if( start == end )
            {
                shape->SetShape( SHAPE_T::CIRCLE );
                shape->SetCenter( c );
                shape->SetRadius( radius );
            }
            else
            {
                shape->SetShape( SHAPE_T::ARC );

                bool clockwise = ( arc.m_SubType & 0x40 ) != 0;

                EDA_ANGLE startangle( start - c );
                EDA_ANGLE endangle( end - c );

                startangle.Normalize();
                endangle.Normalize();

                EDA_ANGLE angle = endangle - startangle;

                if( clockwise && angle < ANGLE_0 )
                    angle += ANGLE_360;

                if( !clockwise && angle > ANGLE_0 )
                    angle -= ANGLE_360;

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
            const auto& seg = static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *segBlock ).GetData();
            VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );
            VECTOR2I    end = scale( { seg.m_EndX, seg.m_EndY } );

            shape->SetShape( SHAPE_T::SEGMENT );
            shape->SetStart( start );
            shape->SetEnd( end );
            shape->SetWidth( m_board.GetDesignSettings().GetLineThickness( layer ) );
            break;
        }
        default:
            wxLogTrace( traceAllegroBuilder, "  Unhandled segment type in outline: %#04x", segBlock->GetBlockType() );
            continue;
        }

        shapes.push_back( std::move( shape ) );
    }

    return shapes;
}


const BLK_0x07_COMPONENT_INST* BOARD_BUILDER::getFpInstRef( const BLK_0x2D_FOOTPRINT_INST& aFpInstance ) const
{
    uint32_t refKey = 0x00;

    if( aFpInstance.m_InstRef.has_value() )
        refKey = aFpInstance.m_InstRef.value();

    if( !refKey && aFpInstance.m_InstRef16x.has_value() )
        refKey = aFpInstance.m_InstRef16x.value();

    // This can happen, for example for dimension "symbols".
    if( refKey == 0 )
        return nullptr;

    const BLK_0x07_COMPONENT_INST* blk07 = expectBlockByKey<BLK_0x07_COMPONENT_INST>( refKey, 0x07 );
    return blk07;
}


std::vector<std::unique_ptr<BOARD_ITEM>> BOARD_BUILDER::buildPadItems( const BLK_0x1C_PADSTACK& aPadstack,
                                                                       FOOTPRINT& aFp, const wxString& aPadName, int aNetcode )
{
    // Not all Allegro PADSTACKS can be represented by a single KiCad pad. For example, the
    // paste and mask layers can have completely independent shapes in Allegro, but in KiCad that
    // would require a separate aperture pad.
    // Also if there are multiple drills, we will need to make a pad for each
    std::vector<std::unique_ptr<BOARD_ITEM>> padItems;

    std::vector<std::unique_ptr<PADSTACK::COPPER_LAYER_PROPS>> copperLayers( aPadstack.m_LayerCount );

    // Thermal relief gap from antipad/pad size difference on the first layer that has both.
    std::optional<int> thermalGap;

    const wxString& padStackName = m_brdDb.GetString( aPadstack.m_PadStr );

    wxLogTrace( traceAllegroBuilder, "Building pad '%s' with %u layers", padStackName, aPadstack.m_LayerCount );

    // First, gather all the copper layers into a set of shape props, which we can then use to decide on the padstack mode
    for( size_t i = 0; i < aPadstack.m_LayerCount; ++i )
    {
        const size_t layerBaseIndex = aPadstack.m_NumFixedCompEntries + i * aPadstack.m_NumCompsPerLayer;
        const ALLEGRO::PADSTACK_COMPONENT& padComp = aPadstack.m_Components[layerBaseIndex + BLK_0x1C_PADSTACK::LAYER_COMP_SLOT::PAD];
        const ALLEGRO::PADSTACK_COMPONENT& antiPadComp = aPadstack.m_Components[layerBaseIndex + BLK_0x1C_PADSTACK::LAYER_COMP_SLOT::ANTIPAD];
        const ALLEGRO::PADSTACK_COMPONENT& thermalComp = aPadstack.m_Components[layerBaseIndex + BLK_0x1C_PADSTACK::LAYER_COMP_SLOT::THERMAL_RELIEF];

        // If this is zero just skip entirely - I don't think we can usefully make pads with just thermal relief
        // Flag up if that happens.
        if( padComp.m_Type == PADSTACK_COMPONENT::TYPE_NULL)
        {
            if( antiPadComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL )
            {
                m_reporter.Report(
                        wxString::Format( "Padstack %s: Copper layer %zu has no pad component, but has antipad",
                                          padStackName, i ),
                        RPT_SEVERITY_WARNING );
            }
            if( thermalComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL )
            {
                m_reporter.Report(
                        wxString::Format( "Copper layer %zu has no pad component, but has thermal relief", i ),
                        RPT_SEVERITY_WARNING );
            }
            continue;
        }

        auto& layerCuProps = copperLayers[i];
        wxLogTrace( traceAllegroBuilder, "  Adding copper layer %zu with pad type %d", i, (int) padComp.m_Type );
        layerCuProps = std::make_unique<PADSTACK::COPPER_LAYER_PROPS>();

        switch( padComp.m_Type )
        {
        case PADSTACK_COMPONENT::TYPE_RECTANGLE:
            layerCuProps->shape.shape = PAD_SHAPE::RECTANGLE;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );
            break;
        case PADSTACK_COMPONENT::TYPE_SQUARE:
            layerCuProps->shape.shape = PAD_SHAPE::RECTANGLE;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_W } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );
            break;
        case PADSTACK_COMPONENT::TYPE_CIRCLE:
            layerCuProps->shape.shape = PAD_SHAPE::CIRCLE;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );
            break;
        case PADSTACK_COMPONENT::TYPE_OBLONG_X:
        case PADSTACK_COMPONENT::TYPE_OBLONG_Y:
            layerCuProps->shape.shape = PAD_SHAPE::OVAL;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );
            break;
        case PADSTACK_COMPONENT::TYPE_ROUNDED_RECTANGLE:
        {
            layerCuProps->shape.shape = PAD_SHAPE::ROUNDRECT;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );

            int minDim = std::min( std::abs( padComp.m_W ), std::abs( padComp.m_H ) );

            if( padComp.m_Z1.has_value() && padComp.m_Z1.value() > 0 && minDim > 0 )
                layerCuProps->shape.round_rect_radius_ratio = padComp.m_Z1.value() / (double) minDim;
            else
                layerCuProps->shape.round_rect_radius_ratio = 0.25;

            break;
        }
        case PADSTACK_COMPONENT::TYPE_CHAMFERED_RECTANGLE:
        {
            layerCuProps->shape.shape = PAD_SHAPE::CHAMFERED_RECT;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );

            int minDim = std::min( std::abs( padComp.m_W ), std::abs( padComp.m_H ) );

            if( padComp.m_Z1.has_value() && padComp.m_Z1.value() > 0 && minDim > 0 )
                layerCuProps->shape.chamfered_rect_ratio = padComp.m_Z1.value() / (double) minDim;
            else
                layerCuProps->shape.chamfered_rect_ratio = 0.25;

            layerCuProps->shape.chamfered_rect_positions = RECT_CHAMFER_ALL;
            break;
        }
        case PADSTACK_COMPONENT::TYPE_OCTAGON:
        {
            // Approximate octagon as a round rectangle with ~29.3% corner radius
            // (tan(22.5°) ≈ 0.414, half of that as ratio ≈ 0.207, but visually 0.293 is closer)
            layerCuProps->shape.shape = PAD_SHAPE::CHAMFERED_RECT;
            layerCuProps->shape.size = scaleSize( VECTOR2I{ padComp.m_W, padComp.m_H } );
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );
            layerCuProps->shape.chamfered_rect_ratio = 1.0 - 1.0 / sqrt( 2.0 );
            layerCuProps->shape.chamfered_rect_positions = RECT_CHAMFER_ALL;
            break;
        }
        case PADSTACK_COMPONENT::TYPE_SHAPE_SYMBOL:
        {
            // Custom shape defined by a 0x28 polygon. Walk the shape's segments and build
            // a polygon primitive for this pad.
            const BLK_0x28_SHAPE* shapeData =
                    expectBlockByKey<BLK_0x28_SHAPE>( padComp.m_StrPtr, 0x28 );

            if( !shapeData )
            {
                wxLogTrace( traceAllegroBuilder,
                            "Padstack %s: SHAPE_SYMBOL on layer %zu has no 0x28 shape at %#010x",
                            padStackName, i, padComp.m_StrPtr );
                break;
            }

            SHAPE_LINE_CHAIN outline = buildSegmentChain( shapeData->m_FirstSegmentPtr );

            if( outline.PointCount() >= 3 )
            {
                outline.SetClosed( true );

                layerCuProps->shape.shape = PAD_SHAPE::CUSTOM;
                layerCuProps->shape.anchor_shape = PAD_SHAPE::CIRCLE;

                // Anchor size based on the shape's bounding box center
                BOX2I bbox = outline.BBox();
                int anchorSize = static_cast<int>(
                        std::min( bbox.GetWidth(), bbox.GetHeight() ) / 4 );

                if( anchorSize < 1 )
                    anchorSize = 1;

                layerCuProps->shape.size = VECTOR2I( anchorSize, anchorSize );

                auto poly = std::make_shared<PCB_SHAPE>( nullptr, SHAPE_T::POLY );
                poly->SetPolyShape( SHAPE_POLY_SET( outline ) );
                poly->SetFilled( true );
                poly->SetWidth( 0 );
                layerCuProps->custom_shapes.push_back( poly );
            }
            else
            {
                wxLogTrace( traceAllegroBuilder,
                            "Padstack %s: SHAPE_SYMBOL on layer %zu produced only %d points",
                            padStackName, i, outline.PointCount() );
            }

            break;
        }
        case PADSTACK_COMPONENT::TYPE_PENTAGON:
        {
            layerCuProps->shape.shape = PAD_SHAPE::CUSTOM;
            layerCuProps->shape.anchor_shape = PAD_SHAPE::CIRCLE;
            layerCuProps->shape.offset = scale( VECTOR2I{ padComp.m_X3, padComp.m_X4 } );

            const int w = std::max( padComp.m_W, 300 );
            const int h = std::max( padComp.m_H, 220 );

            SHAPE_LINE_CHAIN outline;
            auto             S = [&]( int x, int y )
            {
                return scale( VECTOR2I{ x, y } );
            };

            // Regular pentagon with flat bottom edge
            outline.Append( S( 0, -h / 2 ) );
            outline.Append( S( w / 2, -h / 6 ) );
            outline.Append( S( w / 3, h / 2 ) );
            outline.Append( S( -w / 3, h / 2 ) );
            outline.Append( S( -w / 2, -h / 6 ) );
            outline.SetClosed( true );

            BOX2I bbox = outline.BBox();
            int   anchorSize = static_cast<int>(
                    std::min( bbox.GetWidth(), bbox.GetHeight() ) / 7 );

            if( anchorSize < 1 )
                anchorSize = 1;

            layerCuProps->shape.size = VECTOR2I( anchorSize, anchorSize );

            auto poly = std::make_shared<PCB_SHAPE>( nullptr, SHAPE_T::POLY );
            poly->SetPolyShape( SHAPE_POLY_SET( outline ) );
            poly->SetFilled( true );
            poly->SetWidth( 0 );
            layerCuProps->custom_shapes.push_back( poly );
            break;
        }
        default:
            m_reporter.Report(
                    wxString::Format( "Padstack %s: unhandled copper pad shape type %d on layer %zu",
                                      padStackName, static_cast<int>( padComp.m_Type ), i ),
                    RPT_SEVERITY_WARNING );
            break;
        }

        if( antiPadComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL)
        {
            if( antiPadComp.m_Type != padComp.m_Type )
            {
                wxLogTrace( traceAllegroBuilder, "Padstack %s: copper layer %zu antipad shape %d "
                            "differs from pad shape %d",
                            padStackName, i, antiPadComp.m_Type, padComp.m_Type );
            }

            int clearanceX = scale( ( antiPadComp.m_W - padComp.m_W ) / 2 );
            int clearanceY = scale( ( antiPadComp.m_H - padComp.m_H ) / 2 );

            if( clearanceX && clearanceX != clearanceY )
            {
                wxLogTrace( traceAllegroBuilder, "Padstack %s: copper layer %zu unequal antipad "
                            "clearance X=%d Y=%d",
                            padStackName, i, clearanceX, clearanceY );
            }

            if( antiPadComp.m_X3 != 0 || antiPadComp.m_X4 != 0 )
            {
                wxLogTrace( traceAllegroBuilder, "Padstack %s: copper layer %zu antipad offset "
                            "%d, %d",
                            padStackName, i, antiPadComp.m_X3, antiPadComp.m_X4 );
            }

            layerCuProps->clearance = clearanceX;
        }

        if( thermalComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL && !thermalGap.has_value() )
        {
            // The thermal gap is the clearance between the pad copper and the surrounding zone.
            // We derive it from the antipad-to-pad size difference.
            if( antiPadComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL && padComp.m_W > 0 )
            {
                int gap = scale( ( antiPadComp.m_W - padComp.m_W ) / 2 );

                if( gap > 0 )
                    thermalGap = gap;
            }

            wxLogTrace( traceAllegroBuilder,
                        "Padstack %s: thermal relief type=%d, gap=%snm",
                        padStackName, thermalComp.m_Type,
                        thermalGap.has_value() ? wxString::Format( "%d", thermalGap.value() )
                                               : wxString( "N/A" ) );
        }

        // Padstack-level keepouts (antipad relief geometry) are handled via the
        // antipad slot in copperLayers above. Board-level keepouts use BLK_0x34.
    }

    // We have now constructed a list of copper props. We can determine the PADSTACK mode now and assign the shapes
    PADSTACK padStack( &aFp );

    if( copperLayers.size() == 0 )
    {
        // SMD aperture PAD or something?
    }
    else
    {
        const auto layersEqual = [&](size_t aFrom, size_t aTo) -> bool
        {
            bool eq = true;
            for( size_t i = aFrom + 1; i < aTo; ++i )
            {
                if( !copperLayers[i - 1] || !copperLayers[i] || *copperLayers[i - 1] != *copperLayers[i] )
                {
                    eq = false;
                    break;
                }
            }
            return eq;
        };

        for(size_t i = 0; i < copperLayers.size(); ++i )
        {
            wxLogTrace( traceAllegroBuilder, "  Layer %zu: %s", i, copperLayers[i] ? "present" : "null" );
        }

        padStack.SetLayerSet( PAD::PTHMask() );

        if( copperLayers.front() && copperLayers.back() && layersEqual( 0, copperLayers.size() ) )
        {
            wxLogTrace( traceAllegroBuilder, "  Using NORMAL padstack mode (all layers identical)" );
            padStack.SetMode( PADSTACK::MODE::NORMAL );
            PADSTACK::COPPER_LAYER_PROPS& layerProps = padStack.CopperLayer( F_Cu );
            layerProps = *copperLayers.front();
        }
        else if( copperLayers.front() && copperLayers.back()
                 && layersEqual( 1, copperLayers.size() - 1 ) )
        {
            wxLogTrace( traceAllegroBuilder, "  Using FRONT_INNER_BACK padstack mode (inner layers identical)" );
            padStack.SetMode( PADSTACK::MODE::FRONT_INNER_BACK );
            padStack.CopperLayer( F_Cu ) = *copperLayers.front();
            padStack.CopperLayer( B_Cu ) = *copperLayers.back();

            // May be B_Cu if layers = 2, but that's OK
            if( copperLayers.size() > 2 && copperLayers[1] )
                padStack.CopperLayer( In1_Cu ) = *copperLayers[1];
        }
        else
        {
            wxLogTrace( traceAllegroBuilder, "  Using CUSTOM padstack mode (layers differ)" );
            padStack.SetMode( PADSTACK::MODE::CUSTOM );

            for( size_t i = 0; i < copperLayers.size(); ++i )
            {
                if( !copperLayers[i] )
                    continue;

                PCB_LAYER_ID layer = F_Cu;

                if( i == 0 )
                    layer = F_Cu;
                else if( i == copperLayers.size() - 1 )
                    layer = B_Cu;
                else
                    layer = ToLAYER_ID( In1_Cu + static_cast<int>( i - 1 ) * 2 );

                padStack.CopperLayer( layer ) = *copperLayers[i];
            }
        }
    }

    // The drill/slot dimensions are extracted in priority order:
    //
    // 1. V172+ m_SlotAndUnknownArr[0] and [3] hold the true slot outline dimensions (X and Y).
    //    For routed slots (round drill bit + routing path), m_DrillArr only has the bit diameter,
    //    while m_SlotAndUnknownArr has the full slot envelope.
    //
    // 2. V172+ m_DrillArr[4] (width) and m_DrillArr[7] (height, 0 for round).
    //    These match m_SlotAndUnknownArr for punched oblong drills but only have the bit
    //    diameter for routed slots.
    //
    // 3. Pre-V172 m_Drill field (drill diameter, always round).
    int drillW = 0;
    int drillH = 0;

    if( m_brdDb.m_FmtVer >= FMT_VER::V_172 )
    {
        if( aPadstack.m_SlotAndUnknownArr.has_value() )
        {
            const auto& slotArr = aPadstack.m_SlotAndUnknownArr.value();
            int slotX = scale( static_cast<int>( slotArr[0] ) );
            int slotY = scale( static_cast<int>( slotArr[3] ) );

            if( slotX > 0 && slotY > 0 )
            {
                drillW = slotX;
                drillH = slotY;
            }
        }

        if( drillW == 0 )
        {
            drillW = scale( static_cast<int>( aPadstack.m_DrillArr[4] ) );
            drillH = scale( static_cast<int>( aPadstack.m_DrillArr[7] ) );
        }
    }
    else
    {
        drillW = scale( static_cast<int>( aPadstack.m_Drill ) );
    }

    if( drillH == 0 )
        drillH = drillW;

    // Allegro stores slot dimensions as (primary, secondary) regardless of orientation,
    // not as (X, Y). Compare the first copper layer pad's aspect ratio to determine if
    // the drill needs to be rotated 90 degrees.
    if( drillW != drillH && aPadstack.m_LayerCount > 0 )
    {
        size_t firstCopperIdx = aPadstack.m_NumFixedCompEntries;
        const ALLEGRO::PADSTACK_COMPONENT& firstPadComp =
                aPadstack.m_Components[firstCopperIdx + BLK_0x1C_PADSTACK::LAYER_COMP_SLOT::PAD];

        bool padIsTaller = ( std::abs( firstPadComp.m_H ) > std::abs( firstPadComp.m_W ) );
        bool drillIsTaller = ( drillH > drillW );

        if( padIsTaller != drillIsTaller )
            std::swap( drillW, drillH );
    }

    bool isSmd = ( drillW == 0 ) || ( aPadstack.m_LayerCount == 1 );

    if( isSmd )
    {
        padStack.Drill().size = VECTOR2I( 0, 0 );
    }
    else
    {
        padStack.Drill().size = VECTOR2I( drillW, drillH );

        if( drillW != drillH )
            padStack.Drill().shape = PAD_DRILL_SHAPE::OBLONG;
        else
            padStack.Drill().shape = PAD_DRILL_SHAPE::CIRCLE;
    }

    std::unique_ptr<PAD> pad = std::make_unique<PAD>( &aFp );
    pad->SetPadstack( padStack );
    pad->SetNumber( aPadName );
    pad->SetNetCode( aNetcode );

    if( isSmd )
    {
        pad->SetAttribute( PAD_ATTRIB::SMD );
        pad->SetLayerSet( PAD::SMDMask() );
    }
    else
    {
        pad->SetAttribute( PAD_ATTRIB::PTH );
        pad->SetLayerSet( PAD::PTHMask() );
    }

    if( thermalGap.has_value() )
        pad->SetThermalGap( thermalGap.value() );

    padItems.push_back( std::move( pad ) );

    // Now, for each technical layer, we see if we can include it into the existing padstack, or if we need to add
    // it as a standalone pad
    for( size_t i = 0; i < aPadstack.m_NumFixedCompEntries; ++i )
    {
        const ALLEGRO::PADSTACK_COMPONENT& psComp = aPadstack.m_Components[i];

        /// If this is zero just skip entirely
        if( psComp.m_Type == PADSTACK_COMPONENT::TYPE_NULL)
            continue;

        // All fixed slots are technical layers (solder mask, paste mask, film mask,
        // assembly variant, etc). Custom mask expansion extraction is not yet implemented;
        // KiCad's default pad-matches-mask behavior applies.
        wxLogTrace( traceAllegroBuilder,
                    "Fixed padstack slot %zu: type=%d, W=%d, H=%d",
                    i, static_cast<int>( psComp.m_Type ), psComp.m_W, psComp.m_H );
    }

    return padItems;
}


std::unique_ptr<FOOTPRINT> BOARD_BUILDER::buildFootprint( const BLK_0x2D_FOOTPRINT_INST& aFpInstance )
{
    auto fp = std::make_unique<FOOTPRINT>( &m_board );

    const BLK_0x07_COMPONENT_INST* fpInstData = getFpInstRef( aFpInstance );

    wxLogTrace( traceAllegroBuilder, "Building footprint from 0x2D block key %#010x", aFpInstance.m_Key );

    wxString refDesStr;
    if( fpInstData )
    {
        refDesStr = m_brdDb.GetString( fpInstData->m_RefDesStrPtr );

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

    wxLogTrace( traceAllegroBuilder, "  Footprint reference: '%s'", refDesStr );

    const VECTOR2I  fpPos = scale( VECTOR2I{ aFpInstance.m_CoordX, aFpInstance.m_CoordY } );
    const EDA_ANGLE rotation{ aFpInstance.m_Rotation / 1000., DEGREES_T };

    fp->SetPosition( fpPos );
    fp->SetOrientation( rotation );

    // Allegro stores placed instance data in board-absolute form: bottom-side
    // components already have shapes on bottom layers with bottom-side positions.
    // Allegro stores placed footprints in board-absolute form with final layers.
    // KiCad stores footprints in canonical front-side form and uses Flip() to
    // mirror both positions and layers to the back side.
    //
    // Move back-layer items to their front-side counterpart so that fp->Flip()
    // consistently mirrors positions AND layers for all children. Without this,
    // bottom-side footprints would have their back-layer graphics double-flipped
    // to the front.
    const auto canonicalizeLayer = []( BOARD_ITEM* aItem )
    {
        if( IsBackLayer( aItem->GetLayer() ) )
            aItem->SetLayer( FlipLayer( aItem->GetLayer() ) );
    };

    const LL_WALKER graphicsWalker{ aFpInstance.m_GraphicPtr, aFpInstance.m_Key, m_brdDb };

    for( const BLOCK_BASE* graphicsBlock : graphicsWalker )
    {
        const uint8_t type = graphicsBlock->GetBlockType();

        if( type == 0x14 )
        {
            const auto& graphics = static_cast<const BLOCK<BLK_0x14_GRAPHIC>&>( *graphicsBlock ).GetData();

            std::vector<std::unique_ptr<PCB_SHAPE>> shapes = buildShapes( graphics, *fp );

            for( std::unique_ptr<PCB_SHAPE>& shape : shapes )
            {
                canonicalizeLayer( shape.get() );
                fp->Add( shape.release() );
            }
        }
        else
        {
            m_reporter.Report( wxString::Format( "Unexpected type in graphics list: %#04x", type ),
                               RPT_SEVERITY_WARNING );
        }
    }

    bool valueFieldSet = false;

    const LL_WALKER textWalker{ aFpInstance.m_TextPtr, aFpInstance.m_Key, m_brdDb };

    for( const BLOCK_BASE* textBlock : textWalker )
    {
        const uint8_t type = textBlock->GetBlockType();

        if( type != 0x30 )
            continue;

        const auto& strWrapper = static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( *textBlock ).GetData();

        std::unique_ptr<PCB_TEXT> text = buildPcbText( strWrapper, *fp );

        if( !text )
            continue;

        canonicalizeLayer( text.get() );

        const uint8_t textClass = strWrapper.m_Layer.m_Class;
        const uint8_t textSubclass = strWrapper.m_Layer.m_Subclass;

        bool isSilk = ( textSubclass == LAYER_INFO::SUBCLASS::SILKSCREEN_TOP
                        || textSubclass == LAYER_INFO::SUBCLASS::SILKSCREEN_BOTTOM );
        bool isAssembly = ( textSubclass == LAYER_INFO::SUBCLASS::ASSEMBLY_TOP
                            || textSubclass == LAYER_INFO::SUBCLASS::ASSEMBLY_BOTTOM );

        if( textClass == LAYER_INFO::CLASS::REF_DES && isSilk )
        {
            // Visible silkscreen refdes updates the built-in REFERENCE field.
            PCB_FIELD* const refDes = fp->GetField( FIELD_T::REFERENCE );

            // KiCad netlisting requires non-digit + digit annotation.
            if( !text->GetText().IsEmpty() && !wxIsalpha( text->GetText()[0] ) )
                text->SetText( wxString( "UNK" ) + text->GetText() );

            *refDes = PCB_FIELD( *text, FIELD_T::REFERENCE );
        }
        else if( textClass == LAYER_INFO::CLASS::REF_DES && isAssembly )
        {
            // Assembly refdes becomes a user field with the KiCad reference variable
            PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "Reference" ) );
            field->SetText( wxS( "${REFERENCE}" ) );
            field->SetVisible( false );
            fp->Add( field, ADD_MODE::APPEND );
        }
        else if( textClass == LAYER_INFO::CLASS::COMPONENT_VALUE && isAssembly )
        {
            if( !valueFieldSet )
            {
                // First COMPONENT_VALUE on assembly updates the built-in VALUE field
                PCB_FIELD* valField = fp->GetField( FIELD_T::VALUE );
                *valField = PCB_FIELD( *text, FIELD_T::VALUE );
                valField->SetVisible( false );
                valueFieldSet = true;
            }
            else
            {
                PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "Component Value" ) );
                field->SetVisible( false );
                fp->Add( field, ADD_MODE::APPEND );
            }
        }
        else if( textClass == LAYER_INFO::CLASS::DEVICE_TYPE )
        {
            PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "Device Type" ) );
            field->SetVisible( !isAssembly );
            fp->Add( field, ADD_MODE::APPEND );
        }
        else if( textClass == LAYER_INFO::CLASS::TOLERANCE )
        {
            PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "Tolerance" ) );
            field->SetVisible( isSilk );
            fp->Add( field, ADD_MODE::APPEND );
        }
        else if( textClass == LAYER_INFO::CLASS::USER_PART_NUMBER )
        {
            PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "User Part Number" ) );
            field->SetVisible( isSilk );
            fp->Add( field, ADD_MODE::APPEND );
        }
        else if( textClass == LAYER_INFO::CLASS::COMPONENT_VALUE && isSilk )
        {
            PCB_FIELD* field = new PCB_FIELD( *text, FIELD_T::USER, wxS( "Component Value" ) );
            field->SetVisible( true );
            fp->Add( field, ADD_MODE::APPEND );
        }
        else
        {
            fp->Add( text.release() );
        }
    }

    // Assembly drawing
    LL_WALKER assemblyWalker{ aFpInstance.m_AssemblyPtr, aFpInstance.m_Key, m_brdDb };

    for( const BLOCK_BASE* assemblyBlock : assemblyWalker )
    {
        std::vector<std::unique_ptr<BOARD_ITEM>> shapes = buildGraphicItems( *assemblyBlock, *fp );

        for( std::unique_ptr<BOARD_ITEM>& item : shapes )
        {
            canonicalizeLayer( item.get() );
            fp->Add( item.release() );
        }
    }

    // Areas (courtyards, etc)
    LL_WALKER areaWalker{ aFpInstance.m_AreasPtr, aFpInstance.m_Key, m_brdDb };
    for( const BLOCK_BASE* areaBlock : areaWalker )
    {
        std::vector<std::unique_ptr<BOARD_ITEM>> shapes = buildGraphicItems( *areaBlock, *fp );

        for( std::unique_ptr<BOARD_ITEM>& item : shapes )
        {
            canonicalizeLayer( item.get() );
            fp->Add( item.release() );
        }
    }

    // Find the pads
    LL_WALKER padWalker{ aFpInstance.m_FirstPadPtr, aFpInstance.m_Key, m_brdDb };
    padWalker.SetNextFunc( PadGetNextInFootprint );
    for( const BLOCK_BASE* padBlock : padWalker )
    {
        const auto& placedPadInfo = static_cast<const BLOCK<BLK_0x32_PLACED_PAD>&>( *padBlock ).GetData();

        const BLK_0x04_NET_ASSIGNMENT* netAssignment =
                expectBlockByKey<BLK_0x04_NET_ASSIGNMENT>( placedPadInfo.m_NetPtr, 0x04 );
        const BLK_0x0D_PAD* padInfo = expectBlockByKey<BLK_0x0D_PAD>( placedPadInfo.m_PadPtr, 0x0D );

        if( !netAssignment || !padInfo )
            continue;

        const BLK_0x1C_PADSTACK* padStack = expectBlockByKey<BLK_0x1C_PADSTACK>( padInfo->m_PadStack, 0x1C );

        if( !padStack )
            continue;

        auto netIt = m_netCache.find( netAssignment->m_Net );
        const int       netCode = ( netIt != m_netCache.end() ) ? netIt->second->GetNetCode()
                                                                 : NETINFO_LIST::UNCONNECTED;
        const wxString  padName = m_brdDb.GetString( padInfo->m_NameStrId );

        // 0x0D coordinates and rotation are in the footprint's local (unrotated) space.
        // Use SetFPRelativePosition/Orientation to let KiCad handle the transform to
        // board-absolute coordinates (rotating by FP orientation and adding FP position).
        const VECTOR2I  padLocalPos = scale( VECTOR2I{ padInfo->m_CoordsX, padInfo->m_CoordsY } );
        const EDA_ANGLE padLocalRot{ static_cast<double>( padInfo->m_Rotation ) / 1000.0, DEGREES_T };

        std::vector<std::unique_ptr<BOARD_ITEM>> padItems = buildPadItems( *padStack, *fp, padName, netCode );

        for( std::unique_ptr<BOARD_ITEM>& item : padItems )
        {
            if( item->Type() == PCB_PAD_T )
            {
                PAD* pad = static_cast<PAD*>( item.get() );
                pad->SetFPRelativeOrientation( padLocalRot );
            }

            item->SetFPRelativePosition( padLocalPos );
            fp->Add( item.release() );
        }
    }

    // Flip AFTER adding all children so that graphics, text, and pads all get
    // their layers and positions mirrored correctly for bottom-layer footprints.
    //
    // Allegro mirrors bottom-side components via X-mirror (flip around Y axis),
    // then rotates by R. KiCad's Flip(TOP_BOTTOM) is a Y-mirror which negates
    // the orientation. Since X-mirror = Y-mirror + Rotate(180), we need the
    // final orientation to be R+180. Flip negates what we set before it, so
    // set -(R+180) to get R+180 after negation.
    if( aFpInstance.m_Layer != 0 )
    {
        fp->SetOrientation( -rotation - ANGLE_180 );
        fp->Flip( fpPos, FLIP_DIRECTION::TOP_BOTTOM );
    }

    return fp;
}

std::vector<std::unique_ptr<BOARD_ITEM>> BOARD_BUILDER::buildTrack( const BLK_0x05_TRACK& aTrackBlock, int aNetCode )
{
    std::vector<std::unique_ptr<BOARD_ITEM>> items;

    // Anti-etch tracks are thermal relief patterns generated by Allegro.
    // These are handled by pad-level thermal relief properties instead.
    if( aTrackBlock.m_Layer.m_Class == LAYER_INFO::CLASS::ANTI_ETCH )
    {
        wxLogTrace( traceAllegroBuilder, "Skipping ANTI_ETCH track (class=%#04x, subclass=%#04x)",
                    aTrackBlock.m_Layer.m_Class, aTrackBlock.m_Layer.m_Subclass );
        return items;
    }

    const PCB_LAYER_ID layer = getLayer( aTrackBlock.m_Layer );

    LL_WALKER segWalker{ aTrackBlock.m_FirstSegPtr, aTrackBlock.m_Key, m_brdDb };
    for( const BLOCK_BASE* block : segWalker )
    {
        const uint8_t segType = block->GetBlockType();

        switch( segType )
        {
        case 0x15:
        case 0x16:
        case 0x17:
        {
            const BLK_0x15_16_17_SEGMENT& segInfo =
                    static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *block ).GetData();

            VECTOR2I start{ segInfo.m_StartX, segInfo.m_StartY };
            VECTOR2I end{ segInfo.m_EndX, segInfo.m_EndY };
            int      width = static_cast<int>( segInfo.m_Width );

            std::unique_ptr<PCB_TRACK> seg = std::make_unique<PCB_TRACK>( &m_board );

            seg->SetNetCode( aNetCode );
            seg->SetLayer( layer );

            seg->SetStart( scale( start ) );
            seg->SetEnd( scale( end ) );
            seg->SetWidth( scale( width ) );

            items.push_back( std::move( seg ) );
            break;
        }
        case 0x01:
        {
            const BLK_0x01_ARC& arcInfo = static_cast<const BLOCK<BLK_0x01_ARC>&>( *block ).GetData();

            VECTOR2I start = scale( { arcInfo.m_StartX, arcInfo.m_StartY } );
            VECTOR2I end = scale( { arcInfo.m_EndX, arcInfo.m_EndY } );
            VECTOR2I c = scale( KiROUND( VECTOR2D{ arcInfo.m_CenterX, arcInfo.m_CenterY } ) );
            int      width = scale( static_cast<int>( arcInfo.m_Width ) );

            bool clockwise = ( arcInfo.m_SubType & 0x40 ) != 0;

            EDA_ANGLE startAngle( start - c );
            EDA_ANGLE endAngle( end - c );
            startAngle.Normalize();
            endAngle.Normalize();

            EDA_ANGLE angle = endAngle - startAngle;

            if( clockwise && angle < ANGLE_0 )
                angle += ANGLE_360;

            if( !clockwise && angle > ANGLE_0 )
                angle -= ANGLE_360;

            VECTOR2I mid = start;
            RotatePoint( mid, c, -angle / 2.0 );

            std::unique_ptr<PCB_ARC> arc = std::make_unique<PCB_ARC>( &m_board );

            arc->SetNetCode( aNetCode );
            arc->SetLayer( layer );

            arc->SetStart( start );
            arc->SetMid( mid );
            arc->SetEnd( end );
            arc->SetWidth( width );

            items.push_back( std::move( arc ) );
            break;
        }
        default:
            wxLogTrace( traceAllegroBuilder, "Unhandled segment type in track: %#04x", segType );
            break;
        }
    }
    return items;
}


std::unique_ptr<BOARD_ITEM> BOARD_BUILDER::buildVia( const BLK_0x33_VIA& aViaData, int aNetCode )
{
    VECTOR2I viaPos{ aViaData.m_CoordsX, aViaData.m_CoordsY };

    const BLK_0x1C_PADSTACK* viaPadstack = expectBlockByKey<BLK_0x1C_PADSTACK>( aViaData.m_Padstack, 0x1C );

    if( !viaPadstack )
        return nullptr;

    std::unique_ptr<PCB_VIA> via = std::make_unique<PCB_VIA>( &m_board );
    via->SetPosition( scale( viaPos ) );
    via->SetNetCode( aNetCode );

    via->SetTopLayer( F_Cu );
    via->SetBottomLayer( B_Cu );

    // Extract via size from the first copper layer's pad component
    int viaWidth = 0;

    if( viaPadstack->m_LayerCount > 0 )
    {
        const size_t layerBaseIndex = viaPadstack->m_NumFixedCompEntries;
        const ALLEGRO::PADSTACK_COMPONENT& padComp =
                viaPadstack->m_Components[layerBaseIndex + BLK_0x1C_PADSTACK::LAYER_COMP_SLOT::PAD];

        if( padComp.m_Type != PADSTACK_COMPONENT::TYPE_NULL )
        {
            viaWidth = scale( padComp.m_W );
        }
    }

    int viaDrill = 0;

    if( m_brdDb.m_FmtVer >= FMT_VER::V_172 )
        viaDrill = scale( static_cast<int>( viaPadstack->m_DrillArr[4] ) );
    else
        viaDrill = scale( static_cast<int>( viaPadstack->m_Drill ) );

    if( viaDrill == 0 )
    {
        viaDrill = viaWidth / 2;
        wxLogTrace( traceAllegroBuilder, "Via at (%d, %d): no drill in padstack, using fallback %d",
                    aViaData.m_CoordsX, aViaData.m_CoordsY, viaDrill );
    }

    if( viaWidth <= 0 )
    {
        wxLogTrace( traceAllegroBuilder, "Via at (%d, %d) has no valid pad component, using drill-based fallback",
                    aViaData.m_CoordsX, aViaData.m_CoordsY );
        viaWidth = viaDrill * 2;
    }

    via->SetWidth( F_Cu, viaWidth );
    via->SetDrill( viaDrill );

    return via;
}


void BOARD_BUILDER::createTracks()
{
    wxLogTrace( traceAllegroBuilder, "Creating tracks, vias, and other routed items" );

    std::vector<BOARD_ITEM*> newItems;

    // We need to walk this list again - we could do this all in createNets, but this seems tidier.
    LL_WALKER netWalker{ m_brdDb.m_Header->m_LL_0x1B_Nets, m_brdDb };
    for( const BLOCK_BASE* block : netWalker )
    {
        const uint8_t type = block->GetBlockType();
        if( type != BLOCK_TYPE::x1B_NET )
        {
            reportUnexpectedBlockType( type, BLOCK_TYPE::x1B_NET, 0, block->GetOffset(), "Net" );
            continue;
        }

        const auto& net = static_cast<const BLOCK<BLK_0x1B_NET>&>( *block ).GetData();

        auto netIt = m_netCache.find( net.m_Key );

        if( netIt == m_netCache.end() )
            continue;

        const int netCode = netIt->second->GetNetCode();

        LL_WALKER assignmentWalker{ net.m_Assignment, net.m_Key, m_brdDb };
        for( const BLOCK_BASE* assignBlock : assignmentWalker )
        {
            if( assignBlock->GetBlockType() != 0x04 )
            {
                reportUnexpectedBlockType( assignBlock->GetBlockType(), 0x04, 0, block->GetOffset(), "Net assignment" );
                continue;
            }

            const auto& assign = static_cast<const BLOCK<BLK_0x04_NET_ASSIGNMENT>&>( *assignBlock ).GetData();

            // Walk the 0x05/0x32/... list
            LL_WALKER connWalker{ assign.m_ConnItem, assign.m_Key, m_brdDb };
            for( const BLOCK_BASE* connItemBlock : connWalker )
            {
                const uint8_t connType = connItemBlock->GetBlockType();

                // One connected item can be multiple KiCad objects, e.g.
                // 0x05 track -> list of segments/arcs
                std::vector<std::unique_ptr<BOARD_ITEM>> newItemList;

                switch( connType )
                {
                // Track
                case 0x05:
                {
                    const BLK_0x05_TRACK& trackData =
                            static_cast<const BLOCK<BLK_0x05_TRACK>&>( *connItemBlock ).GetData();
                    newItemList = buildTrack( trackData, netCode );
                    break;
                }
                case 0x33:
                {
                    const BLK_0x33_VIA& viaData = static_cast<const BLOCK<BLK_0x33_VIA>&>( *connItemBlock ).GetData();
                    newItemList.push_back( buildVia( viaData, netCode ) );
                    break;
                }
                case 0x32:
                {
                    // This is a pad in a footprint - we don't need to handle this here, as we do all the footprint
                    // pads, connected or not, in the footprint step.
                    break;
                }
                case 0x28:
                {
                    // 0x28 shapes on the net chain are computed copper fills.
                    // Collect them for zone fill polygon import.
                    const BLK_0x28_SHAPE& fillShape =
                            static_cast<const BLOCK<BLK_0x28_SHAPE>&>( *connItemBlock ).GetData();

                    PCB_LAYER_ID fillLayer = getLayer( fillShape.m_Layer );

                    if( fillLayer != UNDEFINED_LAYER )
                        m_zoneFillShapes.push_back( { &fillShape, netCode, fillLayer } );

                    break;
                }
                case 0x2E:
                default:
                {
                    wxLogTrace( traceAllegroBuilder, "  Unhandled connected item code: %#04x",
                                (int) connType );
                }
                }

                for( std::unique_ptr<BOARD_ITEM>& newItem : newItemList )
                {
                    newItems.push_back( newItem.get() );
                    m_board.Add( newItem.release(), ADD_MODE::BULK_APPEND );
                }
            }
        }
    }

    m_board.FinalizeBulkAdd( newItems );

    wxLogTrace( traceAllegroBuilder, "Finished creating %zu track/via items", newItems.size() );
}


void BOARD_BUILDER::createBoardShapes()
{
    wxLogTrace( traceAllegroBuilder, "Creating shapes" );

    // Walk through LL_0x24_0x28 which contains rectangles (0x24) and shapes (0x28)
    const LL_WALKER shapeWalker( m_brdDb.m_Header->m_LL_0x24_0x28, m_brdDb );
    int             blockCount = 0;

    std::vector<std::unique_ptr<BOARD_ITEM>> newItems;

    for( const BLOCK_BASE* block : shapeWalker )
    {
        blockCount++;

        switch( block->GetBlockType() )
        {
        case 0x24:
        {
            const BLK_0x24_RECT& rectData = BlockDataAs<BLK_0x24_RECT>( *block );

            // These are zones, we don't handle them here
            if( layerIsZone( rectData.m_Layer ) )
                continue;

            std::unique_ptr<PCB_SHAPE> rectShape = buildRect( rectData, m_board );
            newItems.push_back( std::move( rectShape ) );
            break;
        }
        case 0x28:
        {
            const BLK_0x28_SHAPE& shapeData = BlockDataAs<BLK_0x28_SHAPE>( *block );

            // These are zones, we don't handle them here
            if( layerIsZone( shapeData.m_Layer ) )
                continue;

            std::vector<std::unique_ptr<PCB_SHAPE>> shapeItems = buildPolygonShapes( shapeData, m_board );

            for( auto& shapeItem : shapeItems )
                newItems.push_back( std::move( shapeItem ) );
            break;
        }
        default:
        {
            wxLogTrace( traceAllegroBuilder, "  Unhandled block type in outline walker: %#04x", block->GetBlockType() );
            break;
        }
        }
    }

    wxLogTrace( traceAllegroBuilder, "  Found %d shape blocks", blockCount, newItems.size() );
    blockCount = 0;

    LL_WALKER outline2Walker( m_brdDb.m_Header->m_LL_Shapes, m_brdDb );
    for( const BLOCK_BASE* block : outline2Walker )
    {
        blockCount++;

        // Skip boundary layer shapes - these are handled in the zones phase
        const auto shouldSkip = []( const LAYER_INFO& aLayer ) -> bool
        {
            return aLayer.m_Class == LAYER_INFO::CLASS::BOUNDARY;
        };

        switch( block->GetBlockType() )
        {
        case 0x0E:
        {
            const BLK_0x0E_RECT& rectData = BlockDataAs<BLK_0x0E_RECT>( *block );

            if( shouldSkip( rectData.m_Layer ) )
                continue;

            std::unique_ptr<PCB_SHAPE> rectShape = buildRect( rectData, m_board );
            newItems.push_back( std::move( rectShape ) );
            break;
        }
        case 0x24:
        {
            const BLK_0x24_RECT& rectData = BlockDataAs<BLK_0x24_RECT>( *block );

            if( shouldSkip( rectData.m_Layer ) )
                continue;

            std::unique_ptr<PCB_SHAPE> rectShape = buildRect( rectData, m_board );
            newItems.push_back( std::move( rectShape ) );
            break;
        }
        case 0x28:
        {
            const BLK_0x28_SHAPE& shapeData = BlockDataAs<BLK_0x28_SHAPE>( *block );

            if( shouldSkip( shapeData.m_Layer ) )
                continue;

            std::vector<std::unique_ptr<PCB_SHAPE>> shapeItems = buildPolygonShapes( shapeData, m_board );

            for( auto& shapeItem : shapeItems )
                newItems.push_back( std::move( shapeItem ) );
            break;
        }
        default:
        {
            wxLogTrace( traceAllegroBuilder, "  Unhandled block type in outline walker: %#04x", block->GetBlockType() );
            break;
        }
        }
    }

    wxLogTrace( traceAllegroBuilder, "  Found %d outline items in m_LL_Shapes", blockCount );
    blockCount = 0;

    LL_WALKER graphicContainerWalker( m_brdDb.m_Header->m_LL_0x14, m_brdDb );
    for( const BLOCK_BASE* block : graphicContainerWalker )
    {
        blockCount++;

        switch( block->GetBlockType() )
        {
        case 0x14:
        {
            const auto& graphicContainer = BlockDataAs<BLK_0x14_GRAPHIC>( *block );

            std::vector<std::unique_ptr<PCB_SHAPE>> graphicItems = buildShapes( graphicContainer, m_board );

            for( auto& item : graphicItems )
                newItems.push_back( std::move( item ) );
            break;
        }
        default:
        {
            wxLogTrace( traceAllegroBuilder, "  Unhandled block type in graphic container walker: %#04x",
                        block->GetBlockType() );
            break;
        }
        }
    }

    wxLogTrace( traceAllegroBuilder, "  Found %d graphic container items", blockCount );

    std::vector<BOARD_ITEM*> addedItems;
    for( std::unique_ptr<BOARD_ITEM>& item : newItems )
    {
        addedItems.push_back( item.get() );
        m_board.Add( item.release(), ADD_MODE::BULK_APPEND );
    }

    m_board.FinalizeBulkAdd( addedItems );

    wxLogTrace( traceAllegroBuilder, "Created %zu board shapes", addedItems.size() );
}


const SHAPE_LINE_CHAIN& BOARD_BUILDER::buildSegmentChain( uint32_t aStartKey ) const
{
    auto cacheIt = m_segChainCache.find( aStartKey );

    if( cacheIt != m_segChainCache.end() )
        return cacheIt->second;

    SHAPE_LINE_CHAIN& outline = m_segChainCache[aStartKey];
    uint32_t          currentKey = aStartKey;

    // Safety limit to prevent infinite loops on corrupt data
    static constexpr int MAX_CHAIN_LENGTH = 50000;
    int visited = 0;

    while( currentKey != 0 && visited < MAX_CHAIN_LENGTH )
    {
        const BLOCK_BASE* block = m_brdDb.GetObjectByKey( currentKey );

        if( !block )
            break;

        visited++;

        switch( block->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *block ).GetData();
            VECTOR2I    start = scale( { arc.m_StartX, arc.m_StartY } );
            VECTOR2I    end = scale( { arc.m_EndX, arc.m_EndY } );
            VECTOR2I    center = scale( KiROUND( VECTOR2D{ arc.m_CenterX, arc.m_CenterY } ) );

            if( start == end )
            {
                SHAPE_ARC shapeArc( center, start, ANGLE_360 );
                outline.Append( shapeArc );
            }
            else
            {
                bool clockwise = ( arc.m_SubType & 0x40 ) != 0;

                EDA_ANGLE startAngle( start - center );
                EDA_ANGLE endAngle( end - center );
                startAngle.Normalize();
                endAngle.Normalize();

                EDA_ANGLE arcAngle = endAngle - startAngle;

                if( clockwise && arcAngle < ANGLE_0 )
                    arcAngle += ANGLE_360;

                if( !clockwise && arcAngle > ANGLE_0 )
                    arcAngle -= ANGLE_360;

                VECTOR2I mid = start;
                RotatePoint( mid, center, -arcAngle / 2.0 );

                SHAPE_ARC shapeArc( start, mid, end, 0 );
                outline.Append( shapeArc );
            }

            currentKey = arc.m_Next;
            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            const auto& seg =
                    static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *block ).GetData();
            VECTOR2I start = scale( { seg.m_StartX, seg.m_StartY } );

            if( outline.PointCount() == 0 || outline.CLastPoint() != start )
                outline.Append( start );

            VECTOR2I end = scale( { seg.m_EndX, seg.m_EndY } );
            outline.Append( end );
            currentKey = seg.m_Next;
            break;
        }
        default:
            currentKey = 0;
            break;
        }
    }

    return outline;
}


SHAPE_LINE_CHAIN BOARD_BUILDER::buildOutline( const BLK_0x0E_RECT& aRect ) const
{
    SHAPE_LINE_CHAIN outline;

    VECTOR2I topLeft = scale( VECTOR2I{ aRect.m_Coords[0], aRect.m_Coords[1] } );
    VECTOR2I botRight = scale( VECTOR2I{ aRect.m_Coords[2], aRect.m_Coords[3] } );
    VECTOR2I topRight{ botRight.x, topLeft.y };
    VECTOR2I botLeft{ topLeft.x, botRight.y };

    outline.Append( topLeft );
    outline.Append( topRight );
    outline.Append( botRight );
    outline.Append( botLeft );

    return outline;
}


SHAPE_LINE_CHAIN BOARD_BUILDER::buildOutline( const BLK_0x24_RECT& aRect ) const
{
    SHAPE_LINE_CHAIN outline;

    VECTOR2I topLeft = scale( VECTOR2I{ aRect.m_Coords[0], aRect.m_Coords[1] } );
    VECTOR2I botRight = scale( VECTOR2I{ aRect.m_Coords[2], aRect.m_Coords[3] } );
    VECTOR2I topRight{ botRight.x, topLeft.y };
    VECTOR2I botLeft{ topLeft.x, botRight.y };

    outline.Append( topLeft );
    outline.Append( topRight );
    outline.Append( botRight );
    outline.Append( botLeft );

    return outline;
}


SHAPE_LINE_CHAIN BOARD_BUILDER::buildOutline( const BLK_0x28_SHAPE& aShape ) const
{
    SHAPE_LINE_CHAIN outline;
    const LL_WALKER  segWalker{ aShape.m_FirstSegmentPtr, aShape.m_Key, m_brdDb };

    for( const BLOCK_BASE* segBlock : segWalker )
    {
        switch( segBlock->GetBlockType() )
        {
        case 0x01:
        {
            const auto& arc = static_cast<const BLOCK<BLK_0x01_ARC>&>( *segBlock ).GetData();
            VECTOR2I    start = scale( { arc.m_StartX, arc.m_StartY } );
            VECTOR2I    end = scale( { arc.m_EndX, arc.m_EndY } );
            VECTOR2I    center = scale( KiROUND( VECTOR2D{ arc.m_CenterX, arc.m_CenterY } ) );

            if( start == end )
            {
                SHAPE_ARC shapeArc( center, start, ANGLE_360 );
                outline.Append( shapeArc );
            }
            else
            {
                bool clockwise = ( arc.m_SubType & 0x40 ) != 0;

                EDA_ANGLE startAngle( start - center );
                EDA_ANGLE endAngle( end - center );
                startAngle.Normalize();
                endAngle.Normalize();

                EDA_ANGLE arcAngle = endAngle - startAngle;

                if( clockwise && arcAngle < ANGLE_0 )
                    arcAngle += ANGLE_360;

                if( !clockwise && arcAngle > ANGLE_0 )
                    arcAngle -= ANGLE_360;

                VECTOR2I mid = start;
                RotatePoint( mid, center, -arcAngle / 2.0 );

                SHAPE_ARC shapeArc( start, mid, end, 0 );
                outline.Append( shapeArc );
            }

            break;
        }
        case 0x15:
        case 0x16:
        case 0x17:
        {
            const auto& seg = static_cast<const BLOCK<BLK_0x15_16_17_SEGMENT>&>( *segBlock ).GetData();
            VECTOR2I    start = scale( { seg.m_StartX, seg.m_StartY } );

            if( outline.PointCount() == 0 || outline.CLastPoint() != start )
                outline.Append( start );

            VECTOR2I end = scale( { seg.m_EndX, seg.m_EndY } );
            outline.Append( end );
            break;
        }
        default:
            wxLogTrace( traceAllegroBuilder, "    Unhandled segment type in shape outline: %#04x",
                        segBlock->GetBlockType() );
            break;
        }
    }

    return outline;
}


SHAPE_POLY_SET BOARD_BUILDER::shapeToPolySet( const BLK_0x28_SHAPE& aShape ) const
{
    SHAPE_POLY_SET   polySet;
    SHAPE_LINE_CHAIN outline = buildSegmentChain( aShape.m_FirstSegmentPtr );

    if( outline.PointCount() < 3 )
    {
        wxLogTrace( traceAllegroBuilder, "  Not enough points for polygon (%d)", outline.PointCount() );
        return polySet;
    }

    outline.SetClosed( true );
    polySet.AddOutline( outline );

    // Walk 0x34 KEEPOUT chain from m_Ptr4 for holes
    uint32_t holeKey = aShape.m_FirstKeepoutPtr;

    while( holeKey != 0 )
    {
        const BLOCK_BASE* holeBlock = m_brdDb.GetObjectByKey( holeKey );

        if( !holeBlock || holeBlock->GetBlockType() != 0x34 )
            break;

        const auto& keepout = static_cast<const BLOCK<BLK_0x34_KEEPOUT>&>( *holeBlock ).GetData();

        SHAPE_LINE_CHAIN holeOutline = buildSegmentChain( keepout.m_FirstSegmentPtr );

        if( holeOutline.PointCount() >= 3 )
        {
            holeOutline.SetClosed( true );
            polySet.AddHole( holeOutline );
        }

        holeKey = keepout.m_Next;
    }

    return polySet;
}


SHAPE_POLY_SET ALLEGRO::BOARD_BUILDER::tryBuildZoneShape( const BLOCK_BASE& aBlock )
{
    SHAPE_POLY_SET polySet;

    switch( aBlock.GetBlockType() )
    {
    case 0x0E:
    {
        const auto& rectData = BlockDataAs<BLK_0x0E_RECT>( aBlock );

        SHAPE_LINE_CHAIN chain( buildOutline( rectData ) );
        chain.SetClosed( true );

        polySet = SHAPE_POLY_SET( chain );
        break;
    }
    case 0x24:
    {
        const auto& rectData = BlockDataAs<BLK_0x24_RECT>( aBlock );

        SHAPE_LINE_CHAIN chain( buildOutline( rectData ) );
        chain.SetClosed( true );

        polySet = SHAPE_POLY_SET( chain );
        break;
    }
    case 0x28:
    {
        const auto& shapeData = BlockDataAs<BLK_0x28_SHAPE>( aBlock );
        polySet = shapeToPolySet( shapeData );
        break;
    }
    default:
        wxLogTrace( traceAllegroBuilder, "  Unhandled block type in tryBuildZoneShape: %#04x", aBlock.GetBlockType() );
    }

    return polySet;
}


static LSET getRuleAreaLayers( const LAYER_INFO& aLayerInfo, PCB_LAYER_ID aDefault )
{
    LSET layerSet{ aDefault };

    switch( aLayerInfo.m_Class )
    {
    case LAYER_INFO::CLASS::ROUTE_KEEPOUT:
    case LAYER_INFO::CLASS::VIA_KEEPOUT:
    case LAYER_INFO::CLASS::PACKAGE_KEEPOUT:
    {
        switch( aLayerInfo.m_Subclass )
        {
        case LAYER_INFO::SUBCLASS::KEEPOUT_ALL:
            layerSet = LSET::AllCuMask();
            break;
        case LAYER_INFO::SUBCLASS::KEEPOUT_TOP:
            layerSet = LSET{ F_Cu };
            break;
        case LAYER_INFO::SUBCLASS::KEEPOUT_BOTTOM:
            layerSet = LSET{ B_Cu };
            break;
        default:
            wxLogTrace( traceAllegroBuilder, "  Unhandled keepout layer subclass %#02x, using default layers",
                        aLayerInfo.m_Subclass );
        }
        break;
    }
    case LAYER_INFO::CLASS::ROUTE_KEEPIN:
    case LAYER_INFO::CLASS::PACKAGE_KEEPIN:
    {
        // This can be ALL, but can it be anything else?
        if( aLayerInfo.m_Subclass == LAYER_INFO::SUBCLASS::KEEPOUT_ALL )
            layerSet = LSET::AllCuMask();
        else
            wxLogTrace( traceAllegroBuilder, "  Unhandled keepin layer subclass %#02x, using default layers",
                        aLayerInfo.m_Subclass );
        break;
    }
    default:
        wxLogTrace( traceAllegroBuilder, "  Unhandled non-copper zone layer class %#02x, using default layers",
                    aLayerInfo.m_Class );
        break;
    }

    return layerSet;
}


std::unique_ptr<ZONE> BOARD_BUILDER::buildZone( const BLOCK_BASE& aBlock, int aNetcode )
{
    const LAYER_INFO layerInfo = expectLayerFromBlock( aBlock );

    bool isCopperZone = ( layerInfo.m_Class == LAYER_INFO::CLASS::ETCH
                          || layerInfo.m_Class == LAYER_INFO::CLASS::BOUNDARY );

    PCB_LAYER_ID layer = UNDEFINED_LAYER;

    if( isCopperZone )
    {
        // BOUNDARY shares the ETCH layer list, so resolve subclass via ETCH class
        if( layerInfo.m_Class == LAYER_INFO::CLASS::BOUNDARY )
        {
            LAYER_INFO etchLayer{};
            etchLayer.m_Class = LAYER_INFO::CLASS::ETCH;
            etchLayer.m_Subclass = layerInfo.m_Subclass;
            layer = getLayer( etchLayer );
        }
        else
        {
            layer = getLayer( layerInfo );
        }
    }
    else
    {
        layer = F_Cu;
    }

    if( isCopperZone && layer == UNDEFINED_LAYER )
    {
        wxLogTrace( traceAllegroBuilder, "  Skipping shape on layer %#02x:%#02x - unmapped copper layer",
                    layerInfo.m_Class, layerInfo.m_Subclass );
        return nullptr;
    }

    const SHAPE_POLY_SET zoneShape = tryBuildZoneShape( aBlock );

    if( zoneShape.OutlineCount() != 1 )
    {
        wxLogTrace( traceAllegroBuilder, "  Skipping zone with type %#04x, key %#010x - failed to build outline",
                    aBlock.GetBlockType(), aBlock.GetKey() );
        return nullptr;
    }

    auto zone = std::make_unique<ZONE>( &m_board );
    zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::NO_HATCH );

    if( isCopperZone )
    {
        zone->SetLayer( layer );
        zone->SetFillMode( ZONE_FILL_MODE::POLYGONS );
    }
    else
    {
        LSET layerSet = getRuleAreaLayers( layerInfo, Cmts_User );

        bool isRouteKeepout = ( layerInfo.m_Class == LAYER_INFO::CLASS::ROUTE_KEEPOUT );
        bool isViaKeepout = ( layerInfo.m_Class == LAYER_INFO::CLASS::VIA_KEEPOUT );
        bool isPackageKeepout = ( layerInfo.m_Class == LAYER_INFO::CLASS::PACKAGE_KEEPOUT );
        bool isRouteKeepin = ( layerInfo.m_Class == LAYER_INFO::CLASS::ROUTE_KEEPIN );
        bool isPackageKeepin = ( layerInfo.m_Class == LAYER_INFO::CLASS::PACKAGE_KEEPIN );

        zone->SetIsRuleArea( true );
        zone->SetLayerSet( layerSet );
        zone->SetDoNotAllowTracks( isRouteKeepout );
        zone->SetDoNotAllowVias( isViaKeepout );
        zone->SetDoNotAllowZoneFills( isRouteKeepout || isViaKeepout );
        zone->SetDoNotAllowPads( false );
        zone->SetDoNotAllowFootprints( isPackageKeepout );

        // Zones utedon't have native keepin functions, so we leave a note for the user here
        // Later, we could consider adding a custom DRC rule for this (or KiCad could add native keepin
        // zone support)
        if( isRouteKeepin )
            zone->SetZoneName( "Route Keepin" );
        else if( isPackageKeepin )
            zone->SetZoneName( "Package Keepin" );
    }

    // Set net code AFTER layer assignment. SetNetCode checks IsOnCopperLayer() and
    // forces net=0 if the zone isn't on a copper layer yet.
    zone->SetNetCode( aNetcode );

    for( const SHAPE_LINE_CHAIN& chain : zoneShape.CPolygon( 0 ) )
        zone->AddPolygon( chain );

    return zone;
}


int BOARD_BUILDER::resolveShapeNet( const BLK_0x28_SHAPE& aShape ) const
{
    // Follow pointer chain: BOUNDARY.Ptr7 -> 0x2C TABLE -> Ptr1 -> 0x37 -> m_Ptrs[0] -> 0x1B NET
    uint32_t ptr7Key = 0;

    if( aShape.m_Ptr7.has_value() )
        ptr7Key = aShape.m_Ptr7.value();
    else if( aShape.m_Ptr7_16x.has_value() )
        ptr7Key = aShape.m_Ptr7_16x.value();

    if( ptr7Key == 0 )
        return NETINFO_LIST::UNCONNECTED;

    const BLK_0x2C_TABLE* tbl = expectBlockByKey<BLK_0x2C_TABLE>( ptr7Key, 0x2C );

    if( !tbl )
        return NETINFO_LIST::UNCONNECTED;

    const BLK_0x37_PTR_ARRAY* ptrArray = expectBlockByKey<BLK_0x37_PTR_ARRAY>( tbl->m_Ptr1, 0x37 );

    if( !ptrArray || ptrArray->m_Count == 0 )
        return NETINFO_LIST::UNCONNECTED;

    uint32_t netKey = ptrArray->m_Ptrs[0];
    auto it = m_netCache.find( netKey );

    if( it != m_netCache.end() )
    {
        wxLogTrace( traceAllegroBuilder, "  Resolved BOUNDARY %#010x -> net '%s' (code %d)",
                    aShape.m_Key, it->second->GetNetname(), it->second->GetNetCode() );
        return it->second->GetNetCode();
    }

    wxLogTrace( traceAllegroBuilder, "  BOUNDARY %#010x: net key %#010x not in cache", aShape.m_Key, netKey );
    return NETINFO_LIST::UNCONNECTED;
}


void BOARD_BUILDER::createBoardText()
{
    const LL_WALKER textWalker( m_brdDb.m_Header->m_LL_0x03_0x30, m_brdDb );
    int textCount = 0;

    for( const BLOCK_BASE* block : textWalker )
    {
        if( block->GetBlockType() != 0x30 )
            continue;

        const auto& strWrapper =
                static_cast<const BLOCK<BLK_0x30_STR_WRAPPER>&>( *block ).GetData();

        std::unique_ptr<PCB_TEXT> text = buildPcbText( strWrapper, m_board );

        if( !text )
            continue;

        // If the text is referenced from a group, it's not board-level text,
        // and we'll pick up up while iterating the group elsewhere.
        if( strWrapper.GetGroupPtr() != 0 )
        {
            // In a group
            continue;
        }

        wxLogTrace( traceAllegroBuilder, "  Board text '%s' on layer %s at (%d, %d)",
                    text->GetText(), m_board.GetLayerName( text->GetLayer() ),
                    text->GetPosition().x, text->GetPosition().y );

        m_board.Add( text.release(), ADD_MODE::APPEND );
        textCount++;
    }

    wxLogTrace( traceAllegroBuilder, "Created %d board-level text objects", textCount );
}


void BOARD_BUILDER::createZones()
{
    wxLogTrace( traceAllegroBuilder, "Creating zones from m_LL_Shapes and m_LL_0x24_0x28" );

    int boundaryCount = 0;
    int mergedCount = 0;
    int keepoutCount = 0;

    std::vector<std::unique_ptr<ZONE>> boundaryZones;

    // Walk m_LL_Shapes to find BOUNDARY shapes (zone outlines).
    // BOUNDARY shapes use class 0x15 with copper layer subclass indices.
    const LL_WALKER shapeWalker( m_brdDb.m_Header->m_LL_Shapes, m_brdDb );

    for( const BLOCK_BASE* block : shapeWalker )
    {
        if( block->GetBlockType() != 0x28 )
            continue;

        const BLK_0x28_SHAPE& shapeData =
                static_cast<const BLOCK<BLK_0x28_SHAPE>&>( *block ).GetData();

        if( shapeData.m_Layer.m_Class != LAYER_INFO::CLASS::BOUNDARY )
            continue;

        int                   netCode = resolveShapeNet( shapeData );
        std::unique_ptr<ZONE> zone = buildZone( *block, netCode );

        if( zone )
        {
            wxLogTrace( traceAllegroBuilder, "  Zone %#010x net=%d layer=%s (subclass=%#04x)",
                        shapeData.m_Key, netCode,
                        m_board.GetLayerName( zone->GetFirstLayer() ),
                        shapeData.m_Layer.m_Subclass );

            zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
            boundaryZones.push_back( std::move( zone ) );
            boundaryCount++;
        }
    }

    // Merge zones with identical polygons and same net into multi-layer zones.
    // Allegro often defines the same zone outline on multiple copper layers (e.g.
    // a ground pour spanning all layers). KiCad represents this as a single zone
    // with multiple fill layers.
    std::vector<bool> merged( boundaryZones.size(), false );

    for( size_t i = 0; i < boundaryZones.size(); i++ )
    {
        if( merged[i] )
            continue;

        ZONE*                          primary = boundaryZones[i].get();
        const SHAPE_POLY_SET::POLYGON& primaryPolygon = primary->Outline()->CPolygon( 0 );
        LSET                           layers = primary->GetLayerSet();

        for( size_t j = i + 1; j < boundaryZones.size(); j++ )
        {
            if( merged[j] )
                continue;

            ZONE* candidate = boundaryZones[j].get();

            if( candidate->GetNetCode() != primary->GetNetCode() )
                continue;

            const SHAPE_POLY_SET::POLYGON& candidatePolygon = candidate->Outline()->CPolygon( 0 );

            if( primaryPolygon.size() != candidatePolygon.size() )
                continue;

            bool polygonsDiffer = false;

            for( size_t lineChainId = 0; lineChainId < primaryPolygon.size(); lineChainId++ )
            {
                const SHAPE_LINE_CHAIN& primaryChain = primaryPolygon[lineChainId];
                const SHAPE_LINE_CHAIN& candidateChain = candidatePolygon[lineChainId];

                if( primaryChain.PointCount() != candidateChain.PointCount()
                    || primaryChain.BBox() != candidateChain.BBox()
                    || !primaryChain.CompareGeometry( candidateChain ) )
                {
                    polygonsDiffer = true;
                    break;
                }
            }

            if( !polygonsDiffer )
            {
                layers |= candidate->GetLayerSet();
                merged[j] = true;
                mergedCount++;

                wxLogTrace( traceAllegroBuilder, "  Merging zone on %s into zone on %s (net %d)",
                            m_board.GetLayerName( candidate->GetFirstLayer() ),
                            m_board.GetLayerName( primary->GetFirstLayer() ), primary->GetNetCode() );
            }
        }

        if( layers != primary->GetLayerSet() )
            primary->SetLayerSet( layers );

        m_board.Add( boundaryZones[i].release(), ADD_MODE::APPEND );
    }

    if( mergedCount > 0 )
    {
        wxLogTrace( traceAllegroBuilder,
                    "  Merged %d zones into multi-layer zones (%d zones remain from %d)",
                    mergedCount, boundaryCount - mergedCount, boundaryCount );
    }

    // Walk m_LL_0x24_0x28 for keepout/in shapes
    const LL_WALKER keepoutWalker( m_brdDb.m_Header->m_LL_0x24_0x28, m_brdDb );

    for( const BLOCK_BASE* block : keepoutWalker )
    {
        std::unique_ptr<ZONE> zone;

        switch( block->GetBlockType() )
        {
        case 0x24:
        {
            const BLK_0x24_RECT& rectData = static_cast<const BLOCK<BLK_0x24_RECT>&>( *block ).GetData();

            if( !layerIsZone( rectData.m_Layer ) )
                continue;

            wxLogTrace( traceAllegroBuilder, "  Processing %s rect %#010x", layerInfoDisplayName( rectData.m_Layer ),
                        rectData.m_Key );

            zone = buildZone( *block, NETINFO_LIST::UNCONNECTED );
            break;
        }
        case 0x28:
        {
            const BLK_0x28_SHAPE& shapeData = static_cast<const BLOCK<BLK_0x28_SHAPE>&>( *block ).GetData();

            if( !layerIsZone( shapeData.m_Layer ) )
                continue;

            wxLogTrace( traceAllegroBuilder, "  Processing %s shape %#010x", layerInfoDisplayName( shapeData.m_Layer ),
                        shapeData.m_Key );

            zone = buildZone( *block, NETINFO_LIST::UNCONNECTED );
            break;
        }
        default:
            break;
        }

        if( zone )
        {
            m_board.Add( zone.release(), ADD_MODE::APPEND );
            keepoutCount++;
        }
    }

    wxLogTrace( traceAllegroBuilder,
                "Created %d zone outlines (%d merged), %d keepout areas",
                boundaryCount - mergedCount, mergedCount, keepoutCount );
}


void BOARD_BUILDER::createTables()
{
    wxLogTrace( traceAllegroBuilder, "Creating tables from m_LL_0x2C" );

    const LL_WALKER tableWalker( m_brdDb.m_Header->m_LL_0x2C, m_brdDb );
    for( const BLOCK_BASE* block : tableWalker )
    {
        if( block->GetBlockType() != 0x2C )
            continue;

        const BLK_0x2C_TABLE& tableData = static_cast<const BLOCK<BLK_0x2C_TABLE>&>( *block ).GetData();

        if( tableData.m_SubType != BLK_0x2C_TABLE::SUBTYPE::SUBTYPE_GRAPHICAL_GROUP )
        {
            // 0x2c tables can have lots of subtypes. Only 0x110 seems useful to iterate in this way for now.
            continue;
        }

        const wxString& tableName = m_brdDb.GetString( tableData.m_StringPtr );

        std::vector<std::unique_ptr<BOARD_ITEM>> newItems;

        LL_WALKER keyTableWalker{ tableData.m_Ptr1, block->GetKey(), m_brdDb };

        for( const BLOCK_BASE* keyTable : keyTableWalker )
        {
            wxLogTrace( traceAllegroBuilder, "  Table '%s' (key %#010x, table block key %#010x)", tableName,
                        tableData.m_Key, tableData.m_Ptr1 );

            if( !keyTable )
            {
                wxLogTrace( traceAllegroBuilder, "    Key table pointer %#010x is invalid", tableData.m_Ptr1 );
                continue;
            }

            switch( keyTable->GetBlockType() )
            {
            case 0x37:
            {
                const BLK_0x37_PTR_ARRAY& ptrArray =
                        static_cast<const BLOCK<BLK_0x37_PTR_ARRAY>&>( *keyTable ).GetData();

                uint32_t count = std::min( ptrArray.m_Count, static_cast<uint32_t>( ptrArray.m_Ptrs.size() ) );

                wxLogTrace( traceAllegroBuilder, "    Pointer array with %zu entries", static_cast<size_t>( count ) );

                for( uint32_t ptrIndex = 0; ptrIndex < count; ptrIndex++ )
                {
                    uint32_t ptrKey = ptrArray.m_Ptrs[ptrIndex];

                    if( ptrKey == 0 )
                        continue;

                    const BLOCK_BASE* entryBlock = m_brdDb.GetObjectByKey( ptrKey );

                    if( !entryBlock )
                    {
                        wxLogTrace( traceAllegroBuilder, "      Entry pointer %#010x is invalid", ptrKey );
                        continue;
                    }

                    for( std::unique_ptr<BOARD_ITEM>& newItem : buildGraphicItems( *entryBlock, m_board ) )
                    {
                        newItems.push_back( std::move( newItem ) );
                    }
                }

                break;
            }
            case 0x3c:
            {
                const BLK_0x3C_KEY_LIST& keyList = static_cast<const BLOCK<BLK_0x3C_KEY_LIST>&>( *keyTable ).GetData();

                wxLogTrace( traceAllegroBuilder, "    Key list with %zu entries",
                            static_cast<size_t>( keyList.m_NumEntries ) );
                break;
            }
            default:
            {
                wxLogTrace( traceAllegroBuilder, "    Table has unhandled key table type %#04x",
                            keyTable->GetBlockType() );
                break;
            }
            }
        }

        if( newItems.size() > 0 )
        {
            wxLogTrace( traceAllegroBuilder, "  Creating group '%s' with %zu items", tableName, newItems.size() );

            std::unique_ptr<PCB_GROUP> group = std::make_unique<PCB_GROUP>( &m_board );
            group->SetName( tableName );

            std::vector<BOARD_ITEM*> bulkAddedItems;
            bulkAddedItems.reserve( newItems.size() + 1 );
            bulkAddedItems.push_back( group.get() );

            for( std::unique_ptr<BOARD_ITEM>& newItem : newItems )
            {
                m_board.Add( newItem.get(), ADD_MODE::BULK_APPEND );
                bulkAddedItems.push_back( newItem.get() );
                group->AddItem( newItem.release() );
            }

            m_board.Add( group.release(), ADD_MODE::BULK_APPEND );
            m_board.FinalizeBulkAdd( bulkAddedItems );
        }
    }
}


void BOARD_BUILDER::applyZoneFills()
{
    if( m_zoneFillShapes.empty() )
        return;

    PROF_TIMER fillTimer;

    wxLogTrace( traceAllegroBuilder, "Applying zone fill polygons from %zu collected fills",
                m_zoneFillShapes.size() );

    int fillCount = 0;
    int totalHoles = 0;
    std::vector<bool> matched( m_zoneFillShapes.size(), false );

    // Index fills by (layer, netCode) to avoid scanning all fills for each zone
    std::unordered_map<uint64_t, std::vector<size_t>> fillIndex;

    for( size_t i = 0; i < m_zoneFillShapes.size(); i++ )
    {
        const ZoneFillEntry& fill = m_zoneFillShapes[i];
        uint64_t key = ( static_cast<uint64_t>( fill.layer ) << 32 )
                       | static_cast<uint32_t>( fill.netCode );
        fillIndex[key].push_back( i );
    }

    for( ZONE* zone : m_board.Zones() )
    {
        if( zone->GetIsRuleArea() || zone->GetNetCode() == NETINFO_LIST::UNCONNECTED )
            continue;

        bool hasFill = false;

        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( !IsCopperLayer( layer ) )
                continue;

            uint64_t key = ( static_cast<uint64_t>( layer ) << 32 )
                           | static_cast<uint32_t>( zone->GetNetCode() );
            auto indexIt = fillIndex.find( key );

            if( indexIt == fillIndex.end() )
                continue;

            SHAPE_POLY_SET combinedFill;
            SHAPE_POLY_SET zoneOutline = *zone->Outline();
            zoneOutline.ClearArcs();

            for( size_t i : indexIt->second )
            {
                const ZoneFillEntry& fill = m_zoneFillShapes[i];
                SHAPE_POLY_SET       fillPolySet = shapeToPolySet( *fill.shape );

                if( fillPolySet.VertexCount() < 3 )
                    continue;

                fillPolySet.ClearArcs();
                BOX2I fillBbox = fillPolySet.BBox();
                BOX2I zoneBbox = zone->GetBoundingBox();

                // Etch shapes can be slightly outside the boundary
                const int c_epsilon = pcbIUScale.MilsToIU( 1.0 );

                if( !zoneBbox.GetInflated( c_epsilon ).Contains( fillBbox ) )
                    continue;

                // Check that the fill is approximately fully contained within the zone outline
                SHAPE_POLY_SET fillCut( fillPolySet );

                fillCut.BooleanSubtract( zoneOutline );
                fillCut.Deflate( c_epsilon, CORNER_STRATEGY::ALLOW_ACUTE_CORNERS, ARC_HIGH_DEF );

                if( fillCut.Area() > 0 )
                    continue;

                combinedFill.BooleanAdd( fillPolySet );

                matched[i] = true;
            }

            if( combinedFill.OutlineCount() > 0 )
            {
                // Make sure the fills stay within the zone outline.
                combinedFill.BooleanIntersection( zoneOutline );

                // Skip Clipper2 Simplify because we've already done a boolean operation.
                if( combinedFill.HasHoles() )
                    combinedFill.Fracture( /* aSimplify */ false );

                zone->SetFilledPolysList( layer, combinedFill );
                hasFill = true;
                fillCount++;
            }
        }

        if( hasFill )
        {
            zone->SetIsFilled( true );
            zone->SetNeedRefill( false );
        }
    }

    wxLogTrace( traceAllegroPerf, wxT( "    applyZoneFills matched loop: %.3f ms (%d fills, %d holes)" ), //format:allow
                fillTimer.msecs( true ), fillCount, totalHoles );

    // Unmatched ETCH shapes are either standalone copper polygons or dynamic copper
    // (teardrops/fillets). On V172+ boards, m_Unknown2 bit 12 (0x1000) marks auto-generated
    // dynamic copper that maps to KiCad teardrop zones. Shapes without this flag are genuine
    // standalone copper imported as filled PCB_SHAPE.
    int copperShapeCount = 0;
    int teardropCount = 0;

    for( size_t i = 0; i < m_zoneFillShapes.size(); i++ )
    {
        if( matched[i] )
            continue;

        const ZoneFillEntry& fill = m_zoneFillShapes[i];

        SHAPE_LINE_CHAIN outline = buildOutline( *fill.shape );

        if( outline.PointCount() < 3 )
            continue;

        outline.SetClosed( true );
        outline.ClearArcs();

        SHAPE_POLY_SET polySet;
        polySet.AddOutline( outline );

        // Walk 0x34 KEEPOUT chain for clearance holes
        uint32_t holeKey = fill.shape->m_FirstKeepoutPtr;

        while( holeKey != 0 )
        {
            const BLOCK_BASE* holeBlock = m_brdDb.GetObjectByKey( holeKey );

            if( !holeBlock || holeBlock->GetBlockType() != 0x34 )
                break;

            const auto& keepout =
                    static_cast<const BLOCK<BLK_0x34_KEEPOUT>&>( *holeBlock ).GetData();

            SHAPE_LINE_CHAIN holeOutline = buildSegmentChain( keepout.m_FirstSegmentPtr );

            if( holeOutline.PointCount() >= 3 )
            {
                holeOutline.SetClosed( true );
                holeOutline.ClearArcs();
                polySet.AddHole( holeOutline );
            }

            holeKey = keepout.m_Next;
        }

        polySet.Simplify();

        for( const SHAPE_POLY_SET::POLYGON& poly : polySet.CPolygons() )
        {
            SHAPE_POLY_SET fractured( poly );
            fractured.Fracture( /* aSimplify */ false );

            const bool isDynCopperShape = ( fill.shape->m_Unknown2.value_or( 0 ) & 0x1000 ) != 0;

            if( isDynCopperShape )
            {
                auto zone = std::make_unique<ZONE>( &m_board );

                zone->SetTeardropAreaType( TEARDROP_TYPE::TD_VIAPAD );
                zone->SetLayer( fill.layer );
                zone->SetNetCode( fill.netCode );
                zone->SetLocalClearance( 0 );
                zone->SetPadConnection( ZONE_CONNECTION::FULL );
                zone->SetIslandRemovalMode( ISLAND_REMOVAL_MODE::NEVER );
                zone->SetHatchStyle( ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER );

                for( const SHAPE_LINE_CHAIN& chain : poly )
                    zone->AddPolygon( chain );

                zone->SetFilledPolysList( fill.layer, fractured );
                zone->SetIsFilled( true );
                zone->SetNeedRefill( false );
                zone->CalculateFilledArea();

                m_board.Add( zone.release(), ADD_MODE::APPEND );
                teardropCount++;
            }
            else
            {
                auto shape = std::make_unique<PCB_SHAPE>( &m_board, SHAPE_T::POLY );
                shape->SetPolyShape( fractured );
                shape->SetFilled( true );
                shape->SetLayer( fill.layer );
                shape->SetNetCode( fill.netCode );
                shape->SetStroke( STROKE_PARAMS( 0, LINE_STYLE::SOLID ) );

                m_board.Add( shape.release(), ADD_MODE::APPEND );
                copperShapeCount++;
            }
        }
    }

    wxLogTrace( traceAllegroPerf, wxT( "    applyZoneFills unmatched loop: %.3f ms (%d shapes, %d teardrops)" ), //format:allow
                fillTimer.msecs( true ), copperShapeCount, teardropCount );

    wxLogTrace( traceAllegroBuilder,
                "Applied fills to %d zone/layer pairs (%d clearance holes), "
                "created %d standalone copper shapes, %d teardrop zones",
                fillCount, totalHoles, copperShapeCount, teardropCount );
}


void BOARD_BUILDER::enablePadTeardrops()
{
    std::unordered_map<int, std::vector<ZONE*>> teardropsByNet;

    for( ZONE* zone : m_board.Zones() )
    {
        if( zone->IsTeardropArea() )
            teardropsByNet[zone->GetNetCode()].push_back( zone );
    }

    if( teardropsByNet.empty() )
        return;

    int padCount = 0;
    int viaCount = 0;

    for( FOOTPRINT* fp : m_board.Footprints() )
    {
        for( PAD* pad : fp->Pads() )
        {
            auto it = teardropsByNet.find( pad->GetNetCode() );

            if( it == teardropsByNet.end() )
                continue;

            for( ZONE* tdZone : it->second )
            {
                if( !pad->IsOnLayer( tdZone->GetLayer() ) )
                    continue;

                if( tdZone->Outline()->Contains( pad->GetPosition() ) )
                {
                    pad->SetTeardropsEnabled( true );
                    padCount++;
                    break;
                }
            }
        }
    }

    for( PCB_TRACK* track : m_board.Tracks() )
    {
        if( track->Type() != PCB_VIA_T )
            continue;

        PCB_VIA* via = static_cast<PCB_VIA*>( track );
        auto it = teardropsByNet.find( via->GetNetCode() );

        if( it == teardropsByNet.end() )
            continue;

        for( ZONE* tdZone : it->second )
        {
            if( !via->IsOnLayer( tdZone->GetLayer() ) )
                continue;

            if( tdZone->Outline()->Contains( via->GetPosition() ) )
            {
                via->SetTeardropsEnabled( true );
                viaCount++;
                break;
            }
        }
    }

    wxLogTrace( traceAllegroBuilder, "Enabled teardrops on %d pads and %d vias", padCount, viaCount );
}


bool BOARD_BUILDER::BuildBoard()
{
    wxLogTrace( traceAllegroBuilder, "Starting BuildBoard() - Phase 2 of Allegro import" );
    wxLogTrace( traceAllegroBuilder, "  Format version: %d (V172+ = %s)",
                static_cast<int>( m_brdDb.m_FmtVer ),
                ( m_brdDb.m_FmtVer >= FMT_VER::V_172 ) ? "yes" : "no" );
    wxLogTrace( traceAllegroBuilder, "  Allegro version string: %.60s",
                m_brdDb.m_Header->m_AllegroVersion.data() );

    if( m_progressReporter )
    {
        m_progressReporter->AddPhases( 4 );
        m_progressReporter->AdvancePhase( _( "Constructing caches" ) );
        m_progressReporter->KeepRefreshing();
    }

    PROF_TIMER buildTimer;

    wxLogTrace( traceAllegroBuilder, "Caching font definitions and setting up layers" );
    cacheFontDefs();
    wxLogTrace( traceAllegroPerf, wxT( "  cacheFontDefs: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    setupLayers();
    wxLogTrace( traceAllegroPerf, wxT( "  setupLayers: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Creating nets" ) );
        m_progressReporter->KeepRefreshing();
    }

    createNets();
    wxLogTrace( traceAllegroPerf, wxT( "  createNets: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Creating tracks" ) );
        m_progressReporter->KeepRefreshing();
    }

    createTracks();
    wxLogTrace( traceAllegroPerf, wxT( "  createTracks: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    if( m_progressReporter )
        m_progressReporter->KeepRefreshing();

    createBoardShapes();
    wxLogTrace( traceAllegroPerf, wxT( "  createBoardShapes: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    createBoardText();
    wxLogTrace( traceAllegroPerf, wxT( "  createBoardText: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    createZones();
    wxLogTrace( traceAllegroPerf, wxT( "  createZones: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    createTables();
    wxLogTrace( traceAllegroPerf, wxT( "  createTables: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    if( m_progressReporter )
        m_progressReporter->KeepRefreshing();

    applyZoneFills();
    wxLogTrace( traceAllegroPerf, wxT( "  applyZoneFills: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    applyConstraintSets();
    wxLogTrace( traceAllegroPerf, wxT( "  applyConstraintSets: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    applyNetConstraints();
    wxLogTrace( traceAllegroPerf, wxT( "  applyNetConstraints: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    applyMatchGroups();
    wxLogTrace( traceAllegroPerf, wxT( "  applyMatchGroups: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase( _( "Converting footprints" ) );
        m_progressReporter->KeepRefreshing();
    }

    const LL_WALKER          fpWalker( m_brdDb.m_Header->m_LL_0x2B, m_brdDb );
    std::vector<BOARD_ITEM*> bulkAddedItems;

    auto lastRefresh = std::chrono::steady_clock::now();

    for( const BLOCK_BASE* fpContainer : fpWalker )
    {
        if( fpContainer->GetBlockType() == 0x2B )
        {
            const BLK_0x2B_FOOTPRINT_DEF& fpBlock =
                    static_cast<const BLOCK<BLK_0x2B_FOOTPRINT_DEF>&>( *fpContainer ).GetData();

            const LL_WALKER instWalker( fpBlock.m_FirstInstPtr, fpBlock.m_Key, m_brdDb );

            for( const BLOCK_BASE* instBlock : instWalker )
            {
                if( instBlock->GetBlockType() != 0x2D )
                {
                    m_reporter.Report(
                            wxString::Format( "Unexpected object of type %#04x found in footprint %#010x",
                                              instBlock->GetBlockType(), fpBlock.m_Key ),
                            RPT_SEVERITY_ERROR );
                }
                else
                {
                    const auto& inst =
                            static_cast<const BLOCK<BLK_0x2D_FOOTPRINT_INST>&>( *instBlock ).GetData();

                    std::unique_ptr<FOOTPRINT> fp = buildFootprint( inst );

                    if( fp )
                    {
                        bulkAddedItems.push_back( fp.get() );
                        m_board.Add( fp.release(), ADD_MODE::BULK_APPEND, true );
                    }
                    else
                    {
                        m_reporter.Report(
                                wxString::Format( "Failed to construct footprint for 0x2D key %#010x",
                                                  inst.m_Key ),
                                RPT_SEVERITY_ERROR );
                    }
                }

                if( m_progressReporter )
                {
                    auto now = std::chrono::steady_clock::now();

                    if( now - lastRefresh >= std::chrono::milliseconds( 100 ) )
                    {
                        m_progressReporter->KeepRefreshing();
                        lastRefresh = now;
                    }
                }
            }
        }
    }

    wxLogTrace( traceAllegroPerf, wxT( "  convertFootprints (%zu footprints): %.3f ms" ), //format:allow
                bulkAddedItems.size(), buildTimer.msecs( true ) );

    if( !bulkAddedItems.empty() )
        m_board.FinalizeBulkAdd( bulkAddedItems );

    wxLogTrace( traceAllegroPerf, wxT( "  FinalizeBulkAdd: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow
    wxLogTrace( traceAllegroBuilder, "Converted %zu footprints", bulkAddedItems.size() );

    enablePadTeardrops();
    wxLogTrace( traceAllegroPerf, wxT( "  enablePadTeardrops: %.3f ms" ), buildTimer.msecs( true ) ); //format:allow
    wxLogTrace( traceAllegroPerf, wxT( "  Phase 2 total: %.3f ms" ), buildTimer.msecs() ); //format:allow

    wxLogTrace( traceAllegroBuilder, "Board construction completed successfully" );
    return true;
}

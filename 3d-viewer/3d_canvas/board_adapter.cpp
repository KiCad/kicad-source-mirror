/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2023 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2023 CERN
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

#include <gal/3d/camera.h>
#include "board_adapter.h"
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <3d_rendering/raytracing/shapes2D/polygon_2d.h>
#include <board.h>
#include <dialogs/dialog_color_picker.h>
#include <layer_range.h>
#include <3d_math.h>
#include "3d_fastmath.h"
#include <geometry/geometry_utils.h>
#include <lset.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <wx/log.h>
#include <pcbnew_settings.h>
#include <advanced_config.h>


#define DEFAULT_BOARD_THICKNESS pcbIUScale.mmToIU( 1.6 )
#define DEFAULT_COPPER_THICKNESS pcbIUScale.mmToIU( 0.035 ) // for 35 um
// The solder mask layer (and silkscreen) thickness
#define DEFAULT_TECH_LAYER_THICKNESS pcbIUScale.mmToIU( 0.025 )
// The solder paste thickness is chosen bigger than the solder mask layer
// to be sure is covers the mask when overlapping.
#define SOLDERPASTE_LAYER_THICKNESS pcbIUScale.mmToIU( 0.04 )


CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_SilkColors;
CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_MaskColors;
CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_PasteColors;
CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_FinishColors;
CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_BoardColors;

KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultBackgroundTop;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultBackgroundBot;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultSilkscreen;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultSolderMask;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultSolderPaste;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultSurfaceFinish;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultBoardBody;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultComments;
KIGFX::COLOR4D       BOARD_ADAPTER::g_DefaultECOs;

// To be used in Raytracing render to create bevels on layer items
float                g_BevelThickness3DU = 0.0f;

static bool          g_ColorsLoaded = false;

/**
 *  Trace mask used to enable or disable the trace output of this class.
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_EDA_CINFO3D_VISU".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 *
 * @ingroup trace_env_vars
 */
const wxChar *BOARD_ADAPTER::m_logTrace = wxT( "KI_TRACE_EDA_CINFO3D_VISU" );


BOARD_ADAPTER::BOARD_ADAPTER() :
        m_Cfg( nullptr ),
        m_IsBoardView( true ),
        m_MousewheelPanning( true ),
        m_IsPreviewer( false ),
        m_board( nullptr ),
        m_3dModelManager( nullptr ),
        m_layerZcoordTop(),
        m_layerZcoordBottom()
{
    wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::BOARD_ADAPTER" ) );

    m_boardPos = VECTOR2I();
    m_boardSize = VECTOR2I();
    m_boardCenter = SFVEC3F( 0.0f );

    m_boardBoundingBox.Reset();

    m_TH_IDs.Clear();
    m_TH_ODs.Clear();
    m_viaAnnuli.Clear();

    m_copperLayersCount = 2;

    m_biuTo3Dunits = 1.0;
    m_boardBodyThickness3DU        = DEFAULT_BOARD_THICKNESS      * m_biuTo3Dunits;
    m_frontCopperThickness3DU      = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_backCopperThickness3DU       = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU   = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_frontMaskThickness3DU        = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_backMaskThickness3DU         = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_solderPasteLayerThickness3DU = SOLDERPASTE_LAYER_THICKNESS  * m_biuTo3Dunits;

    m_trackCount = 0;
    m_viaCount = 0;
    m_averageViaHoleDiameter = 0.0f;
    m_holeCount = 0;
    m_averageHoleDiameter = 0.0f;
    m_averageTrackWidth = 0.0f;

    m_BgColorBot         = SFVEC4F( 0.4, 0.4, 0.5, 1.0 );
    m_BgColorTop         = SFVEC4F( 0.8, 0.8, 0.9, 1.0 );
    m_BoardBodyColor     = SFVEC4F( 0.4, 0.4, 0.5, 0.9 );
    m_SolderMaskColorTop = SFVEC4F( 0.1, 0.2, 0.1, 0.83 );
    m_SolderMaskColorBot = SFVEC4F( 0.1, 0.2, 0.1, 0.83 );
    m_SolderPasteColor   = SFVEC4F( 0.4, 0.4, 0.4, 1.0 );
    m_SilkScreenColorTop = SFVEC4F( 0.9, 0.9, 0.9, 1.0 );
    m_SilkScreenColorBot = SFVEC4F( 0.9, 0.9, 0.9, 1.0 );
    m_CopperColor        = SFVEC4F( 0.75, 0.61, 0.23, 1.0 );
    m_UserDrawingsColor  = SFVEC4F( 0.85, 0.85, 0.85,  1.0 );
    m_UserCommentsColor  = SFVEC4F( 0.85, 0.85, 0.85,  1.0 );
    m_ECO1Color          = SFVEC4F( 0.70, 0.10, 0.10,  1.0 );
    m_ECO2Color          = SFVEC4F( 0.70, 0.10, 0.10,  1.0 );

    for( int ii = 0; ii < 45; ++ii )
        m_UserDefinedLayerColor[ii] = SFVEC4F( 0.70, 0.10, 0.10, 1.0 );

    m_platedPadsFront = nullptr;
    m_platedPadsBack = nullptr;
    m_offboardPadsFront = nullptr;
    m_offboardPadsBack = nullptr;

    m_frontPlatedCopperPolys = nullptr;
    m_backPlatedCopperPolys = nullptr;

    ReloadColorSettings();

    if( !g_ColorsLoaded )
    {
#define ADD_COLOR( list, r, g, b, a, name ) \
    list.emplace_back( r/255.0, g/255.0, b/255.0, a, name )

        ADD_COLOR( g_SilkColors, 245, 245, 245, 1.0, NotSpecifiedPrm() ); // White
        ADD_COLOR( g_SilkColors,  20,  51,  36, 1.0, wxT( "Green" ) );
        ADD_COLOR( g_SilkColors, 181,  19,  21, 1.0, wxT( "Red" ) );
        ADD_COLOR( g_SilkColors,   2,  59, 162, 1.0, wxT( "Blue" ) );
        ADD_COLOR( g_SilkColors,  11,  11,  11, 1.0, wxT( "Black" ) );
        ADD_COLOR( g_SilkColors, 245, 245, 245, 1.0, wxT( "White" ) );
        ADD_COLOR( g_SilkColors,  32,   2,  53, 1.0, wxT( "Purple" ) );
        ADD_COLOR( g_SilkColors, 194,  195,  0, 1.0, wxT( "Yellow" ) );

        ADD_COLOR( g_MaskColors,  20,  51,  36, 0.83, NotSpecifiedPrm() ); // Green
        ADD_COLOR( g_MaskColors,  20,  51,  36, 0.83, wxT( "Green" ) );
        ADD_COLOR( g_MaskColors,  91, 168,  12, 0.83, wxT( "Light Green" ) );
        ADD_COLOR( g_MaskColors,  13, 104,  11, 0.83, wxT( "Saturated Green" ) );
        ADD_COLOR( g_MaskColors, 181,  19,  21, 0.83, wxT( "Red" ) );
        ADD_COLOR( g_MaskColors, 210,  40,  14, 0.83, wxT( "Light Red" ) );
        ADD_COLOR( g_MaskColors, 239,  53,  41, 0.83, wxT( "Red/Orange" ) );
        ADD_COLOR( g_MaskColors,   2,  59, 162, 0.83, wxT( "Blue" ) );
        ADD_COLOR( g_MaskColors,  54,  79, 116, 0.83, wxT( "Light Blue 1" ) );
        ADD_COLOR( g_MaskColors,  61,  85, 130, 0.83, wxT( "Light Blue 2" ) );
        ADD_COLOR( g_MaskColors,  21,  70,  80, 0.83, wxT( "Green/Blue" ) );
        ADD_COLOR( g_MaskColors,  11,  11,  11, 0.83, wxT( "Black" ) );
        ADD_COLOR( g_MaskColors, 245, 245, 245, 0.83, wxT( "White" ) );
        ADD_COLOR( g_MaskColors,  32,   2,  53, 0.83, wxT( "Purple" ) );
        ADD_COLOR( g_MaskColors, 119,  31,  91, 0.83, wxT( "Light Purple" ) );
        ADD_COLOR( g_MaskColors, 194,  195,  0, 0.83, wxT( "Yellow" ) );

        ADD_COLOR( g_PasteColors, 128, 128, 128, 1.0, wxT( "Grey" ) );
        ADD_COLOR( g_PasteColors,  90,  90,  90, 1.0, wxT( "Dark Grey" ) );
        ADD_COLOR( g_PasteColors, 213, 213, 213, 1.0, wxT( "Silver" ) );

        ADD_COLOR( g_FinishColors, 184, 115,  50, 1.0, wxT( "Copper" ) );
        ADD_COLOR( g_FinishColors, 178, 156,   0, 1.0, wxT( "Gold" ) );
        ADD_COLOR( g_FinishColors, 213, 213, 213, 1.0, wxT( "Silver" ) );
        ADD_COLOR( g_FinishColors, 160, 160, 160, 1.0, wxT( "Tin" ) );

        ADD_COLOR( g_BoardColors,  51,  43,  22, 0.83, wxT( "FR4 natural, dark" ) );
        ADD_COLOR( g_BoardColors, 109, 116,  75, 0.83, wxT( "FR4 natural" ) );
        ADD_COLOR( g_BoardColors, 252, 252, 250, 0.90, wxT( "PTFE natural" ) );
        ADD_COLOR( g_BoardColors, 205, 130,   0, 0.68, wxT( "Polyimide" ) );
        ADD_COLOR( g_BoardColors,  92,  17,   6, 0.90, wxT( "Phenolic natural" ) );
        ADD_COLOR( g_BoardColors, 146,  99,  47, 0.83, wxT( "Brown 1" ) );
        ADD_COLOR( g_BoardColors, 160, 123,  54, 0.83, wxT( "Brown 2" ) );
        ADD_COLOR( g_BoardColors, 146,  99,  47, 0.83, wxT( "Brown 3" ) );
        ADD_COLOR( g_BoardColors, 213, 213, 213,  1.0, wxT( "Aluminum" ) );

        g_DefaultBackgroundTop = COLOR4D(  0.80, 0.80, 0.90, 1.0 );
        g_DefaultBackgroundBot = COLOR4D(  0.40, 0.40, 0.50, 1.0 );

        g_DefaultSilkscreen =    COLOR4D( 0.94, 0.94, 0.94,  1.0 );
        g_DefaultSolderMask =    COLOR4D( 0.08, 0.20, 0.14, 0.83 );
        g_DefaultSolderPaste =   COLOR4D( 0.50, 0.50, 0.50,  1.0 );
        g_DefaultSurfaceFinish = COLOR4D( 0.75, 0.61, 0.23,  1.0 );
        g_DefaultBoardBody =     COLOR4D( 0.43, 0.45, 0.30, 0.90 );

        g_DefaultComments =      COLOR4D( 0.85, 0.85, 0.85,  1.0 );
        g_DefaultECOs =          COLOR4D( 0.70, 0.10, 0.10,  1.0 );

        g_ColorsLoaded = true;
    }
#undef ADD_COLOR
}


BOARD_ADAPTER::~BOARD_ADAPTER()
{
    destroyLayers();
}


void BOARD_ADAPTER::ReloadColorSettings() noexcept
{
    PCBNEW_SETTINGS* cfg = GetAppSettings<PCBNEW_SETTINGS>( "pcbnew" );
    COLOR_SETTINGS*  cs = ::GetColorSettings( cfg ? cfg->m_ColorTheme : DEFAULT_THEME );

    for( int layer = F_Cu; layer < PCB_LAYER_ID_COUNT; ++layer )
        m_BoardEditorColors[ layer ] = cs->GetColor( layer );
}


bool BOARD_ADAPTER::Is3dLayerEnabled( PCB_LAYER_ID aLayer,
                                      const std::bitset<LAYER_3D_END>& aVisibilityFlags ) const
{
    wxASSERT( aLayer < PCB_LAYER_ID_COUNT );

    if( m_board && !m_board->IsLayerEnabled( aLayer ) )
        return false;

    switch( aLayer )
    {
    case B_Cu:      return aVisibilityFlags.test( LAYER_3D_COPPER_BOTTOM );
    case F_Cu:      return aVisibilityFlags.test( LAYER_3D_COPPER_TOP );
    case B_Adhes:   return aVisibilityFlags.test( LAYER_3D_ADHESIVE );
    case F_Adhes:   return aVisibilityFlags.test( LAYER_3D_ADHESIVE );
    case B_Paste:   return aVisibilityFlags.test( LAYER_3D_SOLDERPASTE );
    case F_Paste:   return aVisibilityFlags.test( LAYER_3D_SOLDERPASTE );
    case B_SilkS:   return aVisibilityFlags.test( LAYER_3D_SILKSCREEN_BOTTOM );
    case F_SilkS:   return aVisibilityFlags.test( LAYER_3D_SILKSCREEN_TOP );
    case B_Mask:    return aVisibilityFlags.test( LAYER_3D_SOLDERMASK_BOTTOM );
    case F_Mask:    return aVisibilityFlags.test( LAYER_3D_SOLDERMASK_TOP );
    case Dwgs_User: return aVisibilityFlags.test( LAYER_3D_USER_DRAWINGS );
    case Cmts_User: return aVisibilityFlags.test( LAYER_3D_USER_COMMENTS );
    case Eco1_User: return aVisibilityFlags.test( LAYER_3D_USER_ECO1 );
    case Eco2_User: return aVisibilityFlags.test( LAYER_3D_USER_ECO2 );
    default:
    {
        int layer3D = MapPCBLayerTo3DLayer( aLayer );

        if( layer3D != UNDEFINED_LAYER )
            return aVisibilityFlags.test( layer3D );

        return m_board && m_board->IsLayerVisible( aLayer );
    }
    }
}


bool BOARD_ADAPTER::IsFootprintShown( const FOOTPRINT* aFootprint ) const
{
    if( m_IsPreviewer )     // In panel Preview, footprints are always shown, of course
        return true;

    if( !aFootprint )
        return false;

    const wxString variantName = m_board ? m_board->GetCurrentVariant() : wxString();
    const bool     excludedFromPos = aFootprint->GetExcludedFromPosFilesForVariant( variantName );
    const bool     dnp = aFootprint->GetDNPForVariant( variantName );
    const auto     attributes = static_cast<FOOTPRINT_ATTR_T>( aFootprint->GetAttributes() );

    if( excludedFromPos )
    {
        if( !m_Cfg->m_Render.show_footprints_not_in_posfile )
            return false;
    }

    if( dnp )
    {
        if( !m_Cfg->m_Render.show_footprints_dnp )
            return false;
    }

    if( attributes & FP_SMD )
        return m_Cfg->m_Render.show_footprints_insert;

    if( attributes & FP_THROUGH_HOLE )
        return m_Cfg->m_Render.show_footprints_normal;

    return m_Cfg->m_Render.show_footprints_virtual;
}


int BOARD_ADAPTER::GetHolePlatingThickness() const noexcept
{
    return m_board ? m_board->GetDesignSettings().GetHolePlatingThickness()
                   : DEFAULT_COPPER_THICKNESS;
}


unsigned int BOARD_ADAPTER::GetCircleSegmentCount( float aDiameter3DU ) const
{
    wxASSERT( aDiameter3DU > 0.0f );

    return GetCircleSegmentCount( (int)( aDiameter3DU / m_biuTo3Dunits ) );
}


unsigned int BOARD_ADAPTER::GetCircleSegmentCount( int aDiameterBIU ) const
{
    wxASSERT( aDiameterBIU > 0 );

    return GetArcToSegmentCount( aDiameterBIU / 2, m_board->GetDesignSettings().m_MaxError, FULL_CIRCLE );
}


void BOARD_ADAPTER::InitSettings( REPORTER* aStatusReporter, REPORTER* aWarningReporter )
{
    wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::InitSettings" ) );

    if( aStatusReporter )
        aStatusReporter->Report( _( "Build board outline" ) );

    wxString msg;

    const bool haveOutline = createBoardPolygon( &msg );

    if( aWarningReporter )
    {
        if( !haveOutline )
            aWarningReporter->Report( msg, RPT_SEVERITY_WARNING );
        else
            aWarningReporter->Report( wxEmptyString );
    }

    BOX2I bbbox;

    if( m_board )
        bbbox = m_board->ComputeBoundingBox( !m_board->IsFootprintHolder() && haveOutline );

    // Gives a non null size to avoid issues in zoom / scale calculations
    if( ( bbbox.GetWidth() == 0 ) && ( bbbox.GetHeight() == 0 ) )
        bbbox.Inflate( pcbIUScale.mmToIU( 10 ) );

    m_boardSize = bbbox.GetSize();
    m_boardPos  = bbbox.Centre();

    wxASSERT( (m_boardSize.x > 0) && (m_boardSize.y > 0) );

    m_boardPos.y = -m_boardPos.y; // The y coord is inverted in 3D viewer

    m_copperLayersCount = m_board ? m_board->GetCopperLayerCount() : 2;

    // Ensure the board has 2 sides for 3D views, because it is hard to find
    // a *really* single side board in the true life...
    if( m_copperLayersCount < 2 )
        m_copperLayersCount = 2;

    // Calculate the conversion to apply to all positions.
    m_biuTo3Dunits = RANGE_SCALE_3D / std::max( m_boardSize.x, m_boardSize.y );

    // Hack to keep "home" zoom from being too small.
    if( !m_board || !m_board->IsFootprintHolder() )
        m_biuTo3Dunits *= 1.6f;

    ReloadColorSettings();

    m_boardBodyThickness3DU        = DEFAULT_BOARD_THICKNESS      * m_biuTo3Dunits;
    m_frontCopperThickness3DU      = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_backCopperThickness3DU       = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU   = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_frontMaskThickness3DU        = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_backMaskThickness3DU         = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_solderPasteLayerThickness3DU = SOLDERPASTE_LAYER_THICKNESS  * m_biuTo3Dunits;

    g_BevelThickness3DU = pcbIUScale.mmToIU( ADVANCED_CFG::GetCfg().m_3DRT_BevelHeight_um / 1000.0 )
                          * m_biuTo3Dunits;

    if( m_board )
    {
        const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        if( bds.GetStackupDescriptor().GetCount() )
        {
            int body_thickness = 0;

            for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
            {
                switch( item->GetType() )
                {
                case BS_ITEM_TYPE_DIELECTRIC:
                    for( int sublayer = 0; sublayer < item->GetSublayersCount(); sublayer++ )
                        body_thickness += item->GetThickness( sublayer );

                    break;

                case BS_ITEM_TYPE_COPPER:
                {
                    // The copper thickness must be > 0 to avoid draw issues (divide by 0 for
                    // instance).   We use a minimal arbitrary value = 1 micrometer here:
                    int copper_thickness = std::max( item->GetThickness(), pcbIUScale.mmToIU( 0.001 ) );

                    if( item->GetBrdLayerId() == F_Cu )
                        m_frontCopperThickness3DU = copper_thickness * m_biuTo3Dunits;
                    else if( item->GetBrdLayerId() == B_Cu )
                        m_backCopperThickness3DU = copper_thickness * m_biuTo3Dunits;
                    else if( item->IsEnabled() )
                        body_thickness += copper_thickness;

                    break;
                }

                case BS_ITEM_TYPE_SOLDERMASK:
                {
                    // The mask thickness must be > 0 to avoid draw issues (divide by 0 for
                    // instance).   We use a minimal arbitrary value = 1 micrometer here:
                    int mask_thickness = std::max( item->GetThickness(), pcbIUScale.mmToIU( 0.001 ) );

                    if( item->GetBrdLayerId() == F_Mask )
                        m_frontMaskThickness3DU = mask_thickness * m_biuTo3Dunits;
                    else if( item->GetBrdLayerId() == B_Mask )
                        m_backMaskThickness3DU = mask_thickness * m_biuTo3Dunits;
                }

                default:
                    break;
                }
            }

            m_boardBodyThickness3DU = body_thickness * m_biuTo3Dunits;
        }
    }

    // Init  Z position of each layer
    // calculate z position for each copper layer
    // Zstart = -m_epoxyThickness / 2.0 is the z position of the back (bottom layer)
    // (layer id = B_Cu)
    // Zstart = +m_epoxyThickness / 2.0 is the z position of the front (top layer)
    // (layer id = F_Cu)

    //  ____==__________==________==______ <- Bottom = +m_epoxyThickness / 2.0,
    // |                                  |   Top = Bottom + m_copperThickness
    // |__________________________________|
    //   ==         ==         ==     ==   <- Bottom = -m_epoxyThickness / 2.0,
    //                                        Top = Bottom - m_copperThickness

    // Generate the Z position of copper layers
    // A copper layer Z position has 2 values: its top Z position and its bottom Z position
    for( auto layer_id : LAYER_RANGE( F_Cu, B_Cu, m_copperLayersCount ) )
    {
        // This approximates internal layer positions (because we're treating all the dielectric
        // layers as having the same thickness).  But we don't render them anyway so it doesn't
        // really matter.
        int layer_pos;      // the position of the copper layer from board top to bottom

        switch( layer_id )
        {
            case F_Cu: layer_pos = 0; break;
            case B_Cu: layer_pos =  m_copperLayersCount - 1; break;
            default: layer_pos = ( layer_id - B_Cu )/2; break;
        };

        m_layerZcoordBottom[layer_id] = m_boardBodyThickness3DU / 2.0
                                        - ( m_boardBodyThickness3DU * layer_pos / ( m_copperLayersCount - 1 ) );

        if( layer_pos < (m_copperLayersCount / 2) )
            m_layerZcoordTop[layer_id] = m_layerZcoordBottom[layer_id] + m_frontCopperThickness3DU;
        else
            m_layerZcoordTop[layer_id] = m_layerZcoordBottom[layer_id] - m_backCopperThickness3DU;
    }

    #define layerThicknessMargin 1.1
    const float zpos_offset = m_nonCopperLayerThickness3DU * layerThicknessMargin;

    // This is the top of the copper layer thickness.
    const float zpos_copperTop_back  = m_layerZcoordTop[B_Cu];
    const float zpos_copperTop_front = m_layerZcoordTop[F_Cu];

    // Fill not copper layers zpos
    for( int layer = 0; layer < PCB_LAYER_ID_COUNT; layer++ )
    {
        PCB_LAYER_ID layer_id = ToLAYER_ID( layer );

        if( IsCopperLayer( layer_id ) )
            continue;

        float zposBottom;
        float zposTop;

        switch( layer_id )
        {
        case B_Mask:
            zposBottom = zpos_copperTop_back;
            zposTop    = zpos_copperTop_back - m_backMaskThickness3DU;
            break;

        case B_Paste:
            zposBottom = zpos_copperTop_back;
            zposTop    = zpos_copperTop_back - m_solderPasteLayerThickness3DU;
            break;

        case F_Mask:
            zposBottom = zpos_copperTop_front;
            zposTop    = zpos_copperTop_front + m_frontMaskThickness3DU;
            break;

        case F_Paste:
            zposBottom = zpos_copperTop_front;
            zposTop    = zpos_copperTop_front + m_solderPasteLayerThickness3DU;
            break;

        case B_SilkS:
            zposBottom = zpos_copperTop_back - 1.0f * zpos_offset;
            zposTop    = zposBottom - m_nonCopperLayerThickness3DU;
            break;

        case F_SilkS:
            zposBottom = zpos_copperTop_front + 1.0f * zpos_offset;
            zposTop    = zposBottom + m_nonCopperLayerThickness3DU;
            break;

        default:
            if( m_board->IsBackLayer( layer_id ) )
            {
                zposBottom = zpos_copperTop_back - 2.0f * zpos_offset;
                zposTop    = zposBottom - m_nonCopperLayerThickness3DU;
            }
            else
            {
                zposBottom = zpos_copperTop_front + 2.0f * zpos_offset;
                zposTop    = zposBottom + m_nonCopperLayerThickness3DU;
            }
            break;
        }

        m_layerZcoordTop[layer_id] = zposTop;
        m_layerZcoordBottom[layer_id] = zposBottom;
    }

    m_boardCenter = SFVEC3F( m_boardPos.x * m_biuTo3Dunits, m_boardPos.y * m_biuTo3Dunits, 0.0f );

    SFVEC3F boardSize = SFVEC3F( m_boardSize.x * m_biuTo3Dunits, m_boardSize.y * m_biuTo3Dunits, 0.0f );
    boardSize /= 2.0f;

    SFVEC3F boardMin = ( m_boardCenter - boardSize );
    SFVEC3F boardMax = ( m_boardCenter + boardSize );

    boardMin.z = m_layerZcoordTop[B_Adhes];
    boardMax.z = m_layerZcoordTop[F_Adhes];

    m_boardBoundingBox = BBOX_3D( boardMin, boardMax );

#ifdef PRINT_STATISTICS_3D_VIEWER
    int64_t stats_startCreateBoardPolyTime = GetRunningMicroSecs();
#endif

    if( aStatusReporter )
        aStatusReporter->Report( _( "Create layers" ) );

    createLayers( aStatusReporter );

    auto to_SFVEC4F =
            []( const COLOR4D& src )
            {
                return SFVEC4F( src.r, src.g, src.b, src.a );
            };

    std::map<int, COLOR4D> colors = GetLayerColors();

    m_BgColorTop         = to_SFVEC4F( colors[ LAYER_3D_BACKGROUND_TOP ] );
    m_BgColorBot         = to_SFVEC4F( colors[ LAYER_3D_BACKGROUND_BOTTOM ] );
    m_SolderPasteColor   = to_SFVEC4F( colors[ LAYER_3D_SOLDERPASTE ] );
    m_SilkScreenColorBot = to_SFVEC4F( colors[ LAYER_3D_SILKSCREEN_BOTTOM ] );
    m_SilkScreenColorTop = to_SFVEC4F( colors[ LAYER_3D_SILKSCREEN_TOP ] );
    m_SolderMaskColorBot = to_SFVEC4F( colors[ LAYER_3D_SOLDERMASK_BOTTOM ] );
    m_SolderMaskColorTop = to_SFVEC4F( colors[ LAYER_3D_SOLDERMASK_TOP ] );
    m_CopperColor        = to_SFVEC4F( colors[ LAYER_3D_COPPER_TOP ] );
    m_BoardBodyColor     = to_SFVEC4F( colors[ LAYER_3D_BOARD ] );
    m_UserDrawingsColor  = to_SFVEC4F( colors[ LAYER_3D_USER_DRAWINGS ] );
    m_UserCommentsColor  = to_SFVEC4F( colors[ LAYER_3D_USER_COMMENTS ] );
    m_ECO1Color          = to_SFVEC4F( colors[ LAYER_3D_USER_ECO1 ] );
    m_ECO2Color          = to_SFVEC4F( colors[ LAYER_3D_USER_ECO2 ] );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        m_UserDefinedLayerColor[ layer - LAYER_3D_USER_1 ] = to_SFVEC4F( colors[ layer ] );
}


std::map<int, COLOR4D> BOARD_ADAPTER::GetDefaultColors() const
{
    std::map<int, COLOR4D> colors;

    colors[ LAYER_3D_BACKGROUND_TOP ]    = BOARD_ADAPTER::g_DefaultBackgroundTop;
    colors[ LAYER_3D_BACKGROUND_BOTTOM ] = BOARD_ADAPTER::g_DefaultBackgroundBot;
    colors[ LAYER_3D_BOARD ]             = BOARD_ADAPTER::g_DefaultBoardBody;
    colors[ LAYER_3D_COPPER_TOP ]        = BOARD_ADAPTER::g_DefaultSurfaceFinish;
    colors[ LAYER_3D_COPPER_BOTTOM ]     = BOARD_ADAPTER::g_DefaultSurfaceFinish;
    colors[ LAYER_3D_SILKSCREEN_TOP ]    = BOARD_ADAPTER::g_DefaultSilkscreen;
    colors[ LAYER_3D_SILKSCREEN_BOTTOM ] = BOARD_ADAPTER::g_DefaultSilkscreen;
    colors[ LAYER_3D_SOLDERMASK_TOP ]    = BOARD_ADAPTER::g_DefaultSolderMask;
    colors[ LAYER_3D_SOLDERMASK_BOTTOM ] = BOARD_ADAPTER::g_DefaultSolderMask;
    colors[ LAYER_3D_SOLDERPASTE ]       = BOARD_ADAPTER::g_DefaultSolderPaste;
    colors[ LAYER_3D_USER_DRAWINGS ]     = BOARD_ADAPTER::g_DefaultComments;
    colors[ LAYER_3D_USER_COMMENTS ]     = BOARD_ADAPTER::g_DefaultComments;
    colors[ LAYER_3D_USER_ECO1 ]         = BOARD_ADAPTER::g_DefaultECOs;
    colors[ LAYER_3D_USER_ECO2 ]         = BOARD_ADAPTER::g_DefaultECOs;

    COLOR_SETTINGS* settings = ::GetColorSettings( DEFAULT_THEME );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        colors[ layer ] = settings->GetColor( layer );

    return colors;
}


std::map<int, COLOR4D> BOARD_ADAPTER::GetLayerColors() const
{
    std::map<int, COLOR4D> colors;

    if( LAYER_PRESET_3D* preset = m_Cfg->FindPreset( m_Cfg->m_CurrentPreset ) )
    {
        colors = preset->colors;
    }
    else
    {
        COLOR_SETTINGS* settings = ::GetColorSettings( DEFAULT_THEME );

        for( const auto& [ layer, defaultColor /* unused */ ] : GetDefaultColors() )
            colors[ layer ] = settings->GetColor( layer );
    }

    if( m_Cfg->m_UseStackupColors && m_board )
    {
        const BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();
        KIGFX::COLOR4D       bodyColor( 0, 0, 0, 0 );

        // Can't do a const KIGFX::COLOR4D& return type here because there are temporary variables
        auto findColor =
                []( const wxString& aColorName,
                    const CUSTOM_COLORS_LIST& aColorSet ) -> const KIGFX::COLOR4D
                {
                    if( aColorName.StartsWith( wxT( "#" ) ) )
                    {
                        return KIGFX::COLOR4D( aColorName );
                    }
                    else
                    {
                        for( const CUSTOM_COLOR_ITEM& color : aColorSet )
                        {
                            if( color.m_ColorName == aColorName )
                                return color.m_Color;
                        }
                    }

                    return KIGFX::COLOR4D();
                };

        for( const BOARD_STACKUP_ITEM* stackupItem : stackup.GetList() )
        {
            wxString colorName = stackupItem->GetColor();

            switch( stackupItem->GetType() )
            {
            case BS_ITEM_TYPE_SILKSCREEN:
                if( stackupItem->GetBrdLayerId() == F_SilkS )
                    colors[ LAYER_3D_SILKSCREEN_TOP ] = findColor( colorName, g_SilkColors );
                else
                    colors[ LAYER_3D_SILKSCREEN_BOTTOM ] = findColor( colorName, g_SilkColors );

                break;

            case BS_ITEM_TYPE_SOLDERMASK:
                if( stackupItem->GetBrdLayerId() == F_Mask )
                    colors[ LAYER_3D_SOLDERMASK_TOP ] = findColor( colorName, g_MaskColors );
                else
                    colors[ LAYER_3D_SOLDERMASK_BOTTOM ] = findColor( colorName, g_MaskColors );

                break;

            case BS_ITEM_TYPE_DIELECTRIC:
            {
                KIGFX::COLOR4D layerColor = findColor( colorName, g_BoardColors );

                if( bodyColor == COLOR4D( 0, 0, 0, 0 ) )
                    bodyColor = layerColor;
                else
                    bodyColor = bodyColor.Mix( layerColor, 1.0 - layerColor.a );

                bodyColor.a += ( 1.0 - bodyColor.a ) * layerColor.a / 2;
                break;
            }

            default:
                break;
            }
        }

        if( bodyColor != COLOR4D( 0, 0, 0, 0 ) )
            colors[ LAYER_3D_BOARD ] = bodyColor;

        const wxString& finishName = stackup.m_FinishType;

        if( finishName.EndsWith( wxT( "OSP" ) ) )
        {
            colors[ LAYER_3D_COPPER_TOP ] = findColor( wxT( "Copper" ), g_FinishColors );
        }
        else if( finishName.EndsWith( wxT( "IG" ) )
              || finishName.EndsWith( wxT( "gold" ) ) )
        {
            colors[ LAYER_3D_COPPER_TOP ] = findColor( wxT( "Gold" ), g_FinishColors );
        }
        else if( finishName.StartsWith( wxT( "HAL" ) )
              || finishName.StartsWith( wxT( "HASL" ) )
              || finishName.EndsWith( wxT( "tin" ) )
              || finishName.EndsWith( wxT( "nickel" ) ) )
        {
            colors[ LAYER_3D_COPPER_TOP ] = findColor( wxT( "Tin" ), g_FinishColors );
        }
        else if( finishName.EndsWith( wxT( "silver" ) ) )
        {
            colors[ LAYER_3D_COPPER_TOP ] = findColor( wxT( "Silver" ), g_FinishColors );
        }
    }

    colors[ LAYER_3D_COPPER_BOTTOM ] = colors[ LAYER_3D_COPPER_TOP ];

    for( const auto& [layer, val] : m_ColorOverrides )
        colors[layer] = val;

    return colors;
}


void BOARD_ADAPTER::SetLayerColors( const std::map<int, COLOR4D>& aColors )
{
    COLOR_SETTINGS* settings = ::GetColorSettings( DEFAULT_THEME );

    for( const auto& [ layer, color ] : aColors )
    {
        settings->SetColor( layer, color );

        if( layer >= LAYER_3D_USER_1 && layer <= LAYER_3D_USER_45 )
            m_UserDefinedLayerColor[ layer - LAYER_3D_USER_1 ] = GetColor( color );
    }

    Pgm().GetSettingsManager().SaveColorSettings( settings, "3d_viewer" );
}


void BOARD_ADAPTER::SetVisibleLayers( const std::bitset<LAYER_3D_END>& aLayers )
{
    m_Cfg->m_Render.show_board_body                = aLayers.test( LAYER_3D_BOARD );
    m_Cfg->m_Render.show_plated_barrels            = aLayers.test( LAYER_3D_PLATED_BARRELS );
    m_Cfg->m_Render.show_copper_top                = aLayers.test( LAYER_3D_COPPER_TOP );
    m_Cfg->m_Render.show_copper_bottom             = aLayers.test( LAYER_3D_COPPER_BOTTOM );
    m_Cfg->m_Render.show_silkscreen_top            = aLayers.test( LAYER_3D_SILKSCREEN_TOP );
    m_Cfg->m_Render.show_silkscreen_bottom         = aLayers.test( LAYER_3D_SILKSCREEN_BOTTOM );
    m_Cfg->m_Render.show_soldermask_top            = aLayers.test( LAYER_3D_SOLDERMASK_TOP );
    m_Cfg->m_Render.show_soldermask_bottom         = aLayers.test( LAYER_3D_SOLDERMASK_BOTTOM );
    m_Cfg->m_Render.show_solderpaste               = aLayers.test( LAYER_3D_SOLDERPASTE );
    m_Cfg->m_Render.show_adhesive                  = aLayers.test( LAYER_3D_ADHESIVE );
    m_Cfg->m_Render.show_comments                  = aLayers.test( LAYER_3D_USER_COMMENTS );
    m_Cfg->m_Render.show_drawings                  = aLayers.test( LAYER_3D_USER_DRAWINGS );
    m_Cfg->m_Render.show_eco1                      = aLayers.test( LAYER_3D_USER_ECO1 );
    m_Cfg->m_Render.show_eco2                      = aLayers.test( LAYER_3D_USER_ECO2 );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        m_Cfg->m_Render.show_user[ layer - LAYER_3D_USER_1 ] = aLayers.test( layer );

    m_Cfg->m_Render.show_footprints_normal         = aLayers.test( LAYER_3D_TH_MODELS );
    m_Cfg->m_Render.show_footprints_insert         = aLayers.test( LAYER_3D_SMD_MODELS );
    m_Cfg->m_Render.show_footprints_virtual        = aLayers.test( LAYER_3D_VIRTUAL_MODELS );
    m_Cfg->m_Render.show_footprints_not_in_posfile = aLayers.test( LAYER_3D_MODELS_NOT_IN_POS );
    m_Cfg->m_Render.show_footprints_dnp            = aLayers.test( LAYER_3D_MODELS_MARKED_DNP );

    m_Cfg->m_Render.show_fp_references             = aLayers.test( LAYER_FP_REFERENCES );
    m_Cfg->m_Render.show_fp_values                 = aLayers.test( LAYER_FP_VALUES );
    m_Cfg->m_Render.show_fp_text                   = aLayers.test( LAYER_FP_TEXT );

    m_Cfg->m_Render.show_model_bbox                = aLayers.test( LAYER_3D_BOUNDING_BOXES );
    m_Cfg->m_Render.show_off_board_silk            = aLayers.test( LAYER_3D_OFF_BOARD_SILK );
    m_Cfg->m_Render.show_navigator                 = aLayers.test( LAYER_3D_NAVIGATOR );
}


std::bitset<LAYER_3D_END> BOARD_ADAPTER::GetVisibleLayers() const
{
    std::bitset<LAYER_3D_END> ret;

    if( m_IsPreviewer )
    {
        if( m_Cfg->m_Render.preview_show_board_body )
        {
            ret.set( LAYER_3D_BOARD,             m_Cfg->m_Render.show_board_body );
            ret.set( LAYER_3D_SOLDERMASK_TOP,    m_Cfg->m_Render.show_soldermask_top );
            ret.set( LAYER_3D_SOLDERMASK_BOTTOM, m_Cfg->m_Render.show_soldermask_bottom );
            ret.set( LAYER_3D_SOLDERPASTE,       m_Cfg->m_Render.show_solderpaste );
            ret.set( LAYER_3D_ADHESIVE,          m_Cfg->m_Render.show_adhesive );
        }

        ret.set( LAYER_3D_PLATED_BARRELS,    true );
        ret.set( LAYER_3D_COPPER_TOP,        true );
        ret.set( LAYER_3D_COPPER_BOTTOM,     true );
        ret.set( LAYER_3D_SILKSCREEN_TOP,    true );
        ret.set( LAYER_3D_SILKSCREEN_BOTTOM, true );
        ret.set( LAYER_3D_USER_COMMENTS,     true );
        ret.set( LAYER_3D_USER_DRAWINGS,     true );
        ret.set( LAYER_3D_USER_ECO1,         true );
        ret.set( LAYER_3D_USER_ECO2,         true );

        for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
            ret.set( layer, true );

        ret.set( LAYER_FP_REFERENCES,        true );
        ret.set( LAYER_FP_VALUES,            true );
        ret.set( LAYER_FP_TEXT,              true );

        ret.set( LAYER_3D_TH_MODELS,         true );
        ret.set( LAYER_3D_SMD_MODELS,        true );
        ret.set( LAYER_3D_VIRTUAL_MODELS,    true );
        ret.set( LAYER_3D_MODELS_NOT_IN_POS, true );
        ret.set( LAYER_3D_MODELS_MARKED_DNP, true );

        ret.set( LAYER_3D_BOUNDING_BOXES,    m_Cfg->m_Render.show_model_bbox );
        ret.set( LAYER_3D_OFF_BOARD_SILK,    m_Cfg->m_Render.show_off_board_silk );
        ret.set( LAYER_3D_NAVIGATOR,         m_Cfg->m_Render.show_navigator );

        return ret;
    }

    ret.set( LAYER_3D_BOARD,             m_Cfg->m_Render.show_board_body );
    ret.set( LAYER_3D_PLATED_BARRELS,    m_Cfg->m_Render.show_plated_barrels );
    ret.set( LAYER_3D_COPPER_TOP,        m_Cfg->m_Render.show_copper_top );
    ret.set( LAYER_3D_COPPER_BOTTOM,     m_Cfg->m_Render.show_copper_bottom );
    ret.set( LAYER_3D_SILKSCREEN_TOP,    m_Cfg->m_Render.show_silkscreen_top );
    ret.set( LAYER_3D_SILKSCREEN_BOTTOM, m_Cfg->m_Render.show_silkscreen_bottom );
    ret.set( LAYER_3D_SOLDERMASK_TOP,    m_Cfg->m_Render.show_soldermask_top );
    ret.set( LAYER_3D_SOLDERMASK_BOTTOM, m_Cfg->m_Render.show_soldermask_bottom );
    ret.set( LAYER_3D_SOLDERPASTE,       m_Cfg->m_Render.show_solderpaste );
    ret.set( LAYER_3D_ADHESIVE,          m_Cfg->m_Render.show_adhesive );
    ret.set( LAYER_3D_USER_COMMENTS,     m_Cfg->m_Render.show_comments );
    ret.set( LAYER_3D_USER_DRAWINGS,     m_Cfg->m_Render.show_drawings );
    ret.set( LAYER_3D_USER_ECO1,         m_Cfg->m_Render.show_eco1 );
    ret.set( LAYER_3D_USER_ECO2,         m_Cfg->m_Render.show_eco2 );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        ret.set( layer, m_Cfg->m_Render.show_user[ layer - LAYER_3D_USER_1 ] );

    ret.set( LAYER_FP_REFERENCES,        m_Cfg->m_Render.show_fp_references );
    ret.set( LAYER_FP_VALUES,            m_Cfg->m_Render.show_fp_values );
    ret.set( LAYER_FP_TEXT,              m_Cfg->m_Render.show_fp_text );

    ret.set( LAYER_3D_TH_MODELS,         m_Cfg->m_Render.show_footprints_normal );
    ret.set( LAYER_3D_SMD_MODELS,        m_Cfg->m_Render.show_footprints_insert );
    ret.set( LAYER_3D_VIRTUAL_MODELS,    m_Cfg->m_Render.show_footprints_virtual );
    ret.set( LAYER_3D_MODELS_NOT_IN_POS, m_Cfg->m_Render.show_footprints_not_in_posfile );
    ret.set( LAYER_3D_MODELS_MARKED_DNP, m_Cfg->m_Render.show_footprints_dnp );

    ret.set( LAYER_3D_BOUNDING_BOXES,    m_Cfg->m_Render.show_model_bbox );
    ret.set( LAYER_3D_OFF_BOARD_SILK,    m_Cfg->m_Render.show_off_board_silk );
    ret.set( LAYER_3D_NAVIGATOR,         m_Cfg->m_Render.show_navigator );

    if( m_Cfg->m_CurrentPreset == FOLLOW_PCB )
    {
        if( !m_board )
            return ret;

        ret.set( LAYER_3D_BOARD,             true );
        ret.set( LAYER_3D_COPPER_TOP,        m_board->IsLayerVisible( F_Cu ) );
        ret.set( LAYER_3D_COPPER_BOTTOM,     m_board->IsLayerVisible( B_Cu ) );
        ret.set( LAYER_3D_SILKSCREEN_TOP,    m_board->IsLayerVisible( F_SilkS ) );
        ret.set( LAYER_3D_SILKSCREEN_BOTTOM, m_board->IsLayerVisible( B_SilkS ) );
        ret.set( LAYER_3D_SOLDERMASK_TOP,    m_board->IsLayerVisible( F_Mask ) );
        ret.set( LAYER_3D_SOLDERMASK_BOTTOM, m_board->IsLayerVisible( B_Mask ) );
        ret.set( LAYER_3D_SOLDERPASTE,       m_board->IsLayerVisible( F_Paste ) );
        ret.set( LAYER_3D_ADHESIVE,          m_board->IsLayerVisible( F_Adhes ) );
        ret.set( LAYER_3D_USER_COMMENTS,     m_board->IsLayerVisible( Cmts_User ) );
        ret.set( LAYER_3D_USER_DRAWINGS,     m_board->IsLayerVisible( Dwgs_User ) );
        ret.set( LAYER_3D_USER_ECO1,         m_board->IsLayerVisible( Eco1_User ) );
        ret.set( LAYER_3D_USER_ECO2,         m_board->IsLayerVisible( Eco2_User ) );

        for( GAL_LAYER_ID layer : { LAYER_FP_REFERENCES, LAYER_FP_VALUES, LAYER_FP_TEXT } )
            ret.set( layer, m_board->IsElementVisible( layer ) );
    }
    else if( m_Cfg->m_CurrentPreset == FOLLOW_PLOT_SETTINGS )
    {
        if( !m_board )
            return ret;

        const PCB_PLOT_PARAMS& plotParams = m_board->GetPlotOptions();
        LSET                   layers = plotParams.GetLayerSelection();

        for( PCB_LAYER_ID commonLayer : plotParams.GetPlotOnAllLayersSequence() )
            layers.set( commonLayer );

        ret.set( LAYER_3D_BOARD,             true );
        ret.set( LAYER_3D_COPPER_TOP,        layers.test( F_Cu ) );
        ret.set( LAYER_3D_COPPER_BOTTOM,     layers.test( B_Cu ) );
        ret.set( LAYER_3D_SILKSCREEN_TOP,    layers.test( F_SilkS ) );
        ret.set( LAYER_3D_SILKSCREEN_BOTTOM, layers.test( B_SilkS ) );
        ret.set( LAYER_3D_SOLDERMASK_TOP,    layers.test( F_Mask ) );
        ret.set( LAYER_3D_SOLDERMASK_BOTTOM, layers.test( B_Mask ) );
        ret.set( LAYER_3D_SOLDERPASTE,       layers.test( F_Paste ) );
        ret.set( LAYER_3D_ADHESIVE,          layers.test( F_Adhes ) );
        ret.set( LAYER_3D_USER_COMMENTS,     layers.test( Cmts_User ) );
        ret.set( LAYER_3D_USER_DRAWINGS,     layers.test( Dwgs_User ) );
        ret.set( LAYER_3D_USER_ECO1,         layers.test( Eco1_User ) );
        ret.set( LAYER_3D_USER_ECO2,         layers.test( Eco2_User ) );

        for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
            ret.set( layer, layers.test( Map3DLayerToPCBLayer( layer ) ) );

        ret.set( LAYER_FP_REFERENCES,        plotParams.GetPlotReference() );
        ret.set( LAYER_FP_VALUES,            plotParams.GetPlotValue() );
        ret.set( LAYER_FP_TEXT,              plotParams.GetPlotFPText() );
    }
    else if( LAYER_PRESET_3D* preset = m_Cfg->FindPreset( m_Cfg->m_CurrentPreset ) )
    {
        ret = preset->layers;
    }

    return ret;
}


std::bitset<LAYER_3D_END> BOARD_ADAPTER::GetDefaultVisibleLayers() const
{
    std::bitset<LAYER_3D_END> ret;

    ret.set( LAYER_3D_BOARD,             true );
    ret.set( LAYER_3D_PLATED_BARRELS,    true );
    ret.set( LAYER_3D_COPPER_TOP,        true );
    ret.set( LAYER_3D_COPPER_BOTTOM,     true );
    ret.set( LAYER_3D_SILKSCREEN_TOP,    true );
    ret.set( LAYER_3D_SILKSCREEN_BOTTOM, true );
    ret.set( LAYER_3D_SOLDERMASK_TOP,    true );
    ret.set( LAYER_3D_SOLDERMASK_BOTTOM, true );
    ret.set( LAYER_3D_SOLDERPASTE,       true );
    ret.set( LAYER_3D_ADHESIVE,          true );
    ret.set( LAYER_3D_USER_COMMENTS,     false );
    ret.set( LAYER_3D_USER_DRAWINGS,     false );
    ret.set( LAYER_3D_USER_ECO1,         false );
    ret.set( LAYER_3D_USER_ECO2,         false );

    for( int layer = LAYER_3D_USER_1; layer <= LAYER_3D_USER_45; ++layer )
        ret.set( layer, false );

    ret.set( LAYER_FP_REFERENCES,        true );
    ret.set( LAYER_FP_VALUES,            true );
    ret.set( LAYER_FP_TEXT,              true );

    ret.set( LAYER_3D_TH_MODELS,         true );
    ret.set( LAYER_3D_SMD_MODELS,        true );
    ret.set( LAYER_3D_VIRTUAL_MODELS,    true );
    ret.set( LAYER_3D_MODELS_NOT_IN_POS, false );
    ret.set( LAYER_3D_MODELS_MARKED_DNP, false );

    ret.set( LAYER_3D_BOUNDING_BOXES,    false );
    ret.set( LAYER_3D_OFF_BOARD_SILK,    false );
    ret.set( LAYER_3D_NAVIGATOR,         true );

    return ret;
}


bool BOARD_ADAPTER::GetUseBoardEditorCopperLayerColors() const
{
    return m_Cfg->m_Render.use_board_editor_copper_colors && !m_board->IsFootprintHolder();
}


bool BOARD_ADAPTER::createBoardPolygon( wxString* aErrorMsg )
{
    m_board_poly.RemoveAllContours();

    if( !m_board )
        return false;

    bool success;

    if( m_board->IsFootprintHolder() )
    {
        if( !m_board->GetFirstFootprint() )
        {
            if( aErrorMsg )
                *aErrorMsg = _( "No footprint loaded." );

            return false;
        }

        // max dist from one endPt to next startPt
        int chainingEpsilon = m_board->GetOutlinesChainingEpsilon();

        success = BuildFootprintPolygonOutlines( m_board, m_board_poly,
                                                 m_board->GetDesignSettings().m_MaxError,
                                                 chainingEpsilon );
        m_board_poly.Simplify();

        if( !success && aErrorMsg )
        {
            *aErrorMsg = _( "Footprint outline is missing or malformed. Run Footprint Checker for "
                            "a full analysis." );
        }
    }
    else
    {
        success = m_board->GetBoardPolygonOutlines( m_board_poly, true, nullptr, false, true );

        if( !success && aErrorMsg )
            *aErrorMsg = _( "Board outline is missing or malformed. Run DRC for a full analysis." );
    }

    return success;
}


float BOARD_ADAPTER::GetFootprintZPos( bool aIsFlipped ) const
{
    if( aIsFlipped )
    {
        if( auto it = m_layerZcoordBottom.find( B_Paste ); it != m_layerZcoordBottom.end() )
            return it->second;
    }
    else
    {
        if( auto it = m_layerZcoordTop.find( F_Paste ); it != m_layerZcoordTop.end() )
            return it->second;
    }

    return 0.0;
}


SFVEC4F BOARD_ADAPTER::GetLayerColor( int aLayerId ) const
{
    if( aLayerId >= LAYER_3D_USER_1 && aLayerId <= LAYER_3D_USER_45 )
        aLayerId = Map3DLayerToPCBLayer( aLayerId );

    wxASSERT( aLayerId < PCB_LAYER_ID_COUNT );

    return GetColor( m_BoardEditorColors.at( aLayerId ) );
}


SFVEC4F BOARD_ADAPTER::GetColor( const COLOR4D& aColor ) const
{
    return SFVEC4F( aColor.r, aColor.g, aColor.b, aColor.a );
}


SFVEC2F BOARD_ADAPTER::GetSphericalCoord( int i ) const
{
    SFVEC2F sphericalCoord = SFVEC2F( ( m_Cfg->m_Render.raytrace_lightElevation[i] + 90.0f ) / 180.0f,
                                      m_Cfg->m_Render.raytrace_lightAzimuth[i] / 180.0f );

    sphericalCoord.x = glm::clamp( sphericalCoord.x, 0.0f, 1.0f );
    sphericalCoord.y = glm::clamp( sphericalCoord.y, 0.0f, 2.0f );

    return sphericalCoord;
}

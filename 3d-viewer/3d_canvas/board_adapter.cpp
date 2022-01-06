/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "../3d_rendering/camera.h"
#include "board_adapter.h"
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <board_stackup_manager/stackup_predefined_prms.h>
#include <3d_rendering/raytracing/shapes2D/polygon_2d.h>
#include <board.h>
#include <dialogs/dialog_color_picker.h>
#include <3d_math.h>
#include "3d_fastmath.h"
#include <geometry/geometry_utils.h>
#include <convert_to_biu.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <wx/log.h>


#define DEFAULT_BOARD_THICKNESS Millimeter2iu( 1.6 )
#define DEFAULT_COPPER_THICKNESS Millimeter2iu( 0.035 )   // for 35 um
// The solder mask layer (and silkscreen) thickness
#define DEFAULT_TECH_LAYER_THICKNESS Millimeter2iu( 0.025 )
// The solder paste thickness is chosen bigger than the solder mask layer
// to be sure is covers the mask when overlapping.
#define SOLDERPASTE_LAYER_THICKNESS Millimeter2iu( 0.04 )


CUSTOM_COLORS_LIST   BOARD_ADAPTER::g_SilkscreenColors;
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
        m_colors( nullptr ),
        m_layerZcoordTop(),
        m_layerZcoordBottom()
{
    wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::BOARD_ADAPTER" ) );

    if( PgmOrNull() )
        m_colors = Pgm().GetSettingsManager().GetColorSettings();

    m_boardPos = VECTOR2I();
    m_boardSize = VECTOR2I();
    m_boardCenter = SFVEC3F( 0.0f );

    m_boardBoundingBox.Reset();

    m_throughHoleIds.Clear();
    m_throughHoleOds.Clear();
    m_throughHoleAnnularRings.Clear();

    m_copperLayersCount = 2;

    m_biuTo3Dunits = 1.0;
    m_boardBodyThickness3DU        = DEFAULT_BOARD_THICKNESS      * m_biuTo3Dunits;
    m_frontCopperThickness3DU      = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_backCopperThickness3DU       = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU   = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
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

    m_platedPadsFront = nullptr;
    m_platedPadsBack = nullptr;

    m_frontPlatedPadPolys = nullptr;
    m_backPlatedPadPolys = nullptr;

    if( !g_ColorsLoaded )
    {
#define ADD_COLOR( list, r, g, b, a, name ) \
    list.push_back( CUSTOM_COLOR_ITEM( r/255.0, g/255.0, b/255.0, a, name ) )

        ADD_COLOR( g_SilkscreenColors, 245, 245, 245, 1.0, NotSpecifiedPrm() ); // White
        ADD_COLOR( g_SilkscreenColors,  20,  51,  36, 1.0, "Green" );
        ADD_COLOR( g_SilkscreenColors, 181,  19,  21, 1.0, "Red" );
        ADD_COLOR( g_SilkscreenColors,   2,  59, 162, 1.0, "Blue" );
        ADD_COLOR( g_SilkscreenColors,  11,  11,  11, 1.0, "Black" );
        ADD_COLOR( g_SilkscreenColors, 245, 245, 245, 1.0, "White" );
        ADD_COLOR( g_SilkscreenColors,  32,   2,  53, 1.0, "Purple" );
        ADD_COLOR( g_SilkscreenColors, 194,  195,  0, 1.0, "Yellow" );

        ADD_COLOR( g_MaskColors,  20,  51,  36, 0.83, NotSpecifiedPrm() ); // Green
        ADD_COLOR( g_MaskColors,  20,  51,  36, 0.83, "Green" );
        ADD_COLOR( g_MaskColors,  91, 168,  12, 0.83, "Light Green" );
        ADD_COLOR( g_MaskColors,  13, 104,  11, 0.83, "Saturated Green" );
        ADD_COLOR( g_MaskColors, 181,  19,  21, 0.83, "Red" );
        ADD_COLOR( g_MaskColors, 210,  40,  14, 0.83, "Light Red" );
        ADD_COLOR( g_MaskColors, 239,  53,  41, 0.83, "Red/Orange" );
        ADD_COLOR( g_MaskColors,   2,  59, 162, 0.83, "Blue" );
        ADD_COLOR( g_MaskColors,  54,  79, 116, 0.83, "Light Blue 1" );
        ADD_COLOR( g_MaskColors,  61,  85, 130, 0.83, "Light Blue 2" );
        ADD_COLOR( g_MaskColors,  21,  70,  80, 0.83, "Green/Blue" );
        ADD_COLOR( g_MaskColors,  11,  11,  11, 0.83, "Black" );
        ADD_COLOR( g_MaskColors, 245, 245, 245, 0.83, "White" );
        ADD_COLOR( g_MaskColors,  32,   2,  53, 0.83, "Purple" );
        ADD_COLOR( g_MaskColors, 119,  31,  91, 0.83, "Light Purple" );
        ADD_COLOR( g_MaskColors, 194,  195,  0, 0.83, "Yellow" );

        ADD_COLOR( g_PasteColors, 128, 128, 128, 1.0, "Grey" );
        ADD_COLOR( g_PasteColors,  90,  90,  90, 1.0, "Dark Grey" );
        ADD_COLOR( g_PasteColors, 213, 213, 213, 1.0, "Silver" );

        ADD_COLOR( g_FinishColors, 184, 115,  50, 1.0, "Copper" );
        ADD_COLOR( g_FinishColors, 178, 156,   0, 1.0, "Gold" );
        ADD_COLOR( g_FinishColors, 213, 213, 213, 1.0, "Silver" );
        ADD_COLOR( g_FinishColors, 160, 160, 160, 1.0, "Tin" );

        ADD_COLOR( g_BoardColors,  51,  43,  22, 0.83, "FR4 natural, dark" );
        ADD_COLOR( g_BoardColors, 109, 116,  75, 0.83, "FR4 natural" );
        ADD_COLOR( g_BoardColors, 252, 252, 250, 0.90, "PTFE natural" );
        ADD_COLOR( g_BoardColors, 205, 130,   0, 0.68, "Polyimide" );
        ADD_COLOR( g_BoardColors,  92,  17,   6, 0.90, "Phenolic natural" );
        ADD_COLOR( g_BoardColors, 146,  99,  47, 0.83, "Brown 1" );
        ADD_COLOR( g_BoardColors, 160, 123,  54, 0.83, "Brown 2" );
        ADD_COLOR( g_BoardColors, 146,  99,  47, 0.83, "Brown 3" );
        ADD_COLOR( g_BoardColors, 213, 213, 213,  1.0, "Aluminum" );

        g_DefaultBackgroundTop = COLOR4D(  0.80, 0.80, 0.90, 1.0 );
        g_DefaultBackgroundBot = COLOR4D(  0.40, 0.40, 0.50, 1.0 );

        g_DefaultSilkscreen =    COLOR4D( 0.94, 0.94, 0.94,  1.0 );
        g_DefaultSolderMask =    COLOR4D( 0.08, 0.20, 0.14, 0.83 );
        g_DefaultSolderPaste =   COLOR4D( 0.50, 0.50, 0.50,  1.0 );
        g_DefaultSurfaceFinish = COLOR4D( 0.75, 0.61, 0.23,  1.0 );
        g_DefaultBoardBody =     COLOR4D( 0.43, 0.45, 0.30, 0.90 );

        g_ColorsLoaded = true;
    }
#undef ADD_COLOR
}


BOARD_ADAPTER::~BOARD_ADAPTER()
{
    destroyLayers();
}


bool BOARD_ADAPTER::Is3dLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    wxASSERT( aLayer < PCB_LAYER_ID_COUNT );

    if( m_board && !m_board->IsLayerEnabled( aLayer ) )
        return false;

    // see if layer needs to be shown
    // check the flags
    switch( aLayer )
    {
    case B_Adhes:
    case F_Adhes:
        return m_Cfg->m_Render.show_adhesive;

    case B_Paste:
    case F_Paste:
        return m_Cfg->m_Render.show_solderpaste;

    case B_SilkS:
    case F_SilkS:
        return m_Cfg->m_Render.show_silkscreen;

    case B_Mask:
    case F_Mask:
        return m_Cfg->m_Render.show_soldermask;

    case Dwgs_User:
    case Cmts_User:
        return !m_Cfg->m_Render.realistic && m_Cfg->m_Render.show_comments;

    case Eco1_User:
    case Eco2_User:
        return !m_Cfg->m_Render.realistic && m_Cfg->m_Render.show_eco;

    case Edge_Cuts:
        return !m_Cfg->m_Render.realistic && !m_Cfg->m_Render.show_board_body;

    case Margin:
        return !m_Cfg->m_Render.realistic;

    case B_Cu:
    case F_Cu:
        return !m_board || m_board->IsLayerVisible( aLayer ) || m_Cfg->m_Render.realistic;

    default:
        // the layer is an internal copper layer, used the visibility
        return m_board && m_board->IsLayerVisible( aLayer );
    }
}


bool BOARD_ADAPTER::IsFootprintShown( FOOTPRINT_ATTR_T aFPAttributes ) const
{
    if( m_IsPreviewer )     // In panel Preview, footprints are always shown, of cource
        return true;

    if( aFPAttributes & FP_SMD )
        return m_Cfg->m_Render.show_footprints_insert;
    else if( aFPAttributes & FP_THROUGH_HOLE )
        return m_Cfg->m_Render.show_footprints_normal;
    else
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

    return GetArcToSegmentCount( aDiameterBIU / 2, ARC_HIGH_DEF, 360.0 );
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

    EDA_RECT bbbox;

    if( m_board )
    {
        bbbox = m_board->ComputeBoundingBox( !m_board->IsFootprintHolder()
                                                 && m_Cfg->m_Render.realistic
                                                 && haveOutline );
    }

    // Gives a non null size to avoid issues in zoom / scale calculations
    if( ( bbbox.GetWidth() == 0 ) && ( bbbox.GetHeight() == 0 ) )
        bbbox.Inflate( Millimeter2iu( 10 ) );

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

    m_boardBodyThickness3DU        = DEFAULT_BOARD_THICKNESS      * m_biuTo3Dunits;
    m_frontCopperThickness3DU      = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_backCopperThickness3DU       = DEFAULT_COPPER_THICKNESS     * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU   = DEFAULT_TECH_LAYER_THICKNESS * m_biuTo3Dunits;
    m_solderPasteLayerThickness3DU = SOLDERPASTE_LAYER_THICKNESS  * m_biuTo3Dunits;

    if( m_board )
    {
        const BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        if( bds.GetStackupDescriptor().GetCount() )
        {
            int thickness = 0;

            for( BOARD_STACKUP_ITEM* item : bds.GetStackupDescriptor().GetList() )
            {
                switch( item->GetType() )
                {
                case BS_ITEM_TYPE_DIELECTRIC:
                    thickness += item->GetThickness();
                    break;

                case BS_ITEM_TYPE_COPPER:
                    if( item->GetBrdLayerId() == F_Cu )
                        m_frontCopperThickness3DU = item->GetThickness() * m_biuTo3Dunits;
                    else if( item->GetBrdLayerId() == B_Cu )
                        m_backCopperThickness3DU = item->GetThickness() * m_biuTo3Dunits;
                    else if( item->IsEnabled() )
                        thickness += item->GetThickness();

                    break;

                default:
                    break;
                }
            }

            m_boardBodyThickness3DU = thickness * m_biuTo3Dunits;
        }
    }

    // Init  Z position of each layer
    // calculate z position for each copper layer
    // Zstart = -m_epoxyThickness / 2.0 is the z position of the back (bottom layer) (layer id = 31)
    // Zstart = +m_epoxyThickness / 2.0 is the z position of the front (top layer) (layer id = 0)

    //  ____==__________==________==______ <- Bottom = +m_epoxyThickness / 2.0,
    // |                                  |   Top = Bottom + m_copperThickness
    // |__________________________________|
    //   ==         ==         ==     ==   <- Bottom = -m_epoxyThickness / 2.0,
    //                                        Top = Bottom - m_copperThickness

    unsigned int layer;

    for( layer = 0; layer < m_copperLayersCount; ++layer )
    {
        // This approximates internal layer positions (because we're treating all the dielectric
        // layers as having the same thickness).  But we don't render them anyway so it doesn't
        // really matter.
        m_layerZcoordBottom[layer] = m_boardBodyThickness3DU / 2.0f -
                                     (m_boardBodyThickness3DU * layer / (m_copperLayersCount - 1) );

        if( layer < (m_copperLayersCount / 2) )
            m_layerZcoordTop[layer] = m_layerZcoordBottom[layer] + m_frontCopperThickness3DU;
        else
            m_layerZcoordTop[layer] = m_layerZcoordBottom[layer] - m_backCopperThickness3DU;
    }

    #define layerThicknessMargin 1.1
    const float zpos_offset = m_nonCopperLayerThickness3DU * layerThicknessMargin;

    // Fill remaining unused copper layers and back layer zpos with -m_boardBodyThickness / 2.0
    for( ; layer < MAX_CU_LAYERS; layer++ )
    {
        m_layerZcoordBottom[layer] = -( m_boardBodyThickness3DU / 2.0f );
        m_layerZcoordTop[layer]    = -( m_boardBodyThickness3DU / 2.0f ) - m_backCopperThickness3DU;
    }

    // This is the top of the copper layer thickness.
    const float zpos_copperTop_back  = m_layerZcoordTop[B_Cu];
    const float zpos_copperTop_front = m_layerZcoordTop[F_Cu];

    // calculate z position for each non copper layer
    // Solder mask and Solder paste have the same Z position
    for( int layer_id = MAX_CU_LAYERS; layer_id < PCB_LAYER_ID_COUNT; ++layer_id )
    {
        float zposTop;
        float zposBottom;

        switch( layer_id )
        {
        case B_Adhes:
            zposBottom = zpos_copperTop_back - 2.0f * zpos_offset;
            zposTop    = zposBottom - m_nonCopperLayerThickness3DU;
            break;

        case F_Adhes:
            zposBottom = zpos_copperTop_front + 2.0f * zpos_offset;
            zposTop    = zposBottom + m_nonCopperLayerThickness3DU;
            break;

        case B_Mask:
            zposBottom = zpos_copperTop_back;
            zposTop    = zpos_copperTop_back - m_nonCopperLayerThickness3DU;
            break;

        case B_Paste:
            zposBottom = zpos_copperTop_back;
            zposTop    = zpos_copperTop_back - m_solderPasteLayerThickness3DU;
            break;

        case F_Mask:
            zposBottom = zpos_copperTop_front;
            zposTop    = zpos_copperTop_front + m_nonCopperLayerThickness3DU;
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

        // !TODO: review
        default:
            zposTop = zpos_copperTop_front + (layer_id - MAX_CU_LAYERS + 3.0f) * zpos_offset;
            zposBottom = zposTop - m_nonCopperLayerThickness3DU;
            break;
        }

        m_layerZcoordTop[layer_id]    = zposTop;
        m_layerZcoordBottom[layer_id] = zposBottom;
    }

    m_boardCenter = SFVEC3F( m_boardPos.x * m_biuTo3Dunits, m_boardPos.y * m_biuTo3Dunits, 0.0f );

    SFVEC3F boardSize = SFVEC3F( m_boardSize.x * m_biuTo3Dunits, m_boardSize.y * m_biuTo3Dunits,
                                 0.0f );
    boardSize /= 2.0f;

    SFVEC3F boardMin = ( m_boardCenter - boardSize );
    SFVEC3F boardMax = ( m_boardCenter + boardSize );

    boardMin.z = m_layerZcoordTop[B_Adhes];
    boardMax.z = m_layerZcoordTop[F_Adhes];

    m_boardBoundingBox = BBOX_3D( boardMin, boardMax );

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCreateBoardPolyTime = GetRunningMicroSecs();
#endif

    if( aStatusReporter )
        aStatusReporter->Report( _( "Create layers" ) );

    createLayers( aStatusReporter );

    COLOR_SETTINGS* colors = Pgm().GetSettingsManager().GetColorSettings();

    auto to_SFVEC4F =
            []( const COLOR4D& src )
            {
                return SFVEC4F( src.r, src.g, src.b, src.a );
            };

    m_BgColorTop = to_SFVEC4F( colors->GetColor( LAYER_3D_BACKGROUND_TOP ) );
    m_BgColorBot = to_SFVEC4F( colors->GetColor( LAYER_3D_BACKGROUND_BOTTOM ) );

    m_SolderPasteColor = to_SFVEC4F( colors->GetColor( LAYER_3D_SOLDERPASTE ) );

    if( m_board && colors->GetUseBoardStackupColors() )
    {
        const BOARD_STACKUP& stackup = m_board->GetDesignSettings().GetStackupDescriptor();

        auto findColor =
                []( const wxString& aColorName, const CUSTOM_COLORS_LIST& aColorSet )
                {
                    if( aColorName.StartsWith( "#" ) )
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

        m_SilkScreenColorTop = to_SFVEC4F( g_DefaultSilkscreen );
        m_SilkScreenColorBot = to_SFVEC4F( g_DefaultSilkscreen );
        m_SolderMaskColorTop = to_SFVEC4F( g_DefaultSolderMask );
        m_SolderMaskColorBot = to_SFVEC4F( g_DefaultSolderMask );

        KIGFX::COLOR4D bodyColor( 0, 0, 0, 0 );

        for( const BOARD_STACKUP_ITEM* stackupItem : stackup.GetList() )
        {
            wxString colorName = stackupItem->GetColor();

            switch( stackupItem->GetType() )
            {
            case BS_ITEM_TYPE_SILKSCREEN:
                if( stackupItem->GetBrdLayerId() == F_SilkS )
                    m_SilkScreenColorTop = to_SFVEC4F( findColor( colorName, g_SilkscreenColors ) );
                else
                    m_SilkScreenColorBot = to_SFVEC4F( findColor( colorName, g_SilkscreenColors ) );
                break;

            case BS_ITEM_TYPE_SOLDERMASK:
                if( stackupItem->GetBrdLayerId() == F_Mask )
                    m_SolderMaskColorTop = to_SFVEC4F( findColor( colorName, g_MaskColors ) );
                else
                    m_SolderMaskColorBot = to_SFVEC4F( findColor( colorName, g_MaskColors ) );

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
            m_BoardBodyColor = to_SFVEC4F( bodyColor );
        else
            m_BoardBodyColor = to_SFVEC4F( g_DefaultBoardBody );

        const wxString& finishName = stackup.m_FinishType;

        if( finishName.EndsWith( "OSP" ) )
        {
            m_CopperColor = to_SFVEC4F( findColor( "Copper", g_FinishColors ) );
        }
        else if( finishName.EndsWith( "IG" )
              || finishName.EndsWith( "gold" ) )
        {
            m_CopperColor = to_SFVEC4F( findColor( "Gold", g_FinishColors ) );
        }
        else if( finishName.StartsWith( "HAL" )
              || finishName.StartsWith( "HASL" )
              || finishName.EndsWith( "tin" )
              || finishName.EndsWith( "nickel" ) )
        {
            m_CopperColor = to_SFVEC4F( findColor( "Tin", g_FinishColors ) );
        }
        else if( finishName.EndsWith( "silver" ) )
        {
            m_CopperColor = to_SFVEC4F( findColor( "Silver", g_FinishColors ) );
        }
        else
        {
            m_CopperColor = to_SFVEC4F( g_DefaultSurfaceFinish );
        }
    }
    else
    {
        m_SilkScreenColorBot = to_SFVEC4F( colors->GetColor( LAYER_3D_SILKSCREEN_BOTTOM ) );
        m_SilkScreenColorTop = to_SFVEC4F( colors->GetColor( LAYER_3D_SILKSCREEN_TOP ) );
        m_SolderMaskColorBot = to_SFVEC4F( colors->GetColor( LAYER_3D_SOLDERMASK_BOTTOM ) );
        m_SolderMaskColorTop = to_SFVEC4F( colors->GetColor( LAYER_3D_SOLDERMASK_TOP ) );
        m_CopperColor        = to_SFVEC4F( colors->GetColor( LAYER_3D_COPPER ) );
        m_BoardBodyColor     = to_SFVEC4F( colors->GetColor( LAYER_3D_BOARD ) );
    }
}


extern bool BuildFootprintPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                           int aErrorMax, int aChainingEpsilon,
                                           OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );


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

        int chainingEpsilon = Millimeter2iu( 0.02 );  // max dist from one endPt to next startPt

        success = BuildFootprintPolygonOutlines( m_board, m_board_poly,
                                                 m_board->GetDesignSettings().m_MaxError,
                                                 chainingEpsilon );

        // Make polygon strictly simple to avoid issues (especially in 3D viewer)
        m_board_poly.Simplify( SHAPE_POLY_SET::PM_STRICTLY_SIMPLE );

        if( !success && aErrorMsg )
        {
            *aErrorMsg = _( "Footprint outline is missing or malformed. Run Footprint Checker for "
                            "a full analysis." );
        }
    }
    else
    {
        success = m_board->GetBoardPolygonOutlines( m_board_poly );

        if( !success && aErrorMsg )
            *aErrorMsg = _( "Board outline is missing or malformed. Run DRC for a full analysis." );
    }

    return success;
}


float BOARD_ADAPTER::GetFootprintZPos( bool aIsFlipped ) const
{
    if( aIsFlipped )
    {
        if( m_Cfg->m_Render.show_solderpaste )
            return m_layerZcoordBottom[B_SilkS];
        else
            return m_layerZcoordBottom[B_Paste];
    }
    else
    {
        if( m_Cfg->m_Render.show_solderpaste )
            return m_layerZcoordTop[F_SilkS];
        else
            return m_layerZcoordTop[F_Paste];
    }
}


SFVEC4F BOARD_ADAPTER::GetLayerColor( PCB_LAYER_ID aLayerId ) const
{
    wxASSERT( aLayerId < PCB_LAYER_ID_COUNT );

    const COLOR4D color = m_colors->GetColor( aLayerId );

    return SFVEC4F( color.r, color.g, color.b, color.a );
}


SFVEC4F BOARD_ADAPTER::GetItemColor( int aItemId ) const
{
    return GetColor( m_colors->GetColor( aItemId ) );
}


SFVEC4F BOARD_ADAPTER::GetColor( const COLOR4D& aColor ) const
{
    return SFVEC4F( aColor.r, aColor.g, aColor.b, aColor.a );
}


SFVEC2F BOARD_ADAPTER::GetSphericalCoord( int i ) const
{
    SFVEC2F sphericalCoord =
            SFVEC2F( ( m_Cfg->m_Render.raytrace_lightElevation[i] + 90.0f ) / 180.0f,
                       m_Cfg->m_Render.raytrace_lightAzimuth[i] / 180.0f );

    sphericalCoord.x = glm::clamp( sphericalCoord.x, 0.0f, 1.0f );
    sphericalCoord.y = glm::clamp( sphericalCoord.y, 0.0f, 2.0f );

    return sphericalCoord;
}

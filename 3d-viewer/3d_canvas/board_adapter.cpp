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

/**
 * @file  cinfo3d_visu.cpp
 * @brief Handles data related with the board to be visualized
 */

#include "../3d_rendering/camera.h"
#include "board_adapter.h"
#include <3d_rendering/3d_render_raytracing/shapes2D/polygon_2d.h>
#include <board.h>
#include <3d_math.h>
#include "3d_fastmath.h"
#include <geometry/geometry_utils.h>
#include <math/util.h>      // for KiROUND
#include <pgm_base.h>
#include <settings/settings_manager.h>

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
        m_board( nullptr ),
        m_3dModelManager( nullptr ),
        m_colors( nullptr ),
        m_layerZcoordTop(),
        m_layerZcoordBottom()
{
    wxLogTrace( m_logTrace, wxT( "BOARD_ADAPTER::BOARD_ADAPTER" ) );

    m_gridType     = GRID3D_TYPE::NONE;
    m_antiAliasingMode = ANTIALIASING_MODE::AA_8X;
    m_drawFlags.resize( FL_LAST, false );

    if( PgmOrNull() )
        m_colors = Pgm().GetSettingsManager().GetColorSettings();

    m_renderEngine = RENDER_ENGINE::OPENGL_LEGACY;
    m_materialMode = MATERIAL_MODE::NORMAL;

    m_boardPos = wxPoint();
    m_boardSize = wxSize();
    m_boardCenter = SFVEC3F( 0.0f );

    m_boardBoundingBox.Reset();

    m_throughHoleIds.Clear();
    m_throughHoleOds.Clear();
    m_throughHoleAnnularRings.Clear();

    m_copperLayersCount = -1;
    m_epoxyThickness3DU = 0.0f;
    m_copperThickness3DU  = 0.0f;
    m_nonCopperLayerThickness3DU = 0.0f;
    m_biuTo3Dunits = 1.0;

    m_trackCount = 0;
    m_viaCount = 0;
    m_averageViaHoleDiameter = 0.0f;
    m_holeCount = 0;
    m_averageHoleDiameter = 0.0f;
    m_averageTrackWidth = 0.0f;

    SetFlag( FL_USE_REALISTIC_MODE, true );
    SetFlag( FL_FP_ATTRIBUTES_NORMAL, true );
    SetFlag( FL_SHOW_BOARD_BODY, true );
    SetFlag( FL_CLIP_SILK_ON_VIA_ANNULUS, false );
    SetFlag( FL_FP_ATTRIBUTES_NORMAL, true );
    SetFlag( FL_FP_ATTRIBUTES_NORMAL_INSERT, true );
    SetFlag( FL_FP_ATTRIBUTES_VIRTUAL, true );
    SetFlag( FL_ZONE, true );
    SetFlag( FL_SILKSCREEN, true );
    SetFlag( FL_SOLDERMASK, true );
    SetFlag( FL_SUBTRACT_MASK_FROM_SILK, false );

    SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, true );
    SetFlag( FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE, false );
    SetFlag( FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE, false );
    SetFlag( FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE, false );
    SetFlag( FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE, false );

    SetFlag( FL_USE_SELECTION, true );

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

    // Avoid raytracing options not initialized:
    m_RtShadowSampleCount = 0;
    m_RtReflectionSampleCount = 0;
    m_RtRefractionSampleCount = 0;

    m_RtSpreadShadows = 0.0;
    m_RtSpreadReflections = 0.0;
    m_RtSpreadRefractions = 0.0;

    m_RtRecursiveReflectionCount = 0;
    m_RtRecursiveRefractionCount = 0;
}


BOARD_ADAPTER::~BOARD_ADAPTER()
{
    destroyLayers();
}


bool BOARD_ADAPTER::Is3dLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    wxASSERT( aLayer < PCB_LAYER_ID_COUNT );

    if( !m_board->IsLayerEnabled( aLayer ) )
        return false;

    // see if layer needs to be shown
    // check the flags
    switch( aLayer )
    {
    case B_Adhes:
    case F_Adhes:
        return GetFlag( FL_ADHESIVE );

    case B_Paste:
    case F_Paste:
        return GetFlag( FL_SOLDERPASTE );

    case B_SilkS:
    case F_SilkS:
        return GetFlag( FL_SILKSCREEN );

    case B_Mask:
    case F_Mask:
        return GetFlag( FL_SOLDERMASK );

    case Dwgs_User:
    case Cmts_User:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return GetFlag( FL_COMMENTS );

    case Eco1_User:
    case Eco2_User:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return GetFlag( FL_ECO );

    case Edge_Cuts:
        if( GetFlag( FL_SHOW_BOARD_BODY ) || GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return true;

    case Margin:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return true;

    case B_Cu:
    case F_Cu:
        return m_board->IsLayerVisible( aLayer ) || GetFlag( FL_USE_REALISTIC_MODE );

    default:
        // the layer is an internal copper layer, used the visibility
        return m_board->IsLayerVisible( aLayer );
    }
}


bool BOARD_ADAPTER::GetFlag( DISPLAY3D_FLG aFlag ) const
{
    wxASSERT( aFlag < FL_LAST );

    return m_drawFlags[aFlag];
}


void BOARD_ADAPTER::SetFlag( DISPLAY3D_FLG aFlag, bool aState )
{
    wxASSERT( aFlag < FL_LAST );

    m_drawFlags[aFlag] = aState;
}


bool BOARD_ADAPTER::IsFootprintShown( FOOTPRINT_ATTR_T aFPAttributes ) const
{
    if( aFPAttributes & FP_SMD )
        return GetFlag( FL_FP_ATTRIBUTES_NORMAL_INSERT );
    else if( aFPAttributes & FP_THROUGH_HOLE )
        return GetFlag( FL_FP_ATTRIBUTES_NORMAL );
    else
        return GetFlag( FL_FP_ATTRIBUTES_VIRTUAL );
}


// !TODO: define the actual copper thickness by user
#define COPPER_THICKNESS KiROUND( 0.035 * IU_PER_MM )   // for 35 um
#define TECH_LAYER_THICKNESS KiROUND( 0.04 * IU_PER_MM )


int BOARD_ADAPTER::GetHolePlatingThickness() const noexcept
{
    return m_board->GetDesignSettings().GetHolePlatingThickness();
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

    const bool succeedToGetBoardPolygon = createBoardPolygon( &msg );

    if( aWarningReporter )
    {
        if( !succeedToGetBoardPolygon )
            aWarningReporter->Report( msg, RPT_SEVERITY_WARNING );
        else
            aWarningReporter->Report( wxEmptyString );
    }

    // Calculates the board bounding box (board outlines + items)
    // to ensure any item, even outside the board outlines can be seen
    bool boardEdgesOnly = true;

    if( m_board->IsFootprintHolder() || !GetFlag( FL_USE_REALISTIC_MODE )
            || !succeedToGetBoardPolygon )
        boardEdgesOnly = false;

    EDA_RECT bbbox = m_board->ComputeBoundingBox( boardEdgesOnly );

    // Gives a non null size to avoid issues in zoom / scale calculations
    if( ( bbbox.GetWidth() == 0 ) && ( bbbox.GetHeight() == 0 ) )
        bbbox.Inflate( Millimeter2iu( 10 ) );

    m_boardSize = bbbox.GetSize();
    m_boardPos  = bbbox.Centre();

    wxASSERT( (m_boardSize.x > 0) && (m_boardSize.y > 0) );

    m_boardPos.y = -m_boardPos.y; // The y coord is inverted in 3D viewer

    m_copperLayersCount = m_board->GetCopperLayerCount();

    // Ensure the board has 2 sides for 3D views, because it is hard to find
    // a *really* single side board in the true life...
    if( m_copperLayersCount < 2 )
        m_copperLayersCount = 2;

    // Calculate the conversion to apply to all positions.
    m_biuTo3Dunits = RANGE_SCALE_3D / std::max( m_boardSize.x, m_boardSize.y );

    m_epoxyThickness3DU = m_board->GetDesignSettings().GetBoardThickness() * m_biuTo3Dunits;

    // !TODO: use value defined by user (currently use default values by ctor
    m_copperThickness3DU         = COPPER_THICKNESS     * m_biuTo3Dunits;
    m_nonCopperLayerThickness3DU = TECH_LAYER_THICKNESS * m_biuTo3Dunits;

    // Init  Z position of each layer
    // calculate z position for each copper layer
    // Zstart = -m_epoxyThickness / 2.0 is the z position of the back (bottom layer) (layer id = 31)
    // Zstart = +m_epoxyThickness / 2.0 is the z position of the front (top layer) (layer id = 0)
    // all unused copper layer z position are set to 0

    //  ____==__________==________==______ <- Bottom = +m_epoxyThickness / 2.0,
    // |                                  |   Top = Bottom + m_copperThickness
    // |__________________________________|
    //   ==         ==         ==     ==   <- Bottom = -m_epoxyThickness / 2.0,
    //                                        Top = Bottom - m_copperThickness

    unsigned int layer;

    for( layer = 0; layer < m_copperLayersCount; ++layer )
    {
        m_layerZcoordBottom[layer] = m_epoxyThickness3DU / 2.0f -
                                     (m_epoxyThickness3DU * layer / (m_copperLayersCount - 1) );

        if( layer < (m_copperLayersCount / 2) )
            m_layerZcoordTop[layer] = m_layerZcoordBottom[layer] + m_copperThickness3DU;
        else
            m_layerZcoordTop[layer] = m_layerZcoordBottom[layer] - m_copperThickness3DU;
    }

    #define layerThicknessMargin 1.1
    const float zpos_offset = m_nonCopperLayerThickness3DU * layerThicknessMargin;

    // Fill remaining unused copper layers and back layer zpos
    // with -m_epoxyThickness / 2.0
    for( ; layer < MAX_CU_LAYERS; layer++ )
    {
        m_layerZcoordBottom[layer] = -(m_epoxyThickness3DU / 2.0f);
        m_layerZcoordTop[layer]    = -(m_epoxyThickness3DU / 2.0f) - m_copperThickness3DU;
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
        case B_Paste:
            zposBottom = zpos_copperTop_back;
            zposTop    = zpos_copperTop_back - m_nonCopperLayerThickness3DU;
            break;

        case F_Mask:
        case F_Paste:
            zposTop    = zpos_copperTop_front + m_nonCopperLayerThickness3DU;
            zposBottom = zpos_copperTop_front;
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
}


extern bool BuildFootprintPolygonOutlines( BOARD* aBoard, SHAPE_POLY_SET& aOutlines,
                                           int aErrorMax, int aChainingEpsilon,
                                           OUTLINE_ERROR_HANDLER* aErrorHandler = nullptr );


bool BOARD_ADAPTER::createBoardPolygon( wxString* aErrorMsg )
{
    m_board_poly.RemoveAllContours();

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
        if( GetFlag( FL_SOLDERPASTE ) )
            return m_layerZcoordBottom[B_SilkS];
        else
            return m_layerZcoordBottom[B_Paste];
    }
    else
    {
        if( GetFlag( FL_SOLDERPASTE ) )
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


SFVEC4F BOARD_ADAPTER::GetColor( COLOR4D aColor ) const
{
    return SFVEC4F( aColor.r, aColor.g, aColor.b, aColor.a );
}

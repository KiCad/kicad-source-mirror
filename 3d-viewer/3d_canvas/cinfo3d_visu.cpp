/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "../3d_rendering/ccamera.h"
#include "cinfo3d_visu.h"
#include <3d_rendering/3d_render_raytracing/shapes2D/cpolygon2d.h>
#include <class_board.h>
#include <3d_math.h>
#include "3d_fastmath.h"
#include <geometry/geometry_utils.h>

/**
 *  Trace mask used to enable or disable the trace output of this class.
 *  The debug output can be turned on by setting the WXTRACE environment variable to
 *  "KI_TRACE_EDA_CINFO3D_VISU".  See the wxWidgets documentation on wxLogTrace for
 *  more information.
 */
const wxChar *CINFO3D_VISU::m_logTrace = wxT( "KI_TRACE_EDA_CINFO3D_VISU" );


CINFO3D_VISU G_null_CINFO3D_VISU;


CINFO3D_VISU::CINFO3D_VISU() :
    m_currentCamera( m_trackBallCamera ),
    m_trackBallCamera( RANGE_SCALE_3D )
{
    wxLogTrace( m_logTrace, wxT( "CINFO3D_VISU::CINFO3D_VISU" ) );

    m_board = NULL;
    m_3d_model_manager = NULL;
    m_3D_grid_type = GRID3D_NONE;
    m_drawFlags.resize( FL_LAST, false );

    m_render_engine = RENDER_ENGINE_OPENGL_LEGACY;
    m_material_mode = MATERIAL_MODE_NORMAL;

    m_boardPos = wxPoint();
    m_boardSize = wxSize();
    m_boardCenter = SFVEC3F( 0.0f );

    m_boardBoudingBox.Reset();

    m_layers_container2D.clear();
    m_layers_holes2D.clear();
    m_through_holes_inner.Clear();
    m_through_holes_outer.Clear();

    m_copperLayersCount = -1;
    m_epoxyThickness3DU = 0.0f;
    m_copperThickness3DU  = 0.0f;
    m_nonCopperLayerThickness3DU = 0.0f;
    m_biuTo3Dunits = 1.0;

    m_stats_nr_tracks = 0;
    m_stats_nr_vias = 0;
    m_stats_via_med_hole_diameter = 0.0f;
    m_stats_nr_holes = 0;
    m_stats_hole_med_diameter = 0.0f;
    m_stats_track_med_width = 0.0f;

    m_calc_seg_min_factor3DU = 0.0f;
    m_calc_seg_max_factor3DU = 0.0f;


    memset( m_layerZcoordTop, 0, sizeof( m_layerZcoordTop ) );
    memset( m_layerZcoordBottom, 0, sizeof( m_layerZcoordBottom ) );

    SetFlag( FL_USE_REALISTIC_MODE, true );
    SetFlag( FL_MODULE_ATTRIBUTES_NORMAL, true );
    SetFlag( FL_SHOW_BOARD_BODY, true );
    SetFlag( FL_RENDER_OPENGL_COPPER_THICKNESS, true );
    SetFlag( FL_MODULE_ATTRIBUTES_NORMAL, true );
    SetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT, true );
    SetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL, true );
    SetFlag( FL_ZONE, true );
    SetFlag( FL_SILKSCREEN, true );
    SetFlag( FL_SOLDERMASK, true );

    m_BgColorBot        = SFVEC3D( 0.4, 0.4, 0.5 );
    m_BgColorTop        = SFVEC3D( 0.8, 0.8, 0.9 );
    m_BoardBodyColor    = SFVEC3D( 0.4, 0.4, 0.5 );
    m_SolderMaskColor   = SFVEC3D( 0.1, 0.2, 0.1 );
    m_SolderPasteColor  = SFVEC3D( 0.4, 0.4, 0.4 );
    m_SilkScreenColor   = SFVEC3D( 0.9, 0.9, 0.9 );
    m_CopperColor       = SFVEC3D( 0.75, 0.61, 0.23 );
}


CINFO3D_VISU::~CINFO3D_VISU()
{
    destroyLayers();
}


bool CINFO3D_VISU::Is3DLayerEnabled( PCB_LAYER_ID aLayer ) const
{
    wxASSERT( aLayer < PCB_LAYER_ID_COUNT );

    DISPLAY3D_FLG flg;

    // see if layer needs to be shown
    // check the flags
    switch( aLayer )
    {
    case B_Adhes:
    case F_Adhes:
        flg = FL_ADHESIVE;
        break;

    case B_Paste:
    case F_Paste:
        flg = FL_SOLDERPASTE;
        break;

    case B_SilkS:
    case F_SilkS:
        flg = FL_SILKSCREEN;
        break;

    case B_Mask:
    case F_Mask:
        flg = FL_SOLDERMASK;
        break;

    case Dwgs_User:
    case Cmts_User:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        flg = FL_COMMENTS;
        break;

    case Eco1_User:
    case Eco2_User:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        flg = FL_ECO;
        break;

    case Edge_Cuts:
        if( GetFlag( FL_SHOW_BOARD_BODY ) || GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return true;
        break;

    case Margin:
        if( GetFlag( FL_USE_REALISTIC_MODE ) )
            return false;

        return true;
        break;

    case B_Cu:
    case F_Cu:
        return m_board->GetDesignSettings().IsLayerVisible( aLayer ) ||
               GetFlag( FL_USE_REALISTIC_MODE );
        break;

    default:
        // the layer is an internal copper layer, used the visibility
        if( GetFlag( FL_SHOW_BOARD_BODY ) &&
            (m_render_engine == RENDER_ENGINE_OPENGL_LEGACY) )
        {
            // Do not render internal layers if it is overlap with the board
            // (on OpenGL render)
            return false;
        }

        return m_board->GetDesignSettings().IsLayerVisible( aLayer );
    }

    // The layer has a flag, return the flag
    return GetFlag( flg );
}


bool CINFO3D_VISU::GetFlag( DISPLAY3D_FLG aFlag ) const
{
    wxASSERT( aFlag < FL_LAST );

    return m_drawFlags[aFlag];
}


void CINFO3D_VISU::SetFlag( DISPLAY3D_FLG aFlag, bool aState )
{
    wxASSERT( aFlag < FL_LAST );

    m_drawFlags[aFlag] = aState;
}

bool CINFO3D_VISU::ShouldModuleBeDisplayed( MODULE_ATTR_T aModuleAttributs ) const
{
    if( ( ( aModuleAttributs == MOD_DEFAULT ) &&
          GetFlag( FL_MODULE_ATTRIBUTES_NORMAL ) ) ||
        ( ( ( aModuleAttributs & MOD_CMS) == MOD_CMS ) &&
          GetFlag( FL_MODULE_ATTRIBUTES_NORMAL_INSERT ) ) ||
        ( ( ( aModuleAttributs & MOD_VIRTUAL) == MOD_VIRTUAL ) &&
          GetFlag( FL_MODULE_ATTRIBUTES_VIRTUAL ) ) )
    {
        return true;
    }

    return false;
}


// !TODO: define the actual copper thickness by user
#define COPPER_THICKNESS KiROUND( 0.035 * IU_PER_MM )   // for 35 um
#define TECH_LAYER_THICKNESS KiROUND( 0.04 * IU_PER_MM )

int CINFO3D_VISU::GetCopperThicknessBIU() const
{
    return COPPER_THICKNESS;
}

unsigned int CINFO3D_VISU::GetNrSegmentsCircle( float aDiameter3DU ) const
{
    wxASSERT( aDiameter3DU > 0.0f );

    return GetNrSegmentsCircle( (int)( aDiameter3DU / m_biuTo3Dunits ) );
}


unsigned int CINFO3D_VISU::GetNrSegmentsCircle( int aDiameterBIU ) const
{
    wxASSERT( aDiameterBIU > 0 );

    // Require at least 3 segments for a circle
    return std::max( GetArcToSegmentCount( aDiameterBIU / 2, ARC_HIGH_DEF, 360.0 ), 3 );
}


double CINFO3D_VISU::GetCircleCorrectionFactor( int aNrSides ) const
{
    wxASSERT( aNrSides >= 3 );

    return GetCircletoPolyCorrectionFactor( aNrSides );
}


void CINFO3D_VISU::InitSettings( REPORTER *aStatusTextReporter )
{
    wxLogTrace( m_logTrace, wxT( "CINFO3D_VISU::InitSettings" ) );

    // Calculates the board bounding box
    // First, use only the board outlines
    EDA_RECT bbbox = m_board->ComputeBoundingBox( true );

    // If no outlines, use the board with items
    if( ( bbbox.GetWidth() == 0 ) && ( bbbox.GetHeight() == 0 ) )
        bbbox = m_board->ComputeBoundingBox( false );

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

    // Calculate the convertion to apply to all positions.
    m_biuTo3Dunits = RANGE_SCALE_3D / std::max( m_boardSize.x, m_boardSize.y );

    m_epoxyThickness3DU = m_board->GetDesignSettings().GetBoardThickness() *
                          m_biuTo3Dunits;

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

    m_boardCenter = SFVEC3F( m_boardPos.x * m_biuTo3Dunits,
                             m_boardPos.y * m_biuTo3Dunits,
                             0.0f );

    SFVEC3F boardSize = SFVEC3F( m_boardSize.x * m_biuTo3Dunits,
                                 m_boardSize.y * m_biuTo3Dunits,
                                 0.0f );
            boardSize /= 2.0f;

    SFVEC3F boardMin = (m_boardCenter - boardSize);
    SFVEC3F boardMax = (m_boardCenter + boardSize);

    boardMin.z = m_layerZcoordTop[B_Adhes];
    boardMax.z = m_layerZcoordTop[F_Adhes];

    m_boardBoudingBox = CBBOX( boardMin, boardMax );

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_startCreateBoardPolyTime = GetRunningMicroSecs();
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Build board body" ) );

    createBoardPolygon();

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_stopCreateBoardPolyTime = GetRunningMicroSecs();
    unsigned stats_startCreateLayersTime = stats_stopCreateBoardPolyTime;
#endif

    if( aStatusTextReporter )
        aStatusTextReporter->Report( _( "Create layers" ) );

    createLayers( aStatusTextReporter );

#ifdef PRINT_STATISTICS_3D_VIEWER
    unsigned stats_stopCreateLayersTime = GetRunningMicroSecs();

    printf( "CINFO3D_VISU::InitSettings times\n" );
    printf( "  CreateBoardPoly:          %.3f ms\n",
            (float)( stats_stopCreateBoardPolyTime  - stats_startCreateBoardPolyTime  ) / 1e3 );
    printf( "  CreateLayers and holes:   %.3f ms\n",
            (float)( stats_stopCreateLayersTime     - stats_startCreateLayersTime     ) / 1e3 );
    printf( "\n" );
#endif
}


void CINFO3D_VISU::createBoardPolygon()
{
    m_board_poly.RemoveAllContours();

    wxString errmsg;

    if( !m_board->GetBoardPolygonOutlines( m_board_poly, /*allLayerHoles,*/ &errmsg ) )
    {
        errmsg.append( wxT( "\n\n" ) );
        errmsg.append( _( "Cannot determine the board outline." ) );
        wxLogMessage( errmsg );
    }
}


float CINFO3D_VISU::GetModulesZcoord3DIU( bool aIsFlipped ) const
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


void CINFO3D_VISU::CameraSetType( CAMERA_TYPE aCameraType )
{
    switch( aCameraType )
    {
    case CAMERA_TRACKBALL:
        m_currentCamera = m_trackBallCamera;
    break;

    default:
        wxLogMessage( wxT( "CINFO3D_VISU::CameraSetType() error: unknown camera type %d" ),
                      (int)aCameraType );
        break;
    }
}


SFVEC3F CINFO3D_VISU::GetLayerColor( PCB_LAYER_ID aLayerId ) const
{
    wxASSERT( aLayerId < PCB_LAYER_ID_COUNT );

    const COLOR4D color = m_board->Colors().GetLayerColor( aLayerId );

    return SFVEC3F( color.r, color.g, color.b );
}


SFVEC3F CINFO3D_VISU::GetItemColor( int aItemId ) const
{
    return GetColor( m_board->Colors().GetItemColor( aItemId ) );
}


SFVEC3F CINFO3D_VISU::GetColor( COLOR4D aColor ) const
{
    return SFVEC3F( aColor.r, aColor.g, aColor.b );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  cinfo3d_visu.h
 * @brief Handles data related with the board to be visualized
 */

#ifndef CINFO3D_VISU_H
#define CINFO3D_VISU_H

#include <vector>
#include "../3d_rendering/3d_render_raytracing/accelerators/ccontainer2d.h"
#include "../3d_rendering/3d_render_raytracing/accelerators/ccontainer.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/cbbox.h"
#include "../3d_rendering/ccamera.h"
#include "../3d_rendering/ctrack_ball.h"
#include <layers_id_colors_and_visibility.h>
#include <class_pad.h>
#include <class_track.h>
#include <wx/gdicmn.h>
#include <wxBasePcbFrame.h>
#include <clipper.hpp>
#include <class_pcb_text.h>
#include <class_drawsegment.h>
#include <class_zone.h>


/**
 *  Flags used in rendering options
 */
enum DISPLAY3D_FLG {
    FL_AXIS=0, FL_MODULE, FL_ZONE,
    FL_ADHESIVE, FL_SILKSCREEN, FL_SOLDERMASK, FL_SOLDERPASTE,
    FL_COMMENTS, FL_ECO,
    FL_USE_COPPER_THICKNESS,
    FL_SHOW_BOARD_BODY,
    FL_USE_REALISTIC_MODE,
    FL_RENDER_SHADOWS,
    FL_RENDER_SHOW_HOLES_IN_ZONES,
    FL_RENDER_TEXTURES,
    FL_RENDER_SMOOTH_NORMALS,
    FL_RENDER_USE_MODEL_NORMALS,
    FL_RENDER_MATERIAL,
    FL_RENDER_SHOW_MODEL_BBOX,
    FL_LAST
};


/**
 *  Camera types
 */
enum CAMERA_TYPE
{
    CAMERA_TRACKBALL
};


/**
 *  Grid types
 */
enum GRID3D_TYPE
{
    GRID3D_NONE,
    GRID3D_1MM,
    GRID3D_2P5MM,
    GRID3D_5MM,
    GRID3D_10MM
};


/// A type that stores a container of 2d objects for each layer id
typedef std::map< LAYER_ID, CBVHCONTAINER2D *> MAP_CONTAINER_2D;


/// This defines the range that all coord will have to be rendered.
/// It will use this value to convert to a normalized value between
/// -(RANGE_SCALE_3D/2) .. +(RANGE_SCALE_3D/2)
#define RANGE_SCALE_3D 8.0f


/**
 *  Class CINFO3D_VISU
 *  Helper class to handle information needed to display 3D board
 */
class CINFO3D_VISU
{
 public:

    CINFO3D_VISU();

    ~CINFO3D_VISU();

    /**
     * @brief GetFlag - get a configuration status of a flag
     * @param aFlag: the flag to get the status
     * @return true if flag is set, false if not
     */
    bool GetFlag( DISPLAY3D_FLG aFlag ) const ;

    /**
     * @brief SetFlag - set the status of a flag
     * @param aFlag: the flag to get the status
     * @param aState: status to set
     */
    void SetFlag( DISPLAY3D_FLG aFlag, bool aState );

    /**
     * @brief Is3DLayerEnabled - Check if a layer is enabled
     * @param aLayer: layer ID to get status
     * @return true if layer should be displayed, false if not
     */
    bool Is3DLayerEnabled( LAYER_ID aLayer ) const;

    /**
     * @brief SetBoard - Set current board to be rendered
     * @param aBoard: board to process
     */
    void SetBoard( BOARD *aBoard ) { m_board = aBoard; }

    /**
     * @brief GetBoard - Get current board to be rendered
     * @return BOARD pointer
     */
    const BOARD *GetBoard() const { return m_board; }

    /**
     * @brief InitSettings - Function to be called by the render when it need to
     * reload the settings for the board.
     */
    void InitSettings();

    /**
     * Function BiuTo3Dunits
     * @return the conversion factor to transform a position from the board to 3d units
     */
    double BiuTo3Dunits() const { return m_biuTo3Dunits; }

    /**
     * @brief GetBBox3DU - Get the bbox of the pcb board
     * @return
     */
    const CBBOX &GetBBox3DU() const { return m_boardBoudingBox; }

    /**
     * @brief GetEpoxyThickness3DU - Get the current epoxy thickness
     * @return thickness in 3d unities
     */
    float GetEpoxyThickness3DU() const { return m_epoxyThickness; }

    /**
     * @brief GetNonCopperLayerThickness3DU - Get the current non copper layers thickness
     * @return thickness in 3d unities of non copperlayers
     */
    float GetNonCopperLayerThickness3DU() const { return m_nonCopperLayerThickness; }

    /**
     * @brief GetCopperThickness3DU - Get the current copper layer thickness
     * @return thickness in 3d unities of copperlayers
     */
    float GetCopperThickness3DU() const { return m_copperThickness; }

    /**
     * @brief GetCopperThicknessBIU - Get the current copper layer thickness
     * @return thickness in board unities
     */
    int GetCopperThicknessBIU() const;

    /**
     * @brief GetBoardSize3DU - Get the board size
     * @return size in 3D unities
     */
    wxSize GetBoardSize3DU() const { return m_boardSize; }

    /**
     * Function GetBoardCenter
     * @return board center in 3d units
     */
    SFVEC3F &GetBoardCenter3DU() { return m_boardCenter; }

    /**
     *  @param aIsFlipped: true for use in modules on Front (top) layer, false
     *                     if module is on back (bottom) layer
     *  @return the Z position of 3D shapes, in 3D Units
     */
    float GetModulesZcoord3DIU( bool aIsFlipped ) const ;

    /**
     *  @param aCameraType: camera type to use in this canvas
     */
    void CameraSetType( CAMERA_TYPE aCameraType );

    /**
     * @brief CameraGet - get current camera in use
     * @return a camera
     */
    CCAMERA &CameraGet() const { return m_currentCamera; }

    /**
     *  Function GridGet
     *  @return space type of the grid
     */
    GRID3D_TYPE GridGet() const { return m_3D_Grid_type; }

    /**
     *  Function GridSet
     *  @param aGridType = the type space of the grid
     */
    void GridSet( GRID3D_TYPE aGridType ) { m_3D_Grid_type = aGridType; }

    /**
     * @brief GetBoardPoly - Get the current polygon of the epoxy board
     * @return the shape polygon
     */
    const SHAPE_POLY_SET &GetBoardPoly() const { return m_boardPoly; }

    /**
     * @brief GetLayerColor - get the technical color of a layer
     * @param aLayerId: the layer to get the color information
     * @return the color in SFVEC3F format
     */
    SFVEC3F GetLayerColor( LAYER_ID aLayerId ) const;

    /**
     * @brief GetItemColor - get the technical color of a layer
     * @param aItemId: the item id to get the color information
     * @return the color in SFVEC3F format
     */
    SFVEC3F GetItemColor( int aItemId ) const;

    /**
     * @brief GetLayerTopZpos3DU - Get the top z position
     * @param aLayerId: layer id
     * @return position in 3D unities
     */
    float GetLayerTopZpos3DU( LAYER_ID aLayerId ) const { return m_layerZcoordTop[aLayerId]; }

    /**
     * @brief GetLayerBottomZpos3DU - Get the bottom z position
     * @param aLayerId: layer id
     * @return position in 3D unities
     */
    float GetLayerBottomZpos3DU( LAYER_ID aLayerId ) const { return m_layerZcoordBottom[aLayerId]; }

    /**
     * @brief GetMapLayers - Get the map of container that have the objects per layer
     * @return the map containers of this board
     */
    const MAP_CONTAINER_2D &GetMapLayers() const { return m_layers_container2D; }

    /**
     * @brief GetMapLayersHoles -Get the map of container that have the holes per layer
     * @return the map containers of holes from this board
     */
    const MAP_CONTAINER_2D &GetMapLayersHoles() const { return m_layers_holes2D; }

    /**
     * @brief GetThroughHole_Inflated - Get the inflated ThroughHole container
     * @return a container with holes
     */
    const CBVHCONTAINER2D &GetThroughHole_Inflated() const { return m_throughHoles_inflated; }

    /**
     * @brief GetThroughHole - Get the ThroughHole container
     * @return a container with holes
     */
    const CBVHCONTAINER2D &GetThroughHole() const { return m_throughHoles; }

    unsigned int GetStats_Nr_Vias() const { return m_stats_nr_vias; }
    unsigned int GetStats_Nr_Holes() const { return m_stats_nr_holes; }

    float GetStats_Med_Via_Hole_Diameter3DU() const { return m_stats_via_med_hole_diameter; }
    float GetStats_Med_Hole_Diameter3DU() const { return m_stats_hole_med_diameter; }
    float GetStats_Med_Track_Width() const { return m_stats_track_med_width; }

 private:
    void createBoardPolygon();
    void createLayers();
    void destroyLayers();

    // Helper functions to create the board
    COBJECT2D *createNewTrack( const TRACK* aTrack , int aClearanceValue ) const;
    void createNewPad(const D_PAD* aPad, CGENERICCONTAINER2D *aDstContainer, const wxSize &aInflateValue ) const;
    void createNewPadWithClearance( const D_PAD* aPad, CGENERICCONTAINER2D *aDstContainer, int aClearanceValue ) const;
    COBJECT2D *createNewPadDrill(const D_PAD* aPad, int aInflateValue);
    void AddPadsShapesWithClearanceToContainer( const MODULE* aModule, CGENERICCONTAINER2D *aDstContainer, LAYER_ID aLayerId, int aInflateValue, bool aSkipNPTHPadsWihNoCopper );
    void AddGraphicsShapesWithClearanceToContainer( const MODULE* aModule, CGENERICCONTAINER2D *aDstContainer, LAYER_ID aLayerId, int aInflateValue );
    void AddShapeWithClearanceToContainer( const TEXTE_PCB* aTextPCB, CGENERICCONTAINER2D *aDstContainer, LAYER_ID aLayerId, int aClearanceValue );
    void AddShapeWithClearanceToContainer(const DRAWSEGMENT* aDrawSegment, CGENERICCONTAINER2D *aDstContainer, LAYER_ID aLayerId, int aClearanceValue );
    void AddSolidAreasShapesToContainer( const ZONE_CONTAINER* aZoneContainer, CGENERICCONTAINER2D *aDstContainer, LAYER_ID aLayerId );
    void TransformArcToSegments(const wxPoint &aCentre, const wxPoint &aStart, double aArcAngle, int aCircleToSegmentsCount, int aWidth, CGENERICCONTAINER2D *aDstContainer , const BOARD_ITEM &aBoardItem);
    void buildPadShapeThickOutlineAsSegments( const D_PAD*  aPad, CGENERICCONTAINER2D *aDstContainer, int aWidth);

 public:
    wxColour m_BgColor;
    wxColour m_BgColor_Top;

 private:
    BOARD *m_board;

    // Render options
    std::vector< bool > m_drawFlags;                ///< options flags to render the board
    GRID3D_TYPE m_3D_Grid_type;                     ///< Stores the current grid type

    // Pcb board position
    wxPoint m_boardPos;                             ///< center board actual position in board units
    wxSize  m_boardSize;                            ///< board actual size in board units
    SFVEC3F m_boardCenter;                          ///< 3d center position of the pcb board in 3d units

    // Pcb board bounding boxes
    CBBOX   m_boardBoudingBox;                      ///< 3d bouding box of the pcb board in 3d units
    CBBOX2D m_board2dBBox3DU;                       ///< 2d bouding box of the pcb board in 3d units

    SHAPE_POLY_SET    m_boardPoly;                  ///< PCB board outline polygon

    // 2D element containers
    MAP_CONTAINER_2D  m_layers_container2D;         ///< It contains the 2d elements of each layer
    MAP_CONTAINER_2D  m_layers_holes2D;             ///< It contains the holes per each layer
    CBVHCONTAINER2D   m_throughHoles_inflated;      ///< It contains the list of throughHoles of the board, the radius of the hole is inflated with the copper tickness
    CBVHCONTAINER2D   m_throughHoles;               ///< It contains the list of throughHoles of the board, the radius of the hole is inflated with the copper tickness

    // Layers information
    unsigned int m_copperLayersCount;               ///< Number of copper layers actually used by the board
    double m_biuTo3Dunits;                          ///< Normalization scale to convert board internal units to 3D units to normalize 3D units between -1.0 and +1.0
    float  m_layerZcoordTop[LAYER_ID_COUNT];        ///< Top (End) Z position of each layer (normalized)
    float  m_layerZcoordBottom[LAYER_ID_COUNT];     ///< Bottom (Start) Z position of each layer (normalized)
    float  m_copperThickness;                       ///< Copper thickness (normalized)
    float  m_epoxyThickness;                        ///< Epoxy thickness (normalized)
    float  m_nonCopperLayerThickness;               ///< Non copper layers thickness

    // Cameras
    CCAMERA &m_currentCamera;                       ///< Holds a pointer to current camera in use.
    CTRACK_BALL m_trackBallCamera;

    // Statistics
    unsigned int m_stats_nr_tracks;
    float        m_stats_track_med_width;
    unsigned int m_stats_nr_vias;                   ///< Nr of vias
    float        m_stats_via_med_hole_diameter;     ///< Computed medium diameter of the via holes in 3dunits
    unsigned int m_stats_nr_holes;
    float        m_stats_hole_med_diameter;         ///< Computed medium diameter of the holes in 3dunits

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_CINFO3D_VISU".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar *m_logTrace;

};

extern CINFO3D_VISU G_null_CINFO3D_VISU;

#endif // CINFO3D_VISU_H

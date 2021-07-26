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

#ifndef BOARD_ADAPTER_H
#define BOARD_ADAPTER_H

#include <array>
#include <vector>
#include "../3d_rendering/3d_render_raytracing/accelerators/container_2d.h"
#include "../3d_rendering/3d_render_raytracing/accelerators/container_3d.h"
#include "../3d_rendering/3d_render_raytracing/shapes3D/bbox_3d.h"
#include "../3d_rendering/camera.h"
#include "../3d_enums.h"
#include "../3d_cache/3d_cache.h"
#include "../common_ogl/ogl_attr_list.h"

#include <layers_id_colors_and_visibility.h>
#include <pad.h>
#include <pcb_track.h>
#include <wx/gdicmn.h>
#include <pcb_base_frame.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_dimension.h>
#include <zone.h>
#include <footprint.h>
#include <reporter.h>

class COLOR_SETTINGS;

/// A type that stores a container of 2d objects for each layer id
typedef std::map< PCB_LAYER_ID, BVH_CONTAINER_2D *> MAP_CONTAINER_2D_BASE;

/// A type that stores polysets for each layer id
typedef std::map< PCB_LAYER_ID, SHAPE_POLY_SET *> MAP_POLY;

/// This defines the range that all coord will have to be rendered.
/// It will use this value to convert to a normalized value between
/// -(RANGE_SCALE_3D/2) .. +(RANGE_SCALE_3D/2)
#define RANGE_SCALE_3D 8.0f


/**
 *  Helper class to handle information needed to display 3D board.
 */
class BOARD_ADAPTER
{
public:
    BOARD_ADAPTER();

    ~BOARD_ADAPTER();

    /**
     * Update the cache manager pointer.
     *
     * @param aCachePointer: the pointer to the 3D cache manager.
     */
    void Set3dCacheManager( S3D_CACHE* aCachePointer ) noexcept
    {
        m_3dModelManager = aCachePointer;
    }

    /**
     * Return the 3D cache manager pointer.
     */
    S3D_CACHE* Get3dCacheManager() const noexcept
    {
        return m_3dModelManager;
    }

    /**
     * Get a configuration status of a flag.
     *
     * @param aFlag the flag to get the status.
     * @return true if flag is set, false if not.
     */
    bool GetFlag( DISPLAY3D_FLG aFlag ) const ;

    /**
     * Set the status of a flag.
     *
     * @param aFlag the flag to set the status
     * @param aState status to set.
     */
    void SetFlag( DISPLAY3D_FLG aFlag, bool aState );

    /**
     * Check if a layer is enabled.
     *
     * @param aLayer layer ID to get status.
     */
    bool Is3dLayerEnabled( PCB_LAYER_ID aLayer ) const;

    /**
     * Test if footprint should be displayed in relation to attributes and the flags.
     */
    bool IsFootprintShown( FOOTPRINT_ATTR_T aFPAttributes ) const;

    /**
     * Set current board to be rendered.
     *
     * @param aBoard board to process.
     */
    void SetBoard( BOARD* aBoard ) noexcept
    {
        m_board = aBoard;
    }

    /**
     * Get current board to be rendered.
     *
     * @return BOARD pointer
     */
    const BOARD* GetBoard() const noexcept
    {
        return m_board;
    }

    void SetColorSettings( COLOR_SETTINGS* aSettings ) noexcept
    {
        m_colors = aSettings;
    }

    /**
     * Function to be called by the render when it need to reload the settings for the board.
     *
     * @param aStatusReporter the pointer for the status reporter.
     * @param aWarningReporter pointer for the warning reporter.
     */
    void InitSettings( REPORTER* aStatusReporter, REPORTER* aWarningReporter );

    /**
     * Board integer units To 3D units.
     *
     * @return the conversion factor to transform a position from the board to 3D units.
     */
    double BiuTo3dUnits() const noexcept
    {
        return m_biuTo3Dunits;
    }

    /**
     * Get the board outling bounding box.
     *
     * @return the board bounding box in 3D units.
     */
    const BBOX_3D& GetBBox() const noexcept
    {
        return m_boardBoundingBox;
    }

    /**
     * Get the current epoxy thickness.
     *
     * @return epoxy thickness in 3D units.
     */
    float GetEpoxyThickness() const noexcept
    {
        return m_epoxyThickness3DU;
    }

    /**
     * Get the current non copper layers thickness.
     *
     * @return thickness in 3D units of non copper layers.
     */
    float GetNonCopperLayerThickness() const noexcept
    {
        return m_nonCopperLayerThickness3DU;
    }

    /**
     * Get the current copper layer thickness.
     *
     * @return thickness in 3D units of copper layers.
     */
    float GetCopperThickness() const noexcept
    {
        return m_copperThickness3DU;
    }

    /**
     * Get the current copper layer thickness.
     *
     * @return thickness in board units.
     */
    int GetHolePlatingThickness() const noexcept;

    /**
     * Get the board size.
     *
     * @return size in BIU units.
     */
    wxSize GetBoardSize() const noexcept
    {
        return m_boardSize;
    }

    /**
     * Get the board center.
     *
     * @return position in BIU units.
     */
    wxPoint GetBoardPos() const noexcept
    {
        return m_boardPos;
    }

    /**
     * The board center position in 3D units.
     *
     * @return board center vector position in 3D units.
     */
    const SFVEC3F& GetBoardCenter() const noexcept
    {
        return m_boardCenter;
    }

    /**
     * Get the position of the footprint in 3d integer units considering if it is flipped or not.
     *
     * @param aIsFlipped true for use in footprints on Front (top) layer, false
     *                   if footprint is on back (bottom) layer.
     * @return the Z position of 3D shapes, in 3D integer units.
     */
    float GetFootprintZPos( bool aIsFlipped ) const ;

    /**
     * Get the current grid.
     *
     * @return space type of the grid.
     */
    GRID3D_TYPE GetGridType() const noexcept
    {
        return m_gridType;
    }

    /**
     * Set the current grid.
     *
     * @param aGridType the type space of the grid.
     */
    void SetGridType( GRID3D_TYPE aGridType ) noexcept
    {
        m_gridType = aGridType;
    }

    /**
     * Get the current antialiasing mode value.
     *
     * @return antialiasing mode value
     */
    ANTIALIASING_MODE GetAntiAliasingMode() const { return m_antiAliasingMode; }

    /**
     * Set the current antialiasing mode value.
     *
     * @param aAAmode antialiasing mode value.
     */
    void SetAntiAliasingMode( ANTIALIASING_MODE aAAmode ) { m_antiAliasingMode = aAAmode; }

    /**
     * @param aRenderEngine the render engine mode selected.
     */
    void SetRenderEngine( RENDER_ENGINE aRenderEngine ) noexcept
    {
        m_renderEngine = aRenderEngine;
    }

    /**
     * @return render engine on use.
     */
    RENDER_ENGINE GetRenderEngine() const noexcept
    {
        return m_renderEngine;
    }

    /**
     * @param aMaterialMode the render material mode.
     */
    void SetMaterialMode( MATERIAL_MODE aMaterialMode ) noexcept
    {
        m_materialMode = aMaterialMode;
    }

    /**
     * @return material rendering mode.
     */
    MATERIAL_MODE GetMaterialMode() const noexcept
    {
        return m_materialMode;
    }

    /**
     * Get the current polygon of the epoxy board.
     *
     * @return the shape polygon
     */
    const SHAPE_POLY_SET& GetBoardPoly() const noexcept
    {
        return m_board_poly;
    }

    /**
     * Get the technical color of a layer.
     *
     * @param aLayerId the layer to get the color information.
     * @return the color in SFVEC3F format.
     */
    SFVEC4F GetLayerColor( PCB_LAYER_ID aLayerId ) const;

    /**
     * Get the technical color of a layer.
     *
     * @param aItemId the item id to get the color information.
     * @return the color in SFVEC3F format.
     */
    SFVEC4F GetItemColor( int aItemId ) const;

    /**
     * @param[in] aColor is the color mapped.
     * @return the color in SFVEC3F format
     */
    SFVEC4F GetColor( const COLOR4D& aColor ) const;

    /**
     * Get the top z position.
     *
     * @param aLayerId layer id.
     * @return position in 3D units.
     */
    float GetLayerTopZPos( PCB_LAYER_ID aLayerId ) const noexcept
    {
        return m_layerZcoordTop[aLayerId];
    }

    /**
     * Get the bottom z position.
     *
     * @param aLayerId layer id.
     * @return position in 3D units.
     */
    float GetLayerBottomZPos( PCB_LAYER_ID aLayerId ) const noexcept
    {
        return m_layerZcoordBottom[aLayerId];
    }

    /**
     * Get the map of containers that have the objects per layer.
     *
     * @return the map containers of this board.
     */
    const MAP_CONTAINER_2D_BASE& GetLayerMap() const noexcept
    {
        return m_layerMap;
    }

    const BVH_CONTAINER_2D* GetPlatedPadsFront() const noexcept
    {
        return m_platedPadsFront;
    }

    const BVH_CONTAINER_2D* GetPlatedPadsBack() const noexcept
    {
        return m_platedPadsBack;
    }

    /**
     * Get the map of container that have the holes per layer.
     *
     * @return the map containers of holes from this board.
     */
    const MAP_CONTAINER_2D_BASE& GetLayerHoleMap() const noexcept
    {
        return m_layerHoleMap;
    }

    /**
     * Get the inflated through hole outside diameters container.
     *
     * @return a container with holes.
     */
    const BVH_CONTAINER_2D& GetThroughHoleOds() const noexcept
    {
        return m_throughHoleOds;
    }

    /**
     * Get the through hole annular rings container.
     *
     * @return a container with through hole annular rings.
     */
    const BVH_CONTAINER_2D& GetThroughHoleAnnularRings() const noexcept
    {
        return m_throughHoleAnnularRings;
    }

    /**
     * Get through hole outside diameter 2D polygons.
     *
     * The outside diameter 2D polygon is the hole diameter plus the plating thickness.
     *
     * @return a container with through hold outside diameter 2D polygons.
     */
    const SHAPE_POLY_SET& GetThroughHoleOdPolys() const noexcept
    {
        return m_throughHoleOdPolys;
    }

    const SHAPE_POLY_SET& GetThroughHoleAnnularRingPolys() const noexcept
    {
        return m_throughHoleAnnularRingPolys;
    }

    const SHAPE_POLY_SET& GetOuterNonPlatedThroughHolePoly() const noexcept
    {
        return m_nonPlatedThroughHoleOdPolys;
    }

    /**
     * @return a container with through hole via hole outside diameters.
     */
    const BVH_CONTAINER_2D& GetThroughHoleViaOds() const noexcept
    {
        return m_throughHoleViaOds;
    }

    const SHAPE_POLY_SET& GetThroughHoleViaOdPolys() const noexcept
    {
        return m_throughHoleViaOdPolys;
    }

    /**
     * Get the through hole inner diameter container.
     *
     * @return a container with holes inner diameters.
     */
    const BVH_CONTAINER_2D& GetThroughHoleIds() const noexcept
    {
        return m_throughHoleIds;
    }

    /**
     * Get number of vias in this board.
     *
     * @return number of vias.
     */
    unsigned int GetViaCount() const noexcept
    {
        return m_viaCount;
    }

    /**
     * Get number of holes in this board.
     *
     * @return number of holes.
     */
    unsigned int GetHoleCount() const noexcept
    {
        return m_holeCount;
    }

    /**
     * Thee average diameter of the via holes.
     *
     * @return via hole average diameter dimension in 3D units.
     */
    float GetAverageViaHoleDiameter() const noexcept
    {
        return m_averageViaHoleDiameter;
    }

    /**
     * Average diameter of through holes.
     *
     * @return the average diameter of through holes in 3D units.
     */
    float GetAverageHoleDiameter() const noexcept
    {
        return m_averageHoleDiameter;
    }

    /**
     * Average width of the tracks.
     *
     * @return average track width in 3D units.
     */
    float GetAverageTrackWidth() const noexcept
    {
        return m_averageTrackWidth;
    }

    /**
     * @param aDiameter3DU diameter in 3DU.
     * @return number of sides that should be used in a circle with \a aDiameter3DU.
     */
    unsigned int GetCircleSegmentCount( float aDiameter3DU ) const;

    /**
     * @param aDiameterBIU diameter in board internal units.
     * @return number of sides that should be used in circle with \a aDiameterBIU.
     */
    unsigned int GetCircleSegmentCount( int aDiameterBIU ) const;

    /**
     * Get map of polygon's layers.
     *
     * @return the map with polygon's layers.
     */
    const MAP_POLY& GetPolyMap() const noexcept
    {
        return m_layers_poly;
    }

    const SHAPE_POLY_SET* GetFrontPlatedPadPolys()
    {
        return m_frontPlatedPadPolys;
    }

    const SHAPE_POLY_SET* GetBackPlatedPadPolys()
    {
        return m_backPlatedPadPolys;
    }

    const MAP_POLY& GetHoleIdPolysMap() const noexcept
    {
        return m_layerHoleIdPolys;
    }

    const MAP_POLY& GetHoleOdPolysMap() const noexcept
    {
        return m_layerHoleOdPolys;
    }

private:
    /**
     * Create the board outline polygon.
     *
     * @return false if the outline could not be created
     */
    bool createBoardPolygon( wxString* aErrorMsg );
    void createLayers( REPORTER* aStatusReporter );
    void destroyLayers();

    // Helper functions to create the board
     void createTrack( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer,
                       int aClearanceValue );

    void createPadWithClearance( const PAD *aPad, CONTAINER_2D_BASE* aDstContainer,
                                 PCB_LAYER_ID aLayer, wxSize aClearanceValue ) const;

    OBJECT_2D* createPadWithDrill( const PAD* aPad, int aInflateValue );

    void addPadsWithClearance( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aDstContainer,
                               PCB_LAYER_ID aLayerId, int aInflateValue,
                               bool aSkipNPTHPadsWihNoCopper, bool aSkipPlatedPads,
                               bool aSkipNonPlatedPads );

    void addFootprintShapesWithClearance( const FOOTPRINT* aFootprint,
                                          CONTAINER_2D_BASE* aDstContainer,
                                          PCB_LAYER_ID aLayerId, int aInflateValue );

    void addShapeWithClearance( const PCB_TEXT* aText, CONTAINER_2D_BASE* aDstContainer,
                                PCB_LAYER_ID aLayerId, int aClearanceValue );

    void addShapeWithClearance( const PCB_SHAPE* aShape, CONTAINER_2D_BASE* aDstContainer,
                                PCB_LAYER_ID aLayerId, int aClearanceValue );

    void addShapeWithClearance( const PCB_DIMENSION_BASE* aDimension,
                                CONTAINER_2D_BASE* aDstContainer, PCB_LAYER_ID aLayerId,
                                int aClearanceValue );

    void addSolidAreasShapes( const ZONE* aZoneContainer, CONTAINER_2D_BASE* aDstContainer,
                              PCB_LAYER_ID aLayerId );

    void transformArcToSegments( const wxPoint& aCentre, const wxPoint& aStart, double aArcAngle,
                                 int aCircleToSegmentsCount, int aWidth,
                                 CONTAINER_2D_BASE* aDstContainer, const BOARD_ITEM& aBoardItem );

    void buildPadOutlineAsSegments( const PAD* aPad, CONTAINER_2D_BASE* aDstContainer, int aWidth );

    // Helper functions to create poly contours
    void buildPadOutlineAsPolygon( const PAD* aPad, SHAPE_POLY_SET& aCornerBuffer,
                                   int aWidth) const;

    void transformFPShapesToPolygon( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer,
                                     SHAPE_POLY_SET& aCornerBuffer ) const;

public:
    SFVEC4F m_BgColorBot;         ///< background bottom color
    SFVEC4F m_BgColorTop;         ///< background top color
    SFVEC4F m_BoardBodyColor;     ///< in realistic mode: FR4 board color
    SFVEC4F m_SolderMaskColorBot; ///< in realistic mode: solder mask color ( bot )
    SFVEC4F m_SolderMaskColorTop; ///< in realistic mode: solder mask color ( top )
    SFVEC4F m_SolderPasteColor;   ///< in realistic mode: solder paste color
    SFVEC4F m_SilkScreenColorBot; ///< in realistic mode: SilkScreen color ( bot )
    SFVEC4F m_SilkScreenColorTop; ///< in realistic mode: SilkScreen color ( top )
    SFVEC4F m_CopperColor;        ///< in realistic mode: copper color

    SFVEC3F m_OpenGlSelectionColor;

    // Raytracing light colors
    SFVEC3F m_RtCameraLightColor;
    SFVEC3F m_RtLightColorTop;
    SFVEC3F m_RtLightColorBottom;

    std::vector<SFVEC3F> m_RtLightColor;
    std::vector<SFVEC2F> m_RtLightSphericalCoords;

    // Raytracing options
    int m_RtShadowSampleCount;
    int m_RtReflectionSampleCount;
    int m_RtRefractionSampleCount;
    int m_RtRecursiveReflectionCount;
    int m_RtRecursiveRefractionCount;

    float m_RtSpreadShadows;
    float m_RtSpreadReflections;
    float m_RtSpreadRefractions;

private:
    BOARD*              m_board;
    S3D_CACHE*          m_3dModelManager;
    COLOR_SETTINGS*     m_colors;

    std::vector< bool > m_drawFlags;
    GRID3D_TYPE         m_gridType;
    RENDER_ENGINE       m_renderEngine;
    MATERIAL_MODE       m_materialMode;
    ANTIALIASING_MODE   m_antiAliasingMode;


    wxPoint m_boardPos;          ///< Board center position in board internal units.
    wxSize  m_boardSize;         ///< Board size in board internal units.
    SFVEC3F m_boardCenter;       ///< 3D center position of the board in 3D units.
    BBOX_3D m_boardBoundingBox;  ///< 3D bounding box of the board in 3D units.


    ///< Polygon contours for each layer.
    MAP_POLY          m_layers_poly;

    SHAPE_POLY_SET*   m_frontPlatedPadPolys;
    SHAPE_POLY_SET*   m_backPlatedPadPolys;

    ///< Polygon contours for hole outer diameters for each layer.
    MAP_POLY          m_layerHoleOdPolys;

    ///< Polygon contours for hole inner diameters for each layer.
    MAP_POLY          m_layerHoleIdPolys;

    ///< Polygon contours for non plated through hole outer diameters.
    SHAPE_POLY_SET    m_nonPlatedThroughHoleOdPolys;

    ///< Polygon contours for through hole outer diameters.
    SHAPE_POLY_SET    m_throughHoleOdPolys;

    ///< Polygon contours for through holes via outer diameters.
    SHAPE_POLY_SET    m_throughHoleViaOdPolys;

    ///< Polygon contours for through hole via annular rings.
    SHAPE_POLY_SET    m_throughHoleAnnularRingPolys;

    SHAPE_POLY_SET    m_board_poly;       ///< Board outline polygon.

    MAP_CONTAINER_2D_BASE  m_layerMap;    ///< 2D elements for each layer.


    BVH_CONTAINER_2D* m_platedPadsFront;
    BVH_CONTAINER_2D* m_platedPadsBack;

    ///< The holes per each layer.
    MAP_CONTAINER_2D_BASE  m_layerHoleMap;

    ///< List of through holes with the radius of the hole inflated with the copper thickness.
    BVH_CONTAINER_2D   m_throughHoleOds;

    ///< List of plated through hole annular rings.
    BVH_CONTAINER_2D   m_throughHoleAnnularRings;

    ///< List of through hole inner diameters.
    BVH_CONTAINER_2D   m_throughHoleIds;

    ///< List of through hole vias with the radius of the hole inflated with the copper thickness.
    BVH_CONTAINER_2D   m_throughHoleViaOds;

    ///< List of through hole via inner diameters.
    BVH_CONTAINER_2D   m_throughHoleViaIds;

    ///< Number of copper layers actually used by the board.
    unsigned int m_copperLayersCount;

    ///< Scale factor to convert board internal units to 3D units normalized between -1.0 and 1.0.
    double m_biuTo3Dunits;

    ///< Top (End) Z position of each layer in 3D units.
    std::array<float, PCB_LAYER_ID_COUNT>  m_layerZcoordTop;

    ///< Bottom (Start) Z position of each layer in 3D units.
    std::array<float, PCB_LAYER_ID_COUNT>  m_layerZcoordBottom;

    ///< Copper thickness in 3D units.
    float  m_copperThickness3DU;

    ///< Epoxy thickness in 3D units.
    float  m_epoxyThickness3DU;

    ///< Non copper layers thickness in 3D units.
    float  m_nonCopperLayerThickness3DU;

    ///< solder paste layers thickness in 3D units.
    float  m_solderPasteLayerThickness3DU;

    ///< Number of tracks in the board.
    unsigned int m_trackCount;

    ///< Track average width.
    float        m_averageTrackWidth;

    ///< Number of through hole vias in the board.
    unsigned int m_viaCount;

    ///< Computed average diameter of the via holes in 3D units.
    float        m_averageViaHoleDiameter;

    ///< Number of holes in the board.
    unsigned int m_holeCount;

    ///< Computed average diameter of the holes in 3D units.
    float        m_averageHoleDiameter;

    /**
     *  Trace mask used to enable or disable the trace output of this class.
     *  The debug output can be turned on by setting the WXTRACE environment variable to
     *  "KI_TRACE_EDA_CINFO3D_VISU".  See the wxWidgets documentation on wxLogTrace for
     *  more information.
     */
    static const wxChar* m_logTrace;

};


class EDA_3D_BOARD_HOLDER
{
public:
    virtual BOARD_ADAPTER& GetAdapter() = 0;
    virtual CAMERA&        GetCurrentCamera() = 0;

    virtual ~EDA_3D_BOARD_HOLDER() {};
};

#endif // BOARD_ADAPTER_H

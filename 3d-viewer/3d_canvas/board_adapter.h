/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "../3d_rendering/raytracing/accelerators/container_2d.h"
#include "../3d_rendering/raytracing/accelerators/container_3d.h"
#include "../3d_rendering/raytracing/shapes3D/bbox_3d.h"
#include "../3d_rendering/camera.h"
#include "../3d_enums.h"
#include "../3d_cache/3d_cache.h"
#include "../common_ogl/ogl_attr_list.h"
#include "../3d_viewer/eda_3d_viewer_settings.h"

#include <layer_ids.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_base_frame.h>
#include <pcb_text.h>
#include <pcb_shape.h>
#include <pcb_dimension.h>
#include <zone.h>
#include <footprint.h>
#include <reporter.h>
#include <dialogs/dialog_color_picker.h>

class COLOR_SETTINGS;

/// A type that stores a container of 2d objects for each layer id
typedef std::map<PCB_LAYER_ID, BVH_CONTAINER_2D*> MAP_CONTAINER_2D_BASE;

/// A type that stores polysets for each layer id
typedef std::map<PCB_LAYER_ID, SHAPE_POLY_SET*> MAP_POLY;

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
    void Set3dCacheManager( S3D_CACHE* aCacheMgr ) noexcept { m_3dModelManager = aCacheMgr; }
    S3D_CACHE* Get3dCacheManager() const noexcept { return m_3dModelManager; }

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
    void SetBoard( BOARD* aBoard ) noexcept { m_board = aBoard; }
    const BOARD* GetBoard() const noexcept { return m_board; }

    void ReloadColorSettings() noexcept;

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
    double BiuTo3dUnits() const noexcept { return m_biuTo3Dunits; }

    /**
     * Get the board outling bounding box.
     *
     * @return the board bounding box in 3D units.
     */
    const BBOX_3D& GetBBox() const noexcept { return m_boardBoundingBox; }

    /**
     * Get the board body thickness, including internal copper layers (in 3D units).
     */
    float GetBoardBodyThickness() const noexcept { return m_boardBodyThickness3DU; }

    /**
     * Get the non copper layers thickness (in 3D units).
     */
    float GetNonCopperLayerThickness() const noexcept { return m_nonCopperLayerThickness3DU; }

    /**
     * Get the copper layer thicknesses (in 3D units).
     */
    float GetFrontCopperThickness() const noexcept { return m_frontCopperThickness3DU; }
    float GetBackCopperThickness() const noexcept { return m_backCopperThickness3DU; }

    /**
     * Get the hole plating thickness (NB: in BOARD UNITS!).
     */
    int GetHolePlatingThickness() const noexcept;

    /**
     * Get the board size.
     *
     * @return size in BIU units.
     */
    VECTOR2I GetBoardSize() const noexcept { return m_boardSize; }

    /**
     * Get the board center.
     *
     * @return position in BIU units.
     */
    VECTOR2I GetBoardPos() const noexcept { return m_boardPos; }

    /**
     * The board center position in 3D units.
     *
     * @return board center vector position in 3D units.
     */
    const SFVEC3F& GetBoardCenter() const noexcept { return m_boardCenter; }

    /**
     * Get the position of the footprint in 3d integer units considering if it is flipped or not.
     *
     * @param aIsFlipped true for use in footprints on Front (top) layer, false
     *                   if footprint is on back (bottom) layer.
     * @return the Z position of 3D shapes, in 3D integer units.
     */
    float GetFootprintZPos( bool aIsFlipped ) const ;

    /**
     * Get the current polygon of the epoxy board.
     *
     * @return the shape polygon
     */
    const SHAPE_POLY_SET& GetBoardPoly() const noexcept { return m_board_poly; }

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

    SFVEC2F GetSphericalCoord( int i ) const;

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
    const MAP_CONTAINER_2D_BASE& GetLayerMap() const noexcept { return m_layerMap; }
    const BVH_CONTAINER_2D* GetPlatedPadsFront() const noexcept { return m_platedPadsFront; }
    const BVH_CONTAINER_2D* GetPlatedPadsBack() const noexcept { return m_platedPadsBack; }

    const MAP_CONTAINER_2D_BASE& GetLayerHoleMap() const noexcept { return m_layerHoleMap; }
    const BVH_CONTAINER_2D& GetThroughHoleIds() const noexcept { return m_throughHoleIds; }
    const BVH_CONTAINER_2D& GetThroughHoleOds() const noexcept { return m_throughHoleOds; }

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

    const BVH_CONTAINER_2D& GetThroughHoleAnnularRings() const noexcept
    {
        return m_throughHoleAnnularRings;
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

    unsigned int GetViaCount() const noexcept { return m_viaCount; }
    unsigned int GetHoleCount() const noexcept { return m_holeCount; }

    /**
     * @return via hole average diameter dimension in 3D units.
     */
    float GetAverageViaHoleDiameter() const noexcept { return m_averageViaHoleDiameter; }

    /**
     * @return the average diameter of through holes in 3D units.
     */
    float GetAverageHoleDiameter() const noexcept { return m_averageHoleDiameter; }

    /**
     * @return average track width in 3D units.
     */
    float GetAverageTrackWidth() const noexcept { return m_averageTrackWidth; }

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
    const MAP_POLY& GetPolyMap() const noexcept { return m_layers_poly; }

    const SHAPE_POLY_SET* GetFrontPlatedPadPolys() { return m_frontPlatedPadPolys;  }
    const SHAPE_POLY_SET* GetBackPlatedPadPolys() { return m_backPlatedPadPolys; }
    const MAP_POLY& GetHoleIdPolysMap() const noexcept { return m_layerHoleIdPolys; }
    const MAP_POLY& GetHoleOdPolysMap() const noexcept { return m_layerHoleOdPolys; }

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
    void createViaWithMargin( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer,
                              int aMargin );

    void createTrack( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer );

    void createPadWithMargin( const PAD *aPad, CONTAINER_2D_BASE* aDstContainer,
                              PCB_LAYER_ID aLayer, const VECTOR2I& aMargin ) const;

    OBJECT_2D* createPadWithDrill( const PAD* aPad, int aInflateValue );

    void addPads( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aDstContainer,
                  PCB_LAYER_ID aLayerId, bool aSkipPlatedPads, bool aSkipNonPlatedPads );

    void addFootprintShapes( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aDstContainer,
                             PCB_LAYER_ID aLayerId );

    void addText( const EDA_TEXT* aText, CONTAINER_2D_BASE* aDstContainer,
                  const BOARD_ITEM* aOwner );

    void addShape( const PCB_SHAPE* aShape, CONTAINER_2D_BASE* aContainer,
                   const BOARD_ITEM* aOwner );

    void addShape( const PCB_DIMENSION_BASE* aDimension, CONTAINER_2D_BASE* aDstContainer,
                   const BOARD_ITEM* aOwner );

    void addSolidAreasShapes( const ZONE* aZone, CONTAINER_2D_BASE* aDstContainer,
                              PCB_LAYER_ID aLayerId );

    void transformArcToSegments( const VECTOR2I& aCentre, const VECTOR2I& aStart,
                                 const EDA_ANGLE& aArcAngle, int aCircleToSegmentsCount, int aWidth,
                                 CONTAINER_2D_BASE* aDstContainer, const BOARD_ITEM& aOwner );

    void buildPadOutlineAsSegments( const PAD* aPad, CONTAINER_2D_BASE* aDstContainer, int aWidth );

    // Helper functions to create poly contours
    void buildPadOutlineAsPolygon( const PAD* aPad, SHAPE_POLY_SET& aBuffer, int aWidth) const;

    void transformFPShapesToPolySet( const FOOTPRINT* aFootprint, PCB_LAYER_ID aLayer,
                                     SHAPE_POLY_SET& aBuffer ) const;

public:
    static CUSTOM_COLORS_LIST   g_SilkscreenColors;
    static CUSTOM_COLORS_LIST   g_MaskColors;
    static CUSTOM_COLORS_LIST   g_PasteColors;
    static CUSTOM_COLORS_LIST   g_FinishColors;
    static CUSTOM_COLORS_LIST   g_BoardColors;

    static KIGFX::COLOR4D       g_DefaultBackgroundTop;
    static KIGFX::COLOR4D       g_DefaultBackgroundBot;
    static KIGFX::COLOR4D       g_DefaultSilkscreen;
    static KIGFX::COLOR4D       g_DefaultSolderMask;
    static KIGFX::COLOR4D       g_DefaultSolderPaste;
    static KIGFX::COLOR4D       g_DefaultSurfaceFinish;
    static KIGFX::COLOR4D       g_DefaultBoardBody;

public:
    EDA_3D_VIEWER_SETTINGS*     m_Cfg;

    bool              m_IsBoardView;
    bool              m_MousewheelPanning;
    bool              m_IsPreviewer;        ///< true if the board adaptater is living in a 3D
                                            ///<   preview panel, false for the standard 3D viewer

    SFVEC4F           m_BgColorBot;         ///< background bottom color
    SFVEC4F           m_BgColorTop;         ///< background top color
    SFVEC4F           m_BoardBodyColor;     ///< in realistic mode: FR4 board color
    SFVEC4F           m_SolderMaskColorBot; ///< in realistic mode: solder mask color ( bot )
    SFVEC4F           m_SolderMaskColorTop; ///< in realistic mode: solder mask color ( top )
    SFVEC4F           m_SolderPasteColor;   ///< in realistic mode: solder paste color
    SFVEC4F           m_SilkScreenColorBot; ///< in realistic mode: SilkScreen color ( bot )
    SFVEC4F           m_SilkScreenColorTop; ///< in realistic mode: SilkScreen color ( top )
    SFVEC4F           m_CopperColor;        ///< in realistic mode: copper color

private:
    BOARD*            m_board;
    S3D_CACHE*        m_3dModelManager;
    RENDER_SETTINGS*  m_renderSettings;
    COLOR_SETTINGS*   m_colors;

    VECTOR2I          m_boardPos;           ///< Board center position in board internal units.
    VECTOR2I          m_boardSize;          ///< Board size in board internal units.
    SFVEC3F           m_boardCenter;        ///< 3D center position of the board in 3D units.
    BBOX_3D           m_boardBoundingBox;   ///< 3D bounding box of the board in 3D units.

    MAP_POLY          m_layers_poly;        ///< Amalgamated polygon contours for various types of
                                            ///<   items

    SHAPE_POLY_SET*   m_frontPlatedPadPolys;
    SHAPE_POLY_SET*   m_backPlatedPadPolys;

    MAP_POLY          m_layerHoleOdPolys;              ///< Hole outer diameters (per layer)
    MAP_POLY          m_layerHoleIdPolys;              ///< Hole inner diameters (per layer)

    SHAPE_POLY_SET    m_nonPlatedThroughHoleOdPolys;   ///< NPTH outer diameters
    SHAPE_POLY_SET    m_throughHoleOdPolys;            ///< PTH outer diameters
    SHAPE_POLY_SET    m_throughHoleViaOdPolys;         ///< Via hole outer diameters
    SHAPE_POLY_SET    m_throughHoleAnnularRingPolys;   ///< Via annular ring outer diameters

    SHAPE_POLY_SET    m_board_poly;                    ///< Board outline polygon.

    MAP_CONTAINER_2D_BASE  m_layerMap;                 ///< 2D elements for each layer.
    MAP_CONTAINER_2D_BASE  m_layerHoleMap;             ///< Holes for each layer.

    BVH_CONTAINER_2D* m_platedPadsFront;
    BVH_CONTAINER_2D* m_platedPadsBack;

    BVH_CONTAINER_2D  m_throughHoleOds;                ///< List of PTH outer diameters
    BVH_CONTAINER_2D  m_throughHoleIds;                ///< List of PTH inner diameters
    BVH_CONTAINER_2D  m_throughHoleAnnularRings;       ///< List of via annular rings
    BVH_CONTAINER_2D  m_throughHoleViaOds;             ///< List of via hole outer diameters
    BVH_CONTAINER_2D  m_throughHoleViaIds;             ///< List of via hole inner diameters

    unsigned int      m_copperLayersCount;

    double            m_biuTo3Dunits;       ///< Scale factor to convert board internal units to
                                            ///< 3D units normalized between -1.0 and 1.0.

    std::array<float, PCB_LAYER_ID_COUNT> m_layerZcoordTop;    ///< Top (End) Z position of each
                                                               ///< layer in 3D units.

    std::array<float, PCB_LAYER_ID_COUNT> m_layerZcoordBottom; ///< Bottom (Start) Z position of
                                                               ///< each layer in 3D units.

    float             m_frontCopperThickness3DU;
    float             m_backCopperThickness3DU;
    float             m_boardBodyThickness3DU;
    float             m_nonCopperLayerThickness3DU;
    float             m_solderPasteLayerThickness3DU;

    unsigned int      m_trackCount;
    float             m_averageTrackWidth;
    unsigned int      m_viaCount;
    float             m_averageViaHoleDiameter;
    unsigned int      m_holeCount;
    float             m_averageHoleDiameter;

    /**
     * Trace mask used to enable or disable debug output for this class. Output can be turned
     * on by setting the WXTRACE environment variable to "KI_TRACE_EDA_CINFO3D_VISU". See the
     * wxWidgets documentation on wxLogTrace for more information.
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

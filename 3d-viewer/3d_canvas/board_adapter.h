/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef BOARD_ADAPTER_H
#define BOARD_ADAPTER_H

#include <array>
#include <vector>
#include "../3d_rendering/raytracing/accelerators/container_2d.h"
#include "../3d_rendering/raytracing/accelerators/container_3d.h"
#include "../3d_rendering/raytracing/shapes3D/bbox_3d.h"
#include <gal/3d/camera.h>
#include <3d_enums.h>
#include "../3d_cache/3d_cache.h"
#include "../common_ogl/ogl_attr_list.h"
#include "../3d_viewer/eda_3d_viewer_settings.h"

#include <layer_ids.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_base_frame.h>
#include <pcb_text.h>
#include <pcb_textbox.h>
#include <pcb_table.h>
#include <pcb_shape.h>
#include <pcb_dimension.h>
#include <zone.h>
#include <footprint.h>
#include <reporter.h>
#include <dialogs/dialog_color_picker.h>

class COLOR_SETTINGS;
class PCB_TEXTBOX;
class PCB_BARCODE;

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
    bool Is3dLayerEnabled( PCB_LAYER_ID aLayer,
                           const std::bitset<LAYER_3D_END>& aVisibilityFlags ) const;

    /**
     * Test if footprint should be displayed in relation to attributes and the flags.
     */
    bool IsFootprintShown( const FOOTPRINT* aFootprint ) const;

    /**
     * Set current board to be rendered.
     *
     * @param aBoard board to process.
     */
    void SetBoard( BOARD* aBoard ) noexcept { m_board = aBoard; }
    const BOARD* GetBoard() const noexcept { return m_board; }

    void ReloadColorSettings() noexcept;

    /**
     * Build a color list which is used to store colors layers
     */
    std::map<int, COLOR4D> GetLayerColors() const;

    std::map<int, COLOR4D> GetDefaultColors() const;
    void SetLayerColors( const std::map<int, COLOR4D>& aColors );

    std::bitset<LAYER_3D_END> GetVisibleLayers() const;
    std::bitset<LAYER_3D_END> GetDefaultVisibleLayers() const;
    void SetVisibleLayers( const std::bitset<LAYER_3D_END>& aLayers );

    bool GetUseBoardEditorCopperLayerColors() const;

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
     * Get the board outline bounding box.
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
    SFVEC4F GetLayerColor( int aLayerId ) const;

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
        auto it = m_layerZcoordTop.find( aLayerId );

        if( it != m_layerZcoordTop.end() )
            return it->second;
        else
            return -( m_boardBodyThickness3DU / 2.0f );
    }

    /**
     * Get the bottom z position.
     *
     * @param aLayerId layer id.
     * @return position in 3D units.
     */
    float GetLayerBottomZPos( PCB_LAYER_ID aLayerId ) const noexcept
    {
        auto it = m_layerZcoordBottom.find( aLayerId );

        if( it != m_layerZcoordBottom.end() )
            return it->second;
        else
            return -( m_boardBodyThickness3DU / 2.0f ) - m_backCopperThickness3DU;
    }

    /**
     * Get the map of containers that have the objects per layer.
     *
     * @return the map containers of this board.
     */
    const MAP_CONTAINER_2D_BASE& GetLayerMap() const noexcept { return m_layerMap; }
    const BVH_CONTAINER_2D* GetPlatedPadsFront() const noexcept { return m_platedPadsFront; }
    const BVH_CONTAINER_2D* GetPlatedPadsBack() const noexcept { return m_platedPadsBack; }
    const BVH_CONTAINER_2D* GetOffboardPadsFront() const noexcept { return m_offboardPadsFront; }
    const BVH_CONTAINER_2D* GetOffboardPadsBack() const noexcept { return m_offboardPadsBack; }

    const MAP_CONTAINER_2D_BASE& GetLayerHoleMap() const noexcept { return m_layerHoleMap; }
    const BVH_CONTAINER_2D& GetTH_IDs() const noexcept { return m_TH_IDs; }
    const BVH_CONTAINER_2D& GetTH_ODs() const noexcept { return m_TH_ODs; }

    /**
     * Get through hole outside diameter 2D polygons.
     *
     * The outside diameter 2D polygon is the hole diameter plus the plating thickness.
     *
     * @return a container with through hold outside diameter 2D polygons.
     */
    const SHAPE_POLY_SET& GetTH_ODPolys() const noexcept
    {
        return m_TH_ODPolys;
    }

    const BVH_CONTAINER_2D& GetViaAnnuli() const noexcept
    {
        return m_viaAnnuli;
    }

    const SHAPE_POLY_SET& GetViaAnnuliPolys() const noexcept
    {
        return m_viaAnnuliPolys;
    }

    const SHAPE_POLY_SET& GetNPTH_ODPolys() const noexcept
    {
        return m_NPTH_ODPolys;
    }

    /**
     * @return a container with through hole via hole outside diameters.
     */
    const BVH_CONTAINER_2D& GetViaTH_ODs() const noexcept
    {
        return m_viaTH_ODs;
    }

    const SHAPE_POLY_SET& GetViaTH_ODPolys() const noexcept
    {
        return m_viaTH_ODPolys;
    }

    /**
     *  Get the backdrill and tertiary drill polygons.
     *
     */
    const SHAPE_POLY_SET& GetBackdrillPolys() const noexcept
    {
        return m_BackdrillPolys;
    }

    const SHAPE_POLY_SET& GetTertiarydrillPolys() const noexcept
    {
        return m_TertiarydrillPolys;
    }

    const BVH_CONTAINER_2D& GetBackdrillCutouts() const noexcept
    {
        return m_backdrillCutouts;
    }

    const BVH_CONTAINER_2D& GetTertiarydrillCutouts() const noexcept
    {
        return m_tertiarydrillCutouts;
    }

    /**
     * Get the container of counterbore cutout geometry for the front (top) side.
     * These are circles representing the counterbore diameter for cutting into the board.
     */
    const BVH_CONTAINER_2D& GetFrontCounterboreCutouts() const noexcept
    {
        return m_frontCounterboreCutouts;
    }

    /**
     * Get the container of counterbore cutout geometry for the back (bottom) side.
     */
    const BVH_CONTAINER_2D& GetBackCounterboreCutouts() const noexcept
    {
        return m_backCounterboreCutouts;
    }

    /**
     * Get the container of countersink cutout geometry for the front (top) side.
     * These are circles representing the countersink outer diameter for cutting into the board.
     */
    const BVH_CONTAINER_2D& GetFrontCountersinkCutouts() const noexcept
    {
        return m_frontCountersinkCutouts;
    }

    /**
     * Get the container of countersink cutout geometry for the back (bottom) side.
     */
    const BVH_CONTAINER_2D& GetBackCountersinkCutouts() const noexcept
    {
        return m_backCountersinkCutouts;
    }

    /**
     * Get the polygon set of counterbore outer diameters for the front (top) side.
     */
    const SHAPE_POLY_SET& GetFrontCounterborePolys() const noexcept
    {
        return m_frontCounterborePolys;
    }

    /**
     * Get the polygon set of counterbore outer diameters for the back (bottom) side.
     */
    const SHAPE_POLY_SET& GetBackCounterborePolys() const noexcept
    {
        return m_backCounterborePolys;
    }

    /**
     * Get the polygon set of countersink outer diameters for the front (top) side.
     */
    const SHAPE_POLY_SET& GetFrontCountersinkPolys() const noexcept
    {
        return m_frontCountersinkPolys;
    }

    /**
     * Get the polygon set of countersink outer diameters for the back (bottom) side.
     */
    const SHAPE_POLY_SET& GetBackCountersinkPolys() const noexcept
    {
        return m_backCountersinkPolys;
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

    const SHAPE_POLY_SET* GetFrontPlatedCopperPolys()
    {
        return m_frontPlatedCopperPolys;
    }

    const SHAPE_POLY_SET* GetBackPlatedCopperPolys()
    {
        return m_backPlatedCopperPolys;
    }

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
    void createTrackWithMargin( const PCB_TRACK* aTrack, CONTAINER_2D_BASE* aDstContainer,
                                PCB_LAYER_ID aLayer, int aMargin = 0 );

    // Generate the pad shape on board layers. The pad hole is not generated by createPadWithMargin
    void createPadWithMargin( const PAD *aPad, CONTAINER_2D_BASE* aDstContainer,
                              PCB_LAYER_ID aLayer, const VECTOR2I& aMargin ) const;

    // Generate the hole shape of aPad, stored in aDstContainer
    void createPadHoleShape( const PAD* aPad, CONTAINER_2D_BASE* aDstContainer, int aInflateValue );

    void addPads( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aDstContainer,
                  PCB_LAYER_ID aLayerId );

    void addFootprintShapes( const FOOTPRINT* aFootprint, CONTAINER_2D_BASE* aDstContainer,
                             PCB_LAYER_ID aLayerId,
                             const std::bitset<LAYER_3D_END>& aVisibilityFlags );

    void addText( const EDA_TEXT* aText, CONTAINER_2D_BASE* aDstContainer,
                  const BOARD_ITEM* aOwner );

    void addBarCode( const PCB_BARCODE* aBarCode, CONTAINER_2D_BASE* aDstContainer,
                     const BOARD_ITEM* aOwner );

    void addShape( const PCB_SHAPE* aShape, CONTAINER_2D_BASE* aContainer,
                   const BOARD_ITEM* aOwner, PCB_LAYER_ID aLayer );

    void addShape( const PCB_DIMENSION_BASE* aDimension, CONTAINER_2D_BASE* aDstContainer,
                   const BOARD_ITEM* aOwner );

    void addShape( const PCB_TEXTBOX* aTextBox, CONTAINER_2D_BASE* aContainer,
                   const BOARD_ITEM* aOwner );

    void addTable( const PCB_TABLE* aTable, CONTAINER_2D_BASE* aContainer,
                   const BOARD_ITEM* aOwner );

    void addSolidAreasShapes( const ZONE* aZone, CONTAINER_2D_BASE* aDstContainer,
                              PCB_LAYER_ID aLayerId );

    void createArcSegments( const VECTOR2I& aCentre, const VECTOR2I& aStart,
                            const EDA_ANGLE& aArcAngle, int aCircleToSegmentsCount, int aWidth,
                            CONTAINER_2D_BASE* aContainer, const BOARD_ITEM& aOwner );

    void buildPadOutlineAsSegments( const PAD* aPad, PCB_LAYER_ID aLayer,
                                    CONTAINER_2D_BASE* aDstContainer, int aWidth );

public:
    static CUSTOM_COLORS_LIST   g_SilkColors;
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
    static KIGFX::COLOR4D       g_DefaultComments;
    static KIGFX::COLOR4D       g_DefaultECOs;

public:
    EDA_3D_VIEWER_SETTINGS*     m_Cfg;

    bool              m_IsBoardView;
    bool              m_MousewheelPanning;
    bool              m_IsPreviewer;          ///< true if we're in a 3D preview panel, false
                                              ///<   for the standard 3D viewer

    SFVEC4F           m_BgColorBot;           ///< background bottom color
    SFVEC4F           m_BgColorTop;           ///< background top color
    SFVEC4F           m_BoardBodyColor;       ///< in realistic mode: FR4 board color
    SFVEC4F           m_SolderMaskColorBot;   ///< in realistic mode: solder mask color ( bot )
    SFVEC4F           m_SolderMaskColorTop;   ///< in realistic mode: solder mask color ( top )
    SFVEC4F           m_SolderPasteColor;     ///< in realistic mode: solder paste color
    SFVEC4F           m_SilkScreenColorBot;   ///< in realistic mode: SilkScreen color ( bot )
    SFVEC4F           m_SilkScreenColorTop;   ///< in realistic mode: SilkScreen color ( top )
    SFVEC4F           m_CopperColor;          ///< in realistic mode: copper color
    SFVEC4F           m_UserDrawingsColor;
    SFVEC4F           m_UserCommentsColor;
    SFVEC4F           m_ECO1Color;
    SFVEC4F           m_ECO2Color;
    SFVEC4F           m_UserDefinedLayerColor[45];

    std::map<int, COLOR4D> m_ColorOverrides;  ///< allows to override color scheme colors
    std::map<int, COLOR4D> m_BoardEditorColors; ///< list of colors used by the board editor

private:
    BOARD*            m_board;
    S3D_CACHE*        m_3dModelManager;

    VECTOR2I          m_boardPos;             ///< Board center position in board internal units.
    VECTOR2I          m_boardSize;            ///< Board size in board internal units.
    SFVEC3F           m_boardCenter;          ///< 3D center position of the board in 3D units.
    BBOX_3D           m_boardBoundingBox;     ///< 3D bounding box of the board in 3D units.

    MAP_POLY          m_layers_poly;          ///< Amalgamated polygon contours for various types
                                              ///<   of items

    SHAPE_POLY_SET*   m_frontPlatedCopperPolys;
    SHAPE_POLY_SET*   m_backPlatedCopperPolys;

    MAP_POLY          m_layerHoleOdPolys;     ///< Hole outer diameters (per layer)
    MAP_POLY          m_layerHoleIdPolys;     ///< Hole inner diameters (per layer)

    SHAPE_POLY_SET    m_BackdrillPolys;        ///< Board backdrill polygons B.Cu->in
    SHAPE_POLY_SET    m_TertiarydrillPolys;    ///< Board tertiary drill polygons F.Cu->in

    SHAPE_POLY_SET    m_NPTH_ODPolys;         ///< NPTH outer diameters
    SHAPE_POLY_SET    m_TH_ODPolys;           ///< PTH outer diameters
    SHAPE_POLY_SET    m_viaTH_ODPolys;        ///< Via hole outer diameters
    SHAPE_POLY_SET    m_viaAnnuliPolys;       ///< Via annular ring outer diameters

    SHAPE_POLY_SET    m_frontCounterborePolys;   ///< Counterbore outer diameters on front
    SHAPE_POLY_SET    m_backCounterborePolys;    ///< Counterbore outer diameters on back
    SHAPE_POLY_SET    m_frontCountersinkPolys;   ///< Countersink outer diameters on front
    SHAPE_POLY_SET    m_backCountersinkPolys;    ///< Countersink outer diameters on back

    SHAPE_POLY_SET    m_board_poly;           ///< Board outline polygon.

    MAP_CONTAINER_2D_BASE  m_layerMap;        ///< 2D elements for each layer.
    MAP_CONTAINER_2D_BASE  m_layerHoleMap;    ///< Holes for each layer.

    BVH_CONTAINER_2D* m_platedPadsFront;
    BVH_CONTAINER_2D* m_platedPadsBack;
    BVH_CONTAINER_2D* m_offboardPadsFront;
    BVH_CONTAINER_2D* m_offboardPadsBack;

    BVH_CONTAINER_2D  m_TH_ODs;               ///< List of PTH outer diameters
    BVH_CONTAINER_2D  m_TH_IDs;               ///< List of PTH inner diameters
    BVH_CONTAINER_2D  m_viaAnnuli;            ///< List of via annular rings
    BVH_CONTAINER_2D  m_viaTH_ODs;            ///< List of via hole outer diameters

    BVH_CONTAINER_2D  m_backdrillCutouts;          ///< Backdrill cutouts
    BVH_CONTAINER_2D  m_tertiarydrillCutouts;      ///< Tertiary drill cutouts

    BVH_CONTAINER_2D  m_frontCounterboreCutouts;   ///< Counterbore cutouts on front (top)
    BVH_CONTAINER_2D  m_backCounterboreCutouts;    ///< Counterbore cutouts on back (bottom)
    BVH_CONTAINER_2D  m_frontCountersinkCutouts;   ///< Countersink cutouts on front (top)
    BVH_CONTAINER_2D  m_backCountersinkCutouts;    ///< Countersink cutouts on back (bottom)

    unsigned int      m_copperLayersCount;

    double            m_biuTo3Dunits;         ///< Scale factor to convert board internal units
                                              ///<  to 3D units normalized between -1.0 and 1.0.

    std::map<PCB_LAYER_ID, float> m_layerZcoordTop;    ///< Top (End) Z pos of each layer in 3D units.
    std::map<PCB_LAYER_ID, float> m_layerZcoordBottom; ///< Bottom (Start) Z pos of each layer.

    float             m_frontCopperThickness3DU;
    float             m_backCopperThickness3DU;
    float             m_boardBodyThickness3DU;
    float             m_nonCopperLayerThickness3DU;
    float             m_frontMaskThickness3DU;
    float             m_backMaskThickness3DU;
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


#endif // BOARD_ADAPTER_H

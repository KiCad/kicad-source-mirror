/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2015-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file render_3d_legacy.h
 */

#ifndef RENDER_3D_LEGACY_H_
#define RENDER_3D_LEGACY_H_

#include "../render_3d_base.h"
#include "layer_triangles.h"

#include "../3d_render_raytracing/shapes2D/polygon_2d.h"
#include "../3d_render_raytracing/shapes2D/triangle_2d.h"
#include "../3d_render_raytracing/shapes2D/4pt_polygon_2d.h"
#include "../3d_render_raytracing/shapes2D/filled_circle_2d.h"
#include "../3d_render_raytracing/shapes2D/ring_2d.h"
#include "../3d_render_raytracing/shapes2D/round_segment_2d.h"

#include "3d_model.h"

#include "3d_cache/3d_info.h"

#include <map>


typedef std::map< PCB_LAYER_ID, OPENGL_RENDER_LIST* > MAP_OGL_DISP_LISTS;
typedef std::list<TRIANGLE_DISPLAY_LIST* > LIST_TRIANGLES;
typedef std::map< wxString, MODEL_3D* > MAP_3DMODEL;

#define SIZE_OF_CIRCLE_TEXTURE 1024

/**
 * Object to render the board using openGL legacy mode.
 */
class RENDER_3D_LEGACY : public RENDER_3D_BASE
{
public:
    explicit RENDER_3D_LEGACY( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter, CAMERA& aCamera );

    ~RENDER_3D_LEGACY();

    void SetCurWindowSize( const wxSize& aSize ) override;
    bool Redraw( bool aIsMoving, REPORTER* aStatusReporter, REPORTER* aWarningReporter ) override;

    int GetWaitForEditingTimeOut() override;

    void SetCurrentRollOverItem( BOARD_ITEM* aRollOverItem )
    {
        m_currentRollOverItem = aRollOverItem;
    }

    /**
     * Load footprint models if they are not already loaded, i.e. if m_3dModelMap is empty
     */
    void Load3dModelsIfNeeded();

private:
    OPENGL_RENDER_LIST* generateHoles( const LIST_OBJECT2D& aListHolesObject2d,
                                       const SHAPE_POLY_SET& aPoly, float aZtop,
                                       float aZbot, bool aInvertFaces,
                                       const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    OPENGL_RENDER_LIST* generateLayerList( const BVH_CONTAINER_2D* aContainer,
                                           const SHAPE_POLY_SET* aPolyList,
                                           PCB_LAYER_ID aLayerId,
                                           const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    void addTopAndBottomTriangles( TRIANGLE_DISPLAY_LIST* aDst, const SFVEC2F& v0,
                                   const SFVEC2F& v1, const SFVEC2F& v2, float top, float bot );

    void addObjectTriangles( const RING_2D* aRing, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const POLYGON_4PT_2D* aPoly, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const FILLED_CIRCLE_2D* aFilledCircle,
                             TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop, float aZbot );

    void addObjectTriangles( const TRIANGLE_2D* aTri, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const ROUND_SEGMENT_2D* aSeg,
                             TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop, float aZbot );

    void renderSolderMaskLayer( PCB_LAYER_ID aLayerID, float aZPosition,
                                bool aDrawMiddleSegments, bool aSkipRenderHoles );

    void renderBoardBody( bool aSkipRenderHoles );

    void getLayerZPos( PCB_LAYER_ID aLayerID, float& aOutZtop, float& aOutZbot ) const;

    void generateRing( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                       unsigned int aNr_sides_per_circle,
                       std::vector< SFVEC2F >& aInnerContourResult,
                       std::vector< SFVEC2F >& aOuterContourResult, bool aInvertOrder );

    void generateCylinder( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                           float aZtop, float aZbot, unsigned int aNr_sides_per_circle,
                           TRIANGLE_DISPLAY_LIST* aDstLayer );

    void generateViasAndPads();

    /**
     * Load footprint models from the cache and load it to openGL lists in the form of
     * #MODEL_3D objects.
     *
     * This map of models will work as a local cache for this render. (cache based on
     * MODEL_3D with associated openGL lists in GPU memory)
     */
    void load3dModels( REPORTER* aStatusReporter );

    /**
     * @param aRenderTopOrBot true will render Top, false will render bottom
     * @param aRenderTransparentOnly true will render only the transparent objects, false will
     *                               render opaque
     */
    void render3dModels( bool aRenderTopOrBot, bool aRenderTransparentOnly );

    void render3dModelsSelected( bool aRenderTopOrBot, bool aRenderTransparentOnly,
                                 bool aRenderSelectedOnly );

    void renderFootprint( const FOOTPRINT* aFootprint, bool aRenderTransparentOnly,
                          bool aIsSelected );

    void setLightFront( bool enabled );
    void setLightTop( bool enabled );
    void setLightBottom( bool enabled );

    void render3dArrows();

    /**
     * Create a 3D grid to an OpenGL display list.
     *
     * A horizontal grid (XY plane and Z = 0, and a vertical grid (XZ plane and Y = 0).
     */
    void generate3dGrid( GRID3D_TYPE aGridType );

    // Materials
    void setupMaterials();

    void setCopperMaterial();
    void setPlatedCopperAndDepthOffset( PCB_LAYER_ID aLayer_id );
    void unsetDepthOffset();

    void setLayerMaterial( PCB_LAYER_ID aLayerID );
    SFVEC4F getLayerColor( PCB_LAYER_ID aLayerID );

    bool initializeOpenGL();
    OPENGL_RENDER_LIST* createBoard( const SHAPE_POLY_SET& aBoardPoly,
                                     const BVH_CONTAINER_2D* aThroughHoles = nullptr );
    void reload( REPORTER* aStatusReporter, REPORTER* aWarningReporter );

    void setArrowMaterial();

    void freeAllLists();

    struct
    {
        SMATERIAL m_Paste;
        SMATERIAL m_SilkSBot;
        SMATERIAL m_SilkSTop;
        SMATERIAL m_SolderMask;
        SMATERIAL m_EpoxyBoard;
        SMATERIAL m_NonPlatedCopper; // raw copper
        SMATERIAL m_Copper;
        SMATERIAL m_Plastic;
        SMATERIAL m_GrayMaterial;
    } m_materials;

    MAP_OGL_DISP_LISTS  m_layers;
    OPENGL_RENDER_LIST* m_platedPadsFront;
    OPENGL_RENDER_LIST* m_platedPadsBack;
    MAP_OGL_DISP_LISTS  m_outerLayerHoles;
    MAP_OGL_DISP_LISTS  m_innerLayerHoles;
    OPENGL_RENDER_LIST* m_board;
    OPENGL_RENDER_LIST* m_boardWithHoles;
    OPENGL_RENDER_LIST* m_antiBoard;
    OPENGL_RENDER_LIST* m_outerThroughHoles;
    OPENGL_RENDER_LIST* m_outerViaThroughHoles;
    OPENGL_RENDER_LIST* m_outerThroughHoleRings;

    LIST_TRIANGLES      m_triangles;       ///< store pointers so can be deleted latter
    GLuint              m_circleTexture;

    GLuint              m_grid;             ///< oGL list that stores current grid
    GRID3D_TYPE         m_lastGridType;     ///< Stores the last grid type.

    OPENGL_RENDER_LIST* m_vias;
    OPENGL_RENDER_LIST* m_padHoles;

    MAP_3DMODEL         m_3dModelMap;

    BOARD_ITEM*         m_currentRollOverItem;

    SHAPE_POLY_SET      m_antiBoardPolys;   ///< The negative polygon representation of the board
                                            ///< outline.
};

#endif // RENDER_3D_LEGACY_H_

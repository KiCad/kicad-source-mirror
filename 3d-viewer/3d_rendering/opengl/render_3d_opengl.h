/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#ifndef RENDER_3D_OPENGL_H
#define RENDER_3D_OPENGL_H

#include "../render_3d_base.h"
#include "layer_triangles.h"
#include "3d_spheres_gizmo.h"

#include "../raytracing/shapes2D/polygon_2d.h"
#include "../raytracing/shapes2D/triangle_2d.h"
#include "../raytracing/shapes2D/4pt_polygon_2d.h"
#include "../raytracing/shapes2D/filled_circle_2d.h"
#include "../raytracing/shapes2D/ring_2d.h"
#include "../raytracing/shapes2D/round_segment_2d.h"

#include "3d_model.h"

#include "3d_cache/3d_info.h"

#include <geometry/eda_angle.h>

#include <map>

typedef std::map< PCB_LAYER_ID, OPENGL_RENDER_LIST* > MAP_OGL_DISP_LISTS;
typedef std::list<TRIANGLE_DISPLAY_LIST* > LIST_TRIANGLES;

#define SIZE_OF_CIRCLE_TEXTURE 1024

/**
 * Object to render the board using openGL.
 */
class RENDER_3D_OPENGL : public RENDER_3D_BASE
{
public:
    explicit RENDER_3D_OPENGL( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter, CAMERA& aCamera );

    ~RENDER_3D_OPENGL();

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
    void                                handleGizmoMouseInput( int mouseX, int mouseY );
    void                                setGizmoViewport( int x, int y, int width, int height );
    std::tuple<int, int, int, int>      getGizmoViewport() const;
    SPHERES_GIZMO::GizmoSphereSelection getSelectedGizmoSphere() const;
    void                                resetSelectedGizmoSphere();

private:
    OPENGL_RENDER_LIST* generateHoles( const LIST_OBJECT2D& aListHolesObject2d,
                                       const SHAPE_POLY_SET& aPoly, float aZtop, float aZbot,
                                       bool aInvertFaces,
                                       const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    OPENGL_RENDER_LIST* generateLayerList( const BVH_CONTAINER_2D* aContainer,
                                           const SHAPE_POLY_SET* aPolyList, PCB_LAYER_ID aLayer,
                                           const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    OPENGL_RENDER_LIST* generateEmptyLayerList( PCB_LAYER_ID aLayer );

    void addTopAndBottomTriangles( TRIANGLE_DISPLAY_LIST* aDst, const SFVEC2F& v0,
                                   const SFVEC2F& v1, const SFVEC2F& v2, float top, float bot );

    void addObjectTriangles( const RING_2D* aRing, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const POLYGON_4PT_2D* aPoly, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const FILLED_CIRCLE_2D* aCircle, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const TRIANGLE_2D* aTri, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void addObjectTriangles( const ROUND_SEGMENT_2D* aSeg, TRIANGLE_DISPLAY_LIST* aDstLayer,
                             float aZtop, float aZbot );

    void renderSolderMaskLayer( PCB_LAYER_ID aLayerID, float aZPos, bool aShowThickness,
                                bool aSkipRenderHoles );

    void renderBoardBody( bool aSkipRenderHoles );

    void getLayerZPos( PCB_LAYER_ID aLayerID, float& aOutZtop, float& aOutZbot ) const;

    void generateRing( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                       unsigned int aNr_sides_per_circle,
                       std::vector< SFVEC2F >& aInnerContourResult,
                       std::vector< SFVEC2F >& aOuterContourResult, bool aInvertOrder );

    void generateCylinder( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                           float aZtop, float aZbot, unsigned int aNr_sides_per_circle,
                           TRIANGLE_DISPLAY_LIST* aDstLayer );

    void generateInvCone( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                          float aZtop, float aZbot, unsigned int aNr_sides_per_circle,
                          TRIANGLE_DISPLAY_LIST* aDstLayer, EDA_ANGLE aAngle );

    void generateDisk( const SFVEC2F& aCenter, float aRadius, float aZ,
                       unsigned int aNr_sides_per_circle, TRIANGLE_DISPLAY_LIST* aDstLayer,
                       bool aTop );

    void generateDimple( const SFVEC2F& aCenter, float aRadius, float aZ, float aDepth,
                         unsigned int aNr_sides_per_circle, TRIANGLE_DISPLAY_LIST* aDstLayer,
                         bool aTop );

    void generateViasAndPads();

    bool appendPostMachiningGeometry( TRIANGLE_DISPLAY_LIST* aDstLayer,
                                      const SFVEC2F& aHoleCenter,
                                      PAD_DRILL_POST_MACHINING_MODE aMode,
                                      int aSizeIU,
                                      int aDepthIU,
                                      float aHoleInnerRadius,
                                      float aZSurface,
                                      bool aIsFront,
                                      float aPlatingThickness3d,
                                      float aUnitScale,
                                      float* aZEnd );

    void generateViaBarrels( float aPlatingThickness3d, float aUnitScale );

    void generatePlatedHoleShells( int aPlatingThickness, float aUnitScale );

    void generateViaCovers( float aPlatingThickness3d, float aUnitScale );

    /**
     * Load footprint models from the cache and load it to openGL lists in the form of
     * #MODEL_3D objects.
     *
     * This map of models will work as a local cache for this render. (cache based on
     * MODEL_3D with associated openGL lists in GPU memory)
     */
    void load3dModels( REPORTER* aStatusReporter );

    struct MODELTORENDER
    {
        glm::mat4 m_modelWorldMat;
        const MODEL_3D* m_model;
        float m_opacity;
        bool m_isTransparent;
        bool m_isSelected;

        MODELTORENDER( glm::mat4 aModelWorldMat,
                       const MODEL_3D* aNodel,
                       float aOpacity,
                       bool aIsTransparent,
                       bool aIsSelected ) :
                       m_modelWorldMat( std::move( aModelWorldMat ) ),
                       m_model( aNodel ),
                       m_opacity( aOpacity ),
                       m_isTransparent( aIsTransparent ),
                       m_isSelected( aIsSelected )
        {
        }
    };

    void renderOpaqueModels( const glm::mat4 &aCameraViewMatrix );
    void renderTransparentModels( const glm::mat4 &aCameraViewMatrix );

    void renderModel( const glm::mat4 &aCameraViewMatrix, const MODELTORENDER &aModelToRender,
                      const SFVEC3F &aSelColor, const SFVEC3F *aCameraWorldPos );


    void get3dModelsSelected( std::list<MODELTORENDER> &aDstRenderList, bool aGetTop, bool aGetBot,
                              bool aRenderTransparentOnly, bool aRenderSelectedOnly );

    void get3dModelsFromFootprint( std::list<MODELTORENDER> &aDstRenderList,
                                   const FOOTPRINT* aFootprint, bool aRenderTransparentOnly,
                                   bool aIsSelected );

    void setLightFront( bool enabled );
    void setLightTop( bool enabled );
    void setLightBottom( bool enabled );

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

    bool initializeOpenGL();
    OPENGL_RENDER_LIST* createBoard( const SHAPE_POLY_SET& aBoardPoly,
                                     const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    /**
     * Create ring-shaped plugs for holes that have backdrill or post-machining.
     * These plugs represent the board material that remains in the hole where
     * the backdrill or post-machining didn't reach.
     */
    void backfillPostMachine();

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

    EDA_3D_CANVAS* m_canvas;

    MAP_OGL_DISP_LISTS  m_layers;
    OPENGL_RENDER_LIST* m_platedPadsFront;
    OPENGL_RENDER_LIST* m_platedPadsBack;
    OPENGL_RENDER_LIST* m_offboardPadsFront;
    OPENGL_RENDER_LIST* m_offboardPadsBack;
    MAP_OGL_DISP_LISTS  m_outerLayerHoles;
    MAP_OGL_DISP_LISTS  m_innerLayerHoles;
    OPENGL_RENDER_LIST* m_board;
    OPENGL_RENDER_LIST* m_boardWithHoles;
    OPENGL_RENDER_LIST* m_postMachinePlugs;     ///< Board material plugs for backdrill/counterbore/countersink
    OPENGL_RENDER_LIST* m_antiBoard;
    OPENGL_RENDER_LIST* m_outerThroughHoles;
    OPENGL_RENDER_LIST* m_outerViaThroughHoles;
    OPENGL_RENDER_LIST* m_outerThroughHoleRings;

    LIST_TRIANGLES      m_triangles;       ///< store pointers so can be deleted latter
    GLuint              m_circleTexture;

    GLuint              m_grid;             ///< oGL list that stores current grid
    GRID3D_TYPE         m_lastGridType;     ///< Stores the last grid type.

    OPENGL_RENDER_LIST* m_microviaHoles;
    OPENGL_RENDER_LIST* m_padHoles;
    OPENGL_RENDER_LIST* m_viaFrontCover;
    OPENGL_RENDER_LIST* m_viaBackCover;

    // Caches
    std::map<wxString, MODEL_3D*>           m_3dModelMap;
    std::map<std::vector<float>, glm::mat4> m_3dModelMatrixMap;

    BOARD_ITEM*         m_currentRollOverItem;

    SHAPE_POLY_SET m_antiBoardPolys; ///< The negative polygon representation of the board
                                     ///< outline.
    SPHERES_GIZMO* m_spheres_gizmo;
};

#endif // RENDER_3D_OPENGL_H

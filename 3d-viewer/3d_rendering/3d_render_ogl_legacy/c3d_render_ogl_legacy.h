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
 * @file  c3d_render_ogl_legacy.h
 * @brief
 */

#ifndef RENDER_3D_LEGACY_H_
#define RENDER_3D_LEGACY_H_

#include "../c3d_render_base.h"
#include "clayer_triangles.h"

#include "../3d_render_raytracing/shapes2D/cpolygon2d.h"
#include "../3d_render_raytracing/shapes2D/ctriangle2d.h"
#include "../3d_render_raytracing/shapes2D/cpolygon4pts2d.h"
#include "../3d_render_raytracing/shapes2D/cfilledcircle2d.h"
#include "../3d_render_raytracing/shapes2D/cring2d.h"
#include "../3d_render_raytracing/shapes2D/croundsegment2d.h"

#include "c_ogl_3dmodel.h"

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
    explicit RENDER_3D_LEGACY( BOARD_ADAPTER& aAdapter, CAMERA& aCamera );

    ~RENDER_3D_LEGACY();

    void SetCurWindowSize( const wxSize& aSize ) override;
    bool Redraw( bool aIsMoving, REPORTER* aStatusReporter, REPORTER* aWarningReporter ) override;

    int GetWaitForEditingTimeOut() override;

    void SetCurrentIntersectedBoardItem( BOARD_ITEM* aCurrentIntersectedBoardItem )
    {
        m_currentIntersectedBoardItem = aCurrentIntersectedBoardItem;
    }

private:
    OPENGL_RENDER_LIST* generate_holes_display_list( const LIST_OBJECT2D& aListHolesObject2d,
                                                     const SHAPE_POLY_SET& aPoly,
                                                     float aZtop,
                                                     float aZbot,
                                                     bool aInvertFaces,
                                                     const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    OPENGL_RENDER_LIST* generateLayerListFromContainer( const BVH_CONTAINER_2D* aContainer,
                                                        const SHAPE_POLY_SET* aPolyList,
                                                        PCB_LAYER_ID aLayerId,
                                                        const BVH_CONTAINER_2D* aThroughHoles = nullptr );

    void add_triangle_top_bot( TRIANGLE_DISPLAY_LIST* aDst, const SFVEC2F& v0, const SFVEC2F& v1,
                               const SFVEC2F& v2, float top, float bot );

    void add_object_to_triangle_layer( const RING_2D* aRing, TRIANGLE_DISPLAY_LIST* aDstLayer,
                                       float aZtop, float aZbot );

    void add_object_to_triangle_layer( const POLYGON_4PT_2D* aPoly,
                                       TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop, float aZbot );

    void add_object_to_triangle_layer( const FILLED_CIRCLE_2D* aFilledCircle,
                                       TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop, float aZbot );

    void add_object_to_triangle_layer( const TRIANGLE_2D* aTri, TRIANGLE_DISPLAY_LIST* aDstLayer,
                                       float aZtop, float aZbot );

    void add_object_to_triangle_layer( const ROUND_SEGMENT_2D* aSeg,
                                       TRIANGLE_DISPLAY_LIST* aDstLayer, float aZtop, float aZbot );

    void render_solder_mask_layer( PCB_LAYER_ID aLayerID, float aZPosition,
                                   bool aDrawMiddleSegments, bool aSkipRenderHoles );

    void render_board_body( bool aSkipRenderHoles );

    void get_layer_z_pos( PCB_LAYER_ID aLayerID, float& aOutZtop, float& aOutZbot ) const;

    void generate_ring_contour( const SFVEC2F& aCenter, float aInnerRadius,
                                float aOuterRadius, unsigned int aNr_sides_per_circle,
                                std::vector< SFVEC2F >& aInnerContourResult,
                                std::vector< SFVEC2F >& aOuterContourResult,
                                bool aInvertOrder );

    void generate_cylinder( const SFVEC2F& aCenter, float aInnerRadius, float aOuterRadius,
                            float aZtop, float aZbot, unsigned int aNr_sides_per_circle,
                            TRIANGLE_DISPLAY_LIST* aDstLayer );

    void generate_3D_Vias_and_Pads();

    /**
     * Load footprint models from the cache and load it to openGL lists in the form of
     * #MODEL_3D objects.
     *
     * This map of models will work as a local cache for this render. (cache based on
     * MODEL_3D with associated openGL lists in GPU memory)
     */
    void load_3D_models( REPORTER* aStatusReporter );

    /**
     * @param aRenderTopOrBot true will render Top, false will render bottom
     * @param aRenderTransparentOnly true will render only the transparent objects, false will
     *                               render opaque
     */
    void render_3D_models( bool aRenderTopOrBot, bool aRenderTransparentOnly );

    void render_3D_models_selected( bool aRenderTopOrBot, bool aRenderTransparentOnly,
                                    bool aRenderSelectedOnly );

    void render_3D_footprint( const FOOTPRINT* aFootprint, bool aRenderTransparentOnly,
                              bool aIsSelected );

    void setLight_Front( bool enabled );
    void setLight_Top( bool enabled );
    void setLight_Bottom( bool enabled );

    void render_3D_arrows();

    /**
     * Create a 3D grid to an OpenGL display list.
     *
     * A horizontal grid (XY plane and Z = 0, and a vertical grid (XZ plane and Y = 0).
     */
    void generate_new_3DGrid( GRID3D_TYPE aGridType );

    // Materials
    void setupMaterials();

    void setCopperMaterial();
    void setPlatedCopperAndDepthOffset( PCB_LAYER_ID aLayer_id );
    void unsetDepthOffset();

    void set_layer_material( PCB_LAYER_ID aLayerID );
    SFVEC4F get_layer_color( PCB_LAYER_ID aLayerID );

    bool initializeOpenGL();
    OPENGL_RENDER_LIST* createBoard( const SHAPE_POLY_SET& aBoardPoly,
                                     const BVH_CONTAINER_2D* aThroughHoles = nullptr );
    void reload( REPORTER* aStatusReporter, REPORTER* aWarningReporter );

    void ogl_set_arrow_material();

    void ogl_free_all_display_lists();

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
    OPENGL_RENDER_LIST* m_platedPads_F_Cu;
    OPENGL_RENDER_LIST* m_platedPads_B_Cu;
    MAP_OGL_DISP_LISTS  m_layers_holes_outer;
    MAP_OGL_DISP_LISTS  m_layers_holes_inner;
    OPENGL_RENDER_LIST* m_board;
    OPENGL_RENDER_LIST* m_board_with_holes;
    OPENGL_RENDER_LIST* m_anti_board;
    OPENGL_RENDER_LIST* m_through_holes_outer;
    OPENGL_RENDER_LIST* m_through_holes_vias_outer;
    OPENGL_RENDER_LIST* m_through_holes_outer_ring;
    OPENGL_RENDER_LIST* m_vias_and_pad_holes_outer_contourn_and_caps;

    LIST_TRIANGLES m_triangles;     ///< store pointers so can be deleted latter
    GLuint m_ogl_circle_texture;

    GLuint m_grid;                  ///< oGL list that stores current grid
    GRID3D_TYPE m_last_grid_type;   ///< Stores the last grid computed

    OPENGL_RENDER_LIST* m_vias;
    OPENGL_RENDER_LIST* m_pad_holes;

    MAP_3DMODEL m_3dmodel_map;

    BOARD_ITEM* m_currentIntersectedBoardItem;

    SHAPE_POLY_SET m_anti_board_poly; ///< negative polygon representation of the board outline
};

#endif // RENDER_3D_LEGACY_H_

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
 * @file  c3d_render_ogl_legacy.h
 * @brief
 */

#ifndef C3D_RENDER_OGL_LEGACY_H_
#define C3D_RENDER_OGL_LEGACY_H_

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


typedef std::map< LAYER_ID, CLAYERS_OGL_DISP_LISTS* > MAP_OGL_DISP_LISTS;
typedef std::map< LAYER_ID, CLAYER_TRIANGLES * > MAP_TRIANGLES;
typedef std::map< wxString, C_OGL_3DMODEL * > MAP_3DMODEL;

#define SIZE_OF_CIRCLE_TEXTURE 1024

/**
 * @brief The C3D_RENDER_OGL_LEGACY class render the board using openGL legacy mode
 */
class C3D_RENDER_OGL_LEGACY : public C3D_RENDER_BASE
{
public:
    explicit C3D_RENDER_OGL_LEGACY( CINFO3D_VISU &aSettings );

    ~C3D_RENDER_OGL_LEGACY();

    // Imported from C3D_RENDER_BASE
    void SetCurWindowSize( const wxSize &aSize ) override;
    bool Redraw( bool aIsMoving, REPORTER *aStatusTextReporter ) override;

    int GetWaitForEditingTimeOut() override;

private:
    bool initializeOpenGL();
    void reload( REPORTER *aStatusTextReporter );

    void ogl_set_arrow_material();

    void ogl_free_all_display_lists();
    MAP_OGL_DISP_LISTS      m_ogl_disp_lists_layers;
    MAP_OGL_DISP_LISTS      m_ogl_disp_lists_layers_holes_outer;
    MAP_OGL_DISP_LISTS      m_ogl_disp_lists_layers_holes_inner;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_board;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_through_holes_outer;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_through_holes_inner;

    // User for body render
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_through_holes_outer_with_npth;

    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_through_holes_vias_outer;
    //CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_through_holes_vias_inner; // Not in use

    // This is for pads holes of the modules
    //CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_vias_and_pad_holes_inner_contourn_and_caps;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_vias_and_pad_holes_outer_contourn_and_caps;

    MAP_TRIANGLES           m_triangles;

    GLuint m_ogl_circle_texture;

    GLuint m_ogl_disp_list_grid;    ///< oGL list that stores current grid

    GRID3D_TYPE m_last_grid_type;   ///< Stores the last grid computed

    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_via;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_pads_holes;

    MAP_3DMODEL m_3dmodel_map;

private:
    void generate_through_outer_holes();
    void generate_through_inner_holes();

    CLAYERS_OGL_DISP_LISTS *generate_holes_display_list( const LIST_OBJECT2D &aListHolesObject2d,
                                                         const SHAPE_POLY_SET &aPoly,
                                                         float aZtop,
                                                         float aZbot,
                                                         bool aInvertFaces );

    void add_triangle_top_bot( CLAYER_TRIANGLES *aDst,
                               const SFVEC2F &v0,
                               const SFVEC2F &v1,
                               const SFVEC2F &v2,
                               float top,
                               float bot );

    void add_object_to_triangle_layer( const CRING2D *aRing,
                                       CLAYER_TRIANGLES *aDstLayer,
                                       float aZtop,
                                       float aZbot );

    void add_object_to_triangle_layer( const CPOLYGON4PTS2D *aPoly,
                                       CLAYER_TRIANGLES *aDstLayer,
                                       float aZtop,
                                       float aZbot );

    void add_object_to_triangle_layer( const CFILLEDCIRCLE2D *aFilledCircle,
                                       CLAYER_TRIANGLES *aDstLayer,
                                       float aZtop,
                                       float aZbot );

    void add_object_to_triangle_layer( const CTRIANGLE2D *aTri,
                                       CLAYER_TRIANGLES *aDstLayer,
                                       float aZtop,
                                       float aZbot );

    void add_object_to_triangle_layer( const CROUNDSEGMENT2D *aSeg,
                                       CLAYER_TRIANGLES *aDstLayer,
                                       float aZtop,
                                       float aZbot );

    void render_solder_mask_layer( LAYER_ID aLayerID,
                                   float aZPosition,
                                   bool aIsRenderingOnPreviewMode );

    void get_layer_z_pos( LAYER_ID aLayerID,
                          float &aOutZtop,
                          float &aOutZbot ) const;

    void generate_ring_contour( const SFVEC2F &aCenter,
                                float aInnerRadius,
                                float aOuterRadius,
                                unsigned int aNr_sides_per_circle,
                                std::vector< SFVEC2F > &aInnerContourResult,
                                std::vector< SFVEC2F > &aOuterContourResult,
                                bool aInvertOrder );

    void generate_cylinder( const SFVEC2F &aCenter,
                            float aInnerRadius,
                            float aOuterRadius,
                            float aZtop,
                            float aZbot,
                            unsigned int aNr_sides_per_circle,
                            CLAYER_TRIANGLES *aDstLayer );

    void generate_3D_Vias_and_Pads();

    void load_3D_models();

    /**
     * @brief render_3D_models
     * @param aRenderTopOrBot - true will render Top, false will render bottom
     * @param aRenderTransparentOnly - true will render only the transparent
     * objects, false will render opaque
     */
    void render_3D_models( bool aRenderTopOrBot, bool aRenderTransparentOnly );

    void render_3D_module( const MODULE* module, bool aRenderTransparentOnly );

    void setLight_Front( bool enabled );
    void setLight_Top( bool enabled );
    void setLight_Bottom( bool enabled );

    void render_3D_arrows();

    void generate_new_3DGrid( GRID3D_TYPE aGridType );

    // Materials
    void setupMaterials();

    struct
    {
        SMATERIAL m_Paste;
        SMATERIAL m_SilkS;
        SMATERIAL m_SolderMask;
        SMATERIAL m_EpoxyBoard;
        SMATERIAL m_Copper;
        SMATERIAL m_Plastic;
        SMATERIAL m_GrayMaterial;
    }m_materials;

    void set_layer_material( LAYER_ID aLayerID );
    SFVEC3F get_layer_color( LAYER_ID aLayerID );

public:
    const MAP_OGL_DISP_LISTS &GetLayerDispListMap() const { return m_ogl_disp_lists_layers; }
    const CLAYERS_OGL_DISP_LISTS *GetLayerDispList( LAYER_ID aLayerId ) const { return m_ogl_disp_lists_layers.at( aLayerId ); }
    const CLAYERS_OGL_DISP_LISTS *GetBoardDispList() const { return m_ogl_disp_list_board; }
};

#endif // C3D_RENDER_OGL_LEGACY_H_


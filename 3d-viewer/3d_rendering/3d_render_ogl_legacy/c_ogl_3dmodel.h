/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  c_ogl_3dmodel.h
 * @brief implement a legacy 3dmodel render
 */

#ifndef _C_OGL_3DMODEL_H_
#define _C_OGL_3DMODEL_H_

#include <vector>
#include <plugins/3dapi/c3dmodel.h>
#include "../../common_ogl/openGL_includes.h"
#include "../3d_render_raytracing/shapes3D/cbbox.h"
#include "../../3d_enums.h"

///
class  C_OGL_3DMODEL
{
public:
    /**
     * @brief C_OGL_3DMODEL - Load a 3d model. This must be called inside a gl context
     * @param a3DModel: a 3d model data to load.
     * @param aMaterialMode: a mode to render the materials of the model
     */
    C_OGL_3DMODEL( const S3DMODEL &a3DModel, MATERIAL_MODE aMaterialMode );

    ~C_OGL_3DMODEL();

    /**
     * @brief Draw_opaque - render the model into the current context
     */
    void Draw_opaque( bool aUseSelectedMaterial, SFVEC3F aSelectionColor = SFVEC3F( 0.0f ) ) const { Draw( false, 1.0f, aUseSelectedMaterial, aSelectionColor ); }

    /**
     * @brief Draw_transparent - render the model into the current context
     */
    void Draw_transparent( float aOpacity, bool aUseSelectedMaterial, SFVEC3F aSelectionColor = SFVEC3F( 0.0f ) ) const { Draw( true, aOpacity, aUseSelectedMaterial, aSelectionColor ); }

    /**
     * @brief Have_opaque - return true if have opaque meshs to render
     */
    bool Have_opaque() const { return m_have_opaque_meshes; }

    /**
     * @brief Have_transparent - return true if have transparent meshs to render
     */
    bool Have_transparent() const { return m_have_transparent_meshes; }

    /**
     * @brief Draw_bbox - draw main bounding box of the model
     */
    void Draw_bbox() const;

    /**
     * @brief Draw_bboxes - draw individual bounding boxes of each mesh
     */
    void Draw_bboxes() const;

    /**
     * @brief GetBBox - Get main bbox
     * @return the main model bbox
     */
    const CBBOX &GetBBox() const { return m_model_bbox; }

    /**
     * @brief BeginDrawMulti - set some basic render states before drawing multiple models
     */
    static void BeginDrawMulti( bool aUseColorInformation );

    /**
     * @brief EndDrawMulti - cleanup render states after drawing multiple models
     */
    static void EndDrawMulti();

private:
    static const wxChar *m_logTrace;

    // the material mode that was used to generate the rendering data.
    // FIXME: this can be selected at run-time and does not require re-creation
    //        of the whole model objects.
    MATERIAL_MODE m_material_mode;

    CBBOX   m_model_bbox;               ///< global bounding box for this model
    std::vector<CBBOX> m_meshes_bbox;   ///< individual bbox for each mesh

    // unified vertex format for mesh rendering.
    struct VERTEX
    {
        glm::vec3   m_pos;
        glm::u8vec4 m_nrm;        // only 3 components used
        glm::u8vec4 m_color;      // regular color
        glm::u8vec4 m_cad_color;  // "CAD" mode rendering color
        glm::vec2   m_tex_uv;
    };

    // vertex buffer and index buffers that include all the individual meshes
    // lumped together.
    GLuint m_vertex_buffer = 0;
    GLuint m_index_buffer = 0;
    GLenum m_index_buffer_type = GL_INVALID_ENUM;

    // internal material definition
    // all meshes are grouped by material for rendering purposes.
    struct MATERIAL : SMATERIAL
    {
        unsigned int m_render_idx_buffer_offset = 0;
        unsigned int m_render_idx_count = 0;

        MATERIAL( const SMATERIAL &aOther ) : SMATERIAL( aOther ) { }
        bool IsTransparent() const { return m_Transparency > FLT_EPSILON; }
    };

    std::vector<MATERIAL> m_materials;

    // a model can consist of transparent and opaque parts.  remember which
    // ones are present during initial buffer and data setup.  use it later
    // during rendering.
    bool m_have_opaque_meshes = false;
    bool m_have_transparent_meshes = false;

    // vertex buffer and index buffer for the bounding boxes.
    // the first box is always the outer box, followed by inner boxes (one for each mesh).
    static constexpr unsigned int bbox_vtx_count = 8;
    static constexpr unsigned int bbox_idx_count = 24;

    GLuint m_bbox_vertex_buffer = 0;
    GLuint m_bbox_index_buffer = 0;
    GLenum m_bbox_index_buffer_type = GL_INVALID_ENUM;

    static void MakeBbox( const CBBOX &aBox, unsigned int aIdxOffset,
                          VERTEX *aVtxOut, GLuint *aIdxOut,
                          const glm::vec4 &aColor );

    void Draw( bool aTransparent, float aOpacity, bool aUseSelectedMaterial, SFVEC3F aSelectionColor ) const;
};

#endif // _C_OGL_3DMODEL_H_

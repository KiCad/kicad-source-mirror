/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Oleg Endo <olegendo@gcc.gnu.org>
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

#ifndef _MODEL_3D_H_
#define _MODEL_3D_H_

#include <vector>
#include <plugins/3dapi/c3dmodel.h>
#include "../../common_ogl/openGL_includes.h"
#include "../raytracing/shapes3D/bbox_3d.h"
#include <3d_enums.h>

#include <wx/chartype.h>

class MODEL_3D
{
public:
    /**
     * Load a 3D model.
     *
     * @note This must be called inside a gl context.
     *
     * @param a3DModel a 3d model data to load.
     * @param aMaterialMode a mode to render the materials of the model.
     */
    MODEL_3D( const S3DMODEL& a3DModel, MATERIAL_MODE aMaterialMode );

    ~MODEL_3D();

    /**
     * Render the model into the current context.
     */
    void DrawOpaque( bool aUseSelectedMaterial, SFVEC3F aSelectionColor = SFVEC3F( 0.0f ) ) const
    {
        Draw( false, 1.0f, aUseSelectedMaterial, aSelectionColor, nullptr, nullptr );
    }

    /**
     * Render the model into the current context.
     */
    void DrawTransparent( float aOpacity, bool aUseSelectedMaterial,
                          SFVEC3F aSelectionColor = SFVEC3F( 0.0f ) ) const
    {
        Draw( true, aOpacity, aUseSelectedMaterial, aSelectionColor, nullptr, nullptr );
    }

    /**
     * Render the model into the current context.
     * if aModelWorldMatrix and aCameraWorldPos is provided,
     * it renders the material groups sorted.
     */
    void Draw( bool aTransparent, float aOpacity, bool aUseSelectedMaterial,
               const SFVEC3F& aSelectionColor,
               const glm::mat4 *aModelWorldMatrix,
               const SFVEC3F *aCameraWorldPos ) const;

    /**
     * Return true if have opaque meshes to render.
     */
    bool HasOpaqueMeshes() const { return m_have_opaque_meshes; }

    /**
     * Return true if have transparent mesh's to render.
     */
    bool HasTransparentMeshes() const { return m_have_transparent_meshes; }

    /**
     * Draw main bounding box of the model.
     */
    void DrawBbox() const;

    /**
     * Draw individual bounding boxes of each mesh.
     */
    void DrawBboxes() const;

    /**
     * Get the main bounding box.
     * @return the main model bounding box.
     */
    const BBOX_3D& GetBBox() const { return m_model_bbox; }

    /**
     * Set some basic render states before drawing multiple models.
     */
    static void BeginDrawMulti( bool aUseColorInformation );

    /**
     * Cleanup render states after drawing multiple models.
     */
    static void EndDrawMulti();

private:
    static const wxChar* m_logTrace;

    // the material mode that was used to generate the rendering data.
    // FIXME: this can be selected at run-time and does not require re-creation
    //        of the whole model objects.
    MATERIAL_MODE m_materialMode;

    BBOX_3D   m_model_bbox;               ///< global bounding box for this model
    std::vector<BBOX_3D> m_meshes_bbox;   ///< individual bbox for each mesh

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

        BBOX_3D m_bbox; ///< bounding box for this material group, used for transparent
                        ///< material ordering.

        MATERIAL( const SMATERIAL& aOther ) : SMATERIAL( aOther ) { }
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

    static void MakeBbox( const BBOX_3D& aBox, unsigned int aIdxOffset, VERTEX* aVtxOut,
                          GLuint* aIdxOut, const glm::vec4& aColor );
};

#endif // _MODEL_3D_H_

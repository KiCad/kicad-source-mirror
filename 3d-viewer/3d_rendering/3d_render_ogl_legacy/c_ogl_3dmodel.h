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
 * @file  c_ogl_3dmodel.h
 * @brief implement a legacy 3dmodel render
 */

#ifndef _C_OGL_3DMODEL_H_
#define _C_OGL_3DMODEL_H_

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
    void Draw_opaque() const;

    /**
     * @brief Draw_transparent - render the model into the current context
     */
    void Draw_transparent() const;

    /**
     * @brief Have_opaque - return true if have opaque meshs to render
     */
    bool Have_opaque() const;

    /**
     * @brief Have_transparent - return true if have transparent meshs to render
     */
    bool Have_transparent() const;

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

private:
    GLuint  m_ogl_idx_list_opaque;      ///< display list for rendering opaque meshes
    GLuint  m_ogl_idx_list_transparent; ///< display list for rendering transparent meshes
    GLuint  m_ogl_idx_list_meshes;      ///< display lists for all meshes.
    unsigned int m_nr_meshes;           ///< number of meshes of this model

    CBBOX   m_model_bbox;               ///< global bounding box for this model
    CBBOX  *m_meshs_bbox;               ///< individual bbox for each mesh
};

#endif // _C_OGL_3DMODEL_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <map>


typedef std::map< LAYER_ID, CLAYERS_OGL_DISP_LISTS* > MAP_OGL_DISP_LISTS;
typedef std::map< LAYER_ID, CLAYER_TRIANGLES * > MAP_TRIANGLES;

#define SIZE_OF_CIRCLE_TEXTURE 512

/**
 * @brief The C3D_RENDER_OGL_LEGACY class render the board using openGL legacy mode
 */
class C3D_RENDER_OGL_LEGACY : public C3D_RENDER_BASE
{
public:
    C3D_RENDER_OGL_LEGACY( CINFO3D_VISU &aSettings,
                           S3D_CACHE *a3DModelManager );

    ~C3D_RENDER_OGL_LEGACY();

    // Imported from C3D_RENDER_BASE
    void SetCurWindowSize( const wxSize &aSize );
    void Redraw( bool aIsMoving );

private:
    bool initializeOpenGL();
    void reload();

    void ogl_set_arrow_material();

    void ogl_free_all_display_lists();
    MAP_OGL_DISP_LISTS      m_ogl_disp_lists_layers;
    CLAYERS_OGL_DISP_LISTS* m_ogl_disp_list_board;
    MAP_TRIANGLES           m_triangles;

    GLuint m_ogl_circle_texture;

private:
    void add_triangle_top_bot( CLAYER_TRIANGLES *aDst, const SFVEC2F &v0, const SFVEC2F &v1, const SFVEC2F &v2, float top, float bot );

public:
    const MAP_OGL_DISP_LISTS &GetLayerDispListMap() const { return m_ogl_disp_lists_layers; }
    const CLAYERS_OGL_DISP_LISTS *GetLayerDispList( LAYER_ID aLayerId ) const { return m_ogl_disp_lists_layers.at( aLayerId ); }
    const CLAYERS_OGL_DISP_LISTS *GetBoardDispList() const { return m_ogl_disp_list_board; }
};

#endif // C3D_RENDER_OGL_LEGACY_H_

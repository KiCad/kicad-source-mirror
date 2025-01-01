/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2020 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 2024 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#ifndef RENDER_3D_RAYTRACE_GL_H
#define RENDER_3D_RAYTRACE_GL_H

#include "../../common_ogl/openGL_includes.h"
#include "render_3d_raytrace_base.h"


class RENDER_3D_RAYTRACE_GL : public RENDER_3D_RAYTRACE_BASE
{
public:
    explicit RENDER_3D_RAYTRACE_GL( EDA_3D_CANVAS* aCanvas, BOARD_ADAPTER& aAdapter,
                                    CAMERA& aCamera );

    ~RENDER_3D_RAYTRACE_GL();

    void SetCurWindowSize( const wxSize& aSize ) override;
    bool Redraw( bool aIsMoving, REPORTER* aStatusReporter, REPORTER* aWarningReporter ) override;

protected:
    void initPbo() override;
    void deletePbo() override;

    bool   m_openglSupportsVertexBufferObjects;
    GLuint m_pboId;
    GLuint m_pboDataSize;
};


#endif // RENDER_3D_RAYTRACE_GL_H

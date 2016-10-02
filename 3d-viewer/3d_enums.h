/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file  3d_enums.h
 * @brief declared enumerations and flags
 */

#ifndef _3D_ENUMS_H_
#define _3D_ENUMS_H_

/// Flags used in rendering options
enum DISPLAY3D_FLG {
    FL_AXIS=0, FL_ZONE,
    FL_ADHESIVE, FL_SILKSCREEN, FL_SOLDERMASK, FL_SOLDERPASTE,
    FL_COMMENTS, FL_ECO,

    FL_MODULE_ATTRIBUTES_NORMAL,
    FL_MODULE_ATTRIBUTES_NORMAL_INSERT,
    FL_MODULE_ATTRIBUTES_VIRTUAL,

    FL_SHOW_BOARD_BODY,
    FL_MOUSEWHEEL_PANNING,
    FL_USE_REALISTIC_MODE,
    FL_RENDER_SHOW_HOLES_IN_ZONES,

    // OpenGL options
    FL_RENDER_OPENGL_SHOW_MODEL_BBOX,
    FL_RENDER_OPENGL_COPPER_THICKNESS,

    // Raytracing options
    FL_RENDER_RAYTRACING_SHADOWS,
    FL_RENDER_RAYTRACING_BACKFLOOR,
    FL_RENDER_RAYTRACING_REFRACTIONS,
    FL_RENDER_RAYTRACING_REFLECTIONS,
    FL_RENDER_RAYTRACING_POST_PROCESSING,
    FL_RENDER_RAYTRACING_ANTI_ALIASING,
    FL_RENDER_RAYTRACING_PROCEDURAL_TEXTURES,
    FL_LAST
};


/// Camera types
enum CAMERA_TYPE
{
    CAMERA_TRACKBALL
};


/// Grid types
enum GRID3D_TYPE
{
    GRID3D_NONE,
    GRID3D_1MM,
    GRID3D_2P5MM,
    GRID3D_5MM,
    GRID3D_10MM
};


/// Render engine mode
enum RENDER_ENGINE
{
    RENDER_ENGINE_OPENGL_LEGACY,
    RENDER_ENGINE_RAYTRACING,
};


/// Render 3d model shape materials mode
enum MATERIAL_MODE
{
    MATERIAL_MODE_NORMAL,       ///< Use all material properties from model file
    MATERIAL_MODE_DIFFUSE_ONLY, ///< Use only diffuse material properties
    MATERIAL_MODE_CAD_MODE      ///< Use a gray shading based on diffuse material
};

#endif // _3D_ENUMS_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
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
 * @file  3d_enums.h
 * @brief declared enumerations and flags
 */

#ifndef _3D_ENUMS_H_
#define _3D_ENUMS_H_

/// Flags used in rendering options
enum DISPLAY3D_FLG
{
    FL_AXIS = 0,
    FL_ZONE,
    FL_ADHESIVE,
    FL_SILKSCREEN,
    FL_SOLDERMASK,
    FL_SOLDERPASTE,
    FL_COMMENTS,
    FL_ECO,

    FL_FP_ATTRIBUTES_NORMAL,
    FL_FP_ATTRIBUTES_NORMAL_INSERT,
    FL_FP_ATTRIBUTES_VIRTUAL,

    FL_USE_SELECTION,
    FL_HIGHLIGHT_ROLLOVER_ITEM,

    FL_SHOW_BOARD_BODY,
    FL_MOUSEWHEEL_PANNING,
    FL_USE_REALISTIC_MODE,
    FL_SUBTRACT_MASK_FROM_SILK,
    FL_CLIP_SILK_ON_VIA_ANNULUS,
    FL_RENDER_PLATED_PADS_AS_PLATED,

    // OpenGL options
    FL_RENDER_OPENGL_SHOW_MODEL_BBOX,
    FL_RENDER_OPENGL_COPPER_THICKNESS,
    FL_RENDER_OPENGL_AA_DISABLE_ON_MOVE,
    FL_RENDER_OPENGL_THICKNESS_DISABLE_ON_MOVE,
    FL_RENDER_OPENGL_VIAS_DISABLE_ON_MOVE,
    FL_RENDER_OPENGL_HOLES_DISABLE_ON_MOVE,

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


/// Rotation direction for the 3d canvas
enum class ROTATION_DIR
{
    X_CW,
    X_CCW,
    Y_CW,
    Y_CCW,
    Z_CW,
    Z_CCW
};


/// Camera types
enum class CAMERA_TYPE
{
    TRACKBALL
};


/// Grid types
enum class GRID3D_TYPE
{
    NONE,
    GRID_1MM,
    GRID_2P5MM,
    GRID_5MM,
    GRID_10MM
};


/// Render engine mode
enum class RENDER_ENGINE
{
    OPENGL,
    RAYTRACING,
};


/// Render 3d model shape materials mode
enum class MATERIAL_MODE
{
    NORMAL,       ///< Use all material properties from model file
    DIFFUSE_ONLY, ///< Use only diffuse material properties
    CAD_MODE      ///< Use a gray shading based on diffuse material
};

#endif // _3D_ENUMS_H_

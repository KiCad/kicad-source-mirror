/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

/**
 * @file  3d_enums.h
 * @brief declared enumerations and flags
 */

#ifndef _3D_ENUMS_H_
#define _3D_ENUMS_H_

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

// Do not modify the following enum numbers, they are saved in the config file and used in the UI

/// Grid types
enum class GRID3D_TYPE
{
    NONE       = 0,
    GRID_1MM   = 1,
    GRID_2P5MM = 2,
    GRID_5MM   = 3,
    GRID_10MM  = 4
};

/// Render engine mode
enum class RENDER_ENGINE
{
    OPENGL     = 0,
    RAYTRACING = 1,
};

/// Render 3d model shape materials mode
enum class MATERIAL_MODE
{
    NORMAL       = 0, ///< Use all material properties from model file
    DIFFUSE_ONLY = 1, ///< Use only diffuse material properties
    CAD_MODE     = 2  ///< Use a gray shading based on diffuse material
};

enum class VIEW3D_TYPE
{
    // Specific directions
    VIEW3D_TOP,
    VIEW3D_BOTTOM,
    VIEW3D_LEFT,
    VIEW3D_RIGHT,
    VIEW3D_FRONT,
    VIEW3D_BACK,
    VIEW3D_FLIP,

    // Movement commands
    VIEW3D_PAN_UP,
    VIEW3D_PAN_DOWN,
    VIEW3D_PAN_LEFT,
    VIEW3D_PAN_RIGHT,
    VIEW3D_ZOOM_IN,
    VIEW3D_ZOOM_OUT,
    VIEW3D_PIVOT_CENTER,

    // Specific levels
    VIEW3D_FIT_SCREEN
};

#endif // _3D_ENUMS_H_

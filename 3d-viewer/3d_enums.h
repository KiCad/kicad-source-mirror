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
enum GRID3D_TYPE
{
    NONE,
    GRID_1MM,
    GRID_2P5MM,
    GRID_5MM,
    GRID_10MM
};


/// Render engine mode
enum RENDER_ENGINE
{
    OPENGL,
    RAYTRACING,
};


/// Render 3d model shape materials mode
enum MATERIAL_MODE
{
    NORMAL,       ///< Use all material properties from model file
    DIFFUSE_ONLY, ///< Use only diffuse material properties
    CAD_MODE      ///< Use a gray shading based on diffuse material
};

#endif // _3D_ENUMS_H_

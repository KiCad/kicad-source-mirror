/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file ogl_attr_list.h
 * @brief Declaration of the cogl_att_list class.
 */

#ifndef _OGL_ATT_LIST_H
#define _OGL_ATT_LIST_H

#include <kicad_gl/kiglad.h> // Must be included first
#include <wx/glcanvas.h>

/// Anti-aliasing options
enum class ANTIALIASING_MODE
{
    // Do not change these numbers, they are stored in the config file
    AA_NONE = 0,
    AA_2X   = 1,
    AA_4X   = 2,
    AA_8X   = 3
};

/**
 *  Helper class to create an attribute list.
 */
class OGL_ATT_LIST
{

public:
    /**
     *  Get a list of attributes to pass to wxGLCanvas.
     *
     *  @param aAntiAliasingMode = 0 - disabled; try to initialize (if is supported) the
     *  list with anti aliasing capabilities.
     *  @param aAlpha set to enable alpha channel.
     *  @return wxGLAttributes to be passed in the creation of a EDA_3D_CANVAS class
     */
    static const wxGLAttributes GetAttributesList( ANTIALIASING_MODE aAntiAliasingMode,
                                                   bool              aAlpha = false );
};

#endif // _OGL_ATT_LIST_H

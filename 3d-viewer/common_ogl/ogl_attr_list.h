/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * @file ogl_attr_list.h
 * @brief Declaration of the cogl_att_list class.
 */

#ifndef _OGL_ATT_LIST_H
#define _OGL_ATT_LIST_H

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
     *  list with anti aliasing capabilities
     *  @return a list of options to be passed in the creation of a EDA_3D_CANVAS class
     */
    static const int* GetAttributesList( ANTIALIASING_MODE aAntiAliasingMode );

private:
    /**
     *  Attributes list to be passed to a wxGLCanvas creation.
     *
     *  This array should be 2*n+1
     *  Sadly wxwidgets / glx < 13 allowed
     *  a thing named "boolean attributes" that don't take a value.
     *  (See src/unix/glx11.cpp -> wxGLCanvasX11::ConvertWXAttrsToGL() ).
     *  To avoid problems due to this, just specify those attributes twice.
     *  Only WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_STEREO are such boolean
     *  attributes.
     */
    static const int m_openGL_attributes_list[];

    /**
     * Attributes list that was (eventually) changed and are passed to creation.
     */
    static int m_openGL_attributes_list_to_use[];
};

#endif // _OGL_ATT_LIST_H

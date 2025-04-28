/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * This file is used for including the proper GLEW header for the platform.
 */

#ifndef KIGLEW_H_
#define KIGLEW_H_

// Pull in the configuration options for wxWidgets
#include <wx/platform.h>

#if defined( __unix__ ) and not defined( __APPLE__ )

    #ifdef KICAD_USE_EGL

        #if wxUSE_GLCANVAS_EGL
            // wxWidgets was compiled with the EGL canvas, so use the EGL header for GLEW
            #include <GL/eglew.h>
        #else
            #error "KICAD_USE_EGL can only be used when wxWidgets is compiled with the EGL canvas"
        #endif

    #else   // KICAD_USE_EGL

        #if wxUSE_GLCANVAS_EGL
            #error "KICAD_USE_EGL must be defined since wxWidgets has been compiled with the EGL canvas"
        #else
            // wxWidgets wasn't compiled with the EGL canvas, so use the X11 GLEW
            #include <GL/glxew.h>
        #endif

    #endif  // KICAD_USE_EGL

#else   // defined( __unix__ ) and not defined( __APPLE__ )

    // Non-GTK platforms only need the normal GLEW include
    #include <GL/glew.h>

#endif  // defined( __unix__ ) and not defined( __APPLE__ )

#ifdef _WIN32

    #include <GL/wglew.h>

#endif  // _WIN32

#endif  // KIGLEW_H_

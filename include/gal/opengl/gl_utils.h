/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors
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
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef GL_UTILS_H
#define GL_UTILS_H

#include <gal/opengl/kiglew.h> // Must be included first
#include <wx/glcanvas.h>
#include <wx/utils.h>

#ifdef _WIN32
    #ifdef __MINGW32__
    #pragma GCC push_options
    #pragma GCC optimize( "O0" )
    #else
    #pragma optimize( "", off )
    #endif
#endif

class GL_UTILS
{
public:
    /**
     * Attempt to set the OpenGL swap interval.
     *
     * @param aVal if -1 = try to set adaptive swapping, 0 = sync off, 1 = sync with VSYNC rate.
     * @return actual value set
     */
    static int SetSwapInterval( int aVal )
    {
#if defined( __linux__ ) && !defined( KICAD_USE_EGL )

        if( Display* dpy = glXGetCurrentDisplay() )
        {
            GLXDrawable drawable = glXGetCurrentDrawable();

            std::string exts( glXQueryExtensionsString( dpy, DefaultScreen( dpy ) ) );

            if( glXSwapIntervalEXT && glXQueryDrawable && drawable
                && exts.find( "GLX_EXT_swap_control" ) != std::string::npos )
            {
                if( aVal == -1 )
                {
                    if( exts.find( "GLX_EXT_swap_control_tear" ) == std::string::npos )
                    {
                        aVal = 1;
                    }
                    else
                    {
                        // Even though the extensions might be available,
                        // we need to be sure that late/adaptive swaps are
                        // enabled on the drawable.

                        unsigned lateSwapsEnabled = 0;
                        glXQueryDrawable( dpy, drawable, GLX_LATE_SWAPS_TEAR_EXT,
                                          &lateSwapsEnabled );

                        if( !lateSwapsEnabled )
                        {
                            aVal = 0;
                        }
                    }
                }

                unsigned clampedInterval;
                glXSwapIntervalEXT( dpy, drawable, aVal );
                glXQueryDrawable( dpy, drawable, GLX_SWAP_INTERVAL_EXT, &clampedInterval );

                return clampedInterval;
            }

            if( glXSwapIntervalMESA && glXGetSwapIntervalMESA
                && exts.find( "GLX_MESA_swap_control" ) != std::string::npos )
            {
                if( aVal == -1 )
                    aVal = 1;

                if( !glXSwapIntervalMESA( aVal ) )
                    return aVal;
            }

            if( glXSwapIntervalSGI && exts.find( "GLX_SGI_swap_control" ) != std::string::npos )
            {
                if( aVal == -1 )
                    aVal = 1;

                if( !glXSwapIntervalSGI( aVal ) )
                    return aVal;
            }
        }

#elif defined( _WIN32 )

        const GLubyte* vendor = glGetString( GL_VENDOR );
        const GLubyte* version = glGetString( GL_VERSION );

        if( wglSwapIntervalEXT && wxGLCanvas::IsExtensionSupported( "WGL_EXT_swap_control" ) )
        {
            wxString vendorStr = vendor;
            wxString versionStr = version;

            if( aVal == -1 && ( !wxGLCanvas::IsExtensionSupported( "WGL_EXT_swap_control_tear" ) ) )
                aVal = 1;

            // Trying to enable adaptive swapping on AMD drivers from 2017 or older leads to crash
            if( aVal == -1 && vendorStr == wxS( "ATI Technologies Inc." ) )
            {
                wxArrayString parts = wxSplit( versionStr.AfterLast( ' ' ), '.', 0 );

                if( parts.size() == 4 )
                {
                    long majorVer = 0;

                    if( parts[0].ToLong( &majorVer ) )
                    {
                        if( majorVer <= 22 )
                            aVal = 1;
                    }
                }
            }

            HDC   hdc = wglGetCurrentDC();
            HGLRC hglrc = wglGetCurrentContext();

            if( hdc && hglrc )
            {
                int currentInterval = wglGetSwapIntervalEXT();

                if( currentInterval != aVal )
                {
                    wglSwapIntervalEXT( aVal );
                    currentInterval = wglGetSwapIntervalEXT();
                }

                return currentInterval;
            }
        }

#endif
        return 0;
    }
};

#ifdef _WIN32
    #ifdef __MINGW32__
    #pragma GCC pop_options
    #else
    #pragma optimize( "", on )
    #endif
#endif

#endif /* GL_CONTEXT_MANAGER_H */

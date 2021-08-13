/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Kicad Developers, see AUTHORS.txt for contributors
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

#include <gal/opengl/kiglew.h>    // Must be included first

#include <limits>

class GL_UTILS
{
public:
    /**
     * Attempts to set the OpenGL swap interval.
     *
     * @param aVal if -1 = try to set adaptive swapping, 0 = sync off, 1 = sync with VSYNC rate.
     * @return actual value set
     */
    static int SetSwapInterval( int aVal )
    {
        /// This routine is written for Linux using X11 only.  The equivalent functions under
        /// Windows would include <wglext.h> and call wglSwapIntervalEXT
#if defined( __linux__ ) && !defined( KICAD_USE_EGL )
        Display *dpy = glXGetCurrentDisplay();
        GLXDrawable drawable = glXGetCurrentDrawable();

        if( glXSwapIntervalEXT && glXQueryDrawable && dpy && drawable )
        {
            if( aVal < 0 )
            {
                if( !GLX_EXT_swap_control_tear )
                {
                    aVal = 0;
                }
                else
                {
                    // Even though the extensions might be available,
                    // we need to be sure that late/adaptive swaps are
                    // enabled on the drawable.

                    unsigned lateSwapsEnabled;
                    glXQueryDrawable( dpy, drawable, GLX_LATE_SWAPS_TEAR_EXT, &lateSwapsEnabled );

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

        if( glXSwapIntervalMESA &&glXGetSwapIntervalMESA )
        {
            if( aVal < 0 )
                aVal = 0;

            glXSwapIntervalMESA( aVal );
            return glXGetSwapIntervalMESA();
        }

        if( glXSwapIntervalSGI )
        {
            if( aVal < 1 )
                aVal = 1;

            if( glXSwapIntervalSGI( aVal ) )
                glXSwapIntervalSGI( 1 );

            return 1;
        }

        return std::numeric_limits<int>::max();
#else
        return 0;
#endif
    }
};

#endif /* GL_CONTEXT_MANAGER_H */


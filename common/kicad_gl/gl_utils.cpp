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

#include <wx/platform.h>
#include <wx/version.h>

#if ( defined( __unix__ ) and not defined( __APPLE__ ) )
    #if wxHAS_EGL or wxHAS_GLX
        #if wxHAS_EGL
            #include <glad/egl.h>
        #endif
        #if wxHAS_GLX
            #include <glad/glx.h>
        #endif
    #else
        #if wxUSE_GLCANVAS_EGL
            #include <glad/egl.h>
        #else
            #include <glad/glx.h>
        #endif
    #endif
#elif defined( _WIN32 )
    #include <glad/wgl.h>
#endif

#include <kicad_gl/gl_utils.h>
#include <kicad_gl/kiglad.h>

#include <wx/glcanvas.h>
#include <wx/utils.h>
#include <string>


wxString GL_UTILS::DetectGLBackend( wxGLCanvas* aCanvas )
{
    wxString backend;

#ifdef __WXGTK__
    #if wxCHECK_VERSION( 3, 3, 2 )
        int eglMajor = 0, eglMinor = 0;

        if( aCanvas->GetEGLVersion( &eglMajor, &eglMinor ) )
            backend = wxString::Format( "EGL %d.%d", eglMajor, eglMinor );
        else if( int glxVersion = aCanvas->GetGLXVersion() )
            backend = wxString::Format( "GLX %d.%d", glxVersion / 10, glxVersion % 10 );

    #else // !wxCHECK_VERSION( 3, 3, 2 )
        #if wxUSE_GLCANVAS_EGL
            backend = "EGL";
        #else
            if( int glxVersion = aCanvas->GetGLXVersion() )
                backend = wxString::Format( "GLX %d.%d", glxVersion / 10, glxVersion % 10 );
        #endif

    #endif // !wxCHECK_VERSION( 3, 3, 2 )
#endif // __WXGTK__

    return backend;
}

#if !wxCHECK_VERSION( 3, 3, 3 )
int GL_UTILS::SetSwapInterval( wxGLCanvas* aCanvas, int aVal )
{
#if defined( GLAD_GLX )
    // Check that wx is really using GLX
    if( !wxGLCanvas::GetGLXVersion() )
        return 0;

    if( Display* dpy = wxGetX11Display() )
    {
        if( !gladLoaderLoadGLX( dpy, DefaultScreen( dpy ) ) )
            return 0;

        XID drawable = aCanvas->GetXWindow();

        if( glXSwapIntervalEXT && glXQueryDrawable && drawable )
        {
            if( aVal == -1 && !GLAD_GLX_EXT_swap_control_tear )
                aVal = 1; // Late swaps not available

            unsigned clampedInterval;
            glXSwapIntervalEXT( dpy, drawable, aVal );
            glXQueryDrawable( dpy, drawable, GLX_SWAP_INTERVAL_EXT, &clampedInterval );

            if( aVal == -1 )
            {
                unsigned lateSwapsEnabled = 0;
                glXQueryDrawable( dpy, drawable, GLX_LATE_SWAPS_TEAR_EXT, &lateSwapsEnabled );

                if( lateSwapsEnabled )
                    clampedInterval = -1;
            }

            return clampedInterval;
        }

        if( glXSwapIntervalMESA && glXGetSwapIntervalMESA )
        {
            if( aVal == -1 )
                aVal = 1;

            if( !glXSwapIntervalMESA( aVal ) )
                return aVal;
        }

        if( glXSwapIntervalSGI )
        {
            if( aVal == -1 )
                aVal = 1;

            if( !glXSwapIntervalSGI( aVal ) )
                return aVal;
        }
    }

#elif defined( GLAD_WGL ) && defined( GLAD_GL )

    if( !gladLoaderLoadWGL( aCanvas->GetHDC() ) )
        return 0;

    if( !gladLoaderLoadGL() )
        return 0;

    const GLubyte* vendor = glGetString( GL_VENDOR );
    const GLubyte* version = glGetString( GL_VERSION );

    if( wglSwapIntervalEXT )
    {
        wxString vendorStr, versionStr;

        if( vendor )
            vendorStr = wxString( reinterpret_cast<const char*>( vendor ) );

        if( version )
            versionStr = wxString( reinterpret_cast<const char*>( version ) );

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
#endif /* !wxCHECK_VERSION( 3, 3, 3 ) */

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Bernhard Stegmaier <stegmaier@sw-systems.de>
 * Copyright (C) 2016-2017 Kicad Developers, see change_log.txt for contributors.
 *
 * Base class for HiDPI aware wxGLCanvas implementations.
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

#include <gal/hidpi_gl_canvas.h>


HIDPI_GL_CANVAS::HIDPI_GL_CANVAS( wxWindow *parent,
           wxWindowID id,
           const int *attribList,
           const wxPoint& pos,
           const wxSize& size,
           long style,
           const wxString& name,
           const wxPalette& palette ) :
    wxGLCanvas( parent, id, attribList, pos, size, style, name, palette )
{
#ifdef RETINA_OPENGL_PATCH
    SetViewWantsBestResolution( true );
#endif
}


wxSize HIDPI_GL_CANVAS::GetClientSize() const
{
    wxSize size = wxGLCanvas::GetClientSize();

#ifdef RETINA_OPENGL_PATCH
    const float scaleFactor = GetBackingScaleFactor();
    size.x *= scaleFactor;
    size.y *= scaleFactor;
#endif

    return size;
}

float HIDPI_GL_CANVAS::GetBackingScaleFactor() const
{
#ifdef RETINA_OPENGL_PATCH
    // this is ugly, but original method isn't marked const although it doesn't modify anything
    // => clean up when it officially has arrived in wxWidgets
    return static_cast< wxGLCanvas* >( const_cast< HIDPI_GL_CANVAS* >( this ))->GetBackingScaleFactor();
#else
    return 1.0f;
#endif
}

/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2017 Bernhard Stegmaier <stegmaier@sw-systems.de>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dpi_scaling.h>


HIDPI_GL_CANVAS::HIDPI_GL_CANVAS( const KIGFX::VC_SETTINGS& aSettings, wxWindow* aParent,
                                  const wxGLAttributes& aGLAttribs, wxWindowID aId,
                                  const wxPoint& aPos, const wxSize& aSize, long aStyle,
                                  const wxString& aName, const wxPalette& aPalette ) :
        wxGLCanvas( aParent, aGLAttribs, aId, aPos, aSize, aStyle, aName, aPalette ),
        m_settings( aSettings )
{
}


wxSize HIDPI_GL_CANVAS::GetNativePixelSize() const
{
    wxSize size = wxGLCanvas::GetClientSize();

    const double scaleFactor = GetScaleFactor();
    size.x *= scaleFactor;
    size.y *= scaleFactor;

    return size;
}


wxPoint HIDPI_GL_CANVAS::GetNativePosition( const wxPoint& aPoint ) const
{
    wxPoint nativePoint = aPoint;

    const double scaleFactor = GetScaleFactor();
    nativePoint.x *= scaleFactor;
    nativePoint.y *= scaleFactor;

    return nativePoint;
}


double HIDPI_GL_CANVAS::GetScaleFactor() const
{
    return GetContentScaleFactor();
}
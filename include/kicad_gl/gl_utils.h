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

#include <wx/version.h>
#include <wx/string.h>

#include <kicommon.h>

class wxGLCanvas;

class KICOMMON_API GL_UTILS
{
public:
    static wxString DetectGLBackend( wxGLCanvas* aCanvas );

#if !wxCHECK_VERSION( 3, 3, 3 )
    /**
     * Attempt to set the OpenGL swap interval.
     *
     * @param aCanvas the canvas for which to set the swap interval
     * @param aVal if -1 = try to set adaptive swapping, 0 = sync off, 1 = sync with VSYNC rate.
     * @return actual value set
     */
    static int SetSwapInterval( wxGLCanvas* aCanvas, int aVal );
#endif /* !wxCHECK_VERSION( 3, 3, 3 ) */
};

#endif /* GL_UTILS_H */

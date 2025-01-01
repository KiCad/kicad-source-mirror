/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
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

#ifndef KICAD_WX_SPLASH_H
#define KICAD_WX_SPLASH_H

#include <wx/splash.h>

class WX_SPLASH : public wxSplashScreen
{
public:
    WX_SPLASH() : wxSplashScreen() {}

    WX_SPLASH( const wxBitmap& aBitmap, long aSplashStyle, int aMilliseconds, wxWindow* aParent,
               wxWindowID aId, const wxPoint& aPos = wxDefaultPosition,
               const wxSize& aSize = wxDefaultSize,
               long          aStyle = wxSIMPLE_BORDER | wxFRAME_NO_TASKBAR | wxSTAY_ON_TOP ) :
            wxSplashScreen( aBitmap, aSplashStyle, aMilliseconds, aParent, aId, aPos, aSize, aStyle )
    {
    }

    ///< Nullify the virtual in the parent which attempts to close the splash
    ///< on any input
    virtual int FilterEvent( wxEvent& event ) override { return -1; }
};

#endif
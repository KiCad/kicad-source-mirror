/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2017 Kicad Developers, see change_log.txt for contributors.
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

#ifndef GAL_DISPLAY_OPTIONS_H__
#define GAL_DISPLAY_OPTIONS_H__

#include <observable.h>

class wxConfigBase;
class wxString;

namespace KIGFX
{
    /**
     * GRID_STYLE: Type definition of the grid style
     */
    enum class GRID_STYLE
    {
        LINES,      ///< Use lines for the grid
        DOTS,       ///< Use dots for the grid
        SMALL_CROSS ///< Use small cross instead of dots for the grid
    };

    enum class OPENGL_ANTIALIASING_MODE
    {
        NONE,
        SUBSAMPLE_HIGH,
        SUBSAMPLE_ULTRA,
        SUPERSAMPLING_X2,
        SUPERSAMPLING_X4,
    };

    class GAL_DISPLAY_OPTIONS;

    class GAL_DISPLAY_OPTIONS_OBSERVER
    {
    public:
        virtual void OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& ) = 0;
    protected:
        // Observer lifetimes aren't handled by base class pointer
        virtual ~GAL_DISPLAY_OPTIONS_OBSERVER() {}
    };

    class GAL_DISPLAY_OPTIONS : public UTIL::OBSERVABLE<GAL_DISPLAY_OPTIONS_OBSERVER>
    {
    public:
        GAL_DISPLAY_OPTIONS();

        void ReadConfig ( wxConfigBase* aCfg, wxString aBaseName );
        void WriteConfig( wxConfigBase* aCfg, wxString aBaseName );

        void NotifyChanged();

        OPENGL_ANTIALIASING_MODE gl_antialiasing_mode;

        ///> The grid style to draw the grid in
        KIGFX::GRID_STYLE m_gridStyle;

        ///> Thickness to render grid lines/dots
        double m_gridLineWidth;

        ///> Minimum pixel distance between displayed grid lines
        double m_gridMinSpacing;

        ///> Whether or not to draw the coordinate system axes
        bool m_axesEnabled;

        ///> Fullscreen crosshair or small cross
        bool m_fullscreenCursor;

        ///> Force cursor display
        bool m_forceDisplayCursor;
    };

}

#endif


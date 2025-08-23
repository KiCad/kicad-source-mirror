/*
* This program source code file is part of KICAD, a free EDA CAD application.
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

#ifndef GAL_DISPLAY_OPTIONS_H__
#define GAL_DISPLAY_OPTIONS_H__

#include <gal/gal.h>
#include <dpi_scaling.h>
#include <core/observable.h>

class COMMON_SETTINGS;
struct WINDOW_SETTINGS;
class wxString;
class wxWindow;

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4275 )
#endif

namespace KIGFX
{
    /**
     * Type definition of the grid style.
     */
    enum class GRID_STYLE
    {
        DOTS,       ///< Use dots for the grid
        LINES,      ///< Use lines for the grid
        SMALL_CROSS ///< Use small cross instead of dots for the grid
    };

    enum class GAL_ANTIALIASING_MODE
    {
        AA_NONE,
        AA_FAST,
        AA_HIGHQUALITY,
    };

    enum class GRID_SNAPPING
    {
        ALWAYS,
        WITH_GRID,
        NEVER
    };

    enum class CROSS_HAIR_MODE : int
    {
        SMALL_CROSS,
        FULLSCREEN_CROSS,
        FULLSCREEN_DIAGONAL
    };

    class GAL_DISPLAY_OPTIONS;

    class GAL_API GAL_DISPLAY_OPTIONS_OBSERVER
    {
    public:
        virtual void OnGalDisplayOptionsChanged( const GAL_DISPLAY_OPTIONS& ) = 0;

    protected:
        // Observer lifetimes aren't handled by base class pointer
        virtual ~GAL_DISPLAY_OPTIONS_OBSERVER() {}
    };

    class GAL_API GAL_DISPLAY_OPTIONS : public UTIL::OBSERVABLE<GAL_DISPLAY_OPTIONS_OBSERVER>
    {
    public:
        GAL_DISPLAY_OPTIONS();

        virtual ~GAL_DISPLAY_OPTIONS()
        {}

        GAL_ANTIALIASING_MODE antialiasing_mode;

        ///< The grid style to draw the grid in
        KIGFX::GRID_STYLE m_gridStyle;

        ///< Snapping options for the grid
        GRID_SNAPPING m_gridSnapping;

        ///< Thickness to render grid lines/dots
        double m_gridLineWidth;

        ///< Minimum pixel distance between displayed grid lines
        double m_gridMinSpacing;

        ///< Whether or not to draw the coordinate system axes
        bool m_axesEnabled;

        ///< Crosshair drawing mode
        CROSS_HAIR_MODE m_crossHairMode;

        ///< Force cursor display
        bool m_forceDisplayCursor;

        ///< The pixel scale factor (>1 for hi-DPI scaled displays)
        double m_scaleFactor;

        void SetCursorMode( CROSS_HAIR_MODE aMode ) { m_crossHairMode = aMode; }

        CROSS_HAIR_MODE GetCursorMode() const { return m_crossHairMode; }

        void NotifyChanged();
    };

} // namespace KIGFX

#if defined( _MSC_VER )
#pragma warning( pop )
#endif

#endif


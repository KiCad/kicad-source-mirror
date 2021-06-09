/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gal/dpi_scaling.h>
#include <observable.h>

class COMMON_SETTINGS;
struct WINDOW_SETTINGS;
class wxString;
class wxWindow;


namespace KIGFX
{
    /**
     * GRID_STYLE: Type definition of the grid style
     */
    enum class GRID_STYLE
    {
        DOTS,       ///< Use dots for the grid
        LINES,      ///< Use lines for the grid
        SMALL_CROSS ///< Use small cross instead of dots for the grid
    };

    enum class OPENGL_ANTIALIASING_MODE
    {
        NONE,
        SMAA,
        SUPERSAMPLING,
    };

    enum class CAIRO_ANTIALIASING_MODE
    {
        NONE,
        FAST,
        GOOD,
    };

    enum class GRID_SNAPPING
    {
        ALWAYS,
        WITH_GRID,
        NEVER
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

        /**
         * Read GAL config options from application-level config
         * @param aCfg      the window settings to load from
         */
        void ReadWindowSettings( WINDOW_SETTINGS& aCfg );

        /**
         * Read GAL config options from the common config store
         * @param aCommonSettings the common config store
         * @param aWindow         the wx parent window (used for DPI scaling)
         */
        void ReadCommonConfig( COMMON_SETTINGS& aCommonSettings, wxWindow* aWindow );

        /**
         * Read application and common configs
         * @param aCommonConfig the common config store
         * @param aCfg          the application config base
         * @param aBaseName     the application's GAL options key prefix
         * @param aWindow       the wx parent window (used for DPI scaling)
         */
        void ReadConfig( COMMON_SETTINGS& aCommonConfig, WINDOW_SETTINGS& aWindowConfig,
                wxWindow* aWindow );

        void WriteConfig( WINDOW_SETTINGS& aCfg );

        void UpdateScaleFactor();

        void NotifyChanged();

        OPENGL_ANTIALIASING_MODE gl_antialiasing_mode;

        CAIRO_ANTIALIASING_MODE cairo_antialiasing_mode;

        DPI_SCALING m_dpi;

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

        ///< Fullscreen crosshair or small cross
        bool m_fullscreenCursor;

        ///< Force cursor display
        bool m_forceDisplayCursor;

        ///< The pixel scale factor (>1 for hi-DPI scaled displays)
        double m_scaleFactor;
    };

}

#endif


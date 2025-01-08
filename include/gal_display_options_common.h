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

#ifndef GAL_DISPLAY_OPTIONS_COMMON_H__
#define GAL_DISPLAY_OPTIONS_COMMON_H__

#include <gal/gal_display_options.h>
#include <dpi_scaling_common.h>

class COMMON_SETTINGS;
struct WINDOW_SETTINGS;
class wxString;
class wxWindow;


class GAL_DISPLAY_OPTIONS_IMPL : public KIGFX::GAL_DISPLAY_OPTIONS
{
public:
    GAL_DISPLAY_OPTIONS_IMPL();

    /**
     * Read GAL config options from application-level config.
     *
     * @param aCfg      the window settings to load from.
     */
    void ReadWindowSettings( WINDOW_SETTINGS& aCfg );

    /**
     * Read GAL config options from the common config store.
     *
     * @param aCommonSettings the common config store.
     * @param aWindow         the wx parent window (used for DPI scaling).
     */
    void ReadCommonConfig( COMMON_SETTINGS& aCommonSettings, wxWindow* aWindow );

    /**
     * Read application and common configs.
     *
     * @param aCommonConfig the common config store.
     * @param aCfg          the application config base.
     * @param aBaseName     the application's GAL options key prefix.
     * @param aWindow       the wx parent window (used for DPI scaling).
     */
    void ReadConfig( COMMON_SETTINGS& aCommonConfig, WINDOW_SETTINGS& aWindowConfig,
                     wxWindow* aWindow );

    void WriteConfig( WINDOW_SETTINGS& aCfg );

    void UpdateScaleFactor();

    DPI_SCALING_COMMON m_dpi;
};

#endif

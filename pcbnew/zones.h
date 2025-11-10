/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#pragma once

#include <wx/translation.h>

struct CONVERT_SETTINGS;

// Default values in mm for parameters in ZONE_SETTINGS
#define ZONE_THERMAL_RELIEF_GAP_MM 0.5          // ZONE_SETTINGS::m_ThermalReliefGap
#define ZONE_THERMAL_RELIEF_COPPER_WIDTH_MM 0.5 // ZONE_SETTINGS::m_ThermalReliefCopperBridge
#define ZONE_THICKNESS_MM 0.25                  // ZONE_SETTINGS::m_ZoneMinThickness
#define ZONE_THICKNESS_MIN_VALUE_MM 0.025       // Minimum for ZONE_SETTINGS::m_ZoneMinThickness
#define ZONE_CLEARANCE_MM 0.5                   // ZONE_SETTINGS::m_ZoneClearance
#define ZONE_CLEARANCE_MAX_VALUE_MM 100         // Maximum for ZONE_SETTINGS::m_ZoneClearance
#define ZONE_BORDER_HATCH_DIST_MM 0.5           // ZONE_SETTINGS::m_BorderHatchPitch
#define ZONE_BORDER_HATCH_MINDIST_MM 0.1        // Minimum for ZONE_SETTINGS::m_BorderHatchPitch
#define ZONE_BORDER_HATCH_MAXDIST_MM 2.0        // Maximum for ZONE_SETTINGS::m_BorderHatchPitch


#define ZONE_MANAGER_REPOUR 1005 //Reported if repour option is checked while clicking OK

/// How pads are covered by copper in zone
enum class ZONE_CONNECTION
{
    INHERITED = -1,
    NONE,       ///< Pads are not covered
    THERMAL,    ///< Use thermal relief for pads
    FULL,       ///< pads are covered by copper
    THT_THERMAL ///< Thermal relief only for THT pads
};


inline wxString PrintZoneConnection( ZONE_CONNECTION aConnection )
{
    switch( aConnection )
    {
    default:
    case ZONE_CONNECTION::INHERITED:   return _( "inherited" );
    case ZONE_CONNECTION::NONE:        return _( "none" );
    case ZONE_CONNECTION::THERMAL:     return _( "thermal reliefs" );
    case ZONE_CONNECTION::FULL:        return _( "solid" );
    case ZONE_CONNECTION::THT_THERMAL: return _( "thermal reliefs for PTH" );
    }
}


class ZONE;
class ZONE_SETTINGS;
class PCB_BASE_FRAME;
class BOARD;

/**
 * Function InvokeNonCopperZonesEditor
 * invokes up a modal dialog window for non-copper zone editing.
 *
 * @param aParent is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings,
                                CONVERT_SETTINGS* aConvertSettings = nullptr );

/**
 * Function InvokeCopperZonesEditor
 * invokes up a modal dialog window for copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aZone the zone being edited, or nullptr if a zone is being created.
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE* aZone, ZONE_SETTINGS* aSettings,
                             CONVERT_SETTINGS* aConvertSettings = nullptr );

/**
 * Function InvokeRuleAreaEditor
 * invokes up a modal dialog window for copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeRuleAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings,
                          BOARD* aBoard = nullptr, CONVERT_SETTINGS* aConvertSettings = nullptr );

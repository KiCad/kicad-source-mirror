/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2008-2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef ZONES_H_
#define ZONES_H_

// Default values in mils for parameters in ZONE
#define ZONE_THERMAL_RELIEF_GAP_MIL 20     // default value for ZONE_SETTINGS::m_ThermalReliefGap
#define ZONE_THERMAL_RELIEF_COPPER_WIDTH_MIL 20 // default value for ZONE_SETTINGS::m_ThermalReliefCopperBridge
#define ZONE_THICKNESS_MIL 10               // default value for ZONE_SETTINGS::m_ZoneMinThickness
#define ZONE_THICKNESS_MIN_VALUE_MIL 1      // minimum acceptable value for ZONE_SETTINGS::m_ZoneMinThickness
#define ZONE_CLEARANCE_MIL 20               // default value for ZONE_SETTINGS::m_ZoneClearance
#define ZONE_CLEARANCE_MAX_VALUE_MIL 500    // maximum acceptable value for ZONE_SETTINGS::m_ZoneClearance


#define ZONE_EXPORT_VALUES  1004        // Copper zone dialog reports wxID_OK, wxID_CANCEL or
                                        // ZONE_EXPORT_VALUES

/// How pads are covered by copper in zone
enum class ZONE_CONNECTION
{
    INHERITED = -1,
    NONE,       ///< Pads are not covered
    THERMAL,    ///< Use thermal relief for pads
    FULL,       ///< pads are covered by copper
    THT_THERMAL ///< Thermal relief only for THT pads
};

class ZONE;
class ZONE_SETTINGS;
class PCB_BASE_FRAME;

/**
 * Function InvokeNonCopperZonesEditor
 * invokes up a modal dialog window for non-copper zone editing.
 *
 * @param aParent is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );

/**
 * Function InvokeCopperZonesEditor
 * invokes up a modal dialog window for copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings );

/**
 * Function InvokeRuleAreaEditor
 * invokes up a modal dialog window for copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *                and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return int - tells if user aborted, changed only one zone, or all of them.
 */
int InvokeRuleAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings );

#endif  // ZONES_H_

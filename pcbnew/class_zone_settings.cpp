/**
 * @brief class ZONE_SETTINGS used to handle zones parameters
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <fctsys.h>

#include <common.h>
#include <pcbnew.h>
#include <zones.h>

#include <class_zone.h>

ZONE_SETTINGS::ZONE_SETTINGS()
{
    m_ZonePriority = 0;
    m_FillMode = 0;                                            // Mode for filling zone : 1 use segments, 0 use polygons
    // Clearance value
    m_ZoneClearance      = Mils2iu( ZONE_CLEARANCE_MIL );
    // Min thickness value in filled areas (this is the minimum width of copper to fill solid areas) :
    m_ZoneMinThickness   = Mils2iu( ZONE_THICKNESS_MIL );
    m_NetcodeSelection   = 0;                                  // Net code selection for the current zone
    m_CurrentZone_Layer  = 0;                                  // Layer used to create the current zone
    m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;           // Option to show the zone area (outlines only, short hatches or full hatches

    m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;  // Option to select number of segments to approximate a circle
                                                               // ARC_APPROX_SEGMENTS_COUNT_LOW_DEF
                                                               // or ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF segments

    // tickness of the gap in thermal reliefs:
    m_ThermalReliefGap = Mils2iu( ZONE_THERMAL_RELIEF_GAP_MIL );
    // tickness of the copper bridge in thermal reliefs:
    m_ThermalReliefCopperBridge = Mils2iu( ZONE_THERMAL_RELIEF_COPPER_WIDTH_MIL );

    m_PadConnection = THERMAL_PAD;                             // How pads are covered by copper in zone

    m_Zone_45_Only = false;

    cornerSmoothingType = SMOOTHING_NONE;
    cornerRadius = 0;
}


ZONE_SETTINGS& ZONE_SETTINGS::operator << ( const ZONE_CONTAINER& aSource )
{
    m_ZonePriority = aSource.GetPriority();
    m_FillMode     = aSource.m_FillMode;
    m_ZoneClearance      = aSource.m_ZoneClearance;
    m_ZoneMinThickness   = aSource.m_ZoneMinThickness;
    m_NetcodeSelection   = aSource.GetNet();
    m_CurrentZone_Layer  = aSource.GetLayer();
    m_Zone_HatchingStyle = aSource.GetHatchStyle();
    m_ArcToSegmentsCount = aSource.m_ArcToSegmentsCount;
    m_ThermalReliefGap = aSource.m_ThermalReliefGap;
    m_ThermalReliefCopperBridge = aSource.m_ThermalReliefCopperBridge;
    m_PadConnection = aSource.GetPadConnection();
    cornerSmoothingType = aSource.GetCornerSmoothingType();
    cornerRadius = aSource.GetCornerRadius();

    return *this;
}


void ZONE_SETTINGS::ExportSetting( ZONE_CONTAINER& aTarget, bool aFullExport ) const
{
    aTarget.m_FillMode = m_FillMode;
    aTarget.m_ZoneClearance    = m_ZoneClearance;
    aTarget.m_ZoneMinThickness = m_ZoneMinThickness;
    aTarget.m_Poly->SetHatch( m_Zone_HatchingStyle, Mils2iu( 20 ) );
    aTarget.m_ArcToSegmentsCount = m_ArcToSegmentsCount;
    aTarget.m_ThermalReliefGap = m_ThermalReliefGap;
    aTarget.m_ThermalReliefCopperBridge = m_ThermalReliefCopperBridge;
    aTarget.SetPadConnection( m_PadConnection );
    aTarget.SetCornerSmoothingType( cornerSmoothingType );
    aTarget.SetCornerRadius( cornerRadius );

    if( aFullExport )
    {
        aTarget.SetPriority( m_ZonePriority );
        aTarget.SetNet( m_NetcodeSelection );
        aTarget.SetLayer( m_CurrentZone_Layer );
        aTarget.m_Poly->SetLayer( m_CurrentZone_Layer );
    }
}

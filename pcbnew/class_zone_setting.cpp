/******************************************************/
/* class ZONE_SETTING used to handle zones parameters */
/******************************************************/

#ifndef WX_PRECOMP
#include "wx/wx.h"
#endif

/* For compilers that support precompilation:
 */
#include "wx/wxprec.h"

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#include "fctsys.h"
#include "PolyLine.h"

#include "common.h"
#include "pcbnew.h"
#include "zones.h"
#include "class_zone.h"

ZONE_SETTING::ZONE_SETTING( void )
{
    m_FillMode = 0;                                                 // Mode for filling zone : 1 use segments, 0 use polygons
    m_ZoneClearance      = 200;                                     // Clearance value
    m_ZoneMinThickness   = 100;                                     // Min thickness value in filled areas
    m_NetcodeSelection   = 0;                                       // Net code selection for the current zone
    m_CurrentZone_Layer  = 0;                                       // Layer used to create the current zone
    m_Zone_HatchingStyle = CPolyLine::DIAGONAL_EDGE;                // Option to show the zone area (outlines only, short hatches or full hatches
    m_ArcToSegmentsCount = ARC_APPROX_SEGMENTS_COUNT_LOW_DEF;      /* Option to select number of segments to approximate a circle
                                                                    * ARC_APPROX_SEGMENTS_COUNT_LOW_DEF
                                                                    * or ARC_APPROX_SEGMENTS_COUNT_HIGHT_DEF segments */
    m_ThermalReliefGapValue = 200;                                  // tickness of the gap in thermal reliefs
    m_ThermalReliefCopperBridgeValue = 200;                         // tickness of the copper bridge in thermal reliefs

    m_Zone_Pad_Options = THERMAL_PAD;                               // How pads are covered by copper in zone
}


/**
 * Function ImportSetting
 * copy settings from a given zone
 * @param aSource: the given zone
 */
void ZONE_SETTING::ImportSetting( const ZONE_CONTAINER& aSource )
{
    m_FillMode = aSource.m_FillMode;
    m_ZoneClearance      = aSource.m_ZoneClearance;
    m_ZoneMinThickness   = aSource.m_ZoneMinThickness;
    m_NetcodeSelection   = aSource.GetNet();
    m_CurrentZone_Layer  = aSource.GetLayer();
    m_Zone_HatchingStyle = aSource.GetHatchStyle();
    m_ArcToSegmentsCount = aSource.m_ArcToSegmentsCount;
    m_ThermalReliefGapValue = aSource.m_ThermalReliefGapValue;
    m_ThermalReliefCopperBridgeValue = aSource.m_ThermalReliefCopperBridgeValue;
    m_Zone_Pad_Options = aSource.m_PadOption;
}


/**
 * Function ExportSetting
 * copy settings to a given zone
 * @param aTarget: the given zone
 * @param aFullExport: if false: some parameters are NOT exported
 *   because they must not be  exported when export settings from a zone to others zones
 *   Currently:
 *      m_NetcodeSelection
 */
void ZONE_SETTING::ExportSetting( ZONE_CONTAINER& aTarget, bool aFullExport )
{
    aTarget.m_FillMode = m_FillMode;
    aTarget.m_ZoneClearance    = m_ZoneClearance;
    aTarget.m_ZoneMinThickness = m_ZoneMinThickness;
    aTarget.m_Poly->SetHatch( m_Zone_HatchingStyle );
    aTarget.m_ArcToSegmentsCount = m_ArcToSegmentsCount;
    aTarget.m_ThermalReliefGapValue = m_ThermalReliefGapValue;
    aTarget.m_ThermalReliefCopperBridgeValue = m_ThermalReliefCopperBridgeValue;
    aTarget.m_PadOption = m_Zone_Pad_Options;
    if( aFullExport )
    {
        aTarget.SetNet( m_NetcodeSelection );
        aTarget.SetLayer( m_CurrentZone_Layer );
    }
}

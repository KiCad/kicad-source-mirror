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
    m_GridFillValue         = 250;                                  // Grid value for filling zone by segments, 0 to used polygons to fill
    m_ZoneClearance = 200;                            // Clearance value
    m_NetcodeSelection      = 0;                                    // Net code selection for the current zone
    m_CurrentZone_Layer     = 0;                                    // Layer used to create the current zone
    m_Zone_HatchingStyle    = CPolyLine::DIAGONAL_EDGE;             // Option to show the zone area (outlines only, short hatches or full hatches
    m_ArcToSegmentsCount    = 16;               /* Option to select number of segments to approximate a circle
                                                 * 16 or 32 segments */
    m_FilledAreasShowMode   = 0;                                    // Used to select draw options for filled areas in a zone (currently normal =0, sketch = 1)
    m_ThermalReliefGapValue = 200;                                  // tickness of the gap in thermal reliefs
    m_ThermalReliefCopperBridgeValue = 200;                         // tickness of the copper bridge in thermal reliefs

    m_Zone_Pad_Options = THERMAL_PAD;                               // How pads are covered by copper in zone
}


/** function ImportSetting
 * copy settings from a given zone
 * @param aSource: the given zone
 */
void ZONE_SETTING::ImportSetting( const ZONE_CONTAINER& aSource )
{
    m_GridFillValue         = aSource.m_GridFillValue;
    m_ZoneClearance         = aSource.m_ZoneClearance;
    m_NetcodeSelection      = aSource.GetNet();
    m_CurrentZone_Layer     = aSource.GetLayer();
    m_Zone_HatchingStyle    = aSource.GetHatchStyle();
    m_ArcToSegmentsCount    = aSource.m_ArcToSegmentsCount;
    m_FilledAreasShowMode   = aSource.m_DrawOptions;
    m_ThermalReliefGapValue = aSource.m_ThermalReliefGapValue;
    m_ThermalReliefCopperBridgeValue = aSource.m_ThermalReliefCopperBridgeValue;
    m_Zone_Pad_Options = aSource.m_PadOption;
}


/** function ExportSetting
 * copy settings to a given zone
 * @param aTarget: the given zone
 * Note: parameters NOT exported (because they cannot be safely exported):
 * m_NetcodeSelection
 */
void ZONE_SETTING::ExportSetting( ZONE_CONTAINER& aTarget )
{
    aTarget.m_GridFillValue         = m_GridFillValue;
    aTarget.m_ZoneClearance         = m_ZoneClearance;
    aTarget.SetNet(m_NetcodeSelection);
    aTarget.SetLayer(m_CurrentZone_Layer);
    aTarget.m_Poly->SetHatch(m_Zone_HatchingStyle);
    aTarget.m_ArcToSegmentsCount    = m_ArcToSegmentsCount;
    aTarget.m_DrawOptions   = m_FilledAreasShowMode;
    aTarget.m_ThermalReliefGapValue = m_ThermalReliefGapValue;
    aTarget.m_ThermalReliefCopperBridgeValue = m_ThermalReliefCopperBridgeValue;
    aTarget.m_PadOption = m_Zone_Pad_Options;
}

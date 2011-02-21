/****************************************************************/
/* class ZONE_SETTING used to handle zones parameters in dialogs */
/****************************************************************/

#ifndef ZONE_SETTING_H
#define ZONE_SETTING_H

#define MAX_ZONE_CORNER_RADIUS 4000

/*************************************************/
/* Class ZONE_SETTING to handle zones parameters */
/*************************************************/
class ZONE_SETTING
{
public:
    enum {
        SMOOTHING_NONE,
        SMOOTHING_CHAMFER,
        SMOOTHING_FILLET,
        SMOOTHING_LAST
    };
    int  m_FillMode;                                    // Mode for filling zone : 1 use segments, 0 use polygons
    int  m_ZoneClearance;                               // Clearance value
    int  m_ZoneMinThickness;                            // Min thickness value in filled areas
    int  m_NetcodeSelection;                            // Net code selection for the current zone
    int  m_CurrentZone_Layer;                           // Layer used to create the current zone
    int  m_Zone_HatchingStyle;                          // Option to show the zone area (outlines only, short hatches or full hatches
    int  m_ArcToSegmentsCount;                  /* Option to select number of segments to approximate a circle
                                                 * 16 or 32 segments */
    long m_ThermalReliefGapValue;                       // tickness of the gap in thermal reliefs
    long m_ThermalReliefCopperBridgeValue;              // tickness of the copper bridge in thermal reliefs
    int  m_Zone_Pad_Options;                            // How pads are covered by copper in zone
private:
    int  cornerSmoothingType;                           // Corner smoothing type
    unsigned int  cornerRadius;                         // Corner chamfer distance / fillet radius
public:
    ZONE_SETTING( void );

    /**
     * Function ImportSetting
     * copy settings from a given zone
     * @param aSource: the given zone
     */
    void ImportSetting( const ZONE_CONTAINER& aSource );

    /**
     * Function ExportSetting
     * copy settings to a given zone
     * @param aTarget: the given zone
     * @param aFullExport: if false: some parameters are NOT exported
     *   because they must not be  exported when export settings from a zone to others zones
     *   Currently:
     *      m_NetcodeSelection
     */
    void ExportSetting( ZONE_CONTAINER& aTarget, bool aFullExport = true);

    void SetCornerSmoothingType( int aType) { cornerSmoothingType = aType; };
    int  GetCornerSmoothingType() const { return cornerSmoothingType; };
    void SetCornerRadius( int aRadius )
    {
        if( aRadius > MAX_ZONE_CORNER_RADIUS )
            cornerRadius = MAX_ZONE_CORNER_RADIUS;
        else if( aRadius < 0 )
            cornerRadius = 0;
        else
            cornerRadius = aRadius;
    };
    unsigned int  GetCornerRadius() const { return cornerRadius; };
};


#endif  //   ifndef ZONE_SETTING_H

/**
 * @file class_zone_settings.h
 * @brief Class ZONE_SETTINGS used to handle zones parameters in dialogs.
 */

#ifndef ZONE_SETTINGS_H_
#define ZONE_SETTINGS_H_

#include "zones.h"


class ZONE_CONTAINER;


#define MAX_ZONE_CORNER_RADIUS_MILS 400


/**
 * Class ZONE_SETTINGS
 * handles zones parameters.
 * Because a zone can be on copper or non copper layers, and can be also
 * a keepout area, some parameters are irrelevant depending on the type of zone
 */
class ZONE_SETTINGS
{
public:
    enum {
        SMOOTHING_NONE,
        SMOOTHING_CHAMFER,
        SMOOTHING_FILLET,
        SMOOTHING_LAST
    };

    /// Mode for filling zone : 1 use segments, 0 use polygons
    int  m_FillMode;

    int  m_ZonePriority;                ///< Priority (0 ... N) of the zone

    int  m_ZoneClearance;               ///< Clearance value
    int  m_ZoneMinThickness;            ///< Min thickness value in filled areas
    int  m_NetcodeSelection;            ///< Net code selection for the current zone

    LAYER_ID    m_CurrentZone_Layer;    ///< Layer used to create the current zone

    /// Option to show the zone area (outlines only, short hatches or full hatches
    int  m_Zone_HatchingStyle;

    /// Option to select number of segments to approximate a circle 16 or 32 segments.
    int  m_ArcToSegmentsCount;

    long m_ThermalReliefGap;            ///< thickness of the gap in thermal reliefs
    long m_ThermalReliefCopperBridge;   ///< thickness of the copper bridge in thermal reliefs

    bool m_Zone_45_Only;

private:
    int  m_cornerSmoothingType;           ///< Corner smoothing type
    unsigned int  m_cornerRadius;         ///< Corner chamfer distance / fillet radius
    ZoneConnection m_PadConnection;

    /* A zone outline can be a keepout zone.
     * It will be never filled, and DRC should test for pads, tracks and vias
     */
    bool                  m_isKeepout;

    /* For keepout zones only:
     * what is not allowed inside the keepout ( pads, tracks and vias )
     */
    bool m_keepoutDoNotAllowCopperPour;
    bool m_keepoutDoNotAllowVias;
    bool m_keepoutDoNotAllowTracks;


public:
    ZONE_SETTINGS();

    /**
     * operator << ( const ZONE_CONTAINER& )
     * was Function ImportSetting
     * copies settings from a given zone into this object.
     * @param aSource: the given zone
     */
    ZONE_SETTINGS& operator << ( const ZONE_CONTAINER& aSource );

    /**
     * Function ExportSetting
     * copy settings to a given zone
     * @param aTarget: the given zone
     * @param aFullExport: if false: some parameters are NOT exported
     *   because they must not be  exported when export settings from a zone to others zones
     *   Currently:
     *      m_NetcodeSelection
     */
    void ExportSetting( ZONE_CONTAINER& aTarget, bool aFullExport = true ) const;

    void SetCornerSmoothingType( int aType) { m_cornerSmoothingType = aType; }

    int GetCornerSmoothingType() const { return m_cornerSmoothingType; }

    void SetCornerRadius( int aRadius )
    {
        if( aRadius > Mils2iu( MAX_ZONE_CORNER_RADIUS_MILS ) )
            m_cornerRadius = Mils2iu( MAX_ZONE_CORNER_RADIUS_MILS );
        else if( aRadius < 0 )
            m_cornerRadius = 0;
        else
            m_cornerRadius = aRadius;
    };

    unsigned int GetCornerRadius() const { return m_cornerRadius; }

    ZoneConnection GetPadConnection() const { return m_PadConnection; }
    void SetPadConnection( ZoneConnection aPadConnection ) { m_PadConnection = aPadConnection; }

    /**
     * Accessors to parameters used in Keepout zones:
     */
    const bool GetIsKeepout() const { return m_isKeepout; }
    const bool GetDoNotAllowCopperPour() const { return m_keepoutDoNotAllowCopperPour; }
    const bool GetDoNotAllowVias() const { return m_keepoutDoNotAllowVias; }
    const bool GetDoNotAllowTracks() const { return m_keepoutDoNotAllowTracks; }

    void SetIsKeepout( bool aEnable ) { m_isKeepout = aEnable; }
    void SetDoNotAllowCopperPour( bool aEnable ) { m_keepoutDoNotAllowCopperPour = aEnable; }
    void SetDoNotAllowVias( bool aEnable ) { m_keepoutDoNotAllowVias = aEnable; }
    void SetDoNotAllowTracks( bool aEnable ) { m_keepoutDoNotAllowTracks = aEnable; }
};


#endif  // ZONE_SETTINGS_H_

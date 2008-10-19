/************************************************/
/* constants used in zone dialogs and functions */
/************************************************/

#ifndef ZONES_H
#define ZONES_H


#ifndef eda_global
#define eda_global extern
#endif


// keys used to store net sort option in config file :
#define ZONE_NET_OUTLINES_HATCH_OPTION_KEY wxT( "Zone_Ouline_Hatch_Opt" )
#define ZONE_NET_SORT_OPTION_KEY           wxT( "Zone_NetSort_Opt" )
#define ZONE_NET_FILTER_STRING_KEY         wxT( "Zone_Filter_Opt" )
#define ZONE_THERMAL_RELIEF_GAP_STRING_KEY wxT( "Zone_TH_Gap" )
#define ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY wxT( "Zone_TH_Copper_Width" )

enum zone_cmd {
    ZONE_ABORT,
    ZONE_OK
};


/************************************************/
/* variables used in zone dialogs and functions */
/************************************************/

// @todo: make a class like ZONE_GENERAL_SETTING instead of many global variables
eda_global bool g_Zone_45_Only
#ifdef MAIN
= FALSE
#endif
;
eda_global int g_NetcodeSelection;                  // Net code selection for the current zone
eda_global int g_CurrentZone_Layer;                 // Layer used to create the current zone
eda_global int g_Zone_Hatching;                     // Option to show the zone area (outlines only, short hatches or full hatches
eda_global int g_Zone_Arc_Approximation;            // Option to select number of segments to approximate a circle
                                                    // 16 or 32 segments
eda_global int g_FilledAreasShowMode;               // Used to select draw options for filled areas in a zone (currently normal =0, sketch = 1)
eda_global long g_ThermalReliefGapValue             // tickness of the gap in thermal reliefs
#ifdef MAIN
= 200
#endif
;
eda_global long g_ThermalReliefCopperBridgeValue     // tickness of the copper bridge in thermal reliefs
#ifdef MAIN
= 200
#endif
;

eda_global ZONE_CONTAINER::m_PadInZone g_Zone_Pad_Options
#ifdef MAIN
= ZONE_CONTAINER::THERMAL_PAD
#endif
;

#endif  //   ifndef ZONES_H

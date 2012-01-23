/************************************************/
/* constants used in zone dialogs and functions */
/************************************************/

#ifndef ZONES_H
#define ZONES_H


#include <class_zone_setting.h>


// keys used to store net sort option in config file :
#define ZONE_NET_OUTLINES_HATCH_OPTION_KEY          wxT( "Zone_Ouline_Hatch_Opt" )
#define ZONE_NET_SORT_OPTION_KEY                    wxT( "Zone_NetSort_Opt" )
#define ZONE_NET_FILTER_STRING_KEY                  wxT( "Zone_Filter_Opt" )
#define ZONE_THERMAL_RELIEF_GAP_STRING_KEY          wxT( "Zone_TH_Gap" )
#define ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY wxT( "Zone_TH_Copper_Width" )

// Exit codes for dialog edit zones
enum zone_cmd {
    ZONE_ABORT,         //  if no change
    ZONE_OK,            //  if new values accepted
    ZONE_EXPORT_VALUES  //  if values are exported to others zones
};


enum {                          // How pads are covered by copper in zone
    PAD_NOT_IN_ZONE,            // Pads are not covered
    THERMAL_PAD,                // Use thermal relief for pads
    PAD_IN_ZONE                 // pads are covered by copper
};


/************************************************/
/* variables used in zone dialogs and functions */
/************************************************/

extern bool g_Zone_45_Only;

// Default setting used when creating a new zone
extern ZONE_SETTING g_Zone_Default_Setting;

#endif  //   ifndef ZONES_H

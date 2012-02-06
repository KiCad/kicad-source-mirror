/************************************************/
/* constants used in zone dialogs and functions */
/************************************************/

#ifndef ZONES_H_
#define ZONES_H_

// keys used to store net sort option in config file :
#define ZONE_NET_OUTLINES_HATCH_OPTION_KEY          wxT( "Zone_Ouline_Hatch_Opt" )
#define ZONE_NET_SORT_OPTION_KEY                    wxT( "Zone_NetSort_Opt" )
#define ZONE_NET_FILTER_STRING_KEY                  wxT( "Zone_Filter_Opt" )
#define ZONE_THERMAL_RELIEF_GAP_STRING_KEY          wxT( "Zone_TH_Gap" )
#define ZONE_THERMAL_RELIEF_COPPER_WIDTH_STRING_KEY wxT( "Zone_TH_Copper_Width" )

/// Exit codes for zone editing dialogs
enum ZONE_EDIT_T {
    ZONE_ABORT,             ///<  if no change
    ZONE_OK,                ///<  if new values were accepted
    ZONE_EXPORT_VALUES      ///<  if values were exported to others zones
};


/// How pads are covered by copper in zone
enum {
    PAD_NOT_IN_ZONE,        ///< Pads are not covered
    THERMAL_PAD,            ///< Use thermal relief for pads
    PAD_IN_ZONE             ///< pads are covered by copper
};

class ZONE_CONTAINER;
class ZONE_SETTINGS;
class PCB_BASE_FRAME;

/**
 * Function InvokeNonCopperZonesEditor
 * invokes up a modal dialog window for non-copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *        and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aZone is the ZONE_CONTAINER to edit.
 * @param aSettings points to the ZONE_SETTINGS to edit.
 * @return ZONE_EDIT_T - tells if user aborted, changed only one zone, or all of them.
 */
ZONE_EDIT_T InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE_CONTAINER* aZone, ZONE_SETTINGS* aSettings );

/**
 * Function InvokeCopperZonesEditor
 * invokes up a modal dialog window for copper zone editing.
 *
 * @param aCaller is the PCB_BASE_FRAME calling parent window for the modal dialog,
 *        and it gives access to the BOARD through PCB_BASE_FRAME::GetBoard().
 * @param aZone is the ZONE_CONTAINER to edit.
 * @return ZONE_EDIT_T - tells if user aborted, changed only one zone, or all of them.
 */
ZONE_EDIT_T InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings );

#endif  // ZONES_H_

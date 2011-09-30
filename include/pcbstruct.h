/**
 * @file pcbstruct.h
 * @brief Classes and definitions used in Pcbnew.
 */

#ifndef PCBSTRUCT_H
#define PCBSTRUCT_H


// Definitions relatives aux libraries
#define FOOTPRINT_LIBRARY_HEADER       "PCBNEW-LibModule-V1"
#define FOOTPRINT_LIBRARY_HEADER_CNT   18


// Values for m_DisplayViaMode member:
enum VIA_DISPLAY_MODE_T {
    VIA_HOLE_NOT_SHOW = 0,
    VIA_SPECIAL_HOLE_SHOW,
    ALL_VIA_HOLE_SHOW,
    OPT_VIA_HOLE_END
};


/* Values for DISPLAY_OPTIONS.ShowTrackClearanceMode parameter option
 * This parameter controls how to show tracks and vias clearance area
 */
enum TRACE_CLEARANCE_DISPLAY_MODE_T {
    DO_NOT_SHOW_CLEARANCE = 0,                // Do not show clearance areas
    SHOW_CLEARANCE_NEW_TRACKS,                /* Show clearance areas only for new track
                                               * during track creation. */
    SHOW_CLEARANCE_NEW_TRACKS_AND_VIA_AREAS,  /* Show clearance areas only for new track
                                               * during track creation, and shows a via
                                               * clearance area at end of current new
                                               * segment (guide to place a new via
                                               */
    SHOW_CLEARANCE_NEW_AND_EDITED_TRACKS_AND_VIA_AREAS,
                                                /* Show clearance for new, moving and
                                                 * dragging tracks and vias
                                                 */
    SHOW_CLEARANCE_ALWAYS                      /* Show Always clearance areas
                                                * for track and vias
                                                */
};


class DISPLAY_OPTIONS
{
public:
    bool DisplayPadFill;
    bool DisplayViaFill;
    bool DisplayPadNum;
    bool DisplayPadIsol;

    int  DisplayModEdge;
    int  DisplayModText;
    bool DisplayPcbTrackFill;     /* FALSE = sketch , TRUE = filled */

    /// How trace clearances are displayed.  @see TRACE_CLEARANCE_DISPLAY_MODE_T.
    TRACE_CLEARANCE_DISPLAY_MODE_T  ShowTrackClearanceMode;

    VIA_DISPLAY_MODE_T m_DisplayViaMode;  /* 0 do not show via hole,
                                           * 1 show via hole for non default value
                                           * 2 show all via hole */

    bool DisplayPolarCood;
    int  DisplayZonesMode;
    int  DisplayNetNamesMode;   /* 0 do not show netnames,
                                 * 1 show netnames on pads
                                 * 2 show netnames on tracks
                                 * 3 show netnames on tracks and pads
                                 */

    int  DisplayDrawItems;
    bool ContrastModeDisplay;

public:
    DISPLAY_OPTIONS();
};


#endif // PCBSTRUCT_H

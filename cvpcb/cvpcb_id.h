/**
 * @file cvpcb_id.h
 */
/*
 * Command IDs for CvPcb.
 *
 * Please add IDs that are unique to the component library viewer here and
 * not in the global id.h file.  This will prevent the entire project from
 * being rebuilt when adding new commands to the component library viewer.
 */

// Generic IDs:
#include <id.h>

// specific IDs
enum id_cvpcb_frm
{
    ID_CVPCB_QUIT = ID_END_LIST,
    ID_CVPCB_READ_INPUT_NETLIST,
    ID_CVPCB_SAVEQUITCVPCB,
    ID_CVPCB_CREATE_SCREENCMP,
    ID_CVPCB_GOTO_FIRSTNA,
    ID_CVPCB_GOTO_PREVIOUSNA,
    ID_CVPCB_DEL_ASSOCIATIONS,
    ID_CVPCB_AUTO_ASSOCIE,
    ID_CVPCB_COMPONENT_LIST,
    ID_CVPCB_FOOTPRINT_LIST,
    ID_CVPCB_SHOW3D_FRAME,
    ID_CVPCB_FOOTPRINT_DISPLAY_FULL_LIST,
    ID_CVPCB_FOOTPRINT_DISPLAY_FILTERED_LIST,
    ID_CVPCB_FOOTPRINT_DISPLAY_PIN_FILTERED_LIST,
    ID_CVPCB_FOOTPRINT_DISPLAY_BY_LIBRARY_LIST,
    ID_CVPCB_CONFIG_KEEP_OPEN_ON_SAVE,
    ID_CVPCB_LIBRARY_LIST,
    ID_CVPCB_LIB_TABLE_EDIT
};

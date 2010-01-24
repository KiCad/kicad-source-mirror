#ifndef __GERBVIEW_ID_H__
#define __GERBVIEW_ID_H__

#include "id.h"

/**
 * Command IDs for the printed circuit board editor.
 *
 * Please add IDs that are unique to the printed circuit board editor (PCBNew)
 * here and not in the global id.h file.  This will prevent the entire project
 * from being rebuilt when adding new commands to the PCBNew.
 */

enum gerbview_ids
{
    ID_MAIN_MENUBAR = ID_END_LIST,

    ID_TOOLBARH_PCB_SELECT_LAYER,
    ID_PCB_DELETE_ITEM_BUTT,
    ID_PCB_GLOBAL_DELETE,
    ID_POPUP_PCB_DELETE_TRACKSEG,
    ID_PCB_DISPLAY_OPTIONS_SETUP
};

#endif  /* __GERBVIEW_IDS_H__  */

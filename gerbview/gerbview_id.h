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

    ID_MENU_GERBVIEW_SHOW_HIDE_LAYERS_MANAGER_DIALOG,

    ID_TOOLBARH_GERBVIEW_SELECT_LAYER,
    ID_GERBVIEW_DELETE_ITEM_BUTT,
    ID_GERBVIEW_GLOBAL_DELETE,
    ID_GERBVIEW_OPTIONS_SETUP,
    ID_TB_OPTIONS_SHOW_LAYERS_MANAGER_VERTICAL_TOOLBAR,
    ID_TB_OPTIONS_SHOW_DCODES,
    ID_TB_OPTIONS_SHOW_FLASHED_ITEMS_SKETCH,
    ID_TB_OPTIONS_SHOW_LINES_SKETCH,
    ID_TB_OPTIONS_SHOW_GBR_MODE_0,
    ID_TB_OPTIONS_SHOW_GBR_MODE_1,
    ID_TB_OPTIONS_SHOW_GBR_MODE_2,

    ID_GERBER_END_LIST
};

#endif  /* __GERBVIEW_IDS_H__  */

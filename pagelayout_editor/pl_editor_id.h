#ifndef _PL_EDITOR_ID_H_
#define _PL_EDITOR_ID_H_

#include <id.h>

/**
 * Command IDs for the printed circuit board editor.
 *
 * Please add IDs that are unique to the page layout editor (pl_editor) here and not in
 * the global id.h file.  This will prevent the entire project from being rebuilt when
 * adding new commands to the page layout editor.
 */

enum pl_editor_ids
{
    ID_MAIN_MENUBAR = ID_END_LIST,

    ID_PL_EDITOR_SHOW_SOURCE,
    ID_MENU_PL_EDITOR_SELECT_PREFERED_EDITOR,
    ID_DESIGN_TREE_FRAME,

    ID_SHOW_REAL_MODE,
    ID_SHOW_LPEDITOR_MODE,
    ID_SELECT_COORDINATE_ORIGIN,
    ID_LOAD_DEFAULT_PAGE_LAYOUT,
    ID_SELECT_PAGE_NUMBER,

    ID_OPEN_POLYGON_DESCR_FILE,

    ID_POPUP_START_RANGE,
    ID_POPUP_ITEM_DELETE,
    ID_POPUP_DESIGN_TREE_ITEM_DELETE,
    ID_POPUP_ITEM_ADD_LINE,
    ID_POPUP_ITEM_ADD_RECT,
    ID_POPUP_ITEM_ADD_TEXT,
    ID_POPUP_ITEM_ADD_POLY,
    ID_POPUP_ITEM_MOVE,
    ID_POPUP_ITEM_PLACE,
    ID_POPUP_ITEM_MOVE_START_POINT,
    ID_POPUP_ITEM_MOVE_END_POINT,
    ID_POPUP_ITEM_PLACE_CANCEL,
    ID_POPUP_END_RANGE,

    ID_PLEDITOR_END_LIST
};

#endif  /* _PL_EDITOR_IDS_H_ */

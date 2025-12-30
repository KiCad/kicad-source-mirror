/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @brief Common command IDs shared by more than one of the KiCad applications.
 *
 * Only place command IDs used in base window class event tables or shared
 * across multiple applications such as the zoom, grid, and language IDs.
 *
 * Most things should use the new ACTION framwork instead.
 */


#ifndef ID_H_
#define ID_H_

#include <wx/defs.h>

// Define room for IDs, for each sub application
#define ROOM_FOR_KICADMANAGER 50
#define ROOM_FOR_3D_VIEWER 100
#define ROOM_FOR_PANEL_PREV_MODEL 50


/// IDs range for menuitems file history:
/// The default range file history size is 9 (compatible with default wxWidget range).
#define DEFAULT_FILE_HISTORY_SIZE 9
#define MAX_FILE_HISTORY_SIZE 99

enum main_id
{
    ID_AUTO_SAVE_TIMER = wxID_HIGHEST,

    // ID for menuitems used in our file history management,
    // when we do not use wxFILE_ID (restricted to 9 items)
    ID_FILE,
    ID_FILE1,
    ID_FILEMAX = ID_FILE + MAX_FILE_HISTORY_SIZE,
    ID_FILE_LIST_EMPTY,
    ID_FILE_LIST_CLEAR,

    ID_PREFERENCES_RESET_PANEL,

    ID_LANGUAGE_CHOICE,
    ID_LANGUAGE_DANISH,
    ID_LANGUAGE_DEFAULT,
    ID_LANGUAGE_ENGLISH,
    ID_LANGUAGE_FRENCH,
    ID_LANGUAGE_FINNISH,
    ID_LANGUAGE_HEBREW,
    ID_LANGUAGE_SPANISH,
    ID_LANGUAGE_SPANISH_MEXICAN,
    ID_LANGUAGE_GERMAN,
    ID_LANGUAGE_GREEK,
    ID_LANGUAGE_NORWEGIAN,
    ID_LANGUAGE_RUSSIAN,
    ID_LANGUAGE_PORTUGUESE,
    ID_LANGUAGE_PORTUGUESE_BRAZILIAN,
    ID_LANGUAGE_TURKISH,
    ID_LANGUAGE_INDONESIAN,
    ID_LANGUAGE_ITALIAN,
    ID_LANGUAGE_SLOVENIAN,
    ID_LANGUAGE_SLOVAK,
    ID_LANGUAGE_HUNGARIAN,
    ID_LANGUAGE_POLISH,
    ID_LANGUAGE_CZECH,
    ID_LANGUAGE_KOREAN,
    ID_LANGUAGE_CATALAN,
    ID_LANGUAGE_CHINESE_SIMPLIFIED,
    ID_LANGUAGE_CHINESE_TRADITIONAL,
    ID_LANGUAGE_DUTCH,
    ID_LANGUAGE_JAPANESE,
    ID_LANGUAGE_BULGARIAN,
    ID_LANGUAGE_LATVIAN,
    ID_LANGUAGE_LITHUANIAN,
    ID_LANGUAGE_VIETNAMESE,
    ID_LANGUAGE_SERBIAN,
    ID_LANGUAGE_THAI,
    ID_LANGUAGE_SWEDISH,
    ID_LANGUAGE_UKRAINIAN,
    ID_LANGUAGE_ARABIC,
    ID_LANGUAGE_ESTONIAN,
    ID_LANGUAGE_FARSI,
    ID_LANGUAGE_CROATIAN,
    ID_LANGUAGE_ROMANIAN,
    ID_LANGUAGE_NORWEGIAN_BOKMAL,
    ID_LANGUAGE_TAMIL,
    ID_LANGUAGE_CHOICE_END,

    ID_ON_ZOOM_SELECT,
    ID_ON_GRID_SELECT,
    ID_ON_OVERRIDE_LOCKS,
    ID_ON_LAYER_SELECT,

    // Popup Menu (mouse Right button) (id consecutifs)

    /* Reserve IDs for popup menu zoom levels.  If you need more
     * levels of zoom, change ID_POPUP_ZOOM_LEVEL_END.  Note that more
     * than 15 entries in a context submenu may get too large to display
     * cleanly.  Add any additional popup zoom IDs above here or the
     * zoom event handler will not work properly.
     */
    ID_POPUP_ZOOM_LEVEL_START,
    ID_POPUP_ZOOM_LEVEL_END = ID_POPUP_ZOOM_LEVEL_START + 99,

    ID_POPUP_GRID_START,
    ID_POPUP_GRID_END = ID_POPUP_ZOOM_LEVEL_START + 99,

    // KiFace server for standalone operation
    ID_EDA_SOCKET_EVENT_SERV,
    ID_EDA_SOCKET_EVENT,

    // IDs specifics to a sub-application (Eeschema, Kicad manager....) start here
    //
    // We reserve here Ids for each sub-application, to avoid duplicate IDs
    // between them.
    // mainly we experienced issues related to wxUpdateUIEvent calls when 2 (or more) wxFrames
    // share the same ID in menus, mainly in menubars/toolbars
    // The reason is the fact wxWidgets propagates the wxUpdateUIEvent to all parent windows
    // to find wxUpdateUIEvent event functions matching the menuitem IDs found when activate a
    // menu in the first frame.

    // Reserve ROOM_FOR_KICADMANAGER IDs, for Kicad manager
    // Change it if this count is too small.
    ID_KICAD_MANAGER_START,
    ID_KICAD_MANAGER_END = ID_KICAD_MANAGER_START + ROOM_FOR_KICADMANAGER,

    // Reserve ROOM_FOR_KICADMANAGER IDs, for Kicad manager
    // Change it if this count is too small.
    ID_KICAD_3D_VIEWER_START,
    ID_KICAD_3D_VIEWER_END = ID_KICAD_3D_VIEWER_START + ROOM_FOR_3D_VIEWER,

    ID_KICAD_PANEL_PREV_MODEL_START,
    ID_KICAD_PANEL_PREV_MODEL_END = ID_KICAD_PANEL_PREV_MODEL_START + ROOM_FOR_PANEL_PREV_MODEL,

    // Reserve ID for popup menus, when we need to know a menu item is inside a popup menu
    ID_POPUP_MENU_START,

    // The extra here need to minimum be larger than MAX_BUS_UNFOLD_MENU_ITEMS +
    // MAX_UNIT_COUNT_PER_PACKAGE.
    // These values are stored in eeschema_id.h
    ID_POPUP_MENU_END = ID_POPUP_MENU_START + 4096,

    ID_END_LIST
};

#endif  // ID_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __ACTIONS_H
#define __ACTIONS_H

#include <tool/tool_action.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

#define LEGACY_HK_NAME( x ) x

/**
 * Class ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of a subclass of
 * ACTIONS is created inside of ACTION_MANAGER object that registers the actions.
 */
class ACTIONS
{
public:

    virtual ~ACTIONS() {};

    // Generic document actions
    static TOOL_ACTION doNew;           // sadly 'new' is a reserved word
    static TOOL_ACTION newLibrary;
    static TOOL_ACTION addLibrary;
    static TOOL_ACTION open;
    static TOOL_ACTION save;
    static TOOL_ACTION saveAs;
    static TOOL_ACTION saveCopyAs;
    static TOOL_ACTION saveAll;
    static TOOL_ACTION revert;
    static TOOL_ACTION pageSettings;
    static TOOL_ACTION print;
    static TOOL_ACTION plot;
    static TOOL_ACTION quit;

    // Generic edit actions
    static TOOL_ACTION cancelInteractive;
    static TOOL_ACTION showContextMenu;
    static TOOL_ACTION undo;
    static TOOL_ACTION redo;
    static TOOL_ACTION cut;
    static TOOL_ACTION copy;
    static TOOL_ACTION paste;
    static TOOL_ACTION duplicate;
    static TOOL_ACTION doDelete;        // sadly 'delete' is a reserved word
    static TOOL_ACTION deleteTool;

    // Find and Replace
    static TOOL_ACTION find;
    static TOOL_ACTION findAndReplace;
    static TOOL_ACTION findNext;
    static TOOL_ACTION findNextMarker;
    static TOOL_ACTION replaceAndFindNext;
    static TOOL_ACTION replaceAll;
    static TOOL_ACTION updateFind;

    // View controls
    static TOOL_ACTION zoomRedraw;
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION zoomInCenter;
    static TOOL_ACTION zoomOutCenter;
    static TOOL_ACTION zoomCenter;
    static TOOL_ACTION zoomFitScreen;
    static TOOL_ACTION zoomPreset;
    static TOOL_ACTION zoomTool;
    static TOOL_ACTION centerContents;
    static TOOL_ACTION toggleCursor;
    static TOOL_ACTION toggleCursorStyle;
    static TOOL_ACTION highContrastMode;

    static TOOL_ACTION refreshPreview;      // Similar to a synthetic mouseMoved event, but also
                                            // used after a rotate, mirror, etc.

    /// Cursor control with keyboard
    static TOOL_ACTION cursorUp;
    static TOOL_ACTION cursorDown;
    static TOOL_ACTION cursorLeft;
    static TOOL_ACTION cursorRight;

    static TOOL_ACTION cursorUpFast;
    static TOOL_ACTION cursorDownFast;
    static TOOL_ACTION cursorLeftFast;
    static TOOL_ACTION cursorRightFast;

    static TOOL_ACTION cursorClick;
    static TOOL_ACTION cursorDblClick;

    // Panning with keyboard
    static TOOL_ACTION panUp;
    static TOOL_ACTION panDown;
    static TOOL_ACTION panLeft;
    static TOOL_ACTION panRight;

    // Grid control
    static TOOL_ACTION gridFast1;
    static TOOL_ACTION gridFast2;
    static TOOL_ACTION gridNext;
    static TOOL_ACTION gridPrev;
    static TOOL_ACTION gridSetOrigin;
    static TOOL_ACTION gridResetOrigin;
    static TOOL_ACTION gridPreset;
    static TOOL_ACTION toggleGrid;
    static TOOL_ACTION gridProperties;

    // Units
    static TOOL_ACTION imperialUnits;
    static TOOL_ACTION metricUnits;
    static TOOL_ACTION toggleUnits;
    static TOOL_ACTION togglePolarCoords;
    static TOOL_ACTION resetLocalCoords;

    // Common Tools
    static TOOL_ACTION selectionTool;
    static TOOL_ACTION measureTool;
    static TOOL_ACTION pickerTool;

    // Misc
    static TOOL_ACTION show3DViewer;
    static TOOL_ACTION showSymbolBrowser;
    static TOOL_ACTION showSymbolEditor;
    static TOOL_ACTION showFootprintBrowser;
    static TOOL_ACTION showFootprintEditor;
    static TOOL_ACTION updatePcbFromSchematic;
    static TOOL_ACTION acceleratedGraphics;
    static TOOL_ACTION standardGraphics;

    // Internal
    static TOOL_ACTION updateMenu;
    static TOOL_ACTION activatePointEditor;

    // Suite
    static TOOL_ACTION configurePaths;
    static TOOL_ACTION showSymbolLibTable;
    static TOOL_ACTION showFootprintLibTable;
    static TOOL_ACTION gettingStarted;
    static TOOL_ACTION help;
    static TOOL_ACTION listHotKeys;
    static TOOL_ACTION getInvolved;

    /**
     * Function TranslateLegacyId()
     * Translates legacy tool ids to the corresponding TOOL_ACTION name.
     * @param aId is legacy tool id to be translated.
     * @return std::string is name of the corresponding TOOL_ACTION. It may be empty, if there is
     * no corresponding TOOL_ACTION.
     */
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) = 0;

    ///> Cursor control event types
    enum CURSOR_EVENT_TYPE { CURSOR_UP, CURSOR_DOWN, CURSOR_LEFT, CURSOR_RIGHT,
                             CURSOR_CLICK, CURSOR_DBL_CLICK, CURSOR_RIGHT_CLICK,
                             CURSOR_FAST_MOVE = 0x8000 };

    ///> Remove event modifier flags
    enum class REMOVE_FLAGS { NORMAL = 0x00, ALT = 0x01, CUT = 0x02 };
};


/**
 * Class EVENTS
 *
 * Gathers all the events that are shared by tools.
 */
class EVENTS
{
public:
    const static TOOL_EVENT SelectedEvent;
    const static TOOL_EVENT UnselectedEvent;
    const static TOOL_EVENT ClearedEvent;

    const static TOOL_EVENT SelectedItemsModified;
};

#endif // __ACTIONS_H



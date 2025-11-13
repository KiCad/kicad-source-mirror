/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_event.h>

#define LEGACY_HK_NAME( x ) x

class PCB_SELECTION_TOOL;
class GENERAL_COLLECTOR;

using CLIENT_SELECTION_FILTER =
        std::function<void( const VECTOR2I&, GENERAL_COLLECTOR&, PCB_SELECTION_TOOL* )>;


/**
 * Gather all the actions that are shared by tools.
 *
 * The instance of a subclass of ACTIONS is created inside of #ACTION_MANAGER object that
 * registers the actions.
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
    static TOOL_ACTION saveCopy;
    static TOOL_ACTION saveAll;
    static TOOL_ACTION revert;
    static TOOL_ACTION pageSettings;
    static TOOL_ACTION print;
    static TOOL_ACTION plot;
    static TOOL_ACTION quit;
    static TOOL_ACTION ddAddLibrary;    // for drag and drop lib
    static TOOL_ACTION openWithTextEditor;
    static TOOL_ACTION openDirectory;

    // Generic edit actions
    static TOOL_ACTION cancelInteractive;
    static TOOL_ACTION finishInteractive;
    static TOOL_ACTION showContextMenu;
    static TOOL_ACTION undo;
    static TOOL_ACTION redo;
    static TOOL_ACTION cut;
    static TOOL_ACTION copy;
    static TOOL_ACTION copyAsText;
    static TOOL_ACTION paste;
    static TOOL_ACTION pasteSpecial;
    static TOOL_ACTION selectAll;
    static TOOL_ACTION unselectAll;
    static TOOL_ACTION duplicate;
    static TOOL_ACTION doDelete;        // sadly 'delete' is a reserved word
    static TOOL_ACTION deleteTool;
    static TOOL_ACTION leftJustify;
    static TOOL_ACTION centerJustify;
    static TOOL_ACTION rightJustify;
    static TOOL_ACTION expandAll;
    static TOOL_ACTION collapseAll;

    // Incrementing
    static TOOL_ACTION increment;
    static TOOL_ACTION incrementPrimary;
    static TOOL_ACTION decrementPrimary;
    static TOOL_ACTION incrementSecondary;
    static TOOL_ACTION decrementSecondary;

    // Tables
    static TOOL_ACTION selectRows;
    static TOOL_ACTION selectColumns;
    static TOOL_ACTION selectTable;
    static TOOL_ACTION addRowAbove;
    static TOOL_ACTION addRowBelow;
    static TOOL_ACTION addColBefore;
    static TOOL_ACTION addColAfter;
    static TOOL_ACTION deleteRows;
    static TOOL_ACTION deleteColumns;
    static TOOL_ACTION mergeCells;
    static TOOL_ACTION unmergeCells;
    static TOOL_ACTION editTable;
    static TOOL_ACTION exportTableCSV;

    // Find and Replace
    static TOOL_ACTION showSearch;
    static TOOL_ACTION find;
    static TOOL_ACTION findAndReplace;
    static TOOL_ACTION findNext;
    static TOOL_ACTION findPrevious;
    static TOOL_ACTION findNextMarker;
    static TOOL_ACTION replaceAndFindNext;
    static TOOL_ACTION replaceAll;
    static TOOL_ACTION updateFind;

    // RC Lists
    static TOOL_ACTION prevMarker;
    static TOOL_ACTION nextMarker;
    static TOOL_ACTION excludeMarker;

    // View controls
    static TOOL_ACTION zoomRedraw;
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION zoomInCenter;
    static TOOL_ACTION zoomOutCenter;
    static TOOL_ACTION zoomInHorizontally;
    static TOOL_ACTION zoomOutHorizontally;
    static TOOL_ACTION zoomInVertically;
    static TOOL_ACTION zoomOutVertically;
    static TOOL_ACTION zoomCenter;
    static TOOL_ACTION zoomFitScreen;
    static TOOL_ACTION zoomFitObjects; // Zooms to bbox of items on screen (except page border)
    static TOOL_ACTION zoomFitSelection;
    static TOOL_ACTION zoomPreset;
    static TOOL_ACTION zoomTool;
    static TOOL_ACTION zoomUndo;
    static TOOL_ACTION zoomRedo;
    static TOOL_ACTION centerContents;
    static TOOL_ACTION centerSelection;
    static TOOL_ACTION toggleCursor;
    static TOOL_ACTION cursorSmallCrosshairs;
    static TOOL_ACTION cursorFullCrosshairs;
    static TOOL_ACTION cursor45Crosshairs;
    static TOOL_ACTION highContrastMode;
    static TOOL_ACTION highContrastModeCycle;
    static TOOL_ACTION toggleBoundingBoxes;

    static TOOL_ACTION refreshPreview;      // Similar to a synthetic mouseMoved event, but also
                                            // used after a rotate, mirror, etc.

    static TOOL_ACTION pinLibrary;
    static TOOL_ACTION unpinLibrary;
    static TOOL_ACTION showLibraryTree;
    static TOOL_ACTION hideLibraryTree;

    static TOOL_ACTION libraryTreeSearch;

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
    static TOOL_ACTION gridFastCycle;
    static TOOL_ACTION gridNext;
    static TOOL_ACTION gridPrev;
    static TOOL_ACTION gridSetOrigin;
    static TOOL_ACTION gridResetOrigin;
    static TOOL_ACTION gridPreset;
    static TOOL_ACTION toggleGrid;
    static TOOL_ACTION toggleGridOverrides;
    static TOOL_ACTION gridProperties;
    static TOOL_ACTION gridOrigin;

    // Units
    static TOOL_ACTION inchesUnits;
    static TOOL_ACTION milsUnits;
    static TOOL_ACTION millimetersUnits;
    static TOOL_ACTION updateUnits;
    static TOOL_ACTION toggleUnits;
    static TOOL_ACTION togglePolarCoords;
    static TOOL_ACTION resetLocalCoords;

    // Selection
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Select a single item under the cursor position
    static TOOL_ACTION selectionCursor;

    /// Set lasso selection mode
    static TOOL_ACTION selectSetRect;
    static TOOL_ACTION selectSetLasso;

    /// Clear the current selection
    static TOOL_ACTION selectionClear;

    /// Select an item (specified as the event parameter).
    static TOOL_ACTION selectItem;
    static TOOL_ACTION unselectItem;
    static TOOL_ACTION reselectItem;

    /// Select a list of items (specified as the event parameter)
    static TOOL_ACTION selectItems;
    static TOOL_ACTION unselectItems;

    /// Run a selection menu to select from a list of items
    static TOOL_ACTION selectionMenu;

    // Grouping
    static TOOL_ACTION group;
    static TOOL_ACTION ungroup;
    static TOOL_ACTION addToGroup;
    static TOOL_ACTION removeFromGroup;
    static TOOL_ACTION groupEnter;
    static TOOL_ACTION groupLeave;

    // Group Tool
    static TOOL_ACTION groupProperties;
    static TOOL_ACTION pickNewGroupMember;

    // Common Tools
    static TOOL_ACTION selectionTool;
    static TOOL_ACTION measureTool;
    static TOOL_ACTION pickerTool;
    static TOOL_ACTION pickerSubTool;

    // Misc
    static TOOL_ACTION showProjectManager;
    static TOOL_ACTION show3DViewer;
    static TOOL_ACTION showSymbolBrowser;
    static TOOL_ACTION showSymbolEditor;
    static TOOL_ACTION showFootprintBrowser;
    static TOOL_ACTION showFootprintEditor;
    static TOOL_ACTION showCalculatorTools;
    static TOOL_ACTION updatePcbFromSchematic;
    static TOOL_ACTION updateSchematicFromPcb;
    static TOOL_ACTION showProperties;
    static TOOL_ACTION showDatasheet;

    // Internal
    static TOOL_ACTION updateMenu;
    static TOOL_ACTION activatePointEditor;
    static TOOL_ACTION cycleArcEditMode;
    static TOOL_ACTION pointEditorArcKeepCenter;
    static TOOL_ACTION pointEditorArcKeepEndpoint;
    static TOOL_ACTION pointEditorArcKeepRadius;
    static TOOL_ACTION updatePreferences;
    static TOOL_ACTION selectLibTreeColumns;

    // Suite
    static TOOL_ACTION openPreferences;
    static TOOL_ACTION configurePaths;
    static TOOL_ACTION showSymbolLibTable;
    static TOOL_ACTION showFootprintLibTable;
    static TOOL_ACTION showDesignBlockLibTable;
    static TOOL_ACTION gettingStarted;
    static TOOL_ACTION help;
    static TOOL_ACTION about;
    static TOOL_ACTION listHotKeys;
    static TOOL_ACTION donate;
    static TOOL_ACTION getInvolved;
    static TOOL_ACTION reportBug;

    // API
    static TOOL_ACTION pluginsReload;

    // Embedding Files
    static TOOL_ACTION embeddedFiles;
    static TOOL_ACTION extractFile;
    static TOOL_ACTION removeFile;

    ///< Cursor control event types
    enum CURSOR_EVENT_TYPE
    {
        CURSOR_NONE = 0,
        CURSOR_UP,
        CURSOR_UP_FAST,
        CURSOR_DOWN,
        CURSOR_DOWN_FAST,
        CURSOR_LEFT,
        CURSOR_LEFT_FAST,
        CURSOR_RIGHT,
        CURSOR_RIGHT_FAST,
        CURSOR_CLICK,
        CURSOR_DBL_CLICK,
        CURSOR_RIGHT_CLICK
    };

    ///< Remove event modifier flags
    enum class REMOVE_FLAGS
    {
        NORMAL = 0x00,
        ALT    = 0x01,
        CUT    = 0x02
    };

    ///< Increment event parameters
    struct INCREMENT
    {
        // Amount to increment
        int Delta;
        // Which "thing" to increment
        // (what this is depends on the action - a pin might be number, then name)
        int Index;
    };
};


/**
 * Gather all the events that are shared by tools.
 */
class EVENTS
{
public:
    const static TOOL_EVENT PointSelectedEvent;
    const static TOOL_EVENT SelectedEvent;
    const static TOOL_EVENT UnselectedEvent;
    const static TOOL_EVENT ClearedEvent;

    const static TOOL_EVENT ConnectivityChangedEvent;

    ///< Selected item had a property changed (except movement)
    const static TOOL_EVENT SelectedItemsModified;

    ///< Selected items were moved, this can be very high frequency on the canvas, use with care
    const static TOOL_EVENT SelectedItemsMoved;

    ///< Used to inform tools that the selection should temporarily be non-editable
    const static TOOL_EVENT InhibitSelectionEditing;
    const static TOOL_EVENT UninhibitSelectionEditing;

    ///< Used to inform tool that it should display the disambiguation menu
    const static TOOL_EVENT DisambiguatePoint;

    ///< Used for hotkey feedback
    const static TOOL_EVENT GridChangedByKeyEvent;
    const static TOOL_EVENT ContrastModeChangedByKeyEvent;

    const static TOOL_EVENT UndoRedoPreEvent;
    const static TOOL_EVENT UndoRedoPostEvent;

};

#endif // __ACTIONS_H

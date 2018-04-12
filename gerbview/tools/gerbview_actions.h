/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GERBVIEW_ACTIONS_H
#define __GERBVIEW_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Class GERBVIEW_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of GERBVIEW_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class GERBVIEW_ACTIONS : public ACTIONS
{
public:
    // Selection Tool
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Select a single item under the cursor position
    static TOOL_ACTION selectionCursor;

    /// Clears the current selection
    static TOOL_ACTION selectionClear;

    /// Selects an item (specified as the event parameter).
    static TOOL_ACTION selectItem;

    /// Unselects an item (specified as the event parameter).
    static TOOL_ACTION unselectItem;

    /// Activation of the edit tool
    static TOOL_ACTION properties;

    static TOOL_ACTION measureTool;

    // View controls
    static TOOL_ACTION zoomIn;
    static TOOL_ACTION zoomOut;
    static TOOL_ACTION zoomInCenter;
    static TOOL_ACTION zoomOutCenter;
    static TOOL_ACTION zoomCenter;
    static TOOL_ACTION zoomFitScreen;
    static TOOL_ACTION zoomPreset;

    // Display modes
    static TOOL_ACTION linesDisplayOutlines;
    static TOOL_ACTION flashedDisplayOutlines;
    static TOOL_ACTION polygonsDisplayOutlines;
    static TOOL_ACTION negativeObjectDisplay;
    static TOOL_ACTION dcodeDisplay;
    static TOOL_ACTION highContrastMode;
    static TOOL_ACTION highContrastInc;
    static TOOL_ACTION highContrastDec;

    // Layer control
    static TOOL_ACTION layerPrev;
    static TOOL_ACTION layerNext;
    static TOOL_ACTION layerAlphaInc;
    static TOOL_ACTION layerAlphaDec;
    static TOOL_ACTION layerToggle;

    static TOOL_ACTION layerChanged;        // notification

    // Grid control
    static TOOL_ACTION gridFast1;
    static TOOL_ACTION gridFast2;
    static TOOL_ACTION gridNext;
    static TOOL_ACTION gridPrev;
    static TOOL_ACTION gridSetOrigin;
    static TOOL_ACTION gridResetOrigin;
    static TOOL_ACTION gridPreset;

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

    // Miscellaneous
    static TOOL_ACTION selectionTool;
    static TOOL_ACTION zoomTool;
    static TOOL_ACTION panTool;
    static TOOL_ACTION pickerTool;
    static TOOL_ACTION resetCoords;
    static TOOL_ACTION switchCursor;
    static TOOL_ACTION switchUnits;
    static TOOL_ACTION showHelp;
    static TOOL_ACTION toBeDone;

    // Highlighting
    static TOOL_ACTION highlightClear;
    static TOOL_ACTION highlightNet;
    static TOOL_ACTION highlightComponent;
    static TOOL_ACTION highlightAttribute;

    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override;

    ///> @copydoc COMMON_ACTIONS::RegisterAllTools()
    virtual void RegisterAllTools( TOOL_MANAGER* aToolManager ) override;
};

#endif  // __GERBVIEW_ACTIONS_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jon Evans <jon@craftyjon.com>
 * Copyright (C) 2017-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * Gather all the actions that are shared by tools. The instance of GERBVIEW_ACTIONS is created
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

    /// Clear the current selection
    static TOOL_ACTION selectionClear;

    /// Selects an item (specified as the event parameter).
    static TOOL_ACTION selectItem;

    /// Unselects an item (specified as the event parameter).
    static TOOL_ACTION unselectItem;

    /// Activation of the edit tool
    static TOOL_ACTION properties;
    static TOOL_ACTION showDCodes;
    static TOOL_ACTION showSource;

    static TOOL_ACTION exportToPcbnew;

    // Display modes
    static TOOL_ACTION linesDisplayOutlines;
    static TOOL_ACTION flashedDisplayOutlines;
    static TOOL_ACTION polygonsDisplayOutlines;
    static TOOL_ACTION negativeObjectDisplay;
    static TOOL_ACTION dcodeDisplay;
    static TOOL_ACTION toggleDiffMode;
    static TOOL_ACTION flipGerberView;

    // Layer control
    static TOOL_ACTION layerPrev;
    static TOOL_ACTION layerNext;
    static TOOL_ACTION clearLayer;
    static TOOL_ACTION clearAllLayers;
    static TOOL_ACTION reloadAllLayers;

    static TOOL_ACTION layerChanged;        // notification

    // Files
    static TOOL_ACTION openGerber;
    static TOOL_ACTION openDrillFile;
    static TOOL_ACTION openJobFile;
    static TOOL_ACTION openZipFile;

    // Open/close the layer manager
    static TOOL_ACTION toggleLayerManager;

    // Highlighting
    static TOOL_ACTION highlightClear;
    static TOOL_ACTION highlightNet;
    static TOOL_ACTION highlightComponent;
    static TOOL_ACTION highlightAttribute;
    static TOOL_ACTION highlightDCode;
};

#endif  // __GERBVIEW_ACTIONS_H

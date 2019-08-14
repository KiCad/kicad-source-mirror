/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PL_ACTIONS_H
#define PL_ACTIONS_H

#include <tool/tool_action.h>
#include <tool/actions.h>
#include <core/optional.h>

class TOOL_EVENT;
class TOOL_MANAGER;

/**
 * Class PL_ACTIONS
 *
 * Gathers all the actions that are shared by tools. The instance of PL_ACTIONS is created
 * inside of ACTION_MANAGER object that registers the actions.
 */
class PL_ACTIONS : public ACTIONS
{
public:
    // Selection Tool
    /// Activation of the selection tool
    static TOOL_ACTION selectionActivate;

    /// Clears the current selection
    static TOOL_ACTION clearSelection;

    /// Selects an item (specified as the event parameter).
    static TOOL_ACTION addItemToSel;
    static TOOL_ACTION removeItemFromSel;

    /// Selects a list of items (specified as the event parameter)
    static TOOL_ACTION addItemsToSel;
    static TOOL_ACTION removeItemsFromSel;

    /// Runs a selection menu to select from a list of items
    static TOOL_ACTION selectionMenu;

    // Tools
    static TOOL_ACTION pickerTool;
    static TOOL_ACTION placeText;
    static TOOL_ACTION placeImage;
    static TOOL_ACTION drawRectangle;
    static TOOL_ACTION drawLine;
    static TOOL_ACTION appendImportedWorksheet;

    // Editing
    static TOOL_ACTION move;

    // Miscellaneous
    static TOOL_ACTION refreshPreview;
    static TOOL_ACTION toggleBackground;
    static TOOL_ACTION showInspector;
    static TOOL_ACTION previewSettings;

    ///> @copydoc COMMON_ACTIONS::TranslateLegacyId()
    virtual OPT<TOOL_EVENT> TranslateLegacyId( int aId ) override { return OPT<TOOL_EVENT>(); }
};


#endif

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Ian McInerney <ian.s.mcinerney at ieee.org>
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

#ifndef EDITOR_CONDITIONS_H_
#define EDITOR_CONDITIONS_H_

#include <class_draw_panel_gal.h>
#include <functional>
#include <tool/selection.h>
#include <tool/selection_conditions.h>

class EDA_BASE_FRAME;
class EDA_DRAW_FRAME;
class TOOL_ACTION;

namespace KIGFX
{
    enum class CROSS_HAIR_MODE;
}

/**
 * Class that groups generic conditions for editor states.
 */
class EDITOR_CONDITIONS : public SELECTION_CONDITIONS
{
public:
    /**
     * Create an object to define conditions dependent upon a specific frame.
     *
     * @param aFrame is the frame to query for the conditions
     */
    EDITOR_CONDITIONS( EDA_BASE_FRAME* aFrame ) :
        m_frame( aFrame )
    {}

    /**
     * Create a functor that tests if the content of the frame is modified.
     *
     * @return Functor testing for modified content.
     */
    SELECTION_CONDITION ContentModified();

    /**
     * Create a functor that tests if there are any items in the undo queue.
     *
     * @return Functor testing if the undo queue has items.
     */
    virtual SELECTION_CONDITION UndoAvailable();

    /**
     * Create a functor that tests if there are any items in the redo queue.
     *
     * @return Functor testing if the redo queue has items.
     */
    SELECTION_CONDITION RedoAvailable();

    /**
     * Create a functor that tests if the frame has the specified units.
     *
     * @return Functor testing the units of a frame.
     */
    SELECTION_CONDITION Units( EDA_UNITS aUnit );

    /**
     * Create a functor testing if the specified tool is the current active tool in the frame.
     *
     * @return Functor testing the current tool of a frame.
     */
    SELECTION_CONDITION CurrentTool( const TOOL_ACTION& aTool );

    /**
     * Create a functor testing if there are no tools active in the frame.
     *
     * @return Functor testing the frame has no tools running.
     */
    SELECTION_CONDITION NoActiveTool();

    /**
     * Create a functor testing if the grid is visible in a frame.
     *
     * @note This requires the frame passed into the constructor be be derived from EDA_DRAW_FRAME.
     *
     * @return Functor testing if the grid is visible
     */
    SELECTION_CONDITION GridVisible();

    /**
     * Create a functor testing if the grid overrides wires is enabled in a frame.
     *
     * @note This requires the frame passed into the constructor be be derived from EDA_DRAW_FRAME.
     *
     * @return Functor testing if grid overrides are enabled
     */
    SELECTION_CONDITION GridOverrides();

    /**
     * Create a functor testing if polar coordinates are current being used.
     *
     * @note This requires the frame passed into the constructor be be derived from EDA_DRAW_FRAME.
     *
     * @return Functor testing if the grid is visible
     */
    SELECTION_CONDITION PolarCoordinates();

    /**
     * Create a functor testing if the cursor is full screen in a frame.
     *
     * @note This requires the frame passed into the constructor be be derived from EDA_DRAW_FRAME.
     *
     * @return Functor testing if the cursor is full screen
     */
    SELECTION_CONDITION CursorSmallCrosshairs();
    SELECTION_CONDITION CursorFullCrosshairs();
    SELECTION_CONDITION Cursor45Crosshairs();

    SELECTION_CONDITION BoundingBoxes();

    /**
     * Create a functor testing if the python scripting console window is visible.
     *
     * @note This requires the frame passed into the constructor be be derived from EDA_DRAW_FRAME.
     *
     * @return Functor testing if the python scripting console window is visible
     */
    SELECTION_CONDITION ScriptingConsoleVisible();

protected:
    /// Helper function used by ContentModified().
    static bool contentModifiedFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame );

    /// Helper function used by UndoAvailable().
    static bool undoFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame );

    /// Helper function used by RedoAvailable().
    static bool redoFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame );

    /// Helper function used by Units().
    static bool unitsFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame, EDA_UNITS aUnits );

    /// Helper function used by CurrentTool().
    static bool toolFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame,
                          const TOOL_ACTION& aTool );

    /// Helper function used by NoActiveTool().
    static bool noToolFunc( const SELECTION& aSelection, EDA_BASE_FRAME* aFrame );

    /// Helper function used by GridVisible().
    static bool gridFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame );

    /// Helper function used by GridOverrides().
    static bool gridOverridesFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame );

    /// Helper function used by PolarCoordinates().
    static bool polarCoordFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame );

    /// Helper function used by FullscreenCursor().
    static bool cursorFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame,
                            KIGFX::CROSS_HAIR_MODE aMode );

    /// Helper function used by DrawBoundingBoxes().
    static bool bboxesFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame );

    /// Helper function used by ScriptingConsoleVisible().
    static bool consoleVisibleFunc( const SELECTION& aSelection, EDA_DRAW_FRAME* aFrame );

    /// The frame to apply the conditions to.
    EDA_BASE_FRAME* m_frame;
};

#endif /* EDITOR_CONDITIONS_H_ */

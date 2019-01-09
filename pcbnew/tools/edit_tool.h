/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __EDIT_TOOL_H
#define __EDIT_TOOL_H

#include <math/vector2d.h>
#include <tools/pcb_tool.h>
#include <tools/selection_tool.h>
#include <status_popup.h>


class BOARD_COMMIT;
class BOARD_ITEM;
class CONNECTIVITY_DATA;

/**
 * Function EditToolSelectionFilter
 *
 * A CLIENT_SELECTION_FILTER which promotes pad selections to their parent modules and
 * optionally excludes locked items and/or transient items (such as markers).
 */

#define EXCLUDE_LOCKED      0x0001
#define EXCLUDE_LOCKED_PADS 0x0002
#define EXCLUDE_TRANSIENTS  0x0004

void EditToolSelectionFilter( GENERAL_COLLECTOR& aCollector, int aFlags );

/**
 * Class EDIT_TOOL
 *
 * The interactive edit tool. Allows one to move, rotate, flip and change properties of items selected
 * using the pcbnew.InteractiveSelection tool.
 */

class EDIT_TOOL : public PCB_TOOL
{
public:
    EDIT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Function Main()
     *
     * Main loop in which events are handled.
     * @param aEvent is the handled event.
     */
    int Main( const TOOL_EVENT& aEvent );

    /**
     * Function Drag()
     *
     * todo
     */
    int Drag( const TOOL_EVENT& aEvent );

    /**
     * Function Edit()
     *
     * Displays properties window for the selected object.
     */
    int Properties( const TOOL_EVENT& aEvent );

    /**
     * Function Rotate()
     *
     * Rotates currently selected items.
     */
    int Rotate( const TOOL_EVENT& aEvent );

    /**
     * Function Flip()
     *
     * Rotates currently selected items. The rotation point is the current cursor position.
     */
    int Flip( const TOOL_EVENT& aEvent );

    /**
     * Function Mirror
     *
     * Mirrors the current selection. The mirror axis passes through the current point.
     */
    int Mirror( const TOOL_EVENT& aEvent );

    /**
     * Function Remove()
     *
     * Deletes currently selected items. The rotation point is the current cursor position.
     */
    int Remove( const TOOL_EVENT& aEvent );

    /**
     * Function Duplicate()
     *
     * Duplicates the current selection and starts a move action.
     */
    int Duplicate( const TOOL_EVENT& aEvent );

    /**
     * Function MoveExact()
     *
     * Invokes a dialog box to allow moving of the item by an exact amount.
     */
    int MoveExact( const TOOL_EVENT& aEvent );

    /**
     * Function CreateArray()
     *
     * Creates an array of the selected items, invoking the array editor dialog
     * to set the array options
     */
    int CreateArray( const TOOL_EVENT& aEvent );

    /**
     * Function ExchangeFootprints()
     *
     * Invoke the dialog used to update or exchange the footprints used for
     * modules.  The mode depends on the PCB_ACTIONS held by the TOOL_EVENT.
     */
    int ExchangeFootprints( const TOOL_EVENT& aEvent );

    ///> Launches a tool to measure between points
    int MeasureTool( const TOOL_EVENT& aEvent );

    /**
     * Function FootprintFilter()
     *
     * A selection filter which prunes the selection to contain only items
     * of type PCB_MODULE_T
     */
    static void FootprintFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector );

    /**
     * Function PadFilter()
     *
     * A selection filter which prunes the selection to contain only items
     * of type PCB_PAD_T
     */
    static void PadFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector );

    ///> Sets up handlers for various events.
    void setTransitions() override;

    /**
     * Function copyToClipboard()
     * Sends the current selection to the clipboard by formatting it as a fake pcb
     * see AppendBoardFromClipboard for importing
     * @return True if it was sent succesfully
     */
    int copyToClipboard( const TOOL_EVENT& aEvent );

    int copyToClipboardWithAnchor( const TOOL_EVENT& aEvent );

    int doCopyToClipboard( bool withAnchor );

    /**
     * Function cutToClipboard()
     * Cuts the current selection to the clipboard by formatting it as a fake pcb
     * see AppendBoardFromClipboard for importing
     * @return True if it was sent succesfully
     */
    int cutToClipboard( const TOOL_EVENT& aEvent );

    BOARD_COMMIT* GetCurrentCommit() const
    {
        return m_commit.get();
    }

private:
    ///> Selection tool used for obtaining selected items
    SELECTION_TOOL* m_selectionTool;

    ///> Flag determining if anything is being dragged right now
    bool m_dragging;

    ///> Flag determining whether we are prompting for locked removal
    bool m_lockedSelected;

    ///> Last cursor position (needed for getModificationPoint() to avoid changes
    ///> of edit reference point).
    VECTOR2I m_cursor;

    ///> Returns the right modification point (e.g. for rotation), depending on the number of
    ///> selected items.
    bool updateModificationPoint( SELECTION& aSelection );

    int editFootprintInFpEditor( const TOOL_EVENT& aEvent );

    bool invokeInlineRouter( int aDragMode );
    bool isInteractiveDragEnabled() const;

    bool changeTrackWidthOnClick( const SELECTION& selection );
    bool pickCopyReferencePoint( VECTOR2I& aP );

    std::unique_ptr<BOARD_COMMIT> m_commit;
};

#endif

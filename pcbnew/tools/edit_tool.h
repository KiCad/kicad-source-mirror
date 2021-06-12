/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2020 CERN
 * Copyright (C) 2013-2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection_tool.h>
#include <status_popup.h>


class BOARD_COMMIT;
class BOARD_ITEM;
class CONNECTIVITY_DATA;
class STATUS_TEXT_POPUP;

namespace KIGFX
{
    namespace PREVIEW
    {
        class RULER_ITEM;
    }
}


class SPECIAL_TOOLS_CONTEXT_MENU : public CONDITIONAL_MENU
{
public:
    SPECIAL_TOOLS_CONTEXT_MENU( TOOL_INTERACTIVE* aTool );
};

/**
 * The interactive edit tool.
 *
 * Allows one to move, rotate, flip and change properties of items selected using the
 * pcbnew.InteractiveSelection tool.
 */

class EDIT_TOOL : public PCB_TOOL_BASE
{
public:
    EDIT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    ///< Find an item and start moving.
    int GetAndPlace( const TOOL_EVENT& aEvent );

    /**
     * Main loop in which events are handled.
     */
    int Move( const TOOL_EVENT& aEvent );

    /**
     * Invoke the PNS router to drag tracks or do an offline resizing of an arc track
     * if a single arc track is selected.
     */
    int Drag( const TOOL_EVENT& aEvent );

    /**
     * Drag-resize an arc (and change end points of connected straight segments).
     */
    int DragArcTrack( const TOOL_EVENT& aTrack );

    /**
     * Display properties window for the selected object.
     */
    int Properties( const TOOL_EVENT& aEvent );

    /**
     * Rotate currently selected items.
     */
    int Rotate( const TOOL_EVENT& aEvent );

    /**
     * Rotate currently selected items. The rotation point is the current cursor position.
     */
    int Flip( const TOOL_EVENT& aEvent );

    /**
     * Mirror the current selection. The mirror axis passes through the current point.
     */
    int Mirror( const TOOL_EVENT& aEvent );

    int ChangeTrackWidth( const TOOL_EVENT& aEvent );

    /**
     * Fillet (i.e. adds an arc tangent to) all selected straight tracks by a user defined radius.
     */
    int FilletTracks( const TOOL_EVENT& aEvent );

    /**
     * Delete currently selected items.
     */
    int Remove( const TOOL_EVENT& aEvent );

    /**
     * Duplicate the current selection and starts a move action.
     */
    int Duplicate( const TOOL_EVENT& aEvent );

    /**
     * Invoke a dialog box to allow moving of the item by an exact amount.
     */
    int MoveExact( const TOOL_EVENT& aEvent );

    /**
     * Move an item but with a reference point selected first
     */
    int MoveWithReference( const TOOL_EVENT& aEvent );

    /**
     * Create an array of the selected items, invoking the array editor dialog to set the options.
     */
    int CreateArray( const TOOL_EVENT& aEvent );

    /**
     * A selection filter which prunes the selection to contain only items of type #PCB_MODULE_T.
     */
    static void FootprintFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                                 PCB_SELECTION_TOOL* sTool );

    /**
     * A selection filter which prunes the selection to contain only items of type #PCB_PAD_T.
     */
    static void PadFilter( const VECTOR2I&, GENERAL_COLLECTOR& aCollector,
                           PCB_SELECTION_TOOL* sTool );

    BOARD_COMMIT* GetCurrentCommit() const { return m_commit.get(); }

private:
    ///< Set up handlers for various events.
    void setTransitions() override;

    /**
     * Send the current selection to the clipboard by formatting it as a fake pcb
     * see #AppendBoardFromClipboard for importing.
     */
    int copyToClipboard( const TOOL_EVENT& aEvent );

    /**
     * Cut the current selection to the clipboard by formatting it as a fake pcb
     * see #AppendBoardFromClipboard for importing.
     */
    int cutToClipboard( const TOOL_EVENT& aEvent );

    ///< Return the right modification point (e.g. for rotation), depending on the number of
    ///< selected items.
    bool updateModificationPoint( PCB_SELECTION& aSelection );

    bool invokeInlineRouter( int aDragMode );
    bool isInteractiveDragEnabled() const;
    bool isRouterActive() const;

    int doMoveSelection( TOOL_EVENT aEvent, bool aPickReference = false );

    bool pickReferencePoint( const wxString& aTooltip, const wxString& aSuccessMessage,
                             const wxString& aCanceledMessage, VECTOR2I& aReferencePoint );

private:
    PCB_SELECTION_TOOL*           m_selectionTool;
    std::unique_ptr<BOARD_COMMIT> m_commit;
    bool                          m_dragging;   // Indicates objects are currently being dragged
    VECTOR2I                      m_cursor;     // Last cursor position (so getModificationPoint()
                                                // can avoid changes of edit reference point).
    std::unique_ptr<STATUS_TEXT_POPUP> m_statusPopup;
};

#endif

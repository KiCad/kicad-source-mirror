/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef PCB_CONTROL_H
#define PCB_CONTROL_H

#include <io_mgr.h>
#include <memory>
#include <tools/pcb_tool_base.h>

namespace KIGFX {
    class ORIGIN_VIEWITEM;
}

class PCB_BASE_FRAME;
class BOARD_ITEM;

/**
 * Handle actions that are shared between different frames in PcbNew.
 */

class PCB_CONTROL : public PCB_TOOL_BASE
{
public:
    PCB_CONTROL();
    ~PCB_CONTROL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    int AddLibrary( const TOOL_EVENT& aEvent );
    int Print( const TOOL_EVENT& aEvent );
    int Quit( const TOOL_EVENT& aEvent );

    // Display modes
    int ToggleRatsnest( const TOOL_EVENT& aEvent );
    int ZoneDisplayMode( const TOOL_EVENT& aEvent );
    int TrackDisplayMode( const TOOL_EVENT& aEvent );
    int ViaDisplayMode( const TOOL_EVENT& aEvent );

    // Update the view with the new high-contrast mode from the display settings
    int HighContrastMode( const TOOL_EVENT& aEvent );

    // Rotate through the available high-contrast modes
    int HighContrastModeCycle( const TOOL_EVENT& aEvent );

    // Layer control
    int LayerSwitch( const TOOL_EVENT& aEvent );
    int LayerNext( const TOOL_EVENT& aEvent );
    int LayerPrev( const TOOL_EVENT& aEvent );
    int LayerToggle( const TOOL_EVENT& aEvent );
    int LayerAlphaInc( const TOOL_EVENT& aEvent );
    int LayerAlphaDec( const TOOL_EVENT& aEvent );

    // Grid control
    int GridSetOrigin( const TOOL_EVENT& aEvent );
    int GridResetOrigin( const TOOL_EVENT& aEvent );

    // Low-level access (below undo) to setting the grid origin
    static void DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                 EDA_ITEM* originViewItem, const VECTOR2D& aPoint );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );

    // Miscellaneous
    int DeleteItemCursor( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );
    int AppendBoardFromFile( const TOOL_EVENT& aEvent );
    int AppendBoard( PLUGIN& pi, wxString& fileName );
    int UpdateMessagePanel( const TOOL_EVENT& aEvent );

    int FlipPcbView( const TOOL_EVENT& aEvent );

private:
    ///< Sets up handlers for various events.
    void setTransitions() override;

    /**
     * Add and select or just select for move/place command a list of board items.
     *
     * @param aItems is the list of items
     * @param aIsNew = true to add items to the current board, false to just select if
     *               items are already managed by the current board
     * @param aAnchorAtOrigin = true if the items are translated so that the anchor is {0, 0}
     *                        (if false, the top-left item's origin will be used)
     */
    int placeBoardItems( std::vector<BOARD_ITEM*>& aItems, bool aIsNew, bool aAnchorAtOrigin );

    int placeBoardItems( BOARD* aBoard, bool aAnchorAtOrigin );

    ///< Pointer to the currently used edit frame.
    PCB_BASE_FRAME* m_frame;

    ///< Grid origin marker.
    std::unique_ptr<KIGFX::ORIGIN_VIEWITEM> m_gridOrigin;

    BOARD_ITEM* m_pickerItem;
};

#endif

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_io/pcb_io_mgr.h>
#include <memory>
#include <tools/pcb_tool_base.h>
#include <status_popup.h>

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

    // Footprint library control actions
    int LoadFpFromBoard( const TOOL_EVENT& aEvent );
    int SaveFpToBoard( const TOOL_EVENT& aEvent );
    int IterateFootprint( const TOOL_EVENT& aEvent );

    // Display modes
    int ToggleRatsnest( const TOOL_EVENT& aEvent );
    int ZoneDisplayMode( const TOOL_EVENT& aEvent );
    int TrackDisplayMode( const TOOL_EVENT& aEvent );
    int ViaDisplayMode( const TOOL_EVENT& aEvent );

    // Update the view with the new high-contrast mode from the display settings
    int HighContrastMode( const TOOL_EVENT& aEvent );

    int ContrastModeFeedback( const TOOL_EVENT& aEvent );

    // Rotate through the available high-contrast, net color and ratsnest color modes
    int HighContrastModeCycle( const TOOL_EVENT& aEvent );
    int NetColorModeCycle( const TOOL_EVENT& aEvent );
    int RatsnestModeCycle( const TOOL_EVENT& aEvent );

    // Layer control
    int LayerSwitch( const TOOL_EVENT& aEvent );
    int LayerNext( const TOOL_EVENT& aEvent );
    int LayerPrev( const TOOL_EVENT& aEvent );
    int LayerToggle( const TOOL_EVENT& aEvent );
    int LayerAlphaInc( const TOOL_EVENT& aEvent );
    int LayerAlphaDec( const TOOL_EVENT& aEvent );
    int CycleLayerPresets( const TOOL_EVENT& aEvent );
    int LayerPresetFeedback( const TOOL_EVENT& aEvent );

    // Grid control
    int GridPlaceOrigin( const TOOL_EVENT& aEvent );
    int GridResetOrigin( const TOOL_EVENT& aEvent );

    // Low-level access (below undo) to setting the grid origin
    static void DoSetGridOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                                 EDA_ITEM* originViewItem, const VECTOR2D& aPoint );

    int Undo( const TOOL_EVENT& aEvent );
    int Redo( const TOOL_EVENT& aEvent );

    // Snapping control
    int SnapMode( const TOOL_EVENT& aEvent );
    int SnapModeFeedback( const TOOL_EVENT& aEvent );

    // Miscellaneous
    int InteractiveDelete( const TOOL_EVENT& aEvent );
    int Paste( const TOOL_EVENT& aEvent );
    int AppendBoardFromFile( const TOOL_EVENT& aEvent );
    int AppendDesignBlock( const TOOL_EVENT& aEvent );
    int ApplyDesignBlockLayout( const TOOL_EVENT& aEvent );
    int PlaceLinkedDesignBlock( const TOOL_EVENT& aEvent );
    int SaveToLinkedDesignBlock( const TOOL_EVENT& aEvent );
    int AppendBoard( PCB_IO& pi, const wxString& fileName, DESIGN_BLOCK* aDesignBlock = nullptr,
                     BOARD_COMMIT* aCommit = nullptr, bool aSkipMove = false );
    int UpdateMessagePanel( const TOOL_EVENT& aEvent );
    int PlaceCharacteristics( const TOOL_EVENT& aEvent );
    int PlaceStackup( const TOOL_EVENT& aEvent );
    int CollectAndEmbed3DModels( const TOOL_EVENT& aEvent );

    int FlipPcbView( const TOOL_EVENT& aEvent );

    int RehatchShapes( const TOOL_EVENT& aEvent );

    // Drag and drop
    int DdAppendBoard( const TOOL_EVENT& aEvent );
    int DdAddLibrary( const TOOL_EVENT& aEvent );
    int DdImportFootprint( const TOOL_EVENT& aEvent );

private:
    ///< Sets up handlers for various events.
    void setTransitions() override;

    /**
     * We have bug reports indicating that some new users confuse zone filling/unfilling with
     * the display modes.  This will put up a warning if they show zone fills when one or more
     * zones are unfilled.
     */
    void unfilledZoneCheck();

    /**
     * Helper for pasting.  Remove non-enabled layers from the items in \a aItems.  If an item
     * exists only on non-enabled layers, it will be removed entirely.
     */
    void pruneItemLayers( std::vector<BOARD_ITEM*>& aItems );

    /**
     * Add and select or just select for move/place command a list of board items.
     *
     * @param aItems is the list of items
     * @param aIsNew = true to add items to the current board, false to just select if
     *               items are already managed by the current board
     * @param aAnchorAtOrigin = true if the items are translated so that the anchor is {0, 0}
     *                        (if false, the top-left item's origin will be used)
     * @param aReannotateDuplicates = true to reannotate any footprints with a designator
     *                                that already exist in the board.
     */
    bool placeBoardItems( BOARD_COMMIT* aCommit, std::vector<BOARD_ITEM*>& aItems, bool aIsNew, bool aAnchorAtOrigin,
                          bool aReannotateDuplicates, bool aSkipMove );

    bool placeBoardItems( BOARD_COMMIT* aCommit, BOARD* aBoard, bool aAnchorAtOrigin, bool aReannotateDuplicates,
                          bool aSkipMove );

    void rehatchBoardItem( BOARD_ITEM* aItem );

private:
    PCB_BASE_FRAME*                         m_frame;

    std::unique_ptr<KIGFX::ORIGIN_VIEWITEM> m_gridOrigin;

    BOARD_ITEM*                             m_pickerItem;

    std::unique_ptr<STATUS_TEXT_POPUP>      m_statusPopup;
};

#endif

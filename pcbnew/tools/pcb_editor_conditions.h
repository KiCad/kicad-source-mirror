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

#ifndef PCB_EDITOR_CONDITIONS_H_
#define PCB_EDITOR_CONDITIONS_H_

#include <functional>
#include <tool/editor_conditions.h>
#include <tool/selection.h>
#include <tool/tool_action.h>

#include <pcb_base_frame.h>

class EDA_BASE_FRAME;
class EDA_DRAW_FRAME;

/**
 * Group generic conditions for PCB editor states.
 */
class PCB_EDITOR_CONDITIONS : public EDITOR_CONDITIONS
{
public:
    PCB_EDITOR_CONDITIONS( PCB_BASE_FRAME* aFrame ) :
        EDITOR_CONDITIONS( aFrame )
    {}

    /**
     * Create a functor that tests if there are items in the board
     *
     * @return Functor returning true if the current board has items
     */
    SELECTION_CONDITION HasItems();

    /**
     * Create a functor that tests if the pad numbers are displayed
     *
     * @return Functor returning true if the pad numbers are displayed
     */
    SELECTION_CONDITION PadNumbersDisplay();

    /**
     * Create a functor that tests if the frame fills the pads
     *
     * @return Functor returning true if the pads are filled
     */
    SELECTION_CONDITION PadFillDisplay();

    /**
     * Create a functor that tests if the frame fills text items
     *
     * @return Functor returning true if the text items are filled
     */
    SELECTION_CONDITION TextFillDisplay();

    /**
     * Create a functor that tests if the frame fills graphics items
     *
     * @return Functor returning true if graphics items are filled
     */
    SELECTION_CONDITION GraphicsFillDisplay();

    /**
     * Create a functor that tests if the frame fills vias
     *
     * @return Functor returning true if vias are filled
     */
    SELECTION_CONDITION ViaFillDisplay();

    /**
     * Create a functor that tests if the frame fills tracks
     *
     * @return Functor returning true if tracks are filled
     */
    SELECTION_CONDITION TrackFillDisplay();

    /**
     * Create a functor that tests the current zone display mode in the frame
     *
     * @param aMode is the mode to test for
     * @return Functor returning true if the frame is using the specified mode
     */
    SELECTION_CONDITION ZoneDisplayMode( ZONE_DISPLAY_MODE aMode );

    /**
     * Create a functor that tests if the footprint viewer should auto zoom on new footprints.
     *
     * @return Functor returning true if auto zoom is enabled.
     */
    SELECTION_CONDITION FootprintViewerAutoZoom();

protected:
    ///< Helper function used by HasItems()
    static bool hasItemsFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by PadNumbersDisplay()
    static bool padNumberDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by PadFillDisplay()
    static bool padFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by TextFillDisplay()
    static bool textFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by GraphicsFillDisplay()
    static bool graphicsFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by ViaFillDisplay()
    static bool viaFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by TrackFillDisplay()
    static bool trackFillDisplayFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );

    ///< Helper function used by ZoneDisplayMode()
    static bool zoneDisplayModeFunc( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame,
                                     ZONE_DISPLAY_MODE aMode );

    /// Helper function used by FootprintViewerAutoZoom()
    static bool footprintViewerAutoZoom( const SELECTION& aSelection, PCB_BASE_FRAME* aFrame );
};

#endif /* PCB_EDITOR_CONDITIONS_H_ */

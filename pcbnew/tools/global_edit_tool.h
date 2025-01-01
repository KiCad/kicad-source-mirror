/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#ifndef GLOBAL_EDIT_TOOL_H
#define GLOBAL_EDIT_TOOL_H

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <tools/pcb_selection_tool.h>
#include <status_popup.h>


class BOARD_COMMIT;
class BOARD_ITEM;
class CONNECTIVITY_DATA;


class GLOBAL_EDIT_TOOL : public PCB_TOOL_BASE
{
public:
    GLOBAL_EDIT_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /**
     * Invoke the dialog used to update or exchange the footprint definitions used for
     * footprints.
     *
     * The mode depends on the #PCB_ACTIONS held by the #TOOL_EVENT.
     */
    int ExchangeFootprints( const TOOL_EVENT& aEvent );

    int SwapLayers( const TOOL_EVENT& aEvent );

    int EditTracksAndVias( const TOOL_EVENT& aEvent );
    int EditTextAndGraphics( const TOOL_EVENT& aEvent );
    int EditTeardrops( const TOOL_EVENT& aEvent );
    int GlobalDeletions( const TOOL_EVENT& aEvent );
    int CleanupTracksAndVias( const TOOL_EVENT& aEvent );
    int CleanupGraphics( const TOOL_EVENT& aEvent );
    int RemoveUnusedPads( const TOOL_EVENT& aEvent );
    int ZonesManager( const TOOL_EVENT& aEvent );

private:
    bool swapBoardItem( BOARD_ITEM* aItem, std::map<PCB_LAYER_ID, PCB_LAYER_ID>& aLayerMap );

    ///< Set up handlers for various events.
    void setTransitions() override;

private:
    PCB_SELECTION_TOOL*           m_selectionTool;
    std::unique_ptr<BOARD_COMMIT> m_commit;
};

#endif

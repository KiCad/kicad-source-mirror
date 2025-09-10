/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef BOARD_INSPECTION_TOOL_H
#define BOARD_INSPECTION_TOOL_H

#include <dialogs/dialog_book_reporter.h>
#include <drc/drc_rule.h>
#include <drc/drc_engine.h>
#include <pcb_edit_frame.h>
#include <rc_item.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>

class CONNECTIVITY_DATA;
class FOOTPRINT_DIFF_WIDGET;


/**
 * Tool for pcb inspection.
 */
class BOARD_INSPECTION_TOOL : public wxEvtHandler, public PCB_TOOL_BASE
{
public:
    BOARD_INSPECTION_TOOL();

    /// @copydoc TOOL_INTERACTIVE::Init()
    bool Init() override;

    /// @copydoc TOOL_INTERACTIVE::Reset()
    void Reset( RESET_REASON aReason ) override;

    /**
     * Show dialog with board statistics.
     */
    int ShowBoardStatistics( const TOOL_EVENT& aEvent );

    ///< Highlight net belonging to the item under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///< Clear all board highlights
    int ClearHighlight( const TOOL_EVENT& aEvent );

    ///< Perform the appropriate action in response to an Eeschema cross-probe.
    int HighlightItem( const TOOL_EVENT& aEvent );

    ///< Update ratsnest for selected items.
    int UpdateLocalRatsnest( const TOOL_EVENT& aEvent );

    ///< Hide ratsnest for selected items. Called when there are no items selected.
    int HideLocalRatsnest( const TOOL_EVENT& aEvent );

    ///< Show local ratsnest of a component.
    int LocalRatsnestTool( const TOOL_EVENT& aEvent );

    ///< Hide the ratsnest for a given net.
    int HideNetInRatsnest( const TOOL_EVENT& aEvent );

    ///< Show the ratsnest for a given net.
    int ShowNetInRatsnest( const TOOL_EVENT& aEvent );

    wxString InspectDRCErrorMenuText( const std::shared_ptr<RC_ITEM>& aDRCItem );
    void InspectDRCError( const std::shared_ptr<RC_ITEM>& aDRCItem );

    ///< Show the clearance resolution for two selected items.
    int InspectClearance( const TOOL_EVENT& aEvent );

    int InspectConstraints( const TOOL_EVENT& aEvent );

    int ShowFootprintLinks( const TOOL_EVENT& aEvent );

    int DiffFootprint( const TOOL_EVENT& aEvent );
    void DiffFootprint( FOOTPRINT* aFootprint, wxTopLevelWindow* aReparentTo = nullptr );

    /**
     * @return true if a net or nets to highlight have been set
     */
    bool IsNetHighlightSet() const
    {
        return !m_currentlyHighlighted.empty();
    }

private:
    ///< Recalculate dynamic ratsnest for the current selection.
    void calculateSelectionRatsnest( const VECTOR2I& aDelta );

    /**
     * Look for a #BOARD_CONNECTED_ITEM in a given spot and if one is found - it enables
     * highlight for its net.
     *
     * @param aPosition is the point where an item is expected (world coordinates).
     * @param aUseSelection is true if we should use the current selection to pick the netcode
     */
    bool highlightNet( const VECTOR2D& aPosition, bool aUseSelection );

    void doHideRatsnestNet( int aNetCode, bool aHide );

    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

    std::unique_ptr<DRC_ENGINE> makeDRCEngine( bool* aCompileError, bool* aCourtyardError );

    wxString getItemDescription( BOARD_ITEM* aItem );

    void reportCompileError( REPORTER* r );
    void reportHeader( const wxString& aTitle, BOARD_ITEM* a, REPORTER* r );
    void reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b, REPORTER* r );
    void reportHeader( const wxString& aTitle, BOARD_ITEM* a, BOARD_ITEM* b, PCB_LAYER_ID aLayer,
                       REPORTER* r );

    FOOTPRINT_DIFF_WIDGET* constructDiffPanel( wxPanel* aParentPanel );

private:
    PCB_EDIT_FRAME*     m_frame;    // Pointer to the currently used edit frame.

    std::set<int>       m_currentlyHighlighted; // Active net being highlighted, or -1 when off
    std::set<int>       m_lastHighlighted;      // For toggling between last two highlighted nets

    CONNECTIVITY_DATA*  m_dynamicData;      // Cached connectivity data from the selection
};

#endif //BOARD_INSPECTION_TOOL_H

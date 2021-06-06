/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/dialog_board_statistics.h>
#include <dialogs/dialog_net_inspector.h>
#include <dialogs/dialog_HTML_reporter_base.h>
#include <dialogs/dialog_constraints_reporter.h>
#include <drc/drc_rule.h>
#include <pcb_edit_frame.h>
#include <rc_item.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_tool_base.h>

class CONNECTIVITY_DATA;


class DIALOG_INSPECTION_REPORTER : public DIALOG_HTML_REPORTER
{
public:
    DIALOG_INSPECTION_REPORTER( PCB_EDIT_FRAME* aFrame ) :
            DIALOG_HTML_REPORTER( aFrame ),
            m_frame( aFrame )
    {
        m_sdbSizerOK->SetDefault();
        SetInitialFocus( m_sdbSizerOK );
    }

    void OnErrorLinkClicked( wxHtmlLinkEvent& event ) override;

    void OnOK( wxCommandEvent& event ) override
    {
        Close();
    }

protected:
    PCB_EDIT_FRAME* m_frame;
};


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
    int ShowStatisticsDialog( const TOOL_EVENT& aEvent );

    ///< Notify Eeschema about the selected item.
    int CrossProbePcbToSch( const TOOL_EVENT& aEvent );

    ///< Highlight net belonging to the item under the cursor.
    int HighlightNet( const TOOL_EVENT& aEvent );

    ///< Clear all board highlights
    int ClearHighlight( const TOOL_EVENT& aEvent );

    ///< Launch a tool to pick the item whose net is going to be highlighted.
    int HighlightNetTool( const TOOL_EVENT& aEvent );

    ///< Perform the appropriate action in response to an Eeschema cross-probe.
    int HighlightItem( const TOOL_EVENT& aEvent );

    ///< Update ratsnest for selected items.
    int UpdateSelectionRatsnest( const TOOL_EVENT& aEvent );

    ///< Hide ratsnest for selected items. Called when there are no items selected.
    int HideDynamicRatsnest( const TOOL_EVENT& aEvent );

    ///< Show local ratsnest of a component.
    int LocalRatsnestTool( const TOOL_EVENT& aEvent );

    int FlipPcbView( const TOOL_EVENT& aEvent );

    int ListNets( const TOOL_EVENT& aEvent );

    ///< Hide the ratsnest for a given net.
    int HideNet( const TOOL_EVENT& aEvent );

    ///< Show the ratsnest for a given net.
    int ShowNet( const TOOL_EVENT& aEvent );

    void InspectDRCError( const std::shared_ptr<RC_ITEM>& aDRCItem );

    ///< Show the clearance resolution for two selected items.
    int InspectClearance( const TOOL_EVENT& aEvent );

    int InspectConstraints( const TOOL_EVENT& aEvent );

    /**
     * @return true if a net or nets to highlight have been set
     */
    bool IsNetHighlightSet() const
    {
        return !m_currentlyHighlighted.empty();
    }

private:
    ///< Event handler to recalculate dynamic ratsnest.
    void ratsnestTimer( wxTimerEvent& aEvent );

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

    void doHideNet( int aNetCode, bool aHide );

    ///< Bind handlers to corresponding TOOL_ACTIONs.
    void setTransitions() override;

    void onListNetsDialogClosed( wxCommandEvent& aEvent );
    void onInspectClearanceDialogClosed( wxCommandEvent& aEvent );
    void onInspectConstraintsDialogClosed( wxCommandEvent& aEvent );

    void reportZoneConnection( ZONE* aZone, PAD* aPad, REPORTER* r );

    void reportClearance( DRC_CONSTRAINT_T aClearanceType, PCB_LAYER_ID aLayer, BOARD_ITEM* aA,
                          BOARD_ITEM* aB, REPORTER* r );

    wxString getItemDescription( BOARD_ITEM* aItem );

private:
    PCB_EDIT_FRAME* m_frame;    // Pointer to the currently used edit frame.

    bool m_probingSchToPcb;     // Recursion guard when cross-probing to Eeschema
    std::set<int> m_currentlyHighlighted; // Active net being highlighted, or -1 when off
    std::set<int> m_lastHighlighted;      // Used for toggling between last two highlighted nets

    CONNECTIVITY_DATA* m_dynamicData;      // Cached connectivity data from the selection

    std::unique_ptr<DIALOG_NET_INSPECTOR> m_listNetsDialog;
    DIALOG_NET_INSPECTOR::SETTINGS        m_listNetsDialogSettings;

    std::unique_ptr<DIALOG_INSPECTION_REPORTER>  m_inspectClearanceDialog;
    std::unique_ptr<DIALOG_CONSTRAINTS_REPORTER> m_inspectConstraintsDialog;
};

#endif //BOARD_INSPECTION_TOOL_H

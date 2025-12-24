/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <functional>
#include <algorithm>
#include <limits>
#include <kiplatform/ui.h>
#include <board.h>
#include <board_commit.h>
#include <gal/graphics_abstraction_layer.h>
#include <geometry/geometry_utils.h>
#include <pad.h>
#include <pcb_group.h>
#include <pcb_generator.h>
#include <pcb_edit_frame.h>
#include <spread_footprints.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <tools/edit_tool.h>
#include <tools/pcb_grid_helper.h>
#include <tools/drc_tool.h>
#include <tools/zone_filler_tool.h>
#include <router/router_tool.h>
#include <dialogs/dialog_move_exact.h>
#include <zone_filler.h>
#include <drc/drc_engine.h>
#include <drc/drc_interactive_courtyard_clearance.h>
#include <view/view_controls.h>

#include <connectivity/connectivity_data.h>
#include <wx/richmsgdlg.h>
#include <wx/choicdlg.h>
#include <unordered_set>
#include <unordered_map>


static bool PromptConnectedPadDecision( PCB_BASE_EDIT_FRAME* aFrame, const std::vector<PAD*>& aPads,
                                        const wxString& aDialogTitle, bool& aIncludeConnectedPads )
{
    if( aPads.empty() )
    {
        aIncludeConnectedPads = true;
        return true;
    }

    std::unordered_set<PAD*> uniquePads( aPads.begin(), aPads.end() );

    wxString msg;
    msg.Printf( _( "%zu unselected pad(s) are connected to these nets. How do you want to proceed?" ),
                uniquePads.size() );

    wxString details;
    details << _( "Connected tracks, vias, and other non-zone copper items will still swap nets"
                  " even if you ignore the unselected pads." )
            << "\n \n" // Add space so GTK doesn't eat the newlines
            << _( "Unselected pads:" ) << '\n';

    for( PAD* pad : uniquePads )
    {
        const FOOTPRINT* fp = pad->GetParentFootprint();
        details << wxS( "  • " ) << ( fp ? fp->GetReference() : _( "<no reference designator>" ) ) << wxS( ":" )
                << pad->GetNumber() << '\n';
    }


    wxRichMessageDialog dlg( aFrame, msg, aDialogTitle, wxYES_NO | wxCANCEL | wxYES_DEFAULT | wxICON_WARNING );
    dlg.SetYesNoLabels( _( "Ignore Unselected Pads" ), _( "Swap All Connected Pads" ) );
    dlg.SetExtendedMessage( details );

    int ret = dlg.ShowModal();

    if( ret == wxID_CANCEL )
        return false;

    aIncludeConnectedPads = ( ret == wxID_NO );
    return true;
}


int EDIT_TOOL::Swap( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );

                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( item->Type() == PCB_TRACE_T )
                        aCollector.Remove( item );
                }

                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    if( selection.Size() < 2 )
        return 0;

    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    std::vector<EDA_ITEM*> sorted = selection.GetItemsSortedBySelectionOrder();

    // Save items, so changes can be undone
    for( EDA_ITEM* item : selection )
        commit->Modify( item, nullptr, RECURSE_MODE::RECURSE );

    for( size_t i = 0; i < sorted.size() - 1; i++ )
    {
        EDA_ITEM* edaItemA = sorted[i];
        EDA_ITEM* edaItemB = sorted[( i + 1 ) % sorted.size()];

        if( !edaItemA->IsBOARD_ITEM() || !edaItemB->IsBOARD_ITEM() )
            continue;

        BOARD_ITEM* a = static_cast<BOARD_ITEM*>( edaItemA );
        BOARD_ITEM* b = static_cast<BOARD_ITEM*>( edaItemB );

        // Swap X,Y position
        VECTOR2I aPos = a->GetPosition(), bPos = b->GetPosition();
        std::swap( aPos, bPos );
        a->SetPosition( aPos );
        b->SetPosition( bPos );

        // Handle footprints specially. They can be flipped to the back of the board which
        // requires a special transformation.
        if( a->Type() == PCB_FOOTPRINT_T && b->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* aFP = static_cast<FOOTPRINT*>( a );
            FOOTPRINT* bFP = static_cast<FOOTPRINT*>( b );

            // Store initial orientation of footprints, before flipping them.
            EDA_ANGLE aAngle = aFP->GetOrientation();
            EDA_ANGLE bAngle = bFP->GetOrientation();

            // Flip both if needed
            if( aFP->IsFlipped() != bFP->IsFlipped() )
            {
                aFP->Flip( aPos, FLIP_DIRECTION::TOP_BOTTOM );
                bFP->Flip( bPos, FLIP_DIRECTION::TOP_BOTTOM );
            }

            // Set orientation
            std::swap( aAngle, bAngle );
            aFP->SetOrientation( aAngle );
            bFP->SetOrientation( bAngle );
        }
        // We can also do a layer swap safely for two objects of the same type,
        // except groups which don't support layer swaps.
        else if( a->Type() == b->Type() && a->Type() != PCB_GROUP_T )
        {
            // Swap layers
            PCB_LAYER_ID aLayer = a->GetLayer(), bLayer = b->GetLayer();
            std::swap( aLayer, bLayer );
            a->SetLayer( aLayer );
            b->SetLayer( bLayer );
        }
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Swap" ) );

    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    return 0;
}


int EDIT_TOOL::SwapPadNets( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    PCB_SELECTION& selection = m_selectionTool->RequestSelection( &EDIT_TOOL::PadFilter );

    if( selection.Size() < 2 || !selection.OnlyContains( { PCB_PAD_T } ) )
        return 0;

    // Get selected pads in selection order, because swapping is cyclic and we let the user pick
    // the rotation order
    std::vector<EDA_ITEM*> orderedPads = selection.GetItemsSortedBySelectionOrder();
    std::vector<PAD*>      pads;
    const size_t           padsCount = orderedPads.size();

    for( EDA_ITEM* it : orderedPads )
        pads.push_back( static_cast<PAD*>( static_cast<BOARD_ITEM*>( it ) ) );

    // Record original nets and build selected set for quick membership tests
    std::vector<int>         originalNets( padsCount );
    std::unordered_set<PAD*> selectedPads;

    for( size_t i = 0; i < padsCount; ++i )
    {
        originalNets[i] = pads[i]->GetNetCode();
        selectedPads.insert( pads[i] );
    }

    // If all nets are the same, nothing to do
    bool allSame = true;

    for( size_t i = 1; i < padsCount; ++i )
    {
        if( originalNets[i] != originalNets[0] )
        {
            allSame = false;
            break;
        }
    }

    if( allSame )
        return 0;

    // Desired new nets are a cyclic rotation of original nets (like Swap positions)
    auto newNetForIndex =
            [&]( size_t i )
            {
                return originalNets[( i + 1 ) % padsCount];
            };

    // Take an event commit since we will eventually support this while actively routing the board
    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    // Connectivity to find items connected to each pad
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();

    // Accumulate changes: for each item, assign the resulting new net
    std::unordered_map<BOARD_CONNECTED_ITEM*, int> itemNewNets;
    std::vector<PAD*>                              nonSelectedPadsToChange;

    for( size_t i = 0; i < padsCount; ++i )
    {
        PAD* pad = pads[i];
        int  fromNet = originalNets[i];
        int  toNet = newNetForIndex( i );

        // For each connected item, if it matches fromNet, schedule it for toNet
        for( BOARD_CONNECTED_ITEM* ci : connectivity->GetConnectedItems( pad, 0 ) )
        {
            switch( ci->Type() )
            {
            case PCB_TRACE_T:
            case PCB_ARC_T:
            case PCB_VIA_T:
            case PCB_PAD_T:
                break;
            // Exclude zones, user probably doesn't want to change zone nets
            default:
                continue;
            }

            if( ci->GetNetCode() != fromNet )
                continue;

            // Track conflicts: if already assigned a different new net, just overwrite (last wins)
            itemNewNets[ci] = toNet;

            if( ci->Type() == PCB_PAD_T )
            {
                PAD* otherPad = static_cast<PAD*>( ci );

                if( !selectedPads.count( otherPad ) )
                    nonSelectedPadsToChange.push_back( otherPad );
            }
        }
    }

    bool includeConnectedPads = true;

    if( !PromptConnectedPadDecision( frame(), nonSelectedPadsToChange, _( "Swap Pad Nets" ), includeConnectedPads ) )
        return 0;

    // Apply changes
    // 1) Selected pads get their new nets directly
    for( size_t i = 0; i < padsCount; ++i )
    {
        commit->Modify( pads[i] );
        pads[i]->SetNetCode( newNetForIndex( i ) );
    }

    // 2) Connected items propagate, depending on user choice
    for( const auto& itemNewNet : itemNewNets )
    {
        BOARD_CONNECTED_ITEM* item = itemNewNet.first;
        int                   newNet = itemNewNet.second;

        if( item->Type() == PCB_PAD_T )
        {
            PAD* p = static_cast<PAD*>( item );

            if( selectedPads.count( p ) )
                continue; // already changed above

            if( !includeConnectedPads )
                continue; // skip non-selected pads if requested
        }

        commit->Modify( item );
        item->SetNetCode( newNet );
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Swap Pad Nets" ) );

    // Ensure connectivity visuals update
    rebuildConnectivity();
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    return 0;
}


int EDIT_TOOL::SwapGateNets( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() )
    {
        wxBell();
        return 0;
    }

    auto showError =
            [this]()
            {
                frame()->ShowInfoBarError( _( "Gate swapping must be performed on pads within one multi-gate "
                                              "footprint." ) );
            };

    PCB_SELECTION& selection = m_selectionTool->RequestSelection( &EDIT_TOOL::PadFilter );

    // Get our sanity checks out of the way to clean up later loops
    FOOTPRINT* targetFp = nullptr;
    bool       fail = false;

    for( EDA_ITEM* it : selection )
    {
        // This shouldn't happen due to the filter, but just in case
        if( it->Type() != PCB_PAD_T )
        {
            fail = true;
            break;
        }

        FOOTPRINT* fp = static_cast<PAD*>( static_cast<BOARD_ITEM*>( it ) )->GetParentFootprint();

        if( !targetFp )
        {
            targetFp = fp;
        }
        else if( fp && targetFp != fp )
        {
            fail = true;
            break;
        }
    }

    if( fail || !targetFp || targetFp->GetUnitInfo().size() < 2 )
    {
        showError();
        return 0;
    }


    const auto& units = targetFp->GetUnitInfo();

    // Collect unit hits and ordered unit list based on selection order
    std::vector<bool> unitHit( units.size(), false );
    std::vector<int>  unitOrder;

    std::vector<EDA_ITEM*> orderedPads = selection.GetItemsSortedBySelectionOrder();

    for( EDA_ITEM* it : orderedPads )
    {
        PAD* pad = static_cast<PAD*>( static_cast<BOARD_ITEM*>( it ) );

        const wxString& padNum = pad->GetNumber();
        int             unitIdx = -1;

        for( size_t i = 0; i < units.size(); ++i )
        {
            for( const auto& p : units[i].m_pins )
            {
                if( p == padNum )
                {
                    unitIdx = static_cast<int>( i );

                    if( !unitHit[i] )
                        unitOrder.push_back( unitIdx );

                    unitHit[i] = true;
                    break;
                }
            }

            if( unitIdx >= 0 )
                break;
        }
    }

    // Determine active units from selection order: 0 -> bail, 1 -> single-unit flow, 2+ -> cycle
    std::vector<int> activeUnitIdx;
    int              sourceIdx = -1;

    if( unitOrder.size() >= 2 )
    {
        activeUnitIdx = unitOrder;
        sourceIdx = unitOrder.front();
    }
    // If we only have one gate selected, we must have a target unit name parameter to proceed
    else if( unitOrder.size() == 1 && aEvent.HasParameter() )
    {
        sourceIdx = unitOrder.front();
        wxString targetUnitByName = aEvent.Parameter<wxString>();

        int targetIdx = -1;

        for( size_t i = 0; i < units.size(); ++i )
        {
            if( static_cast<int>( i ) == sourceIdx )
                continue;

            if( units[i].m_pins.size() == units[sourceIdx].m_pins.size() && units[i].m_unitName == targetUnitByName )
                targetIdx = static_cast<int>( i );
        }

        if( targetIdx < 0 )
        {
            showError();
            return 0;
        }

        activeUnitIdx.push_back( sourceIdx );
        activeUnitIdx.push_back( targetIdx );
    }
    else
    {
        showError();
        return 0;
    }

    // Verify equal pin counts across all active units
    const size_t pinCount = units[activeUnitIdx.front()].m_pins.size();

    for( int idx : activeUnitIdx )
    {
        if( units[idx].m_pins.size() != pinCount )
        {
            frame()->ShowInfoBarError( _( "Gate swapping must be performed on gates with equal pin counts." ) );
            return 0;
        }
    }

    // Build per-unit pad arrays and net vectors
    const size_t                   unitCount = activeUnitIdx.size();
    std::vector<std::vector<PAD*>> unitPads( unitCount );
    std::vector<std::vector<int>>  unitNets( unitCount );

    for( size_t ui = 0; ui < unitCount; ++ui )
    {
        int         uidx = activeUnitIdx[ui];
        const auto& pins = units[uidx].m_pins;

        for( size_t pi = 0; pi < pinCount; ++pi )
        {
            PAD* p = targetFp->FindPadByNumber( pins[pi] );

            if( !p )
            {
                frame()->ShowInfoBarError( _( "Gate swapping failed: pad in unit missing from footprint." ) );
                return 0;
            }

            unitPads[ui].push_back( p );
            unitNets[ui].push_back( p->GetNetCode() );
        }
    }

    // If all unit nets match across positions, nothing to do
    bool allSame = true;

    for( size_t pi = 0; pi < pinCount && allSame; ++pi )
    {
        int refNet = unitNets[0][pi];

        for( size_t ui = 1; ui < unitCount; ++ui )
        {
            if( unitNets[ui][pi] != refNet )
            {
                allSame = false;
                break;
            }
        }
    }

    if( allSame )
    {
        frame()->ShowInfoBarError( _( "Gate swapping has no effect: all selected gates have identical nets." ) );
        return 0;
    }

    // TODO: someday support swapping while routing and take that commit
    BOARD_COMMIT  localCommit( this );
    BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() );

    if( !commit )
        commit = &localCommit;

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board()->GetConnectivity();

    // Accumulate changes: item -> new net
    std::unordered_map<BOARD_CONNECTED_ITEM*, int> itemNewNets;
    std::vector<PAD*>                              nonSelectedPadsToChange;

    // Selected pads in the swap (for suppressing re-adding in connected pad handling)
    std::unordered_set<PAD*> swapPads;

    for( const auto& v : unitPads )
        swapPads.insert( v.begin(), v.end() );

    // Schedule net swaps for connectivity-attached items
    auto scheduleForPad = [&]( PAD* pad, int fromNet, int toNet )
        {
            for( BOARD_CONNECTED_ITEM* ci : connectivity->GetConnectedItems( pad, 0 ) )
            {
                switch( ci->Type() )
                {
                case PCB_TRACE_T:
                case PCB_ARC_T:
                case PCB_VIA_T:
                case PCB_PAD_T:
                    break;

                default:
                    continue;
                }

                if( ci->GetNetCode() != fromNet )
                    continue;

                itemNewNets[ ci ] = toNet;

                if( ci->Type() == PCB_PAD_T )
                {
                    PAD* other = static_cast<PAD*>( ci );

                    if( !swapPads.count( other ) )
                        nonSelectedPadsToChange.push_back( other );
                }
            }
        };

    // For each position, rotate nets among units forward
    for( size_t pi = 0; pi < pinCount; ++pi )
    {
        for( size_t ui = 0; ui < unitCount; ++ui )
        {
            size_t fromIdx = ui;
            size_t toIdx = ( ui + 1 ) % unitCount;

            PAD* padFrom = unitPads[fromIdx][pi];
            int  fromNet = unitNets[fromIdx][pi];
            int  toNet = unitNets[toIdx][pi];

            scheduleForPad( padFrom, fromNet, toNet );
        }
    }

    bool includeConnectedPads = true;

    if( !PromptConnectedPadDecision( frame(), nonSelectedPadsToChange, _( "Swap Gate Nets" ), includeConnectedPads ) )
    {
        return 0;
    }

    // Apply pad net swaps: rotate per position
    for( size_t pi = 0; pi < pinCount; ++pi )
    {
        // First write back nets for each unit's pad at this position
        for( size_t ui = 0; ui < unitCount; ++ui )
        {
            size_t toIdx = ( ui + 1 ) % unitCount;
            PAD*   pad = unitPads[ui][pi];
            int    newNet = unitNets[toIdx][pi];

            commit->Modify( pad );
            pad->SetNetCode( newNet );
        }
    }

    // Apply connected items
    for( const auto& kv : itemNewNets )
    {
        BOARD_CONNECTED_ITEM* item = kv.first;
        int                   newNet = kv.second;

        if( item->Type() == PCB_PAD_T )
        {
            PAD* p = static_cast<PAD*>( item );

            if( swapPads.count( p ) )
                continue;

            if( !includeConnectedPads )
                continue;
        }

        commit->Modify( item );
        item->SetNetCode( newNet );
    }

    if( !localCommit.Empty() )
        localCommit.Push( _( "Swap Gate Nets" ) );

    rebuildConnectivity();
    m_toolMgr->ProcessEvent( EVENTS::SelectedItemsModified );

    return 0;
}


int EDIT_TOOL::PackAndMoveFootprints( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() || m_dragging )
    {
        wxBell();
        return 0;
    }

    BOARD_COMMIT   commit( this );
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector, true );

                // Iterate from the back so we don't have to worry about removals.
                for( int i = aCollector.GetCount() - 1; i >= 0; --i )
                {
                    BOARD_ITEM* item = aCollector[i];

                    if( !dynamic_cast<FOOTPRINT*>( item ) )
                        aCollector.Remove( item );
                }

                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    std::vector<FOOTPRINT*> footprintsToPack;

    for( EDA_ITEM* item : selection )
        footprintsToPack.push_back( static_cast<FOOTPRINT*>( item ) );

    if( footprintsToPack.empty() )
        return 0;

    BOX2I footprintsBbox;

    for( FOOTPRINT* fp : footprintsToPack )
    {
        commit.Modify( fp );
        fp->SetFlags( IS_MOVING );
        footprintsBbox.Merge( fp->GetBoundingBox( false ) );
    }

    SpreadFootprints( &footprintsToPack, footprintsBbox.Normalize().GetOrigin(), false );

    if( doMoveSelection( aEvent, &commit, true ) )
        commit.Push( _( "Pack Footprints" ) );
    else
        commit.Revert();

    return 0;
}


int EDIT_TOOL::Move( const TOOL_EVENT& aEvent )
{
    if( isRouterActive() || m_dragging )
    {
        wxBell();
        return 0;
    }

    if( BOARD_COMMIT* commit = dynamic_cast<BOARD_COMMIT*>( aEvent.Commit() ) )
    {
        // Most moves will be synchronous unless they are coming from the API
        if( aEvent.SynchronousState() )
            aEvent.SynchronousState()->store( STS_RUNNING );

        if( doMoveSelection( aEvent, commit, true ) )
        {
            if( aEvent.SynchronousState() )
                aEvent.SynchronousState()->store( STS_FINISHED );
        }
        else if( aEvent.SynchronousState() )
        {
            aEvent.SynchronousState()->store( STS_CANCELLED );
        }
    }
    else
    {
        BOARD_COMMIT localCommit( this );

        if( doMoveSelection( aEvent, &localCommit, false ) )
            localCommit.Push( _( "Move" ) );
        else
            localCommit.Revert();
    }

    // Notify point editor.  (While doMoveSelection() will re-select the items and post this
    // event, it's done before the edit flags are cleared in BOARD_COMMIT::Push() so the point
    // editor doesn't fire up.)
    m_toolMgr->ProcessEvent( EVENTS::SelectedEvent );

    return 0;
}


VECTOR2I EDIT_TOOL::getSafeMovement( const VECTOR2I& aMovement, const BOX2I& aSourceBBox,
                                     const VECTOR2D& aBBoxOffset )
{
    typedef std::numeric_limits<int> coord_limits;

    static const double max = coord_limits::max() - (int) COORDS_PADDING;
    static const double min = -max;

    BOX2D testBox( aSourceBBox.GetPosition(), aSourceBBox.GetSize() );
    testBox.Offset( aBBoxOffset );

    // Do not restrict movement if bounding box is already out of bounds
    if( testBox.GetLeft() < min || testBox.GetTop() < min || testBox.GetRight() > max
        || testBox.GetBottom() > max )
    {
        return aMovement;
    }

    testBox.Offset( aMovement );

    if( testBox.GetLeft() < min )
        testBox.Offset( min - testBox.GetLeft(), 0 );

    if( max < testBox.GetRight() )
        testBox.Offset( -( testBox.GetRight() - max ), 0 );

    if( testBox.GetTop() < min )
        testBox.Offset( 0, min - testBox.GetTop() );

    if( max < testBox.GetBottom() )
        testBox.Offset( 0, -( testBox.GetBottom() - max ) );

    return KiROUND( testBox.GetPosition() - aBBoxOffset - aSourceBBox.GetPosition() );
}


bool EDIT_TOOL::doMoveSelection( const TOOL_EVENT& aEvent, BOARD_COMMIT* aCommit, bool aAutoStart )
{
    const bool moveWithReference = aEvent.IsAction( &PCB_ACTIONS::moveWithReference );
    const bool moveIndividually = aEvent.IsAction( &PCB_ACTIONS::moveIndividually );

    PCB_BASE_EDIT_FRAME*               editFrame = getEditFrame<PCB_BASE_EDIT_FRAME>();
    PCBNEW_SETTINGS*                   cfg = editFrame->GetPcbNewSettings();
    BOARD*                             board = editFrame->GetBoard();
    KIGFX::VIEW_CONTROLS*              controls = getViewControls();
    VECTOR2I                           originalCursorPos = controls->GetCursorPosition();
    VECTOR2I                           originalMousePos = controls->GetMousePosition();
    std::unique_ptr<STATUS_TEXT_POPUP> statusPopup;
    size_t                             itemIdx = 0;

    // Be sure that there is at least one item that we can modify. If nothing was selected before,
    // try looking for the stuff under mouse cursor (i.e. KiCad old-style hover selection)
    PCB_SELECTION& selection = m_selectionTool->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
                sTool->FilterCollectorForMarkers( aCollector );
                sTool->FilterCollectorForHierarchy( aCollector, true );
                sTool->FilterCollectorForFreePads( aCollector );
                sTool->FilterCollectorForTableCells( aCollector );
                sTool->FilterCollectorForLockedItems( aCollector );
            } );

    if( m_dragging || selection.Empty() )
        return false;

    TOOL_EVENT pushedEvent = aEvent;
    editFrame->PushTool( aEvent );
    Activate();

    // Must be done after Activate() so that it gets set into the correct context
    controls->ShowCursor( true );
    controls->SetAutoPan( true );
    controls->ForceCursorPosition( false );

    auto displayConstraintsMessage =
            [editFrame]( LEADER_MODE aMode )
            {
                wxString msg;

                switch( aMode )
                {
                case LEADER_MODE::DEG45:
                    msg = _( "Angle snap lines: 45°" );
                    break;

                case LEADER_MODE::DEG90:
                    msg = _( "Angle snap lines: 90°" );
                    break;

                default:
                    msg.clear();
                    break;
                }

                editFrame->DisplayConstraintsMsg( msg );
            };

    auto updateStatusPopup =
            [&]( EDA_ITEM* item, size_t ii, size_t count )
            {
                wxString popuptext = _( "Click to place %s (item %zu of %zu)\n"
                                     "Press <esc> to cancel all; double-click to finish" );
                wxString msg;

                if( item->Type() == PCB_FOOTPRINT_T )
                {
                    FOOTPRINT* fp = static_cast<FOOTPRINT*>( item );
                    msg = fp->GetReference();
                }
                else if( item->Type() == PCB_PAD_T )
                {
                    PAD*       pad = static_cast<PAD*>( item );
                    FOOTPRINT* fp = pad->GetParentFootprint();
                    msg = wxString::Format( _( "%s pad %s" ), fp->GetReference(), pad->GetNumber() );
                }
                else
                {
                    msg = item->GetTypeDesc().Lower();
                }

                if( !statusPopup )
                    statusPopup = std::make_unique<STATUS_TEXT_POPUP>( frame() );

                statusPopup->SetText( wxString::Format( popuptext, msg, ii, count ) );
            };

    std::vector<BOARD_ITEM*> sel_items;         // All the items operated on by the move below
    std::vector<BOARD_ITEM*> orig_items;        // All the original items in the selection

    for( EDA_ITEM* item : selection )
    {
        if( item->IsBOARD_ITEM() )
        {
            BOARD_ITEM* boardItem = static_cast<BOARD_ITEM*>( item );

            if( !selection.IsHover() )
                orig_items.push_back( boardItem );

            sel_items.push_back( boardItem );
        }

        if( item->Type() == PCB_FOOTPRINT_T )
        {
            FOOTPRINT* footprint = static_cast<FOOTPRINT*>( item );

            for( PAD* pad : footprint->Pads() )
                sel_items.push_back( pad );

            // Clear this flag here; it will be set by the netlist updater if the footprint is new
            // so that it was skipped in the initial connectivity update in OnNetlistChanged
            footprint->SetAttributes( footprint->GetAttributes() & ~FP_JUST_ADDED );
        }
    }

    VECTOR2I pickedReferencePoint;

    if( moveWithReference && !pickReferencePoint( _( "Select reference point for move..." ), "", "",
                                                  pickedReferencePoint ) )
    {
        if( selection.IsHover() )
            m_toolMgr->RunAction( ACTIONS::selectionClear );

        editFrame->PopTool( pushedEvent );
        return false;
    }

    if( moveIndividually )
    {
        orig_items.clear();

        for( EDA_ITEM* item : selection.GetItemsSortedBySelectionOrder() )
        {
            if( item->IsBOARD_ITEM() )
                orig_items.push_back( static_cast<BOARD_ITEM*>( item ) );
        }

        updateStatusPopup( orig_items[ itemIdx ], itemIdx + 1, orig_items.size() );
        statusPopup->Popup();
        statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );
        canvas()->SetStatusPopup( statusPopup->GetPanel() );

        m_selectionTool->ClearSelection();
        m_selectionTool->AddItemToSel( orig_items[ itemIdx ] );

        sel_items.clear();
        sel_items.push_back( orig_items[ itemIdx ] );
    }

    bool            restore_state = false;
    VECTOR2I        originalPos = originalCursorPos;  // Initialize to current cursor position
    VECTOR2D        bboxMovement;
    BOX2I           originalBBox;
    bool            updateBBox = true;
    LSET            layers( { editFrame->GetActiveLayer() } );
    PCB_GRID_HELPER grid( m_toolMgr, editFrame->GetMagneticItemsSettings() );
    TOOL_EVENT      copy = aEvent;
    TOOL_EVENT*     evt = &copy;
    VECTOR2I        prevPos;
    bool            enableLocalRatsnest = true;

    LEADER_MODE angleSnapMode = GetAngleSnapMode();
    bool eatFirstMouseUp = true;
    bool allowRedraw3D   = cfg->m_Display.m_Live3DRefresh;
    bool showCourtyardConflicts = !m_isFootprintEditor && cfg->m_ShowCourtyardCollisions;

    // Axis locking for arrow key movement
    enum class AXIS_LOCK { NONE, HORIZONTAL, VERTICAL };
    AXIS_LOCK axisLock = AXIS_LOCK::NONE;
    long      lastArrowKeyAction = 0;

    // Used to test courtyard overlaps
    std::unique_ptr<DRC_INTERACTIVE_COURTYARD_CLEARANCE> drc_on_move = nullptr;

    if( showCourtyardConflicts )
    {
        std::shared_ptr<DRC_ENGINE> drcEngine = m_toolMgr->GetTool<DRC_TOOL>()->GetDRCEngine();
        drc_on_move.reset( new DRC_INTERACTIVE_COURTYARD_CLEARANCE( drcEngine ) );
        drc_on_move->Init( board );
    }

    auto configureAngleSnap =
            [&]( LEADER_MODE aMode )
            {
                std::vector<VECTOR2I> directions;

                switch( aMode )
                {
                case LEADER_MODE::DEG45:
                    directions = { VECTOR2I( 1, 0 ), VECTOR2I( 0, 1 ), VECTOR2I( 1, 1 ), VECTOR2I( 1, -1 ) };
                    break;

                case LEADER_MODE::DEG90:
                    directions = { VECTOR2I( 1, 0 ), VECTOR2I( 0, 1 ) };
                    break;

                default:
                    break;
                }

                grid.SetSnapLineDirections( directions );

                if( directions.empty() )
                {
                    grid.ClearSnapLine();
                }
                else
                {
                    grid.SetSnapLineOrigin( originalPos );
                }
            };

    configureAngleSnap( angleSnapMode );
    displayConstraintsMessage( angleSnapMode );

    // Prime the pump
    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    // Main loop: keep receiving events
    do
    {
        VECTOR2I movement;
        editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::MOVING );
        grid.SetSnap( !evt->Modifier( MD_SHIFT ) );
        grid.SetUseGrid( getView()->GetGAL()->GetGridSnapping() && !evt->DisableGridSnapping() );

        bool isSkip = evt->IsAction( &PCB_ACTIONS::skip ) && moveIndividually;

        if( evt->IsMotion() || evt->IsDrag( BUT_LEFT ) )
            eatFirstMouseUp = false;

        if(   evt->IsAction( &PCB_ACTIONS::move )
           || evt->IsMotion()
           || evt->IsDrag( BUT_LEFT )
           || evt->IsAction( &ACTIONS::refreshPreview )
           || evt->IsAction( &PCB_ACTIONS::moveWithReference )
           || evt->IsAction( &PCB_ACTIONS::moveIndividually ) )
        {
            if( m_dragging && (   evt->IsMotion()
                               || evt->IsDrag( BUT_LEFT )
                               || evt->IsAction( &ACTIONS::refreshPreview ) ) )
            {
                bool redraw3D = false;

                GRID_HELPER_GRIDS selectionGrid = grid.GetSelectionGrid( selection );

                // We need to bypass refreshPreview action here because it is triggered by the move,
                // so we were getting double-key events that toggled the axis locking if you
                // pressed them in a certain order.
                if( controls->GetSettings().m_lastKeyboardCursorPositionValid && ! evt->IsAction( &ACTIONS::refreshPreview ) )
                {
                    VECTOR2I keyboardPos( controls->GetSettings().m_lastKeyboardCursorPosition );
                    long action = controls->GetSettings().m_lastKeyboardCursorCommand;

                    grid.SetSnap( false );
                    m_cursor = grid.Align( keyboardPos, selectionGrid );

                    // Update axis lock based on arrow key press
                    if( action == ACTIONS::CURSOR_LEFT || action == ACTIONS::CURSOR_RIGHT )
                    {
                        if( axisLock == AXIS_LOCK::HORIZONTAL )
                        {
                            // Check if opposite horizontal key pressed to unlock
                            if( ( lastArrowKeyAction == ACTIONS::CURSOR_LEFT && action == ACTIONS::CURSOR_RIGHT ) ||
                                ( lastArrowKeyAction == ACTIONS::CURSOR_RIGHT && action == ACTIONS::CURSOR_LEFT ) )
                            {
                                axisLock = AXIS_LOCK::NONE;
                            }
                            // Same direction axis, keep locked
                        }
                        else
                        {
                            axisLock = AXIS_LOCK::HORIZONTAL;
                        }
                    }
                    else if( action == ACTIONS::CURSOR_UP || action == ACTIONS::CURSOR_DOWN )
                    {
                        if( axisLock == AXIS_LOCK::VERTICAL )
                        {
                            // Check if opposite vertical key pressed to unlock
                            if( ( lastArrowKeyAction == ACTIONS::CURSOR_UP && action == ACTIONS::CURSOR_DOWN ) ||
                                ( lastArrowKeyAction == ACTIONS::CURSOR_DOWN && action == ACTIONS::CURSOR_UP ) )
                            {
                                axisLock = AXIS_LOCK::NONE;
                            }
                            // Same direction axis, keep locked
                        }
                        else
                        {
                            axisLock = AXIS_LOCK::VERTICAL;
                        }
                    }

                    lastArrowKeyAction = action;
                }
                else
                {
                    VECTOR2I mousePos( controls->GetMousePosition() );

                    m_cursor = grid.BestSnapAnchor( mousePos, layers, selectionGrid, sel_items );
                }

                if( axisLock == AXIS_LOCK::HORIZONTAL )
                    m_cursor.y = prevPos.y;
                else if( axisLock == AXIS_LOCK::VERTICAL )
                    m_cursor.x = prevPos.x;

                if( !selection.HasReferencePoint() )
                    originalPos = m_cursor;

                if( updateBBox )
                {
                    originalBBox = BOX2I();
                    bboxMovement = VECTOR2D();

                    for( EDA_ITEM* item : sel_items )
                        originalBBox.Merge( item->ViewBBox() );

                    updateBBox = false;
                }

                // Constrain selection bounding box to coordinates limits
                movement = getSafeMovement( m_cursor - prevPos, originalBBox, bboxMovement );

                // Apply constrained movement
                m_cursor = prevPos + movement;

                controls->ForceCursorPosition( true, m_cursor );
                selection.SetReferencePoint( m_cursor );

                prevPos = m_cursor;
                bboxMovement += movement;

                // Drag items to the current cursor position
                for( BOARD_ITEM* item : sel_items )
                {
                    // Don't double move child items.
                    if( !item->GetParent() || !item->GetParent()->IsSelected() )
                        item->Move( movement );

                    if( item->Type() == PCB_GENERATOR_T && sel_items.size() == 1 )
                    {
                        m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genUpdateEdit, aCommit,
                                                         static_cast<PCB_GENERATOR*>( item ) );
                    }

                    if( item->Type() == PCB_FOOTPRINT_T )
                        redraw3D = true;
                }

                if( redraw3D && allowRedraw3D )
                    editFrame->Update3DView( false, true );

                if( showCourtyardConflicts && drc_on_move->m_FpInMove.size() )
                {
                    drc_on_move->Run();
                    drc_on_move->UpdateConflicts( m_toolMgr->GetView(), true );
                }

                m_toolMgr->PostEvent( EVENTS::SelectedItemsMoved );
            }
            else if( !m_dragging && ( aAutoStart || !evt->IsAction( &ACTIONS::refreshPreview ) ) )
            {
                // Prepare to start dragging
                editFrame->HideSolderMask();

                m_dragging = true;

                for( BOARD_ITEM* item : sel_items )
                {
                    if( item->GetParent() && item->GetParent()->IsSelected() )
                        continue;

                    if( !item->IsNew() && !item->IsMoving() )
                    {
                        if( item->Type() == PCB_GENERATOR_T && sel_items.size() == 1 )
                        {
                            enableLocalRatsnest = false;

                            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genStartEdit, aCommit,
                                                             static_cast<PCB_GENERATOR*>( item ) );
                        }
                        else
                        {
                            aCommit->Modify( item, nullptr, RECURSE_MODE::RECURSE );
                        }

                        item->SetFlags( IS_MOVING );

                        if( item->Type() == PCB_SHAPE_T )
                            static_cast<PCB_SHAPE*>( item )->UpdateHatching();

                        item->RunOnChildren(
                                [&]( BOARD_ITEM* child )
                                {
                                    child->SetFlags( IS_MOVING );

                                    if( child->Type() == PCB_SHAPE_T )
                                        static_cast<PCB_SHAPE*>( child )->UpdateHatching();
                                },
                                RECURSE_MODE::RECURSE );
                    }
                }

                m_cursor = controls->GetCursorPosition();

                if( selection.HasReferencePoint() )
                {
                    // start moving with the reference point attached to the cursor
                    grid.SetAuxAxes( false );

                    movement = m_cursor - selection.GetReferencePoint();

                    // Drag items to the current cursor position
                    for( EDA_ITEM* item : selection )
                    {
                        if( !item->IsBOARD_ITEM() )
                            continue;

                        // Don't double move footprint pads, fields, etc.
                        if( item->GetParent() && item->GetParent()->IsSelected() )
                            continue;

                        static_cast<BOARD_ITEM*>( item )->Move( movement );
                    }

                    selection.SetReferencePoint( m_cursor );
                }
                else
                {
                    if( showCourtyardConflicts )
                    {
                        std::vector<FOOTPRINT*>& FPs = drc_on_move->m_FpInMove;

                        for( BOARD_ITEM* item : sel_items )
                        {
                            if( item->Type() == PCB_FOOTPRINT_T )
                                FPs.push_back( static_cast<FOOTPRINT*>( item ) );

                            item->RunOnChildren(
                                    [&]( BOARD_ITEM* child )
                                    {
                                        if( child->Type() == PCB_FOOTPRINT_T )
                                            FPs.push_back( static_cast<FOOTPRINT*>( child ) );
                                    },
                                    RECURSE_MODE::RECURSE );
                        }
                    }

                    // Use the mouse position over cursor, as otherwise large grids will allow only
                    // snapping to items that are closest to grid points
                    m_cursor = grid.BestDragOrigin( originalMousePos, sel_items, grid.GetSelectionGrid( selection ),
                                                    &m_selectionTool->GetFilter() );

                    // Set the current cursor position to the first dragged item origin, so the
                    // movement vector could be computed later
                    if( moveWithReference )
                    {
                        selection.SetReferencePoint( pickedReferencePoint );

                        if( angleSnapMode != LEADER_MODE::DIRECT )
                            grid.SetSnapLineOrigin( selection.GetReferencePoint() );

                        controls->ForceCursorPosition( true, pickedReferencePoint );
                        m_cursor = pickedReferencePoint;
                    }
                    else
                    {
                        // Don't snap the items on the initial drag start - this would warp
                        // the object position before the mouse moves. Instead, set up construction
                        // lines at the current object position and let the user move from there.

                        // Get the best drag origin (where an item anchor is)
                        VECTOR2I dragOrigin = m_cursor;

                        // Set the reference point to the drag origin (actual item position)
                        selection.SetReferencePoint( dragOrigin );

                        // Set up construction/snap lines at the CURRENT position, not a snapped position
                        if( angleSnapMode != LEADER_MODE::DIRECT )
                            grid.SetSnapLineOrigin( dragOrigin );

                        grid.SetAuxAxes( true, dragOrigin );

                        // Use the original cursor position if not warping
                        if( !editFrame->GetMoveWarpsCursor() )
                            m_cursor = originalCursorPos;
                        else
                        {
                            // Even when warping is enabled, stay at the drag origin initially
                            // to prevent immediate object movement
                            m_cursor = dragOrigin;
                        }
                    }

                    originalPos = selection.GetReferencePoint();
                }

                // Update variables for bounding box collision calculations
                updateBBox = true;

                controls->SetCursorPosition( m_cursor, false );

                prevPos = m_cursor;
                controls->SetAutoPan( true );
                m_toolMgr->PostEvent( EVENTS::SelectedItemsModified );
            }

            if( statusPopup )
                statusPopup->Move( KIPLATFORM::UI::GetMousePosition() + wxPoint( 20, 20 ) );

            if( enableLocalRatsnest )
                m_toolMgr->PostAction( PCB_ACTIONS::updateLocalRatsnest, movement );
        }
        else if( evt->IsCancelInteractive() || evt->IsActivate() )
        {
            if( m_dragging && evt->IsCancelInteractive() )
                evt->SetPassEvent( false );

            restore_state = true; // Canceling the tool means that items have to be restored
            break;                // Finish
        }
        else if( evt->IsClick( BUT_RIGHT ) )
        {
            m_selectionTool->GetToolMenu().ShowContextMenu( selection );
        }
        else if( evt->IsAction( &ACTIONS::undo ) )
        {
            restore_state = true; // Perform undo locally
            break;                // Finish
        }
        else if( evt->IsAction( &ACTIONS::doDelete ) )
        {
            evt->SetPassEvent();
            // Exit on a delete; there will no longer be anything to drag.
            break;
        }
        else if( evt->IsAction( &ACTIONS::duplicate )  && evt != &copy )
        {
            wxBell();
        }
        else if( evt->IsAction( &ACTIONS::cut ) )
        {
            wxBell();
        }
        else if(   evt->IsAction( &PCB_ACTIONS::rotateCw )
                || evt->IsAction( &PCB_ACTIONS::rotateCcw )
                || evt->IsAction( &PCB_ACTIONS::flip )
                || evt->IsAction( &PCB_ACTIONS::mirrorH )
                || evt->IsAction( &PCB_ACTIONS::mirrorV ) )
        {
            updateBBox = true;
            eatFirstMouseUp = false;
            evt->SetPassEvent();
        }
        else if( evt->IsMouseUp( BUT_LEFT ) || evt->IsClick( BUT_LEFT ) || isSkip )
        {
            // Eat mouse-up/-click events that leaked through from the lock dialog
            if( eatFirstMouseUp && !evt->IsAction( &ACTIONS::cursorClick ) )
            {
                eatFirstMouseUp = false;
                continue;
            }
            else if( moveIndividually && m_dragging )
            {
                // Put skipped items back where they started
                if( isSkip )
                    orig_items[itemIdx]->SetPosition( originalPos );

                view()->Update( orig_items[itemIdx] );
                rebuildConnectivity();

                if( ++itemIdx < orig_items.size() )
                {
                    BOARD_ITEM* nextItem = orig_items[itemIdx];

                    m_selectionTool->ClearSelection();

                    originalPos = nextItem->GetPosition();
                    m_selectionTool->AddItemToSel( nextItem );
                    selection.SetReferencePoint( originalPos );
                    if( angleSnapMode != LEADER_MODE::DIRECT )
                        grid.SetSnapLineOrigin( selection.GetReferencePoint() );

                    sel_items.clear();
                    sel_items.push_back( nextItem );
                    updateStatusPopup( nextItem, itemIdx + 1, orig_items.size() );

                    // Pick up new item
                    aCommit->Modify( nextItem, nullptr, RECURSE_MODE::RECURSE );
                    nextItem->Move( controls->GetCursorPosition( true ) - nextItem->GetPosition() );

                    continue;
                }
            }

            break; // finish
        }
        else if( evt->IsDblClick( BUT_LEFT ) )
        {
            // The first click will move the new item, so put it back
            if( moveIndividually )
                orig_items[itemIdx]->SetPosition( originalPos );

            break; // finish
        }
        else if( evt->IsAction( &PCB_ACTIONS::angleSnapModeChanged ) )
        {
            angleSnapMode = GetAngleSnapMode();
            configureAngleSnap( angleSnapMode );
            displayConstraintsMessage( angleSnapMode );
            evt->SetPassEvent( true );
        }
        else if( evt->IsAction( &ACTIONS::increment ) )
        {
            if( evt->HasParameter() )
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, aCommit, evt->Parameter<ACTIONS::INCREMENT>() );
            else
                m_toolMgr->RunSynchronousAction( ACTIONS::increment, aCommit, ACTIONS::INCREMENT { 1, 0 } );
        }
        else if( ZONE_FILLER_TOOL::IsZoneFillAction( evt )
                 || evt->IsAction( &PCB_ACTIONS::moveExact )
                 || evt->IsAction( &PCB_ACTIONS::moveWithReference )
                 || evt->IsAction( &PCB_ACTIONS::copyWithReference )
                 || evt->IsAction( &PCB_ACTIONS::positionRelative )
                 || evt->IsAction( &PCB_ACTIONS::interactiveOffsetTool )
                 || evt->IsAction( &ACTIONS::redo ) )
        {
            wxBell();
        }
        else
        {
            evt->SetPassEvent();
        }

    } while( ( evt = Wait() ) ); // Assignment (instead of equality test) is intentional

    // Clear temporary COURTYARD_CONFLICT flag and ensure the conflict shadow is cleared
    if( showCourtyardConflicts )
        drc_on_move->ClearConflicts( m_toolMgr->GetView() );

    controls->ForceCursorPosition( false );
    controls->ShowCursor( false );
    controls->SetAutoPan( false );

    m_dragging = false;

    // Discard reference point when selection is "dropped" onto the board
    selection.ClearReferencePoint();

    // Unselect all items to clear selection flags and then re-select the originally selected
    // items.
    m_toolMgr->RunAction( ACTIONS::selectionClear );

    if( restore_state )
    {
        if( sel_items.size() == 1 && sel_items.back()->Type() == PCB_GENERATOR_T )
        {
            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genCancelEdit, aCommit,
                                             static_cast<PCB_GENERATOR*>( sel_items.back() ) );
        }
    }
    else
    {
        if( sel_items.size() == 1 && sel_items.back()->Type() == PCB_GENERATOR_T )
        {
            m_toolMgr->RunSynchronousAction( PCB_ACTIONS::genFinishEdit, aCommit,
                                             static_cast<PCB_GENERATOR*>( sel_items.back() ) );
        }

        EDA_ITEMS oItems( orig_items.begin(), orig_items.end() );
        m_toolMgr->RunAction<EDA_ITEMS*>( ACTIONS::selectItems, &oItems );
    }

    // Remove the dynamic ratsnest from the screen
    m_toolMgr->RunAction( PCB_ACTIONS::hideLocalRatsnest );

    editFrame->PopTool( pushedEvent );
    editFrame->GetCanvas()->SetCurrentCursor( KICURSOR::ARROW );

    return !restore_state;
}

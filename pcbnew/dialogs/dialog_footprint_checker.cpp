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

#include <dialog_footprint_checker.h>
#include <widgets/appearance_controls.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <footprint_edit_frame.h>
#include <convert_shape_list_to_polygon.h>
#include <tools/footprint_editor_control.h>


static FOOTPRINT* g_lastFootprint = nullptr;
static bool       g_lastChecksRun = false;


DIALOG_FOOTPRINT_CHECKER::DIALOG_FOOTPRINT_CHECKER( FOOTPRINT_EDIT_FRAME* aParent ) :
        DIALOG_FOOTPRINT_CHECKER_BASE( aParent ),
        m_frame( aParent ),
        m_checksRun( false ),
        m_centerMarkerOnIdle( nullptr )
{
    m_markersProvider = std::make_shared<DRC_ITEMS_PROVIDER>( m_frame->GetBoard(), MARKER_BASE::MARKER_DRC );

    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markersDataView );
    m_markersDataView->AssociateModel( m_markersTreeModel );

    if( m_frame->GetBoard()->GetFirstFootprint() == g_lastFootprint )
        m_checksRun = g_lastChecksRun;

    SetupStandardButtons( { { wxID_OK,     _( "Run Checks" ) },
                            { wxID_CANCEL, _( "Close" )      } } );

    finishDialogSettings();
}


DIALOG_FOOTPRINT_CHECKER::~DIALOG_FOOTPRINT_CHECKER()
{
    m_frame->ClearFocus();

    g_lastFootprint = m_frame->GetBoard()->GetFirstFootprint();
    g_lastChecksRun = m_checksRun;

    m_markersTreeModel->DecRef();
}


void DIALOG_FOOTPRINT_CHECKER::updateData()
{
    m_markersTreeModel->Update( m_markersProvider, getSeverities() );
    updateDisplayedCounts();
}


bool DIALOG_FOOTPRINT_CHECKER::TransferDataToWindow()
{
    updateData();
    return true;
}


void DIALOG_FOOTPRINT_CHECKER::runChecks()
{
    BOARD*     board = m_frame->GetBoard();
    FOOTPRINT* footprint = board->GetFirstFootprint();
    wxString   msg;

    deleteAllMarkers();

    if( !footprint )
    {
        msg = _( "No footprint loaded." );
        return;
    }

    auto errorHandler =
            [&]( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB, const BOARD_ITEM* aItemC,
                 int aErrorCode, const wxString& aMsg, const VECTOR2I& aPt )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( aErrorCode );

                if( !aMsg.IsEmpty() )
                    drcItem->SetErrorDetail( aMsg );

                drcItem->SetItems( aItemA, aItemB, aItemC );

                PCB_MARKER* marker = new PCB_MARKER( drcItem, aPt );
                board->Add( marker );
                m_frame->GetCanvas()->GetView()->Add( marker );
            };

    OUTLINE_ERROR_HANDLER outlineErrorHandler =
            [&]( const wxString& aMsg, BOARD_ITEM* aItemA, BOARD_ITEM* aItemB, const VECTOR2I& aPt )
            {
                if( !aItemA )        // If we only have a single item, make sure it's A
                    std::swap( aItemA, aItemB );

                errorHandler( aItemA, aItemB, nullptr, DRCE_MALFORMED_COURTYARD, aMsg, aPt );
            };

    footprint->BuildCourtyardCaches( &outlineErrorHandler );

    if( !footprint->AllowMissingCourtyard()
            && footprint->GetCourtyard( F_CrtYd ).OutlineCount() == 0
            && footprint->GetCourtyard( B_CrtYd ).OutlineCount() == 0 )
    {
        errorHandler( footprint, nullptr, nullptr, DRCE_MISSING_COURTYARD, wxEmptyString, { 0, 0 } );
    }

    footprint->CheckFootprintAttributes(
            [&]( const wxString& aMsg )
            {
                errorHandler( footprint, nullptr, nullptr, DRCE_FOOTPRINT_TYPE_MISMATCH, aMsg, { 0, 0 } );
            } );

    footprint->CheckPads( m_frame,
            [&]( const PAD* aPad, int aErrorCode, const wxString& aMsg )
            {
                errorHandler( aPad, nullptr, nullptr, aErrorCode, aMsg, aPad->GetPosition() );
            } );

    footprint->CheckShortingPads(
            [&]( const PAD* aPadA, const PAD* aPadB, int aErrorCode, const VECTOR2I& aPosition )
            {
                errorHandler( aPadA, aPadB, nullptr, aErrorCode, wxEmptyString, aPosition );
            } );

    if( footprint->IsNetTie() )
    {
        footprint->CheckNetTiePadGroups(
                [&]( const wxString& aMsg )
                {
                    errorHandler( footprint, nullptr, nullptr, DRCE_FOOTPRINT, aMsg, { 0, 0 } );
                } );

        footprint->CheckNetTies(
                [&]( const BOARD_ITEM* aItemA, const BOARD_ITEM* aItemB, const BOARD_ITEM* aItemC,
                     const VECTOR2I& aPt )
                {
                    errorHandler( aItemA, aItemB, aItemC, DRCE_SHORTING_ITEMS, wxEmptyString, aPt );
                } );
    }

    footprint->CheckClippedSilk(
            [&]( BOARD_ITEM* aItemA, BOARD_ITEM* aItemB, const VECTOR2I& aPt )
            {
                errorHandler( aItemA, aItemB, nullptr, DRCE_SILK_MASK_CLEARANCE, wxEmptyString, aPt );
            } );

    m_checksRun = true;
    updateData();
    refreshEditor();
}


void DIALOG_FOOTPRINT_CHECKER::SelectMarker( const PCB_MARKER* aMarker )
{
    m_markersTreeModel->SelectMarker( aMarker );

    // wxWidgets on some platforms fails to correctly ensure that a selected item is
    // visible, so we have to do it in a separate idle event.
    m_centerMarkerOnIdle = aMarker;
    Bind( wxEVT_IDLE, &DIALOG_FOOTPRINT_CHECKER::centerMarkerIdleHandler, this );
}


void DIALOG_FOOTPRINT_CHECKER::centerMarkerIdleHandler( wxIdleEvent& aEvent )
{
    if( m_markersTreeModel->GetView()->IsFrozen() )
        return;

    m_markersTreeModel->CenterMarker( m_centerMarkerOnIdle );
    m_centerMarkerOnIdle = nullptr;
    Unbind( wxEVT_IDLE, &DIALOG_FOOTPRINT_CHECKER::centerMarkerIdleHandler, this );
}


void DIALOG_FOOTPRINT_CHECKER::OnRunChecksClick( wxCommandEvent& aEvent )
{
    m_checksRun = false;
    runChecks();
}


void DIALOG_FOOTPRINT_CHECKER::OnSelectItem( wxDataViewEvent& aEvent )
{
    BOARD*        board = m_frame->GetBoard();
    RC_TREE_NODE* node = RC_TREE_MODEL::ToNode( aEvent.GetItem() );
    const KIID&   itemID = node ? RC_TREE_MODEL::ToUUID( aEvent.GetItem() ) : niluuid;
    BOARD_ITEM*   item = board->ResolveItem( itemID, true );

    if( m_centerMarkerOnIdle )
    {
        // we already came from a cross-probe of the marker in the document; don't go around in circles
        aEvent.Skip();
        return;
    }

    if( node && item )
    {
        PCB_LAYER_ID             principalLayer = UNDEFINED_LAYER;
        LSET                     violationLayers;
        std::shared_ptr<RC_ITEM> rc_item = node->m_RcItem;

        if( item->GetLayerSet().count() > 0 )
            principalLayer = item->GetLayerSet().Seq().front();

        if( rc_item->GetErrorCode() == DRCE_MALFORMED_COURTYARD )
        {
            BOARD_ITEM* a = board->ResolveItem( rc_item->GetMainItemID(), true );

            if( a && ( a->GetFlags() & MALFORMED_B_COURTYARD ) > 0
                  && ( a->GetFlags() & MALFORMED_F_COURTYARD ) == 0 )
            {
                principalLayer = B_CrtYd;
            }
            else
            {
                principalLayer = F_CrtYd;
            }
        }
        else if (rc_item->GetErrorCode() == DRCE_INVALID_OUTLINE )
        {
            principalLayer = Edge_Cuts;
        }
        else
        {
            BOARD_ITEM*  a = board->ResolveItem( rc_item->GetMainItemID(), true );
            BOARD_ITEM*  b = board->ResolveItem( rc_item->GetAuxItemID(), true );
            BOARD_ITEM*  c = board->ResolveItem( rc_item->GetAuxItem2ID(), true );
            BOARD_ITEM*  d = board->ResolveItem( rc_item->GetAuxItem3ID(), true );

            if( a || b || c || d )
                violationLayers = LSET::AllLayersMask();

            if( a )
                violationLayers &= a->GetLayerSet();

            if( b )
                violationLayers &= b->GetLayerSet();

            if( c )
                violationLayers &= c->GetLayerSet();

            if( d )
                violationLayers &= d->GetLayerSet();
        }

        if( violationLayers.count() )
            principalLayer = violationLayers.Seq().front();
        else if( principalLayer >= 0 )
            violationLayers.set( principalLayer );

        WINDOW_THAWER thawer( m_frame );

        m_frame->FocusOnItem( item );
        m_frame->GetCanvas()->Refresh();

        if( ( violationLayers & board->GetVisibleLayers() ).none() )
        {
            m_frame->GetAppearancePanel()->SetLayerVisible( principalLayer, true );
            m_frame->GetCanvas()->Refresh();
        }

        if( board->GetVisibleLayers().test( principalLayer ) )
            m_frame->SetActiveLayer( principalLayer );
    }

    aEvent.Skip();
}


void DIALOG_FOOTPRINT_CHECKER::OnLeftDClickItem( wxMouseEvent& event )
{
    if( m_markersDataView->GetCurrentItem().IsOk() )
    {
        // turn control over to m_frame, hide this DIALOG_FOOTPRINT_CHECKER window,
        // no destruction so we can preserve listbox cursor
        if( !IsModal() )
            Show( false );
    }

    // Do not skip event here: this is not useful, and Pcbnew crashes if skipped (at least on MSW)
}


int DIALOG_FOOTPRINT_CHECKER::getSeverities() const
{
    int severities = 0;

    if( m_showErrors->GetValue() )
        severities |= RPT_SEVERITY_ERROR;

    if( m_showWarnings->GetValue() )
        severities |= RPT_SEVERITY_WARNING;

    if( m_showExclusions->GetValue() )
        severities |= RPT_SEVERITY_EXCLUSION;

    return severities;
}


void DIALOG_FOOTPRINT_CHECKER::OnSeverity( wxCommandEvent& aEvent )
{
    if( aEvent.GetEventObject() == m_showAll )
    {
        m_showErrors->SetValue( true );
        m_showWarnings->SetValue( aEvent.IsChecked() );
        m_showExclusions->SetValue( aEvent.IsChecked() );
    }

    updateData();
}


void DIALOG_FOOTPRINT_CHECKER::OnCancelClick( wxCommandEvent& aEvent )
{
    m_frame->ClearFocus();

    SetReturnCode( wxID_CANCEL );

    // Leave the tool to destroy (or not) the dialog
    FOOTPRINT_EDITOR_CONTROL* tool = m_frame->GetToolManager()->GetTool<FOOTPRINT_EDITOR_CONTROL>();
    tool->DestroyCheckerDialog();
}


void DIALOG_FOOTPRINT_CHECKER::OnClose( wxCloseEvent& aEvent )
{
    wxCommandEvent dummy;
    OnCancelClick( dummy );
}


void DIALOG_FOOTPRINT_CHECKER::refreshEditor()
{
    WINDOW_THAWER thawer( m_frame );

    m_frame->GetCanvas()->Refresh();
}


void DIALOG_FOOTPRINT_CHECKER::OnDeleteOneClick( wxCommandEvent& aEvent )
{
    // Clear the selection.  It may be the selected DRC marker.
    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    m_markersTreeModel->DeleteCurrentItem( true );

    // redraw the pcb
    refreshEditor();

    updateDisplayedCounts();
}


void DIALOG_FOOTPRINT_CHECKER::OnDeleteAllClick( wxCommandEvent& event )
{
    deleteAllMarkers();

    m_checksRun = false;
    refreshEditor();
    updateDisplayedCounts();
}


void DIALOG_FOOTPRINT_CHECKER::deleteAllMarkers()
{
    // Clear current selection list to avoid selection of deleted items
    m_frame->GetToolManager()->RunAction( ACTIONS::selectionClear );

    m_markersTreeModel->DeleteItems( false, true, false );
    m_frame->GetBoard()->DeleteMARKERs( true, true );
}


void DIALOG_FOOTPRINT_CHECKER::updateDisplayedCounts()
{
    // Collect counts:

    int numErrors = 0;
    int numWarnings = 0;
    int numExcluded = 0;

    if( m_markersProvider )
    {
        numErrors += m_markersProvider->GetCount( RPT_SEVERITY_ERROR );
        numWarnings += m_markersProvider->GetCount( RPT_SEVERITY_WARNING );
        numExcluded += m_markersProvider->GetCount( RPT_SEVERITY_EXCLUSION );
    }

    // Update badges:

    if( !m_checksRun && numErrors == 0 )
        numErrors = -1;

    if( !m_checksRun && numWarnings == 0 )
        numWarnings = -1;

    m_errorsBadge->SetMaximumNumber( numErrors );
    m_errorsBadge->UpdateNumber( numErrors, RPT_SEVERITY_ERROR );

    m_warningsBadge->SetMaximumNumber( numWarnings );
    m_warningsBadge->UpdateNumber( numWarnings, RPT_SEVERITY_WARNING );

    m_exclusionsBadge->SetMaximumNumber( numExcluded );
    m_exclusionsBadge->UpdateNumber( numExcluded, RPT_SEVERITY_EXCLUSION );
}


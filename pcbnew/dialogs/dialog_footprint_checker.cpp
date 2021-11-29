/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_marker.h>
#include <drc/drc_results_provider.h>
#include <footprint_edit_frame.h>
#include <convert_shape_list_to_polygon.h>
#include <tools/footprint_editor_control.h>


DIALOG_FOOTPRINT_CHECKER::DIALOG_FOOTPRINT_CHECKER( FOOTPRINT_EDIT_FRAME* aParent ) :
        DIALOG_FOOTPRINT_CHECKER_BASE( aParent ),
        m_frame( aParent ),
        m_checksRun( false ),
        m_markersProvider( nullptr ),
        m_severities( RPT_SEVERITY_ERROR | RPT_SEVERITY_WARNING )
{
    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markersDataView );
    m_markersDataView->AssociateModel( m_markersTreeModel );

    m_markersTreeModel->SetSeverities( -1 );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizerOK->SetLabel( _( "Run Checks" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    m_sdbSizerOK->SetDefault();
    m_sdbSizer->Layout();

    syncCheckboxes();

    finishDialogSettings();
}


DIALOG_FOOTPRINT_CHECKER::~DIALOG_FOOTPRINT_CHECKER()
{
    m_markersTreeModel->DecRef();
}


bool DIALOG_FOOTPRINT_CHECKER::TransferDataToWindow()
{
    return true;
}


bool DIALOG_FOOTPRINT_CHECKER::TransferDataFromWindow()
{
    return true;
}


// Don't globally define this; different facilities use different definitions of "ALL"
static int RPT_SEVERITY_ALL = RPT_SEVERITY_WARNING | RPT_SEVERITY_ERROR | RPT_SEVERITY_EXCLUSION;


void DIALOG_FOOTPRINT_CHECKER::syncCheckboxes()
{
    m_showAll->SetValue( m_severities == RPT_SEVERITY_ALL );
    m_showErrors->SetValue( m_severities & RPT_SEVERITY_ERROR );
    m_showWarnings->SetValue( m_severities & RPT_SEVERITY_WARNING );
    m_showExclusions->SetValue( m_severities & RPT_SEVERITY_EXCLUSION );
}


void DIALOG_FOOTPRINT_CHECKER::runChecks()
{
    BOARD*     board = m_frame->GetBoard();
    FOOTPRINT* footprint = board->GetFirstFootprint();
    wxString   msg;

    SetMarkersProvider( new BOARD_DRC_ITEMS_PROVIDER( board ) );

    deleteAllMarkers();

    if( !footprint )
    {
        msg = _( "No footprint loaded." );
        return;
    }

    OUTLINE_ERROR_HANDLER errorHandler =
            [&]( const wxString& aMsg, BOARD_ITEM* aItemA, BOARD_ITEM* aItemB, const wxPoint& aPt )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD );

                drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + aMsg );
                drcItem->SetItems( aItemA, aItemB );

                PCB_MARKER* marker = new PCB_MARKER( drcItem, aPt );
                board->Add( marker );
                m_frame->GetCanvas()->GetView()->Add( marker );
            };

    footprint->BuildPolyCourtyards( &errorHandler );


    const std::function<void( const wxString& msg )> typeWarning =
            [&]( const wxString& aMsg )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_FOOTPRINT_TYPE_MISMATCH );

                drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + aMsg );
                drcItem->SetItems( footprint );

                PCB_MARKER* marker = new PCB_MARKER( drcItem, wxPoint( 0, 0 ) );
                board->Add( marker );
                m_frame->GetCanvas()->GetView()->Add( marker );
            };

    const std::function<void( const wxString& msg, const wxPoint& position )> tstHoleInTHPad =
            [&]( const wxString& aMsg, const wxPoint& aPosition )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_PAD_TH_WITH_NO_HOLE );

                drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + aMsg );
                drcItem->SetItems( footprint );

                PCB_MARKER* marker = new PCB_MARKER( drcItem, aPosition );
                board->Add( marker );
                m_frame->GetCanvas()->GetView()->Add( marker );
            };

    footprint->CheckFootprintAttributes( &typeWarning );
    footprint->CheckFootprintTHPadNoHoles( &tstHoleInTHPad );
    m_checksRun = true;

    SetMarkersProvider( new BOARD_DRC_ITEMS_PROVIDER( board ) );

    refreshEditor();
}


void DIALOG_FOOTPRINT_CHECKER::SetMarkersProvider( RC_ITEMS_PROVIDER* aProvider )
{
    m_markersTreeModel->SetProvider( aProvider );
    updateDisplayedCounts();
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
    BOARD_ITEM*   item = board->GetItem( itemID );

    if( node && item )
    {
        PCB_LAYER_ID             principalLayer = item->GetLayer();
        LSET                     violationLayers;
        std::shared_ptr<RC_ITEM> rc_item = node->m_RcItem;

        if( rc_item->GetErrorCode() == DRCE_MALFORMED_COURTYARD )
        {
            BOARD_ITEM* a = board->GetItem( rc_item->GetMainItemID() );

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
            BOARD_ITEM*  a = board->GetItem( rc_item->GetMainItemID() );
            BOARD_ITEM*  b = board->GetItem( rc_item->GetAuxItemID() );
            BOARD_ITEM*  c = board->GetItem( rc_item->GetAuxItem2ID() );
            BOARD_ITEM*  d = board->GetItem( rc_item->GetAuxItem3ID() );

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
        else
            violationLayers.set( principalLayer );

        WINDOW_THAWER thawer( m_frame );

        m_frame->FocusOnItem( item );
        m_frame->GetCanvas()->Refresh();

        if( ( violationLayers & board->GetVisibleLayers() ) == 0 )
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

    // Do not skip aVent here: this is not useful, and Pcbnew crashes
    // if skipped (at least on Windows)
}


void DIALOG_FOOTPRINT_CHECKER::OnSeverity( wxCommandEvent& aEvent )
{
    int flag = 0;

    if( aEvent.GetEventObject() == m_showAll )
        flag = RPT_SEVERITY_ALL;
    else if( aEvent.GetEventObject() == m_showErrors )
        flag = RPT_SEVERITY_ERROR;
    else if( aEvent.GetEventObject() == m_showWarnings )
        flag = RPT_SEVERITY_WARNING;
    else if( aEvent.GetEventObject() == m_showExclusions )
        flag = RPT_SEVERITY_EXCLUSION;

    if( aEvent.IsChecked() )
        m_severities |= flag;
    else if( aEvent.GetEventObject() == m_showAll )
        m_severities = RPT_SEVERITY_ERROR;
    else
        m_severities &= ~flag;

    syncCheckboxes();

    // Set the provider's severity levels through the TreeModel so that the old tree
    // can be torn down before the severity changes.
    //
    // It's not clear this is required, but we've had a lot of issues with wxDataView
    // being cranky on various platforms.

    m_markersTreeModel->SetSeverities( m_severities );

    updateDisplayedCounts();
}


void DIALOG_FOOTPRINT_CHECKER::OnCancelClick( wxCommandEvent& aEvent )
{
    m_frame->FocusOnItem( nullptr );

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
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

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
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markersTreeModel->DeleteItems( false, true, true );
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


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

#include <wx/wx.h>
#include <dialog_footprint_checker.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <pcb_marker.h>
#include <drc/drc_results_provider.h>
#include <footprint_edit_frame.h>
#include <convert_drawsegment_list_to_polygon.h>
#include <tools/footprint_editor_control.h>


DIALOG_FOOTPRINT_CHECKER::DIALOG_FOOTPRINT_CHECKER( FOOTPRINT_EDIT_FRAME* aParent ) :
        DIALOG_FOOTPRINT_CHECKER_BASE( aParent ),
        m_frame( aParent )
{
    m_markersTreeModel = new RC_TREE_MODEL( m_frame, m_markersDataView );
    m_markersDataView->AssociateModel( m_markersTreeModel );

    m_markersTreeModel->SetSeverities( -1 );

    // We use a sdbSizer to get platform-dependent ordering of the action buttons, but
    // that requires us to correct the button labels here.
    m_sdbSizerOK->SetLabel( _( "Run Checks" ) );
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    m_sdbSizerOK->SetDefault();
    m_sdbSizer->SetSizeHints( this );
    GetSizer()->SetSizeHints(this);
    Centre();
}


DIALOG_FOOTPRINT_CHECKER::~DIALOG_FOOTPRINT_CHECKER()
{
    m_markersTreeModel->DecRef();
}


bool DIALOG_FOOTPRINT_CHECKER::TransferDataToWindow()
{
    runChecks();

    return true;
}


bool DIALOG_FOOTPRINT_CHECKER::TransferDataFromWindow()
{
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

    SetMarkersProvider( new BOARD_DRC_ITEMS_PROVIDER( m_frame->GetBoard() ) );

    WINDOW_THAWER thawer( m_frame );

    m_frame->GetCanvas()->Refresh();
}


void DIALOG_FOOTPRINT_CHECKER::SetMarkersProvider( RC_ITEMS_PROVIDER* aProvider )
{
    m_markersTreeModel->SetProvider( aProvider );
}


void DIALOG_FOOTPRINT_CHECKER::OnRunChecksClick( wxCommandEvent& aEvent )
{
    runChecks();
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


void DIALOG_FOOTPRINT_CHECKER::OnSelectItem( wxDataViewEvent& aEvent )
{
    const KIID&   itemID = RC_TREE_MODEL::ToUUID( aEvent.GetItem() );
    BOARD_ITEM*   item = m_frame->GetBoard()->GetItem( itemID );
    WINDOW_THAWER thawer( m_frame );

    m_frame->FocusOnItem( item );
    m_frame->GetCanvas()->Refresh();

    aEvent.Skip();
}


void DIALOG_FOOTPRINT_CHECKER::OnLeftDClickItem( wxMouseEvent& event )
{
    event.Skip();

    if( m_markersDataView->GetCurrentItem().IsOk() )
    {
        if( !IsModal() )
            Show( false );
    }
}


void DIALOG_FOOTPRINT_CHECKER::OnDeleteAllClick( wxCommandEvent& event )
{
    deleteAllMarkers();

    WINDOW_THAWER thawer( m_frame );

    m_frame->GetCanvas()->Refresh();
}


void DIALOG_FOOTPRINT_CHECKER::deleteAllMarkers()
{
    // Clear current selection list to avoid selection of deleted items
    m_frame->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );

    m_markersTreeModel->DeleteItems( false, true, true );
}



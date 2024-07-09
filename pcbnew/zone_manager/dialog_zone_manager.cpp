/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2023, 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <optional>
#include <wx/dataview.h>
#include <wx/debug.h>
#include <wx/event.h>
#include <wx/gdicmn.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <wx/string.h>
#include <board_commit.h>
#include <widgets/std_bitmap_button.h>
#include <zone.h>
#include <pad.h>
#include <board.h>
#include <bitmaps.h>
#include <string_utils.h>
#include <zone_filler.h>

#include "dialog_zone_manager_base.h"
#include "model_zones_overview_table.h"
#include "panel_zone_properties.h"
#include "dialog_zone_manager.h"
#include "widgets/wx_progress_reporters.h"
#include "zone_management_base.h"
#include "zone_manager/model_zones_overview_table.h"
#include "zone_manager/panel_zone_gal.h"
#include "zone_manager/zone_manager_preference.h"
#include "zones_container.h"
#include "pane_zone_viewer.h"
#include "zone_manager_preference.h"


DIALOG_ZONE_MANAGER::DIALOG_ZONE_MANAGER( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aZoneInfo ) :
        DIALOG_ZONE_MANAGER_BASE( aParent ),
        m_pcbFrame( aParent ),
        m_zoneInfo( aZoneInfo ),
        m_zonesContainer( std::make_unique<ZONES_CONTAINER>( aParent->GetBoard() ) ),
        m_priorityDragIndex( {} ),
        m_needZoomGAL( true ),
        m_isFillingZones( false ),
        m_zoneFillComplete( false )
{
#ifdef __APPLE__
    m_sizerZoneOP->InsertSpacer( m_sizerZoneOP->GetItemCount(), 5 );
#endif

    m_btnMoveUp->SetBitmap( KiBitmapBundle( BITMAPS::small_up ) );
    m_btnMoveDown->SetBitmap( KiBitmapBundle( BITMAPS::small_down ) );

    m_panelZoneProperties = new PANEL_ZONE_PROPERTIES( this, aParent, *m_zonesContainer );
    m_sizerProperties->Add( m_panelZoneProperties, 1, wxTOP | wxEXPAND, 5 );

    m_zoneViewer = new PANE_ZONE_VIEWER( this, aParent );
    m_sizerTop->Add( m_zoneViewer, 1, wxBOTTOM | wxLEFT | wxEXPAND, 10 );

    m_checkRepour->SetValue( ZONE_MANAGER_PREFERENCE::GetRepourOnClose() );
    m_zoneViewer->SetId( ZONE_VIEWER );

    for( const auto& [k, v] : MODEL_ZONES_OVERVIEW_TABLE::GetColumnNames() )
    {
        if( k == MODEL_ZONES_OVERVIEW_TABLE::LAYERS )
            m_viewZonesOverview->AppendIconTextColumn( v, k );
        else
            m_viewZonesOverview->AppendTextColumn( v, k );
    }

    m_modelZoneOverviewTable = new MODEL_ZONES_OVERVIEW_TABLE( m_zonesContainer->GetZonesPriorityContainers(),
                                                               aParent->GetBoard(), aParent, this );
    m_viewZonesOverview->AssociateModel( m_modelZoneOverviewTable.get() );

#if wxUSE_DRAG_AND_DROP
    m_viewZonesOverview->EnableDragSource( wxDF_UNICODETEXT );
    m_viewZonesOverview->EnableDropTarget( wxDF_UNICODETEXT );

    Bind( wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &DIALOG_ZONE_MANAGER::OnBeginDrag, this,
          VIEW_ZONE_TABLE );
    Bind( wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, &DIALOG_ZONE_MANAGER::OnDropPossible, this,
          VIEW_ZONE_TABLE );
    Bind( wxEVT_DATAVIEW_ITEM_DROP, &DIALOG_ZONE_MANAGER::OnDrop, this, VIEW_ZONE_TABLE );
#endif // wxUSE_DRAG_AND_DROP

    Bind( wxEVT_BUTTON, &DIALOG_ZONE_MANAGER::OnOk, this, wxID_OK );
    Bind( EVT_ZONE_NAME_UPDATE, &DIALOG_ZONE_MANAGER::OnZoneNameUpdate, this );
    Bind( EVT_ZONES_OVERVIEW_COUNT_CHANGE, &DIALOG_ZONE_MANAGER::OnZonesTableRowCountChange, this );
    Bind( wxEVT_CHECKBOX, &DIALOG_ZONE_MANAGER::OnCheckBoxClicked, this );
    Bind( wxEVT_IDLE, &DIALOG_ZONE_MANAGER::OnIdle, this );
    Bind( wxEVT_BOOKCTRL_PAGE_CHANGED,
            [this]( wxNotebookEvent& aEvent )
            {
                Layout();
            },
            ZONE_VIEWER );

    if( m_modelZoneOverviewTable->GetCount() )
        SelectZoneTableItem( m_modelZoneOverviewTable->GetItem( 0 ) );

    Layout();
    m_MainBoxSizer->Fit( this );

    //NOTE - Works on Windows and MacOS , need further handling in IDLE on Ubuntu
    FitCanvasToScreen();
}


DIALOG_ZONE_MANAGER::~DIALOG_ZONE_MANAGER() = default;


void DIALOG_ZONE_MANAGER::FitCanvasToScreen()
{
    if( PANEL_ZONE_GAL* canvas = m_zoneViewer->GetZoneGAL() )
        canvas->ZoomFitScreen();
}


void DIALOG_ZONE_MANAGER::PostProcessZoneViewSelectionChange( wxDataViewItem const& aItem )
{
    bool textCtrlHasFocus = m_filterCtrl->HasFocus();
    long filterInsertPos = m_filterCtrl->GetInsertionPoint();

    if( aItem.IsOk() )
    {
        m_viewZonesOverview->Select( aItem );
        m_viewZonesOverview->EnsureVisible( aItem );
    }
    else
    {
        if( m_modelZoneOverviewTable->GetCount() )
        {
            wxDataViewItem first_item = m_modelZoneOverviewTable->GetItem( 0 );
            m_viewZonesOverview->Select( first_item );
            m_viewZonesOverview->EnsureVisible( first_item );
            m_zoneViewer->ActivateSelectedZone( m_modelZoneOverviewTable->GetZone( first_item ) );
        }
        else
        {
            m_zoneViewer->ActivateSelectedZone( nullptr );
        }
    }

    if( textCtrlHasFocus )
    {
        m_filterCtrl->SetFocus();
        m_filterCtrl->SetInsertionPoint( filterInsertPos );
    }
}


void DIALOG_ZONE_MANAGER::GenericProcessChar( wxKeyEvent& aEvent )
{
    aEvent.Skip();

    if( aEvent.GetKeyCode() == WXK_DOWN || aEvent.GetKeyCode() == WXK_UP )
        Bind( wxEVT_IDLE, &DIALOG_ZONE_MANAGER::OnIdle, this );
}


void DIALOG_ZONE_MANAGER::OnTableChar( wxKeyEvent& aEvent )
{
    GenericProcessChar( aEvent );
}


void DIALOG_ZONE_MANAGER::OnTableCharHook( wxKeyEvent& aEvent )
{
    GenericProcessChar( aEvent );
}


void DIALOG_ZONE_MANAGER::OnIdle( wxIdleEvent& aEvent )
{
    WXUNUSED( aEvent )
    m_viewZonesOverview->SetFocus();
    Unbind( wxEVT_IDLE, &DIALOG_ZONE_MANAGER::OnIdle, this );

    if( !m_needZoomGAL )
        return;

    m_needZoomGAL = false;
    FitCanvasToScreen();
}


void DIALOG_ZONE_MANAGER::onDialogResize( wxSizeEvent& event )
{
    event.Skip();
    FitCanvasToScreen();
}


void DIALOG_ZONE_MANAGER::OnZoneSelectionChanged( ZONE* zone )
{
    for( ZONE_SELECTION_CHANGE_NOTIFIER* i :
         std::list<ZONE_SELECTION_CHANGE_NOTIFIER*>{ m_panelZoneProperties, m_zoneViewer } )
    {
        i->OnZoneSelectionChanged( zone );
    }

    Layout();
}


void DIALOG_ZONE_MANAGER::OnViewZonesOverviewOnLeftUp( wxMouseEvent& aEvent )
{
    Bind( wxEVT_IDLE, &DIALOG_ZONE_MANAGER::OnIdle, this );
}


void DIALOG_ZONE_MANAGER::OnDataViewCtrlSelectionChanged( wxDataViewEvent& aEvent )
{
    SelectZoneTableItem( aEvent.GetItem() );
}


void DIALOG_ZONE_MANAGER::SelectZoneTableItem( wxDataViewItem const& aItem )
{
    ZONE* zone = m_modelZoneOverviewTable->GetZone( aItem );

    if( !zone )
        return;

    OnZoneSelectionChanged( zone );
}


void DIALOG_ZONE_MANAGER::OnOk( wxCommandEvent& aEvt )
{
    for( ZONE_MANAGEMENT_BASE* zone_management :
         std::list<ZONE_MANAGEMENT_BASE*>{ m_panelZoneProperties, m_zonesContainer.get() } )
    {
        zone_management->OnUserConfirmChange();
    }

    if( m_zoneInfo )
    {
        if( std::shared_ptr<ZONE_SETTINGS> zone = m_panelZoneProperties->GetZoneSettings() )
            *m_zoneInfo = *zone;
    }

    aEvt.Skip();
}


void DIALOG_ZONE_MANAGER::OnRepourCheck( wxCommandEvent& aEvent )
{
    ZONE_MANAGER_PREFERENCE::SetRefillOnClose( m_checkRepour->IsChecked() );
}


#if wxUSE_DRAG_AND_DROP

void DIALOG_ZONE_MANAGER::OnBeginDrag( wxDataViewEvent& aEvent )
{
    wxTextDataObject* obj = new wxTextDataObject;
    obj->SetText( "42" ); //FIXME - Workaround for drop on GTK
    aEvent.SetDataObject( obj );
    aEvent.SetDragFlags( wxDrag_AllowMove );
    const wxDataViewItem it = aEvent.GetItem();

    if( it.IsOk() )
        m_priorityDragIndex = m_modelZoneOverviewTable->GetRow( it );
}


void DIALOG_ZONE_MANAGER::OnDropPossible( wxDataViewEvent& aEvent )
{
    aEvent.SetDropEffect( wxDragMove ); // check 'move' drop effect
}


void DIALOG_ZONE_MANAGER::OnDrop( wxDataViewEvent& aEvent )
{
    if( aEvent.GetDataFormat() != wxDF_UNICODETEXT )
    {
        aEvent.Veto();
        return;
    }

    if( !m_priorityDragIndex.has_value() )
        return;

    const wxDataViewItem it = aEvent.GetItem();

    if( !it.IsOk() )
    {
        aEvent.Veto();
        return;
    }

    unsigned int                  drop_index = m_modelZoneOverviewTable->GetRow( it );
    const std::optional<unsigned> rtn =
            m_modelZoneOverviewTable->SwapZonePriority( *m_priorityDragIndex, drop_index );

    if( rtn.has_value() )
    {
        const wxDataViewItem item = m_modelZoneOverviewTable->GetItem( *rtn );
        if( item.IsOk() )
            m_viewZonesOverview->Select( item );
    }
}

#endif // wxUSE_DRAG_AND_DROP


void DIALOG_ZONE_MANAGER::OnMoveUpClick( wxCommandEvent& aEvent )
{
    MoveSelectedZonePriority( ZONE_INDEX_MOVEMENT::MOVE_UP );
}


void DIALOG_ZONE_MANAGER::OnMoveDownClick( wxCommandEvent& aEvent )
{
    MoveSelectedZonePriority( ZONE_INDEX_MOVEMENT::MOVE_DOWN );
}


void DIALOG_ZONE_MANAGER::OnFilterCtrlCancel( wxCommandEvent& aEvent )
{
    PostProcessZoneViewSelectionChange(
            m_modelZoneOverviewTable->ClearFilter( m_viewZonesOverview->GetSelection() ) );
    aEvent.Skip();
}


void DIALOG_ZONE_MANAGER::OnFilterCtrlSearch( wxCommandEvent& aEvent )
{
    PostProcessZoneViewSelectionChange( m_modelZoneOverviewTable->ApplyFilter(
            aEvent.GetString(), m_viewZonesOverview->GetSelection() ) );
    aEvent.Skip();
}


void DIALOG_ZONE_MANAGER::OnFilterCtrlTextChange( wxCommandEvent& aEvent )
{
    PostProcessZoneViewSelectionChange( m_modelZoneOverviewTable->ApplyFilter(
            aEvent.GetString(), m_viewZonesOverview->GetSelection() ) );
    aEvent.Skip();
}


void DIALOG_ZONE_MANAGER::OnFilterCtrlEnter( wxCommandEvent& aEvent )
{
    PostProcessZoneViewSelectionChange( m_modelZoneOverviewTable->ApplyFilter(
            aEvent.GetString(), m_viewZonesOverview->GetSelection() ) );
    aEvent.Skip();
}


void DIALOG_ZONE_MANAGER::OnUpdateDisplayedZonesClick( wxCommandEvent& aEvent )
{
    if( m_isFillingZones )
        return;

    m_isFillingZones = true;
    m_panelZoneProperties->TransferZoneSettingsFromWindow();
    m_zonesContainer->FlushZoneSettingsChange();
    m_zonesContainer->FlushPriorityChange();

    BOARD* board = m_pcbFrame->GetBoard();
    board->IncrementTimeStamp();

    auto commit = std::make_unique<BOARD_COMMIT>( m_pcbFrame );
    m_filler = std::make_unique<ZONE_FILLER>( board, commit.get() );
    auto reporter = std::make_unique<WX_PROGRESS_REPORTER>( this, _( "Fill All Zones" ), 5 );
    m_filler->SetProgressReporter( reporter.get() );

    // TODO: replace these const_cast calls with a different solution that avoids mutating the
    // container of the board.  This is relatively safe as-is because the original zones list is
    // swapped back in below, but still should be changed to avoid invalidating the board state
    // in case this code is refactored to be a non-modal dialog in the future.
    const_cast<ZONES&>( board->Zones() ) = m_zonesContainer->GetClonedZoneList();

    //NOTE - Nether revert nor commit is needed here , cause the cloned zones are not owned by
    //       the pcb frame.
    m_zoneFillComplete = m_filler->Fill( board->Zones() );
    board->BuildConnectivity();

    if( PANEL_ZONE_GAL* gal = m_zoneViewer->GetZoneGAL() )
    {
        gal->RedrawRatsnest();
        gal->GetView()->UpdateItems();
        gal->Refresh();
        int layer = gal->GetLayer();
        gal->ActivateSelectedZone(
                m_modelZoneOverviewTable->GetZone( m_viewZonesOverview->GetSelection() ) );
        gal->OnLayerSelected( layer );
    }

    //NOTE - But the connectivity need to be rebuild, otherwise if cancelling, it may
    //       segfault.
    const_cast<ZONES&>( board->Zones() ) = m_zonesContainer->GetOriginalZoneList();
    board->BuildConnectivity();

    m_isFillingZones = false;
}


void DIALOG_ZONE_MANAGER::OnZoneNameUpdate( wxCommandEvent& aEvent )
{
    if( ZONE* zone = m_panelZoneProperties->GetZone(); zone != nullptr )
    {
        zone->SetZoneName( aEvent.GetString() );
        m_modelZoneOverviewTable->RowChanged( m_modelZoneOverviewTable->GetRow(
                m_modelZoneOverviewTable->GetItemByZone( zone ) ) );
    }
}


void DIALOG_ZONE_MANAGER::OnZonesTableRowCountChange( wxCommandEvent& aEvent )
{
    unsigned count = aEvent.GetInt();

    for( STD_BITMAP_BUTTON* btn : { m_btnMoveDown, m_btnMoveUp } )
        btn->Enable( count == m_modelZoneOverviewTable->GetAllZonesCount() );
}


void DIALOG_ZONE_MANAGER::OnCheckBoxClicked( wxCommandEvent& aEvent )
{
    const wxObject* sender = aEvent.GetEventObject();

    if( aEvent.GetEventObject() == m_checkName )
    {
        m_modelZoneOverviewTable->EnableFitterByName( aEvent.IsChecked() );
    }
    else if( aEvent.GetEventObject() == m_checkNet )
    {
        m_modelZoneOverviewTable->EnableFitterByNet( aEvent.IsChecked() );
    }

    if( ( sender == m_checkName || sender == m_checkNet ) && !m_filterCtrl->IsEmpty() )
    {
        m_modelZoneOverviewTable->ApplyFilter( m_filterCtrl->GetValue(),
                                               m_viewZonesOverview->GetSelection() );
    }
}


void DIALOG_ZONE_MANAGER::MoveSelectedZonePriority( ZONE_INDEX_MOVEMENT aMove )
{
    if( !m_viewZonesOverview->HasSelection() )
        return;

    const wxDataViewItem selectedItem = m_viewZonesOverview->GetSelection();

    if( !selectedItem.IsOk() )
        return;

    const unsigned int            selectedRow = m_modelZoneOverviewTable->GetRow( selectedItem );
    const std::optional<unsigned> new_index =
            m_modelZoneOverviewTable->MoveZoneIndex( selectedRow, aMove );

    if( new_index.has_value() )
    {
        wxDataViewItem new_item = m_modelZoneOverviewTable->GetItem( *new_index );
        PostProcessZoneViewSelectionChange( new_item );
    }
}

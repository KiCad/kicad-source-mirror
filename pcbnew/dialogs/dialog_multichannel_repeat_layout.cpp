/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dialogs/dialog_multichannel_repeat_layout.h>
#include <widgets/wx_grid.h>
#include <bitmaps.h>
#include <grid_tricks.h>
#include <pcb_edit_frame.h>
#include <tools/multichannel_tool.h>
#include <widgets/grid_icon_text_helpers.h>
#include <zone.h>
#include <board.h>
#include <footprint.h>
#include <utility>
#include <algorithm>


DIALOG_MULTICHANNEL_REPEAT_LAYOUT::DIALOG_MULTICHANNEL_REPEAT_LAYOUT( PCB_BASE_FRAME* aFrame,
                                                                      MULTICHANNEL_TOOL *aParentTool ) :
        DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE( aFrame ),
        m_parentTool( aParentTool )
{
    m_detailsIcon = KiBitmapBundleDef( BITMAPS::help, 16 );

    m_raGrid->PushEventHandler( new GRID_TRICKS( static_cast<WX_GRID*>( m_raGrid ) ) );
    m_raGrid->ClearGrid();
    m_raGrid->EnableEditing( false );
    m_raGrid->AutoSizeColumn( 1 );
    m_raGrid->SetupColumnAutosizer( 1 );

    m_raGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_MULTICHANNEL_REPEAT_LAYOUT::OnGridCellLeftClick, this );

    // The parts of the row under the cursor are marked on the board next to the reference parts
    m_raGrid->Bind( wxEVT_GRID_SELECT_CELL,
                    [this]( wxGridEvent& aEvent )
                    {
                        int row = aEvent.GetRow();

                        if( row >= 0 && row < static_cast<int>( m_targetRAs.size() ) )
                            m_parentTool->HighlightRepeatAreas( m_targetRAs[row].m_targetRA );

                        aEvent.Skip();
                    } );

    // Ticks and the anchor reshape the channels; the other options only change the preview
    m_raGrid->Bind( wxEVT_GRID_CELL_CHANGED, [this]( wxGridEvent& ) { onOptionChanged(); } );
    m_refAnchorFp->Bind( wxEVT_CHOICE, [this]( wxCommandEvent& ) { onOptionChanged(); } );

    Bind( wxEVT_CHECKBOX,
          [this]( wxCommandEvent& )
          {
              readWindow();
              m_parentTool->UpdateRepeatPreview();
          } );

    m_cbLooseMatching->Bind( wxEVT_CHECKBOX,
                             [this]( wxCommandEvent& )
                             {
                                 m_parentTool->DiscoverRepeatCandidates( m_cbLooseMatching->GetValue() );
                                 m_parentTool->UpdateRepeatPreview();
                                 TransferDataToWindow();
                             } );

    // Not modal, so Cancel has to close the dialog itself
    Bind( wxEVT_BUTTON, [this]( wxCommandEvent& ) { Close(); }, wxID_CANCEL );

    Layout();
    SetupStandardButtons();
    finishDialogSettings();
}

DIALOG_MULTICHANNEL_REPEAT_LAYOUT::~DIALOG_MULTICHANNEL_REPEAT_LAYOUT()
{
    m_raGrid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_MULTICHANNEL_REPEAT_LAYOUT::OnGridCellLeftClick, this );
    m_raGrid->PopEventHandler( true );
}


void DIALOG_MULTICHANNEL_REPEAT_LAYOUT::readWindow()
{
    RULE_AREAS_DATA* data = m_parentTool->GetData();

    for( size_t i = 0; i < m_targetRAs.size(); i++ )
        data->m_compatMap[m_targetRAs[i].m_targetRA].m_doCopy = m_raGrid->GetCellValue( i, 0 ) == wxT( "1" );

    data->m_options.m_copyPlacement = m_cbCopyPlacement->GetValue();
    data->m_options.m_copyRouting = m_cbCopyRouting->GetValue();
    data->m_options.m_connectedRoutingOnly = m_cbCopyOnlyConnectedRouting->GetValue();
    data->m_options.m_copyOtherItems = m_cbCopyOtherItems->GetValue();
    data->m_options.m_groupItems = m_cbGroupItems->GetValue();
    data->m_options.m_includeLockedItems = m_cbIncludeLockedComponents->GetValue();
    data->m_linkAreas = m_cbLinkAreas->GetValue();
    data->m_options.m_anchor = niluuid;

    // A duplicate reference designator outside the area must not become the anchor
    for( FOOTPRINT* fp : data->m_refRA->m_components )
    {
        if( fp->GetReference() == m_refAnchorFp->GetStringSelection() )
            data->m_options.m_anchor = fp->m_Uuid;
    }
}


void DIALOG_MULTICHANNEL_REPEAT_LAYOUT::onOptionChanged()
{
    readWindow();

    // The refresh pumps events while it shows progress, so a second one queued behind it would run inside it
    if( std::exchange( m_refreshPending, true ) )
        return;

    // Rebuilding the grid from inside one of its own events is not safe
    CallAfter( [this]()
               {
                   readWindow();
                   m_parentTool->RefreshRepeat();
                   TransferDataToWindow();
                   m_refreshPending = false;
               } );
}


bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataFromWindow()
{
    readWindow();
    m_parentTool->ApplyRepeat();

    // This dialog is not modal, so it closes itself
    Close();
    return true;
}


bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataToWindow()
{
    RULE_AREAS_DATA* data = m_parentTool->GetData();
    const KIID&      linkSet = data->m_refRA->m_zone->GetLinkSet();

    m_targetRAs.clear();

    for( const auto& [ruleArea, ruleAreaData] : data->m_compatMap )
    {
        TABLE_ENTRY ent;
        bool        linked = linkSet != niluuid && ruleArea->m_zone->GetLinkSet() == linkSet;

        ent.m_doCopy = ruleAreaData.m_doCopy;
        ent.m_errMsg = linked && ruleAreaData.m_isOk ? _( "OK (linked)" ) : ruleAreaData.m_errorMsg;
        ent.m_isOK = ruleAreaData.m_isOk;
        ent.m_raName = ruleArea->m_ruleName;
        ent.m_targetRA = ruleArea;
        ent.m_mismatchReasons = ruleAreaData.m_mismatchReasons;

        // A discovered channel is named after its parts until it is created
        if( ent.m_raName.IsEmpty() )
            ent.m_raName = MULTICHANNEL_TOOL::JoinReferences( ruleArea->m_components );

        m_targetRAs.push_back( ent );
    }

    std::sort( m_targetRAs.begin(), m_targetRAs.end(),
            [] ( const TABLE_ENTRY&a ,const TABLE_ENTRY& b ) -> int
            {
                if ( !a.m_isOK && b.m_isOK )
                    return 0;
                else if ( a.m_isOK && !b.m_isOK )
                    return 1;
                else
                    return a.m_raName < b.m_raName;
            } );

    m_raGrid->ClearRows();
    m_raGrid->AppendRows( m_targetRAs.size() );

    int i = 0;

    for( TABLE_ENTRY& entry : m_targetRAs )
    {
        m_raGrid->SetCellValue( i, 1, entry.m_raName );
        m_raGrid->SetCellValue( i, 2, entry.m_errMsg );
        m_raGrid->SetCellValue( i, 3, wxString() );
        m_raGrid->SetCellRenderer( i, 0, new wxGridCellBoolRenderer);
        m_raGrid->SetCellEditor( i, 0, new wxGridCellBoolEditor);
        m_raGrid->SetCellValue( i, 0, entry.m_doCopy ? wxT( "1" ) : wxT( "" ) );

        if( !entry.m_isOK && !entry.m_mismatchReasons.empty() )
        {
            wxGridCellAttr* attr = new wxGridCellAttr;
            attr->SetRenderer( new GRID_CELL_ICON_RENDERER( m_detailsIcon ) );
            attr->SetReadOnly();
            m_raGrid->SetAttr( i, 3, attr );
            m_raGrid->SetCellValue( i, 3, wxString() );
        }

        i++;
    }

    m_refRAName->SetLabelText( data->FromSelection()
                                       ? wxString::Format( _( "%d selected footprints" ),
                                                           (int) data->m_selectedFootprints.size() )
                                       : data->m_refRA->m_zone->GetZoneName() );

    wxArrayString refFpNames;
    refFpNames.push_back( "" );

    for( FOOTPRINT* fp : data->m_refRA->m_components )
        refFpNames.push_back( fp->GetReference() );

    refFpNames.Sort();
    m_refAnchorFp->Set( refFpNames );
    m_refAnchorFp->SetSelection( 0 );

    for( FOOTPRINT* fp : data->m_refRA->m_components )
    {
        if( fp->m_Uuid == data->m_options.m_anchor )
            m_refAnchorFp->SetStringSelection( fp->GetReference() );
    }

    m_cbCopyPlacement->SetValue( data->m_options.m_copyPlacement );
    m_cbCopyRouting->SetValue( data->m_options.m_copyRouting );
    m_cbCopyOnlyConnectedRouting->SetValue( data->m_options.m_connectedRoutingOnly );
    m_cbGroupItems->SetValue( data->m_options.m_groupItems );
    m_cbCopyOtherItems->SetValue( data->m_options.m_copyOtherItems );
    m_cbIncludeLockedComponents->SetValue( data->m_options.m_includeLockedItems );
    m_cbLinkAreas->SetValue( data->m_linkAreas );

    // Channels made from a selection are always groups; loose matching only applies to discovery
    m_cbGroupItems->Enable( !data->FromSelection() );
    m_cbLooseMatching->Enable( data->FromSelection() );

    return true;
}


void DIALOG_MULTICHANNEL_REPEAT_LAYOUT::OnGridCellLeftClick( wxGridEvent& aEvent )
{
    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( col == 3 && row >= 0 && row < static_cast<int>( m_targetRAs.size() ) )
    {
        const TABLE_ENTRY& entry = m_targetRAs[row];

        if( !entry.m_isOK && !entry.m_mismatchReasons.empty() && m_parentTool )
        {
            wxString summary = wxString::Format( _( "Rule area topologies do not match: %s" ), entry.m_errMsg );

            m_parentTool->ShowMismatchDetails( this, summary, entry.m_mismatchReasons );
        }
    }

    aEvent.Skip();
}

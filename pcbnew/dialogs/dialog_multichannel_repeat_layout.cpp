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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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


DIALOG_MULTICHANNEL_REPEAT_LAYOUT::DIALOG_MULTICHANNEL_REPEAT_LAYOUT( PCB_BASE_FRAME* aFrame,
                                                                      MULTICHANNEL_TOOL *aParentTool ) :
        DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE( aFrame ),
        m_parentTool( aParentTool )
{
    m_board = aFrame->GetBoard();
    m_detailsIcon = KiBitmapBundleDef( BITMAPS::help, 16 );

    m_raGrid->PushEventHandler( new GRID_TRICKS( static_cast<WX_GRID*>( m_raGrid ) ) );
    m_raGrid->ClearGrid();
    m_raGrid->EnableEditing( true );
    m_raGrid->AutoSizeColumn( 1 );
    m_raGrid->SetupColumnAutosizer( 1 );

    m_raGrid->Bind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_MULTICHANNEL_REPEAT_LAYOUT::OnGridCellLeftClick, this );

    Layout();
    SetupStandardButtons();
    finishDialogSettings();
}

DIALOG_MULTICHANNEL_REPEAT_LAYOUT::~DIALOG_MULTICHANNEL_REPEAT_LAYOUT()
{
    m_raGrid->Unbind( wxEVT_GRID_CELL_LEFT_CLICK, &DIALOG_MULTICHANNEL_REPEAT_LAYOUT::OnGridCellLeftClick, this );
    m_raGrid->PopEventHandler( true );
}

bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataFromWindow()
{
    auto data = m_parentTool->GetData();

    for( size_t i = 0; i < m_targetRAs.size(); i++ )
    {
        wxString doCopy = m_raGrid->GetCellValue( i, 0 );

        data->m_compatMap[m_targetRAs[i].m_targetRA].m_doCopy = !doCopy.CompareTo( wxT( "1" ) ) ? true : false;
    }

    data->m_options.m_copyPlacement = m_cbCopyPlacement->GetValue();
    data->m_options.m_copyRouting = m_cbCopyRouting->GetValue();
    data->m_options.m_connectedRoutingOnly = m_cbCopyOnlyConnectedRouting->GetValue();
    data->m_options.m_copyOtherItems = m_cbCopyOtherItems->GetValue();
    data->m_options.m_groupItems = m_cbGroupItems->GetValue();
    data->m_options.m_includeLockedItems = m_cbIncludeLockedComponents->GetValue();

    if( m_refAnchorFp->GetString( m_refAnchorFp->GetSelection() ) == "" )
    {
        data->m_options.m_anchorFp = nullptr;
    }
    else
    {
        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            if( fp->GetReference() == m_refAnchorFp->GetString( m_refAnchorFp->GetSelection() ) )
                data->m_options.m_anchorFp = fp;
        }
    }

    return true;
}


bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataToWindow()
{
    RULE_AREAS_DATA* data = m_parentTool->GetData();

    for( const auto& [ruleArea, ruleAreaData] : data->m_compatMap )
    {
        TABLE_ENTRY ent;

        ent.m_doCopy = ruleAreaData.m_isOk;
        ent.m_errMsg = ruleAreaData.m_errorMsg;
        ent.m_isOK = ruleAreaData.m_isOk;
        ent.m_raName = ruleArea->m_ruleName;
        ent.m_targetRA = ruleArea;
        ent.m_mismatchReasons = ruleAreaData.m_mismatchReasons;

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
        m_raGrid->SetCellValue( i, 2, entry.m_isOK ? _( "OK" ) : entry.m_errMsg );
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

    m_raGrid->Fit();

    m_refRAName->SetLabelText( data->m_refRA->m_zone->GetZoneName() );

    wxArrayString refFpNames;
    refFpNames.push_back( "" );

    for( FOOTPRINT* fp : data->m_refRA->m_components )
        refFpNames.push_back( fp->GetReference() );

    refFpNames.Sort();
    m_refAnchorFp->Set( refFpNames );
    m_refAnchorFp->SetSelection( 0 );

    m_cbCopyPlacement->SetValue( data->m_options.m_copyPlacement );
    m_cbCopyRouting->SetValue( data->m_options.m_copyRouting );
    m_cbCopyOnlyConnectedRouting->SetValue( data->m_options.m_connectedRoutingOnly );
    m_cbGroupItems->SetValue( data->m_options.m_groupItems );
    m_cbCopyOtherItems->SetValue( data->m_options.m_copyOtherItems );
    m_cbIncludeLockedComponents->SetValue( data->m_options.m_includeLockedItems );

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

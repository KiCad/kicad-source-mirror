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
#include <grid_tricks.h>
#include <pcb_edit_frame.h>
#include <tools/multichannel_tool.h>
#include <zone.h>
#include <board.h>


DIALOG_MULTICHANNEL_REPEAT_LAYOUT::DIALOG_MULTICHANNEL_REPEAT_LAYOUT (
        PCB_BASE_FRAME* aFrame,
        MULTICHANNEL_TOOL *aParentTool ) :
        DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE( aFrame ),
        m_parentTool( aParentTool )
{
    m_board = aFrame->GetBoard();
    auto data = m_parentTool->GetData();
    m_refRAName->SetLabelText( data->m_refRA->m_area->GetZoneName() );

    for( auto& ra : data->m_compatMap )
    {
        TABLE_ENTRY ent;

        ent.m_doCopy = ra.second.m_isOk;
        ent.m_errMsg = ra.second.m_errorMsg;
        ent.m_isOK = ra.second.m_isOk;
        ent.m_raName = ra.first->m_ruleName;
        ent.m_targetRA = ra.first;

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

    int i = 0;

    m_raGrid->PushEventHandler( new GRID_TRICKS( static_cast<WX_GRID*>( m_raGrid ) ) );
    m_raGrid->ClearGrid();
    m_raGrid->EnableEditing( true );
    m_raGrid->HideRowLabels();
    m_raGrid->SetColLabelValue( 0, wxT("Copy") );
    m_raGrid->SetColLabelValue( 1, wxT("Target Rule Area") );
    m_raGrid->SetColLabelValue( 2, wxT("Status") );
    m_raGrid->SetColLabelValue( 3, wxT( "RefFp" ) );
    m_raGrid->AutoSizeColumn( 1 );
    m_raGrid->AppendRows( m_targetRAs.size() - 1 );

    for( TABLE_ENTRY& entry : m_targetRAs)
    {
        m_raGrid->SetCellValue( i, 1, entry.m_raName );
        m_raGrid->SetCellValue( i, 2, entry.m_isOK ? _("OK") : entry.m_errMsg );
        m_raGrid->SetCellRenderer( i, 0, new wxGridCellBoolRenderer);
        m_raGrid->SetCellEditor( i, 0, new wxGridCellBoolEditor);
        m_raGrid->SetCellValue( i, 0, entry.m_doCopy ? wxT("1") : wxT("") );
        i++;
    }

    m_raGrid->SetMaxSize( wxSize( -1, 400 ) );
    m_raGrid->Fit();

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

    Layout();
    SetupStandardButtons();
    finishDialogSettings();
}

DIALOG_MULTICHANNEL_REPEAT_LAYOUT::~DIALOG_MULTICHANNEL_REPEAT_LAYOUT()
{
    m_raGrid->PopEventHandler( true );
}

bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataFromWindow()
{
    auto data = m_parentTool->GetData();

    for( size_t i = 0; i < m_targetRAs.size(); i++ )
    {
        wxString doCopy = m_raGrid->GetCellValue( i, 0 );

        data->m_compatMap[m_targetRAs[i].m_targetRA].m_doCopy =
                !doCopy.CompareTo( wxT( "1" ) ) ? true : false;
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
    // fixme: I have no idea how to use this together with wxGrid so that it resizes correctly...

    //if( !wxDialog::TransferDataToWindow() )
        //return false;

    return true;
}

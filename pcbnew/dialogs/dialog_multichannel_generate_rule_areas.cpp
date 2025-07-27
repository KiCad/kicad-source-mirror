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

#include <dialogs/dialog_multichannel_generate_rule_areas.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>
#include <pcb_edit_frame.h>

#include <tools/multichannel_tool.h>

DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS( PCB_BASE_FRAME* aFrame,
                                                                                  MULTICHANNEL_TOOL* aParentTool ) :
        DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE( aFrame ),
        m_parentTool( aParentTool )
{
    m_bSizer1 = new wxBoxSizer( wxVERTICAL );
    m_bSizer2 = new wxBoxSizer( wxVERTICAL );

    // Generate the sheet source grid
    m_sheetGrid = new WX_GRID( m_panel1, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    m_sheetGrid->PushEventHandler( new GRID_TRICKS( static_cast<WX_GRID*>( m_sheetGrid ) ) );
    m_sheetGrid->CreateGrid( 0, 3 );
    m_sheetGrid->EnableEditing( false );
    m_sheetGrid->EnableGridLines( true );
    m_sheetGrid->EnableDragGridSize( false );
    m_sheetGrid->SetMargins( 0, 0 );
    m_sheetGrid->SetColSize( 0, 100 );
    m_sheetGrid->SetColSize( 1, 300 );
    m_sheetGrid->SetColSize( 2, 100 );
    m_sheetGrid->AutoSizeColumns();
    m_sheetGrid->EnableDragColMove( true );
    m_sheetGrid->EnableDragColSize( true );
    m_sheetGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_sheetGrid->AutoSizeRows();
    m_sheetGrid->EnableDragRowSize( true );
    m_sheetGrid->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_sheetGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_sheetGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_sheetGrid->EnableEditing( true );
    m_sheetGrid->HideRowLabels();
    m_sheetGrid->SetColLabelValue( 0, _( "Generate" ) );
    m_sheetGrid->SetColLabelValue( 1, _( "Sheet Path" ) );
    m_sheetGrid->SetColLabelValue( 2, _( "Sheet Name" ) );
    m_sheetGrid->AutoSizeColumn( 1 );
    m_bSizer1->Add( m_sheetGrid, 1, wxEXPAND | wxALL, 5 );
    m_panel1->SetSizer( m_bSizer1 );
    m_panel1->Layout();
    m_bSizer1->Fit( m_panel1 );

    // Generate the component class source grid
    m_componentClassGrid = new WX_GRID( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxVSCROLL );
    m_componentClassGrid->CreateGrid( 0, 2 );
    m_componentClassGrid->EnableEditing( false );
    m_componentClassGrid->EnableGridLines( true );
    m_componentClassGrid->EnableDragGridSize( false );
    m_componentClassGrid->SetMargins( 0, 0 );
    m_componentClassGrid->SetColSize( 0, 100 );
    m_componentClassGrid->SetColSize( 1, 300 );
    m_componentClassGrid->AutoSizeColumns();
    m_componentClassGrid->EnableDragColMove( true );
    m_componentClassGrid->EnableDragColSize( true );
    m_componentClassGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_componentClassGrid->AutoSizeRows();
    m_componentClassGrid->EnableDragRowSize( true );
    m_componentClassGrid->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_componentClassGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_componentClassGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_componentClassGrid->EnableEditing( true );
    m_componentClassGrid->HideRowLabels();
    m_componentClassGrid->SetColLabelValue( 0, _( "Generate" ) );
    m_componentClassGrid->SetColLabelValue( 1, _( "Component Class" ) );
    m_componentClassGrid->AutoSizeColumn( 1 );
    m_bSizer2->Add( m_componentClassGrid, 1, wxEXPAND | wxALL, 5 );
    m_panel2->SetSizer( m_bSizer2 );
    m_panel2->Layout();
    m_bSizer2->Fit( m_panel2 );

    // Generate the group source grid
    m_groupGrid = new WX_GRID( m_sourceNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
    m_groupGrid->CreateGrid( 0, 2 );
    m_groupGrid->EnableEditing( false );
    m_groupGrid->EnableGridLines( true );
    m_groupGrid->EnableDragGridSize( false );
    m_groupGrid->SetMargins( 0, 0 );
    m_groupGrid->SetColSize( 0, 100 );
    m_groupGrid->SetColSize( 1, 300 );
    m_groupGrid->AutoSizeColumns();
    m_groupGrid->EnableDragColMove( true );
    m_groupGrid->EnableDragColSize( true );
    m_groupGrid->SetColLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_groupGrid->AutoSizeRows();
    m_groupGrid->EnableDragRowSize( true );
    m_groupGrid->SetRowLabelSize( wxGRID_AUTOSIZE );
    m_groupGrid->SetRowLabelAlignment( wxALIGN_CENTER, wxALIGN_CENTER );
    m_groupGrid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_TOP );
    m_groupGrid->EnableEditing( true );
    m_groupGrid->HideRowLabels();
    m_groupGrid->SetColLabelValue( 0, _( "Generate" ) );
    m_groupGrid->SetColLabelValue( 1, _( "Name" ) );
    m_groupGrid->AutoSizeColumn( 1 );
    m_sourceNotebook->AddPage( m_groupGrid, _( "Groups" ) );

    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    int sheetRowIdx = 0;
    int componentClassRowIdx = 0;
    int groupIdx = 0;

    for( RULE_AREA& ruleArea : raData->m_areas )
    {
        if( ruleArea.m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
            sheetRowIdx++;
        else if( ruleArea.m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
            componentClassRowIdx++;
        else if( ruleArea.m_sourceType == PLACEMENT_SOURCE_T::GROUP_PLACEMENT )
            groupIdx++;
    }

    if( sheetRowIdx > 0 )
        m_sheetGrid->AppendRows( sheetRowIdx );

    if( componentClassRowIdx > 0 )
        m_componentClassGrid->AppendRows( componentClassRowIdx );

    if( groupIdx > 0 )
        m_groupGrid->AppendRows( groupIdx );

    sheetRowIdx = 0;
    componentClassRowIdx = 0;
    groupIdx = 0;

    for( RULE_AREA& ruleArea : raData->m_areas )
    {
        if( ruleArea.m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
        {
            m_sheetGrid->SetCellValue( sheetRowIdx, 1, ruleArea.m_sheetPath );
            m_sheetGrid->SetCellValue( sheetRowIdx, 2, ruleArea.m_sheetName );
            m_sheetGrid->SetCellRenderer( sheetRowIdx, 0, new wxGridCellBoolRenderer );
            m_sheetGrid->SetCellEditor( sheetRowIdx, 0, new wxGridCellBoolEditor );
            m_sheetGrid->SetCellValue( sheetRowIdx, 0, ruleArea.m_generateEnabled ? wxT( "1" ) : wxT( "" ) );
            sheetRowIdx++;
        }
        else if( ruleArea.m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
        {
            m_componentClassGrid->SetCellValue( componentClassRowIdx, 1, ruleArea.m_componentClass );
            m_componentClassGrid->SetCellRenderer( componentClassRowIdx, 0, new wxGridCellBoolRenderer );
            m_componentClassGrid->SetCellEditor( componentClassRowIdx, 0, new wxGridCellBoolEditor );
            m_componentClassGrid->SetCellValue( componentClassRowIdx, 0, ruleArea.m_generateEnabled ? wxT( "1" )
                                                                                                    : wxT( "" ) );
            componentClassRowIdx++;
        }
        else
        {
            m_groupGrid->SetCellValue( groupIdx, 1, ruleArea.m_groupName );
            m_groupGrid->SetCellRenderer( groupIdx, 0, new wxGridCellBoolRenderer );
            m_groupGrid->SetCellEditor( groupIdx, 0, new wxGridCellBoolEditor );
            m_groupGrid->SetCellValue( groupIdx, 0, ruleArea.m_generateEnabled ? wxT( "1" ) : wxT( "" ) );
            groupIdx++;
        }
    }

    m_sheetGrid->Fit();
    m_componentClassGrid->Fit();
    m_groupGrid->SetMaxSize( wxSize( -1, 800 ) );
    m_groupGrid->Fit();
    m_cbGroupItems->SetValue( raData->m_options.m_groupItems );
    m_cbReplaceExisting->SetValue( raData->m_replaceExisting );
    Layout();

    if( m_sheetGrid->GetNumberRows() == 1 )
    {
        if( m_componentClassGrid->GetNumberRows() > 0 )
            m_sourceNotebook->SetSelection( 1 );
        else if( m_groupGrid->GetNumberRows() > 0 )
            m_sourceNotebook->SetSelection( 2 );
    }

    SetupStandardButtons();
    finishDialogSettings();
}


DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::~DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS()
{
    m_sheetGrid->PopEventHandler( true );
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataFromWindow()
{
    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    int sheetRowIdx = 0;
    int componentClassRowIdx = 0;
    int groupIdx = 0;

    for( size_t i = 0; i < raData->m_areas.size(); i++ )
    {
        wxString enabled;

        if( raData->m_areas[i].m_sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
        {
            enabled = m_sheetGrid->GetCellValue( sheetRowIdx, 0 );
            sheetRowIdx++;
        }
        else if( raData->m_areas[i].m_sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
        {
            enabled = m_componentClassGrid->GetCellValue( componentClassRowIdx, 0 );
            componentClassRowIdx++;
        }
        else
        {
            enabled = m_groupGrid->GetCellValue( groupIdx, 0 );
            groupIdx++;
        }

        raData->m_areas[i].m_generateEnabled = ( !enabled.CompareTo( wxT( "1" ) ) ) ? true : false;
    }

    raData->m_replaceExisting = m_cbReplaceExisting->GetValue();

    // Don't allow grouping for groups
    if( m_sourceNotebook->GetSelection() == 2 )
        raData->m_options.m_groupItems = false;
    else
        raData->m_options.m_groupItems = m_cbGroupItems->GetValue();

    return true;
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataToWindow()
{
    // fixme: no idea how to make the wxGrid autoresize to the actual window width when setting
    // grid cells from within this method.
    return true;
}


void DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::OnNotebookPageChanged( wxNotebookEvent& event )
{
    if( event.GetSelection() == 2 )
        m_cbGroupItems->Disable();
    else
        m_cbGroupItems->Enable();
}

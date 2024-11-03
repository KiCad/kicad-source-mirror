/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 John Beard, john.j.beard@gmail.com
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>

#include <tools/multichannel_tool.h>

DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS(
        PCB_BASE_FRAME* aFrame,
        MULTICHANNEL_TOOL* aParentTool ) : 
        DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE( aFrame ),
        m_parentTool( aParentTool )
{
    // Generate the sheet source grid
    m_sheetGrid = new WX_GRID( m_sourceNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
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
    m_sheetGrid->SetColLabelValue( 0, wxT( "Generate" ) );
    m_sheetGrid->SetColLabelValue( 1, wxT( "Sheet Path" ) );
    m_sheetGrid->SetColLabelValue( 2, wxT( "Sheet Name" ) );
    m_sheetGrid->AutoSizeColumn( 1 );
    m_sourceNotebook->AddPage( m_sheetGrid, _( "Sheets" ) );

    // Generate the component class source grid
    m_componentClassGrid =
            new WX_GRID( m_sourceNotebook, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0 );
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
    m_componentClassGrid->SetColLabelValue( 0, wxT( "Generate" ) );
    m_componentClassGrid->SetColLabelValue( 1, wxT( "Component Class" ) );
    m_componentClassGrid->AutoSizeColumn( 1 );
    m_sourceNotebook->AddPage( m_componentClassGrid, _( "Component Classes" ) );

    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    int sheetRowIdx = 0;
    int componentClassRowIdx = 0;

    for( RULE_AREA& ruleArea : raData->m_areas )
    {
        if( ruleArea.m_sourceType == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
            sheetRowIdx++;
        else
            componentClassRowIdx++;
    }

    if( sheetRowIdx > 0 )
        m_sheetGrid->AppendRows( sheetRowIdx );

    if( componentClassRowIdx > 0 )
        m_componentClassGrid->AppendRows( componentClassRowIdx );

    sheetRowIdx = 0;
    componentClassRowIdx = 0;

    for( RULE_AREA& ruleArea : raData->m_areas )
    {
        if( ruleArea.m_sourceType == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
        {
            m_sheetGrid->SetCellValue( sheetRowIdx, 1, ruleArea.m_sheetPath );
            m_sheetGrid->SetCellValue( sheetRowIdx, 2, ruleArea.m_sheetName );
            m_sheetGrid->SetCellRenderer( sheetRowIdx, 0, new wxGridCellBoolRenderer );
            m_sheetGrid->SetCellEditor( sheetRowIdx, 0, new wxGridCellBoolEditor );
            m_sheetGrid->SetCellValue( sheetRowIdx, 0,
                                       ruleArea.m_generateEnabled ? wxT( "1" ) : wxT( "" ) );
            sheetRowIdx++;
        }
        else
        {
            m_componentClassGrid->SetCellValue( componentClassRowIdx, 1,
                                                ruleArea.m_componentClass );
            m_componentClassGrid->SetCellRenderer( componentClassRowIdx, 0,
                                                   new wxGridCellBoolRenderer );
            m_componentClassGrid->SetCellEditor( componentClassRowIdx, 0,
                                                 new wxGridCellBoolEditor );
            m_componentClassGrid->SetCellValue(
                    componentClassRowIdx, 0, ruleArea.m_generateEnabled ? wxT( "1" ) : wxT( "" ) );
            componentClassRowIdx++;
        }
    }

    m_sheetGrid->SetMaxSize( wxSize( -1, 800 ) );
    m_sheetGrid->Fit();
    m_componentClassGrid->SetMaxSize( wxSize( -1, 800 ) );
    m_componentClassGrid->Fit();
    m_cbGroupItems->SetValue( raData->m_options.m_groupItems );
    m_cbReplaceExisting->SetValue( raData->m_replaceExisting );

    Layout();

    if( m_sheetGrid->GetNumberRows() == 1 && m_componentClassGrid->GetNumberRows() > 0 )
        m_sourceNotebook->SetSelection( 1 );

    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataFromWindow()
{
    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    int sheetRowIdx = 0;
    int componentClassRowIdx = 0;

    for( size_t i = 0; i < raData->m_areas.size(); i++ )
    {
        wxString enabled;

        if( raData->m_areas[i].m_sourceType == RULE_AREA_PLACEMENT_SOURCE_TYPE::SHEETNAME )
        {
            enabled = m_sheetGrid->GetCellValue( sheetRowIdx, 0 );
            sheetRowIdx++;
        }
        else
        {
            enabled = m_componentClassGrid->GetCellValue( componentClassRowIdx, 0 );
            componentClassRowIdx++;
        }

        raData->m_areas[i].m_generateEnabled = ( !enabled.CompareTo( wxT( "1" ) ) ) ? true : false;
    }

    raData->m_replaceExisting = m_cbReplaceExisting->GetValue();
    raData->m_options.m_groupItems = m_cbGroupItems->GetValue();

    return true;
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataToWindow()
{
    // fixme: no idea how to make the wxGrid autoresize to the actual window width when setting
    // grid cells from within this method.
    return true;
}

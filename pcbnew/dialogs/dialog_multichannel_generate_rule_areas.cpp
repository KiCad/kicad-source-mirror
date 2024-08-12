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
    int i = 0;

    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    m_sheetsGrid->EnableEditing( true );
    m_sheetsGrid->HideRowLabels();
    m_sheetsGrid->SetColLabelValue( 0, wxT("Generate") );
    m_sheetsGrid->SetColLabelValue( 1, wxT("Sheet Path") );
    m_sheetsGrid->SetColLabelValue( 2, wxT("Sheet Name") );
    m_sheetsGrid->AppendRows( raData->m_areas.size() - 1 );
    m_sheetsGrid->AutoSizeColumn( 1 );

    for( RULE_AREA& sheet : raData->m_areas )
    {
        m_sheetsGrid->SetCellValue( i, 1, sheet.m_sheetPath );
        m_sheetsGrid->SetCellValue( i, 2, sheet.m_sheetName );
        m_sheetsGrid->SetCellRenderer( i, 0, new wxGridCellBoolRenderer );
        m_sheetsGrid->SetCellEditor( i, 0, new wxGridCellBoolEditor );
        m_sheetsGrid->SetCellValue( i, 0, sheet.m_generateEnabled ? wxT("1") : wxT( "" ) );
        i++;
    }

    m_sheetsGrid->SetMaxSize( wxSize( -1, 800 ) );
    m_sheetsGrid->Fit();
    m_cbGroupItems->SetValue( raData->m_groupItems );
    m_cbReplaceExisting->SetValue( raData->m_replaceExisting );

    Layout();
    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataFromWindow()
{
    RULE_AREAS_DATA* raData = m_parentTool->GetData();

    for( size_t i = 0; i < raData->m_areas.size(); i++ )
    {
        wxString enabled = m_sheetsGrid->GetCellValue( i, 0 );
        raData->m_areas[i].m_generateEnabled = ( !enabled.CompareTo( wxT( "1" ) ) ) ? true : false;
    }

    raData->m_replaceExisting = m_cbReplaceExisting->GetValue();
    raData->m_groupItems = m_cbGroupItems->GetValue();

    return true;
}


bool DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS::TransferDataToWindow()
{
    // fixme: no idea how to make the wxGrid autoresize to the actual window width when setting
    // grid cells from within this method.
    return true;
}

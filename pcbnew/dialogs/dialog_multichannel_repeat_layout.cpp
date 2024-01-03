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

#include <dialogs/dialog_multichannel_repeat_layout.h>
#include <widgets/wx_grid.h>
#include <pcb_edit_frame.h>

DIALOG_MULTICHANNEL_REPEAT_LAYOUT::DIALOG_MULTICHANNEL_REPEAT_LAYOUT (
        PCB_BASE_FRAME* aFrame,
        MULTICHANNEL_TOOL *aParentTool ) :
        DIALOG_MULTICHANNEL_REPEAT_LAYOUT_BASE( aFrame )
{
#if 0
    int i = 0;

    m_sheetsGrid->CreateGrid( 10, 3 );
    m_sheetsGrid->EnableEditing( true );
    m_sheetsGrid->HideRowLabels();
    m_sheetsGrid->SetColLabelValue( 0, wxT("Generate") );
    m_sheetsGrid->SetColLabelValue( 1, wxT("Sheet Path") );
    m_sheetsGrid->SetColLabelValue( 2, wxT("Sheet Name") );
    m_sheetsGrid->AppendRows( m_data.m_sheets.size() - 1 );
    for( auto& sheet : m_data.m_sheets )
    {

        m_sheetsGrid->SetCellValue( i, 1, sheet.m_sheetPath );
        m_sheetsGrid->SetCellValue( i, 2, sheet.m_sheetName );
        m_sheetsGrid->SetCellRenderer( i, 0, new wxGridCellBoolRenderer);
        m_sheetsGrid->SetCellEditor( i, 0, new wxGridCellBoolEditor);
        m_sheetsGrid->SetCellValue( i, 0, wxT("1") );

        i++;
    }

    m_sheetsGrid->SetMaxSize( wxSize( -1, 800 ) );
    m_sheetsGrid->Fit();

    Layout();
#endif
    SetupStandardButtons();
    finishDialogSettings();
}


bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataFromWindow()
{
#if 0
    for( int i = 0; i < m_data.m_sheets.size(); i++)
    {
        wxString enabled = m_sheetsGrid->GetCellValue( i, 0 );

        m_data.m_sheets[i].m_generateEnabled = enabled.CompareTo( wxT("1") ) ? true : false;

        //printf("i %d en '%s'\n", i, enabled.c_str().AsChar() );
    }

    m_data.m_replaceExisting = m_cbReplaceExisting->GetValue();
#endif    
    return true;
}


bool DIALOG_MULTICHANNEL_REPEAT_LAYOUT::TransferDataToWindow()
{
    //if( !wxDialog::TransferDataToWindow() )
        //return false;

    return true;
}
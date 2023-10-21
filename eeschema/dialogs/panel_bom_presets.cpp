/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <panel_bom_presets.h>

#include <wx/grid.h>
#include <widgets/wx_grid.h>
#include <bitmaps.h>
#include <widgets/std_bitmap_button.h>


PANEL_BOM_PRESETS::PANEL_BOM_PRESETS( wxWindow* aWindow, SCHEMATIC_SETTINGS& aSettings ) :
        PANEL_BOM_PRESETS_BASE( aWindow ), m_settings( aSettings )
{
    m_btnDeleteBomPreset->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
    m_btnDeleteBomFmtPreset->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );
}


bool PANEL_BOM_PRESETS::TransferDataToWindow()
{
    m_bomPresets = m_settings.m_BomPresets;
    m_bomFmtPresets = m_settings.m_BomFmtPresets;

    BuildGrid();

    return true;
}


bool PANEL_BOM_PRESETS::TransferDataFromWindow()
{
    m_settings.m_BomPresets = m_bomPresets;
    m_settings.m_BomFmtPresets = m_bomFmtPresets;

    return true;
}


void PANEL_BOM_PRESETS::BuildGrid()
{
    m_bomPresetsGrid->ClearRows();
    m_bomFmtPresetsGrid->ClearRows();

    for( const BOM_PRESET& p : m_bomPresets )
    {
        m_bomPresetsGrid->AppendRows( 1 );
        m_bomPresetsGrid->SetCellValue( m_bomPresetsGrid->GetNumberRows() - 1, 0, p.name );
    }

    for( const BOM_FMT_PRESET& p : m_bomFmtPresets )
    {
        m_bomFmtPresetsGrid->AppendRows( 1 );
        m_bomFmtPresetsGrid->SetCellValue( m_bomFmtPresetsGrid->GetNumberRows() - 1, 0, p.name );
    }
}


void PANEL_BOM_PRESETS::OnDeleteBomPreset( wxCommandEvent& event )
{
    int curRow = m_bomPresetsGrid->GetGridCursorRow();

    if( curRow < 0 || m_bomPresetsGrid->GetNumberRows() <= curRow )
        return;

    m_bomPresetsGrid->DeleteRows( curRow, 1 );
    m_bomPresets.erase( m_bomPresets.begin() + curRow );
}


void PANEL_BOM_PRESETS::OnDeleteBomFmtPreset( wxCommandEvent& event )
{
    int curRow = m_bomFmtPresetsGrid->GetGridCursorRow();

    if( curRow < 0 || m_bomFmtPresetsGrid->GetNumberRows() <= curRow )
        return;

    m_bomFmtPresetsGrid->DeleteRows( curRow, 1 );
    // Erase the bom preset from the bom presets list.
    m_bomFmtPresets.erase( m_bomFmtPresets.begin() + curRow );
}


void PANEL_BOM_PRESETS::ImportBomPresetsFrom( SCHEMATIC_SETTINGS& aSettings )
{
    m_bomPresets = aSettings.m_BomPresets;
    BuildGrid();
}


void PANEL_BOM_PRESETS::ImportBomFmtPresetsFrom( SCHEMATIC_SETTINGS& aSettings )
{
    m_bomFmtPresets = aSettings.m_BomFmtPresets;
    BuildGrid();
}

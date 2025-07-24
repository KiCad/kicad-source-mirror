/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2023 Mike Williams <mike@mikebwilliams.com>
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

    m_bomPresetsGrid->SetUseNativeColLabels();
    m_bomFmtPresetsGrid->SetUseNativeColLabels();
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
    m_bomPresetsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_bomPresetsGrid->DeleteRows( row, 1 );
                m_bomPresets.erase( m_bomPresets.begin() + row );
            } );
}


void PANEL_BOM_PRESETS::OnDeleteBomFmtPreset( wxCommandEvent& event )
{
    m_bomFmtPresetsGrid->OnDeleteRows(
            [&]( int row )
            {
                m_bomFmtPresetsGrid->DeleteRows( row, 1 );
                m_bomFmtPresets.erase( m_bomFmtPresets.begin() + row );
            } );
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

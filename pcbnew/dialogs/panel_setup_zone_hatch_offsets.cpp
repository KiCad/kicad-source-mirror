/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include "panel_setup_zone_hatch_offsets.h"
#include <grid_layer_box_helpers.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <widgets/wx_grid.h>
#include <grid_tricks.h>



PANEL_SETUP_ZONE_HATCH_OFFSETS::PANEL_SETUP_ZONE_HATCH_OFFSETS( wxWindow* aParentWindow, PCB_BASE_FRAME* aFrame,
                                                                BOARD_DESIGN_SETTINGS& aBrdSettings ) :
        PANEL_SETUP_ZONE_HATCH_OFFSETS_BASE( aParentWindow ),
        m_frame( aFrame ),
        m_brdSettings( &aBrdSettings )
{
    m_layerPropsTable = new LAYER_PROPERTIES_GRID_TABLE( m_frame,
            [&]() -> LSET
            {
                return LSET();
            } );

    m_layerOffsetsGrid->SetTable( m_layerPropsTable, true );
    m_layerOffsetsGrid->PushEventHandler( new GRID_TRICKS( m_layerOffsetsGrid ) );
    m_layerOffsetsGrid->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    attr->SetReadOnly();
    m_layerOffsetsGrid->SetColAttr( 0, attr );
}


PANEL_SETUP_ZONE_HATCH_OFFSETS::~PANEL_SETUP_ZONE_HATCH_OFFSETS()
{
    m_layerOffsetsGrid->PopEventHandler( true );
}


bool PANEL_SETUP_ZONE_HATCH_OFFSETS::TransferDataToWindow()
{
    for( PCB_LAYER_ID layer : LSET::AllCuMask().UIOrder() )
    {
        if( m_brdSettings->IsLayerEnabled( layer ) )
        {
            if( m_brdSettings->m_ZoneLayerProperties.contains( layer ) )
                m_layerPropsTable->AddItem( layer, m_brdSettings->m_ZoneLayerProperties.at( layer ) );
            else
                m_layerPropsTable->AddItem( layer, ZONE_LAYER_PROPERTIES() );
        }
    }

    Layout();

    return true;
}


void PANEL_SETUP_ZONE_HATCH_OFFSETS::SyncCopperLayers( int aCopperLayerCount )
{
    if( m_layerPropsTable->GetNumberRows() != aCopperLayerCount )
    {
        LSET enabled = LSET::AllCuMask( aCopperLayerCount );
        LSET existing;

        for( int row = m_layerPropsTable->GetNumberRows() - 1; row >= 0; --row )
        {
            PCB_LAYER_ID layer = m_layerPropsTable->GetItems()[row].first;

            if( enabled.test( layer ) )
                existing.set( layer );
            else
                m_layerPropsTable->DeleteRows( row, 1 );
        }

        for( PCB_LAYER_ID layer : enabled )
        {
            if( !existing.test( layer ) )
                m_layerPropsTable->AddItem( layer, ZONE_LAYER_PROPERTIES() );
        }

        Layout();
    }
}


bool PANEL_SETUP_ZONE_HATCH_OFFSETS::TransferDataFromWindow()
{
    if( !m_layerOffsetsGrid->CommitPendingChanges() )
        return false;

    for( const auto& [layer, props] : m_layerPropsTable->GetItems() )
        m_brdSettings->m_ZoneLayerProperties[layer] = props;

    return true;
}


void PANEL_SETUP_ZONE_HATCH_OFFSETS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_brdSettings;

    m_brdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_brdSettings = savedSettings;
}

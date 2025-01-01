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

#include "panel_setup_dimensions.h"

#include <pcb_edit_frame.h>
#include <board_design_settings.h>


PANEL_SETUP_DIMENSIONS::PANEL_SETUP_DIMENSIONS( wxWindow* aParentWindow, UNITS_PROVIDER& aUnitsProvider,
                                                BOARD_DESIGN_SETTINGS& aBrdSettings ) :
        PANEL_SETUP_DIMENSIONS_BASE( aParentWindow ),
        m_parentWindow( aParentWindow ),
        m_unitsProvider( aUnitsProvider ),
        m_brdSettings( &aBrdSettings ),
        m_arrowLength( &m_unitsProvider, aParentWindow, m_lblArrowLength, m_dimensionArrowLength, m_arrowLengthUnits ),
        m_extensionOffset( &m_unitsProvider, aParentWindow, m_lblExtensionOffset, m_dimensionExtensionOffset,
                           m_dimensionExtensionOffsetUnits )
{
    m_parentWindow->Bind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_DIMENSIONS::onUnitsChanged, this );
}


PANEL_SETUP_DIMENSIONS::~PANEL_SETUP_DIMENSIONS()
{
    m_parentWindow->Unbind( EDA_EVT_UNITS_CHANGED, &PANEL_SETUP_DIMENSIONS::onUnitsChanged, this );
}


void PANEL_SETUP_DIMENSIONS::onUnitsChanged( wxCommandEvent& aEvent )
{
    BOARD_DESIGN_SETTINGS  tempBDS( nullptr, "dummy" );
    BOARD_DESIGN_SETTINGS* saveBDS = m_brdSettings;

    m_brdSettings = &tempBDS;       // No, address of stack var does not escape function

    TransferDataFromWindow();
    TransferDataToWindow();

    m_brdSettings = saveBDS;

    aEvent.Skip();
}


bool PANEL_SETUP_DIMENSIONS::TransferDataToWindow()
{
    LoadFromSettings( *m_brdSettings );
    return true;
}


void PANEL_SETUP_DIMENSIONS::LoadFromSettings( const BOARD_DESIGN_SETTINGS& aBrdSettings )
{
    m_dimensionUnits->SetSelection( static_cast<int>( aBrdSettings.m_DimensionUnitsMode ) );
    m_dimensionUnitsFormat->SetSelection( static_cast<int>( aBrdSettings.m_DimensionUnitsFormat ) );
    m_dimensionPrecision->SetSelection( static_cast<int>( aBrdSettings.m_DimensionPrecision ) );
    m_dimensionSuppressZeroes->SetValue( aBrdSettings.m_DimensionSuppressZeroes );

    int position = static_cast<int>( aBrdSettings.m_DimensionTextPosition );
    m_dimensionTextPositionMode->SetSelection( position );

    m_dimensionTextKeepAligned->SetValue( aBrdSettings.m_DimensionKeepTextAligned );
    m_arrowLength.SetValue( aBrdSettings.m_DimensionArrowLength );

    m_extensionOffset.SetValue( aBrdSettings.m_DimensionExtensionOffset );
}


bool PANEL_SETUP_DIMENSIONS::TransferDataFromWindow()
{
    int mode                                  = m_dimensionUnits->GetSelection();
    m_brdSettings->m_DimensionUnitsMode       = static_cast<DIM_UNITS_MODE>( mode );
    int format                                = m_dimensionUnitsFormat->GetSelection();
    m_brdSettings->m_DimensionUnitsFormat     = static_cast<DIM_UNITS_FORMAT>( format );
    int precision                             = m_dimensionPrecision->GetSelection();
    m_brdSettings->m_DimensionPrecision       = static_cast<DIM_PRECISION>( precision );
    m_brdSettings->m_DimensionSuppressZeroes  = m_dimensionSuppressZeroes->GetValue();
    int position                              = m_dimensionTextPositionMode->GetSelection();
    m_brdSettings->m_DimensionTextPosition    = static_cast<DIM_TEXT_POSITION>( position );
    m_brdSettings->m_DimensionKeepTextAligned = m_dimensionTextKeepAligned->GetValue();
    m_brdSettings->m_DimensionArrowLength     = m_arrowLength.GetValue();
    m_brdSettings->m_DimensionExtensionOffset = m_extensionOffset.GetValue();

    return true;
}

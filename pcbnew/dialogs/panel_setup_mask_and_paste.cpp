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


#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <dialogs/dialog_text_entry.h>
#include <panel_setup_mask_and_paste.h>


PANEL_SETUP_MASK_AND_PASTE::PANEL_SETUP_MASK_AND_PASTE( wxWindow* aParentWindow,
                                                        PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_MASK_AND_PASTE_BASE( aParentWindow ),
        m_maskExpansion( aFrame, m_maskMarginLabel, m_maskMarginCtrl, m_maskMarginUnits ),
        m_maskMinWidth( aFrame, m_maskMinWidthLabel, m_maskMinWidthCtrl, m_maskMinWidthUnits ),
        m_maskToCopperClearance( aFrame, m_maskToCopperClearanceLabel, m_maskToCopperClearanceCtrl,
                                 m_maskToCopperClearanceUnits ),
        m_pasteMargin( aFrame, m_pasteMarginLabel, m_pasteMarginCtrl, m_pasteMarginUnits )
{
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();

    m_staticTextInfoPaste->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // Update label text and tooltip for combined offset + ratio field
    m_pasteMarginLabel->SetLabel( _( "Solder paste clearance:" ) );
    m_pasteMarginLabel->SetToolTip( _( "Solder paste clearance relative to pad size.\n"
                                       "Enter an absolute value (e.g., -0.1mm), a percentage "
                                       "(e.g., -5%), or both (e.g., -0.1mm - 5%).\n"
                                       "This value can be superseded by local values for a "
                                       "footprint or a pad." ) );

    // Hide the old ratio controls - they're no longer needed
    m_pasteMarginRatioLabel->Show( false );
    m_pasteMarginRatioCtrl->Show( false );
    m_pasteMarginRatioUnits->Show( false );
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataToWindow()
{
    m_maskExpansion.SetValue( m_BrdSettings->m_SolderMaskExpansion );
    m_maskMinWidth.SetValue( m_BrdSettings->m_SolderMaskMinWidth );
    m_maskToCopperClearance.SetValue( m_BrdSettings->m_SolderMaskToCopperClearance );
    m_tentViasFront->SetValue( m_BrdSettings->m_TentViasFront );
    m_tentViasBack->SetValue( m_BrdSettings->m_TentViasBack );

    m_pasteMargin.SetOffsetValue( m_BrdSettings->m_SolderPasteMargin );
    m_pasteMargin.SetRatioValue( m_BrdSettings->m_SolderPasteMarginRatio );

    m_allowBridges->SetValue( m_BrdSettings->m_AllowSoldermaskBridgesInFPs );

    return true;
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()
    m_BrdSettings->m_SolderMaskExpansion = m_maskExpansion.GetIntValue();
    m_BrdSettings->m_SolderMaskMinWidth = m_maskMinWidth.GetIntValue();
    m_BrdSettings->m_SolderMaskToCopperClearance = m_maskToCopperClearance.GetIntValue();
    m_BrdSettings->m_TentViasFront = m_tentViasFront->GetValue();
    m_BrdSettings->m_TentViasBack = m_tentViasBack->GetValue();

    m_BrdSettings->m_SolderPasteMargin = m_pasteMargin.GetOffsetValue().value_or( 0 );
    m_BrdSettings->m_SolderPasteMarginRatio = m_pasteMargin.GetRatioValue().value_or( 0.0 );

    m_BrdSettings->m_AllowSoldermaskBridgesInFPs = m_allowBridges->GetValue();

    return true;
}


void PANEL_SETUP_MASK_AND_PASTE::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}

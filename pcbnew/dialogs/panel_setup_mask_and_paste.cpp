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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <dialogs/dialog_text_entry.h>
#include <dialogs/panel_setup_constraint_rules.h>
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

    // These two are the values the mask rules are read against, so they are set with those rules.
    // The paste clearance stays here, because one control holds it and its ratio together.
    m_maskExpansionPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_mask_expansion" ) );
    m_maskMinWidthPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_mask_min_width" ) );

    if( !m_maskExpansionPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_maskMarginCtrl, m_maskExpansionPage, 0 );

    if( !m_maskMinWidthPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_maskMinWidthCtrl, m_maskMinWidthPage, 0 );

    // Only the absolute part of this control is a constraint a rule page holds; the ratio is a
    // constraint of its own with no page, so it is only ever set here
    m_pasteMarginPage = PANEL_SETUP_CONSTRAINT_RULES::PageEditingDefault( wxS( "solder_paste_margin" ) );

    if( !m_pasteMarginPage.IsEmpty() )
        PANEL_SETUP_CONSTRAINT_RULES::LinkToPage( m_pasteMarginCtrl, m_pasteMarginPage, 0 );
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataToWindow()
{
    m_maskExpansion.SetValue( m_BrdSettings->m_SolderMaskExpansion );
    m_maskMinWidth.SetValue( m_BrdSettings->m_SolderMaskMinWidth );
    m_maskToCopperClearance.SetValue( m_BrdSettings->m_SolderMaskToCopperClearance );

    m_pasteMargin.SetOffsetValue( m_BrdSettings->m_SolderPasteMargin );
    m_pasteMargin.SetRatioValue( m_BrdSettings->m_SolderPasteMarginRatio );

    m_allowBridges->SetValue( m_BrdSettings->m_AllowSoldermaskBridgesInFPs );

    return true;
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()

    // These two can also be set with the rules read against them, so a field the user did not
    // touch here must not write back over an edit made on that page
    if( m_maskExpansion.GetIntValue() != m_maskExpansionLoaded )
    {
        m_maskExpansionLoaded = m_maskExpansion.GetIntValue();
        m_BrdSettings->m_SolderMaskExpansion = m_maskExpansionLoaded;
    }

    if( m_maskMinWidth.GetIntValue() != m_maskMinWidthLoaded )
    {
        m_maskMinWidthLoaded = m_maskMinWidth.GetIntValue();
        m_BrdSettings->m_SolderMaskMinWidth = m_maskMinWidthLoaded;
    }

    m_BrdSettings->m_SolderMaskToCopperClearance = m_maskToCopperClearance.GetIntValue();

    if( m_pasteMargin.GetOffsetValue().value_or( 0 ) != m_pasteMarginLoaded )
    {
        m_pasteMarginLoaded = m_pasteMargin.GetOffsetValue().value_or( 0 );
        m_BrdSettings->m_SolderPasteMargin = m_pasteMarginLoaded;
    }

    m_BrdSettings->m_SolderPasteMarginRatio = m_pasteMargin.GetRatioValue().value_or( 0.0 );

    m_BrdSettings->m_AllowSoldermaskBridgesInFPs = m_allowBridges->GetValue();

    return true;
}


void PANEL_SETUP_MASK_AND_PASTE::RefreshDefaults()
{
    m_maskExpansionLoaded = m_BrdSettings->m_SolderMaskExpansion;
    m_maskExpansion.SetValue( m_maskExpansionLoaded );

    m_maskMinWidthLoaded = m_BrdSettings->m_SolderMaskMinWidth;
    m_maskMinWidth.SetValue( m_maskMinWidthLoaded );

    m_pasteMarginLoaded = m_BrdSettings->m_SolderPasteMargin;
    m_pasteMargin.SetOffsetValue( m_pasteMarginLoaded );
}


void PANEL_SETUP_MASK_AND_PASTE::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}

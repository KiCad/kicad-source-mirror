/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


PANEL_SETUP_MASK_AND_PASTE::PANEL_SETUP_MASK_AND_PASTE( PAGED_DIALOG* aParent,
                                                        PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_MASK_AND_PASTE_BASE( aParent->GetTreebook() ),
        m_maskMargin( aFrame, m_MaskMarginLabel, m_MaskMarginCtrl, m_MaskMarginUnits ),
        m_maskMinWidth( aFrame, m_MaskMinWidthLabel, m_MaskMinWidthCtrl, m_MaskMinWidthUnits ),
        m_pasteMargin( aFrame, m_PasteMarginLabel, m_PasteMarginCtrl, m_PasteMarginUnits )
{
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataToWindow()
{
    m_maskMargin.SetValue( m_BrdSettings->m_SolderMaskMargin );
    m_maskMinWidth.SetValue( m_BrdSettings->m_SolderMaskMinWidth );
    m_pasteMargin.SetValue( m_BrdSettings->m_SolderPasteMargin );

    // Prefer "-0" to "0" for normally negative values
    if( m_BrdSettings->m_SolderPasteMargin == 0 )
        m_PasteMarginCtrl->SetValue( wxT( "-" ) + m_PasteMarginCtrl->GetValue() );

    // Add solder paste margin ratio in percent
    // for the usual default value 0.0, display -0.0 (or -0,0 in some countries)
    wxString msg;
    msg.Printf( wxT( "%f" ), m_BrdSettings->m_SolderPasteMarginRatio * 100.0 );

    // Sometimes Printf adds a sign if the value is small
    if(  m_BrdSettings->m_SolderPasteMarginRatio == 0.0 && msg[0] == '0' )
        m_SolderPasteMarginRatioCtrl->SetValue( wxT( "-" ) + msg );
    else
        m_SolderPasteMarginRatioCtrl->SetValue( msg );

    return true;
}


bool PANEL_SETUP_MASK_AND_PASTE::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()
    m_BrdSettings->m_SolderMaskMargin = m_maskMargin.GetValue();
    m_BrdSettings->m_SolderMaskMinWidth = m_maskMinWidth.GetValue();

    m_BrdSettings->m_SolderPasteMargin = m_pasteMargin.GetValue();

    double dtmp = 0.0;
    wxString msg = m_SolderPasteMarginRatioCtrl->GetValue();
    msg.ToDouble( &dtmp );

    // A margin ratio de -50% means no paste on a pad, the ratio must be >= 50 %
    if( dtmp < -50 )
        dtmp = -50;

    if( dtmp > +100 )
        dtmp = +100;

    m_BrdSettings->m_SolderPasteMarginRatio = dtmp / 100;

    return true;
}


void PANEL_SETUP_MASK_AND_PASTE::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}

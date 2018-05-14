/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pcbnew.h>
#include <board_design_settings.h>
#include <widgets/text_ctrl_eval.h>
#include <widgets/paged_dialog.h>
#include <footprint_edit_frame.h>

#include <panel_modedit_defaults.h>

PANEL_MODEDIT_DEFAULTS::PANEL_MODEDIT_DEFAULTS( FOOTPRINT_EDIT_FRAME* aFrame, PAGED_DIALOG* aParent) :
        PANEL_MODEDIT_DEFAULTS_BASE( aParent ),
        m_brdSettings( aFrame->GetDesignSettings() ),
        m_frame( aFrame ),
        m_lineWidth( aFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits, true, 0 ),
        m_textThickness( aFrame, m_textThickLabel, m_textThickCtrl, m_textThickUnits, true, 0 ),
        m_textWidth( aFrame, m_textWidthLabel, m_textWidthCtrl, m_textWidthUnits, true, 0 ),
        m_textHeight( aFrame, m_textHeightLabel, m_textHeightCtrl, m_textHeightUnits, true, 0 )
{}


bool PANEL_MODEDIT_DEFAULTS::TransferDataToWindow()
{
    m_lineWidth.SetValue( m_brdSettings.m_ModuleSegmentWidth );

    m_textThickness.SetValue( m_brdSettings.m_ModuleTextWidth );
    m_textWidth.SetValue( m_brdSettings.m_ModuleTextSize.x );
    m_textHeight.SetValue( m_brdSettings.m_ModuleTextSize.y );

    m_textCtrlRefText->SetValue( m_brdSettings.m_RefDefaultText );
    m_choiceLayerReference->SetSelection( m_brdSettings.m_RefDefaultlayer == F_SilkS ? 0 : 1 );
    m_choiceVisibleReference->SetSelection( m_brdSettings.m_RefDefaultVisibility ? 0 : 1 );

    m_textCtrlValueText->SetValue( m_brdSettings.m_ValueDefaultText );
    m_choiceLayerValue->SetSelection( m_brdSettings.m_ValueDefaultlayer == F_SilkS ? 0 : 1 );
    m_choiceVisibleValue->SetSelection( m_brdSettings.m_ValueDefaultVisibility ? 0 : 1 );

    return true;
}


bool PANEL_MODEDIT_DEFAULTS::TransferDataFromWindow()
{
    m_brdSettings.m_ModuleSegmentWidth = m_lineWidth.GetValue();
    m_brdSettings.m_ModuleTextWidth = m_textThickness.GetValue();
    m_brdSettings.m_ModuleTextSize.x = m_textWidth.GetValue();
    m_brdSettings.m_ModuleTextSize.y = m_textHeight.GetValue();

    m_brdSettings.m_RefDefaultText = m_textCtrlRefText->GetValue();
    m_brdSettings.m_RefDefaultlayer = m_choiceLayerReference->GetSelection() == 1 ? F_Fab : F_SilkS;
    m_brdSettings.m_RefDefaultVisibility = m_choiceVisibleReference->GetSelection() != 1;

    m_brdSettings.m_ValueDefaultText = m_textCtrlValueText->GetValue();
    m_brdSettings.m_ValueDefaultlayer = m_choiceLayerValue->GetSelection() == 1 ? F_Fab : F_SilkS;
    m_brdSettings.m_ValueDefaultVisibility = m_choiceVisibleValue->GetSelection() != 1;

    m_frame->SetDesignSettings( m_brdSettings );

    return true;
}

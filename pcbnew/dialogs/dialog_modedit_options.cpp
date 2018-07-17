/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
/**
 * @file dialog_modedit_options.cpp
 */

#include <fctsys.h>
#include <pcbnew.h>
#include <pcb_edit_frame.h>
#include <board_design_settings.h>
#include <base_units.h>
#include <widgets/text_ctrl_eval.h>

#include <footprint_edit_frame.h>

#include <dialog_modedit_options_base.h>


class DIALOG_MODEDIT_OPTIONS : public DIALOG_MODEDIT_OPTIONS_BASE
{
    BOARD_DESIGN_SETTINGS  m_brdSettings;
    FOOTPRINT_EDIT_FRAME * m_parent;

public:
    DIALOG_MODEDIT_OPTIONS( FOOTPRINT_EDIT_FRAME* aParent );

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;
};


DIALOG_MODEDIT_OPTIONS::DIALOG_MODEDIT_OPTIONS( FOOTPRINT_EDIT_FRAME* aParent ) :
    DIALOG_MODEDIT_OPTIONS_BASE( aParent )
{
    m_parent = aParent;
    m_brdSettings = m_parent->GetDesignSettings();
    m_sdbSizer1OK->SetDefault();
    GetSizer()->SetSizeHints( this );
    Centre();
}


bool DIALOG_MODEDIT_OPTIONS::TransferDataToWindow()
{
    if( !wxWindow::TransferDataToWindow() )
        return false;

    EDA_UNITS_T units = g_UserUnit;
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_parent->GetDisplayOptions();

    // Modules: graphic lines width:
    m_staticTextGrLineUnit->SetLabel( GetAbbreviatedUnitsLabel( units ) );
    PutValueInLocalUnits( *m_OptModuleGrLineWidth, m_brdSettings.m_ModuleSegmentWidth );

    // Modules: Texts: Size & width:
    m_staticTextTextWidthUnit->SetLabel( GetAbbreviatedUnitsLabel( units ) );
    PutValueInLocalUnits( *m_OptModuleTextWidth, m_brdSettings.m_ModuleTextWidth );

    m_staticTextTextVSizeUnit->SetLabel( GetAbbreviatedUnitsLabel( units ) );
    PutValueInLocalUnits( *m_OptModuleTextVSize, m_brdSettings.m_ModuleTextSize.y );

    m_staticTextTextHSizeUnit->SetLabel( GetAbbreviatedUnitsLabel( units ) );
    PutValueInLocalUnits( *m_OptModuleTextHSize, m_brdSettings.m_ModuleTextSize.x );

    // Ref: default values
    m_textCtrlRefText->SetValue( m_brdSettings.m_RefDefaultText );
    int sel = m_brdSettings.m_RefDefaultlayer == F_SilkS ? 0 : 1;
    m_choiceLayerReference->SetSelection( sel );
    sel = m_brdSettings.m_RefDefaultVisibility ? 0 : 1;
    m_choiceVisibleReference->SetSelection( sel );

    // Value: default values
    m_textCtrlValueText->SetValue( m_brdSettings.m_ValueDefaultText );
    sel = m_brdSettings.m_ValueDefaultlayer == F_SilkS ? 0 : 1;
    m_choiceLayerValue->SetSelection( sel );
    sel = m_brdSettings.m_ValueDefaultVisibility ? 0 : 1;
    m_choiceVisibleValue->SetSelection( sel );

    // Display options
    m_PolarDisplay->SetSelection( displ_opts->m_DisplayPolarCood ? 1 : 0 );
    m_UnitsSelection->SetSelection( g_UserUnit == INCHES ? 0 : 1 );

    // Editing options
    m_Segments_45_Only_Ctrl->SetValue( m_parent->Settings().m_use45DegreeGraphicSegments );
    m_MagneticPads->SetValue( m_parent->Settings().m_magneticPads == CAPTURE_ALWAYS );
    m_dragSelects->SetValue( m_parent->Settings().m_dragSelects );

    return true;
}


bool DIALOG_MODEDIT_OPTIONS::TransferDataFromWindow()
{
    if( !wxWindow::TransferDataFromWindow() )
        return false;

    m_brdSettings.m_ModuleSegmentWidth = ValueFromTextCtrl( *m_OptModuleGrLineWidth );
    m_brdSettings.m_ModuleTextWidth = ValueFromTextCtrl( *m_OptModuleTextWidth );
    m_brdSettings.m_ModuleTextSize.y = ValueFromTextCtrl( *m_OptModuleTextVSize );
    m_brdSettings.m_ModuleTextSize.x = ValueFromTextCtrl( *m_OptModuleTextHSize );

    // Ref: default values
    m_brdSettings.m_RefDefaultText = m_textCtrlRefText->GetValue();
    int sel = m_choiceLayerReference->GetSelection();
    m_brdSettings.m_RefDefaultlayer = sel == 1 ? F_Fab : F_SilkS;
    sel = m_choiceVisibleReference->GetSelection();
    m_brdSettings.m_RefDefaultVisibility = sel != 1;

    // Value: default values
    m_brdSettings.m_ValueDefaultText = m_textCtrlValueText->GetValue();
    sel = m_choiceLayerValue->GetSelection();
    m_brdSettings.m_ValueDefaultlayer = sel == 1 ? F_Fab : F_SilkS;
    sel = m_choiceVisibleValue->GetSelection();
    m_brdSettings.m_ValueDefaultVisibility = sel != 1;

    m_parent->SetDesignSettings( m_brdSettings );

    // Display options
    auto displ_opts = (PCB_DISPLAY_OPTIONS*)m_parent->GetDisplayOptions();
    displ_opts->m_DisplayPolarCood = m_PolarDisplay->GetSelection() != 0;

    EDA_UNITS_T units = ( m_UnitsSelection->GetSelection() == 0 ) ? INCHES : MILLIMETRES;

    if( units != g_UserUnit )
    {
        g_UserUnit = units;
        m_parent->ReCreateAuxiliaryToolbar();
    }

    // Editing options
    m_parent->Settings().m_use45DegreeGraphicSegments = m_Segments_45_Only_Ctrl->GetValue();
    m_parent->Settings().m_magneticPads = m_MagneticPads->GetValue() ? CAPTURE_ALWAYS : NO_EFFECT;
    m_parent->Settings().m_dragSelects = m_dragSelects->GetValue();

    return true;
}


bool InvokeFPEditorPrefsDlg( FOOTPRINT_EDIT_FRAME* aCaller )
{
    DIALOG_MODEDIT_OPTIONS dlg( aCaller );

    int ret = dlg.ShowModal();

    return ret == wxID_OK;
}

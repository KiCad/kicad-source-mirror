/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_edit_frame.h>
#include <sch_painter.h>
#include <kiface_base.h>
#include <panel_setup_formatting.h>
#include <sch_junction.h>
#include <schematic.h>
#include <schematic_settings.h>
#include <project/project_file.h>
#include <project/net_settings.h>


PANEL_SETUP_FORMATTING::PANEL_SETUP_FORMATTING( wxWindow* aWindow, SCH_EDIT_FRAME* aFrame  ) :
        PANEL_SETUP_FORMATTING_BASE( aWindow ),
        m_frame( aFrame ),
        m_textSize( aFrame, m_textSizeLabel, m_textSizeCtrl, m_textSizeUnits ),
        m_lineWidth( aFrame, m_lineWidthLabel, m_lineWidthCtrl, m_lineWidthUnits ),
        m_pinSymbolSize( aFrame, m_pinSymbolSizeLabel, m_pinSymbolSizeCtrl, m_pinSymbolSizeUnits )
{
    wxSize minSize = m_dashLengthCtrl->GetMinSize();
    int    minWidth = m_dashLengthCtrl->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_dashLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_gapLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    m_dashedLineHelp->SetFont( KIUI::GetInfoFont( this ).Italic() );
}


void PANEL_SETUP_FORMATTING::onCheckBoxIref( wxCommandEvent& event )
{
    bool enabled = m_showIntersheetsReferences->GetValue();

    m_radioFormatStandard->Enable( enabled );
    m_radioFormatAbbreviated->Enable( enabled );
    m_prefixLabel->Enable( enabled );
    m_prefixCtrl->Enable( enabled );
    m_suffixLabel->Enable( enabled );
    m_suffixCtrl->Enable( enabled );
    m_listOwnPage->Enable( enabled );
}


bool PANEL_SETUP_FORMATTING::TransferDataToWindow()
{
    SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();

    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int refStyleSelection;

    switch( LIB_SYMBOL::GetSubpartIdSeparator() )
    {
    default:
    case 0:   refStyleSelection = 0; break;
    case '.': refStyleSelection = LIB_SYMBOL::GetSubpartFirstId() == '1' ? 4 : 1; break;
    case '-': refStyleSelection = LIB_SYMBOL::GetSubpartFirstId() == '1' ? 5 : 2; break;
    case '_': refStyleSelection = LIB_SYMBOL::GetSubpartFirstId() == '1' ? 6 : 3; break;
    }

    m_choiceSeparatorRefId->SetSelection( refStyleSelection );

    m_textSize.SetUnits( EDA_UNITS::MILS );
    m_lineWidth.SetUnits( EDA_UNITS::MILS );
    m_pinSymbolSize.SetUnits( EDA_UNITS::MILS );

    m_textSize.SetValue( settings.m_DefaultTextSize );
    m_lineWidth.SetValue( settings.m_DefaultLineWidth );
    m_pinSymbolSize.SetValue( settings.m_PinSymbolSize );
    m_choiceJunctionDotSize->SetSelection( settings.m_JunctionSizeChoice );

    m_showIntersheetsReferences->SetValue( settings.m_IntersheetRefsShow );

    m_radioFormatStandard->Enable( settings.m_IntersheetRefsShow );
    m_radioFormatAbbreviated->Enable( settings.m_IntersheetRefsShow );
    m_prefixLabel->Enable( settings.m_IntersheetRefsShow );
    m_prefixCtrl->Enable( settings.m_IntersheetRefsShow );
    m_suffixLabel->Enable( settings.m_IntersheetRefsShow );
    m_suffixCtrl->Enable( settings.m_IntersheetRefsShow );
    m_listOwnPage->Enable( settings.m_IntersheetRefsShow );

    m_radioFormatStandard->SetValue( !settings.m_IntersheetRefsFormatShort );
    m_radioFormatAbbreviated->SetValue( settings.m_IntersheetRefsFormatShort );
    m_prefixCtrl->ChangeValue( settings.m_IntersheetRefsPrefix );
    m_suffixCtrl->ChangeValue( settings.m_IntersheetRefsSuffix );
    m_listOwnPage->SetValue( settings.m_IntersheetRefsListOwnPage );

#define SET_VALUE( ctrl, units, value ) \
        ctrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, units, value ) )

    SET_VALUE( m_textOffsetRatioCtrl, EDA_UNITS::PERCENT, settings.m_TextOffsetRatio * 100.0 );
    SET_VALUE( m_dashLengthCtrl, EDA_UNITS::UNSCALED, settings.m_DashedLineDashRatio );
    SET_VALUE( m_gapLengthCtrl, EDA_UNITS::UNSCALED, settings.m_DashedLineGapRatio );
    SET_VALUE( m_labelSizeRatioCtrl, EDA_UNITS::PERCENT, settings.m_LabelSizeRatio * 100.0 );

#undef SET_VALUE

    m_vPrecisionCtrl->SetValue( settings.m_OPO_VPrecision );
    m_vRangeCtrl->SetStringSelection( settings.m_OPO_VRange );
    m_iPrecisionCtrl->SetValue( settings.m_OPO_IPrecision );
    m_iRangeCtrl->SetStringSelection( settings.m_OPO_IRange );

    return true;
}


bool PANEL_SETUP_FORMATTING::TransferDataFromWindow()
{
    SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();

    // Reference style one of: "A" ".A" "-A" "_A" ".1" "-1" "_1"
    int firstRefId, refSeparator;

    switch( m_choiceSeparatorRefId->GetSelection() )
    {
    default:
    case 0: firstRefId = 'A'; refSeparator = 0; break;
    case 1: firstRefId = 'A'; refSeparator = '.'; break;
    case 2: firstRefId = 'A'; refSeparator = '-'; break;
    case 3: firstRefId = 'A'; refSeparator = '_'; break;
    case 4: firstRefId = '1'; refSeparator = '.'; break;
    case 5: firstRefId = '1'; refSeparator = '-'; break;
    case 6: firstRefId = '1'; refSeparator = '_'; break;
    }

    if( refSeparator != LIB_SYMBOL::GetSubpartIdSeparator() ||
        firstRefId != LIB_SYMBOL::GetSubpartFirstId() )
    {
        LIB_SYMBOL::SetSubpartIdNotation( refSeparator, firstRefId );
    }

    settings.m_DefaultTextSize = (int) m_textSize.GetValue();
    settings.m_DefaultLineWidth = (int) m_lineWidth.GetValue();
    settings.m_PinSymbolSize = (int) m_pinSymbolSize.GetValue();

    if( m_choiceJunctionDotSize->GetSelection() != wxNOT_FOUND )
        settings.m_JunctionSizeChoice = m_choiceJunctionDotSize->GetSelection();

    settings.m_IntersheetRefsShow        = m_showIntersheetsReferences->GetValue();
    settings.m_IntersheetRefsFormatShort = !m_radioFormatStandard->GetValue();
    settings.m_IntersheetRefsPrefix      = m_prefixCtrl->GetValue();
    settings.m_IntersheetRefsSuffix      = m_suffixCtrl->GetValue();
    settings.m_IntersheetRefsListOwnPage = m_listOwnPage->GetValue();

#define GET_VALUE( units, str ) EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, units, str )

    settings.m_TextOffsetRatio = GET_VALUE( EDA_UNITS::PERCENT, m_textOffsetRatioCtrl->GetValue() ) / 100.0;
    settings.m_DashedLineDashRatio = GET_VALUE( EDA_UNITS::UNSCALED, m_dashLengthCtrl->GetValue() );
    settings.m_DashedLineGapRatio = GET_VALUE( EDA_UNITS::UNSCALED, m_gapLengthCtrl->GetValue() );
    settings.m_LabelSizeRatio = GET_VALUE( EDA_UNITS::PERCENT, m_labelSizeRatioCtrl->GetValue() ) / 100.0;

#undef GET_VALUE

    settings.m_OPO_VPrecision = m_vPrecisionCtrl->GetValue();

    if( m_vRangeCtrl->GetSelection() == 0 )
        settings.m_OPO_VRange = wxS( "~V" );
    else
        settings.m_OPO_VRange = m_vRangeCtrl->GetStringSelection();

    settings.m_OPO_IPrecision = m_iPrecisionCtrl->GetValue();

    if( m_iRangeCtrl->GetSelection() == 0 )
        settings.m_OPO_IRange = wxS( "~A" );
    else
        settings.m_OPO_IRange = m_iRangeCtrl->GetStringSelection();

    return true;
}


void PANEL_SETUP_FORMATTING::ImportSettingsFrom( SCHEMATIC_SETTINGS& aSettings )
{
    m_textSize.SetValue( aSettings.m_DefaultTextSize );
    m_lineWidth.SetValue( aSettings.m_DefaultLineWidth );
    m_pinSymbolSize.SetValue( aSettings.m_PinSymbolSize );

    m_showIntersheetsReferences->SetValue( aSettings.m_IntersheetRefsShow );
    m_radioFormatStandard->SetValue( aSettings.m_IntersheetRefsFormatShort );
    m_radioFormatAbbreviated->SetValue( !aSettings.m_IntersheetRefsFormatShort );
    m_prefixCtrl->ChangeValue( aSettings.m_IntersheetRefsPrefix );
    m_suffixCtrl->ChangeValue( aSettings.m_IntersheetRefsSuffix );
    m_listOwnPage->SetValue( aSettings.m_IntersheetRefsListOwnPage );

#define SET_VALUE( ctrl, units, value ) \
        ctrl->SetValue( EDA_UNIT_UTILS::UI::StringFromValue( unityScale, units, value ) )

    SET_VALUE( m_textOffsetRatioCtrl, EDA_UNITS::PERCENT, aSettings.m_TextOffsetRatio * 100.0 );
    SET_VALUE( m_dashLengthCtrl, EDA_UNITS::UNSCALED, aSettings.m_DashedLineDashRatio );
    SET_VALUE( m_gapLengthCtrl, EDA_UNITS::UNSCALED, aSettings.m_DashedLineGapRatio );
    SET_VALUE( m_labelSizeRatioCtrl, EDA_UNITS::PERCENT, aSettings.m_LabelSizeRatio * 100.0 );

#undef SET_VALUE
}

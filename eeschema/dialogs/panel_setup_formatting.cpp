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
        m_pinSymbolSize( aFrame, m_pinSymbolSizeLabel, m_pinSymbolSizeCtrl, m_pinSymbolSizeUnits ),
        m_connectionGridSize( aFrame, m_connectionGridLabel, m_connectionGridCtrl,
                              m_connectionGridUnits )
{
    wxSize minSize = m_dashLengthCtrl->GetMinSize();
    int    minWidth = m_dashLengthCtrl->GetTextExtent( wxT( "XXX.XXX" ) ).GetWidth();

    m_dashLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );
    m_gapLengthCtrl->SetMinSize( wxSize( minWidth, minSize.GetHeight() ) );

    m_dashedLineHelp->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
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

    m_textSize.SetUnits( EDA_UNITS::MILS );
    m_lineWidth.SetUnits( EDA_UNITS::MILS );
    m_pinSymbolSize.SetUnits( EDA_UNITS::MILS );
    m_connectionGridSize.SetUnits( EDA_UNITS::MILS );

    m_textSize.SetValue( settings.m_DefaultTextSize );
    m_lineWidth.SetValue( settings.m_DefaultLineWidth );
    m_pinSymbolSize.SetValue( settings.m_PinSymbolSize );
    m_choiceJunctionDotSize->SetSelection( settings.m_JunctionSizeChoice );
    m_choiceHopOverSize->SetSelection( settings.m_HopOverSizeChoice );
    m_connectionGridSize.SetValue( settings.m_ConnectionGridSize );

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
    SET_VALUE( m_overbarHeightCtrl, EDA_UNITS::PERCENT,
               settings.m_FontMetrics.m_OverbarHeight * 100.0 );
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
    if( !m_connectionGridSize.Validate( MIN_CONNECTION_GRID_MILS, 10000, EDA_UNITS::MILS ) )
        return false;

    SCHEMATIC_SETTINGS& settings = m_frame->Schematic().Settings();

    settings.m_DefaultTextSize = m_textSize.GetIntValue();
    settings.m_DefaultLineWidth = m_lineWidth.GetIntValue();
    settings.m_PinSymbolSize = m_pinSymbolSize.GetIntValue();
    settings.m_ConnectionGridSize = m_connectionGridSize.GetIntValue();

    if( m_choiceJunctionDotSize->GetSelection() != wxNOT_FOUND )
        settings.m_JunctionSizeChoice = m_choiceJunctionDotSize->GetSelection();

    if( m_choiceHopOverSize->GetSelection() != wxNOT_FOUND )
        settings.m_HopOverSizeChoice = m_choiceHopOverSize->GetSelection();

    settings.m_IntersheetRefsShow        = m_showIntersheetsReferences->GetValue();
    settings.m_IntersheetRefsFormatShort = !m_radioFormatStandard->GetValue();
    settings.m_IntersheetRefsPrefix      = m_prefixCtrl->GetValue();
    settings.m_IntersheetRefsSuffix      = m_suffixCtrl->GetValue();
    settings.m_IntersheetRefsListOwnPage = m_listOwnPage->GetValue();

#define GET_VALUE( units, str ) EDA_UNIT_UTILS::UI::DoubleValueFromString( unityScale, units, str )

    settings.m_TextOffsetRatio = GET_VALUE( EDA_UNITS::PERCENT,
                                            m_textOffsetRatioCtrl->GetValue() ) / 100.0;
    settings.m_FontMetrics.m_OverbarHeight = GET_VALUE( EDA_UNITS::PERCENT,
                                                        m_overbarHeightCtrl->GetValue() ) / 100.0;
    settings.m_DashedLineDashRatio = GET_VALUE( EDA_UNITS::UNSCALED, m_dashLengthCtrl->GetValue() );
    settings.m_DashedLineGapRatio = GET_VALUE( EDA_UNITS::UNSCALED, m_gapLengthCtrl->GetValue() );
    settings.m_LabelSizeRatio = GET_VALUE( EDA_UNITS::PERCENT,
                                           m_labelSizeRatioCtrl->GetValue() ) / 100.0;

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
    m_connectionGridSize.SetValue( aSettings.m_ConnectionGridSize );

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

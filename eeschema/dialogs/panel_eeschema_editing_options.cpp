/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@verizon.net>
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

#include <pgm_base.h>
#include <layer_ids.h>
#include <settings/settings_manager.h>
#include <settings/color_settings.h>
#include <eeschema_settings.h>
#include "panel_eeschema_editing_options.h"
#include <widgets/ui_common.h>


static int arcEditModeToComboIndex( ARC_EDIT_MODE aMode )
{
    switch( aMode )
    {
    case ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS:
        return 0;
    case ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION:
        return 1;
    case ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE:
        return 2;
    default:
        wxFAIL_MSG( "Invalid ARC_EDIT_MODE" );
        return 0;
    }
};


static ARC_EDIT_MODE arcEditModeToEnum( int aIndex )
{
    switch( aIndex )
    {
    case 0:
        return ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
    case 1:
        return ARC_EDIT_MODE::KEEP_ENDPOINTS_OR_START_DIRECTION;
    case 2:
        return ARC_EDIT_MODE::KEEP_CENTER_ENDS_ADJUST_ANGLE;
    default:
        wxFAIL_MSG( wxString::Format( "Invalid index for ARC_EDIT_MODE: %d", aIndex ) );
        return ARC_EDIT_MODE::KEEP_CENTER_ADJUST_ANGLE_RADIUS;
    }
};


PANEL_EESCHEMA_EDITING_OPTIONS::PANEL_EESCHEMA_EDITING_OPTIONS( wxWindow* aWindow,
                                                                UNITS_PROVIDER* aUnitsProvider,
                                                                wxWindow* aEventSource ) :
        PANEL_EESCHEMA_EDITING_OPTIONS_BASE( aWindow ),
        m_hPitch( aUnitsProvider, aEventSource, m_hPitchLabel, m_hPitchCtrl, m_hPitchUnits ),
        m_vPitch( aUnitsProvider, aEventSource, m_vPitchLabel, m_vPitchCtrl, m_vPitchUnits )
{
    // Make the color swatch show "Clear Color" instead
    m_borderColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );
    m_backgroundColorSwatch->SetDefaultColor( COLOR4D::UNSPECIFIED );

    wxFont helpFont = KIUI::GetSmallInfoFont( this ).Italic();
    m_hint1->SetFont( helpFont );
    m_hint2->SetFont( helpFont );

    m_spinLabelRepeatStep->SetRange( -100000, 100000 );
    m_spinLabelRepeatStep->SetIncrement( 1 );

#ifdef __WXOSX_MAC__
    m_leftClickCmdsBook->SetSelection( 1 );
#else
    m_leftClickCmdsBook->SetSelection( 0 );
#endif
}


void PANEL_EESCHEMA_EDITING_OPTIONS::loadEEschemaSettings( EESCHEMA_SETTINGS* aCfg )
{
    m_hPitch.SetValue( schIUScale.MilsToIU( aCfg->m_Drawing.default_repeat_offset_x ) );
    m_vPitch.SetValue( schIUScale.MilsToIU( aCfg->m_Drawing.default_repeat_offset_y ) );
    m_spinLabelRepeatStep->SetValue( aCfg->m_Drawing.repeat_label_increment );

    COLOR_SETTINGS* settings = ::GetColorSettings( DEFAULT_THEME );
    COLOR4D         schematicBackground = settings->GetColor( LAYER_SCHEMATIC_BACKGROUND );

    m_borderColorSwatch->SetSwatchBackground( schematicBackground );
    m_borderColorSwatch->SetDefaultColor( settings->GetDefaultColor( LAYER_SHEET ) );
    m_borderColorSwatch->SetSwatchColor( aCfg->m_Drawing.default_sheet_border_color, false );

    m_backgroundColorSwatch->SetSwatchBackground( schematicBackground );
    m_backgroundColorSwatch->SetDefaultColor( settings->GetDefaultColor( LAYER_SHEET_BACKGROUND ) );
    m_backgroundColorSwatch->SetSwatchColor( aCfg->m_Drawing.default_sheet_background_color, false );

    m_choiceLineMode->SetSelection( aCfg->m_Drawing.line_mode );
    m_choiceArcMode->SetSelection( arcEditModeToComboIndex( aCfg->m_Drawing.arc_edit_mode ) );
    m_footprintPreview->SetValue( aCfg->m_Appearance.footprint_preview );
    m_neverShowRescue->SetValue( aCfg->m_RescueNeverShow );

    m_checkAutoplaceFields->SetValue( aCfg->m_AutoplaceFields.enable );
    m_checkAutoplaceJustify->SetValue( aCfg->m_AutoplaceFields.allow_rejustify );
    m_checkAutoplaceAlign->SetValue( aCfg->m_AutoplaceFields.align_to_grid );

    m_mouseDragIsDrag->SetValue( !aCfg->m_Input.drag_is_move );

    m_cbAutoStartWires->SetValue( aCfg->m_Drawing.auto_start_wires );
    m_escClearsNetHighlight->SetValue( aCfg->m_Input.esc_clears_net_highlight );
    m_checkAutoAnnotate->SetValue( aCfg->m_AnnotatePanel.automatic );
    m_checkAllowUnconstrainedPinSwaps->SetValue( aCfg->m_Input.allow_unconstrained_pin_swaps );

    m_choicePower->SetSelection( static_cast<int>( aCfg->m_Drawing.new_power_symbols ) );
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataToWindow()
{
    loadEEschemaSettings( GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) );
    return true;
}


bool PANEL_EESCHEMA_EDITING_OPTIONS::TransferDataFromWindow()
{
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        cfg->m_Drawing.new_power_symbols = static_cast<POWER_SYMBOLS>( m_choicePower->GetSelection() );

        cfg->m_Drawing.default_sheet_border_color = m_borderColorSwatch->GetSwatchColor();
        cfg->m_Drawing.default_sheet_background_color = m_backgroundColorSwatch->GetSwatchColor();

        cfg->m_Drawing.default_repeat_offset_x = schIUScale.IUToMils( m_hPitch.GetIntValue() );
        cfg->m_Drawing.default_repeat_offset_y = schIUScale.IUToMils( m_vPitch.GetIntValue() );
        cfg->m_Drawing.repeat_label_increment = m_spinLabelRepeatStep->GetValue();

        cfg->m_Drawing.line_mode = m_choiceLineMode->GetSelection();
        cfg->m_Drawing.arc_edit_mode = arcEditModeToEnum( m_choiceArcMode->GetSelection() );
        cfg->m_Appearance.footprint_preview = m_footprintPreview->GetValue();
        cfg->m_RescueNeverShow = m_neverShowRescue->GetValue();

        cfg->m_AutoplaceFields.enable = m_checkAutoplaceFields->GetValue();
        cfg->m_AutoplaceFields.allow_rejustify = m_checkAutoplaceJustify->GetValue();
        cfg->m_AutoplaceFields.align_to_grid = m_checkAutoplaceAlign->GetValue();

        cfg->m_Input.drag_is_move = !m_mouseDragIsDrag->GetValue();

        cfg->m_Drawing.auto_start_wires = m_cbAutoStartWires->GetValue();
        cfg->m_Input.esc_clears_net_highlight = m_escClearsNetHighlight->GetValue();
        cfg->m_AnnotatePanel.automatic = m_checkAutoAnnotate->GetValue();
        cfg->m_Input.allow_unconstrained_pin_swaps = m_checkAllowUnconstrainedPinSwaps->GetValue();
    }

    return true;
}


void PANEL_EESCHEMA_EDITING_OPTIONS::ResetPanel()
{
    EESCHEMA_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    loadEEschemaSettings( &cfg );
}

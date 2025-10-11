/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
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
#include <settings/settings_manager.h>
#include <eeschema_settings.h>
#include <panel_eeschema_display_options.h>
#include <dialogs/panel_gal_options.h>
#include <widgets/ui_common.h>
#include <widgets/font_choice.h>

// The "official" name of the building Kicad stroke font (always existing)
#include <font/kicad_font_name.h>


PANEL_EESCHEMA_DISPLAY_OPTIONS::PANEL_EESCHEMA_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
        PANEL_EESCHEMA_DISPLAY_OPTIONS_BASE( aParent )
{
    m_galOptsPanel = new PANEL_GAL_OPTIONS( this, aAppSettings );

    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND|wxRIGHT, 0 );

    m_highlightColorNote->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );
}


void PANEL_EESCHEMA_DISPLAY_OPTIONS::loadEEschemaSettings( EESCHEMA_SETTINGS* cfg )
{
    m_defaultFontCtrl->SetStringSelection( cfg->m_Appearance.default_font );

    if( m_defaultFontCtrl->GetSelection() < 0 )
        m_defaultFontCtrl->SetSelection( 0 );

    m_checkShowHiddenPins->SetValue( cfg->m_Appearance.show_hidden_pins );
    m_checkShowHiddenFields->SetValue( cfg->m_Appearance.show_hidden_fields );
    m_checkShowERCErrors->SetValue( cfg->m_Appearance.show_erc_errors );
    m_checkShowERCWarnings->SetValue( cfg->m_Appearance.show_erc_warnings );
    m_checkShowERCExclusions->SetValue( cfg->m_Appearance.show_erc_exclusions );
    m_cbMarkSimExclusions->SetValue( cfg->m_Appearance.mark_sim_exclusions );
    m_checkShowOPVoltages->SetValue( cfg->m_Appearance.show_op_voltages );
    m_checkShowOPCurrents->SetValue( cfg->m_Appearance.show_op_currents );
    m_checkShowPinAltModeIcons->SetValue( cfg->m_Appearance.show_pin_alt_icons );
    m_checkPageLimits->SetValue( cfg->m_Appearance.show_page_limits );

    m_checkSelDrawChildItems->SetValue( cfg->m_Selection.draw_selected_children );
    m_checkSelFillShapes->SetValue( cfg->m_Selection.fill_shapes );
    m_selWidthCtrl->SetValue( cfg->m_Selection.selection_thickness );
    m_collisionMarkerWidthCtrl->SetValue( cfg->m_Selection.drag_net_collision_width );
    m_highlightWidthCtrl->SetValue( cfg->m_Selection.highlight_thickness );
    m_highlightNetclassColors->SetValue( cfg->m_Selection.highlight_netclass_colors );
    m_colHighlightThickness->SetValue( cfg->m_Selection.highlight_netclass_colors_thickness );
    m_colHighlightTransparency->SetValue( cfg->m_Selection.highlight_netclass_colors_alpha * 100 );

    m_checkCrossProbeOnSelection->SetValue( cfg->m_CrossProbing.on_selection );
    m_checkCrossProbeCenter->SetValue( cfg->m_CrossProbing.center_on_items );
    m_checkCrossProbeZoom->SetValue( cfg->m_CrossProbing.zoom_to_fit );
    m_checkCrossProbeAutoHighlight->SetValue( cfg->m_CrossProbing.auto_highlight );
    m_checkCrossProbeFlash->SetValue( cfg->m_CrossProbing.flash_selection );
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataToWindow()
{
    loadEEschemaSettings( GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) );
    m_galOptsPanel->TransferDataToWindow();
    return true;
}


bool PANEL_EESCHEMA_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    if( EESCHEMA_SETTINGS* cfg = GetAppSettings<EESCHEMA_SETTINGS>( "eeschema" ) )
    {
        cfg->m_Appearance.default_font = m_defaultFontCtrl->GetSelection() <= 0
                                         // This is a keyword. Do not translate.
                                         ? wxString( KICAD_FONT_NAME )
                                         : m_defaultFontCtrl->GetStringSelection();
        cfg->m_Appearance.show_hidden_pins = m_checkShowHiddenPins->GetValue();
        cfg->m_Appearance.show_hidden_fields = m_checkShowHiddenFields->GetValue();
        cfg->m_Appearance.show_erc_warnings = m_checkShowERCWarnings->GetValue();
        cfg->m_Appearance.show_erc_errors = m_checkShowERCErrors->GetValue();
        cfg->m_Appearance.show_erc_exclusions = m_checkShowERCExclusions->GetValue();
        cfg->m_Appearance.mark_sim_exclusions = m_cbMarkSimExclusions->GetValue();
        cfg->m_Appearance.show_op_voltages = m_checkShowOPVoltages->GetValue();
        cfg->m_Appearance.show_op_currents = m_checkShowOPCurrents->GetValue();
        cfg->m_Appearance.show_pin_alt_icons = m_checkShowPinAltModeIcons->GetValue();
        cfg->m_Appearance.show_page_limits = m_checkPageLimits->GetValue();

        cfg->m_Selection.draw_selected_children = m_checkSelDrawChildItems->GetValue();
        cfg->m_Selection.fill_shapes = m_checkSelFillShapes->GetValue();
        cfg->m_Selection.selection_thickness = KiROUND( m_selWidthCtrl->GetValue() );
        cfg->m_Selection.drag_net_collision_width = KiROUND( m_collisionMarkerWidthCtrl->GetValue() );
        cfg->m_Selection.highlight_thickness = KiROUND( m_highlightWidthCtrl->GetValue() );
        cfg->m_Selection.highlight_netclass_colors = m_highlightNetclassColors->GetValue();
        cfg->m_Selection.highlight_netclass_colors_thickness = m_colHighlightThickness->GetValue();
        cfg->m_Selection.highlight_netclass_colors_alpha = m_colHighlightTransparency->GetValue() / 100.0;

        cfg->m_CrossProbing.on_selection = m_checkCrossProbeOnSelection->GetValue();
        cfg->m_CrossProbing.center_on_items = m_checkCrossProbeCenter->GetValue();
        cfg->m_CrossProbing.zoom_to_fit = m_checkCrossProbeZoom->GetValue();
        cfg->m_CrossProbing.auto_highlight = m_checkCrossProbeAutoHighlight->GetValue();
    cfg->m_CrossProbing.flash_selection = m_checkCrossProbeFlash->GetValue();
    }

    m_galOptsPanel->TransferDataFromWindow();

    return true;
}


void PANEL_EESCHEMA_DISPLAY_OPTIONS::ResetPanel()
{
    EESCHEMA_SETTINGS cfg;
    cfg.Load();             // Loading without a file will init to defaults

    loadEEschemaSettings( &cfg );

    m_galOptsPanel->ResetPanel( &cfg );
}

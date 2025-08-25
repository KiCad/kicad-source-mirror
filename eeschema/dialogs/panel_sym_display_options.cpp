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

#include "panel_sym_display_options.h"

#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <dialogs/panel_gal_options.h>


PANEL_SYM_DISPLAY_OPTIONS::PANEL_SYM_DISPLAY_OPTIONS( wxWindow* aParent, APP_SETTINGS_BASE* aAppSettings ) :
        PANEL_SYM_DISPLAY_OPTIONS_BASE( aParent )
{
    m_galOptsPanel = new PANEL_GAL_OPTIONS( this, aAppSettings );

    m_galOptionsSizer->Add( m_galOptsPanel, 1, wxEXPAND | wxRIGHT, 5 );
}


void PANEL_SYM_DISPLAY_OPTIONS::loadSymEditorSettings( SYMBOL_EDITOR_SETTINGS* cfg )
{
    m_checkShowHiddenPins->SetValue( cfg->m_ShowHiddenPins );
    m_checkShowHiddenFields->SetValue( cfg->m_ShowHiddenFields );
    m_showPinElectricalTypes->SetValue( cfg->m_ShowPinElectricalType );
    m_checkShowPinAltModeIcons->SetValue( cfg->m_ShowPinAltIcons );
}


bool PANEL_SYM_DISPLAY_OPTIONS::TransferDataToWindow()
{
    loadSymEditorSettings( GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) );

    m_galOptsPanel->TransferDataToWindow();

    return true;
}


bool PANEL_SYM_DISPLAY_OPTIONS::TransferDataFromWindow()
{
    if( SYMBOL_EDITOR_SETTINGS* cfg = GetAppSettings<SYMBOL_EDITOR_SETTINGS>( "symbol_editor" ) )
    {
        cfg->m_ShowHiddenPins = m_checkShowHiddenPins->GetValue();
        cfg->m_ShowHiddenFields = m_checkShowHiddenFields->GetValue();
        cfg->m_ShowPinElectricalType = m_showPinElectricalTypes->GetValue();
        cfg->m_ShowPinAltIcons = m_checkShowPinAltModeIcons->GetValue();
        m_galOptsPanel->TransferDataFromWindow();
    }

    return true;
}


void PANEL_SYM_DISPLAY_OPTIONS::ResetPanel()
{
    SYMBOL_EDITOR_SETTINGS cfg;
    cfg.Load(); // Loading without a file will init to defaults

    loadSymEditorSettings( &cfg );

    m_galOptsPanel->ResetPanel( &cfg );
}

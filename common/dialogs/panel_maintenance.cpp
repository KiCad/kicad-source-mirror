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

#include <dialogs/panel_maintenance.h>

#include <kidialog.h>
#include <pgm_base.h>
#include <kiway.h>
#include <eda_base_frame.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <widgets/paged_dialog.h>


PANEL_MAINTENANCE::PANEL_MAINTENANCE( wxWindow* aParent, EDA_BASE_FRAME* aFrame ) :
        PANEL_MAINTENANCE_BASE( aParent ),
        m_frame( aFrame )
{
}


bool PANEL_MAINTENANCE::TransferDataToWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    applySettingsToPanel( *commonSettings );

    return true;
}


bool PANEL_MAINTENANCE::TransferDataFromWindow()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    commonSettings->m_System.clear_3d_cache_interval = m_Clear3DCacheFilesOlder->GetValue();

    Pgm().GetSettingsManager().Save( commonSettings );

    return true;
}


void PANEL_MAINTENANCE::ResetPanel()
{
    COMMON_SETTINGS defaultSettings;

    defaultSettings.ResetToDefaults();

    applySettingsToPanel( defaultSettings );
}


void PANEL_MAINTENANCE::applySettingsToPanel( COMMON_SETTINGS& aSettings )
{
    m_Clear3DCacheFilesOlder->SetValue( aSettings.m_System.clear_3d_cache_interval );
}


void PANEL_MAINTENANCE::onClearFileHistory( wxCommandEvent& event )
{
    Pgm().GetSettingsManager().ClearFileHistory();

    // We also need to clear the in-memory wxWidgets histories, update menu bars, etc.
    m_frame->Kiway().ClearFileHistory();

    if( PAGED_DIALOG* dlg = PAGED_DIALOG::GetDialog( this ) )
        dlg->GetInfoBar()->ShowMessageFor( _( "File history cleared." ), 6000, wxICON_INFORMATION );
}


void PANEL_MAINTENANCE::doClearDontShowAgain()
{
    if( COMMON_SETTINGS* settings = Pgm().GetSettingsManager().GetCommonSettings() )
    {
        // intra-session do-not-show-agains
        settings->m_DoNotShowAgain = {};
        settings->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( settings ) );
    }

    // Session do-not-show-agains
    KIDIALOG::ClearDoNotShowAgainDialogs();
}


void PANEL_MAINTENANCE::onClearDontShowAgain( wxCommandEvent& event )
{
    doClearDontShowAgain();

    if( PAGED_DIALOG* dlg = PAGED_DIALOG::GetDialog( this ) )
        dlg->GetInfoBar()->ShowMessageFor( _( "\"Don't show again\" dialogs reset." ), 6000, wxICON_INFORMATION );
}


void PANEL_MAINTENANCE::doClearDialogState()
{
    doClearDontShowAgain();

    if( COMMON_SETTINGS* settings = Pgm().GetSettingsManager().GetCommonSettings() )
    {
        settings->m_dialogControlValues = {};
        settings->SaveToFile( Pgm().GetSettingsManager().GetPathForSettingsFile( settings ) );
    }
}


void PANEL_MAINTENANCE::onClearDialogState( wxCommandEvent& event )
{
    doClearDialogState();

    if( PAGED_DIALOG* dlg = PAGED_DIALOG::GetDialog( this ) )
        dlg->GetInfoBar()->ShowMessageFor( _( "All dialogs reset to defaults." ), 6000, wxICON_INFORMATION );
}


void PANEL_MAINTENANCE::onResetAll( wxCommandEvent& event )
{
    doClearDialogState();
    Pgm().GetSettingsManager().ResetToDefaults();

    // Kill changes currently in dialog
    wxQueueEvent( m_parent, new wxCommandEvent( wxEVT_COMMAND_BUTTON_CLICKED, wxID_CANCEL ) );

    // Reopen dialog
    wxQueueEvent( m_frame, new wxCommandEvent( wxEVT_COMMAND_MENU_SELECTED, wxID_PREFERENCES ) );
}

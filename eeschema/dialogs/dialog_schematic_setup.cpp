/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <dialog_sch_import_settings.h>
#include <panel_setup_severities.h>
#include <panel_setup_formatting.h>
#include <panel_setup_pinmap.h>
#include <eeschema_config.h>
#include "dialog_schematic_setup.h"
#include "panel_eeschema_template_fieldnames.h"


bool g_macHack;


DIALOG_SCHEMATIC_SETUP::DIALOG_SCHEMATIC_SETUP( SCH_EDIT_FRAME* aFrame ) :
        PAGED_DIALOG( aFrame, _( "Schematic Setup" ),
                      _( "Import Settings from Another Project..." ) ),
        m_frame( aFrame ),
        m_severities( nullptr )
{
    m_formatting = new PANEL_SETUP_FORMATTING( this, aFrame );
    m_fieldNameTemplates = new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( aFrame, this, false );
    m_pinMap = new PANEL_SETUP_PINMAP( this, aFrame );
    m_severities = new PANEL_SETUP_SEVERITIES( this, aFrame->GetErcSettings().m_Severities,
                                               ERCE_FIRST, ERCE_LAST );
    /*
     * WARNING: If you change page names you MUST update calls to DoShowSchematicSetupDialog().
     */

    m_treebook->AddPage( new wxPanel( this ), _( "General" ) );
    m_treebook->AddSubPage( m_formatting, _( "Formatting" ) );
    m_treebook->AddSubPage( m_fieldNameTemplates, _( "Field Name Templates" ) );

    m_treebook->AddPage( new wxPanel( this ),  _( "Electrical Rules" ) );
    m_treebook->AddSubPage( m_pinMap, _( "Pin Map" ) );
    m_treebook->AddSubPage( m_severities, _( "Violation Severity" ) );

	// Connect Events
	m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_SCHEMATIC_SETUP::OnPageChange ), NULL, this );

    FinishDialogSettings();
    g_macHack = true;
}


DIALOG_SCHEMATIC_SETUP::~DIALOG_SCHEMATIC_SETUP()
{
	m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_SCHEMATIC_SETUP::OnPageChange ), NULL, this );
}


void DIALOG_SCHEMATIC_SETUP::OnPageChange( wxBookCtrlEvent& event )
{
#ifdef __WXMAC__
    // Work around an OSX bug where the wxGrid children don't get placed correctly
    if( g_macHack && m_treebook->GetPage( event.GetSelection() ) == m_fieldNameTemplates )
    {
        m_fieldNameTemplates->SetSize( wxSize( m_fieldNameTemplates->GetSize().x - 1,
                                               m_fieldNameTemplates->GetSize().y ) );

        wxPoint pos = m_fieldNameTemplates->GetPosition();
        m_fieldNameTemplates->Move( pos.x + 6, pos.y + 6 );

        g_macHack = false;
    }
#endif
}


// Run Import Settings... action
void DIALOG_SCHEMATIC_SETUP::OnAuxiliaryAction( wxCommandEvent& event )
{
    DIALOG_SCH_IMPORT_SETTINGS importDlg( this, m_frame );

    if( importDlg.ShowModal() == wxID_CANCEL )
        return;

    wxConfigBase* cfg = new wxFileConfig( wxEmptyString, wxEmptyString, importDlg.GetFilePath() );

    // We do not want expansion of env var values when reading our project config file
    cfg->SetExpandEnvVars( false );
    cfg->SetPath( wxCONFIG_PATH_SEPARATOR );

    if( importDlg.m_formattingOpt->GetValue() )
    {
        std::vector<PARAM_CFG*> params;
        m_frame->AddFormattingParameters( params );

        wxConfigLoadParams( cfg, params, GROUP_SCH_EDIT );
        m_formatting->TransferDataToWindow();
    }

    if( importDlg.m_fieldNameTemplatesOpt->GetValue() )
    {
        TEMPLATES templateMgr;
        PARAM_CFG_FIELDNAMES param( &templateMgr );
        param.ReadParam( cfg );

        m_fieldNameTemplates->ImportSettingsFrom( &templateMgr );
    }

    if( importDlg.m_pinMapOpt->GetValue() )
    {
        // JEY TODO
    }

    if( importDlg.m_SeveritiesOpt->GetValue() )
    {
        ERC_SETTINGS settings;
        settings.LoadDefaults();
        wxConfigLoadParams( cfg, settings.GetProjectFileParameters(), GROUP_SCH_EDIT );

        m_severities->ImportSettingsFrom( settings.m_Severities );
    }

    delete cfg;
}

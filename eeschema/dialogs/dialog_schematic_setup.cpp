/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <sch_edit_frame.h>
#include <schematic.h>
#include <kiface_base.h>
#include <dialog_sch_import_settings.h>
#include <dialogs/panel_setup_netclasses.h>
#include <dialogs/panel_setup_severities.h>
#include <panel_setup_formatting.h>
#include <panel_setup_pinmap.h>
#include <erc_item.h>
#include <panel_text_variables.h>
#include <project/project_file.h>
#include <project/net_settings.h>
#include <settings/settings_manager.h>
#include "dialog_schematic_setup.h"
#include "panel_eeschema_template_fieldnames.h"
#include <wx/treebook.h>


DIALOG_SCHEMATIC_SETUP::DIALOG_SCHEMATIC_SETUP( SCH_EDIT_FRAME* aFrame ) :
        PAGED_DIALOG( aFrame, _( "Schematic Setup" ), true,
                      _( "Import Settings from Another Project..." ) ),
        m_frame( aFrame ),
        m_severities( nullptr )
{
    PROJECT_FILE& project = aFrame->Prj().GetProjectFile();
    SCHEMATIC&    schematic = aFrame->Schematic();

    m_formatting = new PANEL_SETUP_FORMATTING( m_treebook, aFrame );
    m_fieldNameTemplates = new PANEL_EESCHEMA_TEMPLATE_FIELDNAMES( aFrame, m_treebook, false );
    m_pinMap = new PANEL_SETUP_PINMAP( m_treebook, aFrame );

    m_pinToPinError = ERC_ITEM::Create( ERCE_PIN_TO_PIN_WARNING );
    m_severities = new PANEL_SETUP_SEVERITIES( this, ERC_ITEM::GetItemsWithSeverities(),
                                               schematic.ErcSettings().m_Severities,
                                               m_pinToPinError.get() );

    m_textVars = new PANEL_TEXT_VARIABLES( m_treebook, &Prj() );

    m_netclasses = new PANEL_SETUP_NETCLASSES( this, &project.NetSettings().m_NetClasses,
                                               schematic.GetNetClassAssignmentCandidates(), true );

    /*
     * WARNING: If you change page names you MUST update calls to ShowSchematicSetupDialog().
     */

    m_treebook->AddPage( new wxPanel( this ), _( "General" ) );
    m_treebook->AddSubPage( m_formatting, _( "Formatting" ) );
    m_treebook->AddSubPage( m_fieldNameTemplates, _( "Field Name Templates" ) );

    m_treebook->AddPage( new wxPanel( this ),  _( "Electrical Rules" ) );
    m_treebook->AddSubPage( m_severities, _( "Violation Severity" ) );
    m_treebook->AddSubPage( m_pinMap, _( "Pin Conflicts Map" ) );

    m_treebook->AddPage( new wxPanel( this ), _( "Project" ) );
    m_treebook->AddSubPage( m_netclasses, _( "Net Classes" ) );
    m_treebook->AddSubPage( m_textVars, _( "Text Variables" ) );

    for( size_t i = 0; i < m_treebook->GetPageCount(); ++i )
   	    m_macHack.push_back( true );

	// Connect Events
	m_treebook->Connect( wxEVT_TREEBOOK_PAGE_CHANGED,
                         wxBookCtrlEventHandler( DIALOG_SCHEMATIC_SETUP::OnPageChange ), nullptr,
                         this );

	finishDialogSettings();

    if( Prj().IsReadOnly() )
    {
        m_infoBar->ShowMessage( _( "Project is missing or read-only. Settings will not be "
                                   "editable." ), wxICON_WARNING );
    }
}


DIALOG_SCHEMATIC_SETUP::~DIALOG_SCHEMATIC_SETUP()
{
	m_treebook->Disconnect( wxEVT_TREEBOOK_PAGE_CHANGED,
                            wxBookCtrlEventHandler( DIALOG_SCHEMATIC_SETUP::OnPageChange ), nullptr,
                            this );
}


void DIALOG_SCHEMATIC_SETUP::OnPageChange( wxBookCtrlEvent& event )
{
    int page = event.GetSelection();

    if( Prj().IsReadOnly() )
        KIUI::Disable( m_treebook->GetPage( page ) );

    // Enable the reset button only if the page is resettable
    if( m_resetButton )
    {
        if( auto panel = dynamic_cast<RESETTABLE_PANEL*>( m_treebook->GetPage( page ) ) )
        {
            m_resetButton->SetToolTip( panel->GetResetTooltip() );
            m_resetButton->Enable( true );
        }
        else
        {
            m_resetButton->SetToolTip( wxString() );
            m_resetButton->Enable( false );
        }
    }

    // Work around an OSX bug where the wxGrid children don't get placed correctly until
    // the first resize event
#ifdef __WXMAC__
    if( m_macHack[ page ] )
    {
        wxSize pageSize = m_treebook->GetPage( page )->GetSize();
        pageSize.x -= 1;
        pageSize.y += 2;

        m_treebook->GetPage( page )->SetSize( pageSize );
        m_macHack[ page ] = false;
    }
#endif

    Layout();
}


// Run Import Settings... action
void DIALOG_SCHEMATIC_SETUP::OnAuxiliaryAction( wxCommandEvent& event )
{
    DIALOG_SCH_IMPORT_SETTINGS importDlg( this, m_frame );

    if( importDlg.ShowModal() == wxID_CANCEL )
        return;

    wxFileName projectFn( importDlg.GetFilePath() );

    if( !m_frame->GetSettingsManager()->LoadProject( projectFn.GetFullPath(), false ) )
    {
        wxString msg = wxString::Format( _( "Error importing settings from project:\n"
                                            "Project file %s could not be loaded." ),
                                         projectFn.GetFullPath() );
        DisplayErrorMessage( this, msg );

        return;
    }

    PROJECT*      otherPrj = m_frame->GetSettingsManager()->GetProject( projectFn.GetFullPath() );
    SCHEMATIC     otherSch( otherPrj );
    TEMPLATES     templateMgr;
    PROJECT_FILE& file = otherPrj->GetProjectFile();

    wxASSERT( file.m_SchematicSettings );

    file.m_SchematicSettings->LoadFromFile();

    if( importDlg.m_FormattingOpt->GetValue() )
        m_formatting->ImportSettingsFrom( *file.m_SchematicSettings );

    if( importDlg.m_FieldNameTemplatesOpt->GetValue() )
        m_fieldNameTemplates->ImportSettingsFrom( &otherSch.Settings().m_TemplateFieldNames );

    if( importDlg.m_PinMapOpt->GetValue() )
        m_pinMap->ImportSettingsFrom( file.m_ErcSettings->m_PinMap );

    if( importDlg.m_SeveritiesOpt->GetValue() )
        m_severities->ImportSettingsFrom( file.m_ErcSettings->m_Severities );

    if( importDlg.m_NetClassesOpt->GetValue() )
        m_netclasses->ImportSettingsFrom( &file.m_NetSettings->m_NetClasses );

    m_frame->GetSettingsManager()->UnloadProject( otherPrj, false );
}

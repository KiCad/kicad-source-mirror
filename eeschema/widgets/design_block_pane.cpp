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

#include <design_block.h>
#include <widgets/design_block_pane.h>
#include <widgets/panel_design_block_chooser.h>
#include <kiface_base.h>
#include <sch_edit_frame.h>
#include <core/kicad_algo.h>
#include <template_fieldnames.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <sch_actions.h>
#include <tool/tool_manager.h>


// Do not make these static wxStrings; they need to respond to language changes
#define REPEATED_PLACEMENT _( "Place repeated copies" )
#define PLACE_AS_SHEET     _( "Place as sheet" )
#define KEEP_ANNOTATIONS   _( "Keep annotations" )

DESIGN_BLOCK_PANE::DESIGN_BLOCK_PANE( SCH_EDIT_FRAME* aParent, const LIB_ID* aPreselect,
                                      std::vector<LIB_ID>& aHistoryList ) :
        WX_PANEL( aParent ),
        m_frame( aParent )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_chooserPanel = new PANEL_DESIGN_BLOCK_CHOOSER( aParent, this, aHistoryList,
            // Accept handler
            [this]()
            {
                m_frame->GetToolManager()->RunAction( SCH_ACTIONS::placeDesignBlock );
            } );

    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    SetName( wxT( "Design Blocks" ) );

    wxBoxSizer* cbSizer = new wxBoxSizer( wxVERTICAL );

    m_repeatedPlacement = new wxCheckBox( this, wxID_ANY, REPEATED_PLACEMENT );
    m_placeAsSheet = new wxCheckBox( this, wxID_ANY, PLACE_AS_SHEET );
    m_keepAnnotations = new wxCheckBox( this, wxID_ANY, KEEP_ANNOTATIONS );
    setLabelsAndTooltips();
    UpdateCheckboxes();

    // Set all checkbox handlers to the same function
    m_repeatedPlacement->Bind( wxEVT_CHECKBOX, &DESIGN_BLOCK_PANE::OnCheckBox, this );
    m_placeAsSheet->Bind( wxEVT_CHECKBOX, &DESIGN_BLOCK_PANE::OnCheckBox, this );
    m_keepAnnotations->Bind( wxEVT_CHECKBOX, &DESIGN_BLOCK_PANE::OnCheckBox, this );

    cbSizer->Add( m_repeatedPlacement, 0, wxTOP|wxLEFT, 2 );
    cbSizer->Add( m_placeAsSheet, 0, wxTOP|wxLEFT, 2 );
    cbSizer->Add( m_keepAnnotations, 0, wxTOP|wxLEFT|wxBOTTOM, 2 );

    sizer->Add( cbSizer, 0, wxEXPAND, 5 );
    SetSizer( sizer );

    m_chooserPanel->FinishSetup();
    Layout();

    Bind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, m_chooserPanel );
    m_frame->Bind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
    m_frame->Bind( wxEVT_AUI_PANE_CLOSE, &DESIGN_BLOCK_PANE::OnClosed, this );
}


DESIGN_BLOCK_PANE::~DESIGN_BLOCK_PANE()
{
    m_frame->Unbind( wxEVT_AUI_PANE_CLOSE, &DESIGN_BLOCK_PANE::OnClosed, this );
    m_frame->Unbind( EDA_LANG_CHANGED, &DESIGN_BLOCK_PANE::OnLanguageChanged, this );
}


void DESIGN_BLOCK_PANE::OnClosed( wxAuiManagerEvent &aEvent )
{
    if( APP_SETTINGS_BASE* cfg = m_frame->config() )
    {
        if( IsShownOnScreen() )     // Ensure the panel is shown when trying to save its size
            m_frame->SaveSettings( cfg );
    }

    aEvent.Skip();
}


void DESIGN_BLOCK_PANE::setLabelsAndTooltips()
{
    if( m_repeatedPlacement )
    {
        m_repeatedPlacement->SetLabel( REPEATED_PLACEMENT );
        m_repeatedPlacement->SetToolTip( _( "Place copies of the design block on subsequent "
                                            "clicks." ) );
    }

    if( m_placeAsSheet )
    {
        m_placeAsSheet->SetLabel( PLACE_AS_SHEET );
        m_placeAsSheet->SetToolTip( _( "Place the design block as a new sheet." ) );
    }

    if( m_keepAnnotations )
    {
        m_keepAnnotations->SetLabel( KEEP_ANNOTATIONS );
        m_keepAnnotations->SetToolTip( _( "Preserve reference designators in the source "
                                          "schematic. Otherwise, clear then reannotate according "
                                          "to settings." ) );
    }
}


void DESIGN_BLOCK_PANE::OnLanguageChanged( wxCommandEvent& aEvent )
{
    if( m_chooserPanel )
        m_chooserPanel->ShowChangedLanguage();

    setLabelsAndTooltips();

    aEvent.Skip();
}


void DESIGN_BLOCK_PANE::OnCheckBox( wxCommandEvent& aEvent )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_DesignBlockChooserPanel.repeated_placement = m_repeatedPlacement->GetValue();
        cfg->m_DesignBlockChooserPanel.place_as_sheet = m_placeAsSheet->GetValue();
        cfg->m_DesignBlockChooserPanel.keep_annotations = m_keepAnnotations->GetValue();
    }
}


void DESIGN_BLOCK_PANE::UpdateCheckboxes()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        m_repeatedPlacement->SetValue( cfg->m_DesignBlockChooserPanel.repeated_placement );
        m_placeAsSheet->SetValue( cfg->m_DesignBlockChooserPanel.place_as_sheet );
        m_keepAnnotations->SetValue( cfg->m_DesignBlockChooserPanel.keep_annotations );
    }
}


void DESIGN_BLOCK_PANE::SaveSettings()
{
    m_chooserPanel->SaveSettings();
}


LIB_ID DESIGN_BLOCK_PANE::GetSelectedLibId( int* aUnit ) const
{
    return m_chooserPanel->GetSelectedLibId( aUnit );
}


void DESIGN_BLOCK_PANE::SelectLibId( const LIB_ID& aLibId )
{
    m_chooserPanel->SelectLibId( aLibId );
}


void DESIGN_BLOCK_PANE::RefreshLibs()
{
    m_chooserPanel->RefreshLibs();
}


FILEDLG_IMPORT_SHEET_CONTENTS::FILEDLG_IMPORT_SHEET_CONTENTS( EESCHEMA_SETTINGS* aSettings ) :
    m_cbRepeatedPlacement( nullptr ),
    m_cbPlaceAsSheet( nullptr ),
    m_cbKeepAnnotations( nullptr )
{
    wxASSERT( aSettings );
    m_settings = aSettings;
};


void FILEDLG_IMPORT_SHEET_CONTENTS::TransferDataFromCustomControls()
{
    m_settings->m_DesignBlockChooserPanel.repeated_placement = m_cbRepeatedPlacement->GetValue();
    m_settings->m_DesignBlockChooserPanel.place_as_sheet = m_cbPlaceAsSheet->GetValue();
    m_settings->m_DesignBlockChooserPanel.keep_annotations = m_cbKeepAnnotations->GetValue();
}


void FILEDLG_IMPORT_SHEET_CONTENTS::AddCustomControls( wxFileDialogCustomize& customizer )
{
    m_cbRepeatedPlacement = customizer.AddCheckBox( REPEATED_PLACEMENT );
    m_cbRepeatedPlacement->SetValue( m_settings->m_DesignBlockChooserPanel.repeated_placement );
    m_cbPlaceAsSheet = customizer.AddCheckBox( PLACE_AS_SHEET );
    m_cbPlaceAsSheet->SetValue( m_settings->m_DesignBlockChooserPanel.place_as_sheet );
    m_cbKeepAnnotations = customizer.AddCheckBox( KEEP_ANNOTATIONS );
    m_cbKeepAnnotations->SetValue( m_settings->m_DesignBlockChooserPanel.keep_annotations );
}

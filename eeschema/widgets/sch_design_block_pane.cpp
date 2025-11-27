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
#include <widgets/sch_design_block_pane.h>
#include <widgets/sch_design_block_preview_widget.h>
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
#include <tools/sch_design_block_control.h>


// Do not make these static wxStrings; they need to respond to language changes
#define REPEATED_PLACEMENT _( "Place repeated copies" )
#define PLACE_AS_SHEET     _( "Place as sheet" )
#define PLACE_AS_GROUP     _( "Place as group" )
#define KEEP_ANNOTATIONS   _( "Keep annotations" )

SCH_DESIGN_BLOCK_PANE::SCH_DESIGN_BLOCK_PANE( SCH_EDIT_FRAME* aParent, const LIB_ID* aPreselect,
                                              std::vector<LIB_ID>& aHistoryList ) :
        DESIGN_BLOCK_PANE( aParent, aPreselect, aHistoryList )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    m_chooserPanel = new PANEL_DESIGN_BLOCK_CHOOSER( aParent, this, aHistoryList,
                                                     [aParent]()
                                                     {
                                                         aParent->GetToolManager()->RunAction(
                                                                 SCH_ACTIONS::placeDesignBlock );
                                                     },
                                                        aParent->GetToolManager()->GetTool<SCH_DESIGN_BLOCK_CONTROL>()
                                                     );
    // Use the same draw engine type as the one used in parent frame m_frame
    EDA_DRAW_PANEL_GAL::GAL_TYPE canvasType = m_frame->GetCanvas()->GetBackend();
    m_chooserPanel->SetPreviewWidget(
            new SCH_DESIGN_BLOCK_PREVIEW_WIDGET( m_chooserPanel->GetDetailsPanel(), canvasType, true ) );
    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    SetName( wxT( "Design Blocks" ) );

    wxBoxSizer* cbSizer = new wxBoxSizer( wxVERTICAL );

    m_repeatedPlacement = new wxCheckBox( this, wxID_ANY, REPEATED_PLACEMENT );
    m_placeAsGroup = new wxCheckBox( this, wxID_ANY, PLACE_AS_GROUP );
    m_placeAsSheet = new wxCheckBox( this, wxID_ANY, PLACE_AS_SHEET );
    m_keepAnnotations = new wxCheckBox( this, wxID_ANY, KEEP_ANNOTATIONS );
    setLabelsAndTooltips();
    UpdateCheckboxes();

    // Set all checkbox handlers to the same function
    m_repeatedPlacement->Bind( wxEVT_CHECKBOX, &SCH_DESIGN_BLOCK_PANE::OnCheckBox, this );
    m_placeAsGroup->Bind( wxEVT_CHECKBOX, &SCH_DESIGN_BLOCK_PANE::OnCheckBox, this );
    m_placeAsSheet->Bind( wxEVT_CHECKBOX, &SCH_DESIGN_BLOCK_PANE::OnCheckBox, this );
    m_keepAnnotations->Bind( wxEVT_CHECKBOX, &SCH_DESIGN_BLOCK_PANE::OnCheckBox, this );

    cbSizer->Add( m_repeatedPlacement, 0, wxTOP | wxLEFT, 2 );
    cbSizer->Add( m_placeAsGroup, 0, wxTOP | wxLEFT, 2 );
    cbSizer->Add( m_placeAsSheet, 0, wxTOP | wxLEFT, 2 );
    cbSizer->Add( m_keepAnnotations, 0, wxTOP | wxLEFT | wxBOTTOM, 2 );

    sizer->Add( cbSizer, 0, wxEXPAND, 5 );
    SetSizer( sizer );

    m_chooserPanel->FinishSetup();
    Layout();

    Bind( wxEVT_CHAR_HOOK, &PANEL_DESIGN_BLOCK_CHOOSER::OnChar, m_chooserPanel );
}


void SCH_DESIGN_BLOCK_PANE::setLabelsAndTooltips()
{
    if( m_repeatedPlacement )
    {
        m_repeatedPlacement->SetLabel( REPEATED_PLACEMENT );
        m_repeatedPlacement->SetToolTip( _( "Place copies of the design block on subsequent "
                                            "clicks." ) );
    }

    if( m_placeAsGroup )
    {
        m_placeAsGroup->SetLabel( PLACE_AS_GROUP );
        m_placeAsGroup->SetToolTip( _( "Place the design block as a group." ) );
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


void SCH_DESIGN_BLOCK_PANE::OnCheckBox( wxCommandEvent& aEvent )
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        cfg->m_DesignBlockChooserPanel.repeated_placement = m_repeatedPlacement->GetValue();
        cfg->m_DesignBlockChooserPanel.place_as_group = m_placeAsGroup->GetValue();
        cfg->m_DesignBlockChooserPanel.place_as_sheet = m_placeAsSheet->GetValue();
        cfg->m_DesignBlockChooserPanel.keep_annotations = m_keepAnnotations->GetValue();
    }
}


void SCH_DESIGN_BLOCK_PANE::UpdateCheckboxes()
{
    if( EESCHEMA_SETTINGS* cfg = dynamic_cast<EESCHEMA_SETTINGS*>( Kiface().KifaceSettings() ) )
    {
        m_repeatedPlacement->SetValue( cfg->m_DesignBlockChooserPanel.repeated_placement );
        m_placeAsGroup->SetValue( cfg->m_DesignBlockChooserPanel.place_as_group );
        m_placeAsSheet->SetValue( cfg->m_DesignBlockChooserPanel.place_as_sheet );
        m_keepAnnotations->SetValue( cfg->m_DesignBlockChooserPanel.keep_annotations );
    }
}


void SCH_DESIGN_BLOCK_PANE::ProjectChanged()
{
    // Project change will blow up the default project; re-create any active preview canvas
    m_chooserPanel->GetPreviewWidget()->DisplayDesignBlock( GetSelectedDesignBlock( true, true ) );
}


FILEDLG_IMPORT_SHEET_CONTENTS::FILEDLG_IMPORT_SHEET_CONTENTS( EESCHEMA_SETTINGS* aSettings ) :
        m_cbRepeatedPlacement( nullptr ), m_cbPlaceAsGroup( nullptr ), m_cbPlaceAsSheet( nullptr ),
        m_cbKeepAnnotations( nullptr )
{
    wxASSERT( aSettings );
    m_settings = aSettings;
};


void FILEDLG_IMPORT_SHEET_CONTENTS::TransferDataFromCustomControls()
{
    m_settings->m_DesignBlockChooserPanel.repeated_placement = m_cbRepeatedPlacement->GetValue();
    m_settings->m_DesignBlockChooserPanel.place_as_group = m_cbPlaceAsGroup->GetValue();
    m_settings->m_DesignBlockChooserPanel.place_as_sheet = m_cbPlaceAsSheet->GetValue();
    m_settings->m_DesignBlockChooserPanel.keep_annotations = m_cbKeepAnnotations->GetValue();
}


void FILEDLG_IMPORT_SHEET_CONTENTS::AddCustomControls( wxFileDialogCustomize& customizer )
{
#ifdef __WXMAC__
        customizer.AddStaticText( wxT( "\n\n" ) );  // Increase height of static box
#endif

    m_cbRepeatedPlacement = customizer.AddCheckBox( REPEATED_PLACEMENT );
    m_cbRepeatedPlacement->SetValue( m_settings->m_DesignBlockChooserPanel.repeated_placement );
    m_cbPlaceAsGroup = customizer.AddCheckBox( PLACE_AS_GROUP );
    m_cbPlaceAsGroup->SetValue( m_settings->m_DesignBlockChooserPanel.place_as_group );
    m_cbPlaceAsSheet = customizer.AddCheckBox( PLACE_AS_SHEET );
    m_cbPlaceAsSheet->SetValue( m_settings->m_DesignBlockChooserPanel.place_as_sheet );
    m_cbKeepAnnotations = customizer.AddCheckBox( KEEP_ANNOTATIONS );
    m_cbKeepAnnotations->SetValue( m_settings->m_DesignBlockChooserPanel.keep_annotations );
}

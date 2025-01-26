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

#include <dialog_symbol_chooser.h>
#include <widgets/panel_symbol_chooser.h>
#include <eeschema_settings.h>
#include <kiface_base.h>
#include <sch_base_frame.h>
#include <core/kicad_algo.h>
#include <template_fieldnames.h>
#include <widgets/footprint_preview_widget.h>
#include <widgets/footprint_select_widget.h>
#include <widgets/symbol_preview_widget.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>

std::mutex DIALOG_SYMBOL_CHOOSER::g_Mutex;

DIALOG_SYMBOL_CHOOSER::DIALOG_SYMBOL_CHOOSER( SCH_BASE_FRAME* aParent, const LIB_ID* aPreselect,
                                              const SYMBOL_LIBRARY_FILTER* aFilter,
                                              std::vector<PICKED_SYMBOL>&  aHistoryList,
                                              std::vector<PICKED_SYMBOL>&  aAlreadyPlaced,
                                              bool aAllowFieldEdits, bool aShowFootprints,
                                              bool& aCancelled ) :
        DIALOG_SHIM( aParent, wxID_ANY, _( "Choose Symbol" ), wxDefaultPosition, wxDefaultSize,
                     wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_chooserPanel = new PANEL_SYMBOL_CHOOSER( aParent, this, aFilter, aHistoryList, aAlreadyPlaced,
                                               aAllowFieldEdits, aShowFootprints, aCancelled,
                                               // Accept handler
                                               [this]()
                                               {
                                                   EndModal( wxID_OK );
                                               },
                                               // Escape handler
                                               [this]()
                                               {
                                                   EndModal( wxID_CANCEL );
                                               } );

    sizer->Add( m_chooserPanel, 1, wxEXPAND, 5 );

    if( aPreselect && aPreselect->IsValid() )
        m_chooserPanel->SetPreselect( *aPreselect );

    if( aFilter && aFilter->GetFilterPowerSymbols() )
        SetTitle( _( "Choose Power Symbol" ) );

    m_originalTitle = GetTitle();
    onLazyLoadUpdate();
    m_chooserPanel->Adapter()->RegisterLazyLoadHandler(
            std::bind( &DIALOG_SYMBOL_CHOOSER::onLazyLoadUpdate, this ) );

    wxBoxSizer* buttonsSizer = new wxBoxSizer( wxHORIZONTAL );

    m_keepSymbol = new wxCheckBox( this, wxID_ANY, _( "Place repeated copies" ) );
    m_keepSymbol->SetToolTip( _( "Keep the symbol selected for subsequent clicks." ) );

    m_useUnits = new wxCheckBox( this, wxID_ANY, _( "Place all units" ) );
    m_useUnits->SetToolTip( _( "Sequentially place all units of the symbol." ) );

    buttonsSizer->Add( m_keepSymbol, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 5 );
    buttonsSizer->Add( m_useUnits, 0, wxLEFT | wxALIGN_CENTER_VERTICAL, 30 );

    wxStdDialogButtonSizer* sdbSizer = new wxStdDialogButtonSizer();
    wxButton*               okButton = new wxButton( this, wxID_OK );
    wxButton*               cancelButton = new wxButton( this, wxID_CANCEL );

    sdbSizer->AddButton( okButton );
    sdbSizer->AddButton( cancelButton );
    sdbSizer->Realize();

    buttonsSizer->Add( sdbSizer, 1, wxALL, 5 );

    sizer->Add( buttonsSizer, 0, wxEXPAND | wxLEFT, 5 );
    SetSizer( sizer );

    SetInitialFocus( m_chooserPanel->GetFocusTarget() );
    SetupStandardButtons();

    m_chooserPanel->FinishSetup();
    Layout();

    Bind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnChar, m_chooserPanel );
}


DIALOG_SYMBOL_CHOOSER::~DIALOG_SYMBOL_CHOOSER()
{
    Unbind( wxEVT_CHAR_HOOK, &PANEL_SYMBOL_CHOOSER::OnChar, m_chooserPanel );
}


LIB_ID DIALOG_SYMBOL_CHOOSER::GetSelectedLibId( int* aUnit ) const
{
    return m_chooserPanel->GetSelectedLibId( aUnit );
}


std::vector<std::pair<FIELD_T, wxString>> DIALOG_SYMBOL_CHOOSER::GetFields() const
{
    return m_chooserPanel->GetFields();
}



void DIALOG_SYMBOL_CHOOSER::onLazyLoadUpdate()
{
    SetTitle( m_originalTitle + wxString::Format( _( " (%d items loaded)" ),
                                                  m_chooserPanel->GetItemCount() ) );
    m_chooserPanel->Regenerate();
}

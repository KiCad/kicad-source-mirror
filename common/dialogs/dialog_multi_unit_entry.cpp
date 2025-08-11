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

#include "dialogs/dialog_multi_unit_entry.h"

#include <eda_draw_frame.h>
#include <widgets/unit_binder.h>
#include <core/type_helpers.h>
#include <string_utils.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/stattext.h>


WX_MULTI_ENTRY_DIALOG::WX_MULTI_ENTRY_DIALOG( EDA_DRAW_FRAME* aParent, const wxString& aCaption,
                                              std::vector<ENTRY> aEntries ) :
        DIALOG_SHIM( aParent, wxID_ANY, aCaption ),
        m_entries( std::move( aEntries ) )
{
    SetSizeHints( wxDefaultSize, wxDefaultSize );

    wxBoxSizer* bSizerMain = new wxBoxSizer( wxVERTICAL );

    wxGridBagSizer* bSizerContent;

    bSizerContent = new wxGridBagSizer( 0, 0 );
    bSizerContent->SetFlexibleDirection( wxHORIZONTAL );
    bSizerContent->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_ALL );

    bSizerMain->Add( bSizerContent, 1, wxEXPAND | wxRIGHT | wxLEFT, 5 );

    int gbRow = 0;

    for( const ENTRY& entry : m_entries )
    {
        std::visit(
                [&]( const auto& aValue )
                {
                    using EntryType = std::decay_t<decltype( aValue )>;

                    if constexpr( std::is_same_v<EntryType, UNIT_BOUND> )
                    {
                        // Label / Entry / Unit
                        // and a binder
                        wxStaticText* label = new wxStaticText( this, wxID_ANY, entry.m_label );
                        label->Wrap( -1 );
                        bSizerContent->Add( label, wxGBPosition( gbRow, 0 ), wxGBSpan( 1, 1 ),
                                            wxALIGN_CENTER_VERTICAL | wxTOP | wxBOTTOM | wxLEFT, 5 );

                        wxTextCtrl* textCtrl = new wxTextCtrl( this, wxID_ANY );
                        bSizerContent->Add( textCtrl, wxGBPosition( gbRow, 1 ), wxGBSpan( 1, 1 ),
                                            wxALIGN_CENTER_VERTICAL | wxALL | wxEXPAND, 5 );

                        wxStaticText* units = new wxStaticText( this, wxID_ANY, _( "unit" ) );
                        units->Wrap( -1 );
                        bSizerContent->Add( units, wxGBPosition( gbRow, 2 ), wxGBSpan( 1, 1 ),
                                            wxTOP | wxBOTTOM | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );

                        if( !entry.m_tooltip.IsEmpty() )
                            textCtrl->SetToolTip( entry.m_tooltip );

                        m_controls.push_back( textCtrl );
                        m_unit_binders.push_back( std::make_unique<UNIT_BINDER>( aParent, label, textCtrl, units ) );

                        m_unit_binders.back()->SetValue( aValue.m_default );
                    }
                    else if constexpr( std::is_same_v<EntryType, CHECKBOX> )
                    {
                        // Checkbox across all 3 cols
                        wxCheckBox* checkBox = new wxCheckBox( this, wxID_ANY, entry.m_label );
                        checkBox->SetValue( aValue.m_default );
                        bSizerContent->Add( checkBox, wxGBPosition( gbRow, 0 ), wxGBSpan( 1, 3 ),
                                            wxALIGN_CENTER_VERTICAL | wxALL, 5 );

                        if( !entry.m_tooltip.IsEmpty() )
                            checkBox->SetToolTip( entry.m_tooltip );

                        m_controls.push_back( checkBox );
                        m_unit_binders.push_back( nullptr );
                    }
                    else
                    {
                        static_assert( always_false<EntryType>::value, "non-exhaustive visitor" );
                    }
                },
                entry.m_value );

        gbRow++;
    }

    // Grow the value column (now it knows it has a col 1)
    bSizerContent->AddGrowableCol( 1 );

    wxStdDialogButtonSizer* sdbSizer1 = new wxStdDialogButtonSizer();
    wxButton*               sdbSizer1OK = new wxButton( this, wxID_OK );
    sdbSizer1->AddButton( sdbSizer1OK );
    wxButton* sdbSizer1Cancel = new wxButton( this, wxID_CANCEL );
    sdbSizer1->AddButton( sdbSizer1Cancel );
    sdbSizer1->Realize();

    bSizerMain->Add( sdbSizer1, 0, wxALL | wxEXPAND, 5 );

    // DIALOG_SHIM needs a title--specific hash_key so we don't save/restore state between
    // usage cases.
    m_hash_key = TO_UTF8( GetTitle() );

    SetSizer( bSizerMain );
    SetupStandardButtons();
    Layout();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


std::vector<WX_MULTI_ENTRY_DIALOG::RESULT> WX_MULTI_ENTRY_DIALOG::GetValues() const
{
    std::vector<RESULT> results;

    for( size_t ii = 0; ii < m_entries.size(); ++ii )
    {
        wxWindow* const control = m_controls[ii];

        // Visit the value definitons to look up the right control type
        std::visit(
                [&]( const auto& aValueDef )
                {
                    using ArgType = std::decay_t<decltype( aValueDef )>;
                    if constexpr( std::is_same_v<ArgType, UNIT_BOUND> )
                    {
                        UNIT_BINDER* binder = m_unit_binders[ii].get();
                        wxASSERT( binder );
                        results.push_back( binder ? binder->GetValue() : 0 );
                    }
                    else if constexpr( std::is_same_v<ArgType, CHECKBOX> )
                    {
                        wxCheckBox* checkBox = static_cast<wxCheckBox*>( control );
                        results.push_back( checkBox->GetValue() );
                    }
                    else
                    {
                        static_assert( always_false<ArgType>::value, "non-exhaustive visitor" );
                    }
                },
                m_entries[ii].m_value );
    }

    return results;
}

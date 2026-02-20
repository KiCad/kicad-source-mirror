/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <dialogs/rule_editor_dialog_base.h>   
#include "drc_re_rtg_diff_pair_overlay_panel.h"
#include "drc_re_rtg_diff_pair_constraint_data.h"
#include "drc_rule_editor_utils.h"

#include <base_units.h>
#include <eda_base_frame.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>


DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL::DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL(
        wxWindow* aParent,
        DRC_RE_ROUTING_DIFF_PAIR_CONSTRAINT_DATA* aData,
        EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( BITMAPS::constraint_routing_diff_pair );

    std::vector<DRC_RE_FIELD_POSITION> positions = m_data->GetFieldPositions();

    wxWindow* eventSource = nullptr;

    for( wxWindow* win = aParent; win; win = win->GetParent() )
    {
        if( dynamic_cast<EDA_BASE_FRAME*>( win ) )
        {
            eventSource = win;
            break;
        }
    }

    auto* minWidthField = AddField<wxTextCtrl>( wxS( "min_width" ), positions[0], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_minWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minWidthField->GetControl(),
                                           minWidthField->GetLabel(), false, false );
    minWidthField->SetUnitBinder( m_minWidthBinder.get() );

    auto* prefWidthField = AddField<wxTextCtrl>( wxS( "pref_width" ), positions[1],
                                                 wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_preferredWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, prefWidthField->GetControl(),
                                           prefWidthField->GetLabel(), false, false );
    prefWidthField->SetUnitBinder( m_preferredWidthBinder.get() );

    auto* maxWidthField = AddField<wxTextCtrl>( wxS( "max_width" ), positions[2],
                                                wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_maxWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxWidthField->GetControl(),
                                           maxWidthField->GetLabel(), false, false );
    maxWidthField->SetUnitBinder( m_maxWidthBinder.get() );

    // Create gap fields (min/pref/max) — positions[3..5]
    auto* minGapField = AddField<wxTextCtrl>( wxS( "min_gap" ), positions[3],
                                              wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_minGapBinder = std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minGapField->GetControl(),
                                                    minGapField->GetLabel(), false, false );
    minGapField->SetUnitBinder( m_minGapBinder.get() );

    auto* prefGapField = AddField<wxTextCtrl>( wxS( "pref_gap" ), positions[4],
                                               wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_preferredGapBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, prefGapField->GetControl(),
                                           prefGapField->GetLabel(), false, false );
    prefGapField->SetUnitBinder( m_preferredGapBinder.get() );

    auto* maxGapField = AddField<wxTextCtrl>( wxS( "max_gap" ), positions[5],
                                              wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_maxGapBinder = std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxGapField->GetControl(),
                                                    maxGapField->GetLabel(), false, false );
    maxGapField->SetUnitBinder( m_maxGapBinder.get() );

    // Create max uncoupled length field — positions[6]
    auto* maxUncoupledField = AddField<wxTextCtrl>( wxS( "max_uncoupled" ), positions[6],
                                                    wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_maxUncoupledLengthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxUncoupledField->GetControl(),
                                           maxUncoupledField->GetLabel(), false, false );
    maxUncoupledField->SetUnitBinder( m_maxUncoupledLengthBinder.get() );

    auto* maxSkewField = AddField<wxTextCtrl>( wxS( "max_skew" ), positions[7], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_maxSkewBinder = std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxSkewField->GetControl(),
                                                     maxSkewField->GetLabel(), false, false );
    maxSkewField->SetUnitBinder( m_maxSkewBinder.get() );

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    minWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    prefWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    minGapField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    prefGapField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxGapField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxUncoupledField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxSkewField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    minWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    prefWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    minGapField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    prefGapField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxGapField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxUncoupledField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxSkewField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    // Position all fields and update the panel layout
    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    // Convert mm values to internal units and set them in the binders
    m_minGapBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinGap() ) );
    m_preferredGapBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetPreferredGap() ) );
    m_maxGapBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxGap() ) );

    m_minWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinWidth() ) );
    m_preferredWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetPreferredWidth() ) );
    m_maxWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxWidth() ) );

    m_maxUncoupledLengthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxUncoupledLength() ) );
    m_maxSkewBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxSkew() ) );

    return true;
}


bool DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    // Read values from binders (in internal units) and convert to mm for storage
    m_data->SetMinGap( pcbIUScale.IUTomm( m_minGapBinder->GetDoubleValue() ) );
    m_data->SetPreferredGap( pcbIUScale.IUTomm( m_preferredGapBinder->GetDoubleValue() ) );
    m_data->SetMaxGap( pcbIUScale.IUTomm( m_maxGapBinder->GetDoubleValue() ) );

    m_data->SetMinWidth( pcbIUScale.IUTomm( m_minWidthBinder->GetDoubleValue() ) );
    m_data->SetPreferredWidth( pcbIUScale.IUTomm( m_preferredWidthBinder->GetDoubleValue() ) );
    m_data->SetMaxWidth( pcbIUScale.IUTomm( m_maxWidthBinder->GetDoubleValue() ) );

    m_data->SetMaxUncoupledLength( pcbIUScale.IUTomm( m_maxUncoupledLengthBinder->GetDoubleValue() ) );
    m_data->SetMaxSkew( pcbIUScale.IUTomm( m_maxSkewBinder->GetDoubleValue() ) );

    return true;
}


bool DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
                                                             wxString* aValidationMessage )
{
    TransferDataFromWindow();

    VALIDATION_RESULT result = m_data->Validate();

    if( !result.isValid )
    {
        *aErrorCount = result.errors.size();

        for( size_t i = 0; i < result.errors.size(); i++ )
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( i + 1, result.errors[i] );

        return false;
    }

    return true;
}


wxString DRC_RE_ROUTING_DIFF_PAIR_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

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

#include "drc_re_routing_width_overlay_panel.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_validator_numeric_ctrl.h"

#include <base_units.h>
#include <eda_base_frame.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>
#include <dialogs/rule_editor_dialog_base.h>


DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL(
        wxWindow* aParent,
        DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA* aData,
        EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( BITMAPS::constraint_routing_width );

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


    // Create min width field
    auto* minWidthField = AddField<wxTextCtrl>( wxS( "min_width" ), positions[0],
                                                wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_minRoutingWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minWidthField->GetControl(),
                                           minWidthField->GetLabel(), false, false );
    minWidthField->SetUnitBinder( m_minRoutingWidthBinder.get() );
    minWidthField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Create preferred width field
    auto* prefWidthField = AddField<wxTextCtrl>( wxS( "pref_width" ), positions[1],
                                                 wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_preferredRoutingWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, prefWidthField->GetControl(),
                                           prefWidthField->GetLabel(), false, false );
    prefWidthField->SetUnitBinder( m_preferredRoutingWidthBinder.get() );
    prefWidthField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Create max width field
    auto* maxWidthField = AddField<wxTextCtrl>( wxS( "max_width" ), positions[2],
                                                wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_maxRoutingWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxWidthField->GetControl(),
                                           maxWidthField->GetLabel(), false, false );
    maxWidthField->SetUnitBinder( m_maxRoutingWidthBinder.get() );
    maxWidthField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    minWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    prefWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    minWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    prefWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    // Position all fields and update the panel layout
    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    // Convert mm values to internal units and set them in the binders
    m_minRoutingWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinRoutingWidth() ) );
    m_preferredRoutingWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetPreferredRoutingWidth() ) );
    m_maxRoutingWidthBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxRoutingWidth() ) );

    return true;
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    // Read values from binders (in internal units) and convert to mm for storage
    m_data->SetMinRoutingWidth( pcbIUScale.IUTomm( m_minRoutingWidthBinder->GetDoubleValue() ) );
    m_data->SetPreferredRoutingWidth( pcbIUScale.IUTomm( m_preferredRoutingWidthBinder->GetDoubleValue() ) );
    m_data->SetMaxRoutingWidth( pcbIUScale.IUTomm( m_maxRoutingWidthBinder->GetDoubleValue() ) );

    return true;
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
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


wxString DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

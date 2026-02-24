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

#include "drc_re_min_txt_ht_th_overlay_panel.h"
#include "drc_re_min_txt_ht_th_constraint_data.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_validator_numeric_ctrl.h"

#include <dialogs/rule_editor_dialog_base.h>
#include <eda_base_frame.h>
#include <base_units.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>


DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL::DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_MINIMUM_TEXT_HEIGHT_THICKNESS_CONSTRAINT_DATA* aData,
        EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( BITMAPS::constraint_minimum_text_height_and_thickness );

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

    auto* minTextHeightField = AddField<wxTextCtrl>( wxS( "min_text_height" ), positions[0], wxTE_PROCESS_ENTER );
    m_minTextHeightBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minTextHeightField->GetControl(),
                                           minTextHeightField->GetLabel(), false, false );
    minTextHeightField->SetUnitBinder( m_minTextHeightBinder.get() );
    minTextHeightField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    auto* minTextThicknessField = AddField<wxTextCtrl>( wxS( "min_text_thickness" ), positions[1], wxTE_PROCESS_ENTER );
    m_minTextThicknessBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minTextThicknessField->GetControl(),
                                           minTextThicknessField->GetLabel(), false, false );
    minTextThicknessField->SetUnitBinder( m_minTextThicknessBinder.get() );
    minTextThicknessField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    minTextHeightField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    minTextThicknessField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    minTextHeightField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    minTextThicknessField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    m_minTextHeightBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinTextHeight() ) );
    m_minTextThicknessBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinTextThickness() ) );

    return true;
}


bool DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    m_data->SetMinTextHeight( pcbIUScale.IUTomm( m_minTextHeightBinder->GetDoubleValue() ) );
    m_data->SetMinTextThickness( pcbIUScale.IUTomm( m_minTextThicknessBinder->GetDoubleValue() ) );

    return true;
}


bool DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
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


wxString DRC_RE_MIN_TXT_HT_TH_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

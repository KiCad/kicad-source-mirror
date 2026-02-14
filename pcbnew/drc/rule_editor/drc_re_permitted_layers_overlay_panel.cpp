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

#include "drc_re_permitted_layers_overlay_panel.h"
#include "drc_re_permitted_layers_constraint_data.h"
#include "drc_rule_editor_utils.h"

#include <dialogs/rule_editor_dialog_base.h>
#include <wx/checkbox.h>


DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL::DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA* aData ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_topLayerCheckbox( nullptr ),
        m_bottomLayerCheckbox( nullptr )
{
    SetBackgroundBitmap( m_data->GetOverlayBitmap() );

    std::vector<DRC_RE_FIELD_POSITION> positions = m_data->GetFieldPositions();

    DRC_RE_OVERLAY_FIELD* topField = AddCheckbox( wxS( "top_layer" ), positions[0] );
    m_topLayerCheckbox = static_cast<wxCheckBox*>( topField->GetControl() );

    DRC_RE_OVERLAY_FIELD* bottomField = AddCheckbox( wxS( "bottom_layer" ), positions[1] );
    m_bottomLayerCheckbox = static_cast<wxCheckBox*>( bottomField->GetControl() );

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    m_topLayerCheckbox->Bind( wxEVT_CHECKBOX, notifyModified );
    m_bottomLayerCheckbox->Bind( wxEVT_CHECKBOX, notifyModified );

    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data || !m_topLayerCheckbox || !m_bottomLayerCheckbox )
        return false;

    m_topLayerCheckbox->SetValue( m_data->GetTopLayerEnabled() );
    m_bottomLayerCheckbox->SetValue( m_data->GetBottomLayerEnabled() );

    return true;
}


bool DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data || !m_topLayerCheckbox || !m_bottomLayerCheckbox )
        return false;

    m_data->SetTopLayerEnabled( m_topLayerCheckbox->GetValue() );
    m_data->SetBottomLayerEnabled( m_bottomLayerCheckbox->GetValue() );

    return true;
}


bool DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
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


wxString DRC_RE_PERMITTED_LAYERS_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

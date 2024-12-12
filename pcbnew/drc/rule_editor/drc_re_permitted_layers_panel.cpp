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

#include "drc_re_permitted_layers_panel.h"

#include <wx/log.h>


DRC_RE_PERMITTED_LAYERS_PANEL::DRC_RE_PERMITTED_LAYERS_PANEL( wxWindow* aParent, wxString* aConstraintTitle,
        std::shared_ptr<DRC_RE_PERMITTED_LAYERS_CONSTRAINT_DATA> aConstraintData ) :
        DRC_RE_PERMITTED_LAYERS_PANEL_BASE( aParent ), m_constraintData( aConstraintData )
{
    bConstraintImageSizer->Add( GetConstraintImage( this, BITMAPS::constraint_permitted_layers ), 0,
                                wxALL | wxEXPAND, 10 );
}


DRC_RE_PERMITTED_LAYERS_PANEL::~DRC_RE_PERMITTED_LAYERS_PANEL()
{
}


bool DRC_RE_PERMITTED_LAYERS_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
    {
        m_topLayerChkCtrl->SetValue( m_constraintData->GetTopLayerEnabled() );
        m_bottomLayerChkCtrl->SetValue( m_constraintData->GetBottomLayerEnabled() );
    }

    return true;
}


bool DRC_RE_PERMITTED_LAYERS_PANEL::TransferDataFromWindow()
{
    m_constraintData->SetTopLayerEnabled( m_topLayerChkCtrl->GetValue() );
    m_constraintData->SetBottomLayerEnabled( m_bottomLayerChkCtrl->GetValue() );
    return true;
}


bool DRC_RE_PERMITTED_LAYERS_PANEL::ValidateInputs( int* aErrorCount,
                                                    std::string* aValidationMessage )
{
    TransferDataFromWindow();
    VALIDATION_RESULT result = m_constraintData->Validate();

    if( !result.isValid )
    {
        *aErrorCount = result.errors.size();

        for( size_t i = 0; i < result.errors.size(); i++ )
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( i + 1, result.errors[i] );

        return false;
    }

    return true;
}


wxString DRC_RE_PERMITTED_LAYERS_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_constraintData )
        return wxEmptyString;

    wxString code = m_constraintData->GetConstraintCode();

    if( code.IsEmpty() )
        code = wxS( "permitted_layers" );

    const wxString topState = m_constraintData->GetTopLayerEnabled() ? wxS( "true" ) : wxS( "false" );
    const wxString bottomState = m_constraintData->GetBottomLayerEnabled() ? wxS( "true" ) : wxS( "false" );

    wxString clause = wxString::Format( wxS( "(constraint %s (top %s) (bottom %s))" ), code,
                                        topState, bottomState );

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "Permitted layers clause: %s" ), clause );

    return buildRule( aContext, { clause } );
}

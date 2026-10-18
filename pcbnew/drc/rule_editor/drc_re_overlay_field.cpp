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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "drc_re_overlay_field.h"

#include <widgets/field_warning_icon.h>
#include <widgets/unit_binder.h>

#include <wx/control.h>
#include <wx/stattext.h>
#include <wx/window.h>


DRC_RE_OVERLAY_FIELD::DRC_RE_OVERLAY_FIELD( wxWindow* aParent, const wxString& aFieldId,
                                            wxControl* aControl,
                                            const DRC_RE_FIELD_POSITION& aPosition ) :
        m_parent( aParent ),
        m_fieldId( aFieldId ),
        m_control( aControl ),
        m_position( aPosition ),
        m_getter( nullptr ),
        m_setter( nullptr ),
        m_unitBinder( nullptr ),
        m_errorIcon( nullptr ),
        m_label( nullptr ),
        m_prefixLabel( nullptr )
{
}


DRC_RE_OVERLAY_FIELD::~DRC_RE_OVERLAY_FIELD()
{
    if( m_errorIcon )
    {
        m_errorIcon->Destroy();
        m_errorIcon = nullptr;
    }

    if( m_label )
    {
        m_label->Destroy();
        m_label = nullptr;
    }
}


void DRC_RE_OVERLAY_FIELD::ShowError( const wxString& aMessage )
{
    if( !m_errorIcon )
        m_errorIcon = new FIELD_WARNING_ICON( m_control );

    m_errorIcon->SetWarning( aMessage );
}


void DRC_RE_OVERLAY_FIELD::ClearError()
{
    if( m_errorIcon )
        m_errorIcon->SetWarning( wxEmptyString );
}


bool DRC_RE_OVERLAY_FIELD::IsShowingError() const
{
    return m_errorIcon && m_errorIcon->IsShown();
}


bool DRC_RE_OVERLAY_FIELD::TransferToWindow()
{
    if( !m_getter )
        return true;

    try
    {
        double value = m_getter();

        if( m_unitBinder )
        {
            m_unitBinder->SetDoubleValue( value );
        }

        return true;
    }
    catch( ... )
    {
        return false;
    }
}


bool DRC_RE_OVERLAY_FIELD::TransferFromWindow()
{
    if( !m_setter )
        return true;

    try
    {
        if( m_unitBinder )
        {
            double value = m_unitBinder->GetDoubleValue();
            m_setter( value );
        }

        return true;
    }
    catch( ... )
    {
        return false;
    }
}


void DRC_RE_OVERLAY_FIELD::CreateLabels()
{
    if( !m_label && !m_position.labelText.IsEmpty() && m_position.labelPosition != LABEL_POSITION::NONE
        && GetControl()->GetLabel() != m_position.labelText )
    {
        m_label = new wxStaticText( m_parent, wxID_ANY, m_position.labelText );
    }

    if( !m_prefixLabel && !m_position.prefixText.IsEmpty() )
    {
        m_prefixLabel = new wxStaticText( m_parent, wxID_ANY, m_position.prefixText );
    }
}

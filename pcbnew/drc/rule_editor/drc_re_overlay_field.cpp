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

#include "drc_re_overlay_field.h"

#include <bitmaps.h>
#include <widgets/unit_binder.h>

#include <wx/control.h>
#include <wx/statbmp.h>
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
        m_showingError( false ),
        m_label( nullptr )
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


void DRC_RE_OVERLAY_FIELD::createErrorIcon()
{
    if( m_errorIcon )
        return;

    m_errorIcon = new wxStaticBitmap( m_parent, wxID_ANY, KiBitmapBundle( BITMAPS::small_warning ) );

    // Position the icon to the right of the control with a small gap
    wxPoint controlPos = m_control->GetPosition();
    wxSize controlSize = m_control->GetSize();
    wxSize iconSize = m_errorIcon->GetSize();

    int iconX = controlPos.x + controlSize.GetWidth() + 4;
    int iconY = controlPos.y + ( controlSize.GetHeight() - iconSize.GetHeight() ) / 2;

    m_errorIcon->SetPosition( wxPoint( iconX, iconY ) );
    m_errorIcon->Hide();
}


void DRC_RE_OVERLAY_FIELD::ShowError( bool aShow )
{
    if( aShow && !m_errorIcon )
        createErrorIcon();

    if( m_errorIcon )
        m_errorIcon->Show( aShow );

    m_showingError = aShow;
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


void DRC_RE_OVERLAY_FIELD::CreateLabel()
{
    if( m_label )
        return;

    if( m_position.labelText.IsEmpty() || m_position.labelPosition == LABEL_POSITION::NONE )
        return;

    m_label = new wxStaticText( m_parent, wxID_ANY, m_position.labelText );
}

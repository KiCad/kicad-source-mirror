/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <properties/pg_editors.h>
#include <properties/pg_properties.h>
#include <widgets/unit_binder.h>

#include <wx/log.h>


PG_UNIT_EDITOR::PG_UNIT_EDITOR( EDA_DRAW_FRAME* aFrame ) :
        wxPGTextCtrlEditor(),
        m_frame( aFrame )
{
    m_unitBinder = std::make_unique<PROPERTY_EDITOR_UNIT_BINDER>( m_frame );
}


PG_UNIT_EDITOR::~PG_UNIT_EDITOR()
{
}


wxPGWindowList PG_UNIT_EDITOR::CreateControls( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty,
                                               const wxPoint& aPos, const wxSize& aSize ) const
{
    wxPGWindowList ret = wxPGTextCtrlEditor::CreateControls( aPropGrid, aProperty, aPos, aSize );

    m_unitBinder->SetControl( ret.m_primary );
    m_unitBinder->RequireEval();

    if( PGPROPERTY_DISTANCE* prop = dynamic_cast<PGPROPERTY_DISTANCE*>( aProperty ) )
        m_unitBinder->SetCoordType( prop->CoordType() );

    return ret;
}


bool PG_UNIT_EDITOR::GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                                          wxWindow* aCtrl ) const
{
    wxTextCtrl* textCtrl = dynamic_cast<wxTextCtrl*>( aCtrl );
    wxCHECK_MSG( textCtrl, false, "PG_UNIT_EDITOR requires a text control!" );
    wxString textVal = textCtrl->GetValue();

    if( aProperty->UsesAutoUnspecified() && textVal.empty() )
    {
        aVariant.MakeNull();
        return true;
    }

    long result = m_unitBinder->GetValue();

    bool changed = ( result != aVariant.GetLong() );

    if( changed )
    {
        aVariant = result;
        m_unitBinder->SetValue( result );
    }

    // Changing unspecified always causes event (returning
    // true here should be enough to trigger it).
    if( !changed && aVariant.IsNull() )
        changed = true;

    return changed;
}

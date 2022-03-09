/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright (C) 2022 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SIM_PROPERTY_H
#define SIM_PROPERTY_H

#include <sim/sim_value.h>
#include <wx/propgrid/props.h>


class SIM_VALIDATOR : public wxValidator
{
public:
    SIM_VALIDATOR( SIM_VALUE_BASE::TYPE aValueType, SIM_VALUE_PARSER::NOTATION aNotation );
    SIM_VALIDATOR( const SIM_VALIDATOR& aValidator ) = default;

    wxObject* Clone() const override;

    bool Validate( wxWindow* aParent ) override;
    bool TransferToWindow() override;
    bool TransferFromWindow() override;

private:
    bool isValid( const wxString& aString );

    wxTextEntry* getTextEntry();

    void onText( wxCommandEvent& aEvent );
    void onChar( wxKeyEvent& aEvent );
    void onMouse( wxMouseEvent& aEvent );

    SIM_VALUE_BASE::TYPE       m_valueType;
    SIM_VALUE_PARSER::NOTATION m_notation;
    wxString                   m_prevText;
    long                       m_prevInsertionPoint;
    
    wxDECLARE_EVENT_TABLE();
};


class SIM_PROPERTY : public wxStringProperty
{
public:
    SIM_PROPERTY( const wxString& aLabel, const wxString& aName, SIM_VALUE_BASE& aValue,
                  SIM_VALUE_BASE::TYPE aValueType = SIM_VALUE_BASE::TYPE::FLOAT,
                  SIM_VALUE_PARSER::NOTATION aNotation = SIM_VALUE_PARSER::NOTATION::SI );

    wxValidator* DoGetValidator() const override;

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 )
        const override;

protected:
    SIM_VALUE_BASE::TYPE       m_valueType;
    SIM_VALUE_PARSER::NOTATION m_notation;
    SIM_VALUE_BASE&            m_value;
};

#endif // SIM_PROPERTY_H

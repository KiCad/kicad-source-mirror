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

#include <sim/sim_model.h>
#include <wx/window.h>
#include <wx/propgrid/props.h>


class SIM_VALIDATOR : public wxValidator
{
public:
    SIM_VALIDATOR( SIM_VALUE::TYPE aValueType, SIM_VALUE_GRAMMAR::NOTATION aNotation );
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

    SIM_VALUE::TYPE       m_valueType;
    SIM_VALUE_GRAMMAR::NOTATION m_notation;
    wxString                   m_prevText;
    long                       m_prevInsertionPoint;
    
    wxDECLARE_EVENT_TABLE();
};


class SIM_PROPERTY : public wxStringProperty
{
public:
    // We pass shared_ptrs because we need to make sure they are destroyed only after the last time
    // SIM_PROPERTY uses them.
    SIM_PROPERTY( const wxString& aLabel, const wxString& aName,
                  std::shared_ptr<SIM_LIBRARY> aLibrary,
                  std::shared_ptr<SIM_MODEL> aModel,
                  int aParamIndex,
                  SIM_VALUE::TYPE aValueType = SIM_VALUE::TYPE::FLOAT,
                  SIM_VALUE_GRAMMAR::NOTATION aNotation = SIM_VALUE_GRAMMAR::NOTATION::SI );

    wxValidator* DoGetValidator() const override;

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 )
        const override;

    const SIM_MODEL::PARAM& GetParam() const { return m_model->GetParam( m_paramIndex ); }

protected:
    SIM_VALUE::TYPE         m_valueType;
    SIM_VALUE_GRAMMAR::NOTATION  m_notation;
    std::shared_ptr<SIM_LIBRARY> m_library;
    std::shared_ptr<SIM_MODEL>   m_model;
    int                          m_paramIndex;
};

#endif // SIM_PROPERTY_H

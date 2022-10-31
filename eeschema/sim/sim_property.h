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
#include <wx/notebook.h>
#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>
#include <wx/propgrid/props.h>


// This doesn't actually do any validation, but it's a convenient place to fix some navigation
// issues with wxPropertyGrid.
class SIM_VALIDATOR : public wxValidator
{
private:
    void navigate( int flags );

    void onKeyDown( wxKeyEvent& aEvent );

    bool Validate( wxWindow* aParent ) override;

    wxDECLARE_EVENT_TABLE();
};


class SIM_PROPERTY
{
public:
    SIM_PROPERTY( std::shared_ptr<SIM_LIBRARY> aLibrary, std::shared_ptr<SIM_MODEL> aModel,
                  int aParamIndex );

    const SIM_MODEL::PARAM& GetParam() const { return m_model->GetParam( m_paramIndex ); }

protected:
    std::shared_ptr<SIM_LIBRARY> m_library; // We hold a shared_ptr to SIM_LIBRARY to prevent its
                                            // deallocation during this object's lifetime.
    std::shared_ptr<SIM_MODEL>   m_model;
    int                          m_paramIndex;
};


class SIM_BOOL_PROPERTY : public wxBoolProperty, public SIM_PROPERTY
{
public:
    SIM_BOOL_PROPERTY( const wxString& aLabel, const wxString& aName,
                       std::shared_ptr<SIM_LIBRARY> aLibrary,
                       std::shared_ptr<SIM_MODEL> aModel,
                       int aParamIndex );

    wxValidator* DoGetValidator() const override;

    void OnSetValue() override;
};


class SIM_STRING_PROPERTY : public wxStringProperty, public SIM_PROPERTY
{
public:
    // We pass shared_ptrs because we need to make sure they are destroyed only after the last time
    // SIM_PROPERTY uses them.
    SIM_STRING_PROPERTY( const wxString& aLabel, const wxString& aName,
                         std::shared_ptr<SIM_LIBRARY> aLibrary,
                         std::shared_ptr<SIM_MODEL> aModel,
                         int aParamIndex );

    wxValidator* DoGetValidator() const override;

    bool StringToValue( wxVariant& aVariant, const wxString& aText, int aArgFlags = 0 )
        const override;
};


class SIM_ENUM_PROPERTY : public wxEnumProperty, public SIM_PROPERTY
{
public:
    SIM_ENUM_PROPERTY( const wxString& aLabel, const wxString& aName,
                       std::shared_ptr<SIM_LIBRARY> aLibrary,
                       std::shared_ptr<SIM_MODEL> aModel,
                       int aParamIndex );

    bool IntToValue( wxVariant& aVariant, int aNumber, int aArgFlags = 0 ) const override;
};

#endif // SIM_PROPERTY_H

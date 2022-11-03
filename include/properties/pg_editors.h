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

#ifndef KICAD_PG_EDITORS_H
#define KICAD_PG_EDITORS_H

#include <memory>

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/editors.h>

class EDA_DRAW_FRAME;
class PROPERTY_EDITOR_UNIT_BINDER;

class PG_UNIT_EDITOR : public wxPGTextCtrlEditor
{
public:
    PG_UNIT_EDITOR( EDA_DRAW_FRAME* aFrame );

    virtual ~PG_UNIT_EDITOR();

    wxString GetName() const override { return wxT( "UnitEditor" ); }

    wxPGWindowList CreateControls( wxPropertyGrid* aPropGrid, wxPGProperty* aProperty,
                                   const wxPoint& aPos, const wxSize& aSize ) const override;

    bool GetValueFromControl( wxVariant& aVariant, wxPGProperty* aProperty,
                              wxWindow* aCtrl ) const override;
protected:
    EDA_DRAW_FRAME* m_frame;

    std::unique_ptr<PROPERTY_EDITOR_UNIT_BINDER> m_unitBinder;
};

#endif //KICAD_PG_EDITORS_H

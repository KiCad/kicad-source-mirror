/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef GRID_COMBOBOX_H
#define GRID_COMBOBOX_H

#include <wx/combobox.h>
#include <wx/generic/gridctrl.h>
#include <wx/generic/grideditors.h>


class wxGrid;


class GRID_CELL_COMBOBOX : public wxGridCellEditor
{
public:
    GRID_CELL_COMBOBOX( const wxArrayString& names );

    wxGridCellEditor* Clone() const override;
    void Create( wxWindow* aParent, wxWindowID aId, wxEvtHandler* aEventHandler ) override;

    wxString GetValue() const override;

    void SetSize( const wxRect& aRect ) override;

    void BeginEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    bool EndEdit( int aRow, int aCol, const wxGrid*, const wxString&, wxString* aNewVal ) override;
    void ApplyEdit( int aRow, int aCol, wxGrid* aGrid ) override;
    void Reset() override;

protected:
    wxComboBox* Combo() const { return static_cast<wxComboBox*>( m_control ); }

    void onComboCloseUp( wxCommandEvent& aEvent );
    void onComboDropDown( wxCommandEvent& aEvent );

    wxArrayString m_names;
    wxString      m_value;

    wxDECLARE_NO_COPY_CLASS( GRID_CELL_COMBOBOX );
};


#endif  // GRID_COMBOBOX_H

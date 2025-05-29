/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Simon Richter <Simon.Richter@hogyros.de>
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

#include <widgets/wx_bitmap_combobox.h>
#include <pin_type.h>

class PINTYPE_COMBOBOX : public WX_BITMAP_COMBOBOX
{
public:
    PINTYPE_COMBOBOX( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& value = wxEmptyString,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      int n = 0, const wxString choices[] = nullptr, long style = 0,
                      const wxValidator& validator = wxDefaultValidator,
                      const wxString& name = wxBitmapComboBoxNameStr );

    ELECTRICAL_PINTYPE GetPinTypeSelection();
    void SetSelection( ELECTRICAL_PINTYPE aType );

private:
    // fixes hidden overloaded virtual function warnings
    using wxBitmapComboBox::SetSelection;
};

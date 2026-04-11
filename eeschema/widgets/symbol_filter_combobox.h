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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <widgets/filter_combobox.h>


class SYMBOL_FILTER_COMBOPOPUP;


class SYMBOL_FILTER_COMBOBOX : public FILTER_COMBOBOX
{
public:
    // Note: this list of arguments is here because it keeps us from having to customize
    // the constructor calls in wxFormBuilder.
    SYMBOL_FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize, long style = 0 );

    wxString GetValue() const override;

protected:
    SYMBOL_FILTER_COMBOPOPUP* m_selectorPopup;
};

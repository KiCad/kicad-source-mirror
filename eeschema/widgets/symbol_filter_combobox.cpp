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

#include "widgets/symbol_filter_combobox.h"

#include <iostream>


static const wxString kNoParentSymbol( "<do not derive>" );

class SYMBOL_FILTER_COMBOPOPUP : public FILTER_COMBOPOPUP
{
public:
    SYMBOL_FILTER_COMBOPOPUP() {}

private:
    void getListContent( wxArrayString& aListContent ) override
    {
        FILTER_COMBOPOPUP::getListContent( aListContent );

        const wxString filterString = getFilterValue();

        // Special handling for <do not derive>
        if( filterString.IsEmpty() || kNoParentSymbol.Lower().Matches( filterString ) )
            aListContent.insert( aListContent.begin(), kNoParentSymbol );
    }
};


SYMBOL_FILTER_COMBOBOX::SYMBOL_FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                                const wxSize& size, long style ) :
        FILTER_COMBOBOX( parent, id, pos, size, style|wxCB_READONLY )
{
    m_selectorPopup = new SYMBOL_FILTER_COMBOPOPUP();
    setFilterPopup( m_selectorPopup );
}


wxString SYMBOL_FILTER_COMBOBOX::GetValue() const
{
    wxString value = m_selectorPopup->GetStringValue();

    if( value == kNoParentSymbol )
        return wxEmptyString;

    return value;
}
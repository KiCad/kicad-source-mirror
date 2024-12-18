/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018-2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "widgets/symbol_filter_combobox.h"


#include <iostream>

static const wxString kNoParentSymbol( "<do not derive>" );

class SYMBOL_FILTER_COMBOPOPUP : public FILTER_COMBOPOPUP
{
public:
    SYMBOL_FILTER_COMBOPOPUP() {}

    wxString GetStringValue() const override { return m_selectedSymbol; }

    void SetSelectedSymbol( const wxString& aSymbolName )
    {
        m_selectedSymbol = aSymbolName;
        GetComboCtrl()->SetValue( m_selectedSymbol );
    }

    void Accept() override
    {
        wxString selectedSymbol = getSelectedValue().value_or( wxEmptyString );

        Dismiss();

        // No update on empty
        if( !selectedSymbol.IsEmpty() && selectedSymbol != m_selectedSymbol )
        {
            m_selectedSymbol = selectedSymbol;
            GetComboCtrl()->SetValue( m_selectedSymbol );

            wxCommandEvent changeEvent( FILTERED_ITEM_SELECTED );
            wxPostEvent( GetComboCtrl(), changeEvent );
        }
    }

    void SetSymbolList( const wxArrayString& aSymbolList )
    {
        m_symbolList = aSymbolList;
        m_symbolList.Sort();
        rebuildList();
    }

private:
    void getListContent( wxArrayString& aListContent ) override
    {
        const wxString filterString = getFilterValue();

        // Special handling for <no net>
        if( filterString.IsEmpty() || kNoParentSymbol.Lower().Matches( filterString ) )
            aListContent.insert( aListContent.begin(), kNoParentSymbol );

        // Simple substring, case-insensitive search
        for( const wxString& symbol : m_symbolList )
        {
            if( filterString.IsEmpty() || symbol.Lower().Contains( filterString.Lower() ) )
                aListContent.push_back( symbol );
        }
    }

    wxString      m_selectedSymbol;
    wxArrayString m_symbolList;
};


SYMBOL_FILTER_COMBOBOX::SYMBOL_FILTER_COMBOBOX( wxWindow* parent, wxWindowID id, const wxPoint& pos,
                                                const wxSize& size, long style ) :
        FILTER_COMBOBOX( parent, id, pos, size, style )
{
    std::unique_ptr<SYMBOL_FILTER_COMBOPOPUP> popup = std::make_unique<SYMBOL_FILTER_COMBOPOPUP>();
    m_selectorPopup = popup.get();
    setFilterPopup( std::move( popup ) );
}


void SYMBOL_FILTER_COMBOBOX::SetSymbolList( const wxArrayString& aSymbolList )
{
    m_selectorPopup->SetSymbolList( aSymbolList );
}


void SYMBOL_FILTER_COMBOBOX::SetSelectedSymbol( const wxString& aSymbolName )
{
    m_selectorPopup->SetSelectedSymbol( aSymbolName );
}


wxString SYMBOL_FILTER_COMBOBOX::GetValue() const
{
    wxString value = m_selectorPopup->GetStringValue();

    if( value == kNoParentSymbol )
        return wxEmptyString;

    return value;
}
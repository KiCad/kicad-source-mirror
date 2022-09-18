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

#include <sim/spice_generator.h>
#include <locale_io.h>


wxString SPICE_GENERATOR::ModelLine( const wxString& aModelName ) const
{
    LOCALE_IO toggle;

    if( !m_model.HasSpiceNonInstanceOverrides() && !m_model.requiresSpiceModelLine() )
        return "";

    wxString result = "";

    result << wxString::Format( ".model %s ", aModelName );
    size_t indentLength = result.Length();

    result << wxString::Format( "%s \n", m_model.GetSpiceInfo().modelType );

    for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            continue;

        wxString name = ( param.info.spiceModelName == "" ) ?
            param.info.name : param.info.spiceModelName;
        wxString value = param.value->ToSpiceString();

        if( value == "" )
            continue;

        result << wxString::Format( "+%s%s=%s\n", wxString( ' ', indentLength - 1 ), name, value );
    }

    return result;
}


wxString SPICE_GENERATOR::ItemLine( const wxString& aRefName,
                                    const wxString& aModelName ) const
{
    // Use linear symbol pin numbers enumeration. Used in model preview.

    std::vector<wxString> pinNumbers;

    for( int i = 0; i < m_model.GetPinCount(); ++i )
        pinNumbers.push_back( wxString::FromCDouble( i + 1 ) );

    return ItemLine( aRefName, aModelName, pinNumbers );
}


wxString SPICE_GENERATOR::ItemLine( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aSymbolPinNumbers ) const
{
    std::vector<wxString> pinNetNames;

    for( const SIM_MODEL::PIN& pin : GetPins() )
        pinNetNames.push_back( pin.name );

    return ItemLine( aRefName, aModelName, aSymbolPinNumbers, pinNetNames );
}


wxString SPICE_GENERATOR::ItemLine( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aSymbolPinNumbers,
                                    const std::vector<wxString>& aPinNetNames ) const
{
    wxString result;
    result << ItemName( aRefName );
    result << ItemPins( aRefName, aModelName, aSymbolPinNumbers, aPinNetNames );
    result << ItemModelName( aModelName );
    result << ItemParams();
    result << "\n";
    return result;
}


wxString SPICE_GENERATOR::ItemName( const wxString& aRefName ) const
{
    if( aRefName != "" && aRefName.StartsWith( m_model.GetSpiceInfo().itemType ) )
        return aRefName;
    else
        return m_model.GetSpiceInfo().itemType + aRefName;
}


wxString SPICE_GENERATOR::ItemPins( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aSymbolPinNumbers,
                                    const std::vector<wxString>& aPinNetNames ) const
{
    wxString result;
    int ncCounter = 0;

    for( const SIM_MODEL::PIN& pin : GetPins() )
    {
        auto it = std::find( aSymbolPinNumbers.begin(), aSymbolPinNumbers.end(),
                             pin.symbolPinNumber );

        if( it == aSymbolPinNumbers.end() )
        {
            LOCALE_IO toggle;
            result << wxString::Format( " NC-%s-%u", aRefName, ncCounter++ );
        }
        else
        {
            long symbolPinIndex = std::distance( aSymbolPinNumbers.begin(), it );
            result << " " << aPinNetNames.at( symbolPinIndex );
        }
    }

    return result;
}


wxString SPICE_GENERATOR::ItemModelName( const wxString& aModelName ) const
{
    return " " + aModelName;
}


wxString SPICE_GENERATOR::ItemParams() const
{
    wxString result;

    for( const SIM_MODEL::PARAM& param : GetInstanceParams() )
    {
        wxString name = ( param.info.spiceInstanceName == "" ) ?
            param.info.name : param.info.spiceInstanceName;
        wxString value = param.value->ToSpiceString();

        if( value != "" )
            result << " " << name << "=" << value;
    }

    return result;
}


wxString SPICE_GENERATOR::TuningLine( const wxString& aSymbol ) const
{
    // TODO.
    return "";
}


std::vector<wxString> SPICE_GENERATOR::CurrentNames( const wxString& aRefName ) const
{
    LOCALE_IO toggle;
    return { wxString::Format( "I(%s)", ItemName( aRefName ) ) };
}


wxString SPICE_GENERATOR::Preview( const wxString& aModelName ) const
{
    wxString spiceCode = ModelLine( aModelName );

    wxString itemLine = ItemLine( "", aModelName );
    if( spiceCode != "" )
        spiceCode << "\n";

    spiceCode << itemLine;
    return spiceCode.Trim();
}


std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> SPICE_GENERATOR::GetInstanceParams() const
{
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> instanceParams;

    for( const SIM_MODEL::PARAM& param : m_model.GetParams() )
    {
        if( param.info.isSpiceInstanceParam )
            instanceParams.emplace_back( param );
    }

    return instanceParams;
}



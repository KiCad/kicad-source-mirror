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

#ifndef SIM_MODEL_SPICE_H
#define SIM_MODEL_SPICE_H

#include <sim/sim_model.h>


class SIM_MODEL_SPICE : public SIM_MODEL
{
public:
    DEFINE_ENUM_CLASS_WITH_ITERATOR( SPICE_PARAM,
        TYPE,
        MODEL,
        LIB
    )

    static constexpr auto LEGACY_TYPE_FIELD = "Spice_Primitive";
    static constexpr auto LEGACY_PINS_FIELD = "Spice_Node_Sequence";
    static constexpr auto LEGACY_MODEL_FIELD = "Spice_Model";
    static constexpr auto LEGACY_ENABLED_FIELD = "Spice_Netlist_Enabled";
    static constexpr auto LEGACY_LIB_FIELD = "Spice_Lib_File";


    SIM_MODEL_SPICE( TYPE aType );

    //bool ReadSpiceCode( const std::string& aSpiceCode ) override;
    void ReadDataSchFields( unsigned aSymbolPinCount, const std::vector<SCH_FIELD>* aFields ) override;
    void ReadDataLibFields( unsigned aSymbolPinCount, const std::vector<LIB_FIELD>* aFields ) override;

    void WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const override;
    void WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const override;


    wxString GenerateSpiceModelLine( const wxString& aModelName ) const override;
    wxString GenerateSpiceItemName( const wxString& aRefName ) const override;
    wxString GenerateSpiceItemLine( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aSymbolPinNumbers,
                                    const std::vector<wxString>& aPinNetNames ) const override;

protected:
    void CreatePins( unsigned aSymbolPinCount ) override;

    bool SetParamFromSpiceCode( const wxString& aParamName, const wxString& aParamValue,
                                SIM_VALUE_GRAMMAR::NOTATION aNotation
                                    = SIM_VALUE_GRAMMAR::NOTATION::SPICE ) override;

private:
    std::vector<PARAM::INFO> makeParamInfos();

    template <typename T>
    void readLegacyDataFields( unsigned aSymbolPinCount, const std::vector<T>* aFields );

    void parseLegacyPinsField( unsigned aSymbolPinCount, const wxString& aLegacyPinsField );

    std::vector<std::unique_ptr<PARAM::INFO>> m_paramInfos;
};

#endif // SIM_MODEL_SPICE_H

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

#ifndef SIM_MODEL_SOURCE_H
#define SIM_MODEL_SOURCE_H

#include <sim/sim_model.h>


namespace SIM_MODEL_SOURCE_GRAMMAR
{
    using namespace SIM_MODEL_GRAMMAR;

    struct pwlSep : plus<space> {};
    struct pwlValues : seq<opt<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>,
                           star<pwlSep,
                                number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>> {};
    struct pwlValuesGrammar : must<opt<sep>,
                                   pwlValues,
                                   opt<sep>,
                                   tao::pegtl::eof> {};
}


class SIM_MODEL_SOURCE : public SIM_MODEL
{
public:
    SIM_MODEL_SOURCE( TYPE aType );

    void ReadDataSchFields( unsigned aSymbolPinCount,
                            const std::vector<SCH_FIELD>* aFields ) override;
    void ReadDataLibFields( unsigned aSymbolPinCount,
                            const std::vector<LIB_FIELD>* aFields ) override;

    void WriteDataSchFields( std::vector<SCH_FIELD>& aFields ) const override;
    void WriteDataLibFields( std::vector<LIB_FIELD>& aFields ) const override;

    wxString GenerateSpiceModelLine( const wxString& aModelName ) const override;
    wxString GenerateSpiceItemLine( const wxString& aRefName,
                                    const wxString& aModelName,
                                    const std::vector<wxString>& aPinNetNames ) const override;

    bool SetParamValue( unsigned aParamIndex, const wxString& aValue,
                        SIM_VALUE_GRAMMAR::NOTATION aNotation ) override;

    bool HasAutofill() const override { return true; }

protected:
    wxString GenerateParamValuePair( const PARAM& aParam, bool& aIsFirst ) const override;

private:
    template <typename T>
    void inferredWriteDataFields( std::vector<T>& aFields ) const;

    std::vector<wxString> getPinNames() const override;

    wxString getParamValueString( const wxString& aParamName, const wxString& aDefaultValue ) const;

    static const std::vector<PARAM::INFO>& makeParamInfos( TYPE aType );

    static std::vector<PARAM::INFO> makeDcParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeSinParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePulseParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeExpParamInfos( wxString aPrefix, wxString aUnit );
    //static std::vector<PARAM::INFO> makeSfamParamInfos( wxString aPrefix, wxString aUnit );
    //static std::vector<PARAM::INFO> makeSffmParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePwlParamInfos( wxString aPrefix, wxString aQuantity,
                                                       wxString aUnit );
    static std::vector<PARAM::INFO> makeWhiteNoiseParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makePinkNoiseParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeBurstNoiseParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomUniformParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomNormalParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomExpParamInfos( wxString aPrefix, wxString aUnit );
    static std::vector<PARAM::INFO> makeRandomPoissonParamInfos( wxString aPrefix, wxString aUnit );

    static void appendAcParamInfos( std::vector<PARAM::INFO>& aParamInfos, wxString aUnit );

    bool m_isInferred;
};

#endif // SIM_MODEL_SOURCE_H

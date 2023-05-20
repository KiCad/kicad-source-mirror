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
#include <sim/sim_model_serializer.h>
#include <sim/spice_generator.h>


namespace SIM_MODEL_SOURCE_GRAMMAR
{
    using namespace SIM_MODEL_SERIALIZER_GRAMMAR;

    struct pwlSep : plus<space> {};
    struct pwlValues : seq<opt<number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>,
                           star<pwlSep,
                                number<SIM_VALUE::TYPE_FLOAT, NOTATION::SI>>> {};
    struct pwlValuesGrammar : must<opt<sep>,
                                   pwlValues,
                                   opt<sep>,
                                   tao::pegtl::eof> {};
}


class SPICE_GENERATOR_SOURCE : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::string ItemLine( const SPICE_ITEM& aItem ) const override;

private:
    std::string getParamValueString( const std::string& aParamName,
                                     const std::string& aDefaultValue ) const;
};


class SIM_MODEL_SOURCE_SERIALIZER : public SIM_MODEL_SERIALIZER
{
public:
    using SIM_MODEL_SERIALIZER::SIM_MODEL_SERIALIZER;
};


class SIM_MODEL_SOURCE : public SIM_MODEL
{
public:
    SIM_MODEL_SOURCE( TYPE aType );

    bool HasAutofill() const override { return true; }
    bool HasPrimaryValue() const override { return GetType() == TYPE::V || GetType() == TYPE::I; }

    std::vector<std::string> GetPinNames() const override;

protected:
    void doSetParamValue( int aParamIndex, const std::string& aValue ) override;

private:
    static const std::vector<PARAM::INFO>& makeParamInfos( TYPE aType );

    static std::vector<PARAM::INFO> makeDcParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeSinParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makePulseParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeExpParamInfos( std::string aPrefix, std::string aUnit );
    //static std::vector<PARAM::INFO> makeSfamParamInfos( std::string aPrefix, std::string aUnit );
    //static std::vector<PARAM::INFO> makeSffmParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<SIM_MODEL::PARAM::INFO> makeVcParamInfos( std::string aGainUnit );
    static std::vector<SIM_MODEL::PARAM::INFO> makeCcParamInfos( std::string aGainUnit );
    static std::vector<PARAM::INFO> makePwlParamInfos( std::string aPrefix, std::string aQuantity,
                                                       std::string aUnit );
    static std::vector<PARAM::INFO> makeWhiteNoiseParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makePinkNoiseParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeBurstNoiseParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeRandomUniformParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeRandomNormalParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeRandomExpParamInfos( std::string aPrefix, std::string aUnit );
    static std::vector<PARAM::INFO> makeRandomPoissonParamInfos( std::string aPrefix, std::string aUnit );

    static void appendAcParamInfos( std::vector<PARAM::INFO>& aParamInfos, std::string aUnit );
};

#endif // SIM_MODEL_SOURCE_H

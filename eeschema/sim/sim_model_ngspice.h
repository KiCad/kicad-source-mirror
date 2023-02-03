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

#ifndef SIM_MODEL_NGSPICE_H
#define SIM_MODEL_NGSPICE_H

#include <sim/sim_model_spice.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_NGSPICE : public SPICE_GENERATOR_SPICE
{
public:
    using SPICE_GENERATOR_SPICE::SPICE_GENERATOR_SPICE;

    std::vector<std::string> CurrentNames( const SPICE_ITEM& aItem ) const override;
};


class SIM_MODEL_NGSPICE : public SIM_MODEL_SPICE
{
public:
    friend struct MODEL_INFO_MAP;


    SIM_MODEL_NGSPICE( TYPE aType );

    void SetParamFromSpiceCode( const std::string& aParamName, const std::string& aValue,
                                SIM_VALUE_GRAMMAR::NOTATION aNotation ) override;

    std::vector<std::string> GetPinNames() const override;

    // Protected because it's accessed by QA tests.
protected:
    DEFINE_ENUM_CLASS_WITH_ITERATOR( MODEL_TYPE,
        NONE,
        //RESISTOR,
        //CAPACITOR,
        //INDUCTOR,
        //LTRA,
        //TRANLINE,
        //URC,
        //TRANSLINE,
        //SWITCH,
        //CSWITCH,
        DIODE,
        BJT,
        VBIC,
        HICUM2,
        JFET,
        JFET2,
        MES,
        MESA,
        HFET1,
        HFET2,
        VDMOS,
        MOS1,
        MOS2,
        MOS3,
        BSIM1,
        BSIM2,
        MOS6,
        BSIM3,
        MOS9,
        B4SOI,
        BSIM4,
        B3SOIFD,
        B3SOIDD,
        B3SOIPD,
        HISIM2,
        HISIMHV1,
        HISIMHV2
    )

    struct MODEL_INFO
    {
        std::string name;
        std::string variant1;
        std::string variant2;
        std::vector<std::string> pinNames;
        std::string description;
        std::vector<SIM_MODEL::PARAM::INFO> modelParams;
        std::vector<SIM_MODEL::PARAM::INFO> instanceParams;
    };

    static const MODEL_INFO& ModelInfo( MODEL_TYPE aType );

    int doFindParam( const std::string& aParamName ) const override;

private:
    bool requiresSpiceModelLine() const override
    {
        return getModelType() != MODEL_TYPE::NONE;
    }

    bool canSilentlyIgnoreParam( const std::string& aParamName );

    MODEL_TYPE getModelType() const;
    bool getIsOtherVariant();
};

#endif /* SIM_MODEL_NGSPICE_H */

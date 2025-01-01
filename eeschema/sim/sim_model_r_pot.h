/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2022 Mikolaj Wielgus
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef SIM_MODEL_R_POT_H
#define SIM_MODEL_R_POT_H

#include <sim/sim_model.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_R_POT : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::string TunerCommand( const SPICE_ITEM& aItem, double aValue ) const override;
};


class SIM_MODEL_R_POT : public SIM_MODEL
{
public:
    SIM_MODEL_R_POT();

    const PARAM* GetTunerParam() const override { return FindParam( "pos" ); }
    bool HasPrimaryValue() const override { return true; }

    std::vector<std::string> GetPinNames() const override { return { "r0", "wiper", "r1" }; }

private:
    static const std::vector<PARAM::INFO> makeParamInfos();
};

#endif // SIM_MODEL_POT_H

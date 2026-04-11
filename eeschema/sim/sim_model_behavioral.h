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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_MODEL_BEHAVIORAL_H
#define SIM_MODEL_BEHAVIORAL_H

#include <sim/sim_model.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_BEHAVIORAL : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::string ItemLine( const SPICE_ITEM& aItem ) const override;
};


// Not used for now.
class SIM_MODEL_BEHAVIORAL : public SIM_MODEL
{
public:
    SIM_MODEL_BEHAVIORAL( TYPE aType );

    std::vector<std::string> GetPinNames() const override { return { "+", "-" }; }

private:
    bool parseValueField( const std::string& aValueField );

    static PARAM::INFO makeParams( const std::string& aName, const std::string& aDescription,
                                   const std::string& aUnit );
};

#endif // SIM_MODEL_BEHAVIORAL_H

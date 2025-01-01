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

#ifndef SIM_MODEL_SUBCIRCUIT_H
#define SIM_MODEL_SUBCIRCUIT_H

#include <sim/sim_model_spice.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_SUBCKT : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::vector<std::string> CurrentNames( const SPICE_ITEM& aItem ) const override;
};


class SPICE_MODEL_PARSER_SUBCKT : public SPICE_MODEL_PARSER
{
public:
    using SPICE_MODEL_PARSER::SPICE_MODEL_PARSER;

    void ReadModel( const SIM_LIBRARY_SPICE& aLibrary, const std::string& aSpiceCode ) override;
};


class SIM_MODEL_SUBCKT : public SIM_MODEL_SPICE
{
public:
    friend class SPICE_MODEL_PARSER_SUBCKT;

    SIM_MODEL_SUBCKT();

    void SetBaseModel( const SIM_MODEL& aBaseModel ) override;

    std::string GetSpiceCode() const;

private:
    std::vector<std::unique_ptr<PARAM::INFO>> m_paramInfos;
};

#endif // SIM_MODEL_SUBCIRCUIT_H

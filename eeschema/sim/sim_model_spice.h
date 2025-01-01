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

#ifndef SIM_MODEL_SPICE_H
#define SIM_MODEL_SPICE_H

#include <sim/sim_model.h>
#include <sim/spice_generator.h>
#include <sim/spice_model_parser.h>

class SIM_LIBRARY_SPICE;


class SPICE_GENERATOR_SPICE : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string Preview( const SPICE_ITEM& aItem ) const override;
};


class SIM_MODEL_SPICE : public SIM_MODEL
{
public:
    friend class SPICE_GENERATOR_SPICE;
    friend class SPICE_MODEL_PARSER;

    static std::unique_ptr<SIM_MODEL_SPICE> Create( const SIM_LIBRARY_SPICE& aLibrary,
                                                    const std::string& aSpiceCode,
                                                    bool aFirstPass, REPORTER& aReporter );

    SIM_MODEL_SPICE( TYPE aType,
                     std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator );

    SIM_MODEL_SPICE( TYPE aType,
                     std::unique_ptr<SPICE_GENERATOR> aSpiceGenerator,
                     std::unique_ptr<SPICE_MODEL_PARSER> aSpiceModelParser );

protected:
    virtual void SetParamFromSpiceCode( const std::string& aParamName,
                                        const std::string& aParamValue,
                                        SIM_VALUE_GRAMMAR::NOTATION aNotation = SIM_VALUE_GRAMMAR::NOTATION::SPICE );

    std::string m_spiceCode;

private:
    std::unique_ptr<SPICE_MODEL_PARSER> m_spiceModelParser;
};

#endif // SIM_MODEL_SPICE_H

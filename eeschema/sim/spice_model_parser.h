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

#ifndef SPICE_MODEL_PARSER_H
#define SPICE_MODEL_PARSER_H

#include <sim/sim_model.h>

class SIM_MODEL_SPICE;
class SIM_LIBRARY_SPICE;


class SPICE_MODEL_PARSER
{
public:
    static bool ReadType( const SIM_LIBRARY_SPICE& aLibrary, const std::string& aSpiceCode,
                          SIM_MODEL::TYPE* aType, bool aFirstPass );

    SPICE_MODEL_PARSER( SIM_MODEL_SPICE& aModel ) : m_model( aModel ) {}
    virtual ~SPICE_MODEL_PARSER() = default;

    virtual void ReadModel( const SIM_LIBRARY_SPICE& aLibrary, const std::string& aSpiceCode );

protected:
    static SIM_MODEL::TYPE ReadTypeFromSpiceStrings( const std::string& aTypeString,
                                                     const std::string& aLevel = "",
                                                     const std::string& aVersion = "",
                                                     bool aSkipDefaultLevel = true );

    SIM_MODEL_SPICE& m_model;
};

#endif // SPICE_MODEL_PARSER

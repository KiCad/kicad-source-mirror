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

#ifndef SPICE_GENERATOR_H
#define SPICE_GENERATOR_H

#include <sim/sim_model.h>

class SCH_FIELD;


struct SPICE_ITEM
{
    std::string              refName;
    std::vector<std::string> pinNumbers;
    std::vector<std::string> pinNetNames;
    std::string              baseModelName;
    std::string              modelName;
    const SIM_MODEL*         model = nullptr;
    std::vector<SCH_FIELD>   fields;
};


class SPICE_GENERATOR
{
public:

    SPICE_GENERATOR( const SIM_MODEL& aModel ) : m_model( aModel ) {}
    virtual ~SPICE_GENERATOR() = default;

    virtual std::string ModelName( const SPICE_ITEM& aItem ) const;
    virtual std::string ModelLine( const SPICE_ITEM& aItem ) const;

    virtual std::string ItemLine( const SPICE_ITEM& aItem ) const;
    virtual std::string ItemName( const SPICE_ITEM& aItem ) const;
    virtual std::string ItemPins( const SPICE_ITEM& aItem ) const;
    virtual std::string ItemModelName( const SPICE_ITEM& aItem ) const;
    virtual std::string ItemParams() const;

    virtual std::string TunerCommand( const SPICE_ITEM& aItem, double aValue ) const;

    virtual std::vector<std::string> CurrentNames( const SPICE_ITEM& aItem ) const;

    virtual std::string Preview( const SPICE_ITEM& aItem ) const;

protected:
    virtual std::vector<std::reference_wrapper<const SIM_MODEL_PIN>> GetPins() const
    {
        return m_model.GetPins();
    }

    const SIM_MODEL& m_model;
};

#endif // SPICE_GENERATOR_H

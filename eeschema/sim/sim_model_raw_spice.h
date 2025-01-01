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

#ifndef SIM_MODEL_RAW_SPICE_H
#define SIM_MODEL_RAW_SPICE_H

#include <sim/sim_model_spice.h>
#include <sim/spice_generator.h>


class SPICE_GENERATOR_RAW_SPICE : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const SPICE_ITEM& aItem ) const override;

    std::string ItemName( const SPICE_ITEM& aItem ) const override;
    std::string ItemPins( const SPICE_ITEM& aItem ) const override;
    std::string ItemModelName( const SPICE_ITEM& aItem ) const override;
    std::string ItemParams() const override;

    std::string Preview( const SPICE_ITEM& aItem ) const override;
};


class SIM_MODEL_RAW_SPICE : public SIM_MODEL
{
public:
    friend class SPICE_GENERATOR_RAW_SPICE;

    DEFINE_ENUM_CLASS_WITH_ITERATOR( SPICE_PARAM,
        TYPE,
        MODEL,
        LIB
    )

    SIM_MODEL_RAW_SPICE( const std::string& aSpiceSource = "" );

    void SetSource( const std::string& aSpiceSource ) { m_spiceCode = aSpiceSource; }

    std::string GetSource() const
    {
        if( m_baseModel )
            return static_cast<const SIM_MODEL_RAW_SPICE*>( m_baseModel )->GetSource();

        return m_spiceCode;
    }

    void AssignSymbolPinNumberToModelPin( const std::string& aModelPinName,
                                          const wxString& aSymbolPinNumber ) override;

private:
    static std::vector<PARAM::INFO> makeParamInfos();
    bool requiresSpiceModelLine( const SPICE_ITEM& aItem ) const override { return false; }

private:
    std::string m_spiceCode;
};

#endif // SIM_MODEL_RAW_SPICE_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef SIM_MODEL_KIBIS_H
#define SIM_MODEL_KIBIS_H

#include <sim/sim_model.h>
#include <sim/spice_generator.h>

class SIM_LIBRARY_KIBIS;


class SPICE_GENERATOR_KIBIS : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelLine( const std::string& aModelName ) const override;
    std::vector<std::string> CurrentNames( const std::string& aRefName ) const override;

protected:
    std::vector<std::reference_wrapper<const SIM_MODEL::PARAM>> GetInstanceParams() const override;
};

class SIM_MODEL_KIBIS : public SIM_MODEL
{
    friend class SIM_LIBRARY_KIBIS;

    static constexpr auto DRIVER_RECT = "rect";
    static constexpr auto DRIVER_STUCKH = "stuck high";
    static constexpr auto DRIVER_STUCKL = "stuck low";
    static constexpr auto DRIVER_HIGHZ = "high Z";

public:
    SIM_MODEL_KIBIS( TYPE aType, std::string aWfType = "NoInit" );

    // @brief Special copy constructor
    // creates a a model with aType, but tries to match parameters from aSource.
    SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource );

    SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource, const std::vector<LIB_FIELD>& aFields );
    SIM_MODEL_KIBIS( TYPE aType, SIM_MODEL_KIBIS& aSource, const std::vector<SCH_FIELD>& aFields );

    void SetParameters( TYPE aType, std::string aWfType ); // To change between device / driver
    void SetParameters( std::string aWfType );             // To change between device / driver

    std::vector<std::pair<std::string, std::string>> GetIbisPins() const
    {
        return GetBaseModel()
                       ? dynamic_cast<const SIM_MODEL_KIBIS*>( GetBaseModel() )->GetIbisPins()
                       : m_ibisPins;
    };

    std::vector<std::string> GetIbisModels() const { return m_ibisModels; };

    std::string GetComponentName() const
    {
        return GetBaseModel()
                       ? dynamic_cast<const SIM_MODEL_KIBIS*>( GetBaseModel() )->GetComponentName()
                       : m_componentName;
    };

    /** @brief update the list of available models based on the pin number.
     * */
    bool ChangePin( SIM_LIBRARY_KIBIS& aLib, std::string aPinNumber );

protected:
    void CreatePins( unsigned aSymbolPinCount ) override;
    std::vector<std::pair<std::string, std::string>> m_ibisPins;
    std::string                                   m_componentName;

private:
    bool requiresSpiceModelLine() const override { return true; }
    static const std::vector<PARAM::INFO> makeKibisParamInfos( TYPE aType, std::string aWfType );

    std::vector<std::unique_ptr<PARAM::INFO>> m_paramInfos;
    std::vector<std::string>                  m_ibisModels;
};

#endif // SIM_MODEL_KIBIS_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef SIM_MODEL_IBIS_H
#define SIM_MODEL_IBIS_H

#include <sim/kibis/kibis.h>
#include <sim/sim_model.h>
#include <sim/spice_generator.h>
#include <project.h>

class SIM_LIBRARY_IBIS;
class REPORTER;
class SCHEMATIC;

class SPICE_GENERATOR_IBIS : public SPICE_GENERATOR
{
public:
    using SPICE_GENERATOR::SPICE_GENERATOR;

    std::string ModelName( const SPICE_ITEM& aItem ) const override;
    std::string ModelLine( const SPICE_ITEM& aItem ) const override;
    std::vector<std::string> CurrentNames( const SPICE_ITEM& aItem ) const override;

    std::string IbisDevice( const SPICE_ITEM& aItem, SCHEMATIC* aSchematic,
                            const wxString& aCacheDir, REPORTER& aReporter ) const;
};

class SIM_MODEL_IBIS : public SIM_MODEL
{
    friend class SIM_LIBRARY_IBIS;

public:
    SIM_MODEL_IBIS( TYPE aType );

    // @brief Special copy constructor
    // creates a a model with aType, but tries to match parameters from aSource.
    SIM_MODEL_IBIS( TYPE aType, const SIM_MODEL_IBIS& aSource );

    std::vector<std::pair<std::string, std::string>> GetIbisPins() const
    {
        return m_sourceModel ? m_sourceModel->GetIbisPins() : m_ibisPins;
    }

    std::vector<std::string> GetIbisModels() const { return m_ibisModels; };

    std::string GetComponentName() const
    {
        return m_sourceModel ? m_sourceModel->GetComponentName() : m_componentName;
    }


    const PARAM& GetParam( unsigned aParamIndex ) const override
    {
        return m_params.at( aParamIndex );
    };

    /**
     * @brief update the list of available models based on the pin number.
     */
    bool ChangePin( const SIM_LIBRARY_IBIS& aLib, const std::string& aPinNumber );

    void SetBaseModel( const SIM_MODEL& aBaseModel ) override;

    void SwitchSingleEndedDiff( bool aDiff ) override;
    bool CanDifferential() const { return m_enableDiff; } ;
    bool m_enableDiff;

private:
    bool requiresSpiceModelLine( const SPICE_ITEM& aItem ) const override { return true; }

    static std::vector<PARAM::INFO> makeParamInfos( TYPE aType );
    static std::vector<PARAM::INFO> makeDcWaveformParamInfos();
    static std::vector<PARAM::INFO> makeRectWaveformParamInfos();
    static std::vector<PARAM::INFO> makePrbsWaveformParamInfos();

    const SIM_MODEL_IBIS*                            m_sourceModel;
    std::vector<std::string>                         m_ibisModels;
    std::vector<std::pair<std::string, std::string>> m_ibisPins;
    std::string                                      m_componentName;
};

#endif // SIM_MODEL_KIBIS_H

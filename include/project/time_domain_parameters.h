/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef KICAD_TIME_DOMAIN_PARAMETERS_H
#define KICAD_TIME_DOMAIN_PARAMETERS_H

#include <settings/nested_settings.h>
#include <layer_ids.h>

/**
 * Represents a single line in the time domain configuration via overrides configuration grid
 */
struct TUNING_PROFILE_VIA_OVERRIDE_ENTRY
{
    PCB_LAYER_ID m_SignalLayerFrom;
    PCB_LAYER_ID m_SignalLayerTo;
    PCB_LAYER_ID m_ViaLayerFrom;
    PCB_LAYER_ID m_ViaLayerTo;
    int          m_Delay;

    bool operator<( const TUNING_PROFILE_VIA_OVERRIDE_ENTRY& other ) const
    {
        if( m_SignalLayerFrom != other.m_SignalLayerFrom )
            return IsCopperLayerLowerThan( m_SignalLayerFrom, other.m_SignalLayerFrom );

        if( m_SignalLayerTo != other.m_SignalLayerTo )
            return IsCopperLayerLowerThan( m_SignalLayerTo, other.m_SignalLayerTo );

        if( m_ViaLayerFrom != other.m_ViaLayerFrom )
            return IsCopperLayerLowerThan( m_ViaLayerFrom, other.m_ViaLayerFrom );

        if( m_ViaLayerTo != other.m_ViaLayerTo )
            return IsCopperLayerLowerThan( m_ViaLayerTo, other.m_ViaLayerTo );

        return m_Delay < other.m_Delay;
    }
};


/**
 * Represents a single line in the time domain configuration net class configuration grid
 */
struct TIME_DOMAIN_TUNING_PROFILE
{
    wxString                                       m_ProfileName;
    int                         m_ViaPropagationDelay;
    std::map<PCB_LAYER_ID, int> m_LayerPropagationDelays;
    std::vector<TUNING_PROFILE_VIA_OVERRIDE_ENTRY> m_ViaOverrides;
};


/**
 * TIME_DOMAIN_PARAMETERS stores the configuration for time-domain tuning
 */
class KICOMMON_API TIME_DOMAIN_PARAMETERS final : public NESTED_SETTINGS
{
public:
    TIME_DOMAIN_PARAMETERS( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~TIME_DOMAIN_PARAMETERS();

    bool operator==( const TIME_DOMAIN_PARAMETERS& aOther ) const;

    bool operator!=( const TIME_DOMAIN_PARAMETERS& aOther ) const { return !operator==( aOther ); }

    void ClearDelayProfiles() { m_delayProfiles.clear(); }

    void AddDelayProfile( TIME_DOMAIN_TUNING_PROFILE&& aTraceEntry )
    {
        m_delayProfiles.emplace_back( std::move( aTraceEntry ) );
    }

    const std::vector<TIME_DOMAIN_TUNING_PROFILE>& GetDelayProfiles() const { return m_delayProfiles; }

private:
    std::vector<TIME_DOMAIN_TUNING_PROFILE> m_delayProfiles;
};

#endif // KICAD_NET_SETTINGS_H

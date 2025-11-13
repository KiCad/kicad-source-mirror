/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#ifndef KICAD_TUNING_PROFILES_H
#define KICAD_TUNING_PROFILES_H

#include <settings/nested_settings.h>
#include <layer_ids.h>

/**
 * Represents a single line in the time domain configuration via overrides configuration grid
 */
struct DELAY_PROFILE_VIA_OVERRIDE_ENTRY
{
    PCB_LAYER_ID m_SignalLayerFrom;
    PCB_LAYER_ID m_SignalLayerTo;
    PCB_LAYER_ID m_ViaLayerFrom;
    PCB_LAYER_ID m_ViaLayerTo;
    int          m_Delay;

    bool operator<( const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& other ) const
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

    bool operator==( const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& other ) const
    {
        if( m_SignalLayerFrom != other.m_SignalLayerFrom )
            return false;

        if( m_SignalLayerTo != other.m_SignalLayerTo )
            return false;

        if( m_ViaLayerFrom != other.m_ViaLayerFrom )
            return false;

        if( m_ViaLayerTo != other.m_ViaLayerTo )
            return false;

        if( m_Delay != other.m_Delay )
            return false;

        return true;
    }
};

class TUNING_PROFILES;

/**
* Represents a single line in a time domain profile track propagation setup
*/
class DELAY_PROFILE_TRACK_PROPAGATION_ENTRY
{
public:
    friend class TUNING_PROFILES;

    void SetSignalLayer( const PCB_LAYER_ID aLayer ) { m_signalLayer = aLayer; }
    void SetTopReferenceLayer( const PCB_LAYER_ID aLayer ) { m_topReferenceLayer = aLayer; }
    void SetBottomReferenceLayer( const PCB_LAYER_ID aLayer ) { m_bottomReferenceLayer = aLayer; }

    void SetWidth( const int aWidth ) { m_width = aWidth; }
    void SetDiffPairGap( const int aDiffPairGap ) { m_diffPairGap = aDiffPairGap; }
    void SetDelay( const int aDelay ) { m_delay = aDelay; }
    void SetEnableTimeDomainTuning( bool aEnable ) { m_enableTimeDomainTuning = aEnable; }

    PCB_LAYER_ID GetSignalLayer() const { return m_signalLayer; }
    PCB_LAYER_ID GetTopReferenceLayer() const { return m_topReferenceLayer; }
    PCB_LAYER_ID GetBottomReferenceLayer() const { return m_bottomReferenceLayer; }

    int GetWidth() const { return m_width; }
    int GetDiffPairGap() const { return m_diffPairGap; }
    int GetDelay( const bool aForce = false ) const { return ( m_enableTimeDomainTuning || aForce ) ? m_delay : 0; }

    bool operator==( const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& other ) const
    {
        if( m_signalLayer != other.m_signalLayer )
            return false;

        if( m_topReferenceLayer != other.m_topReferenceLayer )
            return false;

        if( m_bottomReferenceLayer != other.m_bottomReferenceLayer )
            return false;

        if( m_width != other.m_width )
            return false;

        if( m_diffPairGap != other.m_diffPairGap )
            return false;

        if( m_delay != other.m_delay )
            return false;

        if( m_enableTimeDomainTuning != other.m_enableTimeDomainTuning )
            return false;

        return true;
    }

private:
    PCB_LAYER_ID m_signalLayer{ UNDEFINED_LAYER };
    PCB_LAYER_ID m_topReferenceLayer{ UNDEFINED_LAYER };
    PCB_LAYER_ID m_bottomReferenceLayer{ UNDEFINED_LAYER };

    int m_width{ 0 };
    int m_diffPairGap{ 0 };
    int m_delay{ 0 };

    bool m_enableTimeDomainTuning{ false };
};


/**
 * Represents a single line in the tuning profile configuration grid
 */
struct TUNING_PROFILE
{
    enum class PROFILE_TYPE
    {
        SINGLE,
        DIFFERENTIAL
    };

    wxString                                           m_ProfileName;
    PROFILE_TYPE                                       m_Type;
    double                                             m_TargetImpedance;
    bool                                               m_EnableTimeDomainTuning;
    std::vector<DELAY_PROFILE_TRACK_PROPAGATION_ENTRY> m_TrackPropagationEntries;
    int                                                m_ViaPropagationDelay;
    std::vector<DELAY_PROFILE_VIA_OVERRIDE_ENTRY>      m_ViaOverrides;

    // This is not persisted - but is used for quick lookup for track statistics calculations
    std::map<PCB_LAYER_ID, DELAY_PROFILE_TRACK_PROPAGATION_ENTRY> m_TrackPropagationEntriesMap;

    bool operator==( const TUNING_PROFILE& aOther ) const
    {
        if( m_ProfileName != aOther.m_ProfileName )
            return false;

        if( m_Type != aOther.m_Type )
            return false;

        if( m_TargetImpedance != aOther.m_TargetImpedance )
            return false;

        if( m_EnableTimeDomainTuning != aOther.m_EnableTimeDomainTuning )
            return false;

        if( m_TrackPropagationEntries != aOther.m_TrackPropagationEntries )
            return false;

        if( m_ViaPropagationDelay != aOther.m_ViaPropagationDelay )
            return false;

        if( m_ViaOverrides != aOther.m_ViaOverrides )
            return false;

        return true;
    }
};


/**
 * TUNING_PROFILES stores the configuration for impedance / delay tuning profiles
 */
class KICOMMON_API TUNING_PROFILES final : public NESTED_SETTINGS
{
public:
    TUNING_PROFILES( JSON_SETTINGS* aParent, const std::string& aPath );

    virtual ~TUNING_PROFILES();

    bool operator==( const TUNING_PROFILES& aOther ) const;

    bool operator!=( const TUNING_PROFILES& aOther ) const { return !operator==( aOther ); }

    void ClearTuningProfiles() { m_tuningProfiles.clear(); }

    void AddTuningProfile( TUNING_PROFILE&& aTraceEntry ) { m_tuningProfiles.emplace_back( std::move( aTraceEntry ) ); }

    const std::vector<TUNING_PROFILE>& GetTuningProfiles() const { return m_tuningProfiles; }

    TUNING_PROFILE& GetTuningProfile( wxString aProfileName );

private:
    std::vector<TUNING_PROFILE> m_tuningProfiles;

    TUNING_PROFILE m_nullDelayProfile;
};

#endif // KICAD_TUNING_PROFILES_H

/*
* This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef PCB_NEW_TUNING_PROFILE_PARAMETERS_USER_DEFINED_H
#define PCB_NEW_TUNING_PROFILE_PARAMETERS_USER_DEFINED_H


#include <length_delay_calculation/tuning_profile_parameters_iface.h>
#include <project/tuning_profiles.h>

class TUNING_PROFILE_PARAMETERS_USER_DEFINED final : public TUNING_PROFILE_PARAMETERS_IFACE
{
public:
    explicit TUNING_PROFILE_PARAMETERS_USER_DEFINED( BOARD* aBoard, LENGTH_DELAY_CALCULATION* aCalculation ) :
            TUNING_PROFILE_PARAMETERS_IFACE( aBoard, aCalculation )
    {
    }

    /**
     * Event called by the length and time calculation architecture if netclass definitions have changed. This can be
     * used to invalidate any calculation / simulation caches.
     */
    void OnSettingsChanged() override;

    /**
     * Gets the propagation delays (in internal units) for the given items in the given geometry context.
     * This assumes that all items are in the same netclass
     *
     * @param aItems the board items to query propagation delay for
     * @param aContext the geometry context in which to query to propagation delay
     */
    std::vector<int64_t> GetPropagationDelays( const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                               const TUNING_PROFILE_GEOMETRY_CONTEXT&            aContext ) override;

    /**
     * Gets the propagation delay (in internal units) for the given item in the given geometry context
     *
     * @param aItem the board item to query propagation delay for
     * @param aContext the geometry context in which to query to propagation delay
     */
    int64_t GetPropagationDelay( const LENGTH_DELAY_CALCULATION_ITEM&   aItem,
                                 const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) override;

    /**
     * Gets the track length (in internal distance units) required for the given propagation delay (in internal time
     * units). The track length should be calculated with the given geometry context.
     *
     * @param aContext the geometry context in which to query to propagation delay
     * @param aDelay the propagation delay for which a length should be calculated
     */
    int64_t GetTrackLengthForPropagationDelay( int64_t                                aDelay,
                                               const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) override;

    /**
     * Gets the propagation delay for the given shape line chain
     *
     * @param aShape is the shape to calculate delay for
     * @param aContext is the geometry context for which to query to propagation delay
     */
    int64_t CalculatePropagationDelayForShapeLineChain( const SHAPE_LINE_CHAIN&                aShape,
                                                        const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) override;

private:
    void rebuildCaches();

    /// Cache key for use with std::map
    struct VIA_OVERRIDE_CACHE_KEY
    {
        PCB_LAYER_ID SignalStart;
        PCB_LAYER_ID SignalEnd;
        PCB_LAYER_ID ViaStart;
        PCB_LAYER_ID ViaEnd;

        bool operator<( const VIA_OVERRIDE_CACHE_KEY& aOther ) const
        {
            if( SignalStart < aOther.SignalStart )
                return true;
            if( aOther.SignalStart < SignalStart )
                return false;
            if( SignalEnd < aOther.SignalEnd )
                return true;
            if( aOther.SignalEnd < SignalEnd )
                return false;
            if( ViaStart < aOther.ViaStart )
                return true;
            if( aOther.ViaStart < ViaStart )
                return false;
            return ViaEnd < aOther.ViaEnd;
        }
    };

    /**
     * Gets the propagation delay (in internal units) for the given item in the given geometry context,
     * for items in the pre-found net class.
     *
     * Preconditions:
     *   - aNetClass must not be null
     *   - Calculation caches must be valid
     *
     * @param aItem the board item to query propagation delay for
     * @param aContext the geometry context in which to query to propagation delay
     * @param aDelayProfile the time domain tuning profile for the given item
     */
    int64_t getPropagationDelay( const LENGTH_DELAY_CALCULATION_ITEM&   aItem,
                                 const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext,
                                 const TUNING_PROFILE*                  aDelayProfile ) const;

    /**
     * Gets the tuning profile pointer for the given tuning profile name
     *
     * @param aDelayProfileName the tuning profile name to return
     * @returns Valid pointer to a tuning profile, or nullptr if no profile found
     */
    const TUNING_PROFILE* GetTuningProfile( const wxString& aDelayProfileName );

    /// Cached map of tuning profile names to per-layer time domain parameters
    std::map<wxString, const TUNING_PROFILE*> m_delayProfilesCache;

    /// Cached per-tuning profile via overrides
    std::map<wxString, std::map<VIA_OVERRIDE_CACHE_KEY, int64_t>> m_viaOverridesCache;
};

#endif //PCB_NEW_TUNING_PROFILE_PARAMETERS_USER_DEFINED_H

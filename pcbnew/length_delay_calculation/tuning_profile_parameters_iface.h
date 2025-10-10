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

#ifndef PCBNEW_TUNING_PROFILE_PARAMETERS_IFACE_H
#define PCBNEW_TUNING_PROFILE_PARAMETERS_IFACE_H

#include <length_delay_calculation/length_delay_calculation_item.h>
#include <netclass.h>

class LENGTH_DELAY_CALCULATION;

/**
* A data structure to contain basic geometry data which can affect signal propagation calculations.
*/
struct TUNING_PROFILE_GEOMETRY_CONTEXT
{
    /// The net class this track belongs to
    const NETCLASS* NetClass;

    /// The layer this track is on
    PCB_LAYER_ID Layer;

    /// The width (in internal units) of the track
    int64_t Width;

    /// Whether this track or via is a member of a coupled differential pair
    bool IsDiffPairCoupled{ false };

    /// The gap between coupled tracks
    int64_t DiffPairCouplingGap{ 0 };
};


/**
 * Interface for providers of tuning profile parameter information. This interface is consumed by the
 * LENGTH_TIME_CALCULATOR object to convert space-domain physical layout information (e.g. track lengths) in to
 * time-domain propagation information.
 */
class TUNING_PROFILE_PARAMETERS_IFACE
{
public:
    explicit TUNING_PROFILE_PARAMETERS_IFACE( BOARD* aBoard, LENGTH_DELAY_CALCULATION* aCalculation ) :
            m_board{ aBoard },
            m_lengthCalculation{ aCalculation }
    {
    }

    virtual ~TUNING_PROFILE_PARAMETERS_IFACE() = default;

    /**
     * Event called by the length and time calculation architecture if the board stackup has changed. This can be used
     * to invalidate any calculation / simulation caches.
     */
    virtual void OnStackupChanged() {};

    /**
     * Event called by the length and time calculation architecture if netclass definitions have changed. This can be
     * used to invalidate any calculation / simulation caches.
     */
    virtual void OnSettingsChanged() {};

    /**
     * Gets the propagation delays (in internal units) for the given items in the given geometry context
     *
     * @param aItems the board items to query propagation delay for
     * @param aContext the geometry context in which to query to propagation delay
     */
    virtual std::vector<int64_t> GetPropagationDelays( const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                       const TUNING_PROFILE_GEOMETRY_CONTEXT&            aContext ) = 0;

    /**
     * Gets the propagation delay (in internal units) for the given item in the given geometry context
     *
     * @param aItem the board item to query propagation delay for
     * @param aContext is the geometry context in which to query to propagation delay
     */
    virtual int64_t GetPropagationDelay( const LENGTH_DELAY_CALCULATION_ITEM&   aItem,
                                         const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) = 0;

    /**
     * Gets the track length (in internal distance units) required for the given propagation delay (in internal time
     * units). The track length should be calculated with the given geometry context.
     *
     * @param aContext the geometry context in which to query to propagation delay
     * @param aDelay the propagation delay for which a length should be calculated
     */
    virtual int64_t GetTrackLengthForPropagationDelay( int64_t                                aDelay,
                                                       const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) = 0;

    /**
     * Gets the propagation delay for the given shape line chain
     *
     * @param aShape is the shape to calculate delay for
     * @param aContext is the geometry context for which to query to propagation delay
     */
    virtual int64_t CalculatePropagationDelayForShapeLineChain( const SHAPE_LINE_CHAIN&                aShape,
                                                                const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext ) = 0;

protected:
    /// The board all calculations are for
    BOARD* m_board;

    /// The parent length / delay calculation object
    LENGTH_DELAY_CALCULATION* m_lengthCalculation;
};

#endif //PCBNEW_TUNING_PROFILE_PARAMETERS_IFACE_H

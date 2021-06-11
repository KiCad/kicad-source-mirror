/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __PNS_SIZES_SETTINGS_H
#define __PNS_SIZES_SETTINGS_H

#include <map>
#include <core/optional.h>

#include "pcb_track.h" // for VIATYPE_T

class BOARD;
class BOARD_DESIGN_SETTINGS;

namespace PNS {

class ITEM;

class SIZES_SETTINGS
{
public:
    SIZES_SETTINGS() :
            m_minClearance( 0 ),
            m_trackWidth( 155000 ),
            m_trackWidthIsExplicit( true ),
            m_viaType( VIATYPE::THROUGH ),
            m_viaDiameter( 600000 ),
            m_viaDrill( 250000 ),
            m_diffPairWidth( 125000 ),
            m_diffPairGap( 180000 ),
            m_diffPairViaGap( 180000 ),
            m_diffPairViaGapSameAsTraceGap( true ),
            m_holeToHole( 0 )
    {};

    ~SIZES_SETTINGS() {};

    void ClearLayerPairs();
    void AddLayerPair( int aL1, int aL2 );

    int MinClearance() const { return m_minClearance; }
    void SetMinClearance( int aClearance ) { m_minClearance = aClearance; }

    int TrackWidth() const { return m_trackWidth; }
    void SetTrackWidth( int aWidth ) { m_trackWidth = aWidth; }

    bool TrackWidthIsExplicit() const { return m_trackWidthIsExplicit; }
    void SetTrackWidthIsExplicit( bool aIsExplicit ) { m_trackWidthIsExplicit = aIsExplicit; }

    int DiffPairWidth() const { return m_diffPairWidth; }
    int DiffPairGap() const { return m_diffPairGap; }

    int DiffPairViaGap() const
    {
        return m_diffPairViaGapSameAsTraceGap ? m_diffPairGap : m_diffPairViaGap;
    }

    bool DiffPairViaGapSameAsTraceGap() const { return m_diffPairViaGapSameAsTraceGap; }

    void SetDiffPairWidth( int aWidth ) { m_diffPairWidth = aWidth; }
    void SetDiffPairGap( int aGap ) { m_diffPairGap = aGap; }
    void SetDiffPairViaGapSameAsTraceGap ( bool aEnable ) { m_diffPairViaGapSameAsTraceGap = aEnable; }
    void SetDiffPairViaGap( int aGap ) { m_diffPairViaGap = aGap; }

    int ViaDiameter() const { return m_viaDiameter; }
    void SetViaDiameter( int aDiameter ) { m_viaDiameter = aDiameter; }

    int ViaDrill() const { return m_viaDrill; }
    void SetViaDrill( int aDrill ) { m_viaDrill = aDrill; }

    OPT<int> PairedLayer( int aLayerId )
    {
        if( m_layerPairs.find(aLayerId) == m_layerPairs.end() )
            return OPT<int>();

        return m_layerPairs[aLayerId];
    }

    int GetLayerTop() const;
    int GetLayerBottom() const;

    void SetHoleToHole( int aHoleToHole ) { m_holeToHole = aHoleToHole; }
    int GetHoleToHole() const { return m_holeToHole; }

    void SetViaType( VIATYPE aViaType ) { m_viaType = aViaType; }
    VIATYPE ViaType() const { return m_viaType; }

private:
    int     m_minClearance;
    int     m_trackWidth;
    bool    m_trackWidthIsExplicit;

    VIATYPE m_viaType;
    int     m_viaDiameter;
    int     m_viaDrill;

    int     m_diffPairWidth;
    int     m_diffPairGap;
    int     m_diffPairViaGap;
    bool    m_diffPairViaGapSameAsTraceGap;

    int     m_holeToHole;

    std::map<int, int> m_layerPairs;
};

}

#endif // __PNS_SIZES_SETTINGS_H

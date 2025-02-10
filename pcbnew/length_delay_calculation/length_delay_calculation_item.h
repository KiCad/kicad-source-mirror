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

#ifndef PCBNEW_LENGTH_DELAY_CALCULATION_ITEM_H
#define PCBNEW_LENGTH_DELAY_CALCULATION_ITEM_H

#include <geometry/shape_line_chain.h>
#include <layer_ids.h>
#include <pad.h>
#include <pcb_track.h>

/**
 * Lightweight class which holds a pad, via, or a routed trace outline.  Proxied objects passed by pointer are not
 * owned by this container.
 */
class LENGTH_DELAY_CALCULATION_ITEM
{
public:
    /// The type of routing object this item proxies
    enum class TYPE
    {
        UNKNOWN,
        PAD,
        LINE,
        VIA
    };

    /// Whether this item is UNMERGED, it has been merged and should be used (MERGED_IN_USE), or it has been merged
    /// and has been retired from use (MERGED_RETIRED). MERGED_RETIRED essentially means the object has been merged
    /// in to a MERGED_IN_USE item.
    enum class MERGE_STATUS
    {
        UNMERGED,
        MERGED_IN_USE,
        MERGED_RETIRED
    };

    /// Gets the routing item type
    TYPE Type() const { return m_type; };

    /// Sets the parent PAD associated with this item
    void SetPad( const PAD* aPad )
    {
        m_type = TYPE::PAD;
        m_pad = aPad;
    }

    /// Gets the parent PAD associated with this item
    const PAD* GetPad() const { return m_pad; }

    /// Sets the source SHAPE_LINE_CHAIN of this item
    void SetLine( const SHAPE_LINE_CHAIN& aLine )
    {
        m_type = TYPE::LINE;
        m_line = aLine;
    }

    /// Gets the SHAPE_LINE_CHAIN associated with this item
    SHAPE_LINE_CHAIN& GetLine() const { return m_line; }

    /// Sets the VIA associated with this item
    void SetVia( const PCB_VIA* aVia )
    {
        m_type = TYPE::VIA;
        m_via = aVia;
    }

    /// Gets the VIA associated with this item
    const PCB_VIA* GetVia() const { return m_via; }

    /// Sets the first and last layers associated with this item. Always stores in copper layer order
    /// (F_Cu to B_Cu)
    void SetLayers( const PCB_LAYER_ID aStart, const PCB_LAYER_ID aEnd = PCB_LAYER_ID::UNDEFINED_LAYER )
    {
        if( aEnd != UNDEFINED_LAYER && IsCopperLayerLowerThan( aStart, aEnd ) )
        {
            m_layerStart = aEnd;
            m_layerEnd = aStart;
        }
        else
        {
            m_layerStart = aStart;
            m_layerEnd = aEnd;
        }
    }

    /// Sets the MERGE_STATUS of this item. MERGED_RETIRED essentially means the object has been merged
    /// in to a MERGED_IN_USE item.
    void SetMergeStatus( const MERGE_STATUS aStatus ) { m_mergeStatus = aStatus; }

    /// Gets the MERGE_STATUS of this item
    MERGE_STATUS GetMergeStatus() const { return m_mergeStatus; }

    /// Gets the upper and lower layers for the proxied item
    std::tuple<PCB_LAYER_ID, PCB_LAYER_ID> GetLayers() const { return { m_layerStart, m_layerEnd }; }

    /// Gets the start board layer for the proxied item
    PCB_LAYER_ID GetStartLayer() const { return m_layerStart; }

    /// Gets the end board layer for the proxied item.
    PCB_LAYER_ID GetEndLayer() const { return m_layerEnd; }

    /// Calculates active via payers for a proxied VIA object
    void CalculateViaLayers( const BOARD* aBoard );

    /// Sets the effective net class for the item
    void SetEffectiveNetClass( const NETCLASS* aNetClass ) { m_netClass = aNetClass; }

    /// Returns the effective net class for the item
    const NETCLASS* GetEffectiveNetClass() const { return m_netClass; }

protected:
    /// A proxied PAD object. Set to nullptr if not proxying a PAD.
    const PAD* m_pad{ nullptr };

    /// A proxied SHAPE_LINE_CHAIN object. Line is empty if not proxying a SHAPE_LINE_CHAIN.
    mutable SHAPE_LINE_CHAIN m_line;

    /// A proxied PCB_VIA object. Set to nullptr if not proxying a VIA.
    const PCB_VIA* m_via{ nullptr };

    /// The start board layer for the proxied object
    PCB_LAYER_ID m_layerStart{ PCB_LAYER_ID::UNDEFINED_LAYER };

    /// The end board layer for the proxied object
    PCB_LAYER_ID m_layerEnd{ PCB_LAYER_ID::UNDEFINED_LAYER };

    /// Flags whether this item has already been merged with another
    MERGE_STATUS m_mergeStatus{ MERGE_STATUS::UNMERGED };

    /// The routing object type of the proxied parent
    TYPE m_type{ TYPE::UNKNOWN };

    /// The net class of the object
    const NETCLASS* m_netClass{ nullptr };
};

#endif //PCBNEW_LENGTH_DELAY_CALCULATION_ITEM_H

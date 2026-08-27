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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file drill_span.h
 * @brief Hole classification and drill span, shared by the drill model and the drill writers.
 */

#ifndef DRILL_SPAN_H
#define DRILL_SPAN_H

#include <utility>

#include <layer_ids.h>


// hole attribute, mainly to identify vias and pads and add this info as comment
// in NC drill files
enum class HOLE_ATTRIBUTE
{
    HOLE_UNKNOWN,           // uninitialized type
    HOLE_VIA_THROUGH,       // a via hole (always plated) from top to bottom
    HOLE_VIA_BURIED,        // a via hole (always plated) not through hole
    HOLE_VIA_BACKDRILL,     // a via hole created by a backdrill operation
    HOLE_PAD,               // a plated or not plated pad hole
    HOLE_PAD_CASTELLATED,   // a plated castelleted pad hole
    HOLE_PAD_PRESSFIT,      // a plated press-fit pad hole
    HOLE_MECHANICAL         // a mechanical pad (provided, not used)
};


typedef std::pair<PCB_LAYER_ID, PCB_LAYER_ID>   DRILL_LAYER_PAIR;


struct DRILL_SPAN
{
    DRILL_SPAN()
    {
        m_StartLayer = F_Cu;
        m_EndLayer = B_Cu;
        m_IsBackdrill = false;
        m_IsNonPlatedFile = false;
    }

    DRILL_SPAN( PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aEndLayer, bool aIsBackdrill,
                bool aIsNonPlated )
    {
        m_StartLayer = aStartLayer;
        m_EndLayer = aEndLayer;
        m_IsBackdrill = aIsBackdrill;
        m_IsNonPlatedFile = aIsNonPlated;
    }

    PCB_LAYER_ID TopLayer() const
    {
        // B_Cu (id=2) is numerically less than inner layers (id>=4), but is physically
        // at the bottom of the stack. Use IsCopperLayerLowerThan for correct ordering.
        return IsCopperLayerLowerThan( m_StartLayer, m_EndLayer ) ? m_EndLayer : m_StartLayer;
    }

    PCB_LAYER_ID BottomLayer() const
    {
        return IsCopperLayerLowerThan( m_StartLayer, m_EndLayer ) ? m_StartLayer : m_EndLayer;
    }

    PCB_LAYER_ID DrillStartLayer() const
    {
        return m_StartLayer;
    }

    PCB_LAYER_ID DrillEndLayer() const
    {
        return m_EndLayer;
    }

    DRILL_LAYER_PAIR Pair() const
    {
        return DRILL_LAYER_PAIR( TopLayer(), BottomLayer() );
    }

    bool operator==( const DRILL_SPAN& aOther ) const
    {
        // Compares the stored layers, not Top/Bottom, so a span and its reverse stay distinct
        return m_StartLayer == aOther.m_StartLayer && m_EndLayer == aOther.m_EndLayer
               && m_IsBackdrill == aOther.m_IsBackdrill
               && m_IsNonPlatedFile == aOther.m_IsNonPlatedFile;
    }

    bool operator!=( const DRILL_SPAN& aOther ) const { return !( *this == aOther ); }

    bool operator<( const DRILL_SPAN& aOther ) const
    {
        if( TopLayer() != aOther.TopLayer() )
            return TopLayer() < aOther.TopLayer();

        if( BottomLayer() != aOther.BottomLayer() )
            return BottomLayer() < aOther.BottomLayer();

        if( m_IsBackdrill != aOther.m_IsBackdrill )
            return m_IsBackdrill && !aOther.m_IsBackdrill;

        if( m_IsNonPlatedFile != aOther.m_IsNonPlatedFile )
            return m_IsNonPlatedFile && !aOther.m_IsNonPlatedFile;

        if( m_StartLayer != aOther.m_StartLayer )
            return m_StartLayer < aOther.m_StartLayer;

        return m_EndLayer < aOther.m_EndLayer;
    }

    PCB_LAYER_ID m_StartLayer;
    PCB_LAYER_ID m_EndLayer;
    bool         m_IsBackdrill;
    bool         m_IsNonPlatedFile;
};

#endif // DRILL_SPAN_H

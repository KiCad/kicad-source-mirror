/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef __PNS_DIFF_PAIR_H
#define __PNS_DIFF_PAIR_H

#include <vector>

#include "pns_line.h"
#include "pns_via.h"
#include "pns_link_holder.h"

#include "ranged_num.h"

namespace PNS {

class DIFF_PAIR;

/**
 * Define a "gateway" for routing a differential pair - e.g. a pair of points (anchors) with
 * certain orientation, spacing and (optionally) predefined entry paths.  The routing algorithm
 * connects such gateways with parallel lines, thus creating a differential pair.
 */
class DP_GATEWAY
{
public:
    DP_GATEWAY( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN, bool aIsDiagonal,
                int aAllowedEntryAngles = DIRECTION_45::ANG_OBTUSE, int aPriority = 0 ) :
            m_anchorP( aAnchorP ),
            m_anchorN( aAnchorN ), m_isDiagonal( aIsDiagonal ),
            m_allowedEntryAngles( aAllowedEntryAngles ), m_priority( aPriority )
    {
        m_hasEntryLines = false;
    }

    ~DP_GATEWAY()
    {
    }

    /**
     * @return true if the gateway anchors lie on a diagonal line.
     */
    bool IsDiagonal() const
    {
        return m_isDiagonal;
    }

    const VECTOR2I& AnchorP() const { return m_anchorP; }

    const VECTOR2I& AnchorN() const { return m_anchorN; }

    /**
     * @return a mask of 45-degree entry directions allowed for the gateway.
     */
    int AllowedAngles () const { return m_allowedEntryAngles; }

    /**
     * @return priority/score value for gateway matching.
     */
    int Priority() const
    {
        return m_priority;
    }

    void SetPriority(int aPriority)
    {
        m_priority = aPriority;
    }

    void SetEntryLines( const SHAPE_LINE_CHAIN& aEntryP, const SHAPE_LINE_CHAIN& aEntryN )
    {
        m_entryP = aEntryP;
        m_entryN = aEntryN;
        m_hasEntryLines = true;
    }

    const SHAPE_LINE_CHAIN& EntryP() const { return m_entryP; }
    const SHAPE_LINE_CHAIN& EntryN() const { return m_entryN; }
    const DIFF_PAIR Entry() const ;

    void Reverse();

    bool HasEntryLines () const
    {
        return m_hasEntryLines;
    }

private:
    SHAPE_LINE_CHAIN m_entryP, m_entryN;
    bool m_hasEntryLines;
    VECTOR2I m_anchorP, m_anchorN;
    bool m_isDiagonal;
    int m_allowedEntryAngles;
    int m_priority;
};

/**
 * Store starting/ending primitives (pads, vias or segments) for a differential pair.
 */
class DP_PRIMITIVE_PAIR
{
public:
    DP_PRIMITIVE_PAIR():
        m_primP( nullptr ), m_primN( nullptr ) {};

    DP_PRIMITIVE_PAIR( const DP_PRIMITIVE_PAIR& aOther );
    DP_PRIMITIVE_PAIR( ITEM* aPrimP, ITEM* aPrimN );
    DP_PRIMITIVE_PAIR( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );

    ~DP_PRIMITIVE_PAIR();

    void SetAnchors( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );

    const VECTOR2I& AnchorP() const { return m_anchorP; }
    const VECTOR2I& AnchorN() const { return m_anchorN; }

    DP_PRIMITIVE_PAIR& operator=( const DP_PRIMITIVE_PAIR& aOther );

    ITEM* PrimP() const { return m_primP; }
    ITEM* PrimN() const { return m_primN; }

    bool Directional() const;

    DIRECTION_45 DirP() const;
    DIRECTION_45 DirN() const;


    void CursorOrientation( const VECTOR2I& aCursorPos, VECTOR2I& aMidpoint,
                            VECTOR2I& aDirection ) const;

    void dump()
    {
        printf( "-- Prim-P %p anchor [%d, %d]\n", m_primP, m_anchorP.x, m_anchorP.y );
        printf( "-- Prim-N %p anchor [%d, %d]\n", m_primN, m_anchorN.x, m_anchorN.y );
    }

private:
    DIRECTION_45 anchorDirection( const ITEM* aItem, const VECTOR2I& aP ) const;

    ITEM* m_primP;
    ITEM* m_primN;
    VECTOR2I m_anchorP, m_anchorN;
};

/**
 * A set of gateways calculated for the cursor or starting/ending primitive pair.
 */
class DP_GATEWAYS
{
public:
    DP_GATEWAYS( int aGap ):
        m_gap( aGap ),
        m_viaGap( aGap )
    {
        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_viaDiameter = 0;
        m_fitVias = true;
    }

    void Clear() { m_gateways.clear(); }

    void SetFitVias( bool aEnable, int aDiameter = 0, int aViaGap = -1 )
    {
        m_fitVias = aEnable;
        m_viaDiameter = aDiameter;

        if( aViaGap < 0 )
            m_viaGap = m_gap;
        else
            m_viaGap = aViaGap;
    }


    void BuildForCursor( const VECTOR2I& aCursorPos );
    void BuildOrthoProjections( DP_GATEWAYS& aEntries, const VECTOR2I& aCursorPos,
                                int aOrthoScore );
    void BuildGeneric( const VECTOR2I& p0_p, const VECTOR2I& p0_n, bool aBuildEntries = false,
                       bool aViaMode = false );
    void BuildFromPrimitivePair( const DP_PRIMITIVE_PAIR& aPair, bool aPreferDiagonal );

    bool FitGateways( DP_GATEWAYS& aEntry, DP_GATEWAYS& aTarget, bool aPrefDiagonal,
                      DIFF_PAIR& aDp );

    std::vector<DP_GATEWAY>& Gateways() { return m_gateways; }

    const std::vector<DP_GATEWAY>& CGateways() const { return m_gateways; }

    void FilterByOrientation( int aAngleMask, DIRECTION_45 aRefOrientation );

private:
    struct DP_CANDIDATE
    {
        SHAPE_LINE_CHAIN p, n;
        VECTOR2I         gw_p, gw_n;
        int              score;
    };

    bool checkDiagonalAlignment( const VECTOR2I& a, const VECTOR2I& b ) const;
    void buildDpContinuation( const DP_PRIMITIVE_PAIR& aPair, bool aIsDiagonal );
    void buildEntries( const VECTOR2I& p0_p, const VECTOR2I& p0_n );

    int  m_gap;
    int  m_viaGap;
    int  m_viaDiameter;
    bool m_fitVias;

    std::vector<DP_GATEWAY> m_gateways;
};


/**
 * Basic class for a differential pair. Stores two PNS_LINEs (for positive and negative nets,
 * respectively), the gap and coupling constraints.
 */
class DIFF_PAIR : public LINK_HOLDER
{
public:
    struct COUPLED_SEGMENTS
    {
        COUPLED_SEGMENTS ( const SEG& aCoupledP, const SEG& aParentP, int aIndexP,
                           const SEG& aCoupledN, const SEG& aParentN, int aIndexN ) :
            coupledP( aCoupledP ),
            coupledN( aCoupledN ),
            parentP( aParentP ),
            parentN( aParentN ),
            indexP( aIndexP ),
            indexN( aIndexN )
        {}

        SEG coupledP;
        SEG coupledN;
        SEG parentP;
        SEG parentN;
        int indexP;
        int indexN;
    };

    typedef std::vector<COUPLED_SEGMENTS> COUPLED_SEGMENTS_VEC;

    DIFF_PAIR() :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_hasVias( false )
    {
        // Initialize some members, to avoid uninitialized variables.
        m_net_p = nullptr;
        m_net_n = nullptr;
        m_width = 0;
        m_gap = 0;
        m_viaGap = 0;
        m_maxUncoupledLength = 0;
        m_chamferLimit = 0;
    }

    DIFF_PAIR( int aGap ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_hasVias( false )
    {
        m_gapConstraint = aGap;

        // Initialize other members, to avoid uninitialized variables.
        m_net_p = nullptr;
        m_net_n = nullptr;
        m_width = 0;
        m_gap = 0;
        m_viaGap = 0;
        m_maxUncoupledLength = 0;
        m_chamferLimit = 0;
    }

    DIFF_PAIR( const SHAPE_LINE_CHAIN &aP, const SHAPE_LINE_CHAIN& aN, int aGap = 0 ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_n( aN ),
        m_p( aP ),
        m_hasVias( false )
    {
        m_gapConstraint = aGap;

        // Initialize other members, to avoid uninitialized variables.
        m_net_p = nullptr;
        m_net_n = nullptr;
        m_width = 0;
        m_gap = 0;
        m_viaGap = 0;
        m_maxUncoupledLength = 0;
        m_chamferLimit = 0;
    }

    DIFF_PAIR( const LINE &aLineP, const LINE &aLineN, int aGap = 0 ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_line_p( aLineP ),
        m_line_n( aLineN ),
        m_hasVias( false )
    {
        m_gapConstraint = aGap;
        m_net_p = aLineP.Net();
        m_net_n = aLineN.Net();
        m_p = aLineP.CLine();
        m_n = aLineN.CLine();

        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_width  = 0;
        m_gap  = 0;
        m_viaGap  = 0;
        m_maxUncoupledLength  = 0;
        m_chamferLimit  = 0;
    }

    DIFF_PAIR( const DIFF_PAIR& aOther ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T )
    {
        *this = aOther;
    }

    static inline bool ClassOf( const ITEM* aItem )
    {
        return aItem && ITEM::DIFF_PAIR_T == aItem->Kind();
    }

    DIFF_PAIR* Clone() const override
    {
        assert( false );
        return nullptr;
    }

    // Copy operator
    DIFF_PAIR& operator=( const DIFF_PAIR& aOther )
    {
        m_n = aOther.m_n;
        m_p = aOther.m_p;
        m_line_n = aOther.m_line_n;
        m_line_p = aOther.m_line_p;
        m_via_n = aOther.m_via_n;
        m_via_p = aOther.m_via_p;

        m_hasVias = aOther.m_hasVias;
        m_net_n = aOther.m_net_n;
        m_net_p = aOther.m_net_p;
        m_width = aOther.m_width;
        m_gap = aOther.m_gap;
        m_viaGap = aOther.m_viaGap;
        m_maxUncoupledLength = aOther.m_maxUncoupledLength;
        m_chamferLimit = aOther.m_chamferLimit;
        m_gapConstraint = aOther.m_gapConstraint;
        return *this;
    }

    // Move assignment operator
    DIFF_PAIR& operator=( DIFF_PAIR&& aOther ) noexcept
    {
        if (this != &aOther)
        {
            m_n = std::move( aOther.m_n );
            m_p = std::move( aOther.m_p );
            m_line_n = std::move( aOther.m_line_n );
            m_line_p = std::move( aOther.m_line_p );
            m_via_n = aOther.m_via_n;
            m_via_p = aOther.m_via_p;

            m_hasVias = aOther.m_hasVias;
            m_net_n = aOther.m_net_n;
            m_net_p = aOther.m_net_p;
            m_width = aOther.m_width;
            m_gap = aOther.m_gap;
            m_viaGap = aOther.m_viaGap;
            m_maxUncoupledLength = aOther.m_maxUncoupledLength;
            m_chamferLimit = aOther.m_chamferLimit;
            m_gapConstraint = aOther.m_gapConstraint;
        }

        return *this;
    }

    virtual void ClearLinks() override
    {
        m_links.clear();
        m_line_p.ClearLinks();
        m_line_n.ClearLinks();
    }

    void SetShape( const SHAPE_LINE_CHAIN &aP, const SHAPE_LINE_CHAIN& aN, bool aSwapLanes = false )
    {
        if( aSwapLanes )
        {
            m_p = aN;
            m_n = aP;
        }
        else
        {
            m_p = aP;
            m_n = aN;
        }
    }

    void SetShape( const DIFF_PAIR& aPair )
    {
        m_p = aPair.m_p;
        m_n = aPair.m_n;
    }

    void SetNets( NET_HANDLE aP, NET_HANDLE aN )
    {
        m_net_p = aP;
        m_net_n = aN;
    }

    void SetWidth( int aWidth )
    {
        m_width = aWidth;
        m_n.SetWidth( aWidth );
        m_p.SetWidth( aWidth );
    }

    int Width() const { return m_width; }

    void SetGap( int aGap )
    {
        m_gap = aGap;
        m_gapConstraint = RANGED_NUM<int>( m_gap, 10000, 10000 );
    }

    int Gap() const
    {
        return m_gap;
    }

    void AppendVias( const VIA &aViaP, const VIA& aViaN )
    {
        m_hasVias = true;
        m_via_p = aViaP;
        m_via_p.SetHole( aViaP.Hole()->Clone() );
        m_via_n = aViaN;
        m_via_n.SetHole( aViaN.Hole()->Clone() );
    }

    void RemoveVias()
    {
        m_hasVias = false;
        m_line_n.RemoveVia();
        m_line_p.RemoveVia();
    }

    bool EndsWithVias() const
    {
        return m_hasVias;
    }

    void SetViaDiameter( int aDiameter )
    {
        m_via_p.SetDiameter( VIA::ALL_LAYERS, aDiameter );
        m_via_n.SetDiameter( VIA::ALL_LAYERS, aDiameter );
    }

    void SetViaDrill( int aDrill )
    {
        m_via_p.SetDrill( aDrill );
        m_via_n.SetDrill( aDrill );
    }

    NET_HANDLE NetP() const
    {
        return m_net_p;
    }

    NET_HANDLE NetN() const
    {
        return m_net_n;
    }

    LINE& PLine()
    {
        if( !m_line_p.IsLinked() )
            updateLine( m_line_p, m_p, m_net_p, m_via_p );

        return m_line_p;
    }

    LINE& NLine()
    {
        if( !m_line_n.IsLinked() )
            updateLine( m_line_n, m_n, m_net_n, m_via_n );

        return m_line_n;
    }

    DP_PRIMITIVE_PAIR EndingPrimitives();

    double CoupledLength() const;
    double TotalLength() const;
    double CoupledLengthFactor() const;
    double Skew() const;

    void CoupledSegmentPairs( COUPLED_SEGMENTS_VEC& aPairs ) const;

    void Clear()
    {
        m_n.Clear();
        m_p.Clear();
    }

    void Append( const DIFF_PAIR& aOther )
    {
        m_n.Append( aOther.m_n );
        m_p.Append( aOther.m_p );
    }

    bool Empty() const
    {
        return ( m_n.SegmentCount() == 0 ) || ( m_p.SegmentCount() == 0 );
    }

    const SHAPE_LINE_CHAIN& CP() const { return m_p; }
    const SHAPE_LINE_CHAIN& CN() const { return m_n; }

    bool BuildInitial( const DP_GATEWAY& aEntry, const DP_GATEWAY& aTarget, bool aPrefDiagonal );
    bool CheckConnectionAngle( const DIFF_PAIR &aOther, int allowedAngles ) const;
    int CoupledLength( const SEG& aP, const SEG& aN ) const;

    int64_t CoupledLength( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const;

    const RANGED_NUM<int> GapConstraint() const
    {
        return m_gapConstraint;
    }

private:
    void updateLine( LINE &aLine, const SHAPE_LINE_CHAIN& aShape, NET_HANDLE aNet, const VIA& aVia )
    {
        aLine.SetShape( aShape );
        aLine.SetWidth( m_width );
        aLine.SetNet( aNet );
        aLine.SetLayer( Layers().Start() );
        aLine.SetParent( m_parent );
        aLine.SetSourceItem( m_sourceItem );

        if( m_hasVias )
            aLine.AppendVia( aVia );
    }

    SHAPE_LINE_CHAIN m_n, m_p;
    LINE m_line_p, m_line_n;
    VIA m_via_p, m_via_n;

    bool m_hasVias;
    NET_HANDLE m_net_p, m_net_n;
    int m_width;
    int m_gap;
    int m_viaGap;
    int m_maxUncoupledLength;
    int m_chamferLimit;
    RANGED_NUM<int> m_gapConstraint;
};

}

#endif

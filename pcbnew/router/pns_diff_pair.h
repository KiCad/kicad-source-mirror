/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015 CERN
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

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_line.h"
#include "pns_via.h"

#include "ranged_num.h"

class PNS_DIFF_PAIR;

/**
 * Class PNS_DP_GATEWAY
 *
 * Defines a "gateway" for routing a differential pair - e.g. a pair of points (anchors) with certain
 * orientation, spacing and (optionally) predefined entry paths. The routing algorithm connects such
 * gateways with parallel lines, thus creating a difrerential pair.
 **/
class PNS_DP_GATEWAY {
public:
    PNS_DP_GATEWAY ( const VECTOR2I& aAnchorP, 
                     const VECTOR2I& aAnchorN, 
                     bool aIsDiagonal, 
                     int aAllowedEntryAngles = DIRECTION_45::ANG_OBTUSE, 
                     int aPriority = 0 ) 
        : m_anchorP(aAnchorP), 
          m_anchorN (aAnchorN), 
          m_isDiagonal( aIsDiagonal ), 
          m_allowedEntryAngles (aAllowedEntryAngles), 
          m_priority(aPriority)
    {
        m_hasEntryLines = false;
    }

    ~PNS_DP_GATEWAY ()
    {

    }

    /**
     * Function IsDiagonal()
     *
     * @return true, if the gateway anchors lie on a diagonal line
     */
    
    bool IsDiagonal() const
    {
        return m_isDiagonal; 
    }

    const VECTOR2I& AnchorP () const { return m_anchorP; }
    const VECTOR2I& AnchorN () const { return m_anchorN; }
    
    /**
     * Function AllowedAngles()
     *
     * @return a mask of 45-degree entry directoins allowed for the
     * gateway.
    */
    int AllowedAngles () const { return m_allowedEntryAngles; }

    /**
     * Function Priority()
     *
     * @return priority/score value for gateway matching
     */
    int Priority() const 
    { 
        return m_priority;
    }
    
    void SetPriority(int aPriority) 
    {
        m_priority = aPriority;
    }

    void SetEntryLines ( const SHAPE_LINE_CHAIN& aEntryP, const SHAPE_LINE_CHAIN& aEntryN )
    { 
        m_entryP = aEntryP;
        m_entryN = aEntryN;
        m_hasEntryLines = true;
    }

    const SHAPE_LINE_CHAIN& EntryP () const { return m_entryP; } 
    const SHAPE_LINE_CHAIN& EntryN () const { return m_entryN; } 
    const PNS_DIFF_PAIR Entry() const ;
    
    void Reverse();
    
    bool HasEntryLines () const 
    {
        return  m_hasEntryLines;
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
 * Class PNS_DP_PRIMITIVE_PAIR
 *
 * Stores staring/ending primitives (pads, vias or segments) for a differential pair.
 **/
class PNS_DP_PRIMITIVE_PAIR
{

public:
    PNS_DP_PRIMITIVE_PAIR():
        m_primP (NULL), m_primN ( NULL ) {};

    PNS_DP_PRIMITIVE_PAIR ( const PNS_DP_PRIMITIVE_PAIR& aOther );
    PNS_DP_PRIMITIVE_PAIR ( PNS_ITEM *aPrimP, PNS_ITEM *aPrimN );
    PNS_DP_PRIMITIVE_PAIR ( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );

    ~PNS_DP_PRIMITIVE_PAIR();

    void SetAnchors ( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );

    const VECTOR2I& AnchorP () const { return m_anchorP; }
    const VECTOR2I& AnchorN () const { return m_anchorN; }

    PNS_DP_PRIMITIVE_PAIR& operator= ( const PNS_DP_PRIMITIVE_PAIR& aOther );

    PNS_ITEM* PrimP () const { return m_primP; }
    PNS_ITEM* PrimN () const { return m_primN; }

    bool Directional() const;

    DIRECTION_45 DirP () const;
    DIRECTION_45 DirN () const;

private:

    DIRECTION_45 anchorDirection ( PNS_ITEM *aItem, const VECTOR2I& aP) const; 

    PNS_ITEM *m_primP, *m_primN;
    VECTOR2I m_anchorP, m_anchorN;
};

/**
 * Class PNS_GATEWAYS
 *
 * A set of gateways calculated for the cursor or starting/ending primitive pair.
 **/

class PNS_DP_GATEWAYS 
{

    public:
        PNS_DP_GATEWAYS ( int aGap ):
            m_gap(aGap), m_viaGap( aGap ) {};

        void SetGap ( int aGap ) { 
            m_gap = aGap; 
            m_viaGap = aGap;
        }
  
        void Clear() 
        { 
            m_gateways.clear(); 
        }

        void SetFitVias ( bool aEnable, int aDiameter = 0, int aViaGap = -1 )
        {
            m_fitVias = aEnable;
            m_viaDiameter = aDiameter;
            if(aViaGap < 0)
                m_viaGap = m_gap;
            else
                m_viaGap = aViaGap;
        }

        
        void BuildForCursor ( const VECTOR2I& aCursorPos );
        void BuildOrthoProjections ( PNS_DP_GATEWAYS &aEntries, const VECTOR2I& aCursorPos, int aOrthoScore );
        void BuildGeneric ( const VECTOR2I& p0_p, const VECTOR2I& p0_n, bool aBuildEntries = false, bool aViaMode = false );
        void BuildFromPrimitivePair( PNS_DP_PRIMITIVE_PAIR aPair, bool aPreferDiagonal );
        
        bool FitGateways (  PNS_DP_GATEWAYS& aEntry,  PNS_DP_GATEWAYS& aTarget, bool aPrefDiagonal, PNS_DIFF_PAIR& aDp );

        std::vector<PNS_DP_GATEWAY>& Gateways()
        {
            return m_gateways;
        }

    private:

        struct DP_CANDIDATE 
        {
            SHAPE_LINE_CHAIN p, n;
            VECTOR2I gw_p, gw_n;
            int score;
        };

        bool checkDiagonalAlignment ( const VECTOR2I& a, const VECTOR2I& b) const;
        void buildDpContinuation ( PNS_DP_PRIMITIVE_PAIR aPair, bool aIsDiagonal );
        void buildEntries ( const VECTOR2I& p0_p, const VECTOR2I& p0_n );

        int m_gap;
        int m_viaGap;
        int m_viaDiameter;
        bool m_fitVias;

        std::vector<PNS_DP_GATEWAY> m_gateways;
};


/**
 * Class PNS_DIFF_PAIR
 *
 * Basic class for a differential pair. Stores two PNS_LINEs (for positive and negative nets, respectively),
 * the gap and coupling constraints.
 **/
class PNS_DIFF_PAIR : public PNS_ITEM {

public:
    struct COUPLED_SEGMENTS {
        COUPLED_SEGMENTS ( const SEG& aCoupledP, const SEG& aParentP, int aIndexP, 
                           const SEG& aCoupledN, const SEG& aParentN, int aIndexN ) :
            coupledP ( aCoupledP ),
            coupledN ( aCoupledN ),
            parentP ( aParentP ),
            parentN ( aParentN ),
            indexP ( aIndexP ),
            indexN ( aIndexN )
        {}
            
        SEG coupledP;
        SEG coupledN;
        SEG parentP;
        SEG parentN;
        int indexP;
        int indexN;
    };

    typedef std::vector<COUPLED_SEGMENTS> COUPLED_SEGMENTS_VEC;

	PNS_DIFF_PAIR ( ) : PNS_ITEM ( DIFF_PAIR ), m_hasVias (false) {}

    PNS_DIFF_PAIR ( int aGap ) : 
        PNS_ITEM ( DIFF_PAIR ), 
        m_hasVias (false)
    {
        m_gapConstraint = aGap;
    }

    PNS_DIFF_PAIR ( const SHAPE_LINE_CHAIN &aP, const SHAPE_LINE_CHAIN& aN, int aGap = 0 ):
        PNS_ITEM ( DIFF_PAIR ), 
        m_n (aN),
        m_p (aP),
        m_hasVias (false)
    {
        m_gapConstraint = aGap;
    }

    PNS_DIFF_PAIR ( const PNS_LINE &aLineP, const PNS_LINE &aLineN, int aGap = 0 ):
        PNS_ITEM ( DIFF_PAIR ), 
        m_line_p ( aLineP ),
        m_line_n ( aLineN ),
        m_hasVias (false)
    {
        m_gapConstraint = aGap;
        m_net_p = aLineP.Net();
        m_net_n = aLineN.Net();
        m_p = aLineP.CLine();
        m_n = aLineN.CLine();
    }

    static inline bool ClassOf( const PNS_ITEM* aItem )
    {
        return aItem && DIFF_PAIR == aItem->Kind();
    }
    
	PNS_DIFF_PAIR * Clone() const { assert(false); return NULL; }

    static PNS_DIFF_PAIR* AssembleDp ( PNS_LINE *aLine );

    void SetShape ( const SHAPE_LINE_CHAIN &aP, const SHAPE_LINE_CHAIN& aN, bool aSwapLanes = false)
    {
        if (aSwapLanes)
        {
            m_p = aN;
            m_n = aP;
        } else {
            m_p = aP;
            m_n = aN;
        }
    }
    
    void SetShape ( const PNS_DIFF_PAIR& aPair )
    {
        m_p = aPair.m_p;
        m_n = aPair.m_n;
    }

    void SetNets ( int aP, int aN )
    {
    	m_net_p = aP;
    	m_net_n = aN;
    }
    
    void SetWidth ( int aWidth )
    {
    	m_width = aWidth;
    }

    int Width()  const { return m_width; }
    
    void SetGap ( int aGap)
    {
    	m_gap = aGap;
        m_gapConstraint = RANGED_NUM<int> ( m_gap, 10000, 10000 );
    }

    int Gap() const {
        return m_gap;
    }

    void AppendVias ( const PNS_VIA &aViaP, const PNS_VIA& aViaN )
    {
        m_hasVias = true;
        m_via_p = aViaP;
        m_via_n = aViaN;
    }

    void RemoveVias ()
    {
        m_hasVias = false;
    }

    bool EndsWithVias() const
    {
        return m_hasVias;
    }

    int NetP() const 
    {
        return m_net_p;
    }

    int NetN() const 
    {
        return m_net_n;
    }

    PNS_LINE& PLine() 
    {
        if ( !m_line_p.IsLinked ( ) )
            updateLine(m_line_p, m_p, m_net_p, m_via_p );
        return m_line_p;
	}

    PNS_LINE& NLine() 
    {
        if ( !m_line_n.IsLinked ( ) )
            updateLine(m_line_n, m_n, m_net_n, m_via_n );
        return m_line_n;
    }

    PNS_DP_PRIMITIVE_PAIR EndingPrimitives();

    double CoupledLength() const;
    double TotalLength() const;
    double CoupledLengthFactor () const;
    double Skew () const;

    void CoupledSegmentPairs ( COUPLED_SEGMENTS_VEC& aPairs ) const;

    void Clear()
    {
        m_n.Clear();
        m_p.Clear();
    }

    void Append (const PNS_DIFF_PAIR& aOther )
    {
        m_n.Append ( aOther.m_n );
        m_p.Append ( aOther.m_p );
    }

    bool Empty() const
    {
        return (m_n.SegmentCount() == 0) || (m_p.SegmentCount() == 0);
    }
    const SHAPE_LINE_CHAIN& CP() const { return m_p; }
    const SHAPE_LINE_CHAIN& CN() const { return m_n; }
    
    bool BuildInitial ( PNS_DP_GATEWAY& aEntry, PNS_DP_GATEWAY& aTarget, bool aPrefDiagonal );
    bool CheckConnectionAngle ( const PNS_DIFF_PAIR &aOther, int allowedAngles ) const;
    int CoupledLength ( const SEG& aP, const SEG& aN ) const;

    int64_t CoupledLength ( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const;

    const RANGED_NUM<int> GapConstraint() const {
        return m_gapConstraint;
    }

private:

    void updateLine( PNS_LINE &aLine, const SHAPE_LINE_CHAIN& aShape, int aNet, PNS_VIA& aVia ) 
    {
        aLine.SetShape( aShape );
        aLine.SetWidth( m_width );
        aLine.SetNet(aNet);
        aLine.SetLayer (Layers().Start());

        if(m_hasVias)
            aLine.AppendVia ( aVia );
    }

    SHAPE_LINE_CHAIN m_n, m_p;
    PNS_LINE m_line_p, m_line_n;
    PNS_VIA m_via_p, m_via_n;

    bool m_hasVias;
    int m_net_p, m_net_n;
    int m_width;
    int m_gap;
    int m_viaGap;
    int m_maxUncoupledLength;
    int m_chamferLimit;
    RANGED_NUM<int> m_gapConstraint;
};


#endif

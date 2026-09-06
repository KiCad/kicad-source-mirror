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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#ifndef __PNS_DIFF_PAIR_H
#define __PNS_DIFF_PAIR_H

#include <core/minoptmax.h>
#include <vector>

#include "pns_line.h"
#include "pns_via.h"
#include "pns_link_holder.h"

#include "ranged_num.h"

namespace PNS {

class DIFF_PAIR;
class DP_PRIMITIVE_PAIR;

typedef MINOPTMAX<int> DP_GAP_CONSTRAINT;

/* Diff pair dimensions on a single layer */
class DP_DIMENSIONS
{
public:
    DP_DIMENSIONS( int aWidth = 0, int aGap = 0, int aViaGap = 0, int aViaDiameter = 0, int aMinClearance = 0 ) :
            m_width( aWidth ),
            m_gap( aGap ),
            m_viaGap( aViaGap ),
            m_viaDiameter( aViaDiameter ),
            m_minClearance( aMinClearance )
    {
    }

    int Width() const { return m_width; }
    int Gap() const { return m_gap; }
    int ViaGap() const { return m_viaGap; }
    int ViaDiameter() const { return m_viaDiameter; }
    int MinClearance() const { return m_minClearance; }

    void SetGap( int aGap ) { m_gap = aGap; }

    void SetGapConstraint( const DP_GAP_CONSTRAINT& aGapConstraint ) { m_gapConstraint = aGapConstraint; }

    void SetMinClearance( int aClearance ) { m_minClearance = aClearance; }

    const DP_GAP_CONSTRAINT& GapConstraint() const { return m_gapConstraint; }

    const wxString Format() const;

private:
    int               m_width;
    int               m_gap;
    int               m_viaGap;
    int               m_viaDiameter;
    int               m_minClearance;
    DP_GAP_CONSTRAINT m_gapConstraint;
};

/**
 * Define a "gateway" for routing a differential pair - e.g. a pair of points (anchors) with
 * certain orientation, spacing and (optionally) predefined entry paths.  The routing algorithm
 * connects such gateways with parallel lines, thus creating a differential pair.
 */
class DP_GATEWAY
{
public:
    DP_GATEWAY( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN, bool aIsDiagonal,
                int aAllowedEntryAngles = DIRECTION_45::ANG_OBTUSE, int aPriority = 0, int aDirectionMask = 0,
                const wxString aName = wxT( "" ) ) :
            m_anchorP( aAnchorP ),
            m_anchorN( aAnchorN ),
            m_isDiagonal( aIsDiagonal ),
            m_allowedEntryAngles( aAllowedEntryAngles ),
            m_priority( aPriority ),
            m_directionMask( aDirectionMask ),
            m_name( aName )
    {
        m_hasEntryLines = false;
    }

    DP_GATEWAY()
    {
        
    }

    ~DP_GATEWAY()
    {
    }

    void SetDimensions( const DP_DIMENSIONS& aDims ) {  m_dims = aDims; }

    void SetPrimaryDirection( DIRECTION_45 aPrimDir )
    {
        m_directionMask = aPrimDir.Mask();
    }    

    void AddPrimaryDirection( DIRECTION_45 aPrimDir )
    {
        m_directionMask |= aPrimDir.Mask();
    }

    bool HasPrimaryDirection() const { return m_directionMask != 0; }

    int PrimaryDirectionMask() const
    {
        return m_directionMask;
    }

    const DP_DIMENSIONS& Dimensions() const { return m_dims; }
    void SetDirections( DIRECTION_45 dP, DIRECTION_45 dN ) { m_dirP = dP; m_dirN = dN; }

    /**
     * @return true if the gateway anchors lie on a diagonal line.
     */
    bool IsDiagonal() const
    {
        return m_isDiagonal;
    }

    void SetName( const wxString& aName) { m_name = aName; }
    const wxString GetName() const;

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

    void SetAnchors( const VECTOR2I& aP, const VECTOR2I& aN )
    {
        m_anchorP = aP;
        m_anchorN = aN;
    }

    const SHAPE_LINE_CHAIN& EntryP() const { return m_entryP; }
    const SHAPE_LINE_CHAIN& EntryN() const { return m_entryN; }
    const DIFF_PAIR Entry() const ;

    void Reverse();

    bool HasEntryLines () const
    {
        return m_hasEntryLines;
    }

    std::optional<DP_GATEWAY> Extend( int aLength );
    std::optional<DP_GATEWAY> AddTurns( bool aSide, bool a90Deg, bool aLeft, bool aWiggle );

    DIRECTION_45 DirP() const { return m_dirP; };
    DIRECTION_45 DirN() const { return m_dirN; };

private:
    DP_DIMENSIONS    m_dims;
    DIRECTION_45     m_dirP, m_dirN;
    SHAPE_LINE_CHAIN m_entryP, m_entryN;
    bool             m_hasEntryLines;
    VECTOR2I         m_anchorP, m_anchorN;
    bool             m_isDiagonal;
    int              m_allowedEntryAngles;
    int              m_priority;
    int              m_directionMask;
    wxString         m_name;
};

/**
 * Store starting/ending primitives (pads, vias or segments) for a differential pair.
 */
class DP_PRIMITIVE_PAIR : public ITEM_OWNER
{
public:
    static constexpr double DP_ASSUME_PRIMS_COLINEAR_FACTOR = 0.1;

    DP_PRIMITIVE_PAIR():
        m_primP( nullptr ), m_primN( nullptr ) {};

    DP_PRIMITIVE_PAIR( const DP_PRIMITIVE_PAIR& aOther );
    DP_PRIMITIVE_PAIR( ITEM* aPrimP, ITEM* aPrimN );
    DP_PRIMITIVE_PAIR( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );

    ~DP_PRIMITIVE_PAIR();

    void SetAnchors( const VECTOR2I& aAnchorP, const VECTOR2I& aAnchorN );
    void SetPrimitives(ITEM* aPrimP, ITEM* aPrimN );
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

    void SetIsMidtrace( bool aMidtrace )
    {
        m_isMidtrace = aMidtrace;
    }

    bool IsMidtrace() const
    {
        return m_isMidtrace;
    }

    void Unlink()
    {
        m_primP = m_primN = nullptr;
    }

    void            SetName( const wxString& aName ) { m_name = aName; }
    const wxString& GetName() const { return m_name; }

    bool HasDefinedGap() const { return m_gap.has_value(); }
    int  GetGap() const { return *m_gap; }
    void SetGap( int aGap ) { m_gap = aGap; }

    void SetFixedDirection( DIRECTION_45 aDir )
    {
        m_fixedDirection = aDir;
    }

    bool HasFixedDirection() const {
        return m_fixedDirection.has_value();
    }

    DIRECTION_45 FixedDirection() const 
    {
        return m_fixedDirection.value();
    }

    // returns the minimum dimension of the parent objects (be that width/height, thickness, radius)
    int GetMinDimension() const;

private:
    DIRECTION_45 anchorDirection( const ITEM* aItem, const VECTOR2I& aP ) const;

    ITEM*                       m_primP;
    ITEM*                       m_primN;
    VECTOR2I                    m_anchorP, m_anchorN;
    bool                        m_isMidtrace;
    std::optional<DIRECTION_45> m_fixedDirection;
    wxString                    m_name;
    std::optional<int>          m_gap;
};

/**
 * A set of gateways calculated for the cursor or starting/ending primitive pair.
 */
class DP_GATEWAYS
{
public:
    DP_GATEWAYS( const DP_DIMENSIONS& aDims = DP_DIMENSIONS() ):
        m_dims( aDims )
    {
        // Do not leave uninitialized members, and keep static analyzer quiet:
        m_fitVias = false;
    }

    void Clear() { m_gateways.clear(); }

    void SetFitVias( bool aEnable )
    {
        m_fitVias = aEnable;
    }

    bool FittingVias() const {
        return m_fitVias;
    }

    void BuildForCursor( const VECTOR2I& aCursorPos, int aDirectionMask = -1 );
    void BuildOrthoProjections( DP_GATEWAYS& aEntries, const VECTOR2I& aCursorPos, int aOrthoScore );
    void BuildGeneric( const VECTOR2I& p0_p, const VECTOR2I& p0_n, int aColinearityThreshold = 0,
                       bool aBuildEntries = false, bool aViaMode = false );
    void BuildFromPrimitivePair( const DP_PRIMITIVE_PAIR& aPair, bool aPreferDiagonal );

    struct FIT_RESULT
    {
        SHAPE_LINE_CHAIN p, n;
        DP_GATEWAY       entry, target;
        float            aspectRatio;
        float            coupledRatio;
        bool             isDiagonal;
        int              score;
        bool             diagonal;
        bool             entryAngleOK;
        bool             targetAngleOK;
    };

    std::vector<FIT_RESULT> FitGateways( DP_GATEWAYS& aEntry, DP_GATEWAYS& aTarget, bool aFitVias );

    std::vector<DP_GATEWAY>& Gateways() { return m_gateways; }

    const std::vector<DP_GATEWAY>& CGateways() const { return m_gateways; }

    void FilterByOrientation( int aDirectionMask );

    void SetDimensions( const DP_DIMENSIONS& aDims )
    {
        m_dims = aDims;
        for( auto& gw : m_gateways )
            gw.SetDimensions( aDims );
    }

private:

    void addGateway( DP_GATEWAY& aGw, const wxString&name = wxT(""), bool aAddTurns = false );

    struct DP_CANDIDATE
    {
        SHAPE_LINE_CHAIN p, n;
        VECTOR2I         gw_p, gw_n;
        int              score;
    };

    bool checkDiagonalAlignment( const VECTOR2I& a, const VECTOR2I& b ) const;
    void buildDpContinuation( const DP_PRIMITIVE_PAIR& aPair, bool aIsDiagonal );
    
    void buildEntries( DP_GATEWAY& aGw, const VECTOR2I& p0_p, const VECTOR2I& p0_n );
    void buildFromPads( const DP_PRIMITIVE_PAIR& aPair );

    DP_DIMENSIONS m_dims;
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

    static constexpr int DP_PARALLELITY_THRESHOLD = 10;

    struct COUPLED_SEGMENTS
    {
        COUPLED_SEGMENTS ( const SEG& aCoupledP, const SEG& aParentP, int aIndexP,
                           const SEG& aCoupledN, const SEG& aParentN, int aIndexN ) :
            coupledP( aCoupledP ),
            coupledN( aCoupledN ),
            parentP( aParentP ),
            parentN( aParentN ),
            indexP( aIndexP ),
            indexN( aIndexN ),
            linkP( nullptr ),
            linkN( nullptr )
        {}

        SEG coupledP;
        SEG coupledN;
        SEG parentP;
        SEG parentN;
        int indexP;
        int indexN;
        ITEM *linkP;
        ITEM *linkN;
    };

    typedef std::vector<COUPLED_SEGMENTS> COUPLED_SEGMENTS_VEC;

    DIFF_PAIR( const DP_DIMENSIONS& aDims = DP_DIMENSIONS()  ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_hasVias( false )
    {
        // Initialize other members, to avoid uninitialized variables.
        m_net_p = nullptr;
        m_net_n = nullptr;
        m_maxUncoupledLength = 0;
        m_chamferLimit = 0;
        m_dims = aDims;
    }

    DIFF_PAIR( const SHAPE_LINE_CHAIN &aP, const SHAPE_LINE_CHAIN& aN, const DP_DIMENSIONS& aDims = DP_DIMENSIONS() ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_n( aN ),
        m_p( aP ),
        m_dims( aDims ),
        m_hasVias( false )
    {
        // Initialize other members, to avoid uninitialized variables.
        m_net_p = nullptr;
        m_net_n = nullptr;
        m_maxUncoupledLength = 0;
        m_chamferLimit = 0;
    }

    DIFF_PAIR( const LINE &aLineP, const LINE &aLineN, const DP_DIMENSIONS& aDims = DP_DIMENSIONS() ) :
        LINK_HOLDER( ITEM::DIFF_PAIR_T ),
        m_line_p( aLineP ),
        m_line_n( aLineN ),
        m_dims( aDims ),
        m_hasVias( false )
    {
        m_net_p = aLineP.Net();
        m_net_n = aLineN.Net();
        m_p = aLineP.CLine();
        m_n = aLineN.CLine();

        // Do not leave uninitialized members, and keep static analyzer quiet:
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
        m_layers = aOther.m_layers;
        m_hasVias = aOther.m_hasVias;
        m_net_n = aOther.m_net_n;
        m_net_p = aOther.m_net_p;
        m_dims = aOther.m_dims;
        m_maxUncoupledLength = aOther.m_maxUncoupledLength;
        m_chamferLimit = aOther.m_chamferLimit;
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
            m_layers = aOther.m_layers;
            m_hasVias = aOther.m_hasVias;
            m_net_n = aOther.m_net_n;
            m_net_p = aOther.m_net_p;
            m_dims = aOther.m_dims;
            m_maxUncoupledLength = aOther.m_maxUncoupledLength;
            m_chamferLimit = aOther.m_chamferLimit;
        }

        return *this;
    }
    
    void SetDimensions( const DP_DIMENSIONS& aDims ) {  m_dims = aDims; }
    
    void SetGap( int aGap )
    {
        m_dims.SetGap( aGap );
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
    double Skew() const;

    
    void CoupledSegmentPairs( COUPLED_SEGMENTS_VEC& aPairs, 
        bool aUseGapConstraint = true,
        const std::optional<DP_GAP_CONSTRAINT>& aOverrideGapConstraint = std::optional<DP_GAP_CONSTRAINT>() ) const;

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

    bool BuildInitial( const DP_GATEWAY& aEntry, const DP_GATEWAY& aTarget, bool aPrefDiagonal, bool aFitVias, float& aBestCouplingRatio, float& aAspectRatio );
    bool CheckConnectionAngle( const DIFF_PAIR &aOther, int allowedAngles ) const;
    int CoupledLength( const SEG& aP, const SEG& aN ) const;

    std::pair<int64_t, bool> CoupledLength( const SHAPE_LINE_CHAIN& aP, const SHAPE_LINE_CHAIN& aN ) const;

    const DP_GAP_CONSTRAINT GapConstraint() const
    {
        return m_dims.GapConstraint();
    }

    void SetLines( const LINE& aP, const LINE& aN )
    {
        m_line_p = aP;
        m_line_n = aN;
    }

    std::optional<DP_PRIMITIVE_PAIR> BuildMidpairIntersection( PNS::SEGMENT* aStartSeg, const VECTOR2I& aP );

    int GuessMostLikelyGap() const;
    const DP_DIMENSIONS& Dimensions() const { return m_dims; }

    DIRECTION_45 DirP( bool aEnd ) const { return getDirection( true, aEnd ); }
    DIRECTION_45 DirN( bool aEnd ) const { return getDirection( false, aEnd ); }

private:

    DIRECTION_45 getDirection( bool aIsP, bool aEnd ) const;

    void updateLine( LINE &aLine, const SHAPE_LINE_CHAIN& aShape, NET_HANDLE aNet, const VIA& aVia )
    {
        aLine.SetShape( aShape );
        aLine.SetWidth( m_dims.Width() );
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

    DP_DIMENSIONS m_dims;
    bool m_hasVias;
    NET_HANDLE m_net_p, m_net_n;
    int m_maxUncoupledLength;
    int m_chamferLimit;
};

}

#endif

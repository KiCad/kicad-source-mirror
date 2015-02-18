/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include <base_units.h> // God forgive me doing this...
#include <colors.h>

#include "trace.h"

#include "pns_node.h"
#include "pns_itemset.h"
#include "pns_topology.h"
#include "pns_dp_meander_placer.h"
#include "pns_diff_pair.h"
#include "pns_router.h"
#include "pns_utils.h"


using boost::optional;



PNS_DP_MEANDER_PLACER::PNS_DP_MEANDER_PLACER( PNS_ROUTER* aRouter ) :
    PNS_MEANDER_PLACER_BASE ( aRouter )
{
    m_world = NULL;
    m_currentNode = NULL;
}


PNS_DP_MEANDER_PLACER::~PNS_DP_MEANDER_PLACER()
{
 
}

const PNS_LINE PNS_DP_MEANDER_PLACER::Trace() const
{
    return m_currentTraceP;
}

PNS_NODE* PNS_DP_MEANDER_PLACER::CurrentNode( bool aLoopsRemoved ) const
{
    if(!m_currentNode)
        return m_world;
    return m_currentNode;
}

bool PNS_DP_MEANDER_PLACER::Start( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
    VECTOR2I p;
    if(!aStartItem || !aStartItem->OfKind ( PNS_ITEM::SEGMENT ))
    {
        Router()->SetFailureReason( _("Please select a track whose length you want to tune.") );
        return false;
    }

    m_initialSegment = static_cast<PNS_SEGMENT *>(aStartItem);

    p = m_initialSegment->Seg().NearestPoint( aP );

    m_currentNode=NULL;
    m_currentStart  = p;

    m_world = Router()->GetWorld()->Branch();

    PNS_TOPOLOGY topo ( m_world );

    if( !topo.AssembleDiffPair ( m_initialSegment, m_originPair ) )
    {
        Router()->SetFailureReason( _( "Unable to find complementary differential pair "
                                       "net for length tuning. Make sure the names of the nets belonging "
                                       "to a differential pair end with either _N/_P or +/-." ) );
        return false;
    }


    m_originPair.SetGap ( Router()->Sizes().DiffPairGap() );

    if( !m_originPair.PLine().SegmentCount() ||
        !m_originPair.NLine().SegmentCount() )
        return false;
    
    m_tunedPathP = topo.AssembleTrivialPath ( m_originPair.PLine().GetLink(0) );
    m_tunedPathN = topo.AssembleTrivialPath ( m_originPair.NLine().GetLink(0) );

    m_world->Remove ( m_originPair.PLine() );
    m_world->Remove ( m_originPair.NLine() );

    m_currentWidth = m_originPair.Width();
    
    return true;
}


void PNS_DP_MEANDER_PLACER::release()
{
    #if 0
    BOOST_FOREACH(PNS_MEANDER *m, m_meanders)
    {
        delete m;
    }

    m_meanders.clear();
    #endif
}

int PNS_DP_MEANDER_PLACER::origPathLength () const 
{
    int totalP = 0;
    int totalN = 0;

    BOOST_FOREACH ( const PNS_ITEM *item, m_tunedPathP.CItems() )
    {
        if ( const PNS_LINE *l = dyn_cast<const PNS_LINE *> (item) )
            totalP += l->CLine( ).Length( );
        
    }
    BOOST_FOREACH ( const PNS_ITEM *item, m_tunedPathN.CItems() )
    {
        if ( const PNS_LINE *l = dyn_cast<const PNS_LINE *> (item) )
            totalN += l->CLine( ).Length( );
        
    }

    return std::max(totalP, totalN);
}

const SEG PNS_DP_MEANDER_PLACER::baselineSegment ( const PNS_DIFF_PAIR::COUPLED_SEGMENTS& aCoupledSegs )
{
    const VECTOR2I a( ( aCoupledSegs.coupledP.A + aCoupledSegs.coupledN.A ) / 2 );
    const VECTOR2I b( ( aCoupledSegs.coupledP.B + aCoupledSegs.coupledN.B ) / 2 );

    return SEG (a, b);
}


#if 0
PNS_MEANDER_PLACER_BASE::TUNING_STATUS PNS_DP_MEANDER_PLACER::tuneLineLength ( PNS_MEANDERED_LINE& aTuned, int aElongation )
{
    int remaining = aElongation; 
    bool finished = false;

    BOOST_FOREACH(PNS_MEANDER_SHAPE *m, aTuned.Meanders())
    {

        if(m->Type() != MT_CORNER )
        {

            if(remaining >= 0)
                remaining -= m->MaxTunableLength() - m->BaselineLength();

            if(remaining < 0)
            {
                if(!finished)
                    {
                        PNS_MEANDER_TYPE newType;

                        if ( m->Type() == MT_START || m->Type() == MT_SINGLE)
                            newType = MT_SINGLE;
                        else
                            newType = MT_FINISH;

                        m->SetType ( newType );
                        m->Recalculate( );
                        
                        finished = true;
                    } else {
                        m->MakeEmpty();
                    }
            }
        }
    }

    remaining = aElongation;
    int meanderCount = 0;

    BOOST_FOREACH(PNS_MEANDER_SHAPE *m, aTuned.Meanders())
    {
        if( m->Type() != MT_CORNER && m->Type() != MT_EMPTY )
        {
            if(remaining >= 0)
            {
                remaining -= m->MaxTunableLength() - m->BaselineLength();
                meanderCount ++;
            }
        }
    }

    int balance = 0;


    if( meanderCount )
        balance = -remaining / meanderCount;
    
    if (balance >= 0)
    {
        BOOST_FOREACH(PNS_MEANDER_SHAPE *m, aTuned.Meanders())
        {
            if(m->Type() != MT_CORNER && m->Type() != MT_EMPTY)
            {
//                int pre = m->MaxTunableLength();
                m->Resize ( std::max( m->Amplitude() - balance / 2, m_settings.m_minAmplitude ) );
            }
        }
        
    }
    return TUNED;
}
#endif



bool pairOrientation( const PNS_DIFF_PAIR::COUPLED_SEGMENTS& aPair ) 
{
    VECTOR2I midp = ( aPair.coupledP.A + aPair.coupledN.A ) / 2;

    //DrawDebugPoint (midp, 6);

    return aPair.coupledP.Side ( midp ) > 0;
}

bool PNS_DP_MEANDER_PLACER::Move( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    


//    return false;

    if(m_currentNode)
        delete m_currentNode;

    m_currentNode = m_world->Branch();
    
    SHAPE_LINE_CHAIN preP, tunedP, postP;
    SHAPE_LINE_CHAIN preN, tunedN, postN;

    cutTunedLine ( m_originPair.CP(), m_currentStart, aP, preP, tunedP, postP );
    cutTunedLine ( m_originPair.CN(), m_currentStart, aP, preN, tunedN, postN );
      
    PNS_DIFF_PAIR tuned ( m_originPair );

    tuned.SetShape ( tunedP, tunedN );

    m_coupledSegments.clear();

    tuned.CoupledSegmentPairs ( m_coupledSegments );

    if (m_coupledSegments.size() == 0)
        return false;

    //Router()->DisplayDebugLine ( tuned.CP(), 5, 20000 );
    //Router()->DisplayDebugLine ( tuned.CN(), 4, 20000 );

    //Router()->DisplayDebugLine ( m_originPair.CP(), 5, 20000 );
    //Router()->DisplayDebugLine ( m_originPair.CN(), 4, 20000 );

    m_result = PNS_MEANDERED_LINE (this, true );
    m_result.SetWidth ( tuned.Width() );
    
    int offset =  ( tuned.Gap() + tuned.Width() ) / 2;

    if ( !pairOrientation ( m_coupledSegments[0] ) )
        offset *= -1;

    
    m_result.SetBaselineOffset ( offset );


    BOOST_FOREACH ( const PNS_ITEM *item, m_tunedPathP.CItems() )
    {
        if ( const PNS_LINE *l = dyn_cast<const PNS_LINE *> (item) )
            Router()->DisplayDebugLine ( l->CLine(), 5, 10000 );
        
    }
    
    BOOST_FOREACH ( const PNS_ITEM *item, m_tunedPathN.CItems() )
    {
        if ( const PNS_LINE *l = dyn_cast<const PNS_LINE *> (item) )
            Router()->DisplayDebugLine ( l->CLine(), 5, 10000 );
    }


    BOOST_FOREACH ( const PNS_DIFF_PAIR::COUPLED_SEGMENTS& sp, m_coupledSegments )
    {
        SEG base = baselineSegment ( sp );
    
    //    DrawDebugSeg ( base, 3 );
        
        m_result.AddCorner ( sp.parentP.A, sp.parentN.A );
        m_result.MeanderSegment ( base );
        m_result.AddCorner ( sp.parentP.B, sp.parentN.B );
    }

    int dpLen = origPathLength();

    m_lastStatus = TUNED;

    if (dpLen - m_settings.m_targetLength > m_settings.m_lengthTollerance)
    {
        m_lastStatus = TOO_LONG;
        m_lastLength = dpLen;
    } else {

        m_lastLength = dpLen - std::max ( tunedP.Length(), tunedN.Length() );
        tuneLineLength(m_result, m_settings.m_targetLength - dpLen );
    }

    if (m_lastStatus != TOO_LONG)
    {
        tunedP.Clear();
        tunedN.Clear();

        BOOST_FOREACH ( PNS_MEANDER_SHAPE *m, m_result.Meanders() )
        {
            if( m->Type() != MT_EMPTY )
            {
                tunedP.Append ( m->CLine(0) );
                tunedN.Append ( m->CLine(1) );
            }            
        }

        m_lastLength += std::max ( tunedP.Length(), tunedN.Length() );
        
        int comp = compareWithTollerance( m_lastLength - m_settings.m_targetLength, 0, m_settings.m_lengthTollerance );

        if( comp > 0 )
            m_lastStatus = TOO_LONG;
        else if( comp < 0 )
            m_lastStatus = TOO_SHORT;
        else
            m_lastStatus = TUNED;

    }

    m_finalShapeP.Clear( );
    m_finalShapeP.Append( preP );
    m_finalShapeP.Append( tunedP );
    m_finalShapeP.Append( postP );
    m_finalShapeP.Simplify();
    
    m_finalShapeN.Clear( );
    m_finalShapeN.Append( preN );
    m_finalShapeN.Append( tunedN );
    m_finalShapeN.Append( postN );
    m_finalShapeN.Simplify();

    return true;
}


bool PNS_DP_MEANDER_PLACER::FixRoute( const VECTOR2I& aP, PNS_ITEM* aEndItem )
{
    PNS_LINE lP ( m_originPair.PLine(), m_finalShapeP );
    PNS_LINE lN ( m_originPair.NLine(), m_finalShapeN );
    
    m_currentNode->Add ( &lP );
    m_currentNode->Add ( &lN );

    Router()->CommitRouting( m_currentNode );
    
    return true;
}
        
bool PNS_DP_MEANDER_PLACER::CheckFit ( PNS_MEANDER_SHAPE *aShape )
{
    PNS_LINE l1 ( m_originPair.PLine(), aShape->CLine(0) );
    PNS_LINE l2 ( m_originPair.NLine(), aShape->CLine(1) );
    
    if( m_currentNode->CheckColliding( &l1 ) )
        return false;

    if( m_currentNode->CheckColliding( &l2 ) )
        return false;

    int w = aShape->Width();
    int clearance = w + m_settings.m_spacing;

    return m_result.CheckSelfIntersections( aShape, clearance );
}

  
const PNS_ITEMSET PNS_DP_MEANDER_PLACER::Traces()
{
    
    m_currentTraceP = PNS_LINE ( m_originPair.PLine(), m_finalShapeP );
    m_currentTraceN = PNS_LINE ( m_originPair.NLine(), m_finalShapeN );

    PNS_ITEMSET traces;
    
    traces.Add (&m_currentTraceP);
    traces.Add (&m_currentTraceN);
        
    return traces;
}

const VECTOR2I& PNS_DP_MEANDER_PLACER::CurrentEnd() const
{
    return m_currentEnd;
}
    
int PNS_DP_MEANDER_PLACER::CurrentNet() const
{
    return m_initialSegment->Net();
}

int PNS_DP_MEANDER_PLACER::CurrentLayer() const
{
    return m_initialSegment->Layers().Start();
}

const wxString PNS_DP_MEANDER_PLACER::TuningInfo() const
{
    wxString status;

    switch (m_lastStatus)
    {
        case TOO_LONG:
            status = _("Too long: ");
            break;
        case TOO_SHORT:
            status = _("Too short: ");
            break;
        case TUNED:
            status = _("Tuned: ");
            break;
        default:
            return _("?");
    }

    status += LengthDoubleToString( (double) m_lastLength, false );
    status += "/";
    status += LengthDoubleToString( (double) m_settings.m_targetLength, false );
    
    return status;
}

PNS_DP_MEANDER_PLACER::TUNING_STATUS PNS_DP_MEANDER_PLACER::TuningStatus() const
{
    return m_lastStatus;
}

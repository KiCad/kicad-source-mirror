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

#include "pns_dragger.h"
#include "pns_shove.h"
#include "pns_router.h"

PNS_DRAGGER::PNS_DRAGGER( PNS_ROUTER* aRouter ) :
    PNS_ALGO_BASE ( aRouter )
{
    m_world = NULL;
    m_shove = NULL;
};

PNS_DRAGGER::~PNS_DRAGGER()
{
    if( m_shove )
        delete m_shove;
}

void PNS_DRAGGER::SetWorld ( PNS_NODE *aWorld )
{
    m_world = aWorld;
}

bool PNS_DRAGGER::startDragSegment( const VECTOR2D& aP, PNS_SEGMENT *aSeg )
{
    int w2 = aSeg->Width() / 2;

    m_draggedLine = m_world->AssembleLine ( aSeg, &m_draggedSegmentIndex );
    m_shove->SetInitialLine (m_draggedLine);
    m_lastValidDraggedLine = *m_draggedLine;
    m_lastValidDraggedLine.ClearSegmentLinks();

    if( (aP - aSeg->Seg().A).EuclideanNorm() <= w2 )
        m_mode = CORNER;
    else if( (aP - aSeg->Seg().B).EuclideanNorm() <= w2 )
    {
        m_draggedSegmentIndex ++;
        m_mode = CORNER;
    } else 
        m_mode = SEGMENT;
    return true;
}

bool PNS_DRAGGER::startDragVia( const VECTOR2D& aP, PNS_VIA *aVia )
{
    m_draggedVia = aVia;
    m_initialVia = aVia;
    m_mode = VIA;

    VECTOR2I p0 ( aVia->Pos() );
    PNS_JOINT *jt = m_world->FindJoint( p0, aVia->Layers().Start(), aVia->Net() );

    BOOST_FOREACH(PNS_ITEM *item, jt->LinkList() )
    {
        if(item->OfKind( PNS_ITEM::SEGMENT ))
        {
            int segIndex;
            PNS_SEGMENT *seg = (PNS_SEGMENT *) item;
            std::auto_ptr<PNS_LINE> l ( m_world->AssembleLine(seg, &segIndex) );
            
            if(segIndex != 0)
                l->Reverse();

            m_origViaConnections.push_back (*l);

        }
    }
          

    return true;
}

bool PNS_DRAGGER::Start ( const VECTOR2I& aP, PNS_ITEM* aStartItem )
{
	m_shove = new PNS_SHOVE ( m_world, Router() );
    m_lastNode = NULL;
    m_draggedItems.Clear();
    m_currentMode = Settings().Mode();

    TRACE(2, "StartDragging: item %p [kind %d]", aStartItem % aStartItem->Kind());

	switch( aStartItem->Kind() )
	{
		case PNS_ITEM::SEGMENT:
			return startDragSegment ( aP, static_cast<PNS_SEGMENT *> (aStartItem) );
		case PNS_ITEM::VIA:
			return startDragVia ( aP, static_cast<PNS_VIA *> (aStartItem) );
		default:
			return false;
	}
}

bool PNS_DRAGGER::dragMarkObstacles(const VECTOR2I& aP)
{   
    if(m_lastNode)
    {
        delete m_lastNode;
        m_lastNode = NULL;
    }

    switch(m_mode)
    {
        case SEGMENT:
        case CORNER:
        {
            int thresh = Settings().SmoothDraggedSegments() ? m_draggedLine->Width() / 4 : 0;
            PNS_LINE tmp (*m_draggedLine);
            
            if(m_mode == SEGMENT)
                tmp.DragSegment ( aP, m_draggedSegmentIndex, thresh );
            else
                tmp.DragCorner ( aP, m_draggedSegmentIndex, thresh );
            
            m_lastNode = m_shove->CurrentNode()->Branch();

            m_lastValidDraggedLine = tmp;
            m_lastValidDraggedLine.ClearSegmentLinks();
            m_lastValidDraggedLine.Unmark();
            
            m_lastNode->Add ( &m_lastValidDraggedLine );
            m_draggedItems = PNS_ITEMSET ( &m_lastValidDraggedLine );

            break;
        }
        case VIA: // fixme...
        {
            m_lastNode = m_shove->CurrentNode()->Branch();
            dumbDragVia ( m_initialVia, m_lastNode, aP );


            break;
        }
    }

    if (Settings().CanViolateDRC())
        m_dragStatus = true;
    else
        m_dragStatus = !m_world->CheckColliding( m_draggedItems );
    
    return true;
}

void PNS_DRAGGER::dumbDragVia ( PNS_VIA *aVia, PNS_NODE *aNode, const VECTOR2I& aP )
{
    // fixme: this is awful.
    m_draggedVia = aVia->Clone();
    m_draggedVia->SetPos( aP );
    m_draggedItems.Clear();
    m_draggedItems.Add(m_draggedVia);
    
    m_lastNode->Remove ( aVia );
    m_lastNode->Add ( m_draggedVia );

    BOOST_FOREACH(PNS_LINE &l, m_origViaConnections)
    {
        PNS_LINE origLine (l);
        PNS_LINE *draggedLine = l.Clone();
        
        draggedLine->DragCorner( aP, 0 );
        draggedLine->ClearSegmentLinks();

        m_draggedItems.AddOwned( draggedLine );            
        
        m_lastNode->Remove ( &origLine );
        m_lastNode->Add ( draggedLine );
    }
}

bool PNS_DRAGGER::dragShove(const VECTOR2I& aP)
{
    bool ok = false;
 

    if(m_lastNode)
    {
        delete m_lastNode;
        m_lastNode = NULL;
    }

    switch(m_mode)
    {
    	case SEGMENT:
    	case CORNER:
    	{
    	    int thresh = Settings().SmoothDraggedSegments() ? m_draggedLine->Width() / 4 : 0;
        	PNS_LINE tmp (*m_draggedLine);
    		if(m_mode == SEGMENT)
    			tmp.DragSegment ( aP, m_draggedSegmentIndex, thresh );
    		else
    			tmp.DragCorner ( aP, m_draggedSegmentIndex, thresh );
            
            PNS_SHOVE::ShoveStatus st = m_shove->ShoveLines( tmp );
            
            if(st == PNS_SHOVE::SH_OK)
            	ok = true;
            else if (st == PNS_SHOVE::SH_HEAD_MODIFIED)
            {
                tmp = m_shove->NewHead();
                ok = true;
            } 

            m_lastNode = m_shove->CurrentNode()->Branch();

            if(ok)            	
                m_lastValidDraggedLine = tmp;
            
            m_lastValidDraggedLine.ClearSegmentLinks();
            m_lastValidDraggedLine.Unmark();
            m_lastNode->Add ( &m_lastValidDraggedLine );
            m_draggedItems = PNS_ITEMSET ( &m_lastValidDraggedLine );

            break;
    	}
        case VIA:
        {  
            PNS_VIA *newVia;
            PNS_SHOVE::ShoveStatus st = m_shove -> ShoveDraggingVia ( m_draggedVia, aP, &newVia );

            if(st == PNS_SHOVE::SH_OK || st == PNS_SHOVE::SH_HEAD_MODIFIED)
                ok = true;

            m_lastNode = m_shove->CurrentNode()->Branch();

            if( ok )
            {
                m_draggedVia = newVia;
                m_draggedItems.Clear();
            }

            break;
        }
            
    }
 
    m_dragStatus = ok;
    return ok;
}

bool PNS_DRAGGER::FixRoute( )
{
    if(m_dragStatus)
    {
        Router()->CommitRouting( CurrentNode() );
        return true;
    } 

    return false;
}
	
bool PNS_DRAGGER::Drag ( const VECTOR2I& aP )
{
	switch ( m_currentMode )
	{
		case RM_MarkObstacles:
			return dragMarkObstacles (aP);
		case RM_Shove:
		case RM_Walkaround:
		case RM_Smart:
			return dragShove ( aP );
		default: 
			return false;
	}
}

PNS_NODE *PNS_DRAGGER::CurrentNode() const
{
   return m_lastNode;
}

const PNS_ITEMSET PNS_DRAGGER::Traces()
{
    return m_draggedItems;
}

PNS_LOGGER *PNS_DRAGGER::Logger()
{
    if(m_shove)
        return m_shove->Logger();
    return NULL;
}

/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
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
 * with this program.  If not, see <http://www.gnu.or/licenses/>.
 */

#ifndef __PNS_LINE_PLACER_H
#define __PNS_LINE_PLACER_H

#include <math/vector2d.h>

#include <geometry/shape.h>
#include <geometry/shape_line_chain.h>

#include "pns_node.h"
#include "pns_via.h"
#include "pns_line.h"
#include "pns_routing_settings.h"
 
class PNS_ROUTER;
class PNS_SHOVE;
class PNS_OPTIMIZER;
class PNS_ROUTER_BASE;

/**
 * Class PNS_LINE_PLACER
 *
 * Interactively routes a single track. Runs shove and walkaround algorithms when needed.
 */

class PNS_LINE_PLACER
{
	public:
		PNS_LINE_PLACER( PNS_NODE *aWorld );
		~PNS_LINE_PLACER();

		///> Appends a via at the end of currently placed line.		
		void AddVia ( bool aEnabled, int aDiameter, int aDrill )
		{
			m_viaDiameter = aDiameter;
			m_viaDrill = aDrill;
			m_placingVia = aEnabled;
		}

		///> Starts placement of a line at point aStart.
		void StartPlacement(const VECTOR2I& aStart, int aNet, int aWidth, int aLayer);
		
		///> Updates the routed line with a new ending point.
		bool Route(const VECTOR2I& aP);
		
		///> Sets initial routing direction/posture
		void SetInitialDirection(const DIRECTION_45& aDirection);
		
		void ApplySettings ( const PNS_ROUTING_SETTINGS& aSettings );

		///> Returns the "head" of the line being placed, that is the volatile part that has not been settled yet
		const PNS_LINE& GetHead() const	{ return m_head; }
		///> Returns the "tail" of the line being placed the part that has been fixed already (follow mouse mode only)
		const PNS_LINE& GetTail() const { return m_tail; }

		///> Returns the whole routed line
		const PNS_LINE GetTrace() const;

		///> Returns the current end of the line being placed. It may not be equal to the cursor position due to collisions.
		const VECTOR2I& CurrentEnd() const
		{
			if(m_head.GetCLine().PointCount() > 0)
				return m_head.GetCLine().CPoint(-1);
			else if(m_tail.GetCLine().PointCount() > 0)
				return m_tail.GetCLine().CPoint(-1);
			else
				return m_p_start;
		}


		///> Returns all items in the world that have been affected by the routing operation. Used
		///  to update data structures of the host application
		void GetUpdatedItems( PNS_NODE::ItemVector& aRemoved,  PNS_NODE::ItemVector& aAdded);
		
		///> Toggles the current posture (straight/diagonal) of the trace head.
		void FlipPosture();

		///> Returns the most recent world state 
		PNS_NODE *GetCurrentNode() const;

	private:
	
		static const double m_shoveLengthThreshold = 1.7;

		bool handleViaPlacement ( PNS_LINE& aHead );

		bool checkObtusity(const SEG& a, const SEG& b) const;
		bool handleSelfIntersections();
		bool handlePullback();
		bool mergeHead();
		bool reduceTail(const VECTOR2I& aEnd); 
		void fixHeadPosture();
		bool optimizeTailHeadTransition();
		
		bool routeHead(const VECTOR2I& aP, PNS_LINE& aNewHead, bool aCwWalkaround = true);
		void routeStep(const VECTOR2I& aP);

		///> routing mode (walkaround, shove, etc.)
		PNS_MODE m_mode;
		///> follow mouse trail by attaching new segments to the head as the cursor moves
		bool m_follow_mouse;
		///> mouse smoothing active
		bool m_smooth_mouse;
		///> mouse smoothing step (in world units)
		int m_smoothing_step;
		///> current routing direction
		DIRECTION_45 m_direction;
		///> routing direction for new traces
		DIRECTION_45 m_initial_direction;
		///> routing "head": volatile part of the track from the previously 
		///  analyzed point to the current routing destination
		PNS_LINE m_head;
		///> routing "tail": part of the track that has been already fixed due to collisions with obstacles
		PNS_LINE m_tail;
		///> current algorithm iteration
		int m_iteration;
		///> pointer to world to search colliding items
		PNS_NODE *m_world;
		///> current routing start point (end of tail, beginning of head)
		VECTOR2I m_p_start;
		///> The shove engine
		PNS_SHOVE *m_shove;
		///> Current world state
		PNS_NODE *m_currentNode;
		///> Are we placing a via?
		bool m_placingVia;
		///> current via diameter
		int m_viaDiameter;
		///> current via drill
		int m_viaDrill;
		///> walkaround algorithm iteration limit
		int m_walkaroundIterationLimit;
		///> smart pads optimizer enabled.
		bool m_smartPads;
};
		
#endif // __PNS_LINE_PLACER_H

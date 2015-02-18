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

#ifndef __PNS_TOPOLOGY_H
#define __PNS_TOPOLOGY_H

#include <vector>
#include <set>

#include "pns_itemset.h"

class PNS_NODE;
class PNS_SEGMENT;
class PNS_JOINT;
class PNS_ITEM;
class PNS_SOLID;
class PNS_DIFF_PAIR;

class PNS_TOPOLOGY
{
	public:
		typedef std::set<PNS_JOINT *> JOINT_SET;

		PNS_TOPOLOGY ( PNS_NODE *aNode ):
			m_world ( aNode ) {};

		~PNS_TOPOLOGY ( ) {};

		bool SimplifyLine ( PNS_LINE *aLine );
		PNS_ITEM* NearestUnconnectedItem( PNS_JOINT* aStart, int* aAnchor = NULL, int aKindMask = PNS_ITEM::ANY );
		bool LeadingRatLine( const PNS_LINE *aTrack, SHAPE_LINE_CHAIN& aRatLine );

		const JOINT_SET ConnectedJoints ( PNS_JOINT* aStart );
		const PNS_ITEMSET ConnectedItems ( PNS_JOINT* aStart, int aKindMask = PNS_ITEM::ANY );
		const PNS_ITEMSET ConnectedItems ( PNS_ITEM* aStart, int aKindMask = PNS_ITEM::ANY );
		int64_t ShortestConnectionLength ( PNS_ITEM *aFrom, PNS_ITEM *aTo );
		


		const PNS_ITEMSET AssembleTrivialPath ( PNS_SEGMENT *aStart );
		const PNS_DIFF_PAIR AssembleDiffPair ( PNS_SEGMENT *aStart );

		int MatchDpSuffix ( wxString aNetName, wxString& aComplementNet, wxString& aBaseDpName );
		int DpCoupledNet( int aNet );
		int DpNetPolarity( int aNet );
		const PNS_LINE DpCoupledLine( PNS_LINE *aLine );
		bool AssembleDiffPair ( PNS_ITEM *aStart, PNS_DIFF_PAIR& aPair );


		
	private:

		bool followTrivialPath ( PNS_LINE *aLine, bool aLeft, PNS_ITEMSET& aSet, std::set<PNS_ITEM *>& aVisited );

		PNS_NODE *m_world;
};

#endif

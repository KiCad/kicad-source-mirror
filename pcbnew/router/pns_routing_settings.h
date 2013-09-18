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
 
#ifndef __PNS_ROUTER_SETTINGS
#define __PNS_ROUTER_SETTINGS

///> Routing modes
enum PNS_MODE {
	RM_Ignore = 0, ///> Ignore collisions
	RM_Shove,      ///> Only shove
	RM_Walkaround, ///> Only walkaround
	RM_Smart       ///> Guess what's better
};

class PNS_ROUTING_SETTINGS 
{
	public:
		PNS_MODE m_routingMode;

		bool m_removeLoops;
		bool m_smartPads;
		bool m_suggestEnding;
		bool m_shoveOnRequest;
		bool m_changePostures;
		bool m_followMouse;

		int m_lineWidth;
		int m_viaDiameter;
		int m_viaDrill;
		int m_preferredLayer;
		int m_walkaroundIterationLimit;
		int m_shoveIterationLimit;
};

#endif


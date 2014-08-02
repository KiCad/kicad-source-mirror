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

#ifndef __PNS_ALGO_BASE_H
#define __PNS_ALGO_BASE_H

#include "pns_routing_settings.h"

class PNS_ROUTER;
class PNS_LOGGER;

/**
 * Class PNS_ALGO_BASE
 *
 * Base class for all P&S algorithms (shoving, walkaround, line placement, dragging, etc.)
 * Holds a bunch of objects commonly used by all algorithms (P&S settings, parent router instance, logging)
 **/

class PNS_ALGO_BASE
{
public:
	PNS_ALGO_BASE( PNS_ROUTER *aRouter ) :
		m_router ( aRouter )
	{}

	virtual ~PNS_ALGO_BASE() {}

	///> Returns the instance of our router
	PNS_ROUTER* Router() const
	{
		return m_router;
	}

	///> Returns current router settings
	PNS_ROUTING_SETTINGS& Settings() const;

	///> Returns the logger object, allowing to dump geometry to a file.
	virtual PNS_LOGGER* Logger();
	
private:
	PNS_ROUTER* m_router;
};

#endif

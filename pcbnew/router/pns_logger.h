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

#ifndef __PNS_LOGGER_H
#define __PNS_LOGGER_H

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

#include <math/vector2d.h>

class PNS_ITEM;
class SHAPE_LINE_CHAIN;
class SHAPE;

class PNS_LOGGER {

public:
	PNS_LOGGER();
	~PNS_LOGGER();

	void Save ( const std::string& filename );

	void Clear();
	void NewGroup ( const std::string& name, int iter = 0);
	void EndGroup();
	void Log ( const PNS_ITEM *item, int kind = 0, const std::string name = std::string () );
	void Log ( const SHAPE_LINE_CHAIN *l, int kind = 0, const std::string name = std::string () );
	void Log ( const VECTOR2I& start, const VECTOR2I& end, int kind = 0, const std::string name = std::string () );

private:
	
	void dumpShape ( const SHAPE* sh );

	bool m_groupOpened;
	std::stringstream m_theLog;
};

#endif

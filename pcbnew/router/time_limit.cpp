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

#include <wx/timer.h>

#include "time_limit.h"

TIME_LIMIT::TIME_LIMIT( int aMilliseconds ) : 
	m_limitMs( aMilliseconds )
{
    Restart();
}


TIME_LIMIT::~TIME_LIMIT()
{}


bool TIME_LIMIT::Expired() const
{
	return ( wxGetLocalTimeMillis().GetValue() - m_startTics ) >= m_limitMs;
}


void TIME_LIMIT::Restart()
{
	m_startTics = wxGetLocalTimeMillis().GetValue();
}


void TIME_LIMIT::Set( int aMilliseconds )
{
	m_limitMs = aMilliseconds;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef SPICE_REPORTER_H
#define SPICE_REPORTER_H

#include <reporter.h>

class SPICE_SIMULATOR;

enum SIM_STATE
{
    SIM_IDLE,
    SIM_RUNNING
};

/**
 * @brief Interface to receive simulation updates from SPICE_SIMULATOR class.
 */
class SPICE_REPORTER : public REPORTER
{
public:
    virtual ~SPICE_REPORTER()
    {
    }

    virtual void OnSimStateChange( SPICE_SIMULATOR* aObject, SIM_STATE aNewState ) = 0;
};

#endif /* SPICE_REPORTER_H */

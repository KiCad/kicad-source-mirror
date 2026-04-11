/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
* Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIM_REPORTER_H
#define SIM_REPORTER_H

#include <reporter.h>

class SIMULATOR;

enum SIM_STATE
{
    SIM_IDLE,
    SIM_RUNNING
};

/**
 * @brief Interface to receive simulation updates from SPICE_SIMULATOR class.
 */
class SIMULATOR_REPORTER : public REPORTER
{
public:
    virtual ~SIMULATOR_REPORTER()
    {
    }

    virtual void OnSimStateChange( SIMULATOR* aObject, SIM_STATE aNewState ) = 0;
};

#endif /* SIM_REPORTER_H */

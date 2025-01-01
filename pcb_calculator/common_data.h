/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 jean-pierre.charras
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file common_data.h
 * @note lists of constants used in different panels
 */

#ifndef COMMON_DATA_H
#define COMMON_DATA_H

#include <wx/arrstr.h>

/**
 * @return a list of dielectric constants (Er) of some materials
 * used to make PCBs
 */
wxArrayString StandardRelativeDielectricConstantList();


/**
 * @return a list of loss tangent of some materials
 * used to make PCBs
 */
wxArrayString StandardLossTangentList();

/**
 * @return a list of resistivity constants (Er) of some conductors
 * used to make PCBs
 */
wxArrayString StandardResistivityList();

/**
 * @return a list of resistivity constants (Er) of some conductors
 * used to make cable cores
 */
wxArrayString StandardCableConductorList();

/**
 * @return a list of temperature coefficient constants of some conductors
 * used to make cable cores
 */
wxArrayString StandardCableTempCoefList();

#endif  // #ifndef COMMON_DATA_H

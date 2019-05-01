/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2019 jean-pierre.charras
 * Copyright (C) 1992-2019 Kicad Developers, see AUTHORS.txt for contributors.
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
 * @file common_data.cpp
 * @note lists of constants used in different panels
 */

#include <common_data.h>

wxArrayString StandardRelativeDielectricConstantList()
{
    wxArrayString list;

    // EpsilonR ( relative dielectric constant) list
    list.Add( wxT( "4.5  FR4" ) );
    list.Add( wxT( "3.67  Isola FR408" ) );
    list.Add( wxT( "4.04  Isola 370HR" ) );
    list.Add( wxT( "3.55  Rogers RO4003C" ) );
    list.Add( wxT( "3.66  Rogers R4350B" ) );
    list.Add( wxT( "9.8  alumina (Al2O3)" ) );
    list.Add( wxT( "3.78  fused quartz" ) );
    list.Add( wxT( "3.38  RO4003" ) );
    list.Add( wxT( "2.2  RT/duroid 5880" ) );
    list.Add( wxT( "10.2  RT/duroid 6010LM" ) );
    list.Add( wxT( "2.1  teflon (PTFE)" ) );
    list.Add( wxT( "4.0  PVC" ) );
    list.Add( wxT( "2.3  PE" ) );
    list.Add( wxT( "6.6  beryllia (BeO)" ) );
    list.Add( wxT( "8.7  aluminum nitride" ) );
    list.Add( wxT( "11.9  silicon" ) );
    list.Add( wxT( "12.9  GaAs" ) );

    return list;
}


wxArrayString StandardLossTangentList()
{
    wxArrayString list;

    // List of current dielectric loss factor (tangent delta)
    list.Clear();
    list.Add( wxT( "2e-2  FR4 @ 1GHz" ) );
    list.Add( wxT( "12e-3  Isola FR408 @ 2 GHz" ) );
    list.Add( wxT( "21e-3  Isola 370HR @ 2 GHz" ) );
    list.Add( wxT( "27e-4  Rogers RO4003C @ 10 GHz" ) );
    list.Add( wxT( "21e-4  Rogers RO4003C @ 2.5 GHz" ) );
    list.Add( wxT( "37e-4  Rogers RO4350B @ 10 GHz" ) );
    list.Add( wxT( "31e-4  Rogers RO4350B @ 2.5 GHz" ) );
    list.Add( wxT( "3e-4  beryllia @ 10GHz" ) );
    list.Add( wxT( "2e-4  aluminia (Al2O3) @ 10GHz" ) );
    list.Add( wxT( "1e-4  fused quartz @ 10GHz" ) );
    list.Add( wxT( "2e-3  RO4003 @ 10GHz" ) );
    list.Add( wxT( "9e-4  RT/duroid 5880 @ 10GHz" ) );
    list.Add( wxT( "2e-4  teflon (PTFE) @ 1MHz" ) );
    list.Add( wxT( "5e-2  PVC @ 1MHz" ) );
    list.Add( wxT( "2e-4  PE @ 1MHz" ) );
    list.Add( wxT( "1e-3  aluminum nitride @ 10GHz" ) );
    list.Add( wxT( "0.015  silicon @ 10GHz" ) );
    list.Add( wxT( "0.002  GaAs @ 10GHz" ) );

    return list;
}


wxArrayString StandardResistivityList()
{
    wxArrayString list;

    // Specific resistance list in ohms*meters (rho):
    list.Clear();
    list.Add( wxT( "2.4e-8  gold" ) );
    list.Add( wxT( "1.72e-8  copper" ) );
    list.Add( wxT( "1.62e-8  silver" ) );
    list.Add( wxT( "12.4e-8  tin" ) );
    list.Add( wxT( "10.5e-8  platinum" ) );
    list.Add( wxT( "2.62e-8  aluminum" ) );
    list.Add( wxT( "6.9e-8  nickel" ) );
    list.Add( wxT( "3.9e-8  brass (66Cu 34Zn)" ) );
    list.Add( wxT( "9.71e-8  iron" ) );
    list.Add( wxT( "6.0e-8  zinc" ) );

    return list;
}

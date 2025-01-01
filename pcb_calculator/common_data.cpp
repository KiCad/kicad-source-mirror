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
 * @file common_data.cpp
 * @note lists of constants used in different panels
 */

#include "common_data.h"

wxArrayString StandardRelativeDielectricConstantList()
{
    wxArrayString list;

    // EpsilonR ( relative dielectric constant) list
    list.Add( wxT( "4.5 \tFR4" ) );
    list.Add( wxT( "3.67 \tIsola FR408" ) );
    list.Add( wxT( "4.04 \tIsola 370HR" ) );
    list.Add( wxT( "3.55 \tRogers RO4003C" ) );
    list.Add( wxT( "3.66 \tRogers R4350B" ) );
    list.Add( wxT( "9.8 \talumina (Al2O3)" ) );
    list.Add( wxT( "3.78 \tfused quartz" ) );
    list.Add( wxT( "3.38 \tRO4003" ) );
    list.Add( wxT( "2.2 \tRT/duroid 5880" ) );
    list.Add( wxT( "10.2 \tRT/duroid 6010LM" ) );
    list.Add( wxT( "2.1 \tteflon (PTFE)" ) );
    list.Add( wxT( "4.0 \tPVC" ) );
    list.Add( wxT( "2.3 \tPE" ) );
    list.Add( wxT( "6.6 \tberyllia (BeO)" ) );
    list.Add( wxT( "8.7 \taluminum nitride" ) );
    list.Add( wxT( "11.9 \tsilicon" ) );
    list.Add( wxT( "12.9 \tGaAs" ) );

    return list;
}


wxArrayString StandardLossTangentList()
{
    wxArrayString list;

    // List of current dielectric loss factor (tangent delta)
    list.Clear();
    list.Add( wxT( "0.02 \tFR4 @ 1GHz" ) );
    list.Add( wxT( "0.012 \tIsola FR408 @ 2 GHz" ) );
    list.Add( wxT( "0.021 \tIsola 370HR @ 2 GHz" ) );
    list.Add( wxT( "0.0027 \tRogers RO4003C @ 10 GHz" ) );
    list.Add( wxT( "0.0021 \tRogers RO4003C @ 2.5 GHz" ) );
    list.Add( wxT( "0.0037 \tRogers RO4350B @ 10 GHz" ) );
    list.Add( wxT( "0.0031 \tRogers RO4350B @ 2.5 GHz" ) );
    list.Add( wxT( "3e-4 \tberyllia @ 10GHz" ) );
    list.Add( wxT( "2e-4 \taluminia (Al2O3) @ 10GHz" ) );
    list.Add( wxT( "1e-4 \tfused quartz @ 10GHz" ) );
    list.Add( wxT( "0.002 \tRO4003 @ 10GHz" ) );
    list.Add( wxT( "9e-4 \tRT/duroid 5880 @ 10GHz" ) );
    list.Add( wxT( "2e-4 \tteflon (PTFE) @ 1MHz" ) );
    list.Add( wxT( "0.05 \tPVC @ 1MHz" ) );
    list.Add( wxT( "2e-4 \tPE @ 1MHz" ) );
    list.Add( wxT( "0.001 \taluminum nitride @ 10GHz" ) );
    list.Add( wxT( "0.015 \tsilicon @ 10GHz" ) );
    list.Add( wxT( "0.002 \tGaAs @ 10GHz" ) );

    return list;
}


wxArrayString StandardResistivityList()
{
    wxArrayString list;

    // Specific resistance list in ohms*meters (rho):
    list.Clear();
    list.Add( wxT( "2.4e-8 \tgold" ) );
    list.Add( wxT( "1.72e-8 \tcopper" ) );
    list.Add( wxT( "1.62e-8 \tsilver" ) );
    list.Add( wxT( "12.4e-8 \ttin" ) );
    list.Add( wxT( "10.5e-8 \tplatinum" ) );
    list.Add( wxT( "2.62e-8 \taluminum" ) );
    list.Add( wxT( "6.9e-8 \tnickel" ) );
    list.Add( wxT( "3.9e-8 \tbrass (66Cu 34Zn)" ) );
    list.Add( wxT( "9.71e-8 \tiron" ) );
    list.Add( wxT( "6.0e-8 \tzinc" ) );

    return list;
}

wxArrayString StandardCableConductorList()
{
    wxArrayString list;

    // Lined the same as StandardCableTempCoefList
    // Specific resistance list in ohms*meters (rho):
    list.Clear();
    list.Add( wxT( "1.72e-8 \tCu, Copper" ) );
    list.Add( wxT( "2.62e-8 \tAl, Aluminum" ) );
    list.Add( wxT( "100e-8 \tNiCr, Nichrome" ) );
    list.Add( wxT( "9.71e-8 \tFe, Iron" ) );
    list.Add( wxT( "5.6e-8 \tW, Tungsten" ) );

    return list;
}

wxArrayString StandardCableTempCoefList()
{
    wxArrayString list;
    // Lined the same as StandardCableConductorList
    // Specific temperature coefficient (20C):
    list.Clear();
    list.Add( wxT( "3.93e-3 \tCu, Copper" ) );
    list.Add( wxT( "4.29e-3 \tAl, Aluminum" ) );
    list.Add( wxT( "0.4e-3 \tNiCr, Nichrome" ) );
    list.Add( wxT( "5e-3 \tFe, Iron" ) );
    list.Add( wxT( "4.5e-3 \tW, Tungsten" ) );

    return list;
}

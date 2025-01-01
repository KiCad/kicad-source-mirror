/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2011 jean-pierre Charras <jean-pierre.charras@gipsa-lab.inpg.fr>
 * Copyright (C) 1992-2011 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


/**
 * @file netlist.h
 */

#ifndef _NETLIST_H_
#define _NETLIST_H_

/// netlist types
enum  NETLIST_TYPE_ID {
    NET_TYPE_UNINIT = 0,
    NET_TYPE_BOM,       // Used by the BOM generator
    NET_TYPE_PCBNEW,
    NET_TYPE_ORCADPCB2,
    NET_TYPE_CADSTAR,
    NET_TYPE_SPICE,
    NET_TYPE_SPICE_MODEL,
    NET_TYPE_ALLEGRO,
    NET_TYPE_PADS,
    NET_TYPE_CUSTOM1,   /* NET_TYPE_CUSTOM1
                         * is the first id for user netlist format
                         * NET_TYPE_CUSTOM1+CUSTOMPANEL_COUNTMAX-1
                         * is the last id for user netlist format
                         */
    //NET_TYPE_CUSTOM_MAX = NET_TYPE_CUSTOM1 + CUSTOMPANEL_COUNTMAX - 1
};


#define NETLIST_HEAD_STRING "EESchema Netlist Version 1.1"

// Max pin number per symbol and footprint.
#define MAXPIN      5000

#endif

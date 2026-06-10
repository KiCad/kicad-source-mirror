/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#ifndef KICAD_PCB_NETLIST_UTILS_H
#define KICAD_PCB_NETLIST_UTILS_H

#include <wx/string.h>

class BOARD;
class LIB_ID;
class NETLIST;
class REPORTER;
class FOOTPRINT;

/**
 * Load a footprint from the project library table and apply board default settings.
 */
FOOTPRINT* LoadFootprintFromProject( BOARD* aBoard, const LIB_ID& aFootprintId,
                                     bool aKeepUuid = false );

/**
 * Load the footprints for each #SCH_COMPONENT in \a aNetlist from the list of libraries.
 *
 * @param aNetlist is the netlist of components to load the footprints into.
 * @param aReporter is the #REPORTER object to report to.
 * @throw IO_ERROR if an I/O error occurs or a #PARSE_ERROR if a file parsing error
 *           occurs while reading footprint library files.
 */
void LoadNetlistFootprints( BOARD* aBoard, NETLIST& aNetlist, REPORTER& aReporter );

#endif

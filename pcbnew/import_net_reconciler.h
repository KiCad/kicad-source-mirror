/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef IMPORT_NET_RECONCILER_H
#define IMPORT_NET_RECONCILER_H

class BOARD;
class NETLIST;
class REPORTER;

/**
 * Outcome of naming the nets of an imported board after the nets of its schematic.
 */
struct NET_RECONCILE_RESULT
{
    int m_renamed = 0;             ///< board nets renamed to the schematic name
    int m_alreadyNamed = 0;        ///< board nets that already carried the schematic name
    int m_conflicting = 0;         ///< board nets whose pads the schematic divides differently
    int m_unconnectedAssigned = 0; ///< pads given the schematic's name for a net that joins nothing
};

/**
 * Give an imported board footprint the reference its schematic symbol uses.
 *
 * Only a footprint whose reference no symbol has is renamed, and only when exactly one unmatched symbol
 * reference equals it ignoring case or once the importer's "UNK" prefix is removed.  Run this before the
 * net names are reconciled, since both match footprints by reference.
 *
 * @param[in] aBoard is the imported board whose footprints are renamed.
 * @param[in] aNetlist is the netlist of the imported schematic.
 * @return the number of footprints renamed.
 */
int ReconcileImportedReferences( BOARD& aBoard, NETLIST& aNetlist );

/**
 * Give each net of an imported board the name its schematic uses.
 *
 * A board net is renamed only when its pads, on the footprints both documents share, are exactly the
 * pins of one schematic net.  A net the two documents divide differently keeps its name and is reported,
 * so a real difference between them stays visible.  A pad that has no net but whose schematic net joins
 * nothing else takes that net's name.
 *
 * Shared by the project manager's import and the `kicad-cli import` job.
 *
 * @param[in] aBoard is the imported board whose nets are renamed.
 * @param[in] aNetlist is the netlist of the imported schematic.
 * @param[in] aReporter receives the counts and the first differences.
 */
NET_RECONCILE_RESULT ReconcileImportedNetNames( BOARD& aBoard, NETLIST& aNetlist, REPORTER& aReporter );

/**
 * Copy what an imported board footprint lacks from the schematic symbol it is linked to.
 *
 * A footprint takes the symbol's value only when its own is empty and gains only the symbol fields it
 * does not have, the same set Update PCB from Schematic copies.  Nothing the board already holds changes.
 *
 * @param[in] aBoard is the imported board whose footprints are completed.
 * @param[in] aNetlist is the netlist of the imported schematic.
 * @return the number of footprints that gained a value or a field.
 */
int FillImportedFootprintFields( BOARD& aBoard, NETLIST& aNetlist );

#endif // IMPORT_NET_RECONCILER_H

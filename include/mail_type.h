/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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


#ifndef MAIL_TYPE_H_
#define MAIL_TYPE_H_

/**
 * The set of mail types sendable via #KIWAY::ExpressMail() and supplied as
 * the @a aCommand parameter to that function.
 *
 * Such mail will be received in KIWAY_PLAYER::KiwayMailIn( KIWAY_EXPRESS& aEvent ) and
 * aEvent.Command() will match aCommand to KIWAY::ExpressMail().
 */
enum MAIL_T
{
    MAIL_CROSS_PROBE,              // PCB<->SCH, CVPCB->SCH cross-probing.
    MAIL_SELECTION,                // SCH<->PCB selection synchronization.
    MAIL_SELECTION_FORCE,          // Explicit selection of SCH->PCB selection synchronization.
    MAIL_ASSIGN_FOOTPRINTS,        // CVPCB->SCH footprint stuffing
    MAIL_SCH_SAVE,                 // CVPCB->SCH save the schematic
    MAIL_EESCHEMA_NETLIST,         // SCH->CVPCB netlist immediately after launching CVPCB
    MAIL_SYMBOL_NETLIST,           // SCH->FP_CHOOSER symbol pin & fp_filter information
    MAIL_PCB_UPDATE,               // SCH->PCB forward update
    MAIL_SCH_UPDATE,               // PCB->SCH forward update
    MAIL_IMPORT_FILE,              // Import a different format file
    MAIL_SCH_GET_NETLIST,          // Fetch a netlist from schematics
    MAIL_SCH_GET_ITEM,             // Fetch item from KIID
    MAIL_PCB_GET_NETLIST,          // Fetch a netlist from PCB layout
    MAIL_PCB_UPDATE_LINKS,         // Update the schematic symbol paths in the PCB's footprints
    MAIL_SCH_REFRESH,              // Tell the schematic editor to refresh the display.
    MAIL_ADD_LOCAL_LIB,            // Add a local library to the project library table
    MAIL_LIB_EDIT,
    MAIL_FP_EDIT,
    MAIL_RELOAD_LIB,               // Reload Library List if one was added
    MAIL_RELOAD_PLUGINS,           // Reload python plugins
    MAIL_REFRESH_SYMBOL,           // Refresh symbol in symbol viewer
    MAIL_SCH_NAVIGATE_TO_SHEET     // Navigate to sheet by filename if in hierarchy
};

#endif  // MAIL_TYPE_H_

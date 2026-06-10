/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_PCB_CONTEXT_H
#define KICAD_PCB_CONTEXT_H

#include <memory>

#include <wx/string.h>

#include <api/board_context.h>

class BOARD_NETLIST_UPDATER;
class NETLIST;
class PCB_EDIT_FRAME;
class REPORTER;


/// PCB-editor-specific context; extends BOARD_CONTEXT with save/filename operations
class PCB_CONTEXT : public BOARD_CONTEXT
{
public:
    virtual wxString GetCurrentFileName() const = 0;

    virtual bool SaveBoard() = 0;

    virtual bool SavePcbCopy( const wxString& aFileName, bool aCreateProject, bool aHeadless ) = 0;

    /**
     * Read a netlist file and preload component footprints.
     */
    virtual bool ReadNetlistFromFile( const wxString& aFilename, NETLIST& aNetlist, REPORTER& aReporter ) = 0;

    /**
     * Create a netlist updater bound to this context's board.
     */
    virtual std::unique_ptr<BOARD_NETLIST_UPDATER> MakeNetlistUpdater() = 0;

    /**
     * Post-import board sync (nets, classes, DRC, ratsnest, new footprint placement).
     */
    virtual void OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater ) = 0;
};


std::shared_ptr<PCB_CONTEXT> CreatePcbFrameContext( PCB_EDIT_FRAME* aFrame );

#endif

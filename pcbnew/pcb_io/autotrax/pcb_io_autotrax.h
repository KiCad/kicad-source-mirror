/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Format interpretation derived from pcb-rnd src_plugins/io_autotrax:
 *   Copyright (C) 2016, 2017, 2018, 2020 Tibor 'Igor2' Palinkas
 *   Copyright (C) 2016, 2017 Erich S. Heinzle
 * Used under GPL v2-or-later.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef PCB_IO_AUTOTRAX_H_
#define PCB_IO_AUTOTRAX_H_

#include <map>

#include <layer_ids.h>
#include <math/util.h>
#include <math/vector2d.h>
#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>

#include "autotrax_model.h"

class BOARD_ITEM;
class FOOTPRINT;
class NETINFO_ITEM;


/**
 * Read-only importer for Protel Autotrax (.PCB, "PCB FILE 4") and Easytrax
 * (.PCB, "PCB FILE 5") layout files.
 *
 * Autotrax stores everything in mils with the Y axis pointing down. The plugin
 * parses the file into an intermediate model, then builds a BOARD, flipping Y
 * about the board bounding box so the result lands in KiCad's coordinate frame.
 *
 * The .PCB extension is shared with gEDA and others, so CanReadBoard() sniffs
 * the magic header line rather than trusting the extension.
 */
class PCB_IO_AUTOTRAX : public PCB_IO
{
public:
    PCB_IO_AUTOTRAX();
    ~PCB_IO_AUTOTRAX() override;

    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Protel Autotrax / Easytrax PCB files" ), { "PCB", "pcb" }, {}, true,
                                      /* aCanRead */ true, /* aCanWrite */ false );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override { return IO_BASE::IO_FILE_DESC( wxEmptyString, {} ); }

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

private:
    void buildBoard( const AUTOTRAX::BOARD_DATA& aData );

    void buildComponent( const AUTOTRAX::COMPONENT& aComp );

    void emitTrack( const AUTOTRAX::TRACK& aTrack, FOOTPRINT* aFootprint );
    void emitArc( const AUTOTRAX::ARC& aArc, FOOTPRINT* aFootprint );
    void emitVia( const AUTOTRAX::VIA& aVia, FOOTPRINT* aFootprint );
    void emitPad( const AUTOTRAX::PAD& aPad, FOOTPRINT* aFootprint );
    void emitFill( const AUTOTRAX::FILL& aFill, FOOTPRINT* aFootprint );
    void emitText( const AUTOTRAX::TEXT& aText, FOOTPRINT* aFootprint );

    /// Map an Autotrax layer number to a KiCad layer. Returns false for layers
    /// that have no KiCad equivalent or are intentionally dropped (0, 12).
    bool mapLayer( int aLayer, PCB_LAYER_ID& aResult ) const;

    NETINFO_ITEM* getNet( const wxString& aNetName );

    /// Parent for a primitive: the owning footprint, or the board for free items.
    BOARD_ITEM* parentOf( FOOTPRINT* aFootprint ) const;

    /// Attach an item to its footprint, or append it to the board.
    void addItem( BOARD_ITEM* aItem, FOOTPRINT* aFootprint );

    /// Convert a mil value to KiCad internal units (nm).
    static int toIU( double aMils ) { return KiROUND( aMils * 25400.0 ); }

    /// Convert an Autotrax point (mils, Y-down) to a board point (nm, Y-up). The
    /// flip uses the parsed Y extent so all coordinates stay positive.
    VECTOR2I toBoard( double aX, double aY ) const { return VECTOR2I( toIU( aX ), m_maxY - toIU( aY ) ); }

    int m_maxY = 0; ///< board Y extent in IU, used to flip the Y axis

    std::map<wxString, NETINFO_ITEM*> m_nets;
};

#endif // PCB_IO_AUTOTRAX_H_

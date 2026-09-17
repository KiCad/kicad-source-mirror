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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
class PCB_TEXT;


/**
 * Read-only importer for Protel Autotrax (.PCB, "PCB FILE 4") and Easytrax
 * (.PCB, "PCB FILE 5") layout files.
 *
 * Autotrax stores everything in mils with the Y axis pointing up. The plugin
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

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

protected:
    void loadBoard( const wxString& aFileName, BOARD& aBoard, bool aIsNewLoad,
                    const std::map<std::string, UTF8>* aProperties = nullptr, PROJECT* aProject = nullptr ) override;

private:
    void buildBoard( const AUTOTRAX::BOARD_DATA& aData );

    void buildComponent( const AUTOTRAX::COMPONENT& aComp );

    /// Attach each NETDEF "refdes-pad" node to the matching imported pads.
    void assignNets( const std::vector<AUTOTRAX::NET_NODE>& aNodes );

    void emitTrack( const AUTOTRAX::TRACK& aTrack, FOOTPRINT* aFootprint );
    void emitArc( const AUTOTRAX::ARC& aArc, FOOTPRINT* aFootprint );
    void emitVia( const AUTOTRAX::VIA& aVia, FOOTPRINT* aFootprint );
    void emitPad( const AUTOTRAX::PAD& aPad, FOOTPRINT* aFootprint );
    void emitFill( const AUTOTRAX::FILL& aFill, FOOTPRINT* aFootprint );
    void emitText( const AUTOTRAX::TEXT& aText, FOOTPRINT* aFootprint );

    /// Apply an Autotrax string record's placement, size, rotation and layer to @p aText.
    /// Returns false when the record's layer has no KiCad equivalent.
    bool applyText( const AUTOTRAX::TEXT& aText, PCB_TEXT* aTarget );

    /// Map an Autotrax layer number to a KiCad layer for an item owned by @p aFootprint, or a free
    /// item when it is null. Returns false for layers that have no KiCad equivalent (0).
    bool mapLayer( int aLayer, const FOOTPRINT* aFootprint, PCB_LAYER_ID& aResult ) const;

    NETINFO_ITEM* getNet( const wxString& aNetName );

    REPORTER& reporter() const;

    /// Report what the import dropped or could not represent.
    void reportGaps() const;

    /// Parent for a primitive: the owning footprint, or the board for free items.
    BOARD_ITEM* parentOf( FOOTPRINT* aFootprint ) const;

    /// Attach an item to its footprint, or append it to the board.
    void addItem( BOARD_ITEM* aItem, FOOTPRINT* aFootprint );

    /// Convert a mil value to KiCad internal units (nm).
    static int toIU( double aMils ) { return KiROUND( aMils * 25400.0 ); }

    /// Convert an Autotrax point (mils, Y-up) to a board point (nm, Y-down). The
    /// flip uses the parsed Y extent so all coordinates stay positive.
    VECTOR2I toBoard( double aX, double aY ) const { return VECTOR2I( toIU( aX ), m_maxY - toIU( aY ) ); }

    int m_maxY = 0; ///< board Y extent in IU, used to flip the Y axis

    int m_targetPads = 0;    ///< pads with a crosshair or moire target shape
    int m_offLayerPads = 0;  ///< pads on a layer KiCad cannot place them on
    int m_planePads = 0;     ///< pads asking for a ground or power plane connection
    int m_unmappedItems = 0; ///< items on a layer with no KiCad equivalent

    bool         m_isNewLoad = true;
    PCB_LAYER_ID m_keepoutLayer = User_1; ///< Edge_Cuts when the keepout outlines the board

    std::map<wxString, NETINFO_ITEM*> m_nets;

    std::multimap<wxString, FOOTPRINT*> m_footprintsByRef; ///< footprints created by this load
};

#endif // PCB_IO_AUTOTRAX_H_

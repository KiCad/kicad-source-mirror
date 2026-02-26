/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright Quilter
 * Copyright (C) 2019 Thomas Pointhuber <thomas.pointhuber@gmx.at>
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

#pragma once

#include <pcb_io/pcb_io.h>
#include <pcb_io/pcb_io_mgr.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>


class PCB_IO_ALLEGRO : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    // -----<PUBLIC PCB_IO API>--------------------------------------------------
    const IO_BASE::IO_FILE_DESC GetBoardFileDesc() const override
    {
        return IO_BASE::IO_FILE_DESC( _HKI( "Allegro PCB files" ), { "brd" } );
    }

    const IO_BASE::IO_FILE_DESC GetLibraryDesc() const override { return IO_BASE::IO_FILE_DESC( wxEmptyString, {} ); }

    bool CanReadBoard( const wxString& aFileName ) const override;
    bool CanReadLibrary( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe, const std::map<std::string, UTF8>* aProperties,
                      PROJECT* aProject ) override;

    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override { return 0; }

    // void FootprintEnumerate( wxArrayString& aFootprintNames, const wxString& aLibraryPath,
    //                          bool aBestEfforts, const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    // This parser should work for .dra, but we'll get there later
    // FOOTPRINT* FootprintLoad( const wxString& aLibraryPath, const wxString& aFootprintName,
    //                           bool              aKeepUUID = false,
    //                           const std::map<std::string, UTF8>* aProperties = nullptr ) override;

    //bool FootprintExists( const wxString& aLibraryPath, const wxString& aFootprintName, const PROPERTIES* aProperties = nullptr );

    bool IsLibraryWritable( const wxString& aLibraryPath ) override { return false; }

    // -----</PUBLIC PCB_IO API>-------------------------------------------------

    bool LoadBoardFromData( const uint8_t* aData, size_t aSize, BOARD& aBoard );

    PCB_IO_ALLEGRO();
};

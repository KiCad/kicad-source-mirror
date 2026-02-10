/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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

#pragma once

#include <pcb_io/pcb_io.h>
#include <pcb_io/common/plugin_common_layer_mapping.h>
#include <io/pads/pads_unit_converter.h>
#include "pads_layer_mapper.h"

#include <map>
#include <string>
#include <vector>

class BOARD;

namespace PADS_IO { class BINARY_PARSER; }

/**
 * PCB I/O plugin for importing PADS binary .pcb files.
 *
 * This plugin reads the native binary format used by PADS Layout, which is
 * distinct from the ASCII export format handled by PCB_IO_PADS. The binary
 * parser populates the same intermediate PADS_IO structs, and this wrapper
 * converts them to KiCad objects using the same patterns as the ASCII importer.
 */
class PCB_IO_PADS_BINARY : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    PCB_IO_PADS_BINARY();
    ~PCB_IO_PADS_BINARY() override;

    const IO_FILE_DESC GetBoardFileDesc() const override;
    const IO_FILE_DESC GetLibraryDesc() const override;
    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties, PROJECT* aProject ) override;

    std::map<wxString, PCB_LAYER_ID> DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

private:
    int          scaleSize( double aVal ) const;
    int          scaleCoord( double aVal, bool aIsX ) const;
    PCB_LAYER_ID getMappedLayer( int aPadsLayer ) const;
    void         ensureNet( const std::string& aNetName );

    void loadBoardSetup();
    void loadNets();
    void loadFootprints();
    void loadBoardOutline();
    void loadTracksAndVias();
    void loadTexts();
    void loadZones();
    void reportStatistics();
    void clearLoadingState();

    // Persistent state
    std::map<wxString, PCB_LAYER_ID>   m_layerMap;

    // Loading state
    BOARD*                              m_loadBoard = nullptr;
    const PADS_IO::BINARY_PARSER*      m_parser = nullptr;
    PADS_UNIT_CONVERTER                 m_unitConverter;
    PADS_LAYER_MAPPER                   m_layerMapper;
    std::vector<PADS_LAYER_INFO>        m_layerInfos;
    double                              m_scaleFactor = 0.0;
    double                              m_originX = 0.0;
    double                              m_originY = 0.0;
    std::map<std::string, std::string>  m_pinToNetMap;
};

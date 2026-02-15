/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

namespace PADS_IO { class PARSER; }

class PCB_IO_PADS : public PCB_IO, public LAYER_MAPPABLE_PLUGIN
{
public:
    PCB_IO_PADS();
    ~PCB_IO_PADS() override;

    const IO_FILE_DESC GetBoardFileDesc() const override;
    const IO_FILE_DESC GetLibraryDesc() const override;
    long long GetLibraryTimestamp( const wxString& aLibraryPath ) const override;

    bool CanReadBoard( const wxString& aFileName ) const override;

    BOARD* LoadBoard( const wxString& aFileName, BOARD* aAppendToMe,
                      const std::map<std::string, UTF8>* aProperties, PROJECT* aProject ) override;

    /**
     * Return the automapped layers.
     *
     * The callback needs to have the context of the current board so it can
     * correctly determine copper layer mapping. Thus, it is not static and is
     * expected to be bound to an instance of PCB_IO_PADS.
     *
     * @param aInputLayerDescriptionVector the input layer descriptions from the PADS file
     * @return Auto-mapped layers
     */
    std::map<wxString, PCB_LAYER_ID> DefaultLayerMappingCallback(
            const std::vector<INPUT_LAYER_DESC>& aInputLayerDescriptionVector );

private:
    // LoadBoard helper methods -- each handles one logical section of the import
    int         scaleSize( double aVal ) const;
    int         scaleCoord( double aVal, bool aIsX ) const;
    PCB_LAYER_ID getMappedLayer( int aPadsLayer ) const;
    void        ensureNet( const std::string& aNetName );

    void        loadBoardSetup();
    void        loadNets();
    void        loadFootprints();
    void        loadReuseBlockGroups();
    void        loadTestPoints();
    void        loadTexts();
    void        loadTracksAndVias();
    void        loadCopperShapes();
    void        loadClusterGroups();
    void        loadZones();
    void        loadBoardOutline();
    void        loadDimensions();
    void        loadKeepouts();
    void        loadGraphicLines();
    void        generateDrcRules( const wxString& aFileName );
    void        reportStatistics();
    void        clearLoadingState();

    // Persistent state
    std::map<wxString, PCB_LAYER_ID>            m_layer_map;      ///< PADS layer names to KiCad layers

    // Loading state -- valid only during LoadBoard, cleared by clearLoadingState()
    BOARD*                                      m_loadBoard = nullptr;
    const PADS_IO::PARSER*                      m_parser = nullptr;
    PADS_UNIT_CONVERTER                         m_unitConverter;
    PADS_LAYER_MAPPER                           m_layerMapper;
    std::vector<PADS_LAYER_INFO>                m_layerInfos;
    double                                      m_scaleFactor = 0.0;
    double                                      m_originX = 0.0;
    double                                      m_originY = 0.0;
    std::map<std::string, std::string>          m_pinToNetMap;
    std::map<std::string, std::string>          m_partToBlockMap;
    int                                         m_testPointIndex = 1;
    int                                         m_minObjectSize = 1000;
};
